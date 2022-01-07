#ifndef AF2_SHARED_ENTITIES_HPP
#define AF2_SHARED_ENTITIES_HPP

#include "../../network/delta.hpp"         // net::TieDeltaCompressableDecompressableBase
#include "../../utilities/algorithm.hpp"   // util::findClosestDistanceSquared
#include "../../utilities/tile_matrix.hpp" // util::TileMatrix
#include "../data/direction.hpp"           // Direction
#include "../data/hat.hpp"                 // Hat
#include "../data/health.hpp"              // Health
#include "../data/latency.hpp"             // Latency
#include "../data/player_class.hpp"        // PlayerClass
#include "../data/player_id.hpp"           // PlayerId, PLAYER_ID_UNCONNECTED
#include "../data/projectile_type.hpp"     // ProjectileType
#include "../data/score.hpp"               // Score
#include "../data/team.hpp"                // Team
#include "../data/vector.hpp"              // Vec2

#include <cstdint> // std::uint16_t
#include <string>  // std::string
#include <tuple>   // std::tie
#include <utility> // std::forward

namespace ent {

template <typename Range>
[[nodiscard]] inline auto findClosestDistanceSquared(Range&& range, Vec2 position) {
	return util::findClosestDistanceSquared(std::forward<Range>(range), position, [](const auto& entity) { return entity.position; });
}

namespace sh {

struct SelfPlayer final : net::TieDeltaCompressableDecompressableBase<SelfPlayer> {
	Vec2 position{};
	Team team = Team::spectators();
	Team skinTeam = Team::spectators();
	bool alive = false;
	Direction aimDirection = Direction::none();
	PlayerClass playerClass = PlayerClass::spectator();
	Health health = 0;
	Ammo primaryAmmo = 0;
	Ammo secondaryAmmo = 0;
	Hat hat = Hat::none();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team, skinTeam, alive, aimDirection, playerClass, health, primaryAmmo, secondaryAmmo, hat);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team, skinTeam, alive, aimDirection, playerClass, health, primaryAmmo, secondaryAmmo, hat);
	}
};

struct FlagInfo final : net::TieDeltaCompressableDecompressableBase<FlagInfo> {
	Team team = Team::spectators();
	Score score = 0;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(team, score);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(team, score);
	}
};

struct PayloadCartInfo final : net::TieDeltaCompressableDecompressableBase<PayloadCartInfo> {
	Team team = Team::spectators();
	std::uint16_t progress = 0;
	std::uint16_t trackLength = 0;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(team, progress, trackLength);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(team, progress, trackLength);
	}
};

struct PlayerInfo final : net::TieDeltaCompressableDecompressableBase<PlayerInfo> {
	PlayerId id = PLAYER_ID_UNCONNECTED;
	Team team = Team::spectators();
	Score score = 0;
	PlayerClass playerClass = PlayerClass::spectator();
	Latency ping = 0;
	std::string name{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(id, team, score, playerClass, ping, name);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(id, team, score, playerClass, ping, name);
	}
};

struct Player final : net::TieDeltaCompressableDecompressableBase<Player> {
	Vec2 position{};
	Team team = Team::spectators();
	Direction aimDirection = Direction::none();
	PlayerClass playerClass = PlayerClass::spectator();
	Hat hat = Hat::none();
	std::string name{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team, aimDirection, playerClass, hat, name);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team, aimDirection, playerClass, hat, name);
	}
};

struct Corpse final : net::TieDeltaCompressableDecompressableBase<Corpse> {
	Vec2 position{};
	Team team = Team::spectators();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team);
	}
};

struct SentryGun final : net::TieDeltaCompressableDecompressableBase<SentryGun> {
	Vec2 position{};
	Team team = Team::spectators();
	Direction aimDirection = Direction::none();
	PlayerId owner = PLAYER_ID_UNCONNECTED;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team, aimDirection, owner);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team, aimDirection, owner);
	}
};

struct Projectile final : net::TieDeltaCompressableDecompressableBase<Projectile> {
	Vec2 position{};
	Team team = Team::spectators();
	ProjectileType type = ProjectileType::none();
	PlayerId owner = PLAYER_ID_UNCONNECTED;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team, type, owner);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team, type, owner);
	}
};

struct Explosion final : net::TieDeltaCompressableDecompressableBase<Explosion> {
	Vec2 position{};
	Team team = Team::spectators();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team);
	}
};

struct Medkit final : net::TieDeltaCompressableDecompressableBase<Medkit> {
	Vec2 position{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position);
	}
};

struct Ammopack final : net::TieDeltaCompressableDecompressableBase<Ammopack> {
	Vec2 position{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position);
	}
};

struct GenericEntity final : net::TieDeltaCompressableDecompressableBase<GenericEntity> {
	Vec2 position{};
	util::TileMatrix<char> matrix{};
	Color color{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, matrix, color);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, matrix, color);
	}
};

struct Flag final : net::TieDeltaCompressableDecompressableBase<Flag> {
	Vec2 position{};
	Team team = Team::spectators();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team);
	}
};

struct PayloadCart final : net::TieDeltaCompressableDecompressableBase<PayloadCart> {
	Vec2 position{};
	Team team = Team::spectators();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(position, team);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(position, team);
	}
};

} // namespace sh
} // namespace ent

#endif
