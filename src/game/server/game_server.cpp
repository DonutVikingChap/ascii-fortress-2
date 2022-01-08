#include "game_server.hpp"

#include "../../console/command.hpp"                       // cmd::...
#include "../../console/command_utilities.hpp"             // cmd::...
#include "../../console/commands/file_commands.hpp"        // data_dir, data_subdir_cfg, data_subdir_maps
#include "../../console/commands/game_commands.hpp"        // game_version, game_url, cmd_disconnect, cmd_quit
#include "../../console/commands/game_server_commands.hpp" // sv_...
#include "../../console/commands/meta_client_commands.hpp" // meta_address, meta_port
#include "../../console/commands/process_commands.hpp"     // cmd_import, cmd_file
#include "../../console/con_command.hpp"                   // GET_COMMAND
#include "../../console/process.hpp"                       // Process
#include "../../debug.hpp"                                 // Msg, DEBUG_MSG, DEBUG_MSG_INDENT, INFO_MSG, INFO_MSG_INDENT
#include "../../network/byte_stream.hpp"                   // net::ByteOutputStream
#include "../../network/delta.hpp"                         // deltaCompress
#include "../../utilities/algorithm.hpp" // util::erase, util::eraseIf, util::transform, util::collect, util::anyOf, util::enumerate, util::findIf, util::contains, util::copy, util::countIf, util::replace
#include "../../utilities/file.hpp"      // util::readFile
#include "../../utilities/string.hpp"    // util::join, util::icontains, util::iequals, util::contains, util::toString
#include "../../utilities/time.hpp"      // util::getLocalTimeStr
#include "../data/actions.hpp"           // Actions, Action
#include "../data/direction.hpp"         // Direction
#include "../game.hpp"                   // Game
#include "../meta/meta_client_messages.hpp" // MetaClientOutputMessages, msg::meta::cl::out::...
#include "../meta/meta_server_messages.hpp" // MetaServerOutputMessages, msg::meta::sv::out::...

#include <algorithm>    // std::max
#include <array>        // std::array
#include <chrono>       // std::chrono::...
#include <cmath>        // std::ceil
#include <filesystem>   // std::filesystem::...
#include <fmt/core.h>   // fmt::format
#include <iterator>     // std::prev
#include <ratio>        // std::milli
#include <system_error> // std::error_code
#include <tuple>        // std::tie

auto GameServer::getConfigHeader() -> std::string {
	return fmt::format(
		"// This file is auto-generated every time your server is shut down, and loaded every time it is "
		"started.\n"
		"// Do not modify this file manually. Use the autoexec file instead.\n"
		"// Last generated {}.",
		util::getLocalTimeStr("%c"));
}

auto GameServer::updateHatDropWeights() -> void {
	static constexpr auto isValidHat = [](const auto& hat) {
		return hat != Hat::none();
	};
	static constexpr auto getHatDropWeight = [](const auto& hat) {
		return static_cast<double>(hat.getDropWeight());
	};
	const auto hatDropWeights = Hat::getAll() | util::filter(isValidHat) | util::transform(getHatDropWeight) | util::collect<std::vector<double>>();
	GameServer::hatDistribution() = HatDistribution{hatDropWeights.begin(), hatDropWeights.end()};
}

auto GameServer::replicate(const ConVar& cvar) -> void {
	GameServer::modifiedCvars().insert(&cvar);
}

GameServer::GameServer(Game& game, VirtualMachine& vm, std::shared_ptr<Environment> env, std::shared_ptr<Process> process)
	: m_game(game)
	, m_vm(vm)
	, m_env(std::make_shared<Environment>(std::move(env)))
	, m_process(std::move(process))
	, m_world(m_game.map(), *this)
	, m_currentClient(m_clients.end()) {
	assert(m_process);
	this->updateTimeout();
	this->updateThrottle();
	this->updateSpamLimit();
	this->updateTickrate();
	this->updateBotTickrate();
	this->updateConfigAutoSaveInterval();
	this->updateResourceUploadInterval();
	this->updateAllowResourceDownload();
	GameServer::updateHatDropWeights();
	Bot::updateHealthProbability();
	Bot::updateClassWeights();
	Bot::updateGoalWeights();
}

GameServer::~GameServer() {
	GameServer::modifiedCvars().clear();
}

auto GameServer::init() -> bool {
	INFO_MSG(Msg::SERVER, "Game server: Initializing...");

	// Initialize crypto library.
	if (!crypto::init()) {
		m_game.error("Failed to initialize crypto library!");
		return false;
	}

	// Initialize inventory server.
	if (!this->initInventoryServer()) {
		m_game.error("Failed to initialize inventory server!");
		return false;
	}

	// Initialize remote console server.
	if (!this->initRconServer()) {
		m_game.error("Failed to initialize remote console server!");
		return false;
	}

	auto ec = std::error_code{};

	// Bind socket.
	m_socket.bind(net::IpEndpoint{net::IpAddress::any(), static_cast<net::PortNumber>(sv_port)}, ec);
	if (ec) {
		m_game.error(fmt::format("Failed to bind server socket to port \"{}\": {}", sv_port, ec.message()));
		return false;
	}

	// Execute server config script.
	if (m_game.consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{sv_config_file}}).status ==
	    cmd::Status::ERROR_MSG) {
		m_game.error("Server config failed.");
		return false;
	}

	// Execute server autoexec script.
	if (m_game.consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{sv_autoexec_file}}).status ==
	    cmd::Status::ERROR_MSG) {
		m_game.error("Server autoexec failed.");
		return false;
	}

	// Load map.
	if (!this->loadMap()) {
		return false;
	}

	// Connect to meta server.
	if (sv_meta_submit) {
		this->connectToMetaServer();
	}

	// Clear modified cvars.
	GameServer::modifiedCvars().clear();

	INFO_MSG(Msg::SERVER,
	         "Game server: Started on \"{}:{}\".",
	         std::string{net::IpAddress::getLocalAddress(ec)},
	         m_socket.getLocalEndpoint(ec).getPort());
	m_game.println(fmt::format("Server started. Use \"{}\" or \"{}\" to stop.", GET_COMMAND(disconnect).getName(), GET_COMMAND(quit).getName()));
	return true;
}

auto GameServer::shutDown() -> void {
	INFO_MSG(Msg::SERVER, "Game server: Shutting down.");

	// Reset entity state.
	m_world.reset();

	// Unload map.
	m_game.map().unLoad();

	// Save server config.
	m_game.awaitConsoleCommand(GET_COMMAND(sv_writeconfig));
}

auto GameServer::stop(std::string_view message) -> bool {
	if (!m_stopping) {
		INFO_MSG_INDENT(Msg::SERVER, "Game server: Shutting down. Message: \"{}\".", message) {
			m_stopping = true;
			if (message.empty()) {
				m_game.println("Server shutting down.");
				message = "Server shutting down.";
			} else {
				m_game.println(fmt::format("Server shutting down. Message: {}", message));
			}
			for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
				this->disconnectClient(it, message);
			}
		}
		return true;
	}
	INFO_MSG(Msg::SERVER, "Game server: Tried to stop when already stopping. Message: \"{}\".", message);
	return false;
}

auto GameServer::update(float deltaTime) -> bool {
	DEBUG_MSG_INDENT(Msg::SERVER_TICK | Msg::CONNECTION_DETAILED, "SERVER @ {} ms", deltaTime * 1000.0f) {
		if (m_stopping) {
			// Wait for all connections to close.
			if (m_clients.empty()) {
				return false;
			}
		}
		this->updateConfigAutoSave(deltaTime);
		this->receivePackets();
		this->updateConnections();
		this->updateMetaServerConnection(deltaTime);
		this->updateRconServer(deltaTime);
		this->updateTicks(deltaTime);
		this->updateProcess();
	}
	return true;
}

auto GameServer::updateTimeout() -> void {
	const auto timeout = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(sv_timeout)});
	for (auto& client : m_clients) {
		client->connection.setTimeout(timeout);
	}
}

auto GameServer::updateThrottle() -> void {
	for (auto& client : m_clients) {
		client->connection.setThrottleMaxSendBufferSize(sv_throttle_limit);
		client->connection.setThrottleMaxPeriod(sv_throttle_max_period);
	}
}

auto GameServer::updateSpamLimit() -> void {
	m_spamInterval = 1.0f / static_cast<float>(sv_spam_limit);
	for (auto& client : m_clients) {
		client->spamCounter = 0;
	}
	m_spamTimer.reset();
}

auto GameServer::updateTickrate() -> void {
	m_tickrate = static_cast<Tickrate>(sv_tickrate);
	m_tickInterval = 1.0f / static_cast<float>(m_tickrate);
	m_tickTimer.reset();
}

auto GameServer::updateBotTickrate() -> void {
	m_botTickInterval = 1.0f / static_cast<float>(sv_bot_tickrate);
	m_botTickTimer.reset();
}

auto GameServer::updateConfigAutoSaveInterval() -> void {
	m_configAutoSaveInterval = static_cast<float>(sv_config_auto_save_interval) * 60.0f;
	m_configAutoSaveTimer.reset();
}

auto GameServer::updateResourceUploadInterval() -> void {
	m_resourceUploadInterval = static_cast<float>(sv_resource_upload_chunk_size) / sv_resource_upload_rate;
	for (auto& client : m_clients) {
		client->resourceUploadTimer.reset();
	}
}

auto GameServer::updateAllowResourceDownload() -> void {
	for (auto& kv : m_resources) {
		kv.second.canDownload = sv_allow_resource_download;
	}
	for (auto& resource : m_resourceInfo) {
		resource.canDownload = sv_allow_resource_download;
	}
}

