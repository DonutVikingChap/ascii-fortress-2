#ifndef AF2_SHARED_SNAPSHOT_HPP
#define AF2_SHARED_SNAPSHOT_HPP

#include "../../network/delta.hpp" // net::TieDeltaCompressableDecompressableBase
#include "../data/tick_count.hpp"  // TickCount
#include "entities.hpp"            // ent::sh::...

#include <cassert> // assert
#include <cstdint> // std::uint32_t
#include <tuple>   // std::tie
#include <vector>  // std::vector

struct Snapshot final : net::TieDeltaCompressableDecompressableBase<Snapshot> {
	TickCount tickCount = 0;
	std::uint32_t roundSecondsLeft = 0;
	ent::sh::SelfPlayer selfPlayer{};
	std::vector<ent::sh::FlagInfo> flagInfo{};
	std::vector<ent::sh::PayloadCartInfo> cartInfo{};
	std::vector<ent::sh::PlayerInfo> playerInfo{};
	std::vector<ent::sh::Player> players{};
	std::vector<ent::sh::Corpse> corpses{};
	std::vector<ent::sh::SentryGun> sentryGuns{};
	std::vector<ent::sh::Projectile> projectiles{};
	std::vector<ent::sh::Explosion> explosions{};
	std::vector<ent::sh::Medkit> medkits{};
	std::vector<ent::sh::Ammopack> ammopacks{};
	std::vector<ent::sh::GenericEntity> genericEntities{};
	std::vector<ent::sh::Flag> flags{};
	std::vector<ent::sh::PayloadCart> carts{};

	[[nodiscard]] auto findPlayerInfo(PlayerId id) -> ent::sh::PlayerInfo*;
	[[nodiscard]] auto findPlayerInfo(PlayerId id) const -> const ent::sh::PlayerInfo*;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(tickCount, roundSecondsLeft, selfPlayer, flagInfo, cartInfo, playerInfo, players, corpses, sentryGuns, projectiles, explosions, medkits, ammopacks, genericEntities, flags, carts);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(tickCount, roundSecondsLeft, selfPlayer, flagInfo, cartInfo, playerInfo, players, corpses, sentryGuns, projectiles, explosions, medkits, ammopacks, genericEntities, flags, carts);
	}
};

#endif
