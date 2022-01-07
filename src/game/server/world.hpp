#ifndef AF2_SERVER_WORLD_HPP
#define AF2_SERVER_WORLD_HPP

#include "../../utilities/countdown.hpp" // util::Countdown, util::CountdownLoop
#include "../../utilities/registry.hpp"  // util::Registry
#include "../data/ammo.hpp"              // Ammo
#include "../data/hat.hpp"               // Hat
#include "../data/health.hpp"            // Health
#include "../data/player_class.hpp"      // PlayerClass
#include "../data/player_id.hpp"         // PlayerId
#include "../data/projectile_type.hpp"   // ProjectileType
#include "../data/rectangle.hpp"         // Rect
#include "../data/score.hpp"             // Score
#include "../data/sound_id.hpp"          // SoundId
#include "../data/team.hpp"              // Team
#include "../data/tick_count.hpp"        // TickCount
#include "../data/vector.hpp"            // Vec2
#include "../data/weapon.hpp"            // Weapon
#include "../shared/snapshot.hpp"        // Snapshot
#include "entities.hpp"                  // ent::sv::...

#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint32_t
#include <memory>        // std::shared_ptr
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <variant>       // std::variant, std::get_if
#include <vector>        // std::vector

class Map;
class GameServer;
namespace util {
template <typename Time>
class CountdownLoop;
} // namespace util

class World final {
public:
	using PlayerId = ::PlayerId;
	using ProjectileId = std::uint32_t;
	using ExplosionId = std::uint32_t;
	using SentryGunId = std::uint32_t;
	using MedkitId = std::uint32_t;
	using AmmopackId = std::uint32_t;
	using GenericEntityId = std::uint32_t;
	using FlagId = std::uint32_t;
	using PayloadCartId = std::uint32_t;

	World(const Map& map, GameServer& server);

	auto reset() -> void;
	auto startMap() -> void;
	auto resetRound() -> void;
	auto win(Team team) -> void;
	auto stalemate() -> void;
	auto startRound(float delay = 0.0f) -> void;

	auto update(float deltaTime) -> void;

	[[nodiscard]] auto getTickCount() const -> TickCount;
	[[nodiscard]] auto getMapTime() const -> float;
	[[nodiscard]] auto getRoundsPlayed() const -> int;
	[[nodiscard]] auto takeSnapshot(PlayerId id) const -> Snapshot;

	auto createPlayer(Vec2 position, std::string name) -> PlayerId;
	auto createProjectile(Vec2 position, Direction moveDirection, ProjectileType type, Team team, PlayerId owner, Weapon weapon,
						  Health damage, SoundId hurtSound, float disappearTime, float moveInterval) -> ProjectileId;
	auto createExplosion(Vec2 position, Team team, PlayerId owner, Weapon weapon, Health damage, SoundId hurtSound, float disappearTime) -> ExplosionId;
	auto createSentryGun(Vec2 position, Team team, Health health, PlayerId owner) -> SentryGunId;
	auto createMedkit(Vec2 position) -> MedkitId;
	auto createAmmopack(Vec2 position) -> AmmopackId;
	auto createGenericEntity(Vec2 position) -> GenericEntityId;
	auto createFlag(Vec2 position, Team team, std::string name) -> FlagId;
	auto createPayloadCart(Team team, std::vector<Vec2> track) -> PayloadCartId;

	auto spawnPlayer(PlayerId id) -> bool;
	auto spawnMedkit(MedkitId id) -> bool;
	auto spawnAmmopack(AmmopackId id) -> bool;

	auto applyDamageToPlayer(PlayerId id, Health damage, SoundId hurtSound, bool allowOverheal, PlayerId inflictor,
							 Weapon weapon = Weapon::none()) -> bool;
	auto applyDamageToSentryGun(SentryGunId id, Health damage, SoundId hurtSound, bool allowOverheal, PlayerId inflictor) -> bool;

	auto killPlayer(PlayerId id, bool announce, PlayerId killer, Weapon weapon = Weapon::none()) -> bool;
	auto killSentryGun(SentryGunId id, PlayerId killer) -> bool;
	auto killMedkit(MedkitId id, float respawnTime) -> bool;
	auto killAmmopack(AmmopackId id, float respawnTime) -> bool;