auto GameServer::updateMetaSubmit() -> void {
	if (sv_meta_submit) {
		if (!m_clients.contains<CLIENT_USERNAME>(std::string{USERNAME_META_SERVER})) {
			this->connectToMetaServer();
		}
	} else {
		if (const auto it = m_clients.find<CLIENT_ENDPOINT>(m_metaServerEndpoint); it != m_clients.end()) {
			this->disconnectClient(it, "Meta submit disabled.");
		}
	}
}

auto GameServer::changeLevel() -> void {
	INFO_MSG(Msg::SERVER, "Game server: Changing map to \"{}\"...", sv_map);

	this->writeServerChatMessage(fmt::format("Changing map to \"{}\"...", sv_map));
	if (!this->loadMap()) {
		this->stop("Failed to load map!");
		return;
	}

	for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
		auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
		if (endpoint != m_metaServerEndpoint) {
			if (!this->writeServerInfo(client)) {
				this->disconnectClient(it, "Failed to write new server info.");
			}
		}
	}
}

auto GameServer::changeLevelToNext() -> void {
	if (!sv_nextlevel.empty()) {
		if (auto result = sv_map.set(sv_nextlevel, m_game, this, nullptr, nullptr, nullptr); result.status == cmd::Status::ERROR_MSG) {
			m_game.warning(std::move(result.value));
		}
	}
	this->changeLevel();
}

auto GameServer::hasPlayers() const -> bool {
	return m_world.getPlayerCount() > m_bots.size();
}

auto GameServer::getStatusString() const -> std::string {
	static constexpr auto formatClient = [](const auto& elem) {
		const auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = elem;
		const auto pingMilliseconds =
			std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(client.connection.getLatestMeasuredPingDuration()).count();
		return fmt::format(
			"{}. \"{}\"\n"
			"  Username: \"{}\"\n"
			"  Latency: {} ms\n"
			"  Packets sent: {}\n"
			"  Packets received: {}\n"
			"  Reliable packets written: {}\n"
			"  Reliable packets received: {}\n"
			"  Reliable packets received out of order: {}\n"
			"  Send rate throttled: {}\n"
			"  Packet send errors: {}\n"
			"  Invalid message types received: {}\n"
			"  Invalid message payloads received: {}\n"
			"  Invalid packet headers received: {}",
			playerId,
			std::string{endpoint},
			(username.empty()) ? USERNAME_UNCONNECTED : std::string_view{username},
			pingMilliseconds,
			client.connection.getStats().packetsSent,
			client.connection.getStats().packetsReceived,
			client.connection.getStats().reliablePacketsWritten,
			client.connection.getStats().reliablePacketsReceived,
			client.connection.getStats().reliablePacketsReceivedOutOfOrder,
			client.connection.getStats().sendRateThrottleCount,
			client.connection.getStats().packetSendErrorCount,
			client.connection.getStats().invalidMessageTypeCount,
			client.connection.getStats().invalidMessagePayloadCount,
			client.connection.getStats().invalidPacketHeaderCount);
	};

	auto ec = std::error_code{};
	return fmt::format(
		"=== SERVER STATUS ===\n"
		"Local address: \"{}:{}\"\n"
		"Hostname: \"{}\"\n"
		"Tickrate: {} Hz\n"
		"Tick count: {}\n"
		"Map: \"{}\"\n"
		"Map time: {} s\n"
		"Players: {}/{} ({} bots)\n"
		"Clients:\n"
		"{}\n"
		"=====================",
		std::string{net::IpAddress::getLocalAddress(ec)},
		m_socket.getLocalEndpoint(ec).getPort(),
		sv_hostname,
		m_tickrate,
		m_world.getTickCount(),
		m_game.map().getName(),
		m_world.getMapTime(),
		m_world.getPlayerCount(),
		sv_playerlimit,
		m_bots.size(),
		m_clients | util::transform(formatClient) | util::join("\n\n"));
}

auto GameServer::kickPlayer(std::string_view ipOrName) -> bool {
	if (const auto it = this->findClient(ipOrName); it != m_clients.end()) {
		this->disconnectClient(it, "You were kicked from the server.");
		return true;
	}
	return false;
}

auto GameServer::banPlayer(std::string_view ipOrName, std::optional<std::string> playerUsername) -> bool {
	if (const auto it = this->findClient(ipOrName); it != m_clients.end()) {
		auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
		m_bannedPlayers.insert_or_assign(address, BannedPlayer{(playerUsername) ? *playerUsername : username});
		this->disconnectClient(it, "You were banned from the server.");
		return true;
	}

	auto ec = std::error_code{};
	if (const auto ip = net::IpAddress::parse(ipOrName, ec); !ec && playerUsername) {
		m_bannedPlayers.insert_or_assign(ip, BannedPlayer{*playerUsername});
		return true;
	}
	return false;
}

auto GameServer::unbanPlayer(net::IpAddress ip) -> bool {
	return m_bannedPlayers.erase(ip) > 0;
}

auto GameServer::addBot(std::string name, Team team, PlayerClass playerClass) -> bool {
	static constexpr auto BOT_TEAMS = std::array{Team::blue(), Team::red()};

	name = this->findValidUsername(fmt::format("BOT {}", name));
	if (const auto playerId = m_world.createPlayer(Vec2{m_game.map().getWidth() / 2, m_game.map().getHeight() / 2}, name); playerId != PLAYER_ID_UNCONNECTED) {
		const auto& bot = m_bots.emplace_back(m_game.map(), m_vm.rng(), m_xCoordinateDistribution, m_yCoordinateDistribution, playerId, std::move(name));
		const auto validTeam = (team != Team::none() && team != Team::spectators()) ? team : BOT_TEAMS[m_currentBotIndex++ % BOT_TEAMS.size()];
		const auto validClass = (playerClass != PlayerClass::none() && playerClass != PlayerClass::spectator()) ? playerClass : bot.getRandomClass();
		this->callIfDefined(Script::command({"on_player_join", cmd::formatPlayerId(playerId)}));
		return m_world.playerTeamSelect(playerId, validTeam, validClass);
	}
	return false;
}

auto GameServer::kickBot(std::string_view name) -> bool {
	bool found = false;
	util::eraseIf(m_bots, [&](const auto& bot) {
		if (util::iequals(bot.getName(), name)) {
			this->writeServerChatMessage(fmt::format("Kicking bot {}.", bot.getName()));
			m_world.deletePlayer(bot.getId());
			found = true;
			return true;
		}
		return false;
	});
	return found;
}

auto GameServer::kickAllBots() -> void {
	this->writeServerChatMessage("Kicking all bots.");
	for (auto& bot : m_bots) {
		m_world.deletePlayer(bot.getId());
	}
	m_bots.clear();
}

auto GameServer::freezeBots() -> void {
	for (auto& bot : m_bots) {
		if (auto&& player = m_world.findPlayer(bot.getId())) {
			player.setActions(Action::NONE);
		}
	}
}

auto GameServer::getBannedPlayers() const -> const BannedPlayers& {
	return m_bannedPlayers;
}

auto GameServer::getConnectedClientIps() const -> std::vector<net::IpEndpoint> {
	static constexpr auto isConnected = [](const auto& elem) {
		return elem.template get<CLIENT_PLAYER_ID>() != PLAYER_ID_UNCONNECTED;
	};
	static constexpr auto getEndpoint = [](const auto& elem) {
		return elem.template get<CLIENT_ENDPOINT>();
	};
	return m_clients | util::filter(isConnected) | util::transform(getEndpoint) | util::collect<std::vector<net::IpEndpoint>>();
}

auto GameServer::getBotNames() const -> std::vector<std::string> {
	static constexpr auto getBotName = [](const auto& bot) {
		return bot.getName();
	};
	return m_bots | util::transform(getBotName) | util::collect<std::vector<std::string>>();
}

auto GameServer::getPlayerIdByIp(net::IpEndpoint endpoint) const -> std::optional<PlayerId> {
	if (const auto it = this->findClientByIp(endpoint); it != m_clients.end()) {
		return it->template get<CLIENT_PLAYER_ID>();
	}
	return std::nullopt;
}

auto GameServer::getPlayerInventoryId(PlayerId id) const -> std::optional<InventoryId> {
	if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(id); it != m_clients.end()) {
		return it->get<CLIENT_INVENTORY_ID>();
	}
	return std::nullopt;
}

auto GameServer::getPlayerIp(PlayerId id) const -> std::optional<net::IpEndpoint> {
	if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(id); it != m_clients.end()) {
		return it->get<CLIENT_ENDPOINT>();
	}
	return std::nullopt;
}

auto GameServer::awardPlayerPoints(PlayerId id, std::string_view name, Score points) -> bool {
	if (const auto inventoryId = this->getPlayerInventoryId(id)) {
		if (auto* const score = this->inventoryPoints(*inventoryId)) {
			*score += points;
			if (points > 0) {
				if (auto* const level = this->inventoryLevel(*inventoryId)) {
					const auto levelInterval = static_cast<Score>(sv_score_level_interval);
					const auto newLevel = static_cast<Score>(*score / levelInterval);
					while (*level < newLevel) {
						++*level;
						this->writeServerChatMessage(fmt::format("{} leveled up to level {}!", name, *level));
						if (const auto hat = this->generateHat(); hat != Hat::none() && this->giveInventoryHat(*inventoryId, hat)) {
							this->writeServerChatMessage(fmt::format("{} has found: {}", name, hat.getName()));
						}
						this->playPlayerInterfaceSound(SoundId::achievement(), id);
					}
				}
			}
			return true;
		}
	}
	return false;
}

