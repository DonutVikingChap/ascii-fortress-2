#include "meta_server.hpp"

#include "../../console/command.hpp"                       // cmd::...
#include "../../console/commands/game_commands.hpp"        // cmd_disconnect, cmd_quit
#include "../../console/commands/meta_server_commands.hpp" // meta_sv_...
#include "../../console/commands/process_commands.hpp"     // cmd_import, cmd_file
#include "../../console/con_command.hpp"                   // GET_COMMAND
#include "../../debug.hpp"                                 // Msg, INFO_MSG, INFO_MSG_INDENT, DEBUG_MSG_INDENT
#include "../../utilities/algorithm.hpp"                   // util::erase, util::findIf, util::countIf
#include "../../utilities/time.hpp"                        // util::getLocalTimeStr
#include "../game.hpp"                                     // Game

#include <algorithm>    // std::max
#include <chrono>       // std::chrono::...
#include <fmt/core.h>   // fmt::format
#include <system_error> // std::error_code

auto MetaServer::getConfigHeader() -> std::string {
	return fmt::format(
		"// This file is auto-generated every time your meta server is shut down, and loaded every time it is "
		"started.\n"
		"// Do not modify this file manually. Use the autoexec file instead.\n"
		"// Last generated {}.",
		util::getLocalTimeStr("%c"));
}

MetaServer::MetaServer(Game& game)
	: m_game(game)
	, m_currentClient(m_clients.end()) {
	this->updateTimeout();
	this->updateThrottle();
	this->updateSpamLimit();
	this->updateTickrate();
	this->updateConfigAutoSaveInterval();
	this->updatePrivateAddressOverride();
}

auto MetaServer::init() -> bool {
	INFO_MSG(Msg::SERVER, "Meta server: Initializing...");

	auto ec = std::error_code{};

	// Bind socket.
	m_socket.bind(net::IpEndpoint{net::IpAddress::any(), static_cast<net::PortNumber>(meta_sv_port)}, ec);
	if (ec) {
		m_game.error(fmt::format("Failed to bind meta server socket on port \"{}\": {}", meta_sv_port, ec.message()));
		return false;
	}

	// Execute meta server config script.
	if (m_game.consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{meta_sv_config_file}}).status ==
		cmd::Status::ERROR_MSG) {
		m_game.error("Meta server config failed.");
		return false;
	}

	// Execute meta server autoexec script.
	if (m_game.consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{meta_sv_autoexec_file}}).status ==
		cmd::Status::ERROR_MSG) {
		m_game.error("Meta server autoexec failed.");
		return false;
	}

	INFO_MSG(Msg::SERVER,
			 "Meta server: Started on \"{}:{}\".",
			 std::string{net::IpAddress::getLocalAddress(ec)},
			 m_socket.getLocalEndpoint(ec).getPort());
	m_game.println(fmt::format("Meta server started. Use \"{}\" or \"{}\" to stop.", GET_COMMAND(disconnect).getName(), GET_COMMAND(quit).getName()));
	return true;
}

auto MetaServer::shutDown() -> void {
	INFO_MSG(Msg::SERVER, "Meta server: Shutting down.");

	// Save server config.
	m_game.awaitConsoleCommand(GET_COMMAND(meta_sv_writeconfig));
}

auto MetaServer::stop(std::string_view message) -> bool {
	if (!m_stopping) {
		INFO_MSG_INDENT(Msg::SERVER, "Meta server: Shutting down. Message: \"{}\".", message) {
			m_stopping = true;
			if (message.empty()) {
				m_game.println("Meta server shutting down.");
				message = "Meta server shutting down.";
			} else {
				m_game.println(fmt::format("Meta server shutting down. Message: {}", message));
			}

			for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
				this->disconnectClient(it, message);
			}
		}
		return true;
	}
	INFO_MSG(Msg::SERVER, "Meta server: Tried to stop when already stopping. Message: \"{}\".", message);
	return false;
}

auto MetaServer::update(float deltaTime) -> bool {
	DEBUG_MSG_INDENT(Msg::SERVER_TICK | Msg::CONNECTION_DETAILED, "META SERVER @ {} ms", deltaTime * 1000.0f) {
		if (m_stopping) {
			// Wait for all connections to close.
			if (m_clients.empty()) {
				return false;
			}
		}
		this->updateConfigAutoSave(deltaTime);
		this->receivePackets();
		this->updateConnections();
		this->updateTicks(deltaTime);
	}
	return true;
}

