#include "game_server_commands.hpp"

#include "../../game/data/player_class.hpp"  // PlayerClass
#include "../../game/data/team.hpp"          // Team
#include "../../game/server/game_server.hpp" // GameServer
#include "../../network/config.hpp"          // net::MAX_USERNAME_LENGTH
#include "../../network/endpoint.hpp"        // net::IpEndpoint
#include "../../utilities/algorithm.hpp"     // util::collect, util::sort, util::transform
#include "../../utilities/file.hpp"          // util::dumpFile
#include "../../utilities/reference.hpp"     // util::Reference
#include "../../utilities/string.hpp"        // util::contains, util::join
#include "../command.hpp"                    // cmd::...
#include "../command_utilities.hpp"          // cmd::...
#include "../script.hpp"                     // Script
#include "../suggestions.hpp"                // Suggestions, SUGGESTIONS
#include "file_commands.hpp"                 // data_dir, data_subdir_maps, data_subdir_cfg
#include "game_commands.hpp"                 // maplist

#include <cassert>      // assert
#include <chrono>       // std::chrono::...
#include <filesystem>   // std::filesystem::...
#include <fmt/core.h>   // fmt::format
#include <optional>     // std::optional
#include <string>       // std::string
#include <system_error> // std::error_code

namespace {

CONVAR_CALLBACK(updateTimeout) {
	if (server) {
		server->updateTimeout();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateThrottle) {
	if (server) {
		server->updateThrottle();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateSpamLimit) {
	if (server) {
		server->updateSpamLimit();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateTickrate) {
	if (server) {
		server->updateTickrate();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateBotTickrate) {
	if (server) {
		server->updateBotTickrate();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateBotAiEnable) {
	if (server && !sv_bot_ai_enable && oldVal != "0") {
		server->freezeBots();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateBotAiRequirePlayers) {
	if (server && sv_bot_ai_require_players && oldVal == "0" && !server->hasPlayers()) {
		server->freezeBots();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateMapName) {
	if (!self.getRaw().empty() && !util::contains(self.getRaw(), '.')) {
		self.setSilent(fmt::format("{}.txt", self.getRaw()));
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateConfigAutoSaveInterval) {
	if (server) {
		server->updateConfigAutoSaveInterval();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateResourceUploadInterval) {
	if (server) {
		server->updateResourceUploadInterval();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateAllowResourceDownload) {
	if (server) {
		server->updateAllowResourceDownload();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateMetaSubmit) {
	if (server) {
		server->updateMetaSubmit();
	}
	return cmd::done();
}

constexpr auto DISCONNECT_DURATION_SECONDS = std::chrono::duration_cast<std::chrono::duration<float>>(net::DISCONNECT_DURATION).count();

} // namespace

// clang-format off
ConVarBool			sv_cheats{						"sv_cheats",						false,											ConVar::SHARED_VARIABLE,							"Allow cheats on the current server."};
ConVarIntMinMax		sv_spam_limit{					"sv_spam_limit",					4,												ConVar::SERVER_SETTING,								"Maximum number of potential spam messages per second to receive before kicking the sender. 0 = unlimited.", 0, -1, updateSpamLimit};
ConVarIntMinMax		sv_tickrate{					"sv_tickrate",						60,												ConVar::SERVER_SETTING,								"The rate (in Hz) at which the server updates.", 1, 1000, updateTickrate};
ConVarIntMinMax		sv_bot_tickrate{				"sv_bot_tickrate",					10,												ConVar::SERVER_SETTING,								"The rate (in Hz) at which bots think. Should be a divisor of sv_tickrate for best results.", 1, 1000, updateBotTickrate};
ConVarBool			sv_bot_ai_enable{				"sv_bot_ai_enable",					true,											ConVar::SERVER_VARIABLE,							"Whether or not to tick bots.", updateBotAiEnable};
ConVarBool			sv_bot_ai_require_players{		"sv_bot_ai_require_players",		true,											ConVar::SERVER_SETTING,								"Whether or not there needs to be players connected to the server in order for bots to update.", updateBotAiRequirePlayers};
ConVarIntMinMax		sv_max_ticks_per_frame{			"sv_max_ticks_per_frame",			10,												ConVar::SERVER_SETTING,								"How many ticks that are allowed to run on one server frame.", 1, -1};
ConVarIntMinMax		sv_playerlimit{					"sv_playerlimit",					24,												ConVar::SERVER_SETTING,								"How many clients are allowed to connect to the server.", 1, 65535};
ConVarIntMinMax		sv_max_username_length{			"sv_max_username_length",			static_cast<int>(net::MAX_USERNAME_LENGTH),		ConVar::SERVER_SETTING,								"Maximum username length for connecting clients.", 1, static_cast<int>(net::MAX_USERNAME_LENGTH)};
ConVarFloatMinMax	sv_disconnect_cooldown{			"sv_disconnect_cooldown",			DISCONNECT_DURATION_SECONDS,					ConVar::SERVER_SETTING,								"How many seconds to wait before letting a client connect again after disconnecting.", 0.0f, -1.0f};
ConVarFloatMinMax	sv_timeout{						"sv_timeout",						10.0f,											ConVar::SERVER_SETTING,								"How many seconds to wait before booting a client that isn't sending messages.", 0.0f, -1.0f, updateTimeout};
ConVarIntMinMax		sv_throttle_limit{				"sv_throttle_limit",				6,												ConVar::SERVER_SETTING,								"How many packets are allowed to be queued in the server send buffer before throttling the outgoing send rate.", 0, -1, updateThrottle};
ConVarIntMinMax		sv_throttle_max_period{			"sv_throttle_max_period",			6,												ConVar::SERVER_SETTING,								"Maximum number of packet sends to skip in a row while the server send rate is throttled.", 0, -1, updateThrottle};
ConVarString		sv_hostname{					"sv_hostname",						"",												ConVar::SERVER_SETTING,								"What name to display your server as."};
ConVarBool			sv_allow_resource_download{		"sv_allow_resource_download",		true,											ConVar::SERVER_SETTING,								"Whether or not to let clients download resources from your server.", updateAllowResourceDownload};
ConVarFloatMinMax	sv_resource_upload_rate{		"sv_resource_upload_rate",			10000.0f,										ConVar::SERVER_SETTING,								"Rate (in bytes per second) at which resources are uploaded to clients.", 1.0f, -1.0f, updateResourceUploadInterval};
ConVarIntMinMax		sv_resource_upload_chunk_size{	"sv_resource_upload_chunk_size",	1000,											ConVar::SERVER_SETTING,								"How big (in bytes) chunks to split resources up into when uploading to clients.", 1, -1, updateResourceUploadInterval};
ConVarHashed		sv_password{					"sv_password",						"",												ConVar::SERVER_PASSWORD,							"Server password for connecting clients."};
ConVarBool			sv_rtv_enable{					"sv_rtv_enable",					true,											ConVar::SERVER_SETTING,								"Whether or not vote-based map switching is enabled."};
ConVarFloatMinMax	sv_rtv_delay{					"sv_rtv_delay",						20.0f,											ConVar::SERVER_SETTING,								"How many seconds to wait after switching maps before allowing players to rock the vote again.", 0.0f, -1.0f};
ConVarFloatMinMax	sv_rtv_needed{					"sv_rtv_needed",					0.6f,											ConVar::SERVER_SETTING,								"Fraction of all connected players whose consent is needed to rock the vote.", 0.0f, 1.0f};
ConVarString		sv_nextlevel{					"sv_nextlevel",						"",												ConVar::SERVER_VARIABLE,							"The next map to switch to."};
ConVarString		sv_map{							"sv_map",							"",												ConVar::SERVER_SETTING | ConVar::NOT_RUNNING_GAME,	"Map to use when starting a server.", updateMapName};
ConVarIntMinMax		sv_bot_count{					"sv_bot_count",						10,												ConVar::SERVER_SETTING,								"Number of bots to add when starting a map.", 0, 65535};
ConVarIntMinMax		sv_port{						"sv_port",							25605,											ConVar::SERVER_SETTING | ConVar::NOT_RUNNING_GAME,	"Local port to use when starting a server.", 0, 65535};
ConVarString		sv_config_file{					"sv_config_file",					"sv_config.cfg",								ConVar::HOST_SETTING,								"Main server config file to read at startup and save to at shutdown."};
ConVarString		sv_autoexec_file{				"sv_autoexec_file",					"sv_autoexec.cfg",								ConVar::HOST_SETTING,								"Server autoexec file to read at startup."};
ConVarString		sv_map_rotation{				"sv_map_rotation",					"",												ConVar::SERVER_SETTING,								"Server map rotation for rock the vote. Map names are separated in the same way as commands."};
ConVarString		sv_motd{						"sv_motd",							"",												ConVar::SERVER_SETTING,								"Server message of the day. Shown to clients when they have successfully connected."};
ConVarIntMinMax		sv_max_clients{					"sv_max_clients",					65536,											ConVar::SERVER_SETTING,								"Maximum number of connections to handle simultaneously. When the limit is hit, any remaining packets received from unconnected addresses will be ignored.", 0, -1};
ConVarIntMinMax		sv_max_connecting_clients{		"sv_max_connecting_clients",		10,												ConVar::SERVER_SETTING,								"Maximum number of new connections to handle simultaneously. When the limit is hit, any remaining packets received from unconnected addresses will be ignored.", 0, -1};
ConVarIntMinMax		sv_config_auto_save_interval{	"sv_config_auto_save_interval",		5,												ConVar::SERVER_SETTING,								"Minutes between automatic server config saves. 0 = Disable autosave.", 0, -1, updateConfigAutoSaveInterval};
ConVarIntMinMax		sv_score_level_interval{		"sv_score_level_interval",			20,												ConVar::SERVER_SETTING,								"Number of points required to level up.", 1, -1};
ConVarFloatMinMax	sv_afk_autokick_time{			"sv_afk_autokick_time",				60.0f,											ConVar::SERVER_SETTING,								"Automatically kick players if they haven't done anything for this many seconds (0 = unlimited).", 0.0f, -1.0f};
ConVarIntMinMax		sv_max_connections_per_ip{		"sv_max_connections_per_ip",		10,												ConVar::SERVER_SETTING,								"Maximum number of connections to accept from the same IP address (0 = unlimited).", 0, -1};
ConVarIntMinMax		sv_max_players_per_ip{			"sv_max_players_per_ip",			1,												ConVar::SERVER_SETTING,								"Maximum number of players to accept from the same IP address (0 = unlimited).", 0, -1};
ConVarBool			sv_meta_submit{					"sv_meta_submit",					false,											ConVar::SERVER_SETTING,								"Whether or not your game server should connect to the meta server and advertise itself on the public server list.", updateMetaSubmit};
ConVarBool			sv_meta_submit_retry{			"sv_meta_submit_retry",				true,											ConVar::SERVER_SETTING,								"Whether or not your game server should retry the connection to the meta server in case it fails.", updateMetaSubmit};
ConVarFloatMinMax	sv_meta_submit_retry_interval{	"sv_meta_submit_retry_interval",	60.0f,											ConVar::SERVER_SETTING,								"How many seconds to wait between meta server reconnection attempts.", 3.0f, -1.0f};
// clang-format on

namespace {

SUGGESTIONS(suggestBotName) {
	if (i == 1 && server) {
		auto bots = server->getBotNames();
		bots.emplace_back("all");
		return util::collect<Suggestions>(bots);
	}
	return Suggestions{};
}

} // namespace

CON_COMMAND(changelevel, "<map>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Change map while the server is running.", {},
			Suggestions::suggestMap<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (auto result = sv_map.set(argv[1], game, server, client, metaServer, metaClient); result.status == cmd::Status::ERROR_MSG) {
		return result;
	}

	const auto filepath = fmt::format("{}/{}/{}", data_dir, data_subdir_maps, sv_map);
	if (!std::filesystem::is_regular_file(filepath)) {
		return cmd::error("{}: Map \"{}\" not found. Try \"{}\".", self.getName(), sv_map, GET_COMMAND(maplist).getName());
	}

	assert(server);
	server->changeLevel();
	return cmd::done();
}

CON_COMMAND(bot_add, "[options...]", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Add one or more computer-controlled clients.",
			cmd::opts(cmd::opt('c', "count", "Number of bots to add.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('n', "name", "Bot name.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('t', "team", "Bot team.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('p', "class", "Bot class.", cmd::OptionType::ARGUMENT_REQUIRED)),
			nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (!args.empty()) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	auto count = std::size_t{1};
	if (const auto countStr = options['c']) {
		auto parseError = cmd::ParseError{};

		count = cmd::parseNumber<std::size_t>(parseError, *countStr, "count");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}
	}

	auto name = std::string{"1"};
	if (const auto nameStr = options['n']) {
		name = *nameStr;
	}

	auto team = Team::none();
	if (const auto teamStr = options['t']) {
		team = Team::findByName(*teamStr);
		if (team == Team::none() || team == Team::spectators()) {
			return cmd::error("{}: Invalid team \"{}\".", self.getName(), *teamStr);
		}
	}

	auto playerClass = PlayerClass::none();
	if (const auto playerClassStr = options['p']) {
		playerClass = PlayerClass::findByName(*playerClassStr);
		if (playerClass == PlayerClass::none() || playerClass == PlayerClass::spectator()) {
			return cmd::error("{}: Invalid class \"{}\".", self.getName(), *playerClassStr);
		}
	}

	assert(server);
	for (auto i = std::size_t{0}; i < count; ++i) {
		if (!server->addBot(name, team, playerClass)) {
			return cmd::error("{}: Failed to add bot #{}.", self.getName(), i + 1);
		}
	}
	return cmd::done();
}

CON_COMMAND(bot_kick, "<name/all>", ConCommand::SERVER, "Kick one or all bots from the server.", {}, suggestBotName) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	if (argv[1] == "all") {
		server->kickAllBots();
	} else {
		if (!server->kickBot(argv[1])) {
			return cmd::error("{}: Bot \"{}\" not found!", self.getName(), argv[1]);
		}
	}
	return cmd::done();
}

CON_COMMAND(sv_has_players, "", ConCommand::SERVER, "Check if the server has any non-bot players.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	return cmd::done(server->hasPlayers());
}

CON_COMMAND(sv_rtv, "<ip>", ConCommand::SERVER, "Have the client with a certain ip rock the vote.", {}, cmd::suggestConnectedClientIp<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto endpoint = cmd::parseIpEndpoint(parseError, argv[1], "ip");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (!server->rockTheVote(endpoint)) {
		return cmd::error("{}: Player with ip \"{}\" not found.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(sv_write_output, "<ip> <message>", ConCommand::SERVER, "Write a server command output message to the client with a certain ip.",
			{}, cmd::suggestConnectedClientIp<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto endpoint = cmd::parseIpEndpoint(parseError, argv[1], "ip");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (!server->writeCommandOutput(endpoint, argv[2])) {
		return cmd::error("{}: Player with ip \"{}\" not found.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(sv_write_error, "<ip> <message>", ConCommand::SERVER, "Write a server command error message to the client with a certain ip.",
			{}, cmd::suggestConnectedClientIp<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto endpoint = cmd::parseIpEndpoint(parseError, argv[1], "ip");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (!server->writeCommandError(endpoint, argv[2])) {
		return cmd::error("{}: Player with ip \"{}\" not found.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(sv_kick, "<name/ip>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Kick a player from the server.", {}, cmd::suggestPlayerName<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	if (!server->kickPlayer(argv[1])) {
		return cmd::error("{}: User not found.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(sv_ban, "<name/ip> [username]", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Ban a player from the server.", {},
			cmd::suggestPlayerName<1>) {
	if (argv.size() != 2 && argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	if (!server->banPlayer(argv[1], (argv.size() == 3) ? std::optional<std::string>{argv[2]} : std::nullopt)) {
		return cmd::error("{}: Player \"{}\" not found. Provide a username to ban by ip instead.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(sv_unban, "<ip>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Remove an ip address from the server's banned user list.", {},
			cmd::suggestBannedPlayerIpAddress<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};
	const auto ip = net::IpAddress::parse(argv[1], ec);
	if (ec) {
		return cmd::error("{}: Couldn't parse ip address \"{}\": {}", self.getName(), argv[1], ec.message());
	}
	assert(server);
	if (!server->unbanPlayer(ip)) {
		return cmd::error("{}: Ip address \"{}\" is not banned. Use \"{}\" for a list of banned ips.",
						  self.getName(),
						  std::string{ip},
						  GET_COMMAND(sv_ban_list).getName());
	}

	return cmd::done();
}

CON_COMMAND(sv_ban_list, "", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "List all banned ips on the server.", {}, nullptr) {
	static constexpr auto getBannedPlayerIp = [](const auto& kv) {
		return std::string{kv.first};
	};

	assert(server);
	return cmd::done(server->getBannedPlayers() | util::transform(getBannedPlayerIp) | util::join('\n'));
}

CON_COMMAND(sv_writeconfig, "", ConCommand::SERVER | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Save the current server config.", {}, nullptr) {
	using BannedPlayerRef = util::Reference<const GameServer::BannedPlayers::value_type>;
	using Refs = std::vector<BannedPlayerRef>;

	static constexpr auto compareBannedPlayerRefs = [](const auto& lhs, const auto& rhs) {
		return lhs->second.username < rhs->second.username;
	};

	static constexpr auto getBannedPlayerCommand = [](const auto& kv) {
		const auto& [ip, bannedPlayer] = *kv;
		return fmt::format("{} {} {}",
						   GET_COMMAND(sv_ban).getName(),
						   Script::escapedString(std::string{ip}),
						   Script::escapedString(bannedPlayer.username));
	};

	assert(server);
	if (!util::dumpFile(fmt::format("{}/{}/{}", data_dir, data_subdir_cfg, sv_config_file),
						fmt::format("{}\n"
									"\n"
									"// Inventories:\n"
									"{}\n"
									"// Remote console:\n"
									"{}\n"
									"\n"
									"// Banned IPs:\n"
									"{}\n",
									GameServer::getConfigHeader(),
									server->getInventoryConfig(),
									server->getRconConfig(),
									server->getBannedPlayers() | util::collect<Refs>() | util::sort(compareBannedPlayerRefs) |
										util::transform(getBannedPlayerCommand) | util::join('\n')))) {
		return cmd::error("{}: Failed to save config file \"{}\"!", self.getName(), sv_config_file);
	}
	return cmd::done();
}