auto GameServer::rockTheVote(net::IpEndpoint endpoint) -> bool {
	if (const auto it = this->findClientByIp(endpoint); it != m_clients.end()) {
		auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
		if (m_world.getMapTime() < sv_rtv_delay) {
			this->writeServerChatMessage(fmt::format("{} wants to rock the vote. (Please wait {} seconds.)",
			                                         username,
			                                         static_cast<int>(std::ceil(sv_rtv_delay - m_world.getMapTime()))));
			return true;
		}

		client.wantsToRtv = true;

		auto rtvCount = std::size_t{0};
		auto playerCount = std::size_t{0};
		for (const auto& [otherClient, otherEndpoint, otherAddress, otherUsername, otherPlayerId, otherInventoryId, otherRconToken] : m_clients) {
			if (otherPlayerId != PLAYER_ID_UNCONNECTED) {
				++playerCount;
				if (otherClient.wantsToRtv) {
					++rtvCount;
				}
			}
		}

		const auto playersNeeded = static_cast<std::size_t>(std::ceil(static_cast<float>(playerCount) * sv_rtv_needed));
		this->writeServerChatMessage(fmt::format("{} wants to rock the vote. ({}/{})", username, rtvCount, playersNeeded));
		if (rtvCount >= playersNeeded) {
			this->changeLevelToNext();
		}
		return true;
	}
	return false;
}

auto GameServer::isPlayerBot(PlayerId id) const -> bool {
	return util::anyOf(m_bots, [&](const auto& bot) { return bot.getId() == id; });
}

auto GameServer::world() noexcept -> World& {
	return m_world;
}

auto GameServer::world() const noexcept -> const World& {
	return m_world;
}

auto GameServer::resetClients() -> void {
	for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
		this->resetClient(it);
	}
}

auto GameServer::resetEnvironment() -> void {
	assert(m_env);
	m_env->reset();
}

auto GameServer::writeCommandOutput(net::IpEndpoint endpoint, std::string_view message) -> bool {
	if (const auto it = this->findClientByIp(endpoint); it != m_clients.end()) {
		this->writeCommandOutput(**it, message);
		return true;
	}
	return false;
}

auto GameServer::writeCommandError(net::IpEndpoint endpoint, std::string_view message) -> bool {
	if (const auto it = this->findClientByIp(endpoint); it != m_clients.end()) {
		this->writeCommandError(**it, message);
		return true;
	}
	return false;
}

auto GameServer::writeServerChatMessage(std::string_view message) -> void {
	INFO_MSG(Msg::CHAT, "[SERVER]: {}", message);
	const auto msg = msg::cl::out::ServerChatMessage{{}, message};
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (playerId != PLAYER_ID_UNCONNECTED) {
			if (!client.write(msg)) {
				INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write server chat message to \"{}\".", std::string{endpoint});
			}
		}
	}
}

auto GameServer::writeServerChatMessage(std::string_view message, Team team) -> void {
	INFO_MSG(Msg::CHAT, "[SERVER to team {}]: {}", team.getName(), message);
	const auto msg = msg::cl::out::ServerChatMessage{{}, message};
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (const auto& player = m_world.findPlayer(playerId)) {
			if (player.getTeam() == team) {
				if (!client.write(msg)) {
					INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT,
					         "Game server: Failed to write server team chat message to \"{}\".",
					         std::string{endpoint});
				}
			}
		}
	}
}

auto GameServer::writeServerChatMessagePersonal(std::string_view message, PlayerId playerId) -> bool {
	if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(playerId); it != m_clients.end()) {
		auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
		if (const auto& player = m_world.findPlayer(playerId)) {
			INFO_MSG(Msg::CHAT, "[SERVER to player {}]: {}", player.getName(), message);
			if (!client.write(msg::cl::out::ServerChatMessage{{}, message})) {
				INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT,
				         "Game server: Failed to write personal server chat message to \"{}\".",
				         std::string{endpoint});
				return false;
			}
			return true;
		}
	}
	return false;
}

auto GameServer::writeServerEventMessage(std::string_view message) -> void {
	INFO_MSG(Msg::CHAT, "[SERVER Event]: {}", message);
	const auto msg = msg::cl::out::ServerEventMessage{{}, message};
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (playerId != PLAYER_ID_UNCONNECTED) {
			if (!client.write(msg)) {
				INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT,
				         "Game server: Failed to write server event message to \"{}\".",
				         std::string{endpoint});
			}
		}
	}
}

auto GameServer::writeServerEventMessage(std::string_view message, Team team) -> void {
	INFO_MSG(Msg::CHAT, "[SERVER Event to team {}]: {}", team.getName(), message);
	const auto msg = msg::cl::out::ServerEventMessage{{}, message};
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (const auto& player = m_world.findPlayer(playerId)) {
			if (player.getTeam() == team) {
				if (!client.write(msg)) {
					INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT,
					         "Game server: Failed to write team server event message to \"{}\".",
					         std::string{endpoint});
				}
			}
		}
	}
}

auto GameServer::writeServerEventMessage(std::string_view message, util::Span<const PlayerId> relevantPlayerIds) -> void {
	INFO_MSG(Msg::CHAT, "[SERVER Event]: {}", message);
	const auto msg = msg::cl::out::ServerEventMessage{{}, message};
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (playerId != PLAYER_ID_UNCONNECTED) {
			if (util::contains(relevantPlayerIds, playerId)) {
				if (!client.write(msg::cl::out::ServerEventMessagePersonal{{}, msg.message})) {
					INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT,
					         "Game server: Failed to write team server event message to \"{}\".",
					         std::string{endpoint});
				}
			} else {
				if (!client.write(msg)) {
					INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT,
					         "Game server: Failed to write team server event message to \"{}\".",
					         std::string{endpoint});
				}
			}
		}
	}
}

auto GameServer::writeServerEventMessagePersonal(std::string_view message, PlayerId playerId) -> bool {
	if (playerId != PLAYER_ID_UNCONNECTED) {
		if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(playerId); it != m_clients.end()) {
			auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
			if (const auto& player = m_world.findPlayer(playerId)) {
				INFO_MSG(Msg::CHAT, "[SERVER Event to player {}]: {}", player.getName(), message);
				if (client.write(msg::cl::out::ServerEventMessagePersonal{{}, message})) {
					return true;
				}
				INFO_MSG(Msg::CHAT | Msg::SERVER | Msg::CONNECTION_EVENT,
				         "Game server: Failed to write personal server event message to \"{}\".",
				         std::string{endpoint});
			}
		}
	}
	return false;
}

auto GameServer::writePlayerTeamSelected(Team oldTeam, Team newTeam, PlayerId playerId) -> void {
	if (playerId != PLAYER_ID_UNCONNECTED) {
		if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(playerId); it != m_clients.end()) {
			auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
			if (!client.write(msg::cl::out::PlayerTeamSelected{{}, oldTeam, newTeam})) {
				INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write team selected message to \"{}\".", std::string{endpoint});
			}
		}
	}
}

auto GameServer::writePlayerClassSelected(PlayerClass oldPlayerClass, PlayerClass newPlayerClass, PlayerId playerId) -> void {
	if (playerId != PLAYER_ID_UNCONNECTED) {
		if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(playerId); it != m_clients.end()) {
			auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
			if (!client.write(msg::cl::out::PlayerClassSelected{{}, oldPlayerClass, newPlayerClass})) {
				INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write player class selected message to \"{}\".", std::string{endpoint});
			}
		}
	}
}

auto GameServer::writeHitConfirmed(Health damage, PlayerId playerId) -> void {
	if (playerId != PLAYER_ID_UNCONNECTED) {
		if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(playerId); it != m_clients.end()) {
			auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
			if (!client.write(msg::cl::out::HitConfirmed{{}, damage})) {
				INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write hit confirmed message to \"{}\".", std::string{endpoint});
			}
		}
	}
}

auto GameServer::playWorldSound(SoundId soundId, Vec2 position) -> void {
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (playerId != PLAYER_ID_UNCONNECTED) {
			if (!client.write(msg::cl::out::PlaySoundPositionalUnreliable{{}, soundId, position})) {
				INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write positional world sound message to \"{}\".", std::string{endpoint});
			}
		}
	}
}

auto GameServer::playWorldSound(SoundId soundId, Vec2 position, PlayerId source) -> void {
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (playerId != PLAYER_ID_UNCONNECTED) {
			if (playerId == source) {
				if (!client.write(msg::cl::out::PlaySoundUnreliable{{}, soundId})) {
					INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write world sound message to \"{}\".", std::string{endpoint});
				}
			} else {
				if (!client.write(msg::cl::out::PlaySoundPositionalUnreliable{{}, soundId, position})) {
					INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT,
					         "Game server: Failed to write positional world sound message to \"{}\".",
					         std::string{endpoint});
				}
			}
		}
	}
}

auto GameServer::playTeamSound(SoundId soundId, Team team) -> void {
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (const auto& player = m_world.findPlayer(playerId)) {
			if (player.getTeam() == team) {
				if (!client.write(msg::cl::out::PlaySoundReliable{{}, soundId})) {
					INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write team sound message to \"{}\".", std::string{endpoint});
				}
			}
		}
	}
}

auto GameServer::playTeamSound(SoundId correctTeamId, SoundId otherTeamId, Team team) -> void {
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (const auto& player = m_world.findPlayer(playerId)) {
			if (player.getTeam() == team) {
				if (!client.write(msg::cl::out::PlaySoundReliable{{}, correctTeamId})) {
					INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write team sound message to \"{}\".", std::string{endpoint});
				}
			} else {
				if (!client.write(msg::cl::out::PlaySoundReliable{{}, otherTeamId})) {
					INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write team sound message to \"{}\".", std::string{endpoint});
				}
			}
		}
	}
}

auto GameServer::playPlayerInterfaceSound(SoundId soundId, PlayerId playerId) -> void {
	if (playerId != PLAYER_ID_UNCONNECTED) {
		if (const auto it = m_clients.find<CLIENT_PLAYER_ID>(playerId); it != m_clients.end()) {
			auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;
			if (!client.write(msg::cl::out::PlaySoundReliable{{}, soundId})) {
				INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write player interface sound message to \"{}\".", std::string{endpoint});
			}
		}
	}
}