	auto deletePlayer(PlayerId id) -> bool;
	auto deleteProjectile(ProjectileId id) -> bool;
	auto deleteExplosion(ExplosionId id) -> bool;
	auto deleteSentryGun(SentryGunId id) -> bool;
	auto deleteMedkit(MedkitId id) -> bool;
	auto deleteAmmopack(AmmopackId id) -> bool;
	auto deleteGenericEntity(GenericEntityId id) -> bool;
	auto deleteFlag(FlagId id) -> bool;
	auto deletePayloadCart(PayloadCartId id) -> bool;

	[[nodiscard]] auto hasPlayerId(PlayerId id) const -> bool;
	[[nodiscard]] auto hasProjectileId(ProjectileId id) const -> bool;
	[[nodiscard]] auto hasExplosionId(ExplosionId id) const -> bool;
	[[nodiscard]] auto hasSentryGunId(SentryGunId id) const -> bool;
	[[nodiscard]] auto hasMedkitId(MedkitId id) const -> bool;
	[[nodiscard]] auto hasAmmopackId(AmmopackId id) const -> bool;
	[[nodiscard]] auto hasGenericEntityId(GenericEntityId id) const -> bool;
	[[nodiscard]] auto hasFlagId(FlagId id) const -> bool;
	[[nodiscard]] auto hasPayloadCartId(PayloadCartId id) const -> bool;

	[[nodiscard]] auto getPlayerCount() const -> std::size_t;
	[[nodiscard]] auto getProjectileCount() const -> std::size_t;
	[[nodiscard]] auto getExplosionCount() const -> std::size_t;
	[[nodiscard]] auto getSentryGunCount() const -> std::size_t;
	[[nodiscard]] auto getMedkitCount() const -> std::size_t;
	[[nodiscard]] auto getAmmopackCount() const -> std::size_t;
	[[nodiscard]] auto getGenericEntityCount() const -> std::size_t;
	[[nodiscard]] auto getFlagCount() const -> std::size_t;
	[[nodiscard]] auto getPayloadCartCount() const -> std::size_t;

	[[nodiscard]] auto getAllPlayerIds() const -> std::vector<PlayerId>;
	[[nodiscard]] auto getAllProjectileIds() const -> std::vector<ProjectileId>;
	[[nodiscard]] auto getAllExplosionIds() const -> std::vector<ExplosionId>;
	[[nodiscard]] auto getAllSentryGunIds() const -> std::vector<SentryGunId>;
	[[nodiscard]] auto getAllMedkitIds() const -> std::vector<MedkitId>;
	[[nodiscard]] auto getAllAmmopackIds() const -> std::vector<AmmopackId>;
	[[nodiscard]] auto getAllGenericEntityIds() const -> std::vector<GenericEntityId>;
	[[nodiscard]] auto getAllFlagIds() const -> std::vector<FlagId>;
	[[nodiscard]] auto getAllPayloadCartIds() const -> std::vector<PayloadCartId>;

	[[nodiscard]] auto findPlayer(PlayerId id) -> ent::sv::PlayerHandle;
	[[nodiscard]] auto findPlayer(PlayerId id) const -> ent::sv::ConstPlayerHandle;
	[[nodiscard]] auto findProjectile(ProjectileId id) -> ent::sv::ProjectileHandle;
	[[nodiscard]] auto findProjectile(ProjectileId id) const -> ent::sv::ConstProjectileHandle;
	[[nodiscard]] auto findExplosion(ExplosionId id) -> ent::sv::ExplosionHandle;
	[[nodiscard]] auto findExplosion(ExplosionId id) const -> ent::sv::ConstExplosionHandle;
	[[nodiscard]] auto findSentryGun(SentryGunId id) -> ent::sv::SentryGunHandle;
	[[nodiscard]] auto findSentryGun(SentryGunId id) const -> ent::sv::ConstSentryGunHandle;
	[[nodiscard]] auto findMedkit(MedkitId id) -> ent::sv::MedkitHandle;
	[[nodiscard]] auto findMedkit(MedkitId id) const -> ent::sv::ConstMedkitHandle;
	[[nodiscard]] auto findAmmopack(AmmopackId id) -> ent::sv::AmmopackHandle;
	[[nodiscard]] auto findAmmopack(AmmopackId id) const -> ent::sv::ConstAmmopackHandle;
	[[nodiscard]] auto findGenericEntity(GenericEntityId id) -> ent::sv::GenericEntityHandle;
	[[nodiscard]] auto findGenericEntity(GenericEntityId id) const -> ent::sv::ConstGenericEntityHandle;
	[[nodiscard]] auto findFlag(FlagId id) -> ent::sv::FlagHandle;
	[[nodiscard]] auto findFlag(FlagId id) const -> ent::sv::ConstFlagHandle;
	[[nodiscard]] auto findPayloadCart(PayloadCartId id) -> ent::sv::PayloadCartHandle;
	[[nodiscard]] auto findPayloadCart(PayloadCartId id) const -> ent::sv::ConstPayloadCartHandle;