auto MetaServer::updateTimeout() -> void {
	const auto timeout = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(meta_sv_timeout)});
	for (auto& client : m_clients) {
		client.connection.setTimeout(timeout);
	}
}

auto MetaServer::updateThrottle() -> void {
	for (auto& client : m_clients) {
		client.connection.setThrottleMaxSendBufferSize(meta_sv_throttle_limit);
		client.connection.setThrottleMaxPeriod(meta_sv_throttle_max_period);
	}
}

auto MetaServer::updateSpamLimit() -> void {
	m_spamInterval = 1.0f / static_cast<float>(meta_sv_spam_limit);
	for (auto& client : m_clients) {
		client.spamCounter = 0;
	}
	m_spamTimer.reset();
}

auto MetaServer::updateTickrate() -> void {
	m_tickInterval = 1.0f / static_cast<float>(meta_sv_tickrate);
	m_tickTimer.reset();
}

auto MetaServer::updateConfigAutoSaveInterval() -> void {
	m_configAutoSaveInterval = static_cast<float>(meta_sv_config_auto_save_interval) * 60.0f;
	m_configAutoSaveTimer.reset();
}

auto MetaServer::updatePrivateAddressOverride() -> void {
	if (meta_sv_private_address_override.empty()) {
		m_hasPrivateAddressOverride = false;
		return;
	}
	auto ec = std::error_code{};
	const auto ip = net::IpAddress::resolve(std::string{meta_sv_private_address_override}.c_str(), ec);
	if (ec) {
		m_game.warning(fmt::format("Couldn't resolve private ip address override \"{}\": {}", meta_sv_private_address_override, ec.message()));
		m_hasPrivateAddressOverride = false;
		return;
	}
	m_privateAddressOverride = ip;
	m_hasPrivateAddressOverride = true;
}

auto MetaServer::getStatusString() const -> std::string {
	static constexpr auto formatClient = [](const auto& client) {
		const auto pingMilliseconds =
			std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(client.connection.getLatestMeasuredPingDuration()).count();
		return fmt::format(
			"{} \"{}\"\n"
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
			(client.heartbeatReceived) ? "Server" : "Client",
			std::string{client.connection.getRemoteEndpoint()},
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
		"=== META SERVER STATUS ===\n"
		"Local address: \"{}:{}\"\n"
		"Tickrate: {} Hz\n"
		"Clients:\n"
		"{}\n"
		"==========================",
		std::string{net::IpAddress::getLocalAddress(ec)},
		m_socket.getLocalEndpoint(ec).getPort(),
		meta_sv_tickrate,
		m_clients | util::transform(formatClient) | util::join("\n\n"));
}

auto MetaServer::kickClient(net::IpAddress ip) -> bool {
	if (const auto it = this->findClient(ip); it != m_clients.end()) {
		this->disconnectClient(it, "You were kicked from the server.");
		return true;
	}
	return false;
}

auto MetaServer::banClient(net::IpAddress ip) -> void {
	if (const auto it = this->findClient(ip); it != m_clients.end()) {
		this->disconnectClient(it, "You were kicked from the server.");
	}
	m_bannedClients.insert(ip);
}

auto MetaServer::unbanClient(net::IpAddress ip) -> bool {
	return m_bannedClients.erase(ip) > 0;
}

auto MetaServer::getBannedClients() const -> const BannedClients& {
	return m_bannedClients;
}

auto MetaServer::handleMessage(net::msg::in::Connect&&) -> void {
	if (this->testSpam()) {
		return;
	}

	INFO_MSG(Msg::SERVER, "Meta server: Client \"{}\" connected.", std::string{m_currentClient->connection.getRemoteEndpoint()});

	--m_connectingClients;
	m_currentClient->connecting = false;
}

auto MetaServer::handleMessage(msg::meta::sv::in::GameServerAddressListRequest&&) -> void {
	if (this->testSpam()) {
		return;
	}

	INFO_MSG(Msg::SERVER,
			 "Meta server: Received game server address list request from client \"{}\".",
			 std::string{m_currentClient->connection.getRemoteEndpoint()});

	m_currentClient->afkTimer.reset();
	if (!m_currentClient->connection.write<MetaClientOutputMessages>(msg::meta::cl::out::GameServerAddressList{{}, m_gameServerAddressList})) {
		this->disconnectClient(m_currentClient, "Failed to write game server address list.");
	}
}

auto MetaServer::handleMessage(msg::meta::sv::in::Heartbeat&&) -> void {
	if (this->testSpam()) {
		return;
	}
	m_currentClient->afkTimer.reset();
	if (!m_currentClient->heartbeatReceived) {
		auto endpoint = m_currentClient->connection.getRemoteEndpoint();
		INFO_MSG(Msg::SERVER, "Meta server: Received initial heartbeat from game server \"{}\". Adding to server list.", std::string{endpoint});
		m_currentClient->heartbeatReceived = true;
		if (m_hasPrivateAddressOverride && (endpoint.getAddress().isLoopback() || endpoint.getAddress().isPrivate())) {
			endpoint = net::IpEndpoint{m_privateAddressOverride, endpoint.getPort()};
		}
		m_currentClient->listedEndpoint = endpoint;
		m_gameServerAddressList.push_back(endpoint);
	}
}

auto MetaServer::testSpam() -> bool {
	assert(m_currentClient != m_clients.end());
	if (meta_sv_spam_limit != 0 && ++m_currentClient->spamCounter > meta_sv_spam_limit) {
		this->disconnectClient(m_currentClient, "Kicked for spamming commands too fast.");
		return true;
	}
	return false;
}

auto MetaServer::tick() -> void {
	if (m_heartbeatRequestTimer.advance(m_tickInterval, meta_sv_heartbeat_interval)) {
		for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
			if (it->heartbeatReceived) {
				if (!it->connection.write<GameServerOutputMessages>(msg::sv::out::HeartbeatRequest{{}})) {
					this->disconnectClient(it, "Failed to write heartbeat request.");
				}
			}
		}
	}
}