auto GameServer::playGameSound(SoundId soundId) -> void {
	for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
		if (playerId != PLAYER_ID_UNCONNECTED) {
			if (!client.write(msg::cl::out::PlaySoundReliable{{}, soundId})) {
				INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write game sound message to \"{}\".", std::string{endpoint});
			}
		}
	}
}

auto GameServer::setObject(std::string name, Environment::Object object) -> void {
	assert(m_env);
	m_env->objects.insert_or_assign(std::move(name), std::move(object));
}

auto GameServer::deleteObject(const std::string& name) -> void {
	assert(m_env);
	m_env->objects.erase(name);
}

auto GameServer::callIfDefined(Script::Command command) -> void {
	assert(m_env);
	assert(!command.empty());
	if (m_process->defined(m_env, command.front().value)) {
		if (const auto frame = m_process->call(m_env, std::move(command))) {
			m_vm.output(frame->run(m_game, this, nullptr, nullptr, nullptr));
		} else {
			m_vm.outputError("Stack overflow.");
		}
	}
}

auto GameServer::callScript(Script script) -> void {
	assert(m_env);
	if (const auto frame = m_process->call(m_env, std::move(script))) {
		m_vm.output(frame->run(m_game, this, nullptr, nullptr, nullptr));
	} else {
		m_vm.outputError("Stack overflow.");
	}
}

auto GameServer::modifiedCvars() -> std::unordered_set<const ConVar*>& {
	static auto cvars = std::unordered_set<const ConVar*>{};
	return cvars;
}

auto GameServer::handleMessage(net::msg::in::Connect&&) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	INFO_MSG(Msg::SERVER, "Game server: Client \"{}\" connected.", std::string{endpoint});

	--m_connectingClients;
	client.connecting = false;

	if (endpoint == m_metaServerEndpoint) {
		if (sv_meta_submit) {
			if (!client.connection.write<MetaServerOutputMessages>(msg::meta::sv::out::Heartbeat{{}})) {
				this->disconnectClient(m_currentClient, "Failed to write initial heartbeat.");
			}
		} else {
			this->disconnectClient(m_currentClient, "Meta submit disabled.");
		}
	}
}

auto GameServer::handleMessage(msg::sv::in::ServerInfoRequest&&) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (!this->writeServerInfo(client)) {
		this->disconnectClient(m_currentClient, "Failed to write write server info.");
	}
}

auto GameServer::handleMessage(msg::sv::in::JoinRequest&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	if (m_stopping) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	client.updateInterval = (msg.updateRate > 0) ? 1.0f / static_cast<float>(msg.updateRate) : 0.0f;
	client.updateTimer.reset();
	m_clients.set<CLIENT_USERNAME>(m_currentClient, this->findValidUsername(msg.username));

	// Determine if we should allow this client to connect or not.
	auto ec = std::error_code{};
	if (const auto maxPlayersPerIp = static_cast<std::size_t>(sv_max_players_per_ip);
	    maxPlayersPerIp != 0 && !address.isLoopback() && !address.isPrivate() && address != net::IpAddress::getLocalAddress(ec) &&
	    this->countPlayersWithIp(address) >= maxPlayersPerIp) {
		this->disconnectClient(
			m_currentClient,
			fmt::format("The server does not allow more than {} player{} from the same IP address.", maxPlayersPerIp, (maxPlayersPerIp == 1) ? "" : "s"));
		return;
	}
	const auto requiredGameVersion = net::sanitizeMessage(msg.gameVersion);
	if (requiredGameVersion < game_version) {
		this->disconnectClient(m_currentClient, fmt::format("This server is running a newer version ({}). Download at: {}", game_version, game_url));
		return;
	}
	if (requiredGameVersion > game_version) {
		this->disconnectClient(m_currentClient, fmt::format("This server is running an older version ({}).", game_version));
		return;
	}
	if (!sv_password.verifyHash(msg.passwordKey)) {
		this->disconnectClient(m_currentClient, "Incorrect password.");
		return;
	}
	if (m_world.getPlayerCount() >= static_cast<std::size_t>(sv_playerlimit)) {
		this->disconnectClient(m_currentClient, "Server is full.");
		return;
	}
	if (msg.mapHash != m_game.map().getHash()) {
		this->disconnectClient(m_currentClient, "Your map version differs from the server's!");
		return;
	}

	// Add a player for this client in the master entity state.
	const auto newPlayerId = m_world.createPlayer(Vec2{m_game.map().getWidth() / 2, m_game.map().getHeight() / 2}, username);
	if (newPlayerId == PLAYER_ID_UNCONNECTED) {
		this->disconnectClient(m_currentClient, "Failed to add player entity.");
		return;
	}
	m_clients.set<CLIENT_PLAYER_ID>(m_currentClient, newPlayerId);

	// Setup inventory.
	if (!this->accessInventory(msg.inventoryId, msg.inventoryToken, address, username)) {
		std::tie(msg.inventoryId, msg.inventoryToken) = this->createInventory(address, username);
		if (msg.inventoryId == INVENTORY_ID_INVALID) {
			this->disconnectClient(m_currentClient, "Failed to create inventory.");
			return;
		}
	}
	m_clients.set<CLIENT_INVENTORY_ID>(m_currentClient, msg.inventoryId);

	// Inform the client that their join request has been accepted.
	if (!client.write(msg::cl::out::Joined{{}, playerId, msg.inventoryId, msg.inventoryToken, std::string{sv_motd}})) {
		this->disconnectClient(m_currentClient, "Failed to write joined message.");
		return;
	}

	// Send the initial state of our replicated cvars.
	auto modifiedCvars = std::vector<ConVarUpdate>{};
	for (const auto& [name, cvar] : ConVar::all()) {
		if ((cvar.getFlags() & ConVar::REPLICATED) != 0) {
			modifiedCvars.push_back(ConVarUpdate{{}, std::string{name}, std::string{cvar.getRaw()}});
		}
	}

	if (!client.write(msg::cl::out::CvarMod{{}, modifiedCvars})) {
		this->disconnectClient(m_currentClient, "Failed to write modified cvars.");
		return;
	}

	// Tell the client to select a team.
	if (!client.write(msg::cl::out::PleaseSelectTeam{{}})) {
		this->disconnectClient(m_currentClient, "Failed to write team select message.");
		return;
	}

	auto joinMessage = fmt::format("{} has joined the game.", username);
	this->writeServerChatMessage(joinMessage);
	m_game.println(std::move(joinMessage));

	INFO_MSG(Msg::SERVER, "Game server: Client \"{}\" ({}) successfully joined with player id \"{}\".", std::string{endpoint}, username, playerId);

	this->callIfDefined(Script::command({"on_player_join", cmd::formatPlayerId(playerId)}));
}

auto GameServer::handleMessage(msg::sv::in::UserCmd&& msg) -> void {
	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (playerId == PLAYER_ID_UNCONNECTED) {
		return;
	}

	if (msg.number > client.latestUserCmdNumber) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Game server: Received snapshot ack #{} from player \"{}\".", msg.latestSnapshotReceived, username);
		client.latestUserCmdNumber = msg.number;
		client.latestSnapshotReceived = msg.latestSnapshotReceived;
		if (msg.actions != client.latestActions) {
			client.afkTimer.reset();
			client.latestActions = msg.actions;
			if (auto&& player = m_world.findPlayer(playerId)) {
				player.setActions(msg.actions);
			}
		}
	}
}

auto GameServer::handleMessage(msg::sv::in::ChatMessage&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (playerId == PLAYER_ID_UNCONNECTED) {
		return;
	}

	const auto message = net::sanitizeMessage(msg.message);
	INFO_MSG(Msg::CHAT, "[CHAT] {}: {}", username, message);
	if (!m_game.gameClient()) {
		m_game.println(fmt::format("[CHAT] {}: {}", username, message));
	}

	for (auto& [otherClient, otherEndpoint, otherAddress, otherUsername, otherPlayerId, otherInventoryId, otherRconToken] : m_clients) {
		if (otherPlayerId != PLAYER_ID_UNCONNECTED) {
			if (!otherClient.write(msg::cl::out::ChatMessage{{}, playerId, message}) ||
			    !otherClient.write(msg::cl::out::PlaySoundReliable{{}, SoundId::chat_message()})) {
				INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::CHAT,
				         "Game server: Failed to write chat message from \"{}\" to \"{}\".",
				         std::string{endpoint},
				         std::string{otherEndpoint});
			}
		}
	}

	this->callIfDefined(Script::command({"on_chat", cmd::formatIpEndpoint(endpoint), message}));
}

auto GameServer::handleMessage(msg::sv::in::TeamChatMessage&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (playerId == PLAYER_ID_UNCONNECTED) {
		return;
	}

	if (const auto& player = m_world.findPlayer(playerId)) {
		const auto message = net::sanitizeMessage(msg.message);
		INFO_MSG(Msg::CHAT, "[{} CHAT] {}: {}", player.getTeam().getName(), username, message);
		if (!m_game.gameClient()) {
			m_game.println(fmt::format("[{} CHAT] {}: {}", player.getTeam().getName(), username, message));
		}

		for (auto& [otherClient, otherEndpoint, otherAddress, otherUsername, otherPlayerId, otherInventoryId, otherRconToken] : m_clients) {
			if (otherPlayerId != PLAYER_ID_UNCONNECTED) {
				if (const auto& otherPlayer = m_world.findPlayer(otherPlayerId)) {
					if (otherPlayer.getTeam() == player.getTeam()) {
						if (!otherClient.write(msg::cl::out::TeamChatMessage{{}, playerId, message}) ||
						    !otherClient.write(msg::cl::out::PlaySoundReliable{{}, SoundId::chat_message()})) {
							INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::CHAT,
							         "Game server: Failed to write team chat message from \"{}\" to \"{}\".",
							         std::string{endpoint},
							         std::string{otherEndpoint});
						}
					}
				}
			}
		}

		this->callIfDefined(Script::command({"on_team_chat", cmd::formatIpEndpoint(endpoint), cmd::formatTeamId(player.getTeam()), message}));
	}
}