	[[nodiscard]] auto teleportPlayer(PlayerId id, Vec2 destination) -> bool;
	[[nodiscard]] auto teleportProjectile(ProjectileId id, Vec2 destination) -> bool;
	[[nodiscard]] auto teleportExplosion(ExplosionId id, Vec2 destination) -> bool;
	[[nodiscard]] auto teleportSentryGun(SentryGunId id, Vec2 destination) -> bool;
	[[nodiscard]] auto teleportMedkit(MedkitId id, Vec2 destination) -> bool;
	[[nodiscard]] auto teleportAmmopack(AmmopackId id, Vec2 destination) -> bool;
	[[nodiscard]] auto teleportGenericEntity(GenericEntityId id, Vec2 destination) -> bool;
	[[nodiscard]] auto teleportFlag(FlagId id, Vec2 destination) -> bool;

	[[nodiscard]] auto findPlayerIdByName(std::string_view name) const -> PlayerId;

	[[nodiscard]] auto isPlayerNameTaken(std::string_view name) const -> bool;

	[[nodiscard]] auto isPlayerCarryingFlag(PlayerId id) const -> bool;

	auto playerTeamSelect(PlayerId id, Team team, PlayerClass playerClass) -> bool;

	auto resupplyPlayer(PlayerId id) -> bool;

	auto setPlayerNoclip(PlayerId id, bool value) -> bool;

	auto setPlayerName(PlayerId id, std::string name) -> bool;

	auto equipPlayerHat(PlayerId id, Hat hat) -> bool;

	auto setRoundTimeLeft(float roundTimeLeft) -> void;
	auto addRoundTimeLeft(float roundTimeLeft) -> void;
	[[nodiscard]] auto getRoundTimeLeft() const -> float;

	auto addSpawnPoint(Vec2 position, Team team) -> void;
	[[nodiscard]] auto containsSpawnPoint(const Rect& rect, Team team) const -> bool;

	[[nodiscard]] auto getTeamPlayerCounts() const -> std::unordered_map<Team, std::size_t>;

	[[nodiscard]] auto getPlayerClassCount(Team team, PlayerClass playerClass) const -> std::size_t;

	[[nodiscard]] auto getTeamWins(Team team) const -> Score;

private:
	static_assert(sizeof(PlayerId) >= 4, "Player id type should be at least 32 bits wide to avoid overflow.");
	static_assert(sizeof(ProjectileId) >= 4, "Projectile id type should be at least 32 bits wide to avoid overflow.");
	static_assert(sizeof(ExplosionId) >= 4, "Explosion id type should be at least 32 bits wide to avoid overflow.");
	static_assert(sizeof(SentryGunId) >= 4, "Sentry gun id type should be at least 32 bits wide to avoid overflow.");
	static_assert(sizeof(MedkitId) >= 4, "Medkit id type should be at least 32 bits wide to avoid overflow.");
	static_assert(sizeof(AmmopackId) >= 4, "Ammopack id type should be at least 32 bits wide to avoid overflow.");
	static_assert(sizeof(GenericEntityId) >= 4, "Generic entity id type should be at least 32 bits wide to avoid overflow.");

	using PlayerRegistry = util::Registry<ent::sv::Player, PlayerId>;
	using ProjectileRegistry = util::Registry<ent::sv::Projectile, ProjectileId>;
	using ExplosionRegistry = util::Registry<ent::sv::Explosion, ExplosionId>;
	using SentryGunRegistry = util::Registry<ent::sv::SentryGun, SentryGunId>;
	using MedkitRegistry = util::Registry<ent::sv::Medkit, MedkitId>;
	using AmmopackRegistry = util::Registry<ent::sv::Ammopack, AmmopackId>;
	using GenericEntityRegistry = util::Registry<ent::sv::GenericEntity, GenericEntityId>;
	using FlagRegistry = util::Registry<ent::sv::Flag, FlagId>;
	using PayloadCartRegistry = util::Registry<ent::sv::PayloadCart, PayloadCartId>;

