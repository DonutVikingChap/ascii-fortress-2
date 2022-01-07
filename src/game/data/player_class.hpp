#ifndef AF2_DATA_PLAYER_CLASS_HPP
#define AF2_DATA_PLAYER_CLASS_HPP

#include "../../debug.hpp" // Msg, DEBUG_MSG
#include "data_type.hpp"   // DataType
#include "health.hpp"      // Health
#include "weapon.hpp"      // Weapon

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <functional>  // std::hash<PlayerClass::ValueType>
#include <string_view> // std::string_view

// clang-format off
// X(name, str, script, primaryWeapon, secondaryWeapon)
#define ENUM_PLAYERCLASSES(X) \
	X(none,			"",				"",							Weapon::none(),					Weapon::none()) \
	X(scout,		"Scout",		"playerclasses/scout",		Weapon::scattergun(),			Weapon::none()) \
	X(soldier,		"Soldier",		"playerclasses/soldier",	Weapon::rocket_launcher(),		Weapon::shotgun()) \
	X(pyro,			"Pyro",			"playerclasses/pyro",		Weapon::flame_thrower(),		Weapon::none()) \
	X(demoman,		"Demoman",		"playerclasses/demoman",	Weapon::stickybomb_launcher(),	Weapon::sticky_detonator()) \
	X(heavy,		"Heavy",		"playerclasses/heavy",		Weapon::minigun(),				Weapon::none()) \
	X(engineer,		"Engineer",		"playerclasses/engineer",	Weapon::shotgun(),				Weapon::build_tool()) \
	X(medic,		"Medic",		"playerclasses/medic",		Weapon::medi_gun(),				Weapon::syringe_gun()) \
	X(sniper,		"Sniper",		"playerclasses/sniper",		Weapon::sniper_rifle(),			Weapon::none()) \
	X(spy,			"Spy",			"playerclasses/spy",		Weapon::knife(),				Weapon::disguise_kit()) \
	X(spectator,	"Spectator",	"",							Weapon::none(),					Weapon::none())
// clang-format on

class PlayerClass final : public DataType<PlayerClass, std::uint8_t> {
private:
	using DataType::DataType;

	static constexpr auto NAMES = std::array{
#define X(name, str, script, primaryWeapon, secondaryWeapon) std::string_view{str},
		ENUM_PLAYERCLASSES(X)
#undef X
	};

	static constexpr auto PRIMARY_WEAPONS = std::array{
#define X(name, str, script, primaryWeapon, secondaryWeapon) primaryWeapon,
		ENUM_PLAYERCLASSES(X)
#undef X
	};

	static constexpr auto SECONDARY_WEAPONS = std::array{
#define X(name, str, script, primaryWeapon, secondaryWeapon) secondaryWeapon,
		ENUM_PLAYERCLASSES(X)
#undef X
	};

	static constexpr auto SCRIPT_PATHS = std::array{
#define X(name, str, script, primaryWeapon, secondaryWeapon) std::string_view{script},
		ENUM_PLAYERCLASSES(X)
#undef X
	};

public:
	enum class Index : ValueType {
#define X(name, str, script, primaryWeapon, secondaryWeapon) name,
		ENUM_PLAYERCLASSES(X)
#undef X
	};

	constexpr operator Index() const noexcept {
		return static_cast<Index>(m_value);
	}

#define X(name, str, script, primaryWeapon, secondaryWeapon) \
	[[nodiscard]] static constexpr auto name() noexcept { \
		return PlayerClass{static_cast<ValueType>(Index::name)}; \
	}
	ENUM_PLAYERCLASSES(X)
#undef X

	constexpr PlayerClass() noexcept
		: PlayerClass(PlayerClass::none()) {}

	[[nodiscard]] static constexpr auto getAll() noexcept {
		return std::array{
#define X(name, str, script, primaryWeapon, secondaryWeapon) PlayerClass::name(),
			ENUM_PLAYERCLASSES(X)
#undef X
		};
	}

	[[nodiscard]] constexpr auto getName() const noexcept {
		return NAMES[m_value];
	}

	[[nodiscard]] constexpr auto getPrimaryWeapon() const noexcept {
		return PRIMARY_WEAPONS[m_value];
	}

	[[nodiscard]] constexpr auto getSecondaryWeapon() const noexcept {
		return SECONDARY_WEAPONS[m_value];
	}

	[[nodiscard]] constexpr auto getScriptPath() const noexcept {
		return SCRIPT_PATHS[m_value];
	}

	[[nodiscard]] constexpr auto getId() const noexcept {
		return m_value;
	}

	[[nodiscard]] auto getGun() const noexcept -> std::string_view;
	[[nodiscard]] auto getHealth() const noexcept -> Health;
	[[nodiscard]] auto getMoveInterval() const noexcept -> float;
	[[nodiscard]] auto getLimit() const noexcept -> std::size_t;

	[[nodiscard]] static auto findByName(std::string_view inputName) noexcept -> PlayerClass;
	[[nodiscard]] static auto findById(ValueType id) noexcept -> PlayerClass;

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const PlayerClass& val) -> Stream& {
		return stream << val.m_value;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, PlayerClass& val) -> Stream& {
		constexpr auto size = PlayerClass::getAll().size();
		if (!(stream >> val.m_value) || val.m_value >= size) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Read invalid PlayerClass value \"{}\".", val.m_value);
			val.m_value = 0;
			stream.invalidate();
		}
		return stream;
	}
};

#ifndef SHARED_PLAYER_CLASS_KEEP_ENUM_PLAYERCLASSES
#undef ENUM_PLAYERCLASSES
#endif

template <>
class std::hash<PlayerClass> {
public:
	auto operator()(const PlayerClass& playerClass) const -> std::size_t {
		return m_hasher(playerClass.value());
	}

private:
	std::hash<PlayerClass::ValueType> m_hasher{};
};

#endif
