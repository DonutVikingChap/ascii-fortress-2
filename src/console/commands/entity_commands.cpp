#include "entity_commands.hpp"

#include "../../game/data/health.hpp"          // Health
#include "../../game/data/projectile_type.hpp" // ProjectileType
#include "../../game/data/sound_id.hpp"        // SoundId
#include "../../game/data/team.hpp"            // Team
#include "../../game/data/vector.hpp"          // Vec2
#include "../../game/data/weapon.hpp"          // Weapon
#include "../../game/server/game_server.hpp"   // GameServer
#include "../../game/server/world.hpp"         // World
#include "../../utilities/algorithm.hpp"       // util::transform
#include "../../utilities/string.hpp"          // util::join
#include "../command_options.hpp"              // cmd::...
#include "../command_utilities.hpp"            // cmd::...
#include "../convar.hpp"                       // ConVar...

extern ConVarIntMinMax mp_sentry_health;
extern ConVarFloatMinMax mp_explosion_disappear_time;
extern ConVarFloatMinMax mp_medkit_respawn_time;
extern ConVarFloatMinMax mp_ammopack_respawn_time;

CON_COMMAND(mp_create_projectile, "[options...] <x> <y> <dx> <dy> <team> <projectile_type_id> <damage>", ConCommand::SERVER,
			"Add a projectile to the world. Returns the new id.",
			cmd::opts(cmd::opt('m', "move-interval", "Projectile move interval (lower = faster).", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('t', "disappear-time", "How long until the projectile disappears.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('w', "weapon", "Projectile weapon id.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('o', "owner", "Projectile owner player id.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('s', "hurt-sound", "Hurt sound filename.", cmd::OptionType::ARGUMENT_REQUIRED)),
			nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 7) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, args[0], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, args[1], "y coordinate");
	const auto dx = cmd::parseNumber<Vec2::Length>(parseError, args[2], "x delta");
	const auto dy = cmd::parseNumber<Vec2::Length>(parseError, args[3], "y delta");
	const auto team = cmd::parseTeam(parseError, args[4], "team");
	const auto type = cmd::parseProjectileType(parseError, args[5], "type");
	const auto damage = cmd::parseNumber<Health>(parseError, args[6], "damage");

	auto moveInterval = type.getMoveInterval();
	if (const auto moveIntervalStr = options['m']) {
		moveInterval = cmd::parseNumber<float, cmd::NumberConstraint::NON_NEGATIVE>(parseError, *moveIntervalStr, "move interval");
	}

	auto disappearTime = type.getDisappearTime();
	if (const auto disappearTimeStr = options['t']) {
		disappearTime = cmd::parseNumber<float, cmd::NumberConstraint::NON_NEGATIVE>(parseError, *disappearTimeStr, "disappear time");
	}

	auto weapon = Weapon::none();
	if (const auto weaponStr = options['w']) {
		weapon = cmd::parseWeapon(parseError, *weaponStr, "weapon");
	}

	auto owner = PLAYER_ID_UNCONNECTED;
	if (const auto ownerStr = options['o']) {
		owner = cmd::parseNumber<World::PlayerId>(parseError, *ownerStr, "owner id");
	}

	auto hurtSound = (weapon == Weapon::none()) ? SoundId::player_hurt() : weapon.getHurtSound();
	if (const auto hurtSoundStr = options['s']) {
		hurtSound = cmd::parseSoundId(parseError, *hurtSoundStr, "sound");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (team == Team::spectators()) {
		return cmd::error("{}: Projectile team cannot be \"spectators\".", self.getName());
	}

	assert(server);
	return cmd::done(server->world().createProjectile(Vec2{x, y}, Direction{dx, dy}, type, team, owner, weapon, damage, hurtSound, disappearTime, moveInterval));
}

CON_COMMAND(mp_create_explosion, "[options...] <x> <y> <team> <damage>", ConCommand::SERVER,
			"Add a projectile to the world. Returns the new id.",
			cmd::opts(cmd::opt('t', "disappear-time", "How long until the explosion disappears.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('w', "weapon", "Explosion weapon id.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('o', "owner", "Explosion owner player id.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('s', "hurt-sound", "Hurt sound filename.", cmd::OptionType::ARGUMENT_REQUIRED)),
			nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 4) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, args[0], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, args[1], "y coordinate");
	const auto team = cmd::parseTeam(parseError, args[2], "team");
	const auto damage = cmd::parseNumber<Health>(parseError, args[3], "damage");

	auto disappearTime = static_cast<float>(mp_explosion_disappear_time);
	if (const auto disappearTimeStr = options['t']) {
		disappearTime = cmd::parseNumber<float, cmd::NumberConstraint::NON_NEGATIVE>(parseError, *disappearTimeStr, "disappear time");
	}

	auto weapon = Weapon::none();
	if (const auto weaponStr = options['w']) {
		weapon = cmd::parseWeapon(parseError, *weaponStr, "weapon");
	}

	auto owner = PLAYER_ID_UNCONNECTED;
	if (const auto ownerStr = options['o']) {
		owner = cmd::parseNumber<World::PlayerId>(parseError, *ownerStr, "owner id");
	}

	auto hurtSound = (weapon == Weapon::none()) ? SoundId::player_hurt() : weapon.getHurtSound();
	if (const auto hurtSoundStr = options['s']) {
		hurtSound = cmd::parseSoundId(parseError, *hurtSoundStr, "sound");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (team == Team::spectators()) {
		return cmd::error("{}: Projectile team cannot be \"spectators\".", self.getName());
	}

	assert(server);
	return cmd::done(server->world().createExplosion(Vec2{x, y}, team, owner, weapon, damage, hurtSound, disappearTime));
}

CON_COMMAND(mp_create_sentry, "[options...] <x> <y> <team>", ConCommand::SERVER, "Add a sentry gun to the world. Returns the new id.",
			cmd::opts(cmd::opt('h', "health", "Sentry gun health.", cmd::OptionType::ARGUMENT_REQUIRED),
					  cmd::opt('o', "owner", "Sentry gun owner player id.", cmd::OptionType::ARGUMENT_REQUIRED)),
			nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, args[0], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, args[1], "y coordinate");
	const auto team = cmd::parseTeam(parseError, args[2], "team");

	auto health = static_cast<Health>(mp_sentry_health);
	if (const auto healthStr = options['h']) {
		health = cmd::parseNumber<Health>(parseError, *healthStr, "health");
	}

	auto owner = PLAYER_ID_UNCONNECTED;
	if (const auto ownerStr = options['o']) {
		owner = cmd::parseNumber<World::PlayerId>(parseError, *ownerStr, "owner id");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (team == Team::spectators()) {
		return cmd::error("{}: Sentry team cannot be \"spectators\".", self.getName());
	}

	assert(server);
	return cmd::done(server->world().createSentryGun(Vec2{x, y}, team, health, owner));
}

CON_COMMAND(mp_create_medkit, "<x> <y>", ConCommand::SERVER, "Add a medkit to the world. Returns the new id.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[1], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "y coordinate");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->world().createMedkit(Vec2{x, y}));
}

CON_COMMAND(mp_create_ammopack, "<x> <y>", ConCommand::SERVER, "Add an ammopack to the world. Returns the new id.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[1], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "y coordinate");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->world().createAmmopack(Vec2{x, y}));
}

CON_COMMAND(mp_create_ent, "<x> <y>", ConCommand::SERVER, "Add a generic entity to the world. Returns the new id.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[1], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "y coordinate");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->world().createGenericEntity(Vec2{x, y}));
}

CON_COMMAND(mp_create_flag, "<x> <y> <team> <name>", ConCommand::SERVER, "Add a flag to the world. Returns the new id.", {}, cmd::suggestTeam<3>) {
	if (argv.size() != 5) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[1], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "y coordinate");
	const auto team = cmd::parseTeam(parseError, argv[3], "team");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->world().createFlag(Vec2{x, y}, team, argv[4]));
}

#define DEFINE_SPAWN_COMMAND(Name, name, prefix, Str, str) \
	CON_COMMAND(mp_spawn_##name, "<" #name "_id>", ConCommand::SERVER, "Cause " prefix " " str " to spawn.", {}, cmd::suggest##Name##Id<1>) { \
		if (argv.size() != 2) \
			return cmd::error(self.getUsage()); \
\
		auto parseError = cmd::ParseError{}; \
\
		const auto id = cmd::parseNumber<World::Name##Id>(parseError, argv[1], str " id"); \
		if (parseError) { \
			return cmd::error("{}: {}", self.getName(), *parseError); \
		} \
\
		assert(server); \
		if (!server->world().spawn##Name(id)) { \
			return cmd::error("{}: Couldn't find " str " with id \"{}\"!", self.getName(), id); \
		} \
		return cmd::done(); \
	}

DEFINE_SPAWN_COMMAND(Player, player, "a", "Player", "player")
DEFINE_SPAWN_COMMAND(Medkit, medkit, "a", "Medkit", "medkit")
DEFINE_SPAWN_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack")

CON_COMMAND(mp_hurt_player, "<player_id> <damage>", ConCommand::SERVER, "Hurt the player with a certain id.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::PlayerId>(parseError, argv[1], "player id");
	const auto damage = cmd::parseNumber<Health>(parseError, argv[2], "damage");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().applyDamageToPlayer(id, damage, SoundId::player_hurt(), true, id)) {
		return cmd::done();
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_hurt_sentry, "<sentry_id> <damage>", ConCommand::SERVER, "Hurt the sentry gun with a certain id.", {}, cmd::suggestSentryGunId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::SentryGunId>(parseError, argv[1], "sentry id");
	const auto damage = cmd::parseNumber<Health>(parseError, argv[2], "damage");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().applyDamageToSentryGun(id, damage, SoundId::sentry_hurt(), true, PLAYER_ID_UNCONNECTED)) {
		return cmd::done();
	}
	return cmd::error("{}: Sentry with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_kill_player, "<player_id> [killer_id]", ConCommand::SERVER, "Kill the player with a certain id.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 2 && argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::PlayerId>(parseError, argv[1], "player id");

	auto killer_id = id;
	if (argv.size() == 3) {
		killer_id = cmd::parseNumber<World::PlayerId>(parseError, argv[2], "killer id");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().killPlayer(id, true, killer_id)) {
		return cmd::done();
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_kill_sentry, "<sentry_id> [killer_id]", ConCommand::SERVER, "Kill the sentry gun with a certain id.", {}, cmd::suggestSentryGunId<1>) {
	if (argv.size() != 2 && argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::SentryGunId>(parseError, argv[1], "sentry id");

	auto killer_id = PLAYER_ID_UNCONNECTED;
	if (argv.size() == 3) {
		killer_id = cmd::parseNumber<World::PlayerId>(parseError, argv[2], "killer id");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().killSentryGun(id, killer_id)) {
		return cmd::done();
	}
	return cmd::error("{}: Sentry with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_kill_medkit, "[options...] <medkit_id>", ConCommand::SERVER, "Kill a medkit.",
			cmd::opts(cmd::opt('r', "respawn-time", "Medkit respawn time.", cmd::OptionType::ARGUMENT_REQUIRED)), cmd::suggestMedkitId<1>) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 1) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::MedkitId>(parseError, args[0], "medkit id");

	auto respawnTime = static_cast<float>(mp_medkit_respawn_time);
	if (const auto& respawnTimeOpt = options['r']) {
		respawnTime = cmd::parseNumber<float, cmd::NumberConstraint::NON_NEGATIVE>(parseError, *respawnTimeOpt, "respawn time");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().killMedkit(id, respawnTime)) {
		return cmd::done();
	}
	return cmd::error("{}: Medkit with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_kill_ammopack, "[options...] <ammopack_id>", ConCommand::SERVER, "Kill an ammopack.",
			cmd::opts(cmd::opt('r', "respawn-time", "Ammopack respawn time.", cmd::OptionType::ARGUMENT_REQUIRED)), cmd::suggestAmmopackId<1>) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 1) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::AmmopackId>(parseError, args[0], "ammopack id");

	auto respawnTime = static_cast<float>(mp_ammopack_respawn_time);
	if (const auto& respawnTimeOpt = options['r']) {
		respawnTime = cmd::parseNumber<float, cmd::NumberConstraint::NON_NEGATIVE>(parseError, *respawnTimeOpt, "respawn time");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().killAmmopack(id, respawnTime)) {
		return cmd::done();
	}
	return cmd::error("{}: Ammopack with id \"{}\" not found.", self.getName(), id);
}

#define DEFINE_DELETE_COMMAND(Name, name, prefix, Str, str) \
	CON_COMMAND(mp_delete_##name, "<" #name "_id>", ConCommand::SERVER, "Delete " prefix " " str ".", {}, cmd::suggest##Name##Id<1>) { \
		if (argv.size() != 2) \
			return cmd::error(self.getUsage()); \
\
		auto parseError = cmd::ParseError{}; \
\
		const auto id = cmd::parseNumber<World::Name##Id>(parseError, argv[1], str " id"); \
		if (parseError) \
			return cmd::error("{}: {}", self.getName(), *parseError); \
\
		assert(server); \
		if (server->world().delete##Name(id)) \
			return cmd::done(); \
		return cmd::error("{}: " Str " with id \"{}\" not found.", self.getName(), id); \
	}

DEFINE_DELETE_COMMAND(Projectile, projectile, "a", "Projectile", "projectile")
DEFINE_DELETE_COMMAND(Explosion, explosion, "an", "Explosion", "explosion")
DEFINE_DELETE_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun")
DEFINE_DELETE_COMMAND(Medkit, medkit, "a", "Medkit", "medkit")
DEFINE_DELETE_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack")
DEFINE_DELETE_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity")
DEFINE_DELETE_COMMAND(Flag, flag, "a", "Flag", "flag")

#define DEFINE_HAS_COMMAND(Name, name, prefix, Str, str) \
	CON_COMMAND(mp_has_##name, "<" #name "_id>", ConCommand::SERVER, "Check if a " str " with a certain id exists.", {}, cmd::suggest##Name##Id<1>) { \
		if (argv.size() != 2) \
			return cmd::error(self.getUsage()); \
\
		auto parseError = cmd::ParseError{}; \
\
		const auto id = cmd::parseNumber<World::Name##Id>(parseError, argv[1], str " id"); \
		if (parseError) \
			return cmd::error("{}: {}", self.getName(), *parseError); \
\
		assert(server); \
		return cmd::done(server->world().has##Name##Id(id)); \
	}

DEFINE_HAS_COMMAND(Player, player, "a", "Player", "player")
DEFINE_HAS_COMMAND(Projectile, projectile, "a", "Projectile", "projectile")
DEFINE_HAS_COMMAND(Explosion, explosion, "an", "Explosion", "explosion")
DEFINE_HAS_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun")
DEFINE_HAS_COMMAND(Medkit, medkit, "a", "Medkit", "medkit")
DEFINE_HAS_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack")
DEFINE_HAS_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity")
DEFINE_HAS_COMMAND(Flag, flag, "a", "Flag", "flag")
DEFINE_HAS_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart")

#define DEFINE_COUNT_COMMAND(Name, name, prefix, Str, str) \
	CON_COMMAND(mp_##name##_count, "", ConCommand::SERVER, "Get the active " str " count.", {}, nullptr) { \
		if (argv.size() != 1) \
			return cmd::error(self.getUsage()); \
\
		assert(server); \
		return cmd::done(server->world().get##Name##Count()); \
	}

DEFINE_COUNT_COMMAND(Player, player, "a", "Player", "player")
DEFINE_COUNT_COMMAND(Projectile, projectile, "a", "Projectile", "projectile")
DEFINE_COUNT_COMMAND(Explosion, explosion, "an", "Explosion", "explosion")
DEFINE_COUNT_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun")
DEFINE_COUNT_COMMAND(Medkit, medkit, "a", "Medkit", "medkit")
DEFINE_COUNT_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack")
DEFINE_COUNT_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity")
DEFINE_COUNT_COMMAND(Flag, flag, "a", "Flag", "flag")
DEFINE_COUNT_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart")

#define DEFINE_LIST_COMMAND(Name, name, prefix, Str, str) \
	CON_COMMAND(mp_##name##_list, "", ConCommand::SERVER, "List every active " str " id.", {}, nullptr) { \
		if (argv.size() != 1) \
			return cmd::error(self.getUsage()); \
\
		assert(server); \
		return cmd::done(server->world().getAll##Name##Ids() | util::transform(cmd::format##Name##Id) | util::join('\n')); \
	}

DEFINE_LIST_COMMAND(Player, player, "a", "Player", "player")
DEFINE_LIST_COMMAND(Projectile, projectile, "a", "Projectile", "projectile")
DEFINE_LIST_COMMAND(Explosion, explosion, "an", "Explosion", "explosion")
DEFINE_LIST_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun")
DEFINE_LIST_COMMAND(Medkit, medkit, "a", "Medkit", "medkit")
DEFINE_LIST_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack")
DEFINE_LIST_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity")
DEFINE_LIST_COMMAND(Flag, flag, "a", "Flag", "flag")
DEFINE_LIST_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart")

#define DEFINE_TELEPORT_COMMAND(Name, name, prefix, Str, str) \
	CON_COMMAND(mp_teleport_##name, \
				"<" #name "_id> <x> <y>", \
				ConCommand::SERVER, \
				"Instantly move " prefix " " str " to a certain destination.", \
				{}, \
				cmd::suggest##Name##Id<1>) { \
		if (argv.size() != 4) \
			return cmd::error(self.getUsage()); \
\
		auto parseError = cmd::ParseError{}; \
\
		const auto id = cmd::parseNumber<World::Name##Id>(parseError, argv[1], str " id"); \
		const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate"); \
		const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate"); \
		if (parseError) \
			return cmd::error("{}: {}", self.getName(), *parseError); \
\
		assert(server); \
		if (server->world().teleport##Name(id, Vec2{x, y})) \
			return cmd::done(); \
		return cmd::error("{}: Couldn't teleport " str " with id \"{}\" to ({}, {})!", self.getName(), id, x, y); \
	}

DEFINE_TELEPORT_COMMAND(Player, player, "a", "Player", "player")
DEFINE_TELEPORT_COMMAND(Projectile, projectile, "a", "Projectile", "projectile")
DEFINE_TELEPORT_COMMAND(Explosion, explosion, "an", "Explosion", "explosion")
DEFINE_TELEPORT_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun")
DEFINE_TELEPORT_COMMAND(Medkit, medkit, "a", "Medkit", "medkit")
DEFINE_TELEPORT_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack")
DEFINE_TELEPORT_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity")
DEFINE_TELEPORT_COMMAND(Flag, flag, "a", "Flag", "flag")

CON_COMMAND(mp_get_player_id_by_name, "<name>", ConCommand::SERVER, "Get the id of the player with a certain name.", {}, cmd::suggestPlayerName<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	if (const auto id = server->world().findPlayerIdByName(argv[1]); id != PLAYER_ID_UNCONNECTED) {
		return cmd::done(id);
	}
	return cmd::error("{}: Player \"{}\" not found.", self.getName(), argv[1]);
}

CON_COMMAND(mp_is_player_carrying_flag, "<player_id>", ConCommand::SERVER, "Check if a player is carrying a flag.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::PlayerId>(parseError, argv[1], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->world().isPlayerCarryingFlag(id));
}

CON_COMMAND(mp_player_team_select, "<player_id> <team> <class>", ConCommand::SERVER, "Set the team and class of a player.", {},
			cmd::suggestPlayerId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	const auto team = cmd::parseTeam(parseError, argv[2], "team");
	const auto playerClass = cmd::parsePlayerClass(parseError, argv[3], "class");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().playerTeamSelect(id, team, playerClass)) {
		return cmd::done();
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_resupply_player, "<player_id>", ConCommand::SERVER, "Refill a player's health and ammo.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().resupplyPlayer(id)) {
		return cmd::done();
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_set_round_time, "<time>", ConCommand::SERVER, "Set remaining round time.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto time = cmd::parseNumber<float>(parseError, argv[1], "time");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->world().setRoundTimeLeft(time);
	return cmd::done();
}

CON_COMMAND(mp_add_round_time, "<time>", ConCommand::SERVER, "Add round time.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto time = cmd::parseNumber<float>(parseError, argv[1], "time");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	server->world().addRoundTimeLeft(time);
	return cmd::done();
}

CON_COMMAND(mp_get_round_time, "", ConCommand::SERVER, "Get remaining round time.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	return cmd::done(server->world().getRoundTimeLeft());
}

#define DEFINE_GET_COMMAND(Name, name, prefix, Str, str, field, accessMember) \
	CON_COMMAND(mp_get_##name##_##field, "<" #name "_id>", ConCommand::SERVER, "Get the " #field " of " prefix " " str ".", {}, cmd::suggest##Name##Id<1>) { \
		if (argv.size() != 2) { \
			return cmd::error(self.getUsage()); \
		} \
\
		auto parseError = cmd::ParseError{}; \
\
		const auto id = cmd::parseNumber<World::Name##Id>(parseError, argv[1], str " id"); \
		if (parseError) { \
			return cmd::error("{}: {}", self.getName(), *parseError); \
		} \
\
		assert(server); \
		if (const auto& entity = server->world().find##Name(id)) { \
			return cmd::done(accessMember); \
		} \
		return cmd::error("{}: " Str " with id \"{}\" not found.", self.getName(), id); \
	}

#define DEFINE_SET_COMMAND(Name, name, prefix, Str, str, field, setMember, Type) \
	CON_COMMAND(mp_set_##name##_##field, "<" #name "_id> <value>", ConCommand::SERVER, "Set the " #field " of " prefix " " str ".", {}, cmd::suggest##Name##Id<1>) { \
		if (argv.size() != 3) { \
			return cmd::error(self.getUsage()); \
		} \
\
		auto parseError = cmd::ParseError{}; \
\
		const auto id = cmd::parseNumber<World::Name##Id>(parseError, argv[1], str " id"); \
		const auto value = cmd::parse##Type(parseError, argv[2], #field); \
		if (parseError) { \
			return cmd::error("{}: {}", self.getName(), *parseError); \
		} \
\
		assert(server); \
		if (const auto& entity = server->world().find##Name(id)) { \
			setMember; \
			return cmd::done(); \
		} \
		return cmd::error("{}: " Str " with id \"{}\" not found.", self.getName(), id); \
	}

DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", name, entity.getName())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", x, entity.getPosition().x)
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", y, entity.getPosition().y)
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", move_x, entity.getMoveDirection().getX())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", move_y, entity.getMoveDirection().getY())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", aim_x, entity.getAimDirection().getX())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", aim_y, entity.getAimDirection().getY())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", attack1, entity.getAttack1())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", attack2, entity.getAttack2())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", team, entity.getTeam().getId())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", class, entity.getPlayerClass().getId())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", alive, entity.isAlive())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", ping, entity.getLatestMeasuredPingDuration())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", disguised, entity.isDisguised())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", health, entity.getHealth())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", score, entity.getScore())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", noclip, entity.isNoclip())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", primary_ammo, entity.getPrimaryAmmo())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", secondary_ammo, entity.getSecondaryAmmo())
DEFINE_GET_COMMAND(Player, player, "a", "Player", "player", hat, entity.getHat().getId())

