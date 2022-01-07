#ifndef CONSOLE_COMMAND_UTILITIES_H
#define CONSOLE_COMMAND_UTILITIES_H

#include "../game/data/color.hpp"           // Color
#include "../game/data/player_class.hpp"    // PlayerClass
#include "../game/data/player_id.hpp"       // PlayerId
#include "../game/data/projectile_type.hpp" // ProjectileType
#include "../game/data/sound_id.hpp"        // SoundId
#include "../game/data/team.hpp"            // Team
#include "../game/data/weapon.hpp"          // Weapon
#include "../game/server/game_server.hpp"   // GameServer
#include "../game/server/world.hpp"         // World
#include "../network/endpoint.hpp"          // net::IpAddress, net::IpEndpoint
#include "../utilities/algorithm.hpp"       // util::collect, util::transform, util::filter
#include "../utilities/string.hpp"          // util::toString, util::stringTo, util::toLower, util::join
#include "command.hpp"                      // cmd::...
#include "suggestions.hpp"                  // Suggestions, SUGGESTIONS

#include <cassert>      // assert
#include <cstddef>      // std::size_t
#include <fmt/core.h>   // fmt::format
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <system_error> // std::error_code
#include <type_traits>  // std::enable_if_t, std::is_..._v
#include <utility>      // std::move
#include <vector>       // std::vector

namespace cmd {

class ParseError final {
public:
	explicit operator bool() const noexcept {
		return !m_errorStrings.empty();
	}

	auto emplace(std::string errorString) -> void {
		m_errorStrings.push_back(std::move(errorString));
	}

