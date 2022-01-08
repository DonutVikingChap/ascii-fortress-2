#include "world_commands.hpp"

#include "../../game/data/player_class.hpp"    // PlayerClass
#include "../../game/data/player_id.hpp"       // PlayerId
#include "../../game/data/projectile_type.hpp" // ProjectileType
#include "../../game/data/score.hpp"           // Score
#include "../../game/data/sound_id.hpp"        // SoundId
#include "../../game/data/team.hpp"            // Team
#include "../../game/data/weapon.hpp"          // Weapon
#include "../../game/server/game_server.hpp"   // GameServer
#include "../../game/server/world.hpp"         // World
#include "../../network/endpoint.hpp"          // net::IpEndpoint
#include "../../utilities/algorithm.hpp"       // util::subview, util::transform, util::collect
#include "../command.hpp"                      // cmd::...
#include "../command_utilities.hpp"            // cmd::...

#include <cassert> // assert
#include <string>  // std::string
#include <vector>  // std::vector

// clang-format off
ConVarFloatMinMax	mp_player_respawn_time{							"mp_player_respawn_time",							3.0f,		ConVar::SERVER_VARIABLE,	"Player respawn time in seconds.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_round_end_time{								"mp_round_end_time",								6.0f,		ConVar::SERVER_VARIABLE,	"How long after a round has ended before the next round starts.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_flag_return_time{							"mp_flag_return_time",								10.0f,		ConVar::SERVER_VARIABLE,	"How many seconds before the flag gets returned after being dropped.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_medkit_respawn_time{							"mp_medkit_respawn_time",							10.0f,		ConVar::SERVER_VARIABLE,	"Medkit respawn time in seconds.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_ammopack_respawn_time{						"mp_ammopack_respawn_time",							10.0f,		ConVar::SERVER_VARIABLE,	"Ammopack respawn time in seconds.", 0.0f, -1.0f};
ConVarIntMinMax		mp_ctf_capture_limit{							"mp_ctf_capture_limit",								3,			ConVar::SERVER_VARIABLE,	"How many times a team has to capture the flag to win.", 1, -1};
ConVarIntMinMax		mp_limitteams{									"mp_limitteams",									2,			ConVar::SERVER_VARIABLE,	"Force players to the team with less players when the difference would otherwise be higher than this value. 0 = disable.", 0, -1};
ConVarFloatMinMax	mp_explosion_disappear_time{					"mp_explosion_disappear_time",						0.25f,		ConVar::SERVER_VARIABLE,	"Time taken for an explosion to disappear.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_sentry_despawn_time{							"mp_sentry_despawn_time",							3.0f,		ConVar::SERVER_VARIABLE,	"Sentry gun corpse despawn time in seconds.", 0.0f, -1.0f};
ConVarIntMinMax		mp_sentry_health{								"mp_sentry_health",									150,		ConVar::SERVER_VARIABLE,	"Sentry gun HP when spawning.", 1, -1};
ConVarIntMinMax		mp_sentry_range{								"mp_sentry_range",									16,			ConVar::SERVER_VARIABLE,	"Sentry gun enemy detection radius.", 0, -1};
ConVarFloatMinMax	mp_sentry_build_time{							"mp_sentry_build_time",								1.5f,		ConVar::SERVER_VARIABLE,	"Time taken to build a sentry gun.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_roundtime_ctf{								"mp_roundtime_ctf",									1200.0f,	ConVar::SERVER_VARIABLE,	"How many seconds a Capture The Flag round should last.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_roundtime_payload{							"mp_roundtime_payload",								600.0f,		ConVar::SERVER_VARIABLE,	"How many seconds a Payload round should last.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_roundtime_tdm{								"mp_roundtime_tdm",									600.0f,		ConVar::SERVER_VARIABLE,	"How many seconds a Team Deathmatch round should last.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_payload_cart_push_time{						"mp_payload_cart_push_time",						0.5f,		ConVar::SERVER_VARIABLE,	"Time taken to push a payload cart.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_payload_defense_respawn_time_coefficient{	"mp_payload_defense_respawn_time_coefficient",		2.0f,		ConVar::SERVER_VARIABLE,	"The defending team's respawn time is multiplied by this value.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_payload_defense_respawn_time_threshold{		"mp_payload_defense_respawn_time_threshold",		0.5f,		ConVar::SERVER_VARIABLE,	"Fraction of how far the attackers need to push the cart before the defenders' respawn time multiplier is activated.", 0.0f, 1.0f};
ConVarFloatMinMax	mp_spy_kill_disguise_cooldown{					"mp_spy_kill_disguise_cooldown",					2.0f,		ConVar::SERVER_VARIABLE,	"Time before spies can re-disguise after killing someone.", 0.0f, -1.0f};
ConVarIntMinMax		mp_sniper_rifle_range{							"mp_sniper_rifle_range",							45,			ConVar::SERVER_VARIABLE,	"Length of sniper rifle trails.", 0, -1};
ConVarFloatMinMax	mp_blast_jump_move_interval{					"mp_blast_jump_move_interval",						0.05f,		ConVar::SERVER_VARIABLE,	"Time taken to move when blast jumping.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_blast_jump_duration{							"mp_blast_jump_duration",							1.0f,		ConVar::SERVER_VARIABLE,	"Time taken before landing after blast jumping.", 0.0f, -1.0f};
ConVarFloatMinMax	mp_blast_jump_chain_duration{					"mp_blast_jump_chain_duration",						0.9f,		ConVar::SERVER_VARIABLE,	"Time taken before landing after chaining blast jumps.", 0.0f, -1.0f};
ConVarFloat			mp_blast_jump_chain_move_interval_coefficient{	"mp_blast_jump_chain_move_interval_coefficient",	0.6f,		ConVar::SERVER_VARIABLE,	"What to multiply the move interval by when chaining blast jumps."};
ConVarFloat			mp_self_damage_coefficient{						"mp_self_damage_coefficient",						0.3f,		ConVar::SERVER_VARIABLE,	"How much of the damage you take when hurting yourself, such as when blast jumping."};
ConVarInt			mp_score_objective{								"mp_score_objective",								4,			ConVar::SERVER_VARIABLE,	"Number of points awarded for completing an objective."};
ConVarInt			mp_score_win{									"mp_score_win",										10,			ConVar::SERVER_VARIABLE,	"Number of points awarded for winning a round."};
ConVarInt			mp_score_lose{									"mp_score_lose",									5,			ConVar::SERVER_VARIABLE,	"Number of points awarded for losing a round."};
ConVarInt			mp_score_kill{									"mp_score_kill",									1,			ConVar::SERVER_VARIABLE,	"Number of points awarded for killing an enemy player."};
ConVarInt			mp_score_kill_sentry{							"mp_score_kill_sentry",								1,			ConVar::SERVER_VARIABLE,	"Number of points awarded for killing an enemy sentry gun."};
ConVarInt			mp_score_heal{									"mp_score_heal",									1,			ConVar::SERVER_VARIABLE,	"Number of points awarded for healing a teammate."};
ConVarBool			mp_shotgun_use_legacy_spread{					"mp_shotgun_use_legacy_spread",						false,		ConVar::SERVER_VARIABLE,	"Whether or not to use the old (pre-2.0.0) style shotgun spread."};
ConVarBool			mp_enable_round_time{							"mp_enable_round_time",								true,		ConVar::SERVER_VARIABLE,	"Whether or not to enable the round countdown."};
ConVarBool			mp_switch_teams_between_rounds{					"mp_switch_teams_between_rounds",					true,		ConVar::SERVER_VARIABLE,	"Whether or not to automatically switch the teams of all players between rounds."};
ConVarIntMinMax		sv_max_shots_per_frame{							"sv_max_shots_per_frame",							20,			ConVar::SERVER_SETTING,		"Maximum number of shots to fire from a weapon in one frame.", 0, 1000};
ConVarIntMinMax		sv_max_move_steps_per_frame{					"sv_max_move_steps_per_frame",						20,			ConVar::SERVER_SETTING,		"Maximum number of steps to move an entity in one frame.", 0, 1000};
ConVarIntMinMax		mp_winlimit{									"mp_winlimit",										2,			ConVar::SERVER_VARIABLE,	"Maximum number of times one team has to win before automatically switching map. 0 = unlimited.", 0, -1};
ConVarIntMinMax		mp_roundlimit{									"mp_roundlimit",									3,			ConVar::SERVER_VARIABLE,	"Maximum number of rounds to play before automatically switching map. 0 = unlimited.", 0, -1};
ConVarFloatMinMax	mp_timelimit{									"mp_timelimit",										1200.0f,	ConVar::SERVER_VARIABLE,	"How many seconds to wait before automatically switching map after the round ends. 0 = unlimited.", 0.0f, -1.0f};
// clang-format on

CON_COMMAND(mp_get_team_id_by_name, "<name>", ConCommand::NO_FLAGS, "Get the id of the team with a certain name.", {}, cmd::suggestTeam<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(Team::findByName(argv[1]).getId());
}

CON_COMMAND(mp_get_class_id_by_name, "<name>", ConCommand::NO_FLAGS, "Get the id of the class with a certain name.", {}, cmd::suggestPlayerClass<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(PlayerClass::findByName(argv[1]).getId());
}

CON_COMMAND(mp_get_projectile_type_id_by_name, "<name>", ConCommand::NO_FLAGS, "Get the id of the projectile type with a certain name.", {},
            cmd::suggestProjectileType<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(ProjectileType::findByName(argv[1]).getId());
}

CON_COMMAND(mp_get_weapon_id_by_name, "<name>", ConCommand::NO_FLAGS, "Get the id of the weapon with a certain name.", {}, cmd::suggestWeapon<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(Weapon::findByName(argv[1]).getId());
}

CON_COMMAND(mp_get_team_name, "<team_id>", ConCommand::NO_FLAGS, "Get the name of the team with a certain id.", {}, cmd::suggestTeamId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto teamId = cmd::parseNumber<Team::ValueType>(parseError, argv[1], "team id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(std::string{Team::findById(teamId).getName()});
}

CON_COMMAND(mp_get_class_name, "<class_id>", ConCommand::NO_FLAGS, "Get the name of the class with a certain id.", {}, cmd::suggestPlayerClassId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto classId = cmd::parseNumber<PlayerClass::ValueType>(parseError, argv[1], "class id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(std::string{PlayerClass::findById(classId).getName()});
}

CON_COMMAND(mp_get_projectile_type_name, "<projectile_type_id>", ConCommand::NO_FLAGS,
            "Get the name of the projectile type with a certain id.", {}, cmd::suggestProjectileTypeId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto typeId = cmd::parseNumber<ProjectileType::ValueType>(parseError, argv[1], "projectile type id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(std::string{ProjectileType::findById(typeId).getName()});
}

CON_COMMAND(mp_get_weapon_name, "<weapon_id>", ConCommand::NO_FLAGS, "Get the name of the weapon type with a certain id.", {},
            cmd::suggestWeaponId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto weaponId = cmd::parseNumber<Weapon::ValueType>(parseError, argv[1], "weapon id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(std::string{Weapon::findById(weaponId).getName()});
}

CON_COMMAND(mp_get_player_id_by_ip, "<ip>", ConCommand::SERVER, "Get the id of the player with a certain ip address.", {},
            cmd::suggestConnectedClientIp<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto endpoint = cmd::parseIpEndpoint(parseError, argv[1], "ip");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto id = server->getPlayerIdByIp(endpoint)) {
		return cmd::done(*id);
	}
	return cmd::error("{}: Player \"{}\" not found.", self.getName(), argv[1]);
}

CON_COMMAND(mp_get_player_ip, "<player_id>", ConCommand::SERVER, "Get the ip address of the player with a certain id.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto endpoint = server->getPlayerIp(id)) {
		return cmd::done(std::string{*endpoint});
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_get_player_inventory_id, "<player_id>", ConCommand::SERVER, "Find the inventory id of the player with a certain player id.",
            {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto inventoryId = server->getPlayerInventoryId(id)) {
		return cmd::done(*inventoryId);
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_award_player_points, "<player_id> <points>", ConCommand::SERVER,
            "Give points to a player with a certain id and potentially level them up.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	const auto points = cmd::parseNumber<Score>(parseError, argv[2], "number of points");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	const auto player = server->world().findPlayer(id);
	if (!player) {
		return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
	}
	const auto name = std::string{player.getName()};
	if (!server->awardPlayerPoints(id, name, points)) {
		return cmd::error("{}: Failed to award points to player \"{}\".", self.getName(), name);
	}

	return cmd::done();
}

CON_COMMAND(mp_is_player_bot, "<player_id>", ConCommand::SERVER, "Check if a certain player is a bot.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->isPlayerBot(id));
}

CON_COMMAND(mp_play_world_sound, "<sound> <x> <y>", ConCommand::SERVER, "Play a sound at (x, y) in the world.", {}, cmd::suggestValidSoundFilename<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto soundId = cmd::parseSoundId(parseError, argv[1], "sound");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->playWorldSound(soundId, Vec2{x, y});
	return cmd::done();
}

CON_COMMAND(mp_play_world_sound_from_player, "<sound> <x> <y> <player_id>", ConCommand::SERVER,
            "Play a sound at (x, y) in the world, originating from a certain player.", {}, cmd::suggestValidSoundFilename<1>) {
	if (argv.size() != 5) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto soundId = cmd::parseSoundId(parseError, argv[1], "sound");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[4], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->playWorldSound(soundId, Vec2{x, y}, id);
	return cmd::done();
}

CON_COMMAND(mp_play_team_sound, "<sound> <team>", ConCommand::SERVER, "Play a sound to everyone in a certain team.", {},
            cmd::suggestValidSoundFilename<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto soundId = cmd::parseSoundId(parseError, argv[1], "sound");
	const auto team = cmd::parseTeam(parseError, argv[2], "team");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->playTeamSound(soundId, team);
	return cmd::done();
}

CON_COMMAND(mp_play_team_sound_separate, "<sound> <other_team_sound> <team>", ConCommand::SERVER,
            "Play a sound to everyone in a certain team, and a different sound to everyone else.", {}, cmd::suggestValidSoundFilename<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto correctTeamSoundId = cmd::parseSoundId(parseError, argv[1], "sound");
	const auto otherTeamSoundId = cmd::parseSoundId(parseError, argv[2], "sound");
	const auto team = cmd::parseTeam(parseError, argv[3], "team");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->playTeamSound(correctTeamSoundId, otherTeamSoundId, team);
	return cmd::done();
}

CON_COMMAND(mp_play_game_sound, "<sound>", ConCommand::SERVER, "Play a sound to every player.", {}, cmd::suggestValidSoundFilename<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto soundId = cmd::parseSoundId(parseError, argv[1], "sound");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->playGameSound(soundId);
	return cmd::done();
}

CON_COMMAND(mp_write_event, "[relevant_player_ids...] <text>", ConCommand::SERVER,
            "Write a server event message to all players, and optionally send them as personal event messages to a set of player ids.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	if (argv.size() > 2) {
		auto parseError = cmd::ParseError{};

		const auto parsePlayerId = [&](const auto& arg) {
			return cmd::parseNumber<PlayerId>(parseError, arg, "player id");
		};
		const auto ids = util::subview(argv, 1, argv.size() - 2) | util::transform(parsePlayerId) | util::collect<std::vector<PlayerId>>();
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}

		server->writeServerEventMessage(argv.back(), ids);
	} else {
		server->writeServerEventMessage(argv[1]);
	}
	return cmd::done();
}

CON_COMMAND(mp_write_event_team, "<team> <text>", ConCommand::SERVER, "Write a server event message to all players in a certain team.", {},
            cmd::suggestValidTeamId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto team = cmd::parseTeam(parseError, argv[1], "team");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->writeServerEventMessage(argv[2], team);
	return cmd::done();
}

CON_COMMAND(mp_write_event_player, "<player_id> <text>", ConCommand::SERVER, "Write a personal server event mesage to a certain player.",
            {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (!server->writeServerEventMessagePersonal(argv[2], id)) {
		return cmd::error("{}: Client with player id \"{}\" not found.", self.getName(), id);
	}
	return cmd::done();
}

CON_COMMAND(mp_write_chat, "<text>", ConCommand::SERVER, "Write a server chat message to all players.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	server->writeServerChatMessage(argv[1]);
	return cmd::done();
}

CON_COMMAND(mp_write_chat_team, "<team> <text>", ConCommand::SERVER, "Write a server chat message to all players in a certain team.", {},
            cmd::suggestValidTeamId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto team = cmd::parseTeam(parseError, argv[1], "team");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->writeServerChatMessage(argv[2], team);
	return cmd::done();
}

CON_COMMAND(mp_write_chat_player, "<player_id> <text>", ConCommand::SERVER, "Write a server chat message to a certain player.", {},
            cmd::suggestPlayerId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (!server->writeServerChatMessagePersonal(argv[2], id)) {
		return cmd::error("{}: Client with player id \"{}\" not found.", self.getName(), id);
	}
	return cmd::done();
}

CON_COMMAND(mp_end_round, "[winning_team]", ConCommand::SERVER, "End the current round.", {}, cmd::suggestValidTeamId<1>) {
	if (argv.size() != 1 && argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	if (argv.size() == 2) {
		auto parseError = cmd::ParseError{};

		const auto team = cmd::parseTeam(parseError, argv[1], "team");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}

		server->world().win(team);
	} else {
		server->world().stalemate();
	}
	return cmd::done();
}

CON_COMMAND(mp_reset_round, "", ConCommand::SERVER, "Reset the current round.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	server->world().resetRound();
	return cmd::done();
}

CON_COMMAND(mp_reset_map, "", ConCommand::SERVER, "Reset the current map.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	server->world().reset();
	server->world().startMap();
	return cmd::done();
}

CON_COMMAND(mp_get_team_wins, "<team>", ConCommand::SERVER, "Get the number of wins since the latest map switch for the given team.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto team = cmd::parseTeam(parseError, argv[1], "team");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->world().getTeamWins(team));
}

CON_COMMAND(mp_time_played, "", ConCommand::SERVER, "Get the time passed since the latest map switch, in seconds.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	return cmd::done(server->world().getMapTime());
}

CON_COMMAND(mp_rounds_played, "", ConCommand::SERVER, "Get the number of rounds played since the latest map switch.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	return cmd::done(server->world().getRoundsPlayed());
}