DEFINE_SET_COMMAND(Player, player, "a", "Player", "player", ping, entity.setLatestMeasuredPingDuration(value), Number<Latency>)
DEFINE_SET_COMMAND(Player, player, "a", "Player", "player", disguised, entity.setDisguised(value), Bool)
DEFINE_SET_COMMAND(Player, player, "a", "Player", "player", score, entity.setScore(value), Number<Score>)
DEFINE_SET_COMMAND(Player, player, "a", "Player", "player", attack1, entity.setAttack1(value), Bool)
DEFINE_SET_COMMAND(Player, player, "a", "Player", "player", attack2, entity.setAttack2(value), Bool)

CON_COMMAND(mp_set_player_move, "<player_id> <dx> <dy>", ConCommand::SERVER, "Set the movement vector of a player.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::PlayerId>(parseError, argv[1], "player id");
	const auto dx = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x value");
	const auto dy = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& player = server->world().findPlayer(id)) {
		player.setMoveDirection(Direction{dx, dy});
		return cmd::done();
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_set_player_aim, "<player_id> <dx> <dy>", ConCommand::SERVER, "Set the aim vector of a player.", {}, cmd::suggestPlayerId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::PlayerId>(parseError, argv[1], "player id");
	const auto dx = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x value");
	const auto dy = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& player = server->world().findPlayer(id)) {
		player.setAimDirection(Direction{dx, dy});
		return cmd::done();
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_set_player_noclip, "<player_id> <value>", ConCommand::SERVER, "Set the noclip state of the player with a certain id.", {},
			cmd::suggestPlayerId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<PlayerId>(parseError, argv[1], "player id");
	const auto value = cmd::parseBool(parseError, argv[2], "value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (server->world().setPlayerNoclip(id, value)) {
		return cmd::done();
	}
	return cmd::error("{}: Player with id \"{}\" not found.", self.getName(), id);
}