	[[nodiscard]] auto operator*() const -> std::string {
		assert(!m_errorStrings.empty());
		if (m_errorStrings.size() == 1) {
			return m_errorStrings.front();
		}
		return "\n  " + (m_errorStrings | util::join("\n  "));
	}

private:
	std::vector<std::string> m_errorStrings{};
};

enum class NumberConstraint : std::uint8_t { NONE, NON_ZERO, POSITIVE, NEGATIVE, NON_NEGATIVE, NON_POSITIVE };

template <typename T, NumberConstraint CONSTRAINT = NumberConstraint::NONE>
[[nodiscard]] inline auto parseNumber(ParseError& parseError, std::string_view str, std::string_view name) -> T {
	static_assert(!(std::is_unsigned_v<T> && (CONSTRAINT == NumberConstraint::NEGATIVE || CONSTRAINT == NumberConstraint::NON_POSITIVE)),
				  "These constraints do not make sense for an unsigned type.");
	static_assert(!(std::is_unsigned_v<T> && (CONSTRAINT == NumberConstraint::NON_ZERO)),
				  "Use POSITIVE instead of NON_ZERO for unsigned types.");

	if (parseError) {
		return T{};
	}

	if (const auto val = util::stringTo<T>(str)) {
		if constexpr (CONSTRAINT == NumberConstraint::NON_ZERO) {
			if (val != T{0}) {
				return *val;
			}
		} else if constexpr (CONSTRAINT == NumberConstraint::POSITIVE) {
			if (val > T{0}) {
				return *val;
			}
		} else if constexpr (CONSTRAINT == NumberConstraint::NEGATIVE) {
			if (val < T{0}) {
				return *val;
			}
		} else if constexpr (CONSTRAINT == NumberConstraint::NON_NEGATIVE) {
			if (val >= T{0}) {
				return *val;
			}
		} else if constexpr (CONSTRAINT == NumberConstraint::NON_POSITIVE) {
			if (val <= T{0}) {
				return *val;
			}
		} else {
			return *val;
		}
	}

	static constexpr auto typeStr = []() -> std::string_view {
		if constexpr (std::is_integral_v<T>) {
			if constexpr (CONSTRAINT == NumberConstraint::NON_ZERO) {
				return "a non-zero integer";
			} else if constexpr (CONSTRAINT == NumberConstraint::POSITIVE) {
				return "a positive integer";
			} else if constexpr (CONSTRAINT == NumberConstraint::NEGATIVE) {
				return "a negative integer";
			} else if constexpr (CONSTRAINT == NumberConstraint::NON_NEGATIVE) {
				return "a non-negative integer";
			} else if constexpr (CONSTRAINT == NumberConstraint::NON_POSITIVE) {
				return "a non-positive integer";
			} else if constexpr (std::is_unsigned_v<T>) {
				return "a non-negative integer";
			} else {
				return "an integer";
			}
		} else {
			if constexpr (CONSTRAINT == NumberConstraint::NON_ZERO) {
				return "a non-zero number";
			} else if constexpr (CONSTRAINT == NumberConstraint::POSITIVE) {
				return "a positive number";
			} else if constexpr (CONSTRAINT == NumberConstraint::NEGATIVE) {
				return "a negative number";
			} else if constexpr (CONSTRAINT == NumberConstraint::NON_NEGATIVE) {
				return "a non-negative number";
			} else if constexpr (CONSTRAINT == NumberConstraint::NON_POSITIVE) {
				return "a non-positive number";
			} else {
				return "a number";
			}
		}
	}();
	parseError.emplace(fmt::format("Invalid {} \"{}\" (must be {}).", name, str, typeStr));
	return T{};
}

[[nodiscard]] inline auto parseBool(ParseError& parseError, std::string_view str, std::string_view name) -> bool {
	if (parseError) {
		return false;
	}
	if (str == "0" || str == "1") {
		return str != "0";
	}
	parseError.emplace(fmt::format("Invalid {} \"{}\" (must be 1 or 0).", name, str));
	return false;
}

[[nodiscard]] inline auto parseColor(ParseError& parseError, std::string_view str, std::string_view name) -> Color {
	if (parseError) {
		return Color{};
	}
	if (const auto color = Color::parse(str)) {
		return *color;
	}
	parseError.emplace(fmt::format("Invalid {} \"{}\".", name, str));
	return Color{};
}

[[nodiscard]] inline auto parseTeam(ParseError& parseError, std::string_view str, std::string_view name) -> Team {
	if (parseError) {
		return Team::none();
	}
	if (const auto team = Team::findByName(str); team != Team::none()) {
		return team;
	}
	parseError.emplace(fmt::format("Invalid {} \"{}\".", name, str));
	return Team::none();
}

[[nodiscard]] inline auto parsePlayerClass(ParseError& parseError, std::string_view str, std::string_view name) -> PlayerClass {
	if (parseError) {
		return PlayerClass::none();
	}
	if (const auto playerClass = PlayerClass::findByName(str); playerClass != PlayerClass::none()) {
		return playerClass;
	}
	parseError.emplace(fmt::format("Invalid {} \"{}\".", name, str));
	return PlayerClass::none();
}

[[nodiscard]] inline auto parseProjectileType(ParseError& parseError, std::string_view str, std::string_view name) -> ProjectileType {
	if (parseError) {
		return ProjectileType::none();
	}
	if (const auto projectileType = ProjectileType::findByName(str); projectileType != ProjectileType::none()) {
		return projectileType;
	}
	parseError.emplace(fmt::format("Invalid {} \"{}\".", name, str));
	return ProjectileType::none();
}

[[nodiscard]] inline auto parseWeapon(ParseError& parseError, std::string_view str, std::string_view name) -> Weapon {
	if (parseError) {
		return Weapon::none();
	}
	if (const auto weapon = Weapon::findByName(str); weapon != Weapon::none()) {
		return weapon;
	}
	parseError.emplace(fmt::format("Invalid {} \"{}\".", name, str));
	return Weapon::none();
}

[[nodiscard]] inline auto parseSoundId(ParseError& parseError, std::string_view str, std::string_view name) -> SoundId {
	if (parseError) {
		return SoundId::none();
	}
	if (const auto soundId = SoundId::findByFilename(str); soundId != SoundId::none()) {
		return soundId;
	}
	parseError.emplace(fmt::format("Invalid {} \"{}\".", name, str));
	return SoundId::none();
}

[[nodiscard]] inline auto parseIpAddress(ParseError& parseError, std::string_view str, std::string_view name) -> net::IpAddress {
	if (parseError) {
		return net::IpAddress{};
	}
	auto ec = std::error_code{};
	const auto ip = net::IpAddress::parse(str, ec);
	if (ec) {
		parseError.emplace(fmt::format("Invalid {} \"{}\": {}", name, str, ec.message()));
		return net::IpAddress{};
	}
	return ip;
}

[[nodiscard]] inline auto parseIpEndpoint(ParseError& parseError, std::string_view str, std::string_view name) -> net::IpEndpoint {
	if (parseError) {
		return net::IpEndpoint{};
	}
	auto ec = std::error_code{};
	const auto endpoint = net::IpEndpoint::parse(str, ec);
	if (ec) {
		parseError.emplace(fmt::format("Invalid {} \"{}\": {}", name, str, ec.message()));
		return net::IpEndpoint{};
	}
	return endpoint;
}

inline auto formatColorName(Color color) -> std::string {
	return std::string{color.getName()};
}

inline auto formatColorCodeName(Color color) -> std::string {
	return std::string{color.getCodeName()};
}

inline auto formatBannedPlayerIpAddress(const GameServer::BannedPlayers::value_type& kv) -> std::string {
	return std::string{kv.first};
}

inline auto formatIpAddress(net::IpAddress address) -> std::string {
	return std::string{address};
}

inline auto formatIpEndpoint(net::IpEndpoint endpoint) -> std::string {
	return std::string{endpoint};
}

constexpr auto isValidTeam(Team team) noexcept -> bool {
	return team != Team::none();
}

inline auto formatTeam(Team team) -> std::string {
	return util::toLower(team.getName());
}

inline auto formatTeamId(Team team) -> std::string {
	return util::toString(team.getId());
}

constexpr auto isValidPlayerClass(PlayerClass playerClass) noexcept -> bool {
	return playerClass != PlayerClass::none();
}

inline auto formatPlayerClass(PlayerClass playerClass) -> std::string {
	return util::toLower(playerClass.getName());
}

inline auto formatPlayerClassId(PlayerClass playerClass) -> std::string {
	return util::toString(playerClass.getId());
}

constexpr auto isValidProjectileType(ProjectileType projectileType) noexcept -> bool {
	return projectileType != ProjectileType::none();
}

inline auto formatProjectileType(ProjectileType projectileType) -> std::string {
	return util::toLower(projectileType.getName());
}

inline auto formatProjectileTypeId(ProjectileType projectileType) -> std::string {
	return util::toString(projectileType.getId());
}

constexpr auto isValidWeapon(Weapon weapon) noexcept -> bool {
	return weapon != Weapon::none();
}

inline auto formatWeapon(Weapon weapon) -> std::string {
	return util::toLower(weapon.getName());
}

inline auto formatWeaponId(Weapon weapon) -> std::string {
	return util::toString(weapon.getId());
}

constexpr auto isValidSoundId(SoundId soundId) noexcept -> bool {
	return soundId != SoundId::none();
}

constexpr auto formatSoundIdFilename(SoundId soundId) noexcept -> std::string_view {
	return soundId.getFilename();
}

inline auto formatPlayerId(PlayerId id) -> std::string {
	return util::toString(id);
}

inline auto formatProjectileId(World::ProjectileId id) -> std::string {
	return util::toString(id);
}

inline auto formatExplosionId(World::ExplosionId id) -> std::string {
	return util::toString(id);
}

inline auto formatSentryGunId(World::SentryGunId id) -> std::string {
	return util::toString(id);
}

inline auto formatMedkitId(World::MedkitId id) -> std::string {
	return util::toString(id);
}

inline auto formatAmmopackId(World::AmmopackId id) -> std::string {
	return util::toString(id);
}

inline auto formatGenericEntityId(World::GenericEntityId id) -> std::string {
	return util::toString(id);
}

inline auto formatFlagId(World::FlagId id) -> std::string {
	return util::toString(id);
}

inline auto formatPayloadCartId(World::PayloadCartId id) -> std::string {
	return util::toString(id);
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestColor) {
	if (i == INDEX) {
		return Color::getAll() | util::transform(cmd::formatColorCodeName) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestPlayerName) {
	if (i == INDEX && server) {
		const auto getPlayerName = [&](PlayerId id) {
			const auto& player = server->world().findPlayer(id);
			assert(player);
			return std::string{player.getName()};
		};
		return server->world().getAllPlayerIds() | util::transform(getPlayerName) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestConnectedClientIp) {
	if (i == INDEX && server) {
		return server->getConnectedClientIps() | util::transform(cmd::formatIpEndpoint) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestBannedPlayerIpAddress) {
	if (i == INDEX && server) {
		return server->getBannedPlayers() | util::transform(cmd::formatBannedPlayerIpAddress) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestTeam) {
	if (i == INDEX) {
		return Team::getAll() | util::transform(cmd::formatTeam) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidTeam) {
	if (i == INDEX) {
		return Team::getAll() | util::filter(cmd::isValidTeam) | util::transform(cmd::formatTeam) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestTeamId) {
	if (i == INDEX) {
		return Team::getAll() | util::transform(cmd::formatTeamId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidTeamId) {
	if (i == INDEX) {
		return Team::getAll() | util::filter(cmd::isValidTeam) | util::transform(cmd::formatTeamId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestPlayerClass) {
	if (i == INDEX) {
		return PlayerClass::getAll() | util::transform(cmd::formatPlayerClass) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidPlayerClass) {
	if (i == INDEX) {
		return PlayerClass::getAll() | util::filter(cmd::isValidPlayerClass) | util::transform(cmd::formatPlayerClass) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestPlayerClassId) {
	if (i == INDEX) {
		return PlayerClass::getAll() | util::transform(cmd::formatPlayerClassId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidPlayerClassId) {
	if (i == INDEX) {
		return PlayerClass::getAll() | util::filter(cmd::isValidPlayerClass) | util::transform(cmd::formatPlayerClassId) |
			   util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestProjectileType) {
	if (i == INDEX) {
		return ProjectileType::getAll() | util::transform(cmd::formatProjectileType) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidProjectileType) {
	if (i == INDEX) {
		return ProjectileType::getAll() | util::filter(cmd::isValidProjectileType) | util::transform(cmd::formatProjectileType) |
			   util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestProjectileTypeId) {
	if (i == INDEX) {
		return ProjectileType::getAll() | util::transform(cmd::formatProjectileTypeId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidProjectileTypeId) {
	if (i == INDEX) {
		return ProjectileType::getAll() | util::filter(cmd::isValidProjectileType) | util::transform(cmd::formatProjectileTypeId) |
			   util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestWeapon) {
	if (i == INDEX) {
		return Weapon::getAll() | util::transform(cmd::formatWeapon) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidWeapon) {
	if (i == INDEX) {
		return Weapon::getAll() | util::filter(cmd::isValidWeapon) | util::transform(cmd::formatWeapon) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestWeaponId) {
	if (i == INDEX) {
		return Weapon::getAll() | util::transform(cmd::formatWeaponId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidWeaponId) {
	if (i == INDEX) {
		return Weapon::getAll() | util::filter(cmd::isValidWeapon) | util::transform(cmd::formatWeapon) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestValidSoundFilename) {
	if (i == INDEX) {
		return SoundId::getAll() | util::filter(cmd::isValidSoundId) | util::transform(cmd::formatSoundIdFilename) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestPlayerId) {
	if (i == INDEX && server) {
		return server->world().getAllPlayerIds() | util::transform(cmd::formatPlayerId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestProjectileId) {
	if (i == INDEX && server) {
		return server->world().getAllProjectileIds() | util::transform(cmd::formatProjectileId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestExplosionId) {
	if (i == INDEX && server) {
		return server->world().getAllExplosionIds() | util::transform(cmd::formatExplosionId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestSentryGunId) {
	if (i == INDEX && server) {
		return server->world().getAllSentryGunIds() | util::transform(cmd::formatSentryGunId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestMedkitId) {
	if (i == INDEX && server) {
		return server->world().getAllMedkitIds() | util::transform(cmd::formatMedkitId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestAmmopackId) {
	if (i == INDEX && server) {
		return server->world().getAllAmmopackIds() | util::transform(cmd::formatAmmopackId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestGenericEntityId) {
	if (i == INDEX && server) {
		return server->world().getAllGenericEntityIds() | util::transform(cmd::formatGenericEntityId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestFlagId) {
	if (i == INDEX && server) {
		return server->world().getAllFlagIds() | util::transform(cmd::formatFlagId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

template <std::size_t INDEX>
inline SUGGESTIONS(suggestPayloadCartId) {
	if (i == INDEX && server) {
		return server->world().getAllPayloadCartIds() | util::transform(cmd::formatPayloadCartId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

} // namespace cmd

#endif
