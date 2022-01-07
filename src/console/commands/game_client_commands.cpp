#include "game_client_commands.hpp"

#include "../../game/client/game_client.hpp" // GameClient
#include "../../network/config.hpp"          // net::MAX_SERVER_COMMAND_SIZE
#include "../../utilities/algorithm.hpp"     // util::filter, util::transform, util::collect
#include "../../utilities/file.hpp"          // util::dumpFile
#include "../../utilities/string.hpp"        // util::concat, util::ifind, util::toLower
#include "../command.hpp"                    // cmd::...
#include "../suggestions.hpp"                // Suggestions, SUGGESTIONS
#include "file_commands.hpp"                 // data_dir, data_subdir_cfg

#include <cassert>    // assert
#include <fmt/core.h> // fmt::format
#include <string>     // std::string
#include <vector>     // std::vector

namespace {

CONVAR_CALLBACK(updateTimeout) {
	if (client) {
		client->updateTimeout();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateThrottle) {
	if (client) {
		client->updateThrottle();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateCommandInterval) {
	if (client) {
		client->updateCommandInterval();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateUpdateRate) {
	if (client) {
		if (!client->updateUpdateRate()) {
			return cmd::error("{}: Failed to update client update rate.", self.getName());
		}
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateUsername) {
	if (client) {
		if (self.getRaw() != oldVal) {
			if (!client->updateUsername()) {
				return cmd::error("{}: Failed to update client username.", self.getName());
			}
		}
	}
	return cmd::done();
}

} // namespace

// clang-format off
ConVarIntMinMax		cl_updaterate{							"cl_updaterate",						0,					ConVar::CLIENT_SETTING,								"The maximum rate (in Hz) at which to request updates from the server. 0 = unlimited.", 0, 1000, updateUpdateRate};
ConVarIntMinMax		cl_cmdrate{								"cl_cmdrate",							60,					ConVar::CLIENT_SETTING,								"The rate (in Hz) at which to send packets to the server.", 1, 1000, updateCommandInterval};
ConVarBool			cl_hitsound_enable{						"cl_hitsound_enable",					true,				ConVar::CLIENT_SETTING,								"Play a sound when you inflict damage to an enemy."};
ConVarBool			cl_mouselook{							"cl_mouselook",							true,				ConVar::CLIENT_SETTING,								"Use the mouse to look around."};
ConVarBool			cl_draw_playernames_friendly{			"cl_draw_playernames_friendly",			true,				ConVar::CLIENT_SETTING,								"Show the names of teammates when you hover your mouse near them."};
ConVarBool			cl_draw_playernames_enemy{				"cl_draw_playernames_enemy",			false,				ConVar::CLIENT_SETTING,								"Show the names of enemies when you hover your mouse near them."};
ConVarBool			cl_draw_playernames_spectator{			"cl_draw_playernames_spectator",		true,				ConVar::CLIENT_SETTING,								"Show the names of players when you hover your mouse near them as a spectator."};
ConVarBool			cl_chat_enable{							"cl_chat_enable",						true,				ConVar::CLIENT_SETTING,								"Print chat messages received from other players."};
ConVarBool			cl_showscores{							"cl_showscores",						false,				ConVar::CLIENT_VARIABLE,							"Show scoreboard."};
ConVarBool			cl_showping{							"cl_showping",							false,				ConVar::CLIENT_SETTING,								"Show ping to the server."};
ConVarFloatMinMax	cl_timeout{								"cl_timeout",							10.0f,				ConVar::CLIENT_SETTING,								"How many seconds to wait before we assume that the server is not responding.", 0.0f, -1.0f, updateTimeout};
ConVarIntMinMax		cl_throttle_limit{						"cl_throttle_limit",					6,					ConVar::CLIENT_SETTING,								"How many packets are allowed to be queued in the client send buffer before throttling the outgoing send rate.", 0, -1, updateThrottle};
ConVarIntMinMax		cl_throttle_max_period{					"cl_throttle_max_period",				6,					ConVar::CLIENT_SETTING,								"Maximum number of packet sends to skip in a row while the client send rate is throttled.", 0, -1, updateThrottle};
ConVarBool			cl_allow_resource_download{				"cl_allow_resource_download",			true,				ConVar::CLIENT_SETTING,								"Whether or not to automatically download resources (like the map) when connecting to a server."};
ConVarIntMinMax		cl_max_resource_download_size{			"cl_max_resource_download_size",		500000,				ConVar::CLIENT_SETTING,								"Maximum size (in bytes) that is allowed for a single resource when downloading from the server (0 = unlimited).", 0, -1};
ConVarIntMinMax		cl_max_resource_total_download_size{	"cl_max_resource_total_download_size",	1000000000,			ConVar::CLIENT_SETTING,								"Maximum total sum of resource sizes (in bytes) to download from the server (0 = unlimited).", 0, -1};
ConVarString		address{								"address",								"",					ConVar::CLIENT_SETTING | ConVar::NOT_RUNNING_GAME,	"Remote address to connect to."};
ConVarIntMinMax		port{									"port",									0,					ConVar::CLIENT_SETTING | ConVar::NOT_RUNNING_GAME,	"Remote port to connect to.", 0, 65535};
ConVarIntMinMax		cl_port{								"cl_port",								0,					ConVar::CLIENT_SETTING | ConVar::NOT_RUNNING_GAME,	"Port used by the client. Set to 0 to choose automatically.", 0, 65535};
ConVarString		username{								"username",								"",					ConVar::CLIENT_SETTING,								"Player username.", updateUsername};
ConVarString		password{								"password",								"",					ConVar::CLIENT_PASSWORD,							"Password to use when connecting to a server. Server hosts should use sv_password."};
ConVarString		cl_config_file{							"cl_config_file",						"cl_config.cfg",	ConVar::CLIENT_VARIABLE,							"Main client config file to read at startup and save to at shutdown."};
ConVarString		cl_autoexec_file{						"cl_autoexec_file",						"cl_autoexec.cfg",	ConVar::CLIENT_VARIABLE,							"Client autoexec file to read at startup."};
ConVarBool			cl_crosshair_enable{					"cl_crosshair_enable",					true,				ConVar::CLIENT_SETTING,								"Draw the crosshair."};
ConVarChar			cl_crosshair{							"cl_crosshair",							'+',				ConVar::CLIENT_SETTING,								"How to draw the crosshair."};
ConVarColor			cl_crosshair_color{						"cl_crosshair_color",					Color::orange(),	ConVar::CLIENT_SETTING,								"How to color the crosshair when it's not set to be team colored."};
ConVarBool			cl_crosshair_use_team_color{			"cl_crosshair_use_team_color",			false,				ConVar::CLIENT_SETTING,								"Color the crosshair using the color of the team you're currently on."};
ConVarBool			cl_crosshair_distance_follow_cursor{	"cl_crosshair_distance_follow_cursor",	true,				ConVar::CLIENT_SETTING,								"Draw the crosshair as close to the mouse cursor as possible."};
ConVarFloatMinMax	cl_crosshair_min_distance{				"cl_crosshair_min_distance",			4.0f,				ConVar::CLIENT_SETTING,								"Minimum crosshair distance.", 1.0f, -1.0f};
ConVarFloatMinMax	cl_crosshair_max_distance{				"cl_crosshair_max_distance",			12.0f,				ConVar::CLIENT_SETTING,								"Maximum crosshair distance.", 1.0f, -1.0f};
ConVarBool			cl_crosshair_collide_world{				"cl_crosshair_collide_world",			true,				ConVar::CLIENT_SETTING,								"Block the crosshair if it hits a wall."};
ConVarBool			cl_crosshair_collide_viewport{			"cl_crosshair_collide_viewport",		true,				ConVar::CLIENT_SETTING,								"Block the crosshair if it hits the edges of the screen."};
ConVarIntMinMax		cl_crosshair_viewport_border{			"cl_crosshair_viewport_border",			2,					ConVar::CLIENT_SETTING,								"If set to collide with the viewport, the crosshair will collide at this distance from the edges.", 0, -1};

ConVarString		cl_chars_explosion{						"cl_chars_explosion",					"xXxXxXxXx",		ConVar::CLIENT_VARIABLE,							"How to draw an explosion."};
ConVarString		cl_gun_sentry{							"cl_gun_sentry",						"\\\"/=\\\"/=",		ConVar::CLIENT_VARIABLE,							"How to draw the gun for a sentry gun."};

ConVarChar			cl_char_player{							"cl_char_player",						'@',				ConVar::CLIENT_VARIABLE,							"How to draw a player."};
ConVarChar			cl_char_corpse{							"cl_char_corpse",						'X',				ConVar::CLIENT_VARIABLE,							"How to draw a corpse."};
ConVarChar			cl_char_sentry{							"cl_char_sentry",						'O',				ConVar::CLIENT_VARIABLE,							"How to draw a sentry gun."};
ConVarChar			cl_char_medkit{							"cl_char_medkit",						'+',				ConVar::CLIENT_VARIABLE,							"How to draw a medkit."};
ConVarChar			cl_char_ammopack{						"cl_char_ammopack",						'a',				ConVar::CLIENT_VARIABLE,							"How to draw an ammopack."};
ConVarChar			cl_char_flag{							"cl_char_flag",							'!',				ConVar::CLIENT_VARIABLE,							"How to draw an flag."};
ConVarChar			cl_char_respawnvis{						"cl_char_respawnvis",					'x',				ConVar::CLIENT_VARIABLE,							"How to draw a respawn room visualizer."};
ConVarChar			cl_char_resupply{						"cl_char_resupply",						'$',				ConVar::CLIENT_VARIABLE,							"How to draw a resupply locker."};
ConVarChar			cl_char_cart{							"cl_char_cart",							'P',				ConVar::CLIENT_VARIABLE,							"How to draw a payload cart."};
ConVarChar			cl_char_track{							"cl_char_track",						'.',				ConVar::CLIENT_VARIABLE,							"How to draw a payload cart track."};

ConVarColor			cl_color_world{							"cl_color_world",						Color::white(),		ConVar::CLIENT_VARIABLE,							"How to color solid parts of the map."};
ConVarColor			cl_color_non_solid{						"cl_color_non_solid",					Color::gray(),		ConVar::CLIENT_VARIABLE,							"How to color non-solid parts of the map."};
ConVarColor			cl_color_respawnvis{					"cl_color_respawnvis",					Color::red(),		ConVar::CLIENT_VARIABLE,							"How to color a respawn room visualizer."};
ConVarColor			cl_color_resupply{						"cl_color_resupply",					Color::gray(),		ConVar::CLIENT_VARIABLE,							"How to color a resupply locker."};
ConVarColor			cl_color_track{							"cl_color_track",						Color::dark_gray(),	ConVar::CLIENT_VARIABLE,							"How to color the payload track."};
ConVarColor			cl_color_name{							"cl_color_name",						Color::dark_gray(),	ConVar::CLIENT_VARIABLE,							"How to color player names."};
ConVarColor			cl_color_medkit{						"cl_color_medkit",						Color::lime(),		ConVar::CLIENT_VARIABLE,							"How to color a medkit."};
ConVarColor			cl_color_ammopack{						"cl_color_ammopack",					Color::gray(),		ConVar::CLIENT_VARIABLE,							"How to color an ammo pack."};
ConVarColor			cl_color_timer{							"cl_color_timer",						Color::gray(),		ConVar::CLIENT_VARIABLE,							"How to color the round timer."};
ConVarColor			cl_color_health{						"cl_color_health",						Color::lime(),		ConVar::CLIENT_VARIABLE,							"How to color the player health."};
ConVarColor			cl_color_low_health{					"cl_color_low_health",					Color::red(),		ConVar::CLIENT_VARIABLE,							"How to color the player health when it's low."};
ConVarColor			cl_color_ammo{							"cl_color_ammo",						Color::gray(),		ConVar::CLIENT_VARIABLE,							"How to color the player ammo."};
// clang-format on

namespace {

auto getValidTeams() -> std::vector<std::string> {
	static constexpr auto isValidTeam = [](const auto& team) {
		return team != Team::none();
	};
	static constexpr auto formatTeam = [](const auto& team) {
		return util::toLower(team.getName());
	};
	auto teams = Team::getAll() | util::filter(isValidTeam) | util::transform(formatTeam) | util::collect<std::vector<std::string>>();
	teams.emplace_back("auto");
	teams.emplace_back("random");
	return teams;
}

auto getValidClasses() -> std::vector<std::string> {
	static constexpr auto isValidClass = [](const auto& playerClass) {
		return playerClass != PlayerClass::none() && playerClass != PlayerClass::spectator();
	};

	static constexpr auto formatClass = [](const auto& playerClass) {
		return util::toLower(playerClass.getName());
	};

	auto playerClasses = PlayerClass::getAll() | util::filter(isValidClass) | util::transform(formatClass) |
						 util::collect<std::vector<std::string>>();
	playerClasses.emplace_back("auto");
	playerClasses.emplace_back("random");
	return playerClasses;
}

SUGGESTIONS(suggestTeam) {
	if (i == 1) {
		return Suggestions{getValidTeams()};
	}
	return Suggestions{};
}

SUGGESTIONS(suggestClass) {
	if (i == 1) {
		return Suggestions{getValidClasses()};
	}
	return Suggestions{};
}

} // namespace

CON_COMMAND(cl_player_id, "", ConCommand::CLIENT, "Get the player id of the local player.", {}, nullptr) {
	assert(client);
	return cmd::done(client->getPlayerId());
}

CON_COMMAND(team_menu, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Toggle the team select menu.", {}, nullptr) {
	assert(client);
	client->toggleTeamSelect();
	return cmd::done();
}

CON_COMMAND(class_menu, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Toggle the class select menu.", {}, nullptr) {
	assert(client);
	client->toggleClassSelect();
	return cmd::done();
}

CON_COMMAND(team, "<name>", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Choose team.", {}, suggestTeam) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	assert(client);

	if (constexpr auto autoStr = std::string_view{"auto"}; util::ifind(autoStr, argv[1]) == 0) {
		if (!client->teamSelectAuto()) {
			return cmd::error("{}: Team select failed.", self.getName());
		}
		return cmd::done();
	}

	if (constexpr auto randomStr = std::string_view{"random"}; util::ifind(randomStr, argv[1]) == 0) {
		if (!client->teamSelectRandom()) {
			return cmd::error("{}: Team select failed.", self.getName());
		}
		return cmd::done();
	}

	if (const auto& team = Team::findByName(argv[1]); team != Team::none()) {
		if (!client->teamSelect(team)) {
			return cmd::error("{}: Team select failed.", self.getName());
		}
		return cmd::done();
	}

	static constexpr auto formatTeam = [](const auto& team) {
		return fmt::format("\n  {}", team);
	};

	return cmd::error("{}: Invalid team \"{}\". Valid teams are:{}", self.getName(), argv[1], getValidTeams() | util::transform(formatTeam) | util::concat());
}

CON_COMMAND(class, "<name>", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Choose class.", {}, suggestClass) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	assert(client);
	if (!client->hasSelectedTeam()) {
		return cmd::error("Please select team first!");
	}

	if (constexpr auto autoStr = std::string_view{"auto"}; util::ifind(autoStr, argv[1]) == 0) {
		if (!client->classSelectAuto()) {
			return cmd::error("{}: Class select failed.", self.getName());
		}
		return cmd::done();
	}

	if (constexpr auto randomStr = std::string_view{"random"}; util::ifind(randomStr, argv[1]) == 0) {
		if (!client->classSelectRandom()) {
			return cmd::error("{}: Class select failed.", self.getName());
		}
		return cmd::done();
	}

	if (const auto& playerClass = PlayerClass::findByName(argv[1]); playerClass != PlayerClass::none() && playerClass != PlayerClass::spectator()) {
		if (!client->classSelect(playerClass)) {
			return cmd::error("{}: Class select failed.", self.getName());
		}
		return cmd::done();
	}

	static constexpr auto formatPlayerClass = [](const auto& playerClass) {
		return fmt::format("\n  {}", playerClass);
	};

	return cmd::error("{}: Invalid class \"{}\". Valid classes are:{}",
					  self.getName(),
					  argv[1],
					  getValidClasses() | util::transform(formatPlayerClass) | util::concat());
}

CON_COMMAND(fwd, "<command...>", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Forward an arbitrary command to the server.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	if (argv.size() - 1 > net::MAX_SERVER_COMMAND_SIZE) {
		return cmd::error("{}: Command is too long ({}/{} args).", self.getName(), argv.size() - 1, net::MAX_SERVER_COMMAND_SIZE);
	}

	assert(client);
	if (!client->forwardCommand(argv.subCommand(1))) {
		return cmd::error("{}: Failed to send command.", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(is_connected, "", ConCommand::NO_FLAGS, "Check if the client is connected.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(client && client->hasJoinedGame());
}

CON_COMMAND(cl_writeconfig, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Save the current client config.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	assert(client);
	if (!util::dumpFile(fmt::format("{}/{}/{}", data_dir, data_subdir_cfg, cl_config_file),
						fmt::format("{}\n"
									"\n"
									"// Inventories:\n"
									"{}\n",
									GameClient::getConfigHeader(),
									client->getInventoryConfig()))) {
		return cmd::error("{}: Failed to save config file \"{}\"!", self.getName(), cl_config_file);
	}
	return cmd::done();
}