DEFINE_GET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", x, entity.getPosition().x)
DEFINE_GET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", y, entity.getPosition().y)
DEFINE_GET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", aim_x, entity.getAimDirection().getX())
DEFINE_GET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", aim_y, entity.getAimDirection().getY())
DEFINE_GET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", team, entity.getTeam().getId())
DEFINE_GET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", health, entity.getHealth())
DEFINE_GET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", owner, entity.getOwner())

DEFINE_SET_COMMAND(SentryGun, sentry, "a", "Sentry gun", "sentry gun", owner, entity.setOwner(value), Number<World::PlayerId>)

CON_COMMAND(mp_set_sentry_aim, "<sentry_id> <dx> <dy>", ConCommand::SERVER, "Set the aim vector of a sentry gun.", {}, cmd::suggestSentryGunId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::SentryGunId>(parseError, argv[1], "sentry id");
	const auto dx = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x value");
	const auto dy = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& sentryGun = server->world().findSentryGun(id)) {
		sentryGun.setAimDirection(Direction{dx, dy});
		return cmd::done();
	}
	return cmd::error("{}: Sentry gun with id \"{}\" not found.", self.getName(), id);
}

DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", x, entity.getPosition().x)
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", y, entity.getPosition().y)
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", type, entity.getType().getId())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", team, entity.getTeam().getId())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", move_x, entity.getMoveDirection().getX())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", move_y, entity.getMoveDirection().getY())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", owner, entity.getOwner())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", weapon, entity.getWeapon().getId())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", damage, entity.getDamage())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", time_left, entity.getTimeLeft())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", move_interval, entity.getMoveInterval())
DEFINE_GET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", sticky, entity.isStickyAttached())