	using PlayerIterator = PlayerRegistry::stable_iterator;
	using ProjectileIterator = ProjectileRegistry::stable_iterator;
	using ExplosionIterator = ExplosionRegistry::stable_iterator;
	using SentryGunIterator = SentryGunRegistry::stable_iterator;
	using MedkitIterator = MedkitRegistry::stable_iterator;
	using AmmopackIterator = AmmopackRegistry::stable_iterator;
	using GenericEntityIterator = GenericEntityRegistry::stable_iterator;
	using FlagIterator = FlagRegistry::stable_iterator;
	using PayloadCartIterator = PayloadCartRegistry::stable_iterator;

	using EntityIterator = std::variant<PlayerIterator, ProjectileIterator, ExplosionIterator, SentryGunIterator, MedkitIterator,
										AmmopackIterator, GenericEntityIterator, FlagIterator, PayloadCartIterator>;
	using CollisionMap = std::unordered_map<Vec2, std::vector<EntityIterator>>;

	struct TeamSpawn final {
		std::vector<Vec2> spawnPoints{};
		std::size_t spawns = 0;
	};
	using TeamSpawns = std::unordered_map<Team, TeamSpawn>;

	using TeamPoints = std::unordered_map<Team, Score>;

	auto updateCollisionMap() -> void;

	auto updatePlayers(float deltaTime) -> void;
	[[nodiscard]] auto updatePlayer(PlayerIterator it, float deltaTime) -> PlayerIterator;
	auto updateSentryGuns(float deltaTime) -> void;
	[[nodiscard]] auto updateSentryGun(SentryGunIterator it, float deltaTime) -> SentryGunIterator;
	auto updateProjectiles(float deltaTime) -> void;
	[[nodiscard]] auto updateProjectile(ProjectileIterator it, float deltaTime) -> ProjectileIterator;
	auto updateExplosions(float deltaTime) -> void;
	[[nodiscard]] auto updateExplosion(ExplosionIterator it, float deltaTime) -> ExplosionIterator;
	auto updateMedkits(float deltaTime) -> void;
	[[nodiscard]] auto updateMedkit(MedkitIterator it, float deltaTime) -> MedkitIterator;
	auto updateAmmopacks(float deltaTime) -> void;
	[[nodiscard]] auto updateAmmopack(AmmopackIterator it, float deltaTime) -> AmmopackIterator;
	auto updateGenericEntities(float deltaTime) -> void;
	[[nodiscard]] auto updateGenericEntity(GenericEntityIterator it, float deltaTime) -> GenericEntityIterator;
	auto updateFlags(float deltaTime) -> void;
	[[nodiscard]] auto updateFlag(FlagIterator it, float deltaTime) -> FlagIterator;
	auto updatePayloadCarts(float deltaTime) -> void;
	[[nodiscard]] auto updatePayloadCart(PayloadCartIterator it, float deltaTime) -> PayloadCartIterator;

	auto updateRoundState(float deltaTime) -> void;
	auto updateTeamSwitchCountdown(float deltaTime) -> void;

	auto checkCollisions(PlayerIterator it) -> void;
	auto checkCollisions(ProjectileIterator it) -> void;
	auto checkCollisions(ExplosionIterator it) -> void;
	auto checkCollisions(ExplosionIterator it, Vec2 position) -> void;
	auto checkCollisions(SentryGunIterator it) -> void;
	auto checkCollisions(MedkitIterator it) -> void;
	auto checkCollisions(AmmopackIterator it) -> void;
	auto checkCollisions(GenericEntityIterator it) -> void;
	auto checkCollisions(GenericEntityIterator it, Vec2 position) -> void;
	auto checkCollisions(FlagIterator it) -> void;
	auto checkCollisions(PayloadCartIterator it) -> void;