auto GameServer::handleMessage(msg::sv::in::TeamSelect&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (playerId == PLAYER_ID_UNCONNECTED) {
		return;
	}

	if (msg.team == Team::none()) {
		this->writeCommandError(client, "Invalid team.");
		if (!client.write(msg::cl::out::PleaseSelectTeam{{}})) {
			this->disconnectClient(m_currentClient, "Failed to write team select response message.");
		}
	} else if (msg.playerClass == PlayerClass::none()) {
		this->writeCommandError(client, "Invalid class.");
		if (!client.write(msg::cl::out::PleaseSelectTeam{{}})) {
			this->disconnectClient(m_currentClient, "Failed to write team select response message.");
		}
	} else if (!m_world.playerTeamSelect(playerId, msg.team, msg.playerClass)) {
		this->writeCommandError(client, "Team select failed.");
		if (!client.write(msg::cl::out::PleaseSelectTeam{{}})) {
			this->disconnectClient(m_currentClient, "Failed to write team select response message.");
		}
	}
}

auto GameServer::handleMessage(msg::sv::in::ResourceDownloadRequest&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	if (m_stopping) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (const auto it = m_resources.find(msg.nameHash); it != m_resources.end()) {
		if (!it->second.canDownload) {
			this->disconnectClient(m_currentClient, "Resource download request denied.");
			return;
		}

		INFO_MSG(Msg::SERVER, "Game server uploading {} to \"{}\".", it->second.name, std::string{endpoint});
		const auto chunkSize = static_cast<std::size_t>(sv_resource_upload_chunk_size);
		if (it->second.data.size() <= chunkSize) {
			if (!client.write(msg::cl::out::ResourceDownloadLast{{}, it->first, it->second.data})) {
				this->disconnectClient(m_currentClient, "Failed to write resource.");
				return;
			}
		} else {
			if (!client.write(msg::cl::out::ResourceDownloadPart{{}, it->first, it->second.data.substr(0, chunkSize)})) {
				this->disconnectClient(m_currentClient, "Failed to write first resource part.");
				return;
			}
			client.resourceUpload = &*it;
			client.resourceUploadProgress = chunkSize;
			client.resourceUploadTimer.reset();
		}
	} else {
		this->disconnectClient(m_currentClient, "Resource download request denied.");
	}
}

auto GameServer::handleMessage(msg::sv::in::UpdateRateChange&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (playerId == PLAYER_ID_UNCONNECTED) {
		return;
	}

	client.updateInterval = (msg.newUpdateRate > 0) ? 1.0f / static_cast<float>(msg.newUpdateRate) : 0.0f;
	client.updateTimer.reset();
}

auto GameServer::handleMessage(msg::sv::in::UsernameChange&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	m_clients.set<CLIENT_USERNAME>(m_currentClient, this->findValidUsername(msg.newUsername));
	m_world.setPlayerName(playerId, username);
}

auto GameServer::handleMessage(msg::sv::in::ForwardedCommand&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (playerId == PLAYER_ID_UNCONNECTED) {
		this->writeCommandError(client, "Not connected.");
		return;
	}

	if (msg.command.size() > net::MAX_SERVER_COMMAND_SIZE) {
		this->writeCommandError(client, "Command is too long.");
		return;
	}

	if (msg.command.empty()) {
		this->writeCommandError(client, "Empty command.");
		return;
	}

	auto command = Script::Command{};
	command.reserve(msg.command.size() + 2);
	command.emplace_back("on_server_receive_command");
	command.emplace_back(std::string{endpoint});
	for (auto& arg : msg.command) {
		command.emplace_back(std::move(arg));
	}
	this->callIfDefined(std::move(command));
}

auto GameServer::handleMessage(msg::sv::in::HeartbeatRequest&&) -> void {
	if (this->testSpam()) {
		return;
	}

	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *m_currentClient;
	if (endpoint == m_metaServerEndpoint) {
		client.afkTimer.reset();
		if (!client.connection.write<MetaServerOutputMessages>(msg::meta::sv::out::Heartbeat{{}})) {
			this->disconnectClient(m_currentClient, "Failed to write heartbeat.");
		}
		m_metaServerRetryTimer.reset();
	}
}

auto GameServer::handleMessage(msg::sv::in::MetaInfoRequest&&) -> void {
	if (this->testSpam()) {
		return;
	}

	if (!(*m_currentClient)
	         ->connection.write<MetaClientOutputMessages>(msg::meta::cl::out::MetaInfo{{},
	                                                                                   m_tickrate,
	                                                                                   static_cast<std::uint32_t>(m_world.getPlayerCount()),
	                                                                                   static_cast<std::uint32_t>(m_bots.size()),
	                                                                                   static_cast<std::uint32_t>(sv_playerlimit),
	                                                                                   m_game.map().getName(),
	                                                                                   sv_hostname,
	                                                                                   game_version})) {
		this->disconnectClient(m_currentClient, "Failed to write meta info.");
	}
}

auto GameServer::hatDistribution() -> HatDistribution& {
	static auto hatDistribution = HatDistribution{};
	return hatDistribution;
}

auto GameServer::generateHat() -> Hat {
	static_assert(Hat::none().getId() == 0);
	return Hat::findById(GameServer::hatDistribution()(m_vm.rng()) + 1);
}

auto GameServer::game() -> Game& {
	return m_game;
}

auto GameServer::gameServer() -> GameServer& {
	return *this;
}

auto GameServer::vm() -> VirtualMachine& {
	return m_vm;
}

auto GameServer::testSpam() -> bool {
	assert(m_currentClient != m_clients.end());
	if (sv_spam_limit != 0 && ++((*m_currentClient)->spamCounter) > sv_spam_limit) {
		this->disconnectClient(m_currentClient, "Kicked for spamming commands too fast.");
		return true;
	}
	return false;
}

auto GameServer::equipHat(InventoryId id, Hat hat) -> void {
	if (const auto it = m_clients.find<CLIENT_INVENTORY_ID>(id); it != m_clients.end()) {
		if (m_world.equipPlayerHat(it->get<CLIENT_PLAYER_ID>(), hat)) {
			if (!(*it)->write(msg::cl::out::InventoryEquipHat{{}, hat})) {
				INFO_MSG(Msg::SERVER, "Game server: Failed to write inventory equip hat message.");
			}
		}
	}
}

auto GameServer::unequipHat(InventoryId id, Hat hat) -> void {
	if (const auto it = m_clients.find<CLIENT_INVENTORY_ID>(id); it != m_clients.end()) {
		const auto playerId = it->get<CLIENT_PLAYER_ID>();
		if (auto&& player = m_world.findPlayer(playerId)) {
			if (player.getHat() == hat && m_world.equipPlayerHat(playerId, Hat::none())) {
				if (!(*it)->write(msg::cl::out::InventoryEquipHat{{}, Hat::none()})) {
					INFO_MSG(Msg::SERVER, "Game server: Failed to write inventory equip hat message.");
				}
			}
		}
	}
}

auto GameServer::getEquippedHat(InventoryId id) const -> Hat {
	if (const auto it = m_clients.find<CLIENT_INVENTORY_ID>(id); it != m_clients.end()) {
		if (const auto& player = m_world.findPlayer(it->get<CLIENT_PLAYER_ID>())) {
			return player.getHat();
		}
	}
	return Hat::none();
}

auto GameServer::reply(msg::cl::out::InventoryEquipHat&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write inventory reply.");
	}
}

auto GameServer::reply(msg::cl::out::RemoteConsoleLoginInfo&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write remote console reply.");
	}
}

auto GameServer::reply(msg::cl::out::RemoteConsoleLoginGranted&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write remote console reply.");
	}
}

auto GameServer::reply(msg::cl::out::RemoteConsoleLoginDenied&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write remote console reply.");
	}
}

auto GameServer::reply(msg::cl::out::RemoteConsoleResult&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write remote console reply.");
	}
}

auto GameServer::reply(msg::cl::out::RemoteConsoleOutput&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write remote console reply.");
	}
}

auto GameServer::reply(msg::cl::out::RemoteConsoleDone&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write remote console reply.");
	}
}

auto GameServer::reply(msg::cl::out::RemoteConsoleLoggedOut&& msg) -> void {
	assert(m_currentClient != m_clients.end());
	if (!(*m_currentClient)->write(msg)) {
		this->disconnectClient(m_currentClient, "Failed to write remote console reply.");
	}
}

auto GameServer::registerCurrentRconClient(std::string_view username) -> void {
	assert(m_currentClient != m_clients.end());
	m_clients.set<CLIENT_RCON_TOKEN>(m_currentClient, ClientInfo::RconToken{username});
}

auto GameServer::unregisterRconClient(std::string_view username) -> void {
	if (const auto it = m_clients.find<CLIENT_RCON_TOKEN>(ClientInfo::RconToken{username}); it != m_clients.end()) {
		m_clients.set<CLIENT_RCON_TOKEN>(it, std::nullopt);
	}
}

auto GameServer::getCurrentClientInventoryId() const -> InventoryId {
	assert(m_currentClient != m_clients.end());
	return m_currentClient->get<CLIENT_INVENTORY_ID>();
}

auto GameServer::getCurrentClientRegisteredRconUsername() const -> std::optional<std::string_view> {
	assert(m_currentClient != m_clients.end());
	return m_currentClient->get<CLIENT_RCON_TOKEN>();
}