DEFINE_SET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", owner, entity.setOwner(value), Number<World::PlayerId>)
DEFINE_SET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", weapon, entity.setWeapon(value), Weapon)
DEFINE_SET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", damage, entity.setDamage(value), Number<Health>)
DEFINE_SET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", time_left, entity.setTimeLeft(value), Number<float>)
DEFINE_SET_COMMAND(Projectile, projectile, "a", "Projectile", "projectile", move_interval, entity.setMoveInterval(value), Number<float>)

CON_COMMAND(mp_set_projectile_move, "<projectile_id> <dx> <dy>", ConCommand::SERVER, "Set the movement vector of a projectile.", {},
			cmd::suggestProjectileId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::ProjectileId>(parseError, argv[1], "projectile id");
	const auto dx = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x value");
	const auto dy = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& projectile = server->world().findProjectile(id)) {
		projectile.setMoveDirection(Direction{dx, dy});
		return cmd::done();
	}
	return cmd::error("{}: Projectile with id \"{}\" not found.", self.getName(), id);
}

DEFINE_GET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", x, entity.getPosition().x)
DEFINE_GET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", y, entity.getPosition().y)
DEFINE_GET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", team, entity.getTeam().getId())
DEFINE_GET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", owner, entity.getOwner())
DEFINE_GET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", weapon, entity.getWeapon().getId())
DEFINE_GET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", damage, entity.getDamage())
DEFINE_GET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", time_left, entity.getTimeLeft())