auto MetaServer::updateConfigAutoSave(float deltaTime) -> void {
	if (m_configAutoSaveTimer.advance(deltaTime, m_configAutoSaveInterval, meta_sv_config_auto_save_interval != 0, 1) != 0) {
		INFO_MSG(Msg::SERVER, "Auto-saving meta server config.");
		m_game.consoleCommand(GET_COMMAND(meta_sv_writeconfig));
	}
}

auto MetaServer::receivePackets() -> void {
	auto buffer = std::vector<std::byte>(net::MAX_PACKET_SIZE);
	while (true) {
		auto ec = std::error_code{};
		auto remoteEndpoint = net::IpEndpoint{};
		const auto receivedBytes = m_socket.receiveFrom(remoteEndpoint, buffer, ec).size();
		if (ec) {
			if (ec != net::SocketError::WAIT) {
				DEBUG_MSG(Msg::SERVER, "Meta server: Failed to receive packet: {}", ec.message());
			}
			break;
		}

		if (const auto it = this->findClient(remoteEndpoint); it != m_clients.end()) {
			buffer.resize(receivedBytes);
			it->connection.receivePacket(std::move(buffer));
			buffer.resize(net::MAX_PACKET_SIZE);
		} else if (m_connectingClients >= static_cast<std::size_t>(meta_sv_max_connecting_clients)) {
			DEBUG_MSG(
				Msg::CONNECTION_DETAILED,
				"Meta server: Ignoring {} bytes from unconnected ip \"{}\" because the max connecting client limit of {} has been reached!",
				receivedBytes,
				std::string{remoteEndpoint},
				static_cast<std::size_t>(meta_sv_max_connecting_clients));
		} else if (m_clients.size() >= static_cast<std::size_t>(meta_sv_max_clients)) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED,
					  "Meta server: Ignoring {} bytes from unconnected ip \"{}\" because the max client limit of {} has been reached!",
					  receivedBytes,
					  std::string{remoteEndpoint},
					  static_cast<std::size_t>(meta_sv_max_clients));
		} else if (m_stopping) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED,
					  "Meta server: Ignoring {} bytes from unconnected ip \"{}\" because the server is stopping!",
					  receivedBytes,
					  std::string{remoteEndpoint});
		} else {
			const auto timeout = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(meta_sv_timeout)});
			auto& client = m_clients.emplace_back(m_socket, timeout, meta_sv_throttle_limit, meta_sv_throttle_max_period, *this);
			INFO_MSG_INDENT(Msg::SERVER, "Meta server: Client \"{}\" connecting...", std::string{remoteEndpoint}) {
				if (!client.connection.accept(remoteEndpoint)) {
					INFO_MSG(Msg::SERVER,
							 "Meta server: Failed to intialize connection to \"{}\": {}",
							 std::string{remoteEndpoint},
							 client.connection.getDisconnectMessage());
					m_clients.pop_back();
				} else {
					++m_connectingClients;
					client.connecting = true;
					buffer.resize(receivedBytes);
					client.connection.receivePacket(std::move(buffer));
					buffer.resize(net::MAX_PACKET_SIZE);
					if (m_bannedClients.count(remoteEndpoint.getAddress()) != 0) {
						INFO_MSG(Msg::SERVER, "Meta server: This ip address is banned from the server. Kicking.");
						this->disconnectClient(std::prev(m_clients.end()), "You are banned from this meta server.");
					} else if (const auto maxClientsPerIp = static_cast<std::size_t>(meta_sv_max_connections_per_ip);
							   maxClientsPerIp != 0 && !remoteEndpoint.getAddress().isLoopback() && !remoteEndpoint.getAddress().isPrivate() &&
							   remoteEndpoint.getAddress() != net::IpAddress::getLocalAddress(ec) &&
							   this->countClientsWithIp(remoteEndpoint.getAddress()) > maxClientsPerIp) {
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

auto MetaServer::updateConnections() -> void {
	for (auto it = m_clients.begin(); it != m_clients.end();) {
		m_currentClient = it;
		if (!it->connection.update()) {
			this->dropClient(it);
			it = m_clients.erase(it);
		} else {
			++it;
		}
	}
	m_currentClient = m_clients.end();
}

auto MetaServer::updateTicks(float deltaTime) -> void {
	if (auto ticks = m_tickTimer.advance(deltaTime, m_tickInterval); ticks > 0) {
		const auto timeSinceLastTick = static_cast<float>(ticks) * m_tickInterval;
		if (ticks > meta_sv_max_ticks_per_frame) {
			INFO_MSG(Msg::SERVER | Msg::SERVER_TICK,
					 "Meta server: Framerate can't keep up with the tickrate! Skipping {} ms.",
					 (ticks - meta_sv_max_ticks_per_frame) * m_tickInterval * 1000.0f);
			ticks = meta_sv_max_ticks_per_frame;
		}

		// Tick.
		while (ticks-- > 0) {
			this->tick();
		}

		this->updateClients(timeSinceLastTick);
		this->sendPackets();
	}
}

auto MetaServer::updateClients(float deltaTime) -> void {
	const auto spamUpdates = m_spamTimer.advance(deltaTime, m_spamInterval);

	for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
		this->updateClient(it, deltaTime, spamUpdates);
	}
}

auto MetaServer::sendPackets() -> void {
	for (auto& client : m_clients) {
		client.connection.sendPackets();
	}
}

auto MetaServer::findClient(net::IpAddress ip) noexcept -> Clients::iterator {
	return util::findIf(m_clients, [ip](const auto& client) { return client.connection.getRemoteAddress() == ip; });
}

auto MetaServer::findClient(net::IpEndpoint endpoint) noexcept -> Clients::iterator {
	return util::findIf(m_clients, [endpoint](const auto& client) { return client.connection.getRemoteEndpoint() == endpoint; });
}

auto MetaServer::updateClient(Clients::iterator it, float deltaTime, int spamUpdates) -> void {
	assert(it != m_clients.end());

	this->updateClientSpamCounter(it, spamUpdates);
	this->updateClientAfkTimer(it, deltaTime);
}

auto MetaServer::updateClientSpamCounter(Clients::iterator it, int spamUpdates) -> void { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_clients.end());

	it->spamCounter = std::max(0, it->spamCounter - spamUpdates);
}

auto MetaServer::updateClientAfkTimer(Clients::iterator it, float deltaTime) -> void {
	assert(it != m_clients.end());

	if (it->afkTimer.advance(deltaTime, meta_sv_afk_autokick_time).first) {
		this->disconnectClient(it, "Kicked for inactivity.");
	}
}

auto MetaServer::disconnectClient(Clients::iterator it, std::string_view reason) -> void {
	assert(it != m_clients.end());

	util::erase(m_gameServerAddressList, it->listedEndpoint);

	const auto delay = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(meta_sv_disconnect_cooldown)});
	it->connection.disconnect(reason, delay);
}

auto MetaServer::dropClient(Clients::iterator it) -> void {
	assert(it != m_clients.end());

	if (it->connecting) {
		--m_connectingClients;
	}

	util::erase(m_gameServerAddressList, it->listedEndpoint);

	INFO_MSG(Msg::SERVER,
			 "Meta server: Client \"{}\" was dropped. Reason: \"{}\".",
			 std::string{it->connection.getRemoteEndpoint()},
			 it->connection.getDisconnectMessage());
}

auto MetaServer::countClientsWithIp(net::IpAddress ip) const noexcept -> std::size_t {
	return util::countIf(m_clients, [ip](const auto& client) { return client.connection.getRemoteAddress() == ip; });
}
