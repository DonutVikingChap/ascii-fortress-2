#include "world.hpp"

#include "../../console/command_utilities.hpp"       // cmd::...
#include "../../console/commands/world_commands.hpp" // mp_..., sv_max_shots_per_frame, sv_max_move_steps_per_frame
#include "../../console/environment.hpp"             // Environment
#include "../../gui/layout.hpp"                      // gui::VIEWPORT_...
#include "../../network/connection.hpp"              // net::Connection
#include "../../utilities/algorithm.hpp" // util::filter, util::eraseIf, util::anyOf, util::findIf, util::countIf, util::transform, util::collect
#include "../../utilities/match.hpp"  // util::match
#include "../../utilities/string.hpp" // util::toString
#include "../data/actions.hpp"        // Actions, Action
#include "../shared/entities.hpp"     // ent::sh::..., ent::findClosestDistanceSquared
#include "../shared/map.hpp"          // Map
#include "game_server.hpp"            // GameServer

#include <algorithm>     // std::min, std::max, std::clamp, std::find_if
#include <array>         // std::array
#include <cassert>       // assert
#include <cmath>         // std::ceil, std::round
#include <fmt/core.h>    // fmt::format
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair
#include <variant>       // std::get_if

World::World(const Map& map, GameServer& server)
	: m_map(map)
	, m_server(server) {}

auto World::reset() -> void {
	m_server.callIfDefined(Script::command({"on_map_end"}));
	m_server.resetClients();
	m_server.resetEnvironment();
	m_tickCount = 0;
	m_roundCountdown.reset();
	m_levelChangeCountdown.reset();
	m_players.clear();
	m_projectiles.clear();
	m_explosions.clear();
	m_sentryGuns.clear();
	m_medkits.clear();
	m_ammopacks.clear();
	m_genericEntities.clear();
	m_flags.clear();
	m_carts.clear();
	m_teamSpawns.clear();
	m_teamWins.clear();
	m_collisionMap.clear();
	m_mapTime = 0.0f;
	m_roundsPlayed = 0;
	m_awaitingLevelChange = false;
}

auto World::startMap() -> void {
	m_server.setObject("map_name", Environment::Constant{std::string{m_map.getName()}});
	m_server.callScript(m_map.getScript());
	m_server.callIfDefined(Script::command({"on_map_start"}));
	this->startRound();
}

auto World::resetRound() -> void {
	m_server.callIfDefined(Script::command({"on_round_reset"}));
	for (auto it = m_flags.stable_begin(); it != m_flags.stable_end(); ++it) {
		it->second->score = 0;
		this->returnFlag(it, false);
	}

	for (auto it = m_carts.stable_begin(); it != m_carts.stable_end(); ++it) {
		it->second->currentTrackIndex = 0;
		it->second->pushTimer.reset();
	}

	for (auto it = m_sentryGuns.stable_begin(); it != m_sentryGuns.stable_end(); ++it) {
		this->killSentryGun(it, m_players.stable_end());
	}

	for (auto it = m_players.stable_begin(); it != m_players.stable_end(); ++it) {
		if (it->second->alive) {
			this->killPlayer(it, false, m_players.stable_end());
			if (!it->second) {
				continue;
			}
			it->second->respawnCountdown.start(mp_round_end_time);
			it->second->respawning = true;
		} else if (it->second->team != Team::none() && it->second->team != Team::spectators()) {
			it->second->respawnCountdown.start(mp_round_end_time);
			it->second->respawning = true;
		}
	}

	for (auto it = m_medkits.stable_begin(); it != m_medkits.stable_end(); ++it) {
		it->second->respawnCountdown.reset();
		it->second->alive = true;
	}

	for (auto it = m_ammopacks.stable_begin(); it != m_ammopacks.stable_end(); ++it) {
		it->second->respawnCountdown.reset();
		it->second->alive = true;
	}

	this->startRound(mp_round_end_time);
}

auto World::win(Team team) -> void {
	const auto rounds = ++m_roundsPlayed;
	const auto wins = ++m_teamWins[team];
	m_server.callIfDefined(Script::command({"on_round_won", cmd::formatTeamId(team)}));
	m_server.playTeamSound(SoundId::victory(), SoundId::defeat(), team);
	m_server.writeServerChatMessage(fmt::format("{} team wins!", team.getName()));
	const auto winPoints = static_cast<Score>(mp_score_win);
	const auto losePoints = static_cast<Score>(mp_score_lose);
	for (const auto& [id, ptr] : m_players.stable()) {
		const auto& player = *ptr;
		if (player.team == team) {
			m_server.awardPlayerPoints(id, player.name, winPoints);
		} else if (player.team != Team::spectators()) {
			m_server.awardPlayerPoints(id, player.name, losePoints);
		}
	}
	this->resetRound();
	if ((mp_winlimit != 0 && wins >= mp_winlimit) || (mp_roundlimit != 0 && rounds >= mp_roundlimit) ||
		(mp_timelimit != 0.0f && m_mapTime >= mp_timelimit)) {
		m_levelChangeCountdown.start(mp_round_end_time);
		m_awaitingLevelChange = true;
	}

	if (mp_switch_teams_between_rounds && !m_awaitingLevelChange) {
		m_teamSwitchCountdown.start(mp_round_end_time * 0.75f);
		m_awaitingTeamSwitch = true;
	}
}

auto World::stalemate() -> void {
	const auto rounds = ++m_roundsPlayed;
	m_server.callIfDefined(Script::command({"on_stalemate"}));
	m_server.playGameSound(SoundId::stalemate());
	m_server.writeServerChatMessage("Stalemate!");
	this->resetRound();
	if ((mp_roundlimit != 0 && rounds >= mp_roundlimit) || (mp_timelimit != 0.0f && m_mapTime >= mp_timelimit)) {
		m_levelChangeCountdown.start(mp_round_end_time);
		m_awaitingLevelChange = true;
	}

	if (mp_switch_teams_between_rounds && !m_awaitingLevelChange) {
		m_teamSwitchCountdown.start(mp_round_end_time * 0.75f);
		m_awaitingTeamSwitch = true;
	}
}

auto World::startRound(float delay) -> void {
	if (!m_carts.empty()) {
		m_roundCountdown.start(mp_roundtime_payload + delay);
	} else if (!m_flags.empty()) {
		m_roundCountdown.start(mp_roundtime_ctf + delay);
	} else {
		m_roundCountdown.start(mp_roundtime_tdm + delay);
	}
	m_server.callIfDefined(Script::command({"on_round_start"}));
}

auto World::update(float deltaTime) -> void {
	if (m_levelChangeCountdown.advance(deltaTime, m_awaitingLevelChange).first) {
		m_server.changeLevelToNext();
		return;
	}

	++m_tickCount;
	m_mapTime += deltaTime;

	// Commit the current state of all the entity registries, reclaiming space
	// and making subsequent iterations faster.
	// NOTE: Commit will invalidate any iterators into the registry. To make
	// sure this doesn't happen in the middle of an update, it MUST be done
	// here at the top level in-between updates. Do NOT call commit inside any
	// of the lower level update functions! Also, make sure not to leak any
	// registry iterators to the public interface of this class, since they
	// will be invalidated here at the beginning of every frame!
	m_players.commit();
	m_projectiles.commit();
	m_explosions.commit();
	m_sentryGuns.commit();
	m_medkits.commit();
	m_ammopacks.commit();
	m_genericEntities.commit();
	m_flags.commit();
	m_carts.commit();

	// Update the collision map after committing so that any methods that use
	// it don't try to dereference invalidated iterators.
	this->updateCollisionMap();

	// Update entities.
	m_server.callIfDefined(Script::command({"on_pre_tick", util::toString(deltaTime)}));
	this->updatePlayers(deltaTime);
	this->updateSentryGuns(deltaTime);
	this->updateProjectiles(deltaTime);
	this->updateExplosions(deltaTime);
	this->updateMedkits(deltaTime);
	this->updateAmmopacks(deltaTime);
	this->updateGenericEntities(deltaTime);
	this->updateFlags(deltaTime);
	this->updatePayloadCarts(deltaTime);
	this->updateRoundState(deltaTime);
	this->updateTeamSwitchCountdown(deltaTime);
	m_server.callIfDefined(Script::command({"on_post_tick", util::toString(deltaTime)}));
}

auto World::getTickCount() const -> TickCount {
	return m_tickCount;
}

auto World::getMapTime() const -> float {
	return m_mapTime;
}

auto World::getRoundsPlayed() const -> int {
	return m_roundsPlayed;
}

auto World::takeSnapshot(PlayerId id) const -> Snapshot {
	auto snap = Snapshot{};
	snap.tickCount = m_tickCount;
	snap.roundSecondsLeft = static_cast<decltype(snap.roundSecondsLeft)>(std::ceil(m_roundCountdown.getTimeLeft()));

	const auto it = m_players.find(id);
	if (it == m_players.end()) {
		return snap;
	}

	auto&& [playerId, player] = *it;

	snap.selfPlayer.position = player.position;
	snap.selfPlayer.team = player.team;
	snap.selfPlayer.skinTeam = (player.disguised) ? player.team.getOppositeTeam() : player.team;
	snap.selfPlayer.alive = player.alive;
	snap.selfPlayer.aimDirection = player.aimDirection;
	snap.selfPlayer.playerClass = player.playerClass;
	snap.selfPlayer.health = player.health;
	snap.selfPlayer.primaryAmmo = player.primaryAmmo;
	snap.selfPlayer.secondaryAmmo = player.secondaryAmmo;
	snap.selfPlayer.hat = player.hat;

	snap.flagInfo.reserve(m_flags.size());
	snap.flags.reserve(m_flags.size());
	for (const auto& [id, flag] : m_flags) {
		auto flagInfoEntity = ent::sh::FlagInfo{};
		flagInfoEntity.team = flag.team;
		flagInfoEntity.score = flag.score;
		snap.flagInfo.push_back(flagInfoEntity);

		auto flagEntity = ent::sh::Flag{};
		flagEntity.position = flag.position;
		flagEntity.team = flag.team;
		snap.flags.push_back(flagEntity);
	}

	snap.cartInfo.reserve(m_carts.size());
	snap.carts.reserve(m_carts.size());
	for (const auto& [id, cart] : m_carts) {
		auto cartInfoEntity = ent::sh::PayloadCartInfo{};
		cartInfoEntity.team = cart.team;
		cartInfoEntity.progress = static_cast<std::uint16_t>(cart.currentTrackIndex);
		cartInfoEntity.trackLength = static_cast<std::uint16_t>(cart.track.size());
		snap.cartInfo.push_back(cartInfoEntity);

		if (cart.currentTrackIndex < cart.track.size()) {
			auto cartEntity = ent::sh::PayloadCart{};
			assert(cart.currentTrackIndex < cart.track.size());
			cartEntity.position = cart.track[cart.currentTrackIndex];
			cartEntity.team = cart.team;
			snap.carts.push_back(cartEntity);
		}
	}

	snap.playerInfo.reserve(m_players.size());
	snap.players.reserve(m_players.size() - 1);
	snap.corpses.reserve((m_players.size() + m_sentryGuns.size()) / 2);
	for (const auto& [id, otherPlayer] : m_players) {
		auto plyInfoEntity = ent::sh::PlayerInfo{};
		plyInfoEntity.id = id;
		plyInfoEntity.team = otherPlayer.team;
		plyInfoEntity.score = otherPlayer.score;
		plyInfoEntity.name = otherPlayer.name;
		if (player.team == Team::spectators() || otherPlayer.team == Team::spectators() || player.team == otherPlayer.team) {
			plyInfoEntity.playerClass = otherPlayer.playerClass;
		} else {
			plyInfoEntity.playerClass = PlayerClass::none();
		}
		plyInfoEntity.ping = otherPlayer.latestMeasuredPingDuration;
		snap.playerInfo.push_back(std::move(plyInfoEntity));

		if (otherPlayer.team != Team::spectators()) {
			if (!otherPlayer.alive) {
				auto corpseEntity = ent::sh::Corpse{};
				corpseEntity.position = otherPlayer.position;
				corpseEntity.team = otherPlayer.team;
				snap.corpses.push_back(corpseEntity);
			} else if (id != playerId) {
				auto playerEntity = ent::sh::Player{};
				playerEntity.position = otherPlayer.position;
				playerEntity.team = otherPlayer.team;
				if (otherPlayer.disguised && player.team != playerEntity.team) {
					playerEntity.team = playerEntity.team.getOppositeTeam();
				}
				playerEntity.aimDirection = otherPlayer.aimDirection;
				playerEntity.playerClass = otherPlayer.playerClass;
				playerEntity.hat = otherPlayer.hat;
				playerEntity.name = otherPlayer.name;
				snap.players.push_back(std::move(playerEntity));
			}
		}
	}

	snap.sentryGuns.reserve(m_sentryGuns.size());
	for (const auto& [id, sentryGun] : m_sentryGuns) {
		if (sentryGun.alive) {
			auto sentryGunEntity = ent::sh::SentryGun{};
			sentryGunEntity.position = sentryGun.position;
			sentryGunEntity.team = sentryGun.team;
			sentryGunEntity.aimDirection = sentryGun.aimDirection;
			sentryGunEntity.owner = sentryGun.owner;
			snap.sentryGuns.push_back(sentryGunEntity);
		} else {
			auto corpseEntity = ent::sh::Corpse{};
			corpseEntity.position = sentryGun.position;
			corpseEntity.team = sentryGun.team;
			snap.corpses.push_back(corpseEntity);
		}
	}

	snap.projectiles.reserve(m_projectiles.size());
	for (const auto& [id, projectile] : m_projectiles) {
		auto projectileEntity = ent::sh::Projectile{};
		projectileEntity.position = projectile.position;
		projectileEntity.team = projectile.team;
		projectileEntity.type = projectile.type;
		projectileEntity.owner = projectile.owner;
		snap.projectiles.push_back(projectileEntity);
	}

	snap.explosions.reserve(m_explosions.size());
	for (const auto& [id, explosion] : m_explosions) {
		auto explosionEntity = ent::sh::Explosion{};
		explosionEntity.position = explosion.position;
		explosionEntity.team = explosion.team;
		snap.explosions.push_back(explosionEntity);
	}

	snap.medkits.reserve(m_medkits.size());
	for (const auto& [id, medkit] : m_medkits) {
		if (medkit.alive) {
			auto medkitEntity = ent::sh::Medkit{};
			medkitEntity.position = medkit.position;
			snap.medkits.push_back(medkitEntity);
		}
	}

	snap.ammopacks.reserve(m_ammopacks.size());
	for (const auto& [id, ammopack] : m_ammopacks) {
		if (ammopack.alive) {
			auto ammopackEntity = ent::sh::Ammopack{};
			ammopackEntity.position = ammopack.position;
			snap.ammopacks.push_back(ammopackEntity);
		}
	}

	snap.genericEntities.reserve(m_genericEntities.size());
	for (const auto& [id, genericEntity] : m_genericEntities) {
		if (genericEntity.visible) {
			auto genericEntityEntity = ent::sh::GenericEntity{};
			genericEntityEntity.position = genericEntity.position;
			genericEntityEntity.matrix = genericEntity.matrix;
			genericEntityEntity.color = genericEntity.color;
			snap.genericEntities.push_back(std::move(genericEntityEntity));
		}
	}

	return snap;
}