DEFINE_SET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", owner, entity.setOwner(value), Number<World::PlayerId>)
DEFINE_SET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", weapon, entity.setWeapon(value), Weapon)
DEFINE_SET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", damage, entity.setDamage(value), Number<Health>)
DEFINE_SET_COMMAND(Explosion, explosion, "an", "Explosion", "explosion", time_left, entity.setTimeLeft(value), Number<float>)

DEFINE_GET_COMMAND(Medkit, medkit, "a", "Medkit", "medkit", x, entity.getPosition().x)
DEFINE_GET_COMMAND(Medkit, medkit, "a", "Medkit", "medkit", y, entity.getPosition().y)
DEFINE_GET_COMMAND(Medkit, medkit, "a", "Medkit", "medkit", alive, entity.isAlive())
DEFINE_GET_COMMAND(Medkit, medkit, "a", "Medkit", "medkit", respawn_time_left, entity.getRespawnTimeLeft())

DEFINE_GET_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack", x, entity.getPosition().x)
DEFINE_GET_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack", y, entity.getPosition().y)
DEFINE_GET_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack", alive, entity.isAlive())
DEFINE_GET_COMMAND(Ammopack, ammopack, "an", "Ammopack", "ammopack", respawn_time_left, entity.getRespawnTimeLeft())

DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", name, entity.getName())
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", x, entity.getPosition().x)
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", y, entity.getPosition().y)
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", spawn_x, entity.getSpawnPosition().x)
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", spawn_y, entity.getSpawnPosition().y)
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", team, entity.getTeam().getId())
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", score, entity.getScore())
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", carrier, entity.getCarrier())
DEFINE_GET_COMMAND(Flag, flag, "a", "Flag", "flag", return_time_left, entity.getReturnTimeLeft())

CON_COMMAND(mp_set_flag_name, "<flag_id> <value>", ConCommand::SERVER, "Set the name of a flag.", {}, cmd::suggestFlagId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::FlagId>(parseError, argv[1], "flag id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& entity = server->world().findFlag(id)) {
		entity.setName(argv[2]);
		return cmd::done();
	}
	return cmd::error("{}: Flag with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_set_flag_spawn, "<flag_id> <x> <y>", ConCommand::SERVER, "Set the spawn of a flag.", {}, cmd::suggestFlagId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::FlagId>(parseError, argv[1], "flag id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& entity = server->world().findFlag(id)) {
		entity.setSpawnPosition(Vec2{x, y});
		return cmd::done();
	}
	return cmd::error("{}: Flag with id \"{}\" not found.", self.getName(), id);
}

DEFINE_GET_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart", x, entity.getPosition().x)
DEFINE_GET_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart", y, entity.getPosition().y)
DEFINE_GET_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart", team, entity.getTeam().getId())
DEFINE_GET_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart", track_size, entity.getTrackSize())
DEFINE_GET_COMMAND(PayloadCart, cart, "a", "Payload cart", "payload cart", track_index, entity.getTrackIndex())

DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", x, entity.getPosition().x)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", y, entity.getPosition().y)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", move_x, entity.getVelocity().x)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", move_y, entity.getVelocity().y)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", color, entity.getColor().getString())
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", move_interval, entity.getMoveInterval())
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", visible, entity.isVisible())
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", w, entity.matrix().getWidth())
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", h, entity.matrix().getHeight())
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", matrix, entity.matrix().getString())

