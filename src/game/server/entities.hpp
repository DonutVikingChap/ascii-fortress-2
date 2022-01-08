#ifndef AF2_SERVER_ENTITIES_HPP
#define AF2_SERVER_ENTITIES_HPP

#include "../../utilities/countdown.hpp"   // util::Countdown, util::CountdownLoop, util::Countup
#include "../../utilities/tile_matrix.hpp" // util::TileMatrix
#include "../data/actions.hpp"             // Actions, Action
#include "../data/ammo.hpp"                // Ammo
#include "../data/color.hpp"               // Color
#include "../data/direction.hpp"           // Direction
#include "../data/hat.hpp"                 // Hat
#include "../data/health.hpp"              // Health
#include "../data/latency.hpp"             // Latency
#include "../data/player_class.hpp"        // PlayerClass
#include "../data/player_id.hpp"           // PlayerId, PLAYER_ID_UNCONNECTED
#include "../data/projectile_type.hpp"     // ProjectileType
#include "../data/score.hpp"               // Score
#include "../data/team.hpp"                // Team
#include "../data/vector.hpp"              // Vec2, Vector2
#include "../data/weapon.hpp"              // Weapon
#include "solid.hpp"                       // Solid

#include <cassert>       // assert
#include <cstddef>       // std::size_t
#include <string>        // std::string
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace ent {
namespace sv {

template <typename EntityType>
class EntityHandle {
public:
	constexpr EntityHandle() noexcept = default;
	constexpr explicit EntityHandle(EntityType* entity) noexcept
		: m_entity(entity) {}

	[[nodiscard]] constexpr auto isValid() const noexcept -> bool {
		return m_entity != nullptr;
	}

	constexpr explicit operator bool() const noexcept {
		return this->isValid();
	}

protected:
	[[nodiscard]] constexpr auto entity() const noexcept -> EntityType& {
		assert(this->isValid());
		return *m_entity;
	}

private:
	EntityType* m_entity = nullptr;
};

struct Player final {
	std::string name{};
	Vec2 position{};
	Direction blastJumpDirection = Direction::none();
	util::CountdownLoop<float> blastJumpTimer{};
	util::Countdown<float> blastJumpCountdown{};
	bool blastJumping = false;
	float blastJumpInterval = 0.0f;
	Latency latestMeasuredPingDuration = 0;
	Direction moveDirection = Direction::none();
	Direction aimDirection = Direction::up();
	bool attack1 = false;
	bool attack2 = false;
	Team team = Team::spectators();
	PlayerClass playerClass = PlayerClass::spectator();
	bool alive = false;
	bool disguised = false;
	Health health = 0;
	Score score = 0;
	int nStickies = 0;
	bool noclip = false;
	util::Countdown<float> respawnCountdown{};
	bool respawning = false;
	util::CountdownLoop<float> moveTimer{};
	util::CountdownLoop<float> attack1Timer{};
	util::CountdownLoop<float> attack2Timer{};
	util::CountdownLoop<float> primaryReloadTimer{};
	util::CountdownLoop<float> secondaryReloadTimer{};
	Ammo primaryAmmo = 0;
	Ammo secondaryAmmo = 0;
	Hat hat = Hat::none();
};

struct ConstPlayerHandle : EntityHandle<Player> {
	constexpr explicit ConstPlayerHandle(Player* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstPlayerHandle(const Player* entity) noexcept
		: EntityHandle(const_cast<Player*>(entity)) {}

	[[nodiscard]] auto getName() const noexcept -> std::string_view {
		return this->entity().name;
	}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto getMoveDirection() const noexcept -> Direction {
		return this->entity().moveDirection;
	}

	[[nodiscard]] auto getAimDirection() const noexcept -> Direction {
		return this->entity().aimDirection;
	}

	[[nodiscard]] auto getAttack1() const noexcept -> bool {
		return this->entity().attack1;
	}

	[[nodiscard]] auto getAttack2() const noexcept -> bool {
		return this->entity().attack2;
	}

	[[nodiscard]] auto getTeam() const noexcept -> Team {
		return this->entity().team;
	}

	[[nodiscard]] auto getPlayerClass() const noexcept -> PlayerClass {
		return this->entity().playerClass;
	}

	[[nodiscard]] auto isAlive() const noexcept -> bool {
		return this->entity().alive;
	}

	[[nodiscard]] auto getLatestMeasuredPingDuration() const noexcept -> Latency {
		return this->entity().latestMeasuredPingDuration;
	}

	[[nodiscard]] auto isDisguised() const noexcept -> bool {
		return this->entity().disguised;
	}

	[[nodiscard]] auto getHealth() const noexcept -> Health {
		return this->entity().health;
	}

	[[nodiscard]] auto getScore() const noexcept -> Score {
		return this->entity().score;
	}

	[[nodiscard]] auto isNoclip() const noexcept -> bool {
		return this->entity().noclip;
	}

	[[nodiscard]] auto getPrimaryAmmo() const noexcept -> Ammo {
		return this->entity().primaryAmmo;
	}

	[[nodiscard]] auto getSecondaryAmmo() const noexcept -> Ammo {
		return this->entity().secondaryAmmo;
	}

	[[nodiscard]] auto getHat() const noexcept -> Hat {
		return this->entity().hat;
	}
};

struct PlayerHandle : ConstPlayerHandle {
	constexpr explicit PlayerHandle(Player* entity) noexcept
		: ConstPlayerHandle(entity) {}

	auto setLatestMeasuredPingDuration(Latency ping) const noexcept -> void {
		this->entity().latestMeasuredPingDuration = ping;
	}

	auto setDisguised(bool disguised) const noexcept -> void {
		this->entity().disguised = disguised;
	}

	auto setScore(Score score) const noexcept -> void {
		this->entity().score = score;
	}

	auto setMoveDirection(Direction moveDirection) const noexcept -> void {
		this->entity().moveDirection = moveDirection;
	}

	auto setAimDirection(Direction aimDirection) const noexcept -> void {
		this->entity().aimDirection = aimDirection;
	}

	auto setAttack1(bool attack1) const noexcept -> void {
		this->entity().attack1 = attack1;
	}

	auto setAttack2(bool attack2) const noexcept -> void {
		this->entity().attack2 = attack2;
	}

	auto setActions(Actions actions) const noexcept -> void {
		const auto newMoveDirection = Direction{(actions & Action::MOVE_LEFT) != 0,
		                                        (actions & Action::MOVE_RIGHT) != 0,
		                                        (actions & Action::MOVE_UP) != 0,
		                                        (actions & Action::MOVE_DOWN) != 0};

		const auto newAimDirection = Direction{(actions & Action::AIM_LEFT) != 0,
		                                       (actions & Action::AIM_RIGHT) != 0,
		                                       (actions & Action::AIM_UP) != 0,
		                                       (actions & Action::AIM_DOWN) != 0};

		const auto newAttack1 = (actions & Action::ATTACK1) != 0;
		const auto newAttack2 = (actions & Action::ATTACK2) != 0;

		auto& moveDirection = this->entity().moveDirection;
		auto& aimDirection = this->entity().aimDirection;
		auto& attack1 = this->entity().attack1;
		auto& attack2 = this->entity().attack2;

		moveDirection = newMoveDirection;
		if (newAimDirection.isAny()) {
			aimDirection = newAimDirection;
		}
		attack1 = newAttack1;
		attack2 = newAttack2;
	}
};

struct SentryGun final {
	Vec2 position{};
	Direction aimDirection = Direction::none();
	Team team = Team::spectators();
	Health health = 0;
	PlayerId owner = PLAYER_ID_UNCONNECTED;
	util::CountdownLoop<float> shootTimer{};
	util::Countdown<float> despawnTimer{};
	bool alive = false;
};

struct ConstSentryGunHandle : EntityHandle<SentryGun> {
	constexpr explicit ConstSentryGunHandle(SentryGun* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstSentryGunHandle(const SentryGun* entity) noexcept
		: EntityHandle(const_cast<SentryGun*>(entity)) {}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto getAimDirection() const noexcept -> Direction {
		return this->entity().aimDirection;
	}

	[[nodiscard]] auto getTeam() const noexcept -> Team {
		return this->entity().team;
	}

	[[nodiscard]] auto getHealth() const noexcept -> Health {
		return this->entity().health;
	}

	[[nodiscard]] auto getOwner() const noexcept -> PlayerId {
		return this->entity().owner;
	}
};

struct SentryGunHandle : ConstSentryGunHandle {
	constexpr explicit SentryGunHandle(SentryGun* entity) noexcept
		: ConstSentryGunHandle(entity) {}

	auto setAimDirection(Direction aimDirection) const noexcept -> void {
		this->entity().aimDirection = aimDirection;
	}

	auto setOwner(PlayerId owner) const noexcept -> void {
		this->entity().owner = owner;
	}
};

struct Projectile final {
	Vec2 position{};
	ProjectileType type = ProjectileType::none();
	Team team = Team::spectators();
	Direction moveDirection = Direction::none();
	PlayerId owner = PLAYER_ID_UNCONNECTED;
	Weapon weapon = Weapon::none();
	Health damage = 0;
	SoundId hurtSound = SoundId::none();
	util::Countdown<float> disappearTimer{};
	float moveInterval = 0.0f;
	util::CountdownLoop<float> moveTimer{};
	bool stickyAttached = false;
};

struct ConstProjectileHandle : EntityHandle<Projectile> {
	constexpr explicit ConstProjectileHandle(Projectile* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstProjectileHandle(const Projectile* entity) noexcept
		: EntityHandle(const_cast<Projectile*>(entity)) {}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto getType() const noexcept -> ProjectileType {
		return this->entity().type;
	}

	[[nodiscard]] auto getTeam() const noexcept -> Team {
		return this->entity().team;
	}

	[[nodiscard]] auto getMoveDirection() const noexcept -> Direction {
		return this->entity().moveDirection;
	}

	[[nodiscard]] auto getOwner() const noexcept -> PlayerId {
		return this->entity().owner;
	}

	[[nodiscard]] auto getWeapon() const noexcept -> Weapon {
		return this->entity().weapon;
	}

	[[nodiscard]] auto getDamage() const noexcept -> Health {
		return this->entity().damage;
	}

	[[nodiscard]] auto getHurtSound() const noexcept -> SoundId {
		return this->entity().hurtSound;
	}

	[[nodiscard]] auto getTimeLeft() const noexcept -> float {
		return this->entity().disappearTimer.getTimeLeft();
	}

	[[nodiscard]] auto getMoveInterval() const noexcept -> float {
		return this->entity().moveInterval;
	}

	[[nodiscard]] auto isStickyAttached() const noexcept -> bool {
		return this->entity().stickyAttached;
	}
};

struct ProjectileHandle : ConstProjectileHandle {
	constexpr explicit ProjectileHandle(Projectile* entity) noexcept
		: ConstProjectileHandle(entity) {}

	auto setMoveDirection(Direction moveDirection) const noexcept -> void {
		this->entity().moveDirection = moveDirection;
	}

	auto setOwner(PlayerId owner) const noexcept -> void {
		this->entity().owner = owner;
	}

	auto setWeapon(Weapon weapon) const noexcept -> void {
		this->entity().weapon = weapon;
	}

	auto setDamage(Health damage) const noexcept -> void {
		this->entity().damage = damage;
	}

	auto setHurtSound(SoundId hurtSound) const noexcept -> void {
		this->entity().hurtSound = hurtSound;
	}

	auto setTimeLeft(float time) const noexcept -> void {
		this->entity().disappearTimer.start(time);
	}

	auto setMoveInterval(float moveInterval) const noexcept -> void {
		this->entity().moveInterval = moveInterval;
	}
};

struct Explosion final {
	Vec2 position{};
	Team team = Team::spectators();
	PlayerId owner = PLAYER_ID_UNCONNECTED;
	Weapon weapon = Weapon::none();
	Health damage = 0;
	SoundId hurtSound = SoundId::none();
	std::unordered_set<PlayerId> damagedPlayers{};
	std::unordered_set<PlayerId> damagedSentryGuns{};
	util::Countdown<float> disappearTimer{};
};

struct ConstExplosionHandle : EntityHandle<Explosion> {
	constexpr explicit ConstExplosionHandle(Explosion* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstExplosionHandle(const Explosion* entity) noexcept
		: EntityHandle(const_cast<Explosion*>(entity)) {}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto getTeam() const noexcept -> Team {
		return this->entity().team;
	}

	[[nodiscard]] auto getOwner() const noexcept -> PlayerId {
		return this->entity().owner;
	}

	[[nodiscard]] auto getWeapon() const noexcept -> Weapon {
		return this->entity().weapon;
	}

	[[nodiscard]] auto getDamage() const noexcept -> Health {
		return this->entity().damage;
	}

	[[nodiscard]] auto getHurtSound() const noexcept -> SoundId {
		return this->entity().hurtSound;
	}

	[[nodiscard]] auto getTimeLeft() const noexcept -> float {
		return this->entity().disappearTimer.getTimeLeft();
	}
};

struct ExplosionHandle : ConstExplosionHandle {
	constexpr explicit ExplosionHandle(Explosion* entity) noexcept
		: ConstExplosionHandle(entity) {}

	auto setOwner(PlayerId owner) const noexcept -> void {
		this->entity().owner = owner;
	}

	auto setWeapon(Weapon weapon) const noexcept -> void {
		this->entity().weapon = weapon;
	}

	auto setDamage(Health damage) const noexcept -> void {
		this->entity().damage = damage;
	}

	auto setHurtSound(SoundId hurtSound) const noexcept -> void {
		this->entity().hurtSound = hurtSound;
	}

	auto setTimeLeft(float time) const noexcept -> void {
		this->entity().disappearTimer.start(time);
	}
};

struct Medkit final {
	Vec2 position{};
	util::Countdown<float> respawnCountdown{};
	bool alive = false;
};

struct ConstMedkitHandle : EntityHandle<Medkit> {
	constexpr explicit ConstMedkitHandle(Medkit* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstMedkitHandle(const Medkit* entity) noexcept
		: EntityHandle(const_cast<Medkit*>(entity)) {}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto isAlive() const noexcept -> bool {
		return this->entity().alive;
	}

	[[nodiscard]] auto getRespawnTimeLeft() const noexcept -> float {
		return this->entity().respawnCountdown.getTimeLeft();
	}
};

struct MedkitHandle : ConstMedkitHandle {
	constexpr explicit MedkitHandle(Medkit* entity) noexcept
		: ConstMedkitHandle(entity) {}
};

struct Ammopack final {
	Vec2 position{};
	util::Countdown<float> respawnCountdown{};
	bool alive = false;
};

struct ConstAmmopackHandle : EntityHandle<Ammopack> {
	constexpr explicit ConstAmmopackHandle(Ammopack* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstAmmopackHandle(const Ammopack* entity) noexcept
		: EntityHandle(const_cast<Ammopack*>(entity)) {}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto isAlive() const noexcept -> bool {
		return this->entity().alive;
	}

	[[nodiscard]] auto getRespawnTimeLeft() const noexcept -> float {
		return this->entity().respawnCountdown.getTimeLeft();
	}
};

struct AmmopackHandle : ConstAmmopackHandle {
	constexpr explicit AmmopackHandle(Ammopack* entity) noexcept
		: ConstAmmopackHandle(entity) {}
};

struct Flag final {
	std::string name{};
	Vec2 position{};
	Vec2 spawnPosition{};
	Team team = Team::spectators();
	Score score = 0;
	PlayerId carrier = PLAYER_ID_UNCONNECTED;
	util::Countdown<float> returnCountdown{};
	bool returning = false;
};

struct ConstFlagHandle : EntityHandle<Flag> {
	constexpr explicit ConstFlagHandle(Flag* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstFlagHandle(const Flag* entity) noexcept
		: EntityHandle(const_cast<Flag*>(entity)) {}

	[[nodiscard]] auto getName() const noexcept -> std::string_view {
		return this->entity().name;
	}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto getSpawnPosition() const noexcept -> Vec2 {
		return this->entity().spawnPosition;
	}

	[[nodiscard]] auto getTeam() const noexcept -> Team {
		return this->entity().team;
	}

	[[nodiscard]] auto getScore() const noexcept -> Score {
		return this->entity().score;
	}

	[[nodiscard]] auto getCarrier() const noexcept -> PlayerId {
		return this->entity().carrier;
	}

	[[nodiscard]] auto getReturnTimeLeft() const noexcept -> float {
		return this->entity().returnCountdown.getTimeLeft();
	}
};

struct FlagHandle : ConstFlagHandle {
	constexpr explicit FlagHandle(Flag* entity) noexcept
		: ConstFlagHandle(entity) {}

	auto setName(std::string name) const -> void {
		this->entity().name = std::move(name);
	}

	auto setSpawnPosition(Vec2 spawnPosition) const noexcept -> void {
		this->entity().spawnPosition = spawnPosition;
	}
};

struct PayloadCart final {
	Team team = Team::spectators();
	std::vector<Vec2> track{};
	std::size_t currentTrackIndex = 0;
	util::CountdownLoop<float> pushTimer{};
};

struct ConstPayloadCartHandle : EntityHandle<PayloadCart> {
	constexpr explicit ConstPayloadCartHandle(PayloadCart* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstPayloadCartHandle(const PayloadCart* entity) noexcept
		: EntityHandle(const_cast<PayloadCart*>(entity)) {}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		assert(this->entity().currentTrackIndex < this->entity().track.size());
		return this->entity().track[this->entity().currentTrackIndex];
	}

	[[nodiscard]] auto getTeam() const noexcept -> Team {
		return this->entity().team;
	}

	[[nodiscard]] auto getTrackSize() const noexcept -> std::size_t {
		return this->entity().track.size();
	}

	[[nodiscard]] auto getTrackIndex() const noexcept -> std::size_t {
		return this->entity().currentTrackIndex;
	}
};

struct PayloadCartHandle : ConstPayloadCartHandle {
	constexpr explicit PayloadCartHandle(PayloadCart* entity) noexcept
		: ConstPayloadCartHandle(entity) {}
};

struct GenericEntity final {
	Vec2 position{};
	Vec2 velocity{};
	util::TileMatrix<char> matrix{};
	Color color{};
	Solid::Flags solidFlags = Solid::NONE;
	float moveInterval = 0.0f;
	util::CountdownLoop<float> moveTimer{};
	bool visible = true;
};

struct ConstGenericEntityHandle : EntityHandle<GenericEntity> {
	constexpr explicit ConstGenericEntityHandle(GenericEntity* entity) noexcept
		: EntityHandle(entity) {}

	constexpr explicit ConstGenericEntityHandle(const GenericEntity* entity) noexcept
		: EntityHandle(const_cast<GenericEntity*>(entity)) {}

	[[nodiscard]] auto getPosition() const noexcept -> Vec2 {
		return this->entity().position;
	}

	[[nodiscard]] auto getVelocity() const noexcept -> Vec2 {
		return this->entity().velocity;
	}

	[[nodiscard]] auto getColor() const noexcept -> Color {
		return this->entity().color;
	}

	[[nodiscard]] auto getSolidFlags() const noexcept -> Solid::Flags {
		return this->entity().solidFlags;
	}

	[[nodiscard]] auto getMoveInterval() const noexcept -> float {
		return this->entity().moveInterval;
	}

	[[nodiscard]] auto isVisible() const noexcept -> bool {
		return this->entity().visible;
	}

	[[nodiscard]] auto matrix() const noexcept -> const util::TileMatrix<char>& {
		return this->entity().matrix;
	}
};

struct GenericEntityHandle : ConstGenericEntityHandle {
	constexpr explicit GenericEntityHandle(GenericEntity* entity) noexcept
		: ConstGenericEntityHandle(entity) {}

	auto setVelocity(Vec2 velocity) const noexcept -> void {
		this->entity().velocity = velocity;
	}

	auto setColor(Color color) const noexcept -> void {
		this->entity().color = color;
	}

	auto setSolidFlags(Solid::Flags solidFlags) const noexcept -> void {
		this->entity().solidFlags = solidFlags;
	}

	auto setMoveInterval(float moveInterval) const noexcept -> void {
		this->entity().moveInterval = moveInterval;
	}

	auto setVisible(bool visible) const noexcept -> void {
		this->entity().visible = visible;
	}

	[[nodiscard]] auto matrix() const noexcept -> util::TileMatrix<char>& {
		return this->entity().matrix;
	}
};

} // namespace sv
} // namespace ent

#endif