	[[nodiscard]] auto isCollideable(PlayerIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(ProjectileIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(ExplosionIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(SentryGunIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(MedkitIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(AmmopackIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(GenericEntityIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(FlagIterator it) const -> bool;
	[[nodiscard]] auto isCollideable(PayloadCartIterator it) const -> bool;

	[[nodiscard]] auto canCollide(PlayerIterator itPlayer, ProjectileIterator itProjectile) const -> bool;
	[[nodiscard]] auto canCollide(PlayerIterator itPlayer, ExplosionIterator itExplosion) const -> bool;
	[[nodiscard]] auto canCollide(PlayerIterator itPlayer, MedkitIterator itMedkit) const -> bool;
	[[nodiscard]] auto canCollide(PlayerIterator itPlayer, AmmopackIterator itAmmopack) const -> bool;
	[[nodiscard]] auto canCollide(PlayerIterator itPlayer, GenericEntityIterator itGenericEntity) const -> bool;
	[[nodiscard]] auto canCollide(PlayerIterator itPlayer, FlagIterator itFlag) const -> bool;
	[[nodiscard]] auto canCollide(ProjectileIterator itProjectile, PlayerIterator itPlayer) const -> bool;
	[[nodiscard]] auto canCollide(ProjectileIterator itProjectileA, ProjectileIterator itProjectileB) const -> bool;
	[[nodiscard]] auto canCollide(ProjectileIterator itProjectile, SentryGunIterator itSentryGun) const -> bool;
	[[nodiscard]] auto canCollide(ProjectileIterator itProjectile, GenericEntityIterator itGenericEntity) const -> bool;
	[[nodiscard]] auto canCollide(ExplosionIterator itExplosion, PlayerIterator itPlayer) const -> bool;
	[[nodiscard]] auto canCollide(ExplosionIterator itExplosion, SentryGunIterator itSentryGun) const -> bool;
	[[nodiscard]] auto canCollide(ExplosionIterator itExplosion, GenericEntityIterator itGenericEntity) const -> bool;
	[[nodiscard]] auto canCollide(SentryGunIterator itSentryGun, ProjectileIterator itProjectile) const -> bool;
	[[nodiscard]] auto canCollide(SentryGunIterator itSentryGun, ExplosionIterator itExplosion) const -> bool;
	[[nodiscard]] auto canCollide(SentryGunIterator itSentryGun, GenericEntityIterator itGenericEntity) const -> bool;
	[[nodiscard]] auto canCollide(MedkitIterator itMedkit, PlayerIterator itPlayer) const -> bool;
	[[nodiscard]] auto canCollide(MedkitIterator itMedkit, GenericEntityIterator itGenericEntity) const -> bool;
	[[nodiscard]] auto canCollide(AmmopackIterator itAmmopack, PlayerIterator itPlayer) const -> bool;
	[[nodiscard]] auto canCollide(AmmopackIterator itAmmopack, GenericEntityIterator itGenericEntity) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, PlayerIterator itPlayer) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, ProjectileIterator itProjectile) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, ExplosionIterator itExplosion) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, SentryGunIterator itSentryGun) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, MedkitIterator itMedkit) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, AmmopackIterator itAmmopack) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntityA, GenericEntityIterator itGenericEntityB) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, FlagIterator itFlag) const -> bool;
	[[nodiscard]] auto canCollide(GenericEntityIterator itGenericEntity, PayloadCartIterator itCart) const -> bool;
	[[nodiscard]] auto canCollide(FlagIterator itFlag, PlayerIterator itPlayer) const -> bool;
	[[nodiscard]] auto canCollide(FlagIterator itFlag, GenericEntityIterator itGenericEntity) const -> bool;
	[[nodiscard]] auto canCollide(PayloadCartIterator itCart, GenericEntityIterator itGenericEntity) const -> bool;

	auto collide(PlayerIterator itPlayer, ProjectileIterator itProjectile) -> void;
	auto collide(PlayerIterator itPlayer, ExplosionIterator itExplosion) -> void;
	auto collide(PlayerIterator itPlayer, MedkitIterator itMedkit) -> void;
	auto collide(PlayerIterator itPlayer, AmmopackIterator itAmmopack) -> void;
	auto collide(PlayerIterator itPlayer, GenericEntityIterator itGenericEntity) -> void;
	auto collide(PlayerIterator itPlayer, FlagIterator itFlag) -> void;
	auto collide(ProjectileIterator itProjectile, PlayerIterator itPlayer) -> void;
	auto collide(ProjectileIterator itProjectileA, ProjectileIterator itProjectileB) -> void;
	auto collide(ProjectileIterator itProjectile, SentryGunIterator itSentryGun) -> void;
	auto collide(ProjectileIterator itProjectile, GenericEntityIterator itGenericEntity) -> void;
	auto collide(ExplosionIterator itExplosion, PlayerIterator itPlayer) -> void;
	auto collide(ExplosionIterator itExplosion, SentryGunIterator itSentryGun) -> void;
	auto collide(ExplosionIterator itExplosion, GenericEntityIterator itGenericEntity) -> void;
	auto collide(SentryGunIterator itSentryGun, ProjectileIterator itProjectile) -> void;
	auto collide(SentryGunIterator itSentryGun, ExplosionIterator itExplosion) -> void;
	auto collide(SentryGunIterator itSentryGun, GenericEntityIterator itGenericEntity) -> void;
	auto collide(MedkitIterator itMedkit, PlayerIterator itPlayer) -> void;
	auto collide(MedkitIterator itMedkit, GenericEntityIterator itGenericEntity) -> void;
	auto collide(AmmopackIterator itAmmopack, PlayerIterator itPlayer) -> void;
	auto collide(AmmopackIterator itAmmopack, GenericEntityIterator itGenericEntity) -> void;
	auto collide(GenericEntityIterator itGenericEntity, PlayerIterator itPlayer) -> void;
	auto collide(GenericEntityIterator itGenericEntity, ProjectileIterator itProjectile) -> void;
	auto collide(GenericEntityIterator itGenericEntity, ExplosionIterator itExplosion) -> void;
	auto collide(GenericEntityIterator itGenericEntity, SentryGunIterator itSentryGun) -> void;
	auto collide(GenericEntityIterator itGenericEntity, MedkitIterator itMedkit) -> void;
	auto collide(GenericEntityIterator itGenericEntity, AmmopackIterator itAmmopack) -> void;
	auto collide(GenericEntityIterator itGenericEntityA, GenericEntityIterator itGenericEntityB) -> void;
	auto collide(GenericEntityIterator itGenericEntity, FlagIterator itFlag) -> void;
	auto collide(GenericEntityIterator itGenericEntity, PayloadCartIterator itCart) -> void;
	auto collide(FlagIterator itFlag, PlayerIterator itPlayer) -> void;
	auto collide(FlagIterator itFlag, GenericEntityIterator itGenericEntity) -> void;
	auto collide(PayloadCartIterator itCart, GenericEntityIterator itGenericEntity) -> void;

	auto stepPlayer(PlayerIterator it, Direction direction) -> void;
	auto stepProjectile(ProjectileIterator it, Direction direction) -> void;
	auto stepGenericEntity(GenericEntityIterator it, int steps = 0) -> void;

	auto teleportPlayer(PlayerIterator it, Vec2 destination) -> bool;
	auto teleportProjectile(ProjectileIterator it, Vec2 destination) -> bool;
	auto teleportExplosion(ExplosionIterator it, Vec2 destination) -> bool;
	auto teleportSentryGun(SentryGunIterator it, Vec2 destination) -> bool;
	auto teleportMedkit(MedkitIterator it, Vec2 destination) -> bool;
	auto teleportAmmopack(AmmopackIterator it, Vec2 destination) -> bool;
	auto teleportGenericEntity(GenericEntityIterator it, Vec2 destination) -> bool;
	auto teleportFlag(FlagIterator it, Vec2 destination) -> bool;

	auto applyDamageToPlayer(PlayerIterator it, Health damage, SoundId hurtSound, bool allowOverheal, PlayerIterator inflictor,
							 Weapon weapon = Weapon::none()) -> void;
	auto applyDamageToSentryGun(SentryGunIterator it, Health damage, SoundId hurtSound, bool allowOverheal, PlayerIterator inflictor) -> void;

	auto killPlayer(PlayerIterator it, bool announce, PlayerIterator killer, Weapon weapon = Weapon::none()) -> void;
	auto killSentryGun(SentryGunIterator it, PlayerIterator killer) -> void;

	auto updatePlayerSpectatorMovement(PlayerIterator it, float deltaTime) -> void;
	auto updatePlayerMovement(PlayerIterator it, float deltaTime) -> void;

	auto updatePlayerWeapon(PlayerIterator it, float deltaTime, Weapon weapon, Weapon otherWeapon, util::CountdownLoop<float>& shootTimer,
							util::CountdownLoop<float>& secondaryShootTimer, Ammo& ammo, bool shooting, util::CountdownLoop<float>& reloadTimer) -> void;
	auto shootPlayerWeapon(PlayerIterator it, Weapon weapon, Weapon otherWeapon, util::CountdownLoop<float>& secondaryShootTimer) -> void;

	auto teleportPlayerToSpawn(PlayerIterator it) -> bool;

	auto playerTeamSelect(PlayerIterator it, Team team, PlayerClass playerClass) -> void;

	auto spawnPlayer(PlayerIterator it) -> void;
	auto resupplyPlayer(PlayerIterator it) -> void;

	auto removePlayerStickies(PlayerIterator it) -> void;
	auto detonatePlayerStickiesUntil(PlayerIterator it, int maxRemaining) -> void;

	auto setPlayerNoclip(PlayerIterator it, bool value) -> void;

	auto setPlayerName(PlayerIterator it, std::string name) -> void;

	auto equipPlayerHat(PlayerIterator it, Hat hat) -> void;

	auto spawnMedkit(MedkitIterator it) -> void;
	auto spawnAmmopack(AmmopackIterator it) -> void;

	auto pickupFlag(FlagIterator it, PlayerIterator carrier) -> void;
	auto dropFlag(FlagIterator it, PlayerIterator carrier) -> void;
	auto returnFlag(FlagIterator it, bool announce) -> void;
	auto captureFlag(FlagIterator it, PlayerIterator carrier) -> void;

	auto createShotgunSpread(Vec2 position, Direction direction, ProjectileType type, Team team, PlayerId owner, Weapon weapon,
							 Health damage, SoundId hurtSound, float disappearTime, float moveInterval) -> void;
	auto createSniperRifleTrail(Vec2 position, Direction direction, ProjectileType type, Team team, PlayerId owner, Weapon weapon,
								Health damage, SoundId hurtSound, float disappearTime, float moveInterval) -> void;

	auto cleanupSentryGuns(PlayerId id) -> void;
	auto cleanupProjectiles(PlayerId id) -> void;

	[[nodiscard]] auto canTeleport(bool red, bool blue, bool noclip, Vec2 destination) const -> bool;
	[[nodiscard]] auto canMove(bool red, bool blue, bool noclip, Vec2 destination, Direction moveDirection) const -> bool;
	[[nodiscard]] auto canMove(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> bool;

	[[nodiscard]] auto getClippedMovementDestination(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> Vec2;
	[[nodiscard]] auto getClippedMovementOffset(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> Vec2;
	[[nodiscard]] auto getClippedMovementDirection(Vec2 position, bool red, bool blue, bool noclip, Direction moveDirection) const -> Direction;

	[[nodiscard]] auto getPlayersPushingCart(PayloadCartIterator it) -> std::vector<PlayerIterator>;

	[[nodiscard]] auto isKnifeTarget(Vec2 position, Team team) const -> bool;
	[[nodiscard]] auto findKnifeTargetPlayer(Vec2 position, Team team) -> PlayerIterator;
	[[nodiscard]] auto findKnifeTargetSentryGun(Vec2 position, Team team) -> SentryGunIterator;

	const Map& m_map;
	GameServer& m_server;
	TickCount m_tickCount = 0;
	util::Countdown<float> m_roundCountdown{};
	util::Countdown<float> m_levelChangeCountdown{};
	util::Countdown<float> m_teamSwitchCountdown{};
	PlayerRegistry m_players{};
	ProjectileRegistry m_projectiles{};
	ExplosionRegistry m_explosions{};
	SentryGunRegistry m_sentryGuns{};
	MedkitRegistry m_medkits{};
	AmmopackRegistry m_ammopacks{};
	GenericEntityRegistry m_genericEntities{};
	FlagRegistry m_flags{};
	PayloadCartRegistry m_carts{};
	TeamSpawns m_teamSpawns{};
	TeamPoints m_teamWins{};
	CollisionMap m_collisionMap{};
	float m_mapTime = 0.0f;
	int m_roundsPlayed = 0;
	bool m_awaitingLevelChange = false;
	bool m_awaitingTeamSwitch = false;
};

#endif