CON_COMMAND(mp_get_ent_matrix_at, "<ent_id> <matrix_x> <matrix_y>", ConCommand::SERVER,
			"Get a character in the matrix of a generic entity.", {}, cmd::suggestGenericEntityId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::GenericEntityId>(parseError, argv[1], "generic entity id");
	const auto localX = cmd::parseNumber<std::size_t>(parseError, argv[2], "matrix x");
	const auto localY = cmd::parseNumber<std::size_t>(parseError, argv[3], "matrix y");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& entity = server->world().findGenericEntity(id)) {
		if (localX >= entity.matrix().getWidth()) {
			return cmd::error("{}: Matrix x out of range.", self.getName());
		}
		if (localY >= entity.matrix().getHeight()) {
			return cmd::error("{}: Matrix y out of range.", self.getName());
		}
		return cmd::done(entity.matrix().getUnchecked(localX, localY));
	}

	return cmd::error("{}: Generic entity with id \"{}\" not found.", self.getName(), id);
}

DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_world, (entity.getSolidFlags() & Solid::WORLD) == Solid::WORLD)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_environment,
				   (entity.getSolidFlags() & Solid::RED_ENVIRONMENT) == Solid::RED_ENVIRONMENT)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_environment,
				   (entity.getSolidFlags() & Solid::BLUE_ENVIRONMENT) == Solid::BLUE_ENVIRONMENT)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_player,
				   (entity.getSolidFlags() & Solid::RED_PLAYERS) == Solid::RED_PLAYERS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_player,
				   (entity.getSolidFlags() & Solid::BLUE_PLAYERS) == Solid::BLUE_PLAYERS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_projectile,
				   (entity.getSolidFlags() & Solid::RED_PROJECTILES) == Solid::RED_PROJECTILES)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_projectile,
				   (entity.getSolidFlags() & Solid::BLUE_PROJECTILES) == Solid::BLUE_PROJECTILES)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_explosion,
				   (entity.getSolidFlags() & Solid::RED_EXPLOSIONS) == Solid::RED_EXPLOSIONS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_explosion,
				   (entity.getSolidFlags() & Solid::BLUE_EXPLOSIONS) == Solid::BLUE_EXPLOSIONS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_sentry,
				   (entity.getSolidFlags() & Solid::RED_SENTRY_GUNS) == Solid::RED_SENTRY_GUNS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_sentry,
				   (entity.getSolidFlags() & Solid::BLUE_SENTRY_GUNS) == Solid::BLUE_SENTRY_GUNS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_medkit, (entity.getSolidFlags() & Solid::MEDKITS) == Solid::MEDKITS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_ammopack,
				   (entity.getSolidFlags() & Solid::AMMOPACKS) == Solid::AMMOPACKS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_flag,
				   (entity.getSolidFlags() & Solid::RED_FLAGS) == Solid::RED_FLAGS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_flag,
				   (entity.getSolidFlags() & Solid::BLUE_FLAGS) == Solid::BLUE_FLAGS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_cart,
				   (entity.getSolidFlags() & Solid::RED_PAYLOAD_CARTS) == Solid::RED_PAYLOAD_CARTS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_cart,
				   (entity.getSolidFlags() & Solid::BLUE_PAYLOAD_CARTS) == Solid::BLUE_PAYLOAD_CARTS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_ent,
				   (entity.getSolidFlags() & Solid::GENERIC_ENTITIES) == Solid::GENERIC_ENTITIES)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_environment,
				   (entity.getSolidFlags() & Solid::ENVIRONMENT) == Solid::ENVIRONMENT)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_player, (entity.getSolidFlags() & Solid::PLAYERS) == Solid::PLAYERS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_projectile,
				   (entity.getSolidFlags() & Solid::PROJECTILES) == Solid::PROJECTILES)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_explosion,
				   (entity.getSolidFlags() & Solid::EXPLOSIONS) == Solid::EXPLOSIONS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_sentry,
				   (entity.getSolidFlags() & Solid::SENTRY_GUNS) == Solid::SENTRY_GUNS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_flag, (entity.getSolidFlags() & Solid::FLAGS) == Solid::FLAGS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_cart,
				   (entity.getSolidFlags() & Solid::PAYLOAD_CARTS) == Solid::PAYLOAD_CARTS)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_all, (entity.getSolidFlags() & Solid::RED_ALL) == Solid::RED_ALL)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_all,
				   (entity.getSolidFlags() & Solid::BLUE_ALL) == Solid::BLUE_ALL)
DEFINE_GET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_all, (entity.getSolidFlags() & Solid::ALL) == Solid::ALL)

DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", color, entity.setColor(value), Color)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", move_interval, entity.setMoveInterval(value), Number<float>)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", visible, entity.setVisible(value), Bool)