auto World::createPlayer(Vec2 position, std::string name) -> PlayerId {
	const auto it = m_players.stable_emplace_back();

	it->second->position = position;
	it->second->name = std::move(name);
	m_server.callIfDefined(Script::command({"on_player_create", cmd::formatPlayerId(it->first)}));
	if (!it->second) {
		return PlayerRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return PlayerRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createProjectile(Vec2 position, Direction moveDirection, ProjectileType type, Team team, PlayerId owner, Weapon weapon,
							 Health damage, SoundId hurtSound, float disappearTime, float moveInterval) -> World::ProjectileId {
	const auto it = m_projectiles.stable_emplace_back();

	it->second->position = position;
	it->second->type = type;
	it->second->team = team;
	it->second->moveDirection = moveDirection;
	it->second->owner = owner;
	it->second->weapon = weapon;
	it->second->damage = damage;
	it->second->hurtSound = hurtSound;
	it->second->disappearTimer.start(disappearTime);
	it->second->moveInterval = moveInterval;
	if (it->second->type == ProjectileType::sticky()) {
		if (const auto itPlayer = m_players.find(it->second->owner); itPlayer != m_players.end()) {
			++itPlayer->second.nStickies;
		}
	}
	m_server.callIfDefined(Script::command({"on_projectile_create", cmd::formatProjectileId(it->first)}));
	if (!it->second) {
		return ProjectileRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return ProjectileRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createExplosion(Vec2 position, Team team, PlayerId owner, Weapon weapon, Health damage, SoundId hurtSound, float disappearTime)
	-> World::ExplosionId {
	const auto it = m_explosions.stable_emplace_back();

	it->second->position = position;
	it->second->team = team;
	it->second->owner = owner;
	it->second->weapon = weapon;
	it->second->damage = damage;
	it->second->hurtSound = hurtSound;
	it->second->disappearTimer.start(disappearTime);
	m_server.callIfDefined(Script::command({"on_explosion_create", cmd::formatExplosionId(it->first)}));
	if (!it->second) {
		return ExplosionRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return ExplosionRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createSentryGun(Vec2 position, Team team, Health health, PlayerId owner) -> World::SentryGunId {
	const auto it = m_sentryGuns.stable_emplace_back();

	it->second->position = position;
	it->second->team = team;
	it->second->health = health;
	it->second->owner = owner;
	it->second->shootTimer.setTimeLeft(mp_sentry_build_time);
	it->second->alive = true;
	m_server.callIfDefined(Script::command({"on_sentry_create", cmd::formatSentryGunId(it->first)}));
	if (!it->second) {
		return SentryGunRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return SentryGunRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createMedkit(Vec2 position) -> World::MedkitId {
	const auto it = m_medkits.stable_emplace_back();

	it->second->position = position;
	it->second->alive = true;
	m_server.callIfDefined(Script::command({"on_medkit_create", cmd::formatMedkitId(it->first)}));
	if (!it->second) {
		return MedkitRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return MedkitRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createAmmopack(Vec2 position) -> World::AmmopackId {
	const auto it = m_ammopacks.stable_emplace_back();

	it->second->position = position;
	it->second->alive = true;
	m_server.callIfDefined(Script::command({"on_ammopack_create", cmd::formatAmmopackId(it->first)}));
	if (!it->second) {
		return AmmopackRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return AmmopackRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createGenericEntity(Vec2 position) -> World::GenericEntityId {
	const auto it = m_genericEntities.stable_emplace_back();

	it->second->position = position;
	m_server.callIfDefined(Script::command({"on_ent_create", cmd::formatGenericEntityId(it->first)}));
	if (!it->second) {
		return GenericEntityRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createFlag(Vec2 position, Team team, std::string name) -> World::FlagId {
	const auto it = m_flags.stable_emplace_back();

	it->second->position = position;
	it->second->spawnPosition = position;
	it->second->team = team;
	it->second->name = std::move(name);
	m_server.callIfDefined(Script::command({"on_flag_create", cmd::formatFlagId(it->first)}));
	if (!it->second) {
		return FlagRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return FlagRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::createPayloadCart(Team team, std::vector<Vec2> track) -> World::PayloadCartId {
	const auto it = m_carts.stable_emplace_back();

	it->second->team = team;
	it->second->track = std::move(track);
	m_server.callIfDefined(Script::command({"on_cart_create", cmd::formatPayloadCartId(it->first)}));
	if (!it->second) {
		return PayloadCartRegistry::INVALID_KEY;
	}
	this->checkCollisions(it);
	if (!it->second) {
		return PayloadCartRegistry::INVALID_KEY;
	}
	return it->first;
}

auto World::spawnPlayer(PlayerId id) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->spawnPlayer(it);
		return true;
	}
	return false;
}

auto World::spawnMedkit(MedkitId id) -> bool {
	if (const auto it = m_medkits.stable_find(id); it != m_medkits.stable_end()) {
		this->spawnMedkit(it);
		return true;
	}
	return false;
}

auto World::spawnAmmopack(AmmopackId id) -> bool {
	if (const auto it = m_ammopacks.stable_find(id); it != m_ammopacks.stable_end()) {
		this->spawnAmmopack(it);
		return true;
	}
	return false;
}

auto World::applyDamageToPlayer(PlayerId id, Health damage, SoundId hurtSound, bool allowOverheal, PlayerId inflictor, Weapon weapon) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->applyDamageToPlayer(it, damage, hurtSound, allowOverheal, m_players.stable_find(inflictor), weapon);
		return true;
	}
	return false;
}

auto World::applyDamageToSentryGun(SentryGunId id, Health damage, SoundId hurtSound, bool allowOverheal, PlayerId inflictor) -> bool {
	if (const auto it = m_sentryGuns.stable_find(id); it != m_sentryGuns.stable_end()) {
		this->applyDamageToSentryGun(it, damage, hurtSound, allowOverheal, m_players.stable_find(inflictor));
		return true;
	}
	return false;
}

auto World::killPlayer(PlayerId id, bool announce, PlayerId killer, Weapon weapon) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->killPlayer(it, announce, m_players.stable_find(killer), weapon);
		return true;
	}
	return false;
}

auto World::killSentryGun(SentryGunId id, PlayerId killer) -> bool {
	if (const auto it = m_sentryGuns.stable_find(id); it != m_sentryGuns.stable_end()) {
		this->killSentryGun(it, m_players.stable_find(killer));
		return true;
	}
	return false;
}

auto World::killMedkit(MedkitId id, float respawnTime) -> bool {
	if (const auto it = m_medkits.stable_find(id); it != m_medkits.stable_end()) {
		it->second->respawnCountdown.start(respawnTime);
		it->second->alive = false;
		return true;
	}
	return false;
}

auto World::killAmmopack(AmmopackId id, float respawnTime) -> bool {
	if (const auto it = m_ammopacks.stable_find(id); it != m_ammopacks.stable_end()) {
		it->second->respawnCountdown.start(respawnTime);
		it->second->alive = false;
		return true;
	}
	return false;
}

auto World::deletePlayer(PlayerId id) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		m_server.callIfDefined(Script::command({"on_player_leave", cmd::formatPlayerId(it->first)}));
		this->cleanupSentryGuns(id);
		this->cleanupProjectiles(id);
		if (!it->second) {
			return true;
		}

		this->killPlayer(it, true, m_players.stable_end());
		if (!it->second) {
			return true;
		}

		m_players.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deleteProjectile(ProjectileId id) -> bool {
	if (const auto it = m_projectiles.stable_find(id); it != m_projectiles.stable_end()) {
		if (it->second->type == ProjectileType::sticky()) {
			if (const auto itPlayer = m_players.stable_find(it->second->owner); itPlayer != m_players.stable_end()) {
				--itPlayer->second->nStickies;
			}
		}
		m_projectiles.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deleteExplosion(ExplosionId id) -> bool {
	if (const auto it = m_explosions.stable_find(id); it != m_explosions.stable_end()) {
		m_explosions.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deleteSentryGun(SentryGunId id) -> bool {
	if (const auto it = m_sentryGuns.stable_find(id); it != m_sentryGuns.stable_end()) {
		m_sentryGuns.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deleteMedkit(MedkitId id) -> bool {
	if (const auto it = m_medkits.stable_find(id); it != m_medkits.stable_end()) {
		m_medkits.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deleteAmmopack(AmmopackId id) -> bool {
	if (const auto it = m_ammopacks.stable_find(id); it != m_ammopacks.stable_end()) {
		m_ammopacks.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deleteGenericEntity(GenericEntityId id) -> bool {
	if (const auto it = m_genericEntities.stable_find(id); it != m_genericEntities.stable_end()) {
		m_genericEntities.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deleteFlag(FlagId id) -> bool {
	if (const auto it = m_flags.stable_find(id); it != m_flags.stable_end()) {
		m_flags.stable_erase(it);
		return true;
	}
	return false;
}

auto World::deletePayloadCart(PayloadCartId id) -> bool {
	if (const auto it = m_carts.stable_find(id); it != m_carts.stable_end()) {
		m_carts.stable_erase(it);
		return true;
	}
	return false;
}

auto World::hasPlayerId(PlayerId id) const -> bool {
	return m_players.contains(id);
}

auto World::hasProjectileId(ProjectileId id) const -> bool {
	return m_projectiles.contains(id);
}

auto World::hasExplosionId(ExplosionId id) const -> bool {
	return m_explosions.contains(id);
}

auto World::hasSentryGunId(SentryGunId id) const -> bool {
	return m_sentryGuns.contains(id);
}

auto World::hasMedkitId(MedkitId id) const -> bool {
	return m_medkits.contains(id);
}

auto World::hasAmmopackId(AmmopackId id) const -> bool {
	return m_ammopacks.contains(id);
}

auto World::hasGenericEntityId(GenericEntityId id) const -> bool {
	return m_genericEntities.contains(id);
}

auto World::hasFlagId(FlagId id) const -> bool {
	return m_flags.contains(id);
}

auto World::hasPayloadCartId(PayloadCartId id) const -> bool {
	return m_carts.contains(id);
}

auto World::getPlayerCount() const -> std::size_t {
	return m_players.size();
}

auto World::getProjectileCount() const -> std::size_t {
	return m_projectiles.size();
}

auto World::getExplosionCount() const -> std::size_t {
	return m_explosions.size();
}

auto World::getSentryGunCount() const -> std::size_t {
	return m_sentryGuns.size();
}

auto World::getMedkitCount() const -> std::size_t {
	return m_medkits.size();
}

auto World::getAmmopackCount() const -> std::size_t {
	return m_ammopacks.size();
}

auto World::getGenericEntityCount() const -> std::size_t {
	return m_genericEntities.size();
}

auto World::getFlagCount() const -> std::size_t {
	return m_flags.size();
}

auto World::getPayloadCartCount() const -> std::size_t {
	return m_carts.size();
}

auto World::getAllPlayerIds() const -> std::vector<PlayerId> {
	return m_players | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<PlayerId>>();
}

auto World::getAllProjectileIds() const -> std::vector<World::ProjectileId> {
	return m_projectiles | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::ProjectileId>>();
}

auto World::getAllExplosionIds() const -> std::vector<World::ExplosionId> {
	return m_explosions | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::ExplosionId>>();
}

auto World::getAllSentryGunIds() const -> std::vector<World::SentryGunId> {
	return m_sentryGuns | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::SentryGunId>>();
}

auto World::getAllMedkitIds() const -> std::vector<World::MedkitId> {
	return m_medkits | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::MedkitId>>();
}

auto World::getAllAmmopackIds() const -> std::vector<World::AmmopackId> {
	return m_ammopacks | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::AmmopackId>>();
}

auto World::getAllGenericEntityIds() const -> std::vector<World::GenericEntityId> {
	return m_genericEntities | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::GenericEntityId>>();
}

auto World::getAllFlagIds() const -> std::vector<World::FlagId> {
	return m_flags | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::FlagId>>();
}

auto World::getAllPayloadCartIds() const -> std::vector<World::PayloadCartId> {
	return m_carts | util::transform([](const auto& kv) { return kv.first; }) | util::collect<std::vector<World::PayloadCartId>>();
}

auto World::findPlayer(PlayerId id) -> ent::sv::PlayerHandle {
	const auto it = m_players.stable_find(id);
	return ent::sv::PlayerHandle{(it == m_players.stable_end()) ? nullptr : it->second};
}

auto World::findPlayer(PlayerId id) const -> ent::sv::ConstPlayerHandle {
	const auto it = m_players.stable_find(id);
	return ent::sv::ConstPlayerHandle{(it == m_players.stable_end()) ? nullptr : it->second};
}

auto World::findProjectile(ProjectileId id) -> ent::sv::ProjectileHandle {
	const auto it = m_projectiles.stable_find(id);
	return ent::sv::ProjectileHandle{(it == m_projectiles.stable_end()) ? nullptr : it->second};
}

auto World::findProjectile(ProjectileId id) const -> ent::sv::ConstProjectileHandle {
	const auto it = m_projectiles.stable_find(id);
	return ent::sv::ConstProjectileHandle{(it == m_projectiles.stable_end()) ? nullptr : it->second};
}

auto World::findExplosion(ExplosionId id) -> ent::sv::ExplosionHandle {
	const auto it = m_explosions.stable_find(id);
	return ent::sv::ExplosionHandle{(it == m_explosions.stable_end()) ? nullptr : it->second};
}

auto World::findExplosion(ExplosionId id) const -> ent::sv::ConstExplosionHandle {
	const auto it = m_explosions.stable_find(id);
	return ent::sv::ConstExplosionHandle{(it == m_explosions.stable_end()) ? nullptr : it->second};
}

auto World::findSentryGun(SentryGunId id) -> ent::sv::SentryGunHandle {
	const auto it = m_sentryGuns.stable_find(id);
	return ent::sv::SentryGunHandle{(it == m_sentryGuns.stable_end()) ? nullptr : it->second};
}

auto World::findSentryGun(SentryGunId id) const -> ent::sv::ConstSentryGunHandle {
	const auto it = m_sentryGuns.stable_find(id);
	return ent::sv::ConstSentryGunHandle{(it == m_sentryGuns.stable_end()) ? nullptr : it->second};
}

auto World::findMedkit(MedkitId id) -> ent::sv::MedkitHandle {
	const auto it = m_medkits.stable_find(id);
	return ent::sv::MedkitHandle{(it == m_medkits.stable_end()) ? nullptr : it->second};
}

auto World::findMedkit(MedkitId id) const -> ent::sv::ConstMedkitHandle {
	const auto it = m_medkits.stable_find(id);
	return ent::sv::ConstMedkitHandle{(it == m_medkits.stable_end()) ? nullptr : it->second};
}

auto World::findAmmopack(AmmopackId id) -> ent::sv::AmmopackHandle {
	const auto it = m_ammopacks.stable_find(id);
	return ent::sv::AmmopackHandle{(it == m_ammopacks.stable_end()) ? nullptr : it->second};
}

auto World::findAmmopack(AmmopackId id) const -> ent::sv::ConstAmmopackHandle {
	const auto it = m_ammopacks.stable_find(id);
	return ent::sv::ConstAmmopackHandle{(it == m_ammopacks.stable_end()) ? nullptr : it->second};
}

auto World::findGenericEntity(GenericEntityId id) -> ent::sv::GenericEntityHandle {
	const auto it = m_genericEntities.stable_find(id);
	return ent::sv::GenericEntityHandle{(it == m_genericEntities.stable_end()) ? nullptr : it->second};
}

auto World::findGenericEntity(GenericEntityId id) const -> ent::sv::ConstGenericEntityHandle {
	const auto it = m_genericEntities.stable_find(id);
	return ent::sv::ConstGenericEntityHandle{(it == m_genericEntities.stable_end()) ? nullptr : it->second};
}

auto World::findFlag(FlagId id) -> ent::sv::FlagHandle {
	const auto it = m_flags.stable_find(id);
	return ent::sv::FlagHandle{(it == m_flags.stable_end()) ? nullptr : it->second};
}

auto World::findFlag(FlagId id) const -> ent::sv::ConstFlagHandle {
	const auto it = m_flags.stable_find(id);
	return ent::sv::ConstFlagHandle{(it == m_flags.stable_end()) ? nullptr : it->second};
}

auto World::findPayloadCart(PayloadCartId id) -> ent::sv::PayloadCartHandle {
	const auto it = m_carts.stable_find(id);
	return ent::sv::PayloadCartHandle{(it == m_carts.stable_end()) ? nullptr : it->second};
}

auto World::findPayloadCart(PayloadCartId id) const -> ent::sv::ConstPayloadCartHandle {
	const auto it = m_carts.stable_find(id);
	return ent::sv::ConstPayloadCartHandle{(it == m_carts.stable_end()) ? nullptr : it->second};
}

auto World::teleportPlayer(PlayerId id, Vec2 destination) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		return this->teleportPlayer(it, destination);
	}
	return false;
}

auto World::teleportProjectile(ProjectileId id, Vec2 destination) -> bool {
	if (const auto it = m_projectiles.stable_find(id); it != m_projectiles.stable_end()) {
		return this->teleportProjectile(it, destination);
	}
	return false;
}

auto World::teleportExplosion(ExplosionId id, Vec2 destination) -> bool {
	if (const auto it = m_explosions.stable_find(id); it != m_explosions.stable_end()) {
		return this->teleportExplosion(it, destination);
	}
	return false;
}

auto World::teleportSentryGun(SentryGunId id, Vec2 destination) -> bool {
	if (const auto it = m_sentryGuns.stable_find(id); it != m_sentryGuns.stable_end()) {
		return this->teleportSentryGun(it, destination);
	}
	return false;
}

auto World::teleportMedkit(MedkitId id, Vec2 destination) -> bool {
	if (const auto it = m_medkits.stable_find(id); it != m_medkits.stable_end()) {
		return this->teleportMedkit(it, destination);
	}
	return false;
}

auto World::teleportAmmopack(AmmopackId id, Vec2 destination) -> bool {
	if (const auto it = m_ammopacks.stable_find(id); it != m_ammopacks.stable_end()) {
		return this->teleportAmmopack(it, destination);
	}
	return false;
}

auto World::teleportGenericEntity(GenericEntityId id, Vec2 destination) -> bool {
	if (const auto it = m_genericEntities.stable_find(id); it != m_genericEntities.stable_end()) {
		return this->teleportGenericEntity(it, destination);
	}
	return false;
}

auto World::teleportFlag(FlagId id, Vec2 destination) -> bool {
	if (const auto it = m_flags.stable_find(id); it != m_flags.stable_end()) {
		return this->teleportFlag(it, destination);
	}
	return false;
}

auto World::findPlayerIdByName(std::string_view name) const -> PlayerId {
	const auto it = util::findIf(m_players, [&](const auto& kv) { return kv.second.name == name; });
	return (it == m_players.stable_end()) ? PlayerRegistry::INVALID_KEY : it->first;
}

auto World::isPlayerNameTaken(std::string_view name) const -> bool {
	return util::anyOf(m_players, [&](const auto& kv) { return kv.second.name == name; });
}

auto World::isPlayerCarryingFlag(PlayerId id) const -> bool {
	return util::anyOf(m_flags, [&](const auto& kv) { return kv.second.carrier == id; });
}

auto World::playerTeamSelect(PlayerId id, Team team, PlayerClass playerClass) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->playerTeamSelect(it, team, playerClass);
		return true;
	}
	return false;
}

auto World::resupplyPlayer(PlayerId id) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->resupplyPlayer(it);
		return true;
	}
	return false;
}

auto World::setPlayerNoclip(PlayerId id, bool value) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->setPlayerNoclip(it, value);
		return true;
	}
	return false;
}

auto World::setPlayerName(PlayerId id, std::string name) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->setPlayerName(it, std::move(name));
		return true;
	}
	return false;
}

auto World::equipPlayerHat(PlayerId id, Hat hat) -> bool {
	if (const auto it = m_players.stable_find(id); it != m_players.stable_end()) {
		this->equipPlayerHat(it, hat);
		return true;
	}
	return false;
}

auto World::setRoundTimeLeft(float roundTimeLeft) -> void {
	m_roundCountdown.start(roundTimeLeft);
}

auto World::addRoundTimeLeft(float roundTimeLeft) -> void {
	m_roundCountdown.addTimeLeft(roundTimeLeft);
}

auto World::getRoundTimeLeft() const -> float {
	return m_roundCountdown.getTimeLeft();
}

auto World::addSpawnPoint(Vec2 position, Team team) -> void {
	m_teamSpawns[team].spawnPoints.push_back(position);
}

auto World::containsSpawnPoint(const Rect& rect, Team team) const -> bool {
	if (const auto it = m_teamSpawns.find(team); it != m_teamSpawns.end()) {
		return util::anyOf(it->second.spawnPoints, [&](const auto& position) { return rect.contains(position); });
	}
	return false;
}

auto World::getTeamPlayerCounts() const -> std::unordered_map<Team, std::size_t> {
	auto playerCounts = std::unordered_map<Team, std::size_t>{};
	for (const auto& team : Team::getAll()) {
		if (team != Team::none() && team != Team::spectators()) {
			playerCounts[team] = 0;
		}
	}
	for (const auto& [id, player] : m_players) {
		if (player.team != Team::none() && player.team != Team::spectators()) {
			++playerCounts[player.team];
		}
	}
	return playerCounts;
}

auto World::getPlayerClassCount(Team team, PlayerClass playerClass) const -> std::size_t {
	return util::countIf(m_players, [&](const auto& kv) { return kv.second.team == team && kv.second.playerClass == playerClass; });
}

auto World::getTeamWins(Team team) const -> Score {
	if (const auto it = m_teamWins.find(team); it != m_teamWins.end()) {
		return it->second;
	}
	return Score{0};
}

auto World::updateCollisionMap() -> void {
	m_collisionMap.clear();
	m_collisionMap.reserve(m_players.size() + m_projectiles.size() + m_explosions.size() * 9 + m_sentryGuns.size() + m_medkits.size() +
						   m_ammopacks.size() + m_genericEntities.size() + m_flags.size() + m_carts.size());

	for (auto it = m_players.stable_begin(); it != m_players.stable_end(); ++it) {
		if (it->second->team != Team::spectators() && it->second->alive) {
			m_collisionMap[it->second->position].emplace_back(it);
		}
	}

	for (auto it = m_projectiles.stable_begin(); it != m_projectiles.stable_end(); ++it) {
		m_collisionMap[it->second->position].emplace_back(it);
	}

	for (auto it = m_explosions.stable_begin(); it != m_explosions.stable_end(); ++it) {
		constexpr auto r = Vec2::Length{1};

		const auto yFirst = static_cast<Vec2::Length>(it->second->position.y - r);
		const auto yLast = static_cast<Vec2::Length>(it->second->position.y + r);
		const auto xFirst = static_cast<Vec2::Length>(it->second->position.x - r);
		const auto xLast = static_cast<Vec2::Length>(it->second->position.x + r);
		for (auto y = yFirst; y <= yLast; ++y) {
			for (auto x = xFirst; x <= xLast; ++x) {
				m_collisionMap[Vec2{x, y}].emplace_back(it);
			}
		}
	}

	for (auto it = m_sentryGuns.stable_begin(); it != m_sentryGuns.stable_end(); ++it) {
		if (it->second->alive) {
			m_collisionMap[it->second->position].emplace_back(it);
		}
	}

	for (auto it = m_medkits.stable_begin(); it != m_medkits.stable_end(); ++it) {
		if (it->second->alive) {
			m_collisionMap[it->second->position].emplace_back(it);
		}
	}

	for (auto it = m_ammopacks.stable_begin(); it != m_ammopacks.stable_end(); ++it) {
		if (it->second->alive) {
			m_collisionMap[it->second->position].emplace_back(it);
		}
	}

	for (auto it = m_genericEntities.stable_begin(); it != m_genericEntities.stable_end(); ++it) {
		const auto xBegin = it->second->position.x;
		const auto yBegin = it->second->position.y;
		const auto xEnd = static_cast<Vec2::Length>(xBegin + it->second->matrix.getWidth());
		const auto yEnd = static_cast<Vec2::Length>(yBegin + it->second->matrix.getHeight());

		auto localY = std::size_t{0};
		for (auto y = yBegin; y != yEnd; ++y) {
			auto localX = std::size_t{0};
			for (auto x = xBegin; x != xEnd; ++x) {
				if (it->second->matrix.getUnchecked(localX, localY) != Map::AIR_CHAR) {
					m_collisionMap[Vec2{x, y}].emplace_back(it);
				}
				++localX;
			}
			++localY;
		}
	}

	for (auto it = m_flags.stable_begin(); it != m_flags.stable_end(); ++it) {
		m_collisionMap[it->second->position].emplace_back(it);
	}

	for (auto it = m_carts.stable_begin(); it != m_carts.stable_end(); ++it) {
		m_collisionMap[it->second->track[it->second->currentTrackIndex]].emplace_back(it);
	}
}

auto World::updatePlayers(float deltaTime) -> void {
	for (auto it = m_players.stable_begin(); it != m_players.stable_end();) {
		it = this->updatePlayer(it, deltaTime);
	}
}

auto World::updatePlayer(PlayerIterator it, float deltaTime) -> PlayerIterator {
	assert(it != m_players.stable_end());
	assert(it->second);
	if (it->second->respawnCountdown.advance(deltaTime, it->second->respawning).first) {
		this->spawnPlayer(it);
		return ++it;
	}

	if (it->second->playerClass == PlayerClass::spectator()) {
		this->updatePlayerSpectatorMovement(it, deltaTime);
		return ++it;
	}

	if (!it->second->alive) {
		return ++it;
	}

	this->updatePlayerMovement(it, deltaTime);
	if (!it->second || !it->second->alive) {
		return ++it;
	}

	this->updatePlayerWeapon(it,
							 deltaTime,
							 it->second->playerClass.getPrimaryWeapon(),
							 it->second->playerClass.getSecondaryWeapon(),
							 it->second->attack1Timer,
							 it->second->attack2Timer,
							 it->second->primaryAmmo,
							 it->second->attack1,
							 it->second->primaryReloadTimer);
	if (!it->second || !it->second->alive) {
		return ++it;
	}

	this->updatePlayerWeapon(it,
							 deltaTime,
							 it->second->playerClass.getSecondaryWeapon(),
							 it->second->playerClass.getPrimaryWeapon(),
							 it->second->attack2Timer,
							 it->second->attack1Timer,
							 it->second->secondaryAmmo,
							 it->second->attack2,
							 it->second->secondaryReloadTimer);
	return ++it;
}

auto World::updateSentryGuns(float deltaTime) -> void {
	for (auto it = m_sentryGuns.stable_begin(); it != m_sentryGuns.stable_end();) {
		it = this->updateSentryGun(it, deltaTime);
	}
}

auto World::updateSentryGun(SentryGunIterator it, float deltaTime) -> World::SentryGunIterator {
	assert(it != m_sentryGuns.stable_end());
	assert(it->second);
	if (it->second->despawnTimer.advance(deltaTime, !it->second->alive).first) {
		return m_sentryGuns.stable_erase(it);
	}

	if (!it->second->alive) {
		return ++it;
	}

	const auto isPotentialSentryGunTarget = [&](const auto& kv) {
		return kv.second->alive && kv.second->team != it->second->team && !kv.second->disguised &&
			   m_map.lineOfSight(it->second->position, kv.second->position);
	};
	const auto visibleEnemies = m_players.stable() | util::filter(isPotentialSentryGunTarget) |
								util::transform([](const auto& kv) { return *kv.second; });
	const auto closestEnemy = ent::findClosestDistanceSquared(visibleEnemies, it->second->position);
	const auto shouldShoot = [&] {
		if (closestEnemy.first != visibleEnemies.end()) {
			if (const auto range = static_cast<Vec2::Length>(mp_sentry_range); closestEnemy.second <= range * range) {
				it->second->aimDirection = Direction{closestEnemy.first->position - it->second->position};
				if (it->second->aimDirection.isAny()) {
					return true;
				}
			}
		}
		return false;
	}();

	for (auto ticks = it->second->shootTimer.advance(deltaTime, Weapon::sentry_gun().getShootInterval(), shouldShoot, sv_max_shots_per_frame);
		 ticks > 0;
		 --ticks) {
		constexpr auto weapon = Weapon::sentry_gun();
		constexpr auto projectileType = weapon.getProjectileType();
		m_server.playWorldSound(weapon.getShootSound(), it->second->position);
		this->createProjectile(it->second->position + it->second->aimDirection.getVector(),
							   it->second->aimDirection,
							   projectileType,
							   it->second->team,
							   it->second->owner,
							   weapon,
							   weapon.getDamage(),
							   weapon.getHurtSound(),
							   projectileType.getDisappearTime(),
							   projectileType.getMoveInterval());
		if (!it->second || !it->second->alive) {
			return ++it;
		}
	}
	return ++it;
}

auto World::updateProjectiles(float deltaTime) -> void {
	for (auto it = m_projectiles.stable_begin(); it != m_projectiles.stable_end();) {
		it = this->updateProjectile(it, deltaTime);
	}
}

auto World::updateProjectile(ProjectileIterator it, float deltaTime) -> World::ProjectileIterator {
	assert(it != m_projectiles.stable_end());
	assert(it->second);
	if (it->second->disappearTimer.advance(deltaTime, !it->second->stickyAttached).first) {
		if (it->second->type == ProjectileType::sticky()) {
			it->second->stickyAttached = true;
		} else {
			return m_projectiles.stable_erase(it);
		}
	}

	for (auto ticks = it->second->moveTimer.advance(deltaTime, it->second->moveInterval, !it->second->stickyAttached, sv_max_move_steps_per_frame);
		 ticks > 0;
		 --ticks) {
		this->stepProjectile(it, it->second->moveDirection);
		if (!it->second) {
			return ++it;
		}
	}
	return ++it;
}

auto World::updateExplosions(float deltaTime) -> void {
	for (auto it = m_explosions.stable_begin(); it != m_explosions.stable_end();) {
		it = this->updateExplosion(it, deltaTime);
	}
}

auto World::updateExplosion(ExplosionIterator it, float deltaTime) -> World::ExplosionIterator {
	assert(it != m_explosions.stable_end());
	assert(it->second);
	if (it->second->disappearTimer.advance(deltaTime).first) {
		return m_explosions.stable_erase(it);
	}
	return ++it;
}

auto World::updateMedkits(float deltaTime) -> void {
	for (auto it = m_medkits.stable_begin(); it != m_medkits.stable_end();) {
		it = this->updateMedkit(it, deltaTime);
	}
}

auto World::updateMedkit(MedkitIterator it, float deltaTime) -> World::MedkitIterator {
	assert(it != m_medkits.stable_end());
	assert(it->second);
	if (it->second->respawnCountdown.advance(deltaTime, !it->second->alive).first) {
		this->spawnMedkit(it);
	}
	return ++it;
}

auto World::updateAmmopacks(float deltaTime) -> void {
	for (auto it = m_ammopacks.stable_begin(); it != m_ammopacks.stable_end();) {
		it = this->updateAmmopack(it, deltaTime);
	}
}

auto World::updateAmmopack(AmmopackIterator it, float deltaTime) -> World::AmmopackIterator {
	assert(it != m_ammopacks.stable_end());
	assert(it->second);
	if (it->second->respawnCountdown.advance(deltaTime, !it->second->alive).first) {
		this->spawnAmmopack(it);
	}
	return ++it;
}

auto World::updateGenericEntities(float deltaTime) -> void {
	for (auto it = m_genericEntities.stable_begin(); it != m_genericEntities.stable_end();) {
		it = this->updateGenericEntity(it, deltaTime);
	}
}

auto World::updateGenericEntity(GenericEntityIterator it, float deltaTime) -> World::GenericEntityIterator {
	assert(it != m_genericEntities.stable_end());
	assert(it->second);
	for (auto loops = it->second->moveTimer.advance(deltaTime, it->second->moveInterval, it->second->velocity != Vec2{}, sv_max_move_steps_per_frame);
		 loops > 0;
		 --loops) {
		this->stepGenericEntity(it);
		if (!it->second) {
			return ++it;
		}
	}
	return ++it;
}

auto World::updateFlags(float deltaTime) -> void {
	for (auto it = m_flags.stable_begin(); it != m_flags.stable_end();) {
		it = this->updateFlag(it, deltaTime);
	}
}

auto World::updateFlag(FlagIterator it, float deltaTime) -> World::FlagIterator {
	assert(it != m_flags.stable_end());
	assert(it->second);
	if (it->second->returnCountdown.advance(deltaTime, it->second->returning).first) {
		this->returnFlag(it, true);
		return ++it;
	}

	if (it->second->carrier != PlayerRegistry::INVALID_KEY) {
		if (const auto itCarrier = m_players.stable_find(it->second->carrier); itCarrier != m_players.stable_end()) {
			it->second->position.x = itCarrier->second->position.x;
			it->second->position.y = static_cast<Vec2::Length>(itCarrier->second->position.y - 1);

			for (auto itOtherFlag = m_flags.stable_begin(); itOtherFlag != m_flags.stable_end(); ++itOtherFlag) {
				if (itOtherFlag->first != it->first && itOtherFlag->second->team == itCarrier->second->team) {
					if (Rect{static_cast<Rect::Length>(itOtherFlag->second->spawnPosition.x - 1),
							 static_cast<Rect::Length>(itOtherFlag->second->spawnPosition.y - 1),
							 3,
							 3}
							.contains(itCarrier->second->position)) {
						this->captureFlag(it, itCarrier);
						if (!it->second) {
							return ++it;
						}
						if (!itCarrier->second) {
							break;
						}
					}
				}
			}
		}
	}
	return ++it;
}

auto World::updatePayloadCarts(float deltaTime) -> void {
	for (auto it = m_carts.stable_begin(); it != m_carts.stable_end();) {
		it = this->updatePayloadCart(it, deltaTime);
	}
}

auto World::updatePayloadCart(PayloadCartIterator it, float deltaTime) -> World::PayloadCartIterator {
	assert(it != m_carts.stable_end());
	assert(it->second);
	if (it->second->currentTrackIndex + 1 >= it->second->track.size()) {
		return ++it;
	}

	const auto pushingPlayers = this->getPlayersPushingCart(it);
	const auto scaledDeltaTime = deltaTime + ((pushingPlayers.empty()) ?
												  0.0f :
                                                  (deltaTime * static_cast<float>(std::min(pushingPlayers.size() - 1, std::size_t{2})) * 0.25f));
	for (auto loops = it->second->pushTimer.advance(scaledDeltaTime, mp_payload_cart_push_time, !pushingPlayers.empty(), sv_max_move_steps_per_frame);
		 loops > 0;
		 --loops) {
		m_server.playWorldSound(SoundId::push_cart(), it->second->track[it->second->currentTrackIndex]);
		m_server.callIfDefined(Script::command({"on_push_cart", cmd::formatPayloadCartId(it->first)}));
		if (!it->second) {
			return ++it;
		}

		++it->second->currentTrackIndex;
		this->checkCollisions(it);
		if (!it->second) {
			return ++it;
		}

		if (it->second->currentTrackIndex + 1 == it->second->track.size()) {
			const auto team = it->second->team;
			for (const auto& itPushingPlayer : pushingPlayers) {
				if (itPushingPlayer->second) {
					const auto points = static_cast<Score>(mp_score_objective);
					itPushingPlayer->second->score += points;
					m_server.writeServerChatMessage(fmt::format("{} delivered the payload!", itPushingPlayer->second->name));
					m_server.awardPlayerPoints(itPushingPlayer->first, itPushingPlayer->second->name, points);
				}
			}
			m_server.callIfDefined(Script::command({"on_capture_cart", cmd::formatPayloadCartId(it->first)}));
			this->win(team);
			break;
		}
	}
	return ++it;
}

auto World::updateRoundState(float deltaTime) -> void {
	if (!mp_enable_round_time) {
		return;
	}

	const auto oldTime = m_roundCountdown.getTimeLeft();
	if (m_roundCountdown.advance(deltaTime).first) {
		if (m_carts.size() == 1) {
			this->win(m_carts.begin()->second.team.getOppositeTeam());
		} else if (!m_flags.empty()) {
			this->stalemate();
		} else {
			auto bestScore = Score{};
			auto secondBestScore = Score{};
			auto bestTeam = Team::none();
			auto scores = std::unordered_map<Team, Score>{};
			for (const auto& [id, player] : m_players) {
				auto& score = scores[player.team];
				score += player.score;
				if (score > bestScore) {
					secondBestScore = bestScore;
					bestScore = score;
					bestTeam = player.team;
				} else if (score > secondBestScore) {
					secondBestScore = score;
				}
			}
			if (bestScore == secondBestScore) {
				this->stalemate();
			} else {
				this->win(bestTeam);
			}

			for (auto&& [id, player] : m_players) {
				player.score = 0;
			}
		}
	} else if (oldTime > 300.0f && m_roundCountdown.getTimeLeft() <= 300.0f) {
		m_server.playGameSound(SoundId::ends_5min());
	} else if (oldTime > 60.0f && m_roundCountdown.getTimeLeft() <= 60.0f) {
		m_server.playGameSound(SoundId::ends_60sec());
	} else if (oldTime > 30.0f && m_roundCountdown.getTimeLeft() <= 30.0f) {
		m_server.playGameSound(SoundId::ends_30sec());
	} else if (oldTime > 10.0f && m_roundCountdown.getTimeLeft() <= 10.0f) {
		m_server.playGameSound(SoundId::ends_10sec());
	} else if (oldTime > 5.0f && m_roundCountdown.getTimeLeft() <= 5.0f) {
		m_server.playGameSound(SoundId::ends_5sec());
	} else if (oldTime > 4.0f && m_roundCountdown.getTimeLeft() <= 4.0f) {
		m_server.playGameSound(SoundId::ends_4sec());
	} else if (oldTime > 3.0f && m_roundCountdown.getTimeLeft() <= 3.0f) {
		m_server.playGameSound(SoundId::ends_3sec());
	} else if (oldTime > 2.0f && m_roundCountdown.getTimeLeft() <= 2.0f) {
		m_server.playGameSound(SoundId::ends_2sec());
	} else if (oldTime > 1.0f && m_roundCountdown.getTimeLeft() <= 1.0f) {
		m_server.playGameSound(SoundId::ends_1sec());
	}
}

auto World::updateTeamSwitchCountdown(float deltaTime) -> void {
	if (m_teamSwitchCountdown.advance(deltaTime, m_awaitingTeamSwitch).first) {
		m_awaitingTeamSwitch = false;
		m_server.writeServerChatMessage("Switching teams.");
		for (auto it = m_players.stable_begin(); it != m_players.stable_end(); ++it) {
			if (const auto oppositeTeam = it->second->team.getOppositeTeam(); oppositeTeam != it->second->team) {
				m_server.writePlayerTeamSelected(it->second->team, oppositeTeam, it->first);
				it->second->team = oppositeTeam;
			}
		}
	}
}

auto World::checkCollisions(PlayerIterator it) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	if (m_map.isResupplyLocker(it->second->position)) {
		this->resupplyPlayer(it);
		if (!it->second || !it->second->alive) {
			return;
		}
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[it->second->position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PlayerIterator itPlayer) -> bool {
					if (itPlayer->first == it->first) {
						foundSelf = true;
					}
					return true;
				},
				[&](ProjectileIterator itProjectile) -> bool {
					if (!this->canCollide(it, itProjectile)) {
						return true;
					}
					this->collide(it, itProjectile);
					return it->second && it->second->alive;
				},
				[&](ExplosionIterator itExplosion) -> bool {
					if (!this->canCollide(it, itExplosion)) {
						return true;
					}
					this->collide(it, itExplosion);
					return it->second && it->second->alive;
				},
				[&](MedkitIterator itMedkit) -> bool {
					if (!this->canCollide(it, itMedkit)) {
						return true;
					}
					this->collide(it, itMedkit);
					return it->second && it->second->alive;
				},
				[&](AmmopackIterator itAmmopack) -> bool {
					if (!this->canCollide(it, itAmmopack)) {
						return true;
					}
					this->collide(it, itAmmopack);
					return it->second && it->second->alive;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second && it->second->alive;
				},
				[&](FlagIterator itFlag) -> bool {
					if (!this->canCollide(it, itFlag)) {
						return true;
					}
					this->collide(it, itFlag);
					return it->second && it->second->alive;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(ProjectileIterator it) -> void {
	assert(it != m_projectiles.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	if (m_map.isSolid(it->second->position, it->second->team == Team::red(), it->second->team == Team::blue())) {
		if (it->second->type == ProjectileType::sticky()) {
			it->second->stickyAttached = true;
			it->second->position -= it->second->moveDirection.getVector();
		} else {
			if (it->second->type == ProjectileType::rocket()) {
				const auto explosionPosition = it->second->position - it->second->moveDirection.getVector();
				m_server.playWorldSound(SoundId::explosion(), explosionPosition);
				this->createExplosion(explosionPosition,
									  it->second->team,
									  it->second->owner,
									  it->second->weapon,
									  it->second->damage,
									  it->second->hurtSound,
									  mp_explosion_disappear_time);
				if (!it->second) {
					return;
				}
			}
			m_projectiles.stable_erase(it);
			return;
		}
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[it->second->position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PlayerIterator itPlayer) -> bool {
					if (!this->canCollide(it, itPlayer)) {
						return true;
					}
					this->collide(it, itPlayer);
					return it->second;
				},
				[&](ProjectileIterator itProjectile) -> bool {
					if (itProjectile->first == it->first) {
						foundSelf = true;
					} else if (this->canCollide(it, itProjectile)) {
						this->collide(it, itProjectile);
						return it->second;
					}
					return true;
				},
				[&](SentryGunIterator itSentryGun) -> bool {
					if (!this->canCollide(it, itSentryGun)) {
						return true;
					}
					this->collide(it, itSentryGun);
					return it->second;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(ExplosionIterator it) -> void {
	assert(it != m_explosions.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	constexpr auto r = Vec2::Length{1};

	const auto yFirst = static_cast<Vec2::Length>(it->second->position.y - r);
	const auto yLast = static_cast<Vec2::Length>(it->second->position.y + r);
	const auto xFirst = static_cast<Vec2::Length>(it->second->position.x - r);
	const auto xLast = static_cast<Vec2::Length>(it->second->position.x + r);
	for (auto y = yFirst; y <= yLast; ++y) {
		for (auto x = xFirst; x <= xLast; ++x) {
			this->checkCollisions(it, Vec2{x, y});
			if (!it->second) {
				return;
			}
		}
	}
}

auto World::checkCollisions(ExplosionIterator it, Vec2 position) -> void {
	assert(it != m_explosions.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PlayerIterator itPlayer) -> bool {
					if (!this->canCollide(it, itPlayer)) {
						return true;
					}
					this->collide(it, itPlayer);
					return it->second;
				},
				[&](ExplosionIterator itExplosion) -> bool {
					if (itExplosion->first == it->first) {
						foundSelf = true;
					}
					return true;
				},
				[&](SentryGunIterator itSentryGun) -> bool {
					if (!this->canCollide(it, itSentryGun)) {
						return true;
					}
					this->collide(it, itSentryGun);
					return it->second;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(SentryGunIterator it) -> void {
	assert(it != m_sentryGuns.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[it->second->position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](ProjectileIterator itProjectile) -> bool {
					if (!this->canCollide(it, itProjectile)) {
						return true;
					}
					this->collide(it, itProjectile);
					return it->second;
				},
				[&](ExplosionIterator itExplosion) -> bool {
					if (!this->canCollide(it, itExplosion)) {
						return true;
					}
					this->collide(it, itExplosion);
					return it->second;
				},
				[&](SentryGunIterator itSentryGun) -> bool {
					if (itSentryGun->first == it->first) {
						foundSelf = true;
					}
					return true;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(MedkitIterator it) -> void {
	assert(it != m_medkits.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[it->second->position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PlayerIterator itPlayer) -> bool {
					if (!this->canCollide(it, itPlayer)) {
						return true;
					}
					this->collide(it, itPlayer);
					return it->second;
				},
				[&](MedkitIterator itMedkit) -> bool {
					if (itMedkit->first == it->first) {
						foundSelf = true;
					}
					return true;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(AmmopackIterator it) -> void {
	assert(it != m_ammopacks.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[it->second->position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PlayerIterator itPlayer) -> bool {
					if (!this->canCollide(it, itPlayer)) {
						return true;
					}
					this->collide(it, itPlayer);
					return it->second;
				},
				[&](AmmopackIterator itAmmopack) -> bool {
					if (itAmmopack->first == it->first) {
						foundSelf = true;
					}
					return true;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(GenericEntityIterator it) -> void {
	assert(it != m_genericEntities.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	const auto xBegin = it->second->position.x;
	const auto yBegin = it->second->position.y;
	const auto xEnd = static_cast<Vec2::Length>(xBegin + it->second->matrix.getWidth());
	const auto yEnd = static_cast<Vec2::Length>(yBegin + it->second->matrix.getHeight());

	auto localY = std::size_t{0};
	for (auto y = yBegin; y != yEnd; ++y) {
		auto localX = std::size_t{0};
		for (auto x = xBegin; x != xEnd; ++x) {
			if (it->second->matrix.getUnchecked(localX, localY) != Map::AIR_CHAR) {
				this->checkCollisions(it, Vec2{x, y});
				if (!it->second) {
					return;
				}
			}
			++localX;
		}
		++localY;
	}
}

auto World::checkCollisions(GenericEntityIterator it, Vec2 position) -> void {
	assert(it != m_genericEntities.stable_end());
	assert(it->second);
	if (!this->isCollideable(it)) {
		return;
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PlayerIterator itPlayer) -> bool {
					if (!this->canCollide(it, itPlayer)) {
						return true;
					}
					this->collide(it, itPlayer);
					return it->second;
				},
				[&](ProjectileIterator itProjectile) -> bool {
					if (!this->canCollide(it, itProjectile)) {
						return true;
					}
					this->collide(it, itProjectile);
					return it->second;
				},
				[&](ExplosionIterator itExplosion) -> bool {
					if (!this->canCollide(it, itExplosion)) {
						return true;
					}
					this->collide(it, itExplosion);
					return it->second;
				},
				[&](SentryGunIterator itSentryGun) -> bool {
					if (!this->canCollide(it, itSentryGun)) {
						return true;
					}
					this->collide(it, itSentryGun);
					return it->second;
				},
				[&](MedkitIterator itMedkit) -> bool {
					if (!this->canCollide(it, itMedkit)) {
						return true;
					}
					this->collide(it, itMedkit);
					return it->second;
				},
				[&](AmmopackIterator itAmmopack) -> bool {
					if (!this->canCollide(it, itAmmopack)) {
						return true;
					}
					this->collide(it, itAmmopack);
					return it->second;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (itGenericEntity->first == it->first) {
						foundSelf = true;
					} else if (this->canCollide(it, itGenericEntity)) {
						this->collide(it, itGenericEntity);
						return it->second;
					}
					return true;
				},
				[&](FlagIterator itFlag) -> bool {
					if (!this->canCollide(it, itFlag)) {
						return true;
					}
					this->collide(it, itFlag);
					return it->second;
				},
				[&](PayloadCartIterator itCart) -> bool {
					if (!this->canCollide(it, itCart)) {
						return true;
					}
					this->collide(it, itCart);
					return it->second;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(FlagIterator it) -> void {
	assert(it->second);
	assert(it != m_flags.stable_end());
	if (!this->isCollideable(it)) {
		return;
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[it->second->position];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PlayerIterator itPlayer) -> bool {
					if (!this->canCollide(it, itPlayer)) {
						return true;
					}
					this->collide(it, itPlayer);
					return it->second;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second;
				},
				[&](FlagIterator itFlag) -> bool {
					if (itFlag->first == it->first) {
						foundSelf = true;
					}
					return true;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::checkCollisions(PayloadCartIterator it) -> void {
	assert(it->second);
	assert(it != m_carts.stable_end());
	assert(it->second->currentTrackIndex < it->second->track.size());
	if (!this->isCollideable(it)) {
		return;
	}

	auto foundSelf = false;

	auto& collidingEntities = m_collisionMap[it->second->track[it->second->currentTrackIndex]];
	for (auto i = std::size_t{0}; i < collidingEntities.size(); ++i) {
		if (!util::match(collidingEntities[i])(
				[&](PayloadCartIterator itCart) -> bool {
					if (itCart->first == it->first) {
						foundSelf = true;
					}
					return true;
				},
				[&](GenericEntityIterator itGenericEntity) -> bool {
					if (!this->canCollide(it, itGenericEntity)) {
						return true;
					}
					this->collide(it, itGenericEntity);
					return it->second;
				},
				[](auto) -> bool { return true; })) {
			return;
		}
	}

	if (!foundSelf) {
		collidingEntities.emplace_back(it);
	}
}

auto World::isCollideable(PlayerIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_players.stable_end());
	return it->second && it->second->alive;
}

auto World::isCollideable(ProjectileIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_projectiles.stable_end());
	return it->second;
}

auto World::isCollideable(ExplosionIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_explosions.stable_end());
	return it->second;
}

auto World::isCollideable(SentryGunIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_sentryGuns.stable_end());
	return it->second && it->second->alive;
}

auto World::isCollideable(MedkitIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_medkits.stable_end());
	return it->second && it->second->alive;
}

auto World::isCollideable(AmmopackIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_ammopacks.stable_end());
	return it->second && it->second->alive;
}

auto World::isCollideable(GenericEntityIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_genericEntities.stable_end());
	return it->second && it->second->solidFlags != Solid::NONE;
}

auto World::isCollideable(FlagIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_flags.stable_end());
	return it->second && it->second->carrier == PlayerRegistry::INVALID_KEY;
}

auto World::isCollideable(PayloadCartIterator it) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_carts.stable_end());
	return it->second;
}

auto World::canCollide(PlayerIterator itPlayer, ProjectileIterator itProjectile) const -> bool {
	return this->isCollideable(itPlayer) && this->isCollideable(itProjectile) && itProjectile->second->type != ProjectileType::sticky() &&
		   ((itProjectile->second->type == ProjectileType::heal_beam() && itPlayer->second->team == itProjectile->second->team &&
			 itPlayer->first != itProjectile->second->owner) ||
			(itProjectile->second->type != ProjectileType::heal_beam() && itPlayer->second->team != itProjectile->second->team));
}

auto World::canCollide(PlayerIterator itPlayer, ExplosionIterator itExplosion) const -> bool {
	return this->isCollideable(itPlayer) && this->isCollideable(itExplosion) &&
		   (itPlayer->second->team != itExplosion->second->team || itPlayer->first == itExplosion->second->owner);
}

auto World::canCollide(PlayerIterator itPlayer, MedkitIterator itMedkit) const -> bool {
	return this->isCollideable(itPlayer) && this->isCollideable(itMedkit);
}

auto World::canCollide(PlayerIterator itPlayer, AmmopackIterator itAmmopack) const -> bool {
	return this->isCollideable(itPlayer) && this->isCollideable(itAmmopack);
}

auto World::canCollide(PlayerIterator itPlayer, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itPlayer);
}

auto World::canCollide(PlayerIterator itPlayer, FlagIterator itFlag) const -> bool {
	return this->isCollideable(itPlayer) && this->isCollideable(itFlag) && itPlayer->second->team != itFlag->second->team;
}

auto World::canCollide(ProjectileIterator itProjectile, PlayerIterator itPlayer) const -> bool {
	return this->canCollide(itPlayer, itProjectile);
}

auto World::canCollide(ProjectileIterator itProjectileA, ProjectileIterator itProjectileB) const -> bool {
	return this->isCollideable(itProjectileA) && this->isCollideable(itProjectileB) && itProjectileA->second->team != itProjectileB->second->team &&
		   ((itProjectileA->second->stickyAttached &&
			 (itProjectileB->second->type == ProjectileType::bullet() || itProjectileB->second->type == ProjectileType::syringe() ||
			  itProjectileB->second->type == ProjectileType::sniper_trail())) ||
			(itProjectileB->second->stickyAttached &&
			 (itProjectileA->second->type == ProjectileType::bullet() || itProjectileA->second->type == ProjectileType::syringe() ||
			  itProjectileA->second->type == ProjectileType::sniper_trail())));
}

auto World::canCollide(ProjectileIterator itProjectile, SentryGunIterator itSentryGun) const -> bool {
	return this->canCollide(itSentryGun, itProjectile);
}

auto World::canCollide(ProjectileIterator itProjectile, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itProjectile);
}

auto World::canCollide(ExplosionIterator itExplosion, PlayerIterator itPlayer) const -> bool {
	return this->canCollide(itPlayer, itExplosion);
}

auto World::canCollide(ExplosionIterator itExplosion, SentryGunIterator itSentryGun) const -> bool {
	return this->canCollide(itSentryGun, itExplosion);
}

auto World::canCollide(ExplosionIterator itExplosion, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itExplosion);
}

auto World::canCollide(SentryGunIterator itSentryGun, ProjectileIterator itProjectile) const -> bool {
	return this->isCollideable(itSentryGun) && this->isCollideable(itProjectile) && itSentryGun->second->team != itProjectile->second->team &&
		   itProjectile->second->type != ProjectileType::sticky() && itProjectile->second->type != ProjectileType::heal_beam();
}

auto World::canCollide(SentryGunIterator itSentryGun, ExplosionIterator itExplosion) const -> bool {
	return this->isCollideable(itSentryGun) && this->isCollideable(itExplosion) && itSentryGun->second->team != itExplosion->second->team;
}

auto World::canCollide(SentryGunIterator itSentryGun, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itSentryGun);
}

auto World::canCollide(MedkitIterator itMedkit, PlayerIterator itPlayer) const -> bool {
	return this->canCollide(itPlayer, itMedkit);
}

auto World::canCollide(MedkitIterator itMedkit, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itMedkit);
}

auto World::canCollide(AmmopackIterator itAmmopack, PlayerIterator itPlayer) const -> bool {
	return this->canCollide(itPlayer, itAmmopack);
}

auto World::canCollide(AmmopackIterator itAmmopack, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itAmmopack);
}

auto World::canCollide(GenericEntityIterator itGenericEntity, PlayerIterator itPlayer) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itPlayer) && (itGenericEntity->second->solidFlags & Solid::PLAYERS) != 0 &&
		   !(itPlayer->second->team == Team::red() && (itGenericEntity->second->solidFlags & Solid::RED_PLAYERS) == 0) &&
		   !(itPlayer->second->team == Team::blue() && (itGenericEntity->second->solidFlags & Solid::BLUE_PLAYERS) == 0);
}

auto World::canCollide(GenericEntityIterator itGenericEntity, ProjectileIterator itProjectile) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itProjectile) &&
		   (itGenericEntity->second->solidFlags & Solid::PROJECTILES) != 0 &&
		   !(itProjectile->second->team == Team::red() && (itGenericEntity->second->solidFlags & Solid::RED_PROJECTILES) == 0) &&
		   !(itProjectile->second->team == Team::blue() && (itGenericEntity->second->solidFlags & Solid::BLUE_PROJECTILES) == 0);
}

auto World::canCollide(GenericEntityIterator itGenericEntity, ExplosionIterator itExplosion) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itExplosion) &&
		   (itGenericEntity->second->solidFlags & Solid::EXPLOSIONS) != 0 &&
		   !(itExplosion->second->team == Team::red() && (itGenericEntity->second->solidFlags & Solid::RED_EXPLOSIONS) == 0) &&
		   !(itExplosion->second->team == Team::blue() && (itGenericEntity->second->solidFlags & Solid::BLUE_EXPLOSIONS) == 0);
}

auto World::canCollide(GenericEntityIterator itGenericEntity, SentryGunIterator itSentryGun) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itSentryGun) &&
		   (itGenericEntity->second->solidFlags & Solid::SENTRY_GUNS) != 0 &&
		   !(itSentryGun->second->team == Team::red() && (itGenericEntity->second->solidFlags & Solid::RED_SENTRY_GUNS) == 0) &&
		   !(itSentryGun->second->team == Team::blue() && (itGenericEntity->second->solidFlags & Solid::BLUE_SENTRY_GUNS) == 0);
}

auto World::canCollide(GenericEntityIterator itGenericEntity, MedkitIterator itMedkit) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itMedkit) && (itGenericEntity->second->solidFlags & Solid::MEDKITS) != 0;
}

auto World::canCollide(GenericEntityIterator itGenericEntity, AmmopackIterator itAmmopack) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itAmmopack) && (itGenericEntity->second->solidFlags & Solid::AMMOPACKS) != 0;
}

auto World::canCollide(GenericEntityIterator itGenericEntityA, GenericEntityIterator itGenericEntityB) const -> bool {
	return this->isCollideable(itGenericEntityA) && this->isCollideable(itGenericEntityB) &&
		   (itGenericEntityA->second->solidFlags & Solid::GENERIC_ENTITIES) != 0 &&
		   (itGenericEntityB->second->solidFlags & Solid::GENERIC_ENTITIES) != 0;
}

auto World::canCollide(GenericEntityIterator itGenericEntity, FlagIterator itFlag) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itFlag) && (itGenericEntity->second->solidFlags & Solid::FLAGS) != 0 &&
		   !(itFlag->second->team == Team::red() && (itGenericEntity->second->solidFlags & Solid::RED_FLAGS) == 0) &&
		   !(itFlag->second->team == Team::blue() && (itGenericEntity->second->solidFlags & Solid::BLUE_FLAGS) == 0);
}

auto World::canCollide(GenericEntityIterator itGenericEntity, PayloadCartIterator itCart) const -> bool {
	return this->isCollideable(itGenericEntity) && this->isCollideable(itCart) && (itGenericEntity->second->solidFlags & Solid::PAYLOAD_CARTS) != 0 &&
		   !(itCart->second->team == Team::red() && (itGenericEntity->second->solidFlags & Solid::RED_PAYLOAD_CARTS) == 0) &&
		   !(itCart->second->team == Team::blue() && (itGenericEntity->second->solidFlags & Solid::BLUE_PAYLOAD_CARTS) == 0);
}

auto World::canCollide(FlagIterator itFlag, PlayerIterator itPlayer) const -> bool {
	return this->canCollide(itPlayer, itFlag);
}

auto World::canCollide(FlagIterator itFlag, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itFlag);
}

auto World::canCollide(PayloadCartIterator itCart, GenericEntityIterator itGenericEntity) const -> bool {
	return this->canCollide(itGenericEntity, itCart);
}

auto World::collide(PlayerIterator itPlayer, ProjectileIterator itProjectile) -> void {
	assert(this->canCollide(itPlayer, itProjectile));
	if (itProjectile->second->type == ProjectileType::rocket()) {
		m_server.playWorldSound(SoundId::explosion(), itProjectile->second->position);
		this->createExplosion(itProjectile->second->position,
							  itProjectile->second->team,
							  itProjectile->second->owner,
							  itProjectile->second->weapon,
							  itProjectile->second->damage,
							  itProjectile->second->hurtSound,
							  mp_explosion_disappear_time);
	} else {
		this->applyDamageToPlayer(itPlayer,
								  itProjectile->second->damage,
								  itProjectile->second->hurtSound,
								  false,
								  m_players.stable_find(itProjectile->second->owner),
								  itProjectile->second->weapon);
	}
	if (!itProjectile->second) {
		return;
	}
	if (itProjectile->second->type == ProjectileType::sticky()) {
		if (const auto itPlayer = m_players.stable_find(itProjectile->second->owner); itPlayer != m_players.stable_end()) {
			--itPlayer->second->nStickies;
		}
	}
	m_projectiles.stable_erase(itProjectile);
}

auto World::collide(PlayerIterator itPlayer, ExplosionIterator itExplosion) -> void {
	assert(this->canCollide(itPlayer, itExplosion));
	if (itExplosion->second->damagedPlayers.insert(itPlayer->first).second) {
		if (itPlayer->first == itExplosion->second->owner) {
			if (itPlayer->second->position == itExplosion->second->position) {
				const auto blastJumpOffset = this->getClippedMovementOffset(itPlayer->second->position,
																			itPlayer->second->team == Team::red(),
																			itPlayer->second->team == Team::blue(),
																			itPlayer->second->noclip,
																			itPlayer->second->moveDirection);
				auto blastJumpVector = itPlayer->second->blastJumpDirection.getVector();
				if (blastJumpOffset.x != -blastJumpVector.x) {
					blastJumpVector.x = blastJumpOffset.x;
				}
				if (blastJumpOffset.y != -blastJumpVector.y) {
					blastJumpVector.y = blastJumpOffset.y;
				}
				if (blastJumpVector == Vec2{0, 0}) {
					itPlayer->second->blastJumpDirection = itPlayer->second->aimDirection.getOpposite();
				} else {
					itPlayer->second->blastJumpDirection = Direction{blastJumpVector.x<0, blastJumpVector.x> 0,
																	 blastJumpVector.y<0, blastJumpVector.y> 0};
				}
			} else {
				itPlayer->second->blastJumpDirection |= Direction{itPlayer->second->position - itExplosion->second->position};
			}

			if (itPlayer->second->blastJumping) {
				itPlayer->second->blastJumpCountdown.start(mp_blast_jump_chain_duration);
				itPlayer->second->blastJumpInterval *= mp_blast_jump_chain_move_interval_coefficient;
			} else {
				itPlayer->second->blastJumping = true;
				itPlayer->second->blastJumpCountdown.start(mp_blast_jump_duration);
				itPlayer->second->blastJumpInterval = mp_blast_jump_move_interval;
			}
		}
		this->applyDamageToPlayer(itPlayer,
								  itExplosion->second->damage,
								  itExplosion->second->hurtSound,
								  false,
								  m_players.stable_find(itExplosion->second->owner),
								  itExplosion->second->weapon);
	}
}

auto World::collide(PlayerIterator itPlayer, MedkitIterator itMedkit) -> void {
	assert(this->canCollide(itPlayer, itMedkit));
	if (const auto classHealth = itPlayer->second->playerClass.getHealth(); itPlayer->second->health < classHealth) {
		itMedkit->second->respawnCountdown.start(mp_medkit_respawn_time);
		itMedkit->second->alive = false;
		itPlayer->second->health = itPlayer->second->playerClass.getHealth();
		m_server.playWorldSound(SoundId::medkit_collect(), itMedkit->second->position, itPlayer->first);
		m_server.callIfDefined(Script::command({"on_pickup_medkit", cmd::formatMedkitId(itMedkit->first), cmd::formatPlayerId(itPlayer->first)}));
	}
}

auto World::collide(PlayerIterator itPlayer, AmmopackIterator itAmmopack) -> void {
	assert(this->canCollide(itPlayer, itAmmopack));
	const auto primaryMaxAmmo = itPlayer->second->playerClass.getPrimaryWeapon().getAmmoPerClip();
	const auto secondaryMaxAmmo = itPlayer->second->playerClass.getSecondaryWeapon().getAmmoPerClip();
	if (itPlayer->second->primaryAmmo < primaryMaxAmmo || itPlayer->second->secondaryAmmo < secondaryMaxAmmo) {
		itAmmopack->second->respawnCountdown.start(mp_ammopack_respawn_time);
		itAmmopack->second->alive = false;
		itPlayer->second->primaryAmmo = primaryMaxAmmo;
		itPlayer->second->secondaryAmmo = secondaryMaxAmmo;
		m_server.playWorldSound(SoundId::player_spawn(), itAmmopack->second->position, itPlayer->first);
		m_server.callIfDefined(Script::command({"on_pickup_ammopack", cmd::formatAmmopackId(itAmmopack->first), cmd::formatPlayerId(itPlayer->first)}));
	}
}

auto World::collide(PlayerIterator itPlayer, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itPlayer, itGenericEntity));
	this->collide(itGenericEntity, itPlayer);
}

auto World::collide(PlayerIterator itPlayer, FlagIterator itFlag) -> void {
	assert(this->canCollide(itPlayer, itFlag));
	this->pickupFlag(itFlag, itPlayer);
}

auto World::collide(ProjectileIterator itProjectile, PlayerIterator itPlayer) -> void {
	assert(this->canCollide(itProjectile, itPlayer));
	this->collide(itPlayer, itProjectile);
}

auto World::collide(ProjectileIterator itProjectileA, ProjectileIterator itProjectileB) -> void {
	assert(this->canCollide(itProjectileA, itProjectileB));
	if (itProjectileB->second->stickyAttached) {
		std::swap(itProjectileA, itProjectileB);
	}
	m_server.playWorldSound(SoundId::sentry_hurt(), itProjectileA->second->position);
	if (itProjectileA->second->type == ProjectileType::sticky()) {
		if (const auto itPlayer = m_players.stable_find(itProjectileA->second->owner); itPlayer != m_players.stable_end()) {
			--itPlayer->second->nStickies;
		}
	}
	if (itProjectileB->second->type == ProjectileType::sticky()) {
		if (const auto itPlayer = m_players.stable_find(itProjectileB->second->owner); itPlayer != m_players.stable_end()) {
			--itPlayer->second->nStickies;
		}
	}
	m_projectiles.stable_erase(itProjectileA);
	m_projectiles.stable_erase(itProjectileB);
}

auto World::collide(ProjectileIterator itProjectile, SentryGunIterator itSentryGun) -> void {
	assert(this->canCollide(itProjectile, itSentryGun));
	this->collide(itSentryGun, itProjectile);
}

auto World::collide(ProjectileIterator itProjectile, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itProjectile, itGenericEntity));
	this->collide(itGenericEntity, itProjectile);
}