auto GameServer::write(std::string_view username, msg::cl::out::RemoteConsoleResult&& msg) -> void {
	if (const auto it = m_clients.find<CLIENT_RCON_TOKEN>(ClientInfo::RconToken{username}); it != m_clients.end()) {
		if (!(*it)->write(msg)) {
			this->disconnectClient(it, "Failed to write remote console message.");
		}
	}
}

auto GameServer::write(std::string_view username, msg::cl::out::RemoteConsoleOutput&& msg) -> void {
	if (const auto it = m_clients.find<CLIENT_RCON_TOKEN>(ClientInfo::RconToken{username}); it != m_clients.end()) {
		if (!(*it)->write(msg)) {
			this->disconnectClient(it, "Failed to write remote console message.");
		}
	}
}

auto GameServer::write(std::string_view username, msg::cl::out::RemoteConsoleDone&& msg) -> void {
	if (const auto it = m_clients.find<CLIENT_RCON_TOKEN>(ClientInfo::RconToken{username}); it != m_clients.end()) {
		if (!(*it)->write(msg)) {
			this->disconnectClient(it, "Failed to write remote console message.");
		}
	}
}

auto GameServer::write(std::string_view username, msg::cl::out::RemoteConsoleLoggedOut&& msg) -> void {
	if (const auto it = m_clients.find<CLIENT_RCON_TOKEN>(ClientInfo::RconToken{username}); it != m_clients.end()) {
		if (!(*it)->write(msg)) {
			this->disconnectClient(it, "Failed to write remote console message.");
		}
	}
}

auto GameServer::tick() -> void {
	DEBUG_MSG_INDENT(Msg::SERVER_TICK | Msg::CONNECTION_DETAILED, "Tick @ {} ms", m_tickInterval * 1000.0f) {
		// Update bots.
		if (sv_bot_ai_enable && (!sv_bot_ai_require_players || this->hasPlayers())) {
			if (m_botTickTimer.advance(m_tickInterval, m_botTickInterval)) {
				for (auto& bot : m_bots) {
					if (auto&& player = m_world.findPlayer(bot.getId())) {
						bot.setSnapshot(m_world.takeSnapshot(bot.getId()));
						bot.think(m_botTickInterval);
						player.setActions(bot.getActions());
					}
				}
			}
		}

		// Update entity state.
		m_world.update(m_tickInterval);
	}
}

auto GameServer::updateConfigAutoSave(float deltaTime) -> void {
	if (m_configAutoSaveTimer.advance(deltaTime, m_configAutoSaveInterval, sv_config_auto_save_interval != 0)) {
		INFO_MSG(Msg::SERVER, "Auto-saving game server config.");
		m_game.consoleCommand(GET_COMMAND(sv_writeconfig));
	}
}

auto GameServer::receivePackets() -> void {
	auto buffer = std::vector<std::byte>(net::MAX_PACKET_SIZE);
	while (true) {
		auto ec = std::error_code{};
		auto remoteEndpoint = net::IpEndpoint{};
		const auto receivedBytes = m_socket.receiveFrom(remoteEndpoint, buffer, ec).size();
		if (ec) {
			if (ec != net::SocketError::WAIT) {
				DEBUG_MSG(Msg::SERVER, "Game server: Failed to receive packet: {}", ec.message());
			}
			break;
		}

		if (const auto it = m_clients.find<CLIENT_ENDPOINT>(remoteEndpoint); it != m_clients.end()) {
			buffer.resize(receivedBytes);
			(*it)->connection.receivePacket(std::move(buffer));
			buffer.resize(net::MAX_PACKET_SIZE);
		} else if (m_connectingClients >= static_cast<std::size_t>(sv_max_connecting_clients)) {
			DEBUG_MSG(
				Msg::CONNECTION_DETAILED,
				"Game server: Ignoring {} bytes from unconnected ip \"{}\" because the max connecting client limit of {} has been reached!",
				receivedBytes,
				std::string{remoteEndpoint},
				static_cast<std::size_t>(sv_max_connecting_clients));
		} else if (m_clients.size() >= static_cast<std::size_t>(sv_max_clients)) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED,
			          "Game server: Ignoring {} bytes from unconnected ip \"{}\" because the max client limit of {} has been reached!",
			          receivedBytes,
			          std::string{remoteEndpoint},
			          static_cast<std::size_t>(sv_max_clients));
		} else if (m_stopping) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED,
			          "Game server: Ignoring {} bytes from unconnected ip \"{}\" because the server is stopping!",
			          receivedBytes,
			          std::string{remoteEndpoint});
		} else if (remoteEndpoint == m_metaServerEndpoint) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Game server: Ignoring {} bytes from meta server ip \"{}\"!", receivedBytes, std::string{remoteEndpoint});
		} else {
			const auto timeout = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(sv_timeout)});
			auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = m_clients.emplace_back(
				ClientInfo{m_socket, timeout, sv_throttle_limit, sv_throttle_max_period, *this},
				remoteEndpoint,
				remoteEndpoint.getAddress(),
				std::string{},
				PLAYER_ID_UNCONNECTED,
				INVENTORY_ID_INVALID,
				std::nullopt);
			INFO_MSG_INDENT(Msg::SERVER, "Game server: Client \"{}\" connecting...", std::string{endpoint}) {
				if (!client.connection.accept(endpoint)) {
					INFO_MSG(Msg::SERVER,
					         "Game server: Failed to intialize connection to \"{}\": {}",
					         std::string{endpoint},
					         client.connection.getDisconnectMessage());
					m_clients.pop_back();
				} else {
					++m_connectingClients;
					client.connecting = true;
					buffer.resize(receivedBytes);
					client.connection.receivePacket(std::move(buffer));
					buffer.resize(net::MAX_PACKET_SIZE);
					if (m_bannedPlayers.count(address) != 0) {
						INFO_MSG(Msg::SERVER, "Game server: This ip address is banned from the server. Kicking.");
						this->disconnectClient(std::prev(m_clients.end()), "You are banned from this server.");
					} else if (const auto maxClientsPerIp = static_cast<std::size_t>(sv_max_connections_per_ip);
					           maxClientsPerIp != 0 && !address.isLoopback() && !address.isPrivate() &&
					           address != net::IpAddress::getLocalAddress(ec) && this->countClientsWithIp(address) > maxClientsPerIp) {
						INFO_MSG(Msg::SERVER, "Meta server: Too many clients with the same ip address. Kicking.");
						this->disconnectClient(std::prev(m_clients.end()),
						                       fmt::format("The server does not allow more than {} client{} from the same IP address.",
						                                   maxClientsPerIp,
						                                   (maxClientsPerIp == 1) ? "" : "s"));
					}
				}
			}
		}
	}
}

auto GameServer::updateConnections() -> void {
	for (auto it = m_clients.begin(); it != m_clients.end();) {
		m_currentClient = it;
		if (!(*it)->connection.update()) {
			this->dropClient(it);
			it = m_clients.erase(it);
		} else {
			++it;
		}
	}
	m_currentClient = m_clients.end();
}

auto GameServer::updateMetaServerConnection(float deltaTime) -> void {
	if (m_metaServerRetryTimer.advance(deltaTime, sv_meta_submit_retry_interval, sv_meta_submit && sv_meta_submit_retry)) {
		if (!m_clients.contains<CLIENT_USERNAME>(std::string{USERNAME_META_SERVER})) {
			this->connectToMetaServer();
		}
	}
}

auto GameServer::updateTicks(float deltaTime) -> void {
	if (auto ticks = m_tickTimer.advance(deltaTime, m_tickInterval); ticks > 0) {
		const auto timeSinceLastTick = static_cast<float>(ticks) * m_tickInterval;
		if (ticks > sv_max_ticks_per_frame) {
			INFO_MSG(Msg::SERVER | Msg::SERVER_TICK,
			         "Game server: Framerate can't keep up with the tickrate! Skipping {} ms.",
			         (ticks - sv_max_ticks_per_frame) * m_tickInterval * 1000.0f);
			ticks = sv_max_ticks_per_frame;
		}

		while (ticks-- > 0) {
			this->tick();
		}

		this->updateClients(timeSinceLastTick);
		this->writeWorldStateToClients();
		this->sendPackets();
	}
}

auto GameServer::updateClients(float deltaTime) -> void {
	const auto spamUpdates = m_spamTimer.advance(deltaTime, m_spamInterval);
	const auto modifiedCvars = this->pollModifiedCvars();
	const auto modifiedCvarsMessage = msg::cl::out::CvarMod{{}, modifiedCvars};
	for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
		this->updateClient(it, deltaTime, spamUpdates, modifiedCvarsMessage);
	}
}

auto GameServer::sendPackets() -> void {
	for (auto& client : m_clients) {
		client->connection.sendPackets();
	}
}

auto GameServer::updateProcess() -> void {
	m_vm.output(m_process->run(m_game, this, nullptr, nullptr, nullptr));
}

auto GameServer::pollModifiedCvars() -> std::vector<ConVarUpdate> { // NOLINT(readability-convert-member-functions-to-static)
	auto modifiedCvars = std::vector<ConVarUpdate>{};
	for (const auto& cvar : GameServer::modifiedCvars()) {
		modifiedCvars.push_back(ConVarUpdate{{}, std::string{cvar->getName()}, std::string{cvar->getRaw()}});
	}
	GameServer::modifiedCvars().clear();
	return modifiedCvars;
}

auto GameServer::updateClient(Clients::iterator it, float deltaTime, int spamUpdates, const msg::cl::out::CvarMod& modifiedCvarsMessage) -> void {
	assert(it != m_clients.end());

	this->updateClientSpamCounter(it, spamUpdates);
	this->updateClientAfkTimer(it, deltaTime);
	this->writeClientModifiedCvars(it, modifiedCvarsMessage);
	this->updateClientResourceUpload(it, deltaTime);
	this->updateClientPing(it);
}