CON_COMMAND(mp_set_ent_move, "<ent_id> <dx> <dy>", ConCommand::SERVER, "Set the movement vector of a generic entity.", {},
			cmd::suggestGenericEntityId<1>) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::GenericEntityId>(parseError, argv[1], "generic entity id");
	const auto dx = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x value");
	const auto dy = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& entity = server->world().findGenericEntity(id)) {
		entity.setVelocity(Vec2{dx, dy});
		return cmd::done();
	}
	return cmd::error("{}: Generic entity with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_set_ent_matrix, "<ent_id> <matrix>", ConCommand::SERVER, "Set the matrix of a generic entity.", {}, cmd::suggestGenericEntityId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::GenericEntityId>(parseError, argv[1], "generic entity id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (const auto& entity = server->world().findGenericEntity(id)) {
		entity.matrix() = util::TileMatrix<char>{argv[2]};
		return cmd::done();
	}
	return cmd::error("{}: Generic entity with id \"{}\" not found.", self.getName(), id);
}

CON_COMMAND(mp_set_ent_matrix_at, "<ent_id> <matrix_x> <matrix_y> <value>", ConCommand::SERVER,
			"Set a character in the matrix of a generic entity.", {}, cmd::suggestGenericEntityId<1>) {
	if (argv.size() != 5) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<World::GenericEntityId>(parseError, argv[1], "generic entity id");
	const auto localX = cmd::parseNumber<std::size_t>(parseError, argv[2], "matrix x");
	const auto localY = cmd::parseNumber<std::size_t>(parseError, argv[3], "matrix y");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (argv[4].size() != 1) {
		return cmd::error("{}: Invalid value: Must be exactly one character.");
	}

	assert(server);
	if (const auto& entity = server->world().findGenericEntity(id)) {
		if (localX >= entity.matrix().getWidth()) {
			return cmd::error("{}: Matrix x out of range.", self.getName());
		}
		if (localY >= entity.matrix().getHeight()) {
			return cmd::error("{}: Matrix y out of range.", self.getName());
		}
		entity.matrix().setUnchecked(localX, localY, argv[4].front());
		return cmd::done();
	}
	return cmd::error("{}: Generic entity with id \"{}\" not found.", self.getName(), id);
}

DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_world,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::WORLD) : (entity.getSolidFlags() & ~Solid::WORLD)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_environment,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_ENVIRONMENT) : (entity.getSolidFlags() & ~Solid::RED_ENVIRONMENT)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_environment,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_ENVIRONMENT) : (entity.getSolidFlags() & ~Solid::BLUE_ENVIRONMENT)),
				   Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_player,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_PLAYERS) : (entity.getSolidFlags() & ~Solid::RED_PLAYERS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_player,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_PLAYERS) : (entity.getSolidFlags() & ~Solid::BLUE_PLAYERS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_projectile,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_PROJECTILES) : (entity.getSolidFlags() & ~Solid::RED_PROJECTILES)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_projectile,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_PROJECTILES) : (entity.getSolidFlags() & ~Solid::BLUE_PROJECTILES)),
				   Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_explosion,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_EXPLOSIONS) : (entity.getSolidFlags() & ~Solid::RED_EXPLOSIONS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_explosion,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_EXPLOSIONS) : (entity.getSolidFlags() & ~Solid::BLUE_EXPLOSIONS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_sentry,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_SENTRY_GUNS) : (entity.getSolidFlags() & ~Solid::RED_SENTRY_GUNS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_sentry,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_SENTRY_GUNS) : (entity.getSolidFlags() & ~Solid::BLUE_SENTRY_GUNS)),
				   Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_medkit,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::MEDKITS) : (entity.getSolidFlags() & ~Solid::MEDKITS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_ammopack,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::AMMOPACKS) : (entity.getSolidFlags() & ~Solid::AMMOPACKS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_flag,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_FLAGS) : (entity.getSolidFlags() & ~Solid::RED_FLAGS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_flag,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_FLAGS) : (entity.getSolidFlags() & ~Solid::BLUE_FLAGS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_cart,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_PAYLOAD_CARTS) : (entity.getSolidFlags() & ~Solid::RED_PAYLOAD_CARTS)),
				   Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_cart,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_PAYLOAD_CARTS) : (entity.getSolidFlags() & ~Solid::BLUE_PAYLOAD_CARTS)),
				   Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_ent,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::GENERIC_ENTITIES) : (entity.getSolidFlags() & ~Solid::GENERIC_ENTITIES)),
				   Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_environment,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::ENVIRONMENT) : (entity.getSolidFlags() & ~Solid::ENVIRONMENT)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_player,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::PLAYERS) : (entity.getSolidFlags() & ~Solid::PLAYERS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_projectile,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::PROJECTILES) : (entity.getSolidFlags() & ~Solid::PROJECTILES)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_explosion,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::EXPLOSIONS) : (entity.getSolidFlags() & ~Solid::EXPLOSIONS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_sentry,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::SENTRY_GUNS) : (entity.getSolidFlags() & ~Solid::SENTRY_GUNS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_flag,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::FLAGS) : (entity.getSolidFlags() & ~Solid::FLAGS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_cart,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::PAYLOAD_CARTS) : (entity.getSolidFlags() & ~Solid::PAYLOAD_CARTS)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_red_all,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::RED_ALL) : (entity.getSolidFlags() & ~Solid::RED_ALL)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_blue_all,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::BLUE_ALL) : (entity.getSolidFlags() & ~Solid::BLUE_ALL)), Bool)
DEFINE_SET_COMMAND(GenericEntity, ent, "a", "Generic entity", "generic entity", solid_to_all,
				   entity.setSolidFlags((value) ? (entity.getSolidFlags() | Solid::ALL) : (entity.getSolidFlags() & ~Solid::ALL)), Bool)