auto World::collide(ExplosionIterator itExplosion, PlayerIterator itPlayer) -> void {
	assert(this->canCollide(itExplosion, itPlayer));
	this->collide(itPlayer, itExplosion);
}

auto World::collide(ExplosionIterator itExplosion, SentryGunIterator itSentryGun) -> void {
	assert(this->canCollide(itExplosion, itSentryGun));
	this->collide(itSentryGun, itExplosion);
}

auto World::collide(ExplosionIterator itExplosion, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itExplosion, itGenericEntity));
	this->collide(itGenericEntity, itExplosion);
}

auto World::collide(SentryGunIterator itSentryGun, ProjectileIterator itProjectile) -> void {
	assert(this->canCollide(itSentryGun, itProjectile));
	if (itProjectile->second->type == ProjectileType::rocket()) {
		m_server.playWorldSound(SoundId::explosion(), itProjectile->second->position);
		this->createExplosion(itProjectile->second->position,
							  itProjectile->second->team,
							  itProjectile->second->owner,
							  itProjectile->second->weapon,
							  itProjectile->second->damage,
							  itProjectile->second->hurtSound,
							  mp_explosion_disappear_time);
	} else {
		this->applyDamageToSentryGun(itSentryGun,
									 itProjectile->second->damage,
									 SoundId::sentry_hurt(),
									 false,
									 m_players.stable_find(itProjectile->second->owner));
	}
	if (!itProjectile->second) {
		return;
	}
	if (itProjectile->second->type == ProjectileType::sticky()) {
		if (const auto itPlayer = m_players.stable_find(itProjectile->second->owner); itPlayer != m_players.stable_end()) {
			--itPlayer->second->nStickies;
		}
	}
	m_projectiles.stable_erase(itProjectile);
}