auto GameServer::updateClientSpamCounter(Clients::iterator it, int spamUpdates) -> void { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_clients.end());
	auto& client = **it;

	client.spamCounter = std::max(0, client.spamCounter - spamUpdates);
}

auto GameServer::updateClientAfkTimer(Clients::iterator it, float deltaTime) -> void {
	assert(it != m_clients.end());
	auto& client = **it;

	if (client.afkTimer.advance(deltaTime, sv_afk_autokick_time).first) {
		if (sv_afk_autokick_time != 0.0f && !client.connection.getRemoteAddress().isLoopback()) {
			this->disconnectClient(it, "Kicked for inactivity.");
		}
	}
}

auto GameServer::writeClientModifiedCvars(Clients::iterator it, const msg::cl::out::CvarMod& modifiedCvarsMessage) -> void {
	assert(it != m_clients.end());
	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;

	if (playerId != PLAYER_ID_UNCONNECTED) {
		if (!modifiedCvarsMessage.cvars.empty()) {
			if (!client.write(modifiedCvarsMessage)) {
				this->disconnectClient(it, "Failed to write modified cvars.");
			}
		}
	}
}

auto GameServer::updateClientResourceUpload(Clients::iterator it, float deltaTime) -> void {
	assert(it != m_clients.end());
	auto& client = **it;

	const auto chunkSize = static_cast<std::size_t>(sv_resource_upload_chunk_size);
	for (auto parts = client.resourceUploadTimer.advance(deltaTime, m_resourceUploadInterval, client.resourceUpload != nullptr); parts > 0; --parts) {
		const auto& [nameHash, resource] = *client.resourceUpload;
		const auto remainingSize = resource.data.size() - client.resourceUploadProgress;
		if (remainingSize <= chunkSize) {
			if (!client.write(msg::cl::out::ResourceDownloadLast{{}, nameHash, resource.data.substr(client.resourceUploadProgress)})) {
				this->disconnectClient(m_currentClient, "Failed to write last resource part.");
			}
			client.resourceUpload = nullptr;
			client.resourceUploadProgress = 0;
			client.resourceUploadTimer.reset();
			break;
		}
		if (!client.write(msg::cl::out::ResourceDownloadPart{{}, nameHash, resource.data.substr(client.resourceUploadProgress, chunkSize)})) {
			this->disconnectClient(m_currentClient, "Failed to write resource part.");
		}
		client.resourceUploadProgress += chunkSize;
	}
}

auto GameServer::updateClientPing(Clients::iterator it) -> void {
	assert(it != m_clients.end());
	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;

	if (auto&& player = m_world.findPlayer(playerId)) {
		const auto pingMilliseconds =
			std::chrono::duration_cast<std::chrono::duration<Latency, std::milli>>(client.connection.getLatestMeasuredPingDuration()).count();
		player.setLatestMeasuredPingDuration(pingMilliseconds);
	}
}

auto GameServer::addResource(std::string name, std::string data) -> void { // NOLINT(performance-unnecessary-value-param)
	const auto extension = std::filesystem::path{name}.extension();
	const auto size = data.size();
	const auto isText = extension == ".txt" || extension == ".cfg";
	const auto nameHash = util::CRC32{util::asBytes(util::Span{name})};
	const auto fileHash = util::CRC32{util::asBytes(util::Span{data})};
	if (m_resources.try_emplace(nameHash, name, std::move(data), sv_allow_resource_download).second) {
		m_resourceInfo.emplace_back(std::move(name), nameHash, fileHash, size, isText, sv_allow_resource_download);
	} else {
		m_game.warning(fmt::format("Failed to add resource \"{}\"!", name));
	}
}

auto GameServer::loadMap() -> bool {
	m_world.reset();
	m_bots.clear();

	for (auto& client : m_clients) {
		client->resourceUpload = nullptr;
		client->resourceUploadProgress = 0;
		client->resourceUploadTimer.reset();
	}
	m_resources.clear();
	m_resourceInfo.clear();

	INFO_MSG(Msg::SERVER, "Game server: Loading map \"{}\"...", sv_map);
	auto buf = util::readFile(fmt::format("{}/{}/{}", data_dir, data_subdir_maps, sv_map));
	if (!buf) {
		buf = util::readFile(fmt::format("{}/{}/{}/{}", data_dir, data_subdir_downloads, data_subdir_maps, sv_map));
		if (!buf) {
			m_game.warning(fmt::format("Failed to load map \"{}\" (couldn't read file).", sv_map));
			return false;
		}
	}

	if (!m_game.map().load(std::string{sv_map}, *buf)) {
		m_game.warning(fmt::format("Failed to load map \"{}\" (invalid format).", sv_map));
		return false;
	}
	this->addResource(fmt::format("{}/{}", data_subdir_maps, sv_map), std::move(*buf));

	for (const auto& resourceName : m_game.map().getResources()) {
		const auto resourceFilepath = fmt::format("{}/{}", data_dir, resourceName);
		INFO_MSG(Msg::SERVER, "Game server: Loading resource \"{}\".", resourceFilepath);
		if (auto buf = util::readFile(resourceFilepath)) {
			this->addResource(resourceName, std::move(*buf));
		} else {
			m_game.warning(fmt::format("Failed to load resource \"{}\" (couldn't read file).", resourceName));
			return false;
		}
	}

	m_xCoordinateDistribution = Bot::CoordinateDistributionX{0, static_cast<Vec2::Length>(m_game.map().getWidth() - 1)};
	m_yCoordinateDistribution = Bot::CoordinateDistributionY{0, static_cast<Vec2::Length>(m_game.map().getHeight() - 1)};

	for (const auto& position : m_game.map().getRedSpawns()) {
		m_world.addSpawnPoint(position, Team::red());
	}

	for (const auto& position : m_game.map().getBlueSpawns()) {
		m_world.addSpawnPoint(position, Team::blue());
	}

	for (const auto& position : m_game.map().getMedkitSpawns()) {
		m_world.createMedkit(position);
	}

	for (const auto& position : m_game.map().getAmmopackSpawns()) {
		m_world.createAmmopack(position);
	}

	for (const auto& position : m_game.map().getRedFlagSpawns()) {
		m_world.createFlag(position, Team::red(), "RED intelligence");
	}

	for (const auto& position : m_game.map().getBlueFlagSpawns()) {
		m_world.createFlag(position, Team::blue(), "BLU intelligence");
	}

	if (const auto& path = m_game.map().getRedCartPath(); !path.empty()) {
		m_world.createPayloadCart(Team::red(), std::vector<Vec2>{path.begin(), path.end()});
	}

	if (const auto& path = m_game.map().getBlueCartPath(); !path.empty()) {
		m_world.createPayloadCart(Team::blue(), std::vector<Vec2>{path.begin(), path.end()});
	}

	m_world.startMap();

	// Find the next map in the map rotation and set the next level.
	const auto& mapRotation = Script::parse(sv_map_rotation);

	bool found = false;
	for (const auto [i, command] : util::enumerate(mapRotation)) {
		if (const auto name = Script::commandString(command); name == m_game.map().getName()) {
			const auto nextName = Script::commandString(mapRotation[(i + 1) % mapRotation.size()]);
			if (auto result = sv_nextlevel.set(nextName, m_game, this, nullptr, nullptr, nullptr); result.status == cmd::Status::ERROR_MSG) {
				m_game.warning(std::move(result.value));
			}
			found = true;
			break;
		}
	}

	// If this map was not found in the map rotation, Set the next level to the first map in the map rotation.
	if (!found && !mapRotation.empty()) {
		const auto nextName = Script::commandString(mapRotation.front());
		if (auto result = sv_nextlevel.set(nextName, m_game, this, nullptr, nullptr, nullptr); result.status == cmd::Status::ERROR_MSG) {
			m_game.warning(std::move(result.value));
		}
	}

	// Add bots.
	for (auto i = 0; i < sv_bot_count; ++i) {
		if (!this->addBot()) {
			m_game.warning(fmt::format("Failed to add bot #{}.", i + 1));
		}
	}

	INFO_MSG(Msg::SERVER, "Game server: Successfully loaded map.");
	return true;
}

auto GameServer::connectToMetaServer() -> void {
	auto ec = std::error_code{};
	const auto ip = net::IpAddress::resolve(std::string{meta_address}.c_str(), ec);
	if (ec) {
		m_game.warning(
			fmt::format("Couldn't resolve meta server ip address \"{}\": {}\n"
		                "Your server will not be shown in the server list.\n"
		                "Set {} to 0 to disable connecting to the meta server.",
		                meta_address,
		                ec.message(),
		                sv_meta_submit.cvar().getName()));
		if (sv_meta_submit_retry) {
			m_game.warning(fmt::format("The connection will be retried in {} seconds.", static_cast<float>(sv_meta_submit_retry_interval)));
		}
		return;
	}
	// Initialize connection.
	m_metaServerEndpoint = net::IpEndpoint{ip, static_cast<net::PortNumber>(meta_port)};
	const auto timeout = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(sv_timeout)});
	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = m_clients.emplace_back(
		ClientInfo{m_socket, timeout, sv_throttle_limit, sv_throttle_max_period, *this},
		m_metaServerEndpoint,
		m_metaServerEndpoint.getAddress(),
		std::string{USERNAME_META_SERVER},
		PLAYER_ID_UNCONNECTED,
		INVENTORY_ID_INVALID,
		std::nullopt);
	INFO_MSG_INDENT(Msg::SERVER, "Game server: Connecting to meta server \"{}\"...", std::string{endpoint}) {
		if (!client.connection.connect(endpoint)) {
			INFO_MSG(Msg::SERVER, "Game server: Failed to intialize connection to \"{}\": {}", std::string{endpoint}, client.connection.getDisconnectMessage());
			m_game.warning(
				fmt::format("Failed to intialize connection to meta server: {}\n"
			                "Your server will not be shown in the server list.\n"
			                "Set {} to 0 to disable connecting to the meta server.",
			                client.connection.getDisconnectMessage(),
			                sv_meta_submit.cvar().getName()));
			if (sv_meta_submit_retry) {
				m_game.warning(fmt::format("The connection will be retried in {} seconds.", static_cast<float>(sv_meta_submit_retry_interval)));
			}
			m_clients.pop_back();
		} else {
			++m_connectingClients;
			client.connecting = true;
		}
	}
}