auto World::collide(SentryGunIterator itSentryGun, ExplosionIterator itExplosion) -> void {
	assert(this->canCollide(itSentryGun, itExplosion));
	if (itExplosion->second->damagedSentryGuns.insert(itSentryGun->second->owner).second) {
		this->applyDamageToSentryGun(itSentryGun,
									 itExplosion->second->damage,
									 SoundId::sentry_hurt(),
									 false,
									 m_players.stable_find(itExplosion->second->owner));
	}
}

auto World::collide(SentryGunIterator itSentryGun, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itSentryGun, itGenericEntity));
	this->collide(itGenericEntity, itSentryGun);
}

auto World::collide(MedkitIterator itMedkit, PlayerIterator itPlayer) -> void {
	assert(this->canCollide(itMedkit, itPlayer));
	this->collide(itPlayer, itMedkit);
}

auto World::collide(MedkitIterator itMedkit, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itMedkit, itGenericEntity));
	this->collide(itGenericEntity, itMedkit);
}

auto World::collide(AmmopackIterator itAmmopack, PlayerIterator itPlayer) -> void {
	assert(this->canCollide(itAmmopack, itPlayer));
	this->collide(itPlayer, itAmmopack);
}

auto World::collide(AmmopackIterator itAmmopack, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itAmmopack, itGenericEntity));
	this->collide(itGenericEntity, itAmmopack);
}

auto World::collide(GenericEntityIterator itGenericEntity, PlayerIterator itPlayer) -> void {
	assert(this->canCollide(itGenericEntity, itPlayer));
	m_server.callIfDefined(
		Script::command({"on_collide_ent_player", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatPlayerId(itPlayer->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntity, ProjectileIterator itProjectile) -> void {
	assert(this->canCollide(itGenericEntity, itProjectile));
	m_server.callIfDefined(Script::command(
		{"on_collide_ent_projectile", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatProjectileId(itProjectile->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntity, ExplosionIterator itExplosion) -> void {
	assert(this->canCollide(itGenericEntity, itExplosion));
	m_server.callIfDefined(Script::command(
		{"on_collide_ent_explosion", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatExplosionId(itExplosion->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntity, SentryGunIterator itSentryGun) -> void {
	assert(this->canCollide(itGenericEntity, itSentryGun));
	m_server.callIfDefined(Script::command(
		{"on_collide_ent_sentry", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatSentryGunId(itSentryGun->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntity, MedkitIterator itMedkit) -> void {
	assert(this->canCollide(itGenericEntity, itMedkit));
	m_server.callIfDefined(
		Script::command({"on_collide_ent_medkit", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatMedkitId(itMedkit->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntity, AmmopackIterator itAmmopack) -> void {
	assert(this->canCollide(itGenericEntity, itAmmopack));
	m_server.callIfDefined(Script::command(
		{"on_collide_ent_ammopack", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatAmmopackId(itAmmopack->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntityA, GenericEntityIterator itGenericEntityB) -> void {
	assert(this->canCollide(itGenericEntityA, itGenericEntityB));
	m_server.callIfDefined(Script::command(
		{"on_collide_ent_ent", cmd::formatGenericEntityId(itGenericEntityA->first), cmd::formatGenericEntityId(itGenericEntityB->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntity, FlagIterator itFlag) -> void {
	assert(this->canCollide(itGenericEntity, itFlag));
	m_server.callIfDefined(
		Script::command({"on_collide_ent_flag", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatFlagId(itFlag->first)}));
}

auto World::collide(GenericEntityIterator itGenericEntity, PayloadCartIterator itCart) -> void {
	assert(this->canCollide(itGenericEntity, itCart));
	m_server.callIfDefined(
		Script::command({"on_collide_ent_cart", cmd::formatGenericEntityId(itGenericEntity->first), cmd::formatPayloadCartId(itCart->first)}));
}

auto World::collide(FlagIterator itFlag, PlayerIterator itPlayer) -> void {
	assert(this->canCollide(itFlag, itPlayer));
	this->collide(itPlayer, itFlag);
}

auto World::collide(FlagIterator itFlag, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itFlag, itGenericEntity));
	this->collide(itGenericEntity, itFlag);
}

auto World::collide(PayloadCartIterator itCart, GenericEntityIterator itGenericEntity) -> void {
	assert(this->canCollide(itCart, itGenericEntity));
	this->collide(itGenericEntity, itCart);
}

auto World::teleportPlayer(PlayerIterator it, Vec2 destination) -> bool {
	assert(it != m_players.stable_end());
	assert(it->second);
	if (this->canTeleport(it->second->team == Team::red(), it->second->team == Team::blue(), it->second->noclip, destination)) {
		if (it->second->position != destination) {
			it->second->position = destination;
			this->checkCollisions(it);
		}
		return true;
	}
	return false;
}

auto World::teleportProjectile(ProjectileIterator it, Vec2 destination) -> bool {
	assert(it != m_projectiles.stable_end());
	assert(it->second);
	if (this->canTeleport(it->second->team == Team::red(), it->second->team == Team::blue(), false, destination)) {
		if (it->second->position != destination) {
			it->second->position = destination;
			this->checkCollisions(it);
		}
		return true;
	}
	return false;
}

auto World::teleportExplosion(ExplosionIterator it, Vec2 destination) -> bool {
	assert(it != m_explosions.stable_end());
	assert(it->second);
	if (this->canTeleport(it->second->team == Team::red(), it->second->team == Team::blue(), false, destination)) {
		if (it->second->position != destination) {
			it->second->position = destination;
			this->checkCollisions(it);
		}
		return true;
	}
	return false;
}

auto World::teleportSentryGun(SentryGunIterator it, Vec2 destination) -> bool {
	assert(it != m_sentryGuns.stable_end());
	assert(it->second);
	if (this->canTeleport(it->second->team == Team::red(), it->second->team == Team::blue(), false, destination)) {
		if (it->second->position != destination) {
			it->second->position = destination;
			this->checkCollisions(it);
		}
		return true;
	}
	return false;
}

auto World::teleportMedkit(MedkitIterator it, Vec2 destination) -> bool {
	assert(it != m_medkits.stable_end());
	assert(it->second);
	if (this->canTeleport(false, false, true, destination)) {
		if (it->second->position != destination) {
			it->second->position = destination;
			this->checkCollisions(it);
		}
		return true;
	}
	return false;
}

auto World::teleportAmmopack(AmmopackIterator it, Vec2 destination) -> bool {
	assert(it != m_ammopacks.stable_end());
	assert(it->second);
	if (this->canTeleport(false, false, true, destination)) {
		if (it->second->position != destination) {
			it->second->position = destination;
			this->checkCollisions(it);
		}
		return true;
	}
	return false;
}

auto World::teleportGenericEntity(GenericEntityIterator it, Vec2 destination) -> bool {
	assert(it != m_genericEntities.stable_end());
	assert(it->second);
	const auto xEnd = static_cast<Vec2::Length>(destination.x + it->second->matrix.getWidth());
	const auto yEnd = static_cast<Vec2::Length>(destination.y + it->second->matrix.getHeight());
	const auto red = (it->second->solidFlags & Solid::RED_ENVIRONMENT) == 0;
	const auto blue = (it->second->solidFlags & Solid::BLUE_ENVIRONMENT) == 0;
	const auto noclip = (it->second->solidFlags & Solid::WORLD) == 0;

	auto localY = std::size_t{0};
	for (auto y = destination.y; y != yEnd; ++y) {
		auto localX = std::size_t{0};
		for (auto x = destination.x; x != xEnd; ++x) {
			if (it->second->matrix.getUnchecked(localX, localY) != Map::AIR_CHAR) {
				if (!this->canTeleport(red, blue, noclip, Vec2{x, y})) {
					return false;
				}
			}
			++localX;
		}
		++localY;
	}

	if (it->second->position != destination) {
		it->second->position = destination;
		this->checkCollisions(it);
	}
	return true;
}

auto World::teleportFlag(FlagIterator it, Vec2 destination) -> bool {
	assert(it != m_flags.stable_end());
	assert(it->second);
	if (it->second->carrier == PlayerRegistry::INVALID_KEY &&
		this->canTeleport(it->second->team == Team::red(), it->second->team == Team::blue(), false, destination)) {
		if (it->second->position != destination) {
			it->second->position = destination;
			this->checkCollisions(it);
		}
		return true;
	}
	return false;
}

auto World::applyDamageToPlayer(PlayerIterator it, Health damage, SoundId hurtSound, bool allowOverheal, PlayerIterator inflictor, Weapon weapon) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	assert(inflictor == m_players.stable_end() || inflictor->second);
	const auto hasInflictor = inflictor != m_players.stable_end();
	if (hasInflictor && inflictor->first == it->first && weapon != Weapon::none()) {
		damage = static_cast<Health>(std::round(static_cast<float>(damage) * mp_self_damage_coefficient));
	}

	const auto previousHealth = it->second->health;
	if (damage < 0) {
		const auto maxHealth = it->second->playerClass.getHealth();
		if (it->second->health < maxHealth) {
			it->second->health = std::max(it->second->health - damage, Health{0});
			if (!allowOverheal) {
				it->second->health = std::min(it->second->health, it->second->playerClass.getHealth());
			}
		}
	} else {
		it->second->health = std::max(it->second->health - damage, Health{0});
	}

	if (hurtSound != SoundId::none()) {
		m_server.playWorldSound(hurtSound, it->second->position, it->first);
	}

	if (hasInflictor && inflictor->first != it->first) {
		const auto damageDealt = previousHealth - it->second->health;
		if (damageDealt < 0) {
			const auto points = static_cast<Score>(mp_score_heal);
			inflictor->second->score += points;
			m_server.awardPlayerPoints(inflictor->first, inflictor->second->name, points);
		}
		if (!it->second->disguised) {
			m_server.writeHitConfirmed(damageDealt, inflictor->first);
		}
	}

	if (it->second && it->second->health == 0) {
		this->killPlayer(it, true, inflictor, weapon);
	}
}

auto World::applyDamageToSentryGun(SentryGunIterator it, Health damage, SoundId hurtSound, bool allowOverheal, PlayerIterator inflictor) -> void {
	assert(it != m_sentryGuns.stable_end());
	assert(it->second);
	assert(it->second->alive);
	it->second->health = std::max(static_cast<Health>(it->second->health - damage), Health{0});
	if (!allowOverheal) {
		it->second->health = std::min(it->second->health, static_cast<Health>(mp_sentry_health));
	}

	if (hurtSound != SoundId::none()) {
		m_server.playWorldSound(hurtSound, it->second->position);
	}

	if (it->second->health == 0) {
		this->killSentryGun(it, inflictor);
	}
}

auto World::killPlayer(PlayerIterator it, bool announce, PlayerIterator killer, Weapon weapon) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	assert(killer == m_players.stable_end() || killer->second);
	const auto hasKiller = killer != m_players.stable_end();
	const auto wasAlive = it->second->alive;
	it->second->health = 0;
	it->second->primaryAmmo = 0;
	it->second->secondaryAmmo = 0;
	it->second->disguised = false;
	it->second->moveTimer.reset();
	it->second->attack1Timer.reset();
	it->second->attack2Timer.reset();
	it->second->primaryReloadTimer.reset();
	it->second->secondaryReloadTimer.reset();
	it->second->blastJumpDirection = Direction{};
	it->second->blastJumpTimer.reset();
	it->second->blastJumpCountdown.reset();
	it->second->blastJumping = false;
	it->second->blastJumpInterval = 0.0f;
	it->second->alive = false;

	// Drop flag if carried by player.
	for (auto itFlag = m_flags.stable_begin(); itFlag != m_flags.stable_end(); ++itFlag) {
		if (itFlag->second->carrier == it->first) {
			this->dropFlag(itFlag, it);
			if (!it->second) {
				return;
			}
		}
	}

	this->removePlayerStickies(it);
	if (!it->second) {
		return;
	}

	if (wasAlive && announce) {
		m_server.playWorldSound(SoundId::player_death(), it->second->position, it->first);

		// Announce death and award points.
		if (hasKiller) {
			const auto points = static_cast<Score>(mp_score_kill);
			if (killer->first == it->first) {
				it->second->score -= points;
				m_server.writeServerEventMessage(fmt::format("{} died.", it->second->name), std::array{it->first});
			} else {
				if (killer->second) {
					if (it->second) {
						if (weapon == Weapon::none()) {
							m_server.writeServerEventMessage(fmt::format("{} killed {}.", killer->second->name, it->second->name),
															 std::array{killer->first, it->first});
						} else {
							m_server.writeServerEventMessage(
								fmt::format("{} killed {} with {}.", killer->second->name, it->second->name, weapon.getName()),
								std::array{killer->first, it->first});
						}
					}
					killer->second->score += points;
					m_server.awardPlayerPoints(killer->first, killer->second->name, points);
				}
			}
		}

		m_server.callIfDefined(Script::command(
			{"on_kill_player", cmd::formatPlayerId(it->first), cmd::formatPlayerId((hasKiller) ? killer->first : PlayerRegistry::INVALID_KEY)}));
	}

	// Set respawn timer.
	if (it->second) {
		if (it->second->team == Team::none() || it->second->team == Team::spectators()) {
			it->second->respawnCountdown.reset();
			it->second->respawning = false;
		} else if (!it->second->respawning) {
			auto respawnTime = static_cast<float>(mp_player_respawn_time);
			for (const auto& [id, cart] : m_carts) {
				if (cart.team != it->second->team && static_cast<float>(cart.currentTrackIndex) / static_cast<float>(cart.track.size()) >=
														 mp_payload_defense_respawn_time_threshold) {
					respawnTime *= mp_payload_defense_respawn_time_coefficient;
				}
			}
			if (announce) {
				m_server.writeServerEventMessagePersonal(fmt::format("Respawning in {:g} seconds...", respawnTime), it->first);
			}
			it->second->respawnCountdown.start(respawnTime);
			it->second->respawning = true;
		}
	}
}

auto World::killSentryGun(SentryGunIterator it, PlayerIterator killer) -> void {
	assert(it != m_sentryGuns.stable_end());
	assert(it->second);
	assert(killer == m_players.stable_end() || killer->second);
	const auto hasKiller = killer != m_players.stable_end();
	it->second->health = 0;
	if (it->second->alive) {
		it->second->despawnTimer.start(mp_sentry_despawn_time);
		it->second->alive = false;
		m_server.playWorldSound(SoundId::sentry_death(), it->second->position);
		if (hasKiller) {
			const auto points = static_cast<Score>(mp_score_kill_sentry);
			killer->second->score += points;
			m_server.awardPlayerPoints(killer->first, killer->second->name, points);
		}
		m_server.callIfDefined(Script::command(
			{"on_kill_sentry", cmd::formatSentryGunId(it->first), cmd::formatPlayerId((hasKiller) ? killer->first : PlayerRegistry::INVALID_KEY)}));
	}
}

auto World::updatePlayerSpectatorMovement(PlayerIterator it, float deltaTime) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	const auto moveVector = it->second->moveDirection.getVector();
	for (auto loops = it->second->moveTimer.advance(deltaTime, PlayerClass::spectator().getMoveInterval(), moveVector != Vec2{}, sv_max_move_steps_per_frame);
		 loops > 0;
		 --loops) {
		it->second->position += moveVector;
		const auto xMin = static_cast<Vec2::Length>(gui::VIEWPORT_W / 2);
		const auto xMax = static_cast<Vec2::Length>(m_map.getWidth() - 1 - gui::VIEWPORT_W / 2);
		if (it->second->position.x < xMin) {
			it->second->position.x = xMin;
		} else if (it->second->position.x > xMax) {
			it->second->position.x = xMax;
		}

		const auto yMin = static_cast<Vec2::Length>(gui::VIEWPORT_H / 2);
		const auto yMax = static_cast<Vec2::Length>(m_map.getHeight() - 1 - gui::VIEWPORT_H / 2);
		if (it->second->position.y < yMin) {
			it->second->position.y = yMin;
		} else if (it->second->position.y > yMax) {
			it->second->position.y = yMax;
		}
	}
}

auto World::updatePlayerMovement(PlayerIterator it, float deltaTime) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	assert(it->second->alive);
	for (auto loops = it->second->blastJumpTimer.advance(deltaTime, it->second->blastJumpInterval, it->second->blastJumping, sv_max_move_steps_per_frame);
		 loops > 0;
		 --loops) {
		if (it->second->blastJumpCountdown.advance(it->second->blastJumpInterval, it->second->blastJumping).first) {
			it->second->blastJumpDirection = Direction::none();
			it->second->blastJumpInterval = 0.0f;
			it->second->blastJumping = false;
			break;
		}
		this->stepPlayer(it, it->second->blastJumpDirection);
		if (!it->second || !it->second->alive) {
			return;
		}
	}

	for (auto loops = it->second->moveTimer.advance(deltaTime,
													it->second->playerClass.getMoveInterval(),
													!it->second->blastJumping && it->second->moveDirection.isAny(),
													sv_max_move_steps_per_frame);
		 loops > 0;
		 --loops) {
		this->stepPlayer(it, it->second->moveDirection);
		if (!it->second || !it->second->alive) {
			return;
		}
	}
}

auto World::updatePlayerWeapon(PlayerIterator it, float deltaTime, Weapon weapon, Weapon otherWeapon,
							   util::CountdownLoop<float>& shootTimer, util::CountdownLoop<float>& secondaryShootTimer, Ammo& ammo,
							   bool shooting, util::CountdownLoop<float>& reloadTimer) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	assert(it->second->alive);
	assert(it->second->aimDirection.isAny());
	const auto shootInterval = weapon.getShootInterval();
	const auto ammoPerShot = weapon.getAmmoPerShot();
	const auto ammoPerClip = weapon.getAmmoPerClip();
	const auto reloadDelay = weapon.getReloadDelay();

	if (weapon == Weapon::knife()) {
		shooting = this->isKnifeTarget(it->second->position + it->second->aimDirection.getVector(), it->second->team);
	}

	const auto reloadTime = reloadTimer.getTimeLeft();

	reloadTimer.advance(deltaTime, shootInterval, (!shooting || ammo < ammoPerShot) && ammo < ammoPerClip, [&]() -> bool {
		ammo = std::min(static_cast<Ammo>(ammo + ammoPerShot), ammoPerClip);
		return ammo < ammoPerClip;
	});

	if (const auto reloadSoundTime = shootInterval * 0.5f; reloadTime > reloadSoundTime && reloadTimer.getTimeLeft() <= reloadSoundTime) {
		if (const auto reloadSound = weapon.getReloadSound(); reloadSound != SoundId::none()) {
			m_server.playWorldSound(reloadSound, it->second->position, it->first);
		}
	}

	const auto shootTime = shootTimer.getTimeLeft();

	auto shots = 0;
	for (auto loops = shootTimer.advance(deltaTime,
										 shootInterval,
										 shooting && ammo >= ammoPerShot,
										 [&] {
											 if (shots >= sv_max_shots_per_frame) {
												 return false;
											 }
											 if (ammo >= ammoPerShot) {
												 ammo -= ammoPerShot;
												 ++shots;
												 return true;
											 }
											 return false;
										 });
		 loops > 0;
		 --loops) {
		reloadTimer.setTimeLeft(shootInterval + reloadDelay - deltaTime);
		this->shootPlayerWeapon(it, weapon, otherWeapon, secondaryShootTimer);
		if (!it->second || !it->second->alive) {
			return;
		}
	}

	if (shooting && ammo < ammoPerShot && shootTime > 0.0f && shootTimer.getTimeLeft() <= 0.0f) {
		m_server.playWorldSound(SoundId::dry_fire(), it->second->position, it->first);
	}
}

auto World::shootPlayerWeapon(PlayerIterator it, Weapon weapon, Weapon otherWeapon, util::CountdownLoop<float>& secondaryShootTimer) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	assert(it->second->alive);

	if (weapon != Weapon::disguise_kit()) {
		it->second->disguised = false;
	}

	if (const auto shootSound = weapon.getShootSound(); shootSound != SoundId::none()) {
		m_server.playWorldSound(shootSound, it->second->position, it->first);
	}

	switch (weapon) {
		case Weapon::scattergun(): [[fallthrough]];
		case Weapon::shotgun():
			if (const auto projectileType = weapon.getProjectileType(); projectileType != ProjectileType::none()) {
				secondaryShootTimer.addTimeLeft(weapon.getShootInterval());
				this->createShotgunSpread(it->second->position,
										  it->second->aimDirection,
										  projectileType,
										  it->second->team,
										  it->first,
										  weapon,
										  weapon.getDamage(),
										  weapon.getHurtSound(),
										  projectileType.getDisappearTime(),
										  projectileType.getMoveInterval());
			}
			break;
		case Weapon::stickybomb_launcher(): {
			this->detonatePlayerStickiesUntil(it, 7);
			if (!it->second || !it->second->alive) {
				return;
			}
			if (const auto projectileType = weapon.getProjectileType(); projectileType != ProjectileType::none()) {
				if (otherWeapon != Weapon::sticky_detonator()) {
					secondaryShootTimer.addTimeLeft(weapon.getShootInterval());
				}
				const auto aimVector = it->second->aimDirection.getVector();
				const auto moveVector = it->second->moveDirection.getVector();

				// Increase/decrease speed by 40% depending on player movement.
				const auto aimVectorNormalized = (aimVector == Vec2{}) ? Vector2<float>{} : static_cast<Vector2<float>>(aimVector).normalized();
				const auto moveVectorNormalized = (moveVector == Vec2{}) ? Vector2<float>{} : static_cast<Vector2<float>>(moveVector).normalized();
				const auto moveIntervalCoefficient = 1.0f - 0.4f * Vector2<float>::dotProduct(aimVectorNormalized, moveVectorNormalized);
				this->createProjectile(it->second->position + aimVector,
									   it->second->aimDirection,
									   projectileType,
									   it->second->team,
									   it->first,
									   weapon,
									   weapon.getDamage(),
									   weapon.getHurtSound(),
									   projectileType.getDisappearTime(),
									   projectileType.getMoveInterval() * moveIntervalCoefficient);
			}
			break;
		}
		case Weapon::syringe_gun(): [[fallthrough]];
		case Weapon::medi_gun():
			if (const auto projectileType = weapon.getProjectileType(); projectileType != ProjectileType::none()) {
				secondaryShootTimer.addTimeLeft(weapon.getShootInterval());
				this->createProjectile(it->second->position + it->second->aimDirection.getVector(),
									   it->second->aimDirection,
									   projectileType,
									   it->second->team,
									   it->first,
									   weapon,
									   weapon.getDamage(),
									   weapon.getHurtSound(),
									   projectileType.getDisappearTime(),
									   projectileType.getMoveInterval());
			}
			break;
		case Weapon::sniper_rifle():
			if (const auto projectileType = weapon.getProjectileType(); projectileType != ProjectileType::none()) {
				secondaryShootTimer.addTimeLeft(weapon.getShootInterval());
				this->createSniperRifleTrail(it->second->position + it->second->aimDirection.getVector(),
											 it->second->aimDirection,
											 projectileType,
											 it->second->team,
											 it->first,
											 weapon,
											 weapon.getDamage(),
											 weapon.getHurtSound(),
											 projectileType.getDisappearTime(),
											 projectileType.getMoveInterval());
			}
			break;
		case Weapon::build_tool():
			secondaryShootTimer.addTimeLeft(mp_sentry_build_time);
			for (auto itSentryGun = m_sentryGuns.stable_begin(); itSentryGun != m_sentryGuns.stable_end(); ++itSentryGun) {
				if (itSentryGun->second->alive && itSentryGun->second->owner == it->first) {
					this->killSentryGun(itSentryGun, m_players.stable_end());
				}
			}
			if (!it->second || !it->second->alive) {
				return;
			}
			this->createSentryGun(it->second->position, it->second->team, static_cast<Health>(mp_sentry_health), it->first);
			break;
		case Weapon::disguise_kit():
			secondaryShootTimer.addTimeLeft(weapon.getShootInterval());
			if (it->second->disguised) {
				it->second->disguised = false;
			} else if (!this->isPlayerCarryingFlag(it->first)) {
				it->second->disguised = true;
			}
			break;
		case Weapon::sticky_detonator(): this->detonatePlayerStickiesUntil(it, 0); break;
		case Weapon::knife(): {
			const auto knifePosition = it->second->position + it->second->aimDirection.getVector();
			if (const auto itPlayer = this->findKnifeTargetPlayer(knifePosition, it->second->team); itPlayer != m_players.stable_end()) {
				secondaryShootTimer.setTimeLeft(mp_spy_kill_disguise_cooldown);
				this->applyDamageToPlayer(itPlayer, weapon.getDamage(), weapon.getHurtSound(), false, it, weapon);
			}

			if (const auto itSentryGun = this->findKnifeTargetSentryGun(knifePosition, it->second->team); itSentryGun != m_sentryGuns.stable_end()) {
				secondaryShootTimer.setTimeLeft(mp_spy_kill_disguise_cooldown);
				this->applyDamageToSentryGun(itSentryGun, weapon.getDamage(), weapon.getHurtSound(), false, it);
			}
			break;
		}
		default:
			if (const auto projectileType = weapon.getProjectileType(); projectileType != ProjectileType::none()) {
				secondaryShootTimer.addTimeLeft(weapon.getShootInterval());
				this->createProjectile(it->second->position + it->second->aimDirection.getVector(),
									   it->second->aimDirection,
									   projectileType,
									   it->second->team,
									   it->first,
									   weapon,
									   weapon.getDamage(),
									   weapon.getHurtSound(),
									   projectileType.getDisappearTime(),
									   projectileType.getMoveInterval());
			}
			break;
	}
}

auto World::stepPlayer(PlayerIterator it, Direction direction) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	const auto destination = this->getClippedMovementDestination(it->second->position,
																 it->second->team == Team::red(),
																 it->second->team == Team::blue(),
																 it->second->noclip,
																 direction);
	if (it->second->position != destination) {
		it->second->position = destination;
		this->checkCollisions(it);
	}
}

auto World::stepProjectile(ProjectileIterator it, Direction direction) -> void {
	assert(it != m_projectiles.stable_end());
	assert(it->second);
	const auto destination = it->second->position + direction.getVector();
	if (it->second->position != destination) {
		it->second->position = destination;
		this->checkCollisions(it);
	}
}

auto World::stepGenericEntity(GenericEntityIterator it, int steps) -> void {
	assert(it != m_genericEntities.stable_end());
	assert(it->second);
	if (it->second->velocity == Vec2{}) {
		return;
	}

	auto position = it->second->position;
	const auto velocity = it->second->velocity;
	const auto destination = position + velocity;
	const auto red = (it->second->solidFlags & Solid::RED_ENVIRONMENT) == 0;
	const auto blue = (it->second->solidFlags & Solid::BLUE_ENVIRONMENT) == 0;
	const auto noclip = (it->second->solidFlags & Solid::WORLD) == 0;
	const auto moveDirection = Direction{velocity};
	const auto dx = std::abs(velocity.x);
	const auto dy = std::abs(velocity.y);
	const auto sx = (velocity.x < 0) ? Vec2::Length{-1} : Vec2::Length{1};
	const auto sy = (velocity.y < 0) ? Vec2::Length{-1} : Vec2::Length{1};
	auto err = ((dx > dy) ? dx : -dy) / 2;
	while (position != destination) {
		if (steps >= sv_max_move_steps_per_frame) {
			return;
		}
		++steps;
		const auto previousPosition = it->second->position;
		const auto error = err;
		if (error > -dx) {
			err -= dy;
			position.x = static_cast<Vec2::Length>(position.x + sx);
		}
		if (error < dy) {
			err += dx;
			position.y = static_cast<Vec2::Length>(position.y + sy);
		}
		it->second->position = position;
		if (!this->canMove(red, blue, noclip, position, moveDirection)) {
			const auto canMoveHorizontal = this->canMove(red, blue, noclip, Vec2{position.x, previousPosition.y}, moveDirection);
			const auto canMoveVertical = this->canMove(red, blue, noclip, Vec2{previousPosition.x, position.y}, moveDirection);
			auto normal = -it->second->velocity;
			if (canMoveHorizontal && !canMoveVertical) {
				normal.x = 0;
			} else if (canMoveVertical && !canMoveHorizontal) {
				normal.y = 0;
			}
			m_server.callIfDefined(Script::command(
				{"on_collide_ent_world", cmd::formatGenericEntityId(it->first), util::toString(normal.x), util::toString(normal.y)}));
			if (!it->second) {
				return;
			}
			if (it->second->position != position) {
				return;
			}
			it->second->position = previousPosition;
			if (it->second->velocity != velocity) {
				this->stepGenericEntity(it, steps);
			}
			return;
		}
		this->checkCollisions(it);
		if (!it->second) {
			return;
		}
		if (it->second->position != position) {
			return;
		}
		if (it->second->velocity != velocity) {
			it->second->position = previousPosition;
			this->stepGenericEntity(it, steps);
			return;
		}
		m_server.callIfDefined(
			Script::command({"on_ent_step", cmd::formatGenericEntityId(it->first), util::toString(position.x), util::toString(position.y)}));
	}
}

auto World::teleportPlayerToSpawn(PlayerIterator it) -> bool {
	assert(it != m_players.stable_end());
	assert(it->second);
	if (const auto itTeamSpawn = m_teamSpawns.find(it->second->team); itTeamSpawn != m_teamSpawns.end() && !itTeamSpawn->second.spawnPoints.empty()) {
		const auto& spawnPoints = itTeamSpawn->second.spawnPoints;
		return this->teleportPlayer(it, spawnPoints[itTeamSpawn->second.spawns % spawnPoints.size()]);
	}
	return false;
}

auto World::playerTeamSelect(PlayerIterator it, Team team, PlayerClass playerClass) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	auto switchedTeam = false;
	if (it->second->team != team) {
		switchedTeam = true;
		m_server.writePlayerTeamSelected(it->second->team, team, it->first);
		if (team == Team::spectators() || playerClass == PlayerClass::spectator()) {
			it->second->team = Team::spectators();
		} else if (team != Team::none()) {
			it->second->team = team;
			if (it->second->team != Team::spectators() && mp_limitteams != 0) {
				const auto playerCounts = this->getTeamPlayerCounts();
				const auto itCount = std::min_element(playerCounts.begin(), playerCounts.end(), [](const auto& lhs, const auto& rhs) {
					return lhs.second < rhs.second;
				});
				if (itCount != playerCounts.end() &&
					static_cast<std::size_t>(playerCounts.at(it->second->team) - itCount->second) > static_cast<std::size_t>(mp_limitteams)) {
					it->second->team = itCount->first;
					m_server.writeServerChatMessage(
						fmt::format("{} was moved to team {} for game balance.", it->second->name, it->second->team.getName()));
				}
			}

			if (it->second->team == team) {
				m_server.writeServerChatMessage(fmt::format("{} joined team {}.", it->second->name, it->second->team.getName()));
			}
		}
	}

	if (it->second->team == Team::spectators()) {
		it->second->playerClass = PlayerClass::spectator();
	} else {
		const auto desiredClass = playerClass;
		const auto& playerClasses = PlayerClass::getAll();

		auto i = std::size_t{0};
		while (i < playerClasses.size() && (playerClass == PlayerClass::none() || playerClass == PlayerClass::spectator() ||
											this->getPlayerClassCount(it->second->team, playerClass) >= playerClass.getLimit())) {
			const auto newPlayerClass = playerClasses[i];
			if (newPlayerClass != PlayerClass::none() && newPlayerClass != PlayerClass::spectator() && newPlayerClass != playerClass) {
				m_server.writeServerChatMessage(fmt::format("{} switched class to {}. ({} is full at {} players.)",
															it->second->name,
															newPlayerClass.getName(),
															playerClass.getName(),
															playerClass.getLimit()),
												it->second->team);
				playerClass = newPlayerClass;
			}
			++i;
		}

		if (playerClass != it->second->playerClass) {
			m_server.writePlayerClassSelected(it->second->playerClass, playerClass, it->first);
			it->second->playerClass = playerClass;
			if (playerClass == desiredClass) {
				m_server.writeServerChatMessage(fmt::format("{} switched class to {}.", it->second->name, it->second->playerClass.getName()),
												it->second->team);
			}
		}
	}

	this->cleanupSentryGuns(it->first);
	this->cleanupProjectiles(it->first);
	if (!it->second) {
		return;
	}

	m_server.callIfDefined(Script::command({"on_team_select", cmd::formatPlayerId(it->first)}));
	if (!it->second) {
		return;
	}

	if (it->second->alive && !switchedTeam &&
		this->containsSpawnPoint(Rect{static_cast<Rect::Length>(it->second->position.x - 2), static_cast<Rect::Length>(it->second->position.y - 2), 5, 5},
								 it->second->team)) {
		this->spawnPlayer(it);
	} else {
		this->killPlayer(it, true, it);
		if (switchedTeam && it->second) {
			this->teleportPlayerToSpawn(it);
		}
	}
}

auto World::spawnPlayer(PlayerIterator it) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	if (const auto itTeamSpawn = m_teamSpawns.find(it->second->team); itTeamSpawn != m_teamSpawns.end() && !itTeamSpawn->second.spawnPoints.empty()) {
		const auto& spawnPoints = itTeamSpawn->second.spawnPoints;
		this->teleportPlayer(it, spawnPoints[itTeamSpawn->second.spawns++ % spawnPoints.size()]);
		if (!it->second) {
			return;
		}
	}
	it->second->alive = true;
	it->second->respawnCountdown.reset();
	it->second->respawning = false;
	it->second->moveTimer.reset();
	it->second->attack1Timer.reset();
	it->second->attack2Timer.reset();
	it->second->primaryReloadTimer.reset();
	it->second->secondaryReloadTimer.reset();
	it->second->health = it->second->playerClass.getHealth();
	it->second->primaryAmmo = it->second->playerClass.getPrimaryWeapon().getAmmoPerClip();
	it->second->secondaryAmmo = it->second->playerClass.getSecondaryWeapon().getAmmoPerClip();
	it->second->blastJumpDirection = Direction{};
	it->second->blastJumpTimer.reset();
	it->second->blastJumpCountdown.reset();
	it->second->blastJumping = false;
	it->second->blastJumpInterval = 0.0f;
	m_server.playWorldSound(SoundId::player_spawn(), it->second->position, it->first);
	this->removePlayerStickies(it);
	m_server.callIfDefined(Script::command({"on_player_spawn", cmd::formatPlayerId(it->first)}));
}

auto World::resupplyPlayer(PlayerIterator it) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	const auto classHealth = it->second->playerClass.getHealth();
	const auto primaryMaxAmmo = it->second->playerClass.getPrimaryWeapon().getAmmoPerClip();
	const auto secondaryMaxAmmo = it->second->playerClass.getSecondaryWeapon().getAmmoPerClip();
	if (it->second->health < classHealth || it->second->primaryAmmo < primaryMaxAmmo || it->second->secondaryAmmo < secondaryMaxAmmo) {
		it->second->health = classHealth;
		it->second->primaryAmmo = primaryMaxAmmo;
		it->second->secondaryAmmo = secondaryMaxAmmo;
		m_server.playWorldSound(SoundId::resupply(), it->second->position, it->first);
		m_server.callIfDefined(Script::command({"on_resupply", cmd::formatPlayerId(it->first)}));
	}
}

auto World::removePlayerStickies(PlayerIterator it) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	it->second->nStickies = 0;
	for (auto itProjectile = m_projectiles.stable_begin(); itProjectile != m_projectiles.stable_end();) {
		if (itProjectile->second->type == ProjectileType::sticky() && itProjectile->second->owner == it->first) {
			itProjectile = m_projectiles.stable_erase(itProjectile);
		} else {
			++itProjectile;
		}
	}
}

auto World::detonatePlayerStickiesUntil(PlayerIterator it, int maxRemaining) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	for (auto itProjectile = m_projectiles.stable_begin(); itProjectile != m_projectiles.stable_end() && it->second->nStickies > maxRemaining;) {
		if (itProjectile->second->type == ProjectileType::sticky() && itProjectile->second->stickyAttached &&
			itProjectile->second->disappearTimer.getTimeLeft() <= 0.0f && itProjectile->second->owner == it->first) {
			m_server.playWorldSound(SoundId::explosion(), itProjectile->second->position);
			this->createExplosion(itProjectile->second->position,
								  itProjectile->second->team,
								  itProjectile->second->owner,
								  itProjectile->second->weapon,
								  itProjectile->second->damage,
								  itProjectile->second->hurtSound,
								  mp_explosion_disappear_time);
			if (!it->second || !it->second->alive) {
				return;
			}
			--it->second->nStickies;
			if (itProjectile->second) {
				itProjectile = m_projectiles.stable_erase(itProjectile);
			} else {
				++itProjectile;
			}
		} else {
			++itProjectile;
		}
	}
}

auto World::setPlayerNoclip(PlayerIterator it, bool value) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	const auto oldValue = it->second->noclip;
	it->second->noclip = value;
	if (!it->second->noclip && oldValue) {
		this->checkCollisions(it);
	}
}

auto World::setPlayerName(PlayerIterator it, std::string name) -> void {
	assert(it != m_players.stable_end());
	assert(it->second);
	m_server.writeServerChatMessage(fmt::format("{} changed name to \"{}\".", it->second->name, name));
	it->second->name = std::move(name);
}

auto World::equipPlayerHat(PlayerIterator it, Hat hat) -> void { // NOLINT(readability-convert-member-functions-to-static)
	assert(it != m_players.stable_end());
	assert(it->second);
	it->second->hat = hat;
}

auto World::spawnMedkit(MedkitIterator it) -> void {
	assert(it != m_medkits.stable_end());
	assert(it->second);
	it->second->respawnCountdown.reset();
	it->second->alive = true;
	m_server.playWorldSound(SoundId::medkit_spawn(), it->second->position);
	m_server.callIfDefined(Script::command({"on_medkit_spawn", cmd::formatMedkitId(it->first)}));
	if (it->second && it->second->alive) {
		this->checkCollisions(it);
	}
}

auto World::spawnAmmopack(AmmopackIterator it) -> void {
	assert(it != m_ammopacks.stable_end());
	assert(it->second);
	it->second->respawnCountdown.reset();
	it->second->alive = true;
	m_server.playWorldSound(SoundId::medkit_spawn(), it->second->position);
	m_server.callIfDefined(Script::command({"on_ammopack_spawn", cmd::formatAmmopackId(it->first)}));
	if (it->second && it->second->alive) {
		this->checkCollisions(it);
	}
}

auto World::pickupFlag(FlagIterator it, PlayerIterator carrier) -> void {
	assert(it != m_flags.stable_end());
	assert(it->second);
	assert(carrier != m_players.stable_end());
	assert(carrier->second);
	it->second->carrier = carrier->first;
	it->second->returnCountdown.reset();
	it->second->returning = false;
	carrier->second->disguised = false;
	m_server.playTeamSound(SoundId::we_picked_intel(), SoundId::they_picked_intel(), carrier->second->team);
	m_server.writeServerChatMessage(fmt::format("{} picked up the {}!", carrier->second->name, it->second->name));
	m_server.callIfDefined(Script::command({"on_pickup_flag", cmd::formatFlagId(it->first), cmd::formatPlayerId(carrier->first)}));
}

auto World::dropFlag(FlagIterator it, PlayerIterator carrier) -> void {
	assert(it != m_flags.stable_end());
	assert(it->second);
	assert(carrier != m_players.stable_end());
	assert(carrier->second);
	it->second->carrier = PlayerRegistry::INVALID_KEY;
	it->second->returnCountdown.start(mp_flag_return_time);
	it->second->returning = true;
	it->second->position = carrier->second->position;
	this->checkCollisions(it);
	if (!it->second || !carrier->second) {
		return;
	}
	m_server.playTeamSound(SoundId::we_dropped_intel(), SoundId::they_dropped_intel(), carrier->second->team);
	m_server.writeServerChatMessage(fmt::format("{} dropped the {}!", carrier->second->name, it->second->name));
	m_server.callIfDefined(Script::command({"on_drop_flag", cmd::formatFlagId(it->first), cmd::formatPlayerId(carrier->first)}));
}

auto World::returnFlag(FlagIterator it, bool announce) -> void {
	assert(it != m_flags.stable_end());
	assert(it->second);
	it->second->carrier = PlayerRegistry::INVALID_KEY;
	it->second->returnCountdown.reset();
	it->second->returning = false;
	it->second->position = it->second->spawnPosition;
	this->checkCollisions(it);
	if (!it->second) {
		return;
	}

	if (announce) {
		m_server.playTeamSound(SoundId::we_returned_intel(), SoundId::they_returned_intel(), it->second->team);
		m_server.writeServerChatMessage(fmt::format("{} has returned!", it->second->name));
		m_server.callIfDefined(Script::command({"on_return_flag", cmd::formatFlagId(it->first)}));
	}
}

auto World::captureFlag(FlagIterator it, PlayerIterator carrier) -> void {
	assert(it != m_flags.stable_end());
	assert(it->second);
	assert(carrier != m_players.stable_end());
	assert(carrier->second);
	this->returnFlag(it, false);
	if (!it->second || !carrier->second) {
		return;
	}

	if (const auto itFlag = util::findIf(m_flags.stable(), [&](const auto& kv) { return kv.second->team == carrier->second->team; });
		itFlag != m_flags.stable_end()) {
		const auto points = static_cast<Score>(mp_score_objective);
		carrier->second->score += points;
		++itFlag->second->score;
		m_server.callIfDefined(Script::command({"on_capture_flag", cmd::formatFlagId(it->first), cmd::formatPlayerId(carrier->first)}));
		if (!itFlag->second || !it->second || !carrier->second) {
			return;
		}

		if (itFlag->second->score >= mp_ctf_capture_limit) {
			m_server.writeServerChatMessage(fmt::format("{} captured the {}!", carrier->second->name, it->second->name));
			this->win(carrier->second->team);
		} else {
			m_server.playTeamSound(SoundId::we_captured_intel(), SoundId::they_captured_intel(), carrier->second->team);
			m_server.writeServerChatMessage(fmt::format("{} captured the {}!", carrier->second->name, it->second->name));
		}

		if (carrier->second) {
			m_server.awardPlayerPoints(carrier->first, carrier->second->name, points);
		}
	}
}

auto World::createShotgunSpread(Vec2 position, Direction direction, ProjectileType type, Team team, PlayerId owner, Weapon weapon,
								Health damage, SoundId hurtSound, float disappearTime, float moveInterval) -> void {
	const auto bulletPositions = [position, direction]() -> std::pair<Vec2, Vec2> {
		if (mp_shotgun_use_legacy_spread) {
			if (direction.isRight()) {
				if (direction.isUp()) {
					return {Vec2{position.x + 1, position.y + 1}, Vec2{position.x - 1, position.y - 1}};
				}
				if (direction.isDown()) {
					return {Vec2{position.x + 1, position.y - 1}, Vec2{position.x - 1, position.y + 1}};
				}
				return {Vec2{position.x, position.y + 1}, Vec2{position.x, position.y - 1}};
			}
			if (direction.isLeft()) {
				if (direction.isUp()) {
					return {Vec2{position.x + 1, position.y - 1}, Vec2{position.x - 1, position.y + 1}};
				}
				if (direction.isDown()) {
					return {Vec2{position.x - 1, position.y - 1}, Vec2{position.x + 1, position.y + 1}};
				}
				return {Vec2{position.x, position.y + 1}, Vec2{position.x, position.y - 1}};
			}
			return {Vec2{position.x - 1, position.y}, Vec2{position.x + 1, position.y}};
		}

		if (direction.isRight()) {
			if (direction.isUp()) {
				return {Vec2{position.x + 1, position.y}, Vec2{position.x, position.y - 1}};
			}
			if (direction.isDown()) {
				return {Vec2{position.x + 1, position.y}, Vec2{position.x, position.y + 1}};
			}
			return {Vec2{position.x, position.y + 1}, Vec2{position.x, position.y - 1}};
		}
		if (direction.isLeft()) {
			if (direction.isUp()) {
				return {Vec2{position.x, position.y - 1}, Vec2{position.x - 1, position.y}};
			}
			if (direction.isDown()) {
				return {Vec2{position.x - 1, position.y}, Vec2{position.x, position.y + 1}};
			}
			return {Vec2{position.x, position.y + 1}, Vec2{position.x, position.y - 1}};
		}
		return {Vec2{position.x - 1, position.y}, Vec2{position.x + 1, position.y}};
	}();
	this->createProjectile(position, direction, type, team, owner, weapon, damage, hurtSound, disappearTime, moveInterval);
	this->createProjectile(bulletPositions.first, direction, type, team, owner, weapon, damage, hurtSound, disappearTime, moveInterval);
	this->createProjectile(bulletPositions.second, direction, type, team, owner, weapon, damage, hurtSound, disappearTime, moveInterval);
}

auto World::createSniperRifleTrail(Vec2 position, Direction direction, ProjectileType type, Team team, PlayerId owner, Weapon weapon,
								   Health damage, SoundId hurtSound, float disappearTime, float moveInterval) -> void {
	if (direction.isAny()) {
		const auto aim = direction.getVector();
		const auto red = team == Team::red();
		const auto blue = team == Team::blue();
		for (auto i = 0; i < mp_sniper_rifle_range; ++i) {
			if (m_map.isSolid(position, red, blue)) {
				break;
			}
			this->createProjectile(position, Direction::none(), type, team, owner, weapon, damage, hurtSound, disappearTime, moveInterval);
			position += aim;
		}
	} else if (mp_sniper_rifle_range > 0) {
		this->createProjectile(position, Direction::none(), type, team, owner, weapon, damage, hurtSound, disappearTime, moveInterval);
	}
}

auto World::cleanupSentryGuns(PlayerId id) -> void {
	for (auto itSentryGun = m_sentryGuns.stable_begin(); itSentryGun != m_sentryGuns.stable_end();) {
		if (itSentryGun->second->owner == id) {
			itSentryGun = m_sentryGuns.stable_erase(itSentryGun);
		} else {
			++itSentryGun;
		}
	}
}

auto World::cleanupProjectiles(PlayerId id) -> void {
	if (const auto itPlayer = m_players.stable_find(id); itPlayer != m_players.stable_end()) {
		itPlayer->second->nStickies = 0;
	}

	for (auto itProjectile = m_projectiles.stable_begin(); itProjectile != m_projectiles.stable_end();) {
		if (itProjectile->second->owner == id) {
			itProjectile = m_projectiles.stable_erase(itProjectile);
		} else {
			++itProjectile;
		}
	}
}

auto World::canTeleport(bool red, bool blue, bool noclip, Vec2 destination) const -> bool {
	return (noclip && destination.x >= 0 && destination.x < m_map.getWidth() && destination.y >= 0 && destination.y < m_map.getHeight()) ||
		   !m_map.isSolid(destination, red, blue);
}

auto World::canMove(bool red, bool blue, bool noclip, Vec2 destination, Direction moveDirection) const -> bool {
	return (noclip && destination.x >= 0 && destination.x < m_map.getWidth() && destination.y >= 0 && destination.y < m_map.getHeight()) ||
		   !m_map.isSolid(destination, red, blue, moveDirection);
}

auto World::canMove(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> bool {
	const auto moveVector = moveDirection.getVector();
	if (moveVector == Vec2{}) {
		return true;
	}
	const auto destination = position + moveVector;
	return this->canMove(red, blue, noclip, destination, moveDirection) ||
		   this->canMove(red, blue, noclip, {destination.x, position.y}, moveDirection.getHorizontal()) ||
		   this->canMove(red, blue, noclip, {position.x, destination.y}, moveDirection.getVertical());
}

auto World::getClippedMovementDestination(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> Vec2 {
	const auto moveVector = moveDirection.getVector();
	if (moveVector == Vec2{}) {
		return position;
	}

	const auto destination = position + moveVector;
	if (this->canMove(red, blue, noclip, destination, moveDirection)) {
		return destination;
	}

	const auto horizontal = moveDirection.getHorizontal();
	const auto vertical = moveDirection.getVertical();

	const auto horizontalDestination = Vec2{destination.x, position.y};
	const auto verticalDestination = Vec2{position.x, destination.y};

	const auto xIsBlocked = !this->canMove(red, blue, noclip, horizontalDestination, horizontal);
	const auto yIsBlocked = !this->canMove(red, blue, noclip, verticalDestination, vertical);
	if (xIsBlocked && !yIsBlocked) {
		return verticalDestination;
	}
	if (yIsBlocked && !xIsBlocked) {
		return horizontalDestination;
	}
	return position;
}

auto World::getClippedMovementOffset(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> Vec2 {
	return this->getClippedMovementDestination(position, red, blue, noclip, moveDirection) - position;
}

auto World::getClippedMovementDirection(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> Direction {
	const auto offset = this->getClippedMovementOffset(position, red, blue, noclip, moveDirection);
	return Direction{offset.x<0, offset.x> 0, offset.y<0, offset.y> 0};
}

auto World::getPlayersPushingCart(PayloadCartIterator it) -> std::vector<World::PlayerIterator> {
	assert(it != m_carts.stable_end());
	assert(it->second);

	const auto position = it->second->track[it->second->currentTrackIndex];
	const auto rect = Rect{position.x - 2, position.y - 2, 5, 5};

	auto pushingPlayers = std::vector<PlayerIterator>{};
	for (auto itPlayer = m_players.stable_begin(); itPlayer != m_players.stable_end(); ++itPlayer) {
		if (itPlayer->second->alive && rect.contains(itPlayer->second->position)) {
			if (itPlayer->second->team == it->second->team) {
				if (!itPlayer->second->disguised) {
					pushingPlayers.push_back(itPlayer);
				}
			} else {
				pushingPlayers.clear();
				break;
			}
		}
	}
	return pushingPlayers;
}

auto World::isKnifeTarget(Vec2 position, Team team) const -> bool {
	if (const auto it = m_collisionMap.find(position); it != m_collisionMap.end()) {
		for (const auto& itEntity : it->second) {
			if (util::match(itEntity)(
					[&](PlayerIterator itPlayer) -> bool { return this->isCollideable(itPlayer) && itPlayer->second->team != team; },
					[&](SentryGunIterator itSentryGun) -> bool {
						return this->isCollideable(itSentryGun) && itSentryGun->second->team != team;
					},
					[](auto) -> bool { return false; })) {
				return true;
			}
		}
	}
	return false;
}

auto World::findKnifeTargetPlayer(Vec2 position, Team team) -> World::PlayerIterator {
	if (const auto it = m_collisionMap.find(position); it != m_collisionMap.end()) {
		for (const auto& itEntity : it->second) {
			if (const auto* const ptr = std::get_if<PlayerIterator>(&itEntity)) {
				const auto itPlayer = *ptr;
				if (this->isCollideable(itPlayer) && itPlayer->second->team != team) {
					return itPlayer;
				}
			}
		}
	}
	return m_players.stable_end();
}

auto World::findKnifeTargetSentryGun(Vec2 position, Team team) -> World::SentryGunIterator {
	if (const auto it = m_collisionMap.find(position); it != m_collisionMap.end()) {
		for (const auto& itEntity : it->second) {
			if (const auto* const ptr = std::get_if<SentryGunIterator>(&itEntity)) {
				const auto itSentryGun = *ptr;
				if (this->isCollideable(itSentryGun) && itSentryGun->second->team != team) {
					return itSentryGun;
				}
			}
		}
	}
	return m_sentryGuns.stable_end();
}