auto GameServer::disconnectClient(Clients::iterator it, std::string_view reason) -> void {
	assert(it != m_clients.end());
	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;

	const auto delay = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(sv_disconnect_cooldown)});
	client.connection.disconnect(reason, delay);
	if (playerId != PLAYER_ID_UNCONNECTED) {
		const auto name = (username.empty()) ? USERNAME_UNCONNECTED : std::string_view{username};
		auto leaveMessage = (reason.empty()) ? fmt::format("{} left the game.", name) : fmt::format("{} left the game: {}", name, reason);
		this->writeServerChatMessage(leaveMessage);
		m_game.println(std::move(leaveMessage));
		m_world.deletePlayer(playerId);
		this->resetClient(it);
	}
}

auto GameServer::dropClient(Clients::iterator it) -> void {
	assert(it != m_clients.end());
	auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] = *it;

	if (rconToken) {
		this->endRconSession(*rconToken);
	}

	if (playerId != PLAYER_ID_UNCONNECTED) {
		const auto reason = client.connection.getDisconnectMessage();
		const auto name = (username.empty()) ? USERNAME_UNCONNECTED : std::string_view{username};
		auto leaveMessage = (reason.empty()) ? fmt::format("{} left the game.", name) : fmt::format("{} left the game: {}", name, reason);
		this->writeServerChatMessage(leaveMessage);
		m_game.println(std::move(leaveMessage));
		m_world.deletePlayer(playerId);
		this->resetClient(it);
	}

	if (sv_meta_submit) {
		if (endpoint == m_metaServerEndpoint) {
			if (client.connection.getDisconnectMessage() == Connection::HANDSHAKE_TIMED_OUT_MESSAGE) {
				m_game.warning(
					fmt::format("Failed to connect to the meta server.\n"
				                "Your server will not be shown in the server list.\n"
				                "Set {} to 0 to disable connecting to the meta server.",
				                sv_meta_submit.cvar().getName()));
				if (sv_meta_submit_retry) {
					m_game.warning(fmt::format("The connection will be retried in {} seconds.", static_cast<float>(sv_meta_submit_retry_interval)));
				}
			}
		}
	}

	if (client.connecting) {
		--m_connectingClients;
	}

	INFO_MSG(Msg::SERVER,
	         "Game server: Client \"{}\" was dropped.{}",
	         std::string{endpoint},
	         (client.connection.getDisconnectMessage().empty()) ? "" : fmt::format(" Reason: {}", client.connection.getDisconnectMessage()));
}

auto GameServer::writeServerInfo(ClientInfo& client) -> bool {
	if (const auto& saltView = sv_password.getHashSalt()) {
		auto salt = crypto::pw::Salt{};
		util::copy(*saltView, salt.begin());
		return client.write(msg::cl::out::ServerInfo{{},
		                                             m_tickrate,
		                                             static_cast<std::uint32_t>(m_world.getPlayerCount()),
		                                             static_cast<std::uint32_t>(m_bots.size()),
		                                             static_cast<std::uint32_t>(sv_playerlimit),
		                                             m_game.map().getName(),
		                                             sv_hostname,
		                                             game_version,
		                                             salt,
		                                             sv_password.getHashType(),
		                                             m_resourceInfo});
	}
	return false;
}

auto GameServer::writeCommandOutput(ClientInfo& client, std::string_view message) -> void { // NOLINT(readability-convert-member-functions-to-static)
	if (!client.write(msg::cl::out::CommandOutput{{}, false, message})) {
		INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT,
		         "Game server: Failed to write special command output to \"{}\".",
		         std::string{client.connection.getRemoteEndpoint()});
	}
}

auto GameServer::writeCommandError(ClientInfo& client, std::string_view message) -> void { // NOLINT(readability-convert-member-functions-to-static)
	if (!client.write(msg::cl::out::CommandOutput{{}, true, message})) {
		INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT,
		         "Game server: Failed to write special command error to \"{}\".",
		         std::string{client.connection.getRemoteEndpoint()});
	}
}

auto GameServer::writeWorldStateToClients() -> void {
	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Game server: Writing world state to clients...") {
		auto deltaData = std::vector<std::byte>{};
		for (auto& [client, endpoint, address, username, playerId, inventoryId, rconToken] : m_clients) {
			if (playerId != PLAYER_ID_UNCONNECTED && client.updateTimer.advance(m_tickInterval, client.updateInterval)) {
				const auto tick = m_world.getTickCount();
				const auto& snapshot = ((*client.snapshots)[tick % client.snapshots->size()] = m_world.takeSnapshot(playerId));
				if (client.latestSnapshotReceived == 0 || client.latestSnapshotReceived + client.snapshots->size() <= tick) {
					DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Game server: Player \"{}\": Writing full snapshot #{}.", username, tick) {
						if (!client.write(msg::cl::out::Snapshot{{}, snapshot})) {
							INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write snapshot to \"{}\".", std::string{endpoint});
						}
					}
				} else {
					DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED,
					                 "Game server: Player \"{}\": Writing snapshot delta from #{} to #{}.",
					                 username,
					                 client.latestSnapshotReceived,
					                 tick) {
						const auto sourceTick = client.latestSnapshotReceived;
						const auto& sourceSnapshot = (*client.snapshots)[sourceTick % client.snapshots->size()];

						auto deltaDataStream = net::ByteOutputStream{deltaData};
						deltaCompress(deltaDataStream, sourceSnapshot, snapshot);

						if (!client.write(msg::cl::out::SnapshotDelta{{}, sourceTick, deltaData})) {
							INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT, "Game server: Failed to write snapshot delta to \"{}\".", std::string{endpoint});
						}

						deltaData.clear();
					}
				}
			}
		}
	}
}

auto GameServer::findValidUsername(std::string_view original) const -> std::string {
	auto name = std::string{original.substr(0, static_cast<std::size_t>(sv_max_username_length))};
	if (util::iequals(name, USERNAME_META_SERVER) || util::iequals(name, USERNAME_UNCONNECTED)) {
		name.clear();
	}

	util::eraseIf(name, [](char ch) {
		return !Script::isPrintableChar(ch) || ch == '\"' || ch == '(' || ch == '{' || ch == ')' || ch == '}' || ch == '\\' || ch == '|' || ch == '$';
	});
	if (name.empty()) {
		name = "Player";
	}

	auto num = std::size_t{0};
	while (m_world.isPlayerNameTaken(name)) {
		++num;
		if (const auto lastNonNumberPos = name.find_last_not_of("0123456789"); lastNonNumberPos != std::string::npos) {
			name.replace(lastNonNumberPos + 1, std::string::npos, util::toString(num));
		} else {
			name.append(util::toString(num));
		}
	}
	return name;
}

auto GameServer::findClient(std::string_view ipOrName) -> Clients::iterator {
	const auto str = std::string{ipOrName};
	auto ec = std::error_code{};
	if (const auto endpoint = net::IpEndpoint::parse(str, ec); !ec) {
		if (const auto it = this->findClientByIp(endpoint); it != m_clients.end()) {
			return it;
		}
	}
	return m_clients.find<CLIENT_USERNAME>(str);
}

auto GameServer::findClient(std::string_view ipOrName) const -> Clients::const_iterator {
	const auto str = std::string{ipOrName};
	auto ec = std::error_code{};
	if (const auto endpoint = net::IpEndpoint::parse(str, ec); !ec) {
		if (const auto it = this->findClientByIp(endpoint); it != m_clients.end()) {
			return it;
		}
	}
	return m_clients.find<CLIENT_USERNAME>(str);
}

auto GameServer::findClientByIp(net::IpEndpoint endpoint) -> Clients::iterator {
	if (endpoint.getPort() != 0) {
		if (const auto it = m_clients.find<CLIENT_ENDPOINT>(endpoint); it != m_clients.end()) {
			return it;
		}
	} else {
		if (const auto it = m_clients.find<CLIENT_ADDRESS>(endpoint.getAddress()); it != m_clients.end()) {
			return it;
		}
	}
	return m_clients.end();
}

auto GameServer::findClientByIp(net::IpEndpoint endpoint) const -> Clients::const_iterator {
	if (endpoint.getPort() != 0) {
		if (const auto it = m_clients.find<CLIENT_ENDPOINT>(endpoint); it != m_clients.end()) {
			return it;
		}
	} else {
		if (const auto it = m_clients.find<CLIENT_ADDRESS>(endpoint.getAddress()); it != m_clients.end()) {
			return it;
		}
	}
	return m_clients.end();
}

auto GameServer::countClientsWithIp(net::IpAddress ip) const noexcept -> std::size_t {
	return util::countIf(m_clients, [ip](const auto& elem) { return elem.template get<CLIENT_ADDRESS>() == ip; });
}

auto GameServer::countPlayersWithIp(net::IpAddress ip) const noexcept -> std::size_t {
	return util::countIf(m_clients, [ip](const auto& elem) {
		return elem.template get<CLIENT_PLAYER_ID>() != PLAYER_ID_UNCONNECTED && elem.template get<CLIENT_ADDRESS>() == ip;
	});
}
