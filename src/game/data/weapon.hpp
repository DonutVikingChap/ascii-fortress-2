#ifndef AF2_DATA_WEAPON_HPP
#define AF2_DATA_WEAPON_HPP

#include "../../debug.hpp"     // Msg, DEBUG_MSG
#include "ammo.hpp"            // Ammo
#include "data_type.hpp"       // DataType
#include "health.hpp"          // Health
#include "projectile_type.hpp" // ProjectileType
#include "sound_id.hpp"        // SoundId

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <functional>  // std::hash<Weapon::ValueType>
#include <string_view> // std::string_view

// clang-format off
// X(name, str, projectileType, shootSound, hurtSound, reloadSound)
#define ENUM_WEAPON_TYPES( X ) \
	X(none,					"",						ProjectileType::none(),			SoundId::none(),				SoundId::none(),				SoundId::none()) \
	X(scattergun,			"Scattergun",			ProjectileType::bullet(),		SoundId::shoot_scattergun(),	SoundId::player_hurt(),			SoundId::reload_scattergun()) \
	X(rocket_launcher,		"Rocket Launcher",		ProjectileType::rocket(),		SoundId::shoot_rocket(),		SoundId::player_hurt(),			SoundId::reload_rocket()) \
	X(flame_thrower,		"Flamethrower",			ProjectileType::flame(),		SoundId::shoot_flame(),			SoundId::player_hurt_flame(),	SoundId::none()) \
	X(stickybomb_launcher,	"Stickybomb Launcher",	ProjectileType::sticky(),		SoundId::shoot_sticky(),		SoundId::player_hurt(),			SoundId::reload_sticky()) \
	X(minigun,				"Minigun",				ProjectileType::bullet(),		SoundId::shoot_minigun(),		SoundId::player_hurt(),			SoundId::none()) \
	X(shotgun,				"Shotgun",				ProjectileType::bullet(),		SoundId::shoot_shotgun(),		SoundId::player_hurt(),			SoundId::reload_shotgun()) \
	X(syringe_gun,			"Syringe Gun",			ProjectileType::syringe(),		SoundId::shoot_syringe(),		SoundId::player_hurt(),			SoundId::none()) \
	X(sniper_rifle,			"Sniper Rifle",			ProjectileType::sniper_trail(),	SoundId::shoot_sniper(),		SoundId::player_hurt(),			SoundId::reload_sniper()) \
	X(knife,				"Knife",				ProjectileType::none(),			SoundId::none(),				SoundId::spy_kill(),			SoundId::none()) \
	X(build_tool,			"Build Tool",			ProjectileType::none(),			SoundId::sentry_build(),		SoundId::none(),				SoundId::none()) \
	X(medi_gun,				"Medi Gun",				ProjectileType::heal_beam(),	SoundId::shoot_heal_beam(),		SoundId::player_heal(),			SoundId::none()) \
	X(disguise_kit,			"Disguise Kit",			ProjectileType::none(),			SoundId::spy_disguise(),		SoundId::none(),				SoundId::none()) \
	X(sentry_gun,			"Sentry Gun",			ProjectileType::bullet(),		SoundId::shoot_sentry(),		SoundId::player_hurt(),			SoundId::none()) \
	X(sticky_detonator,		"Stickybomb Detonator",	ProjectileType::none(),			SoundId::none(),				SoundId::none(),				SoundId::none())
// clang-format on

class Weapon final : public DataType<Weapon, std::uint8_t> {
private:
	using DataType::DataType;

	static constexpr auto NAMES = std::array{
#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) std::string_view{str},
		ENUM_WEAPON_TYPES(X)
#undef X
	};

	static constexpr auto PROJECTILE_TYPES = std::array{
#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) projectileType,
		ENUM_WEAPON_TYPES(X)
#undef X
	};

	static constexpr auto SHOOT_SOUNDS = std::array{
#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) shootSound,
		ENUM_WEAPON_TYPES(X)
#undef X
	};

	static constexpr auto HURT_SOUNDS = std::array{
#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) hurtSound,
		ENUM_WEAPON_TYPES(X)
#undef X
	};

	static constexpr auto RELOAD_SOUNDS = std::array{
#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) reloadSound,
		ENUM_WEAPON_TYPES(X)
#undef X
	};

public:
	enum class Index : ValueType {
#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) name,
		ENUM_WEAPON_TYPES(X)
#undef X
	};

	constexpr operator Index() const noexcept {
		return static_cast<Index>(m_value);
	}

#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) \
	[[nodiscard]] static constexpr auto name() noexcept { \
		return Weapon{static_cast<ValueType>(Index::name)}; \
	}
	ENUM_WEAPON_TYPES(X)
#undef X

	constexpr Weapon() noexcept
		: Weapon(Weapon::none()) {}

	[[nodiscard]] static constexpr auto getAll() noexcept {
		return std::array{
#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) Weapon::name(),
			ENUM_WEAPON_TYPES(X)
#undef X
		};
	}

	[[nodiscard]] constexpr auto getName() const noexcept {
		return NAMES[m_value];
	}

	[[nodiscard]] constexpr auto getProjectileType() const noexcept {
		return PROJECTILE_TYPES[m_value];
	}

	[[nodiscard]] constexpr auto getShootSound() const noexcept {
		return SHOOT_SOUNDS[m_value];
	}

	[[nodiscard]] constexpr auto getHurtSound() const noexcept {
		return HURT_SOUNDS[m_value];
	}

	[[nodiscard]] constexpr auto getReloadSound() const noexcept {
		return RELOAD_SOUNDS[m_value];
	}

	[[nodiscard]] constexpr auto getId() const noexcept {
		return m_value;
	}

	[[nodiscard]] auto getAmmoPerShot() const noexcept -> Ammo;
	[[nodiscard]] auto getAmmoPerClip() const noexcept -> Ammo;
	[[nodiscard]] auto getDamage() const noexcept -> Health;
	[[nodiscard]] auto getShootInterval() const noexcept -> float;
	[[nodiscard]] auto getReloadDelay() const noexcept -> float;

	[[nodiscard]] static auto findByName(std::string_view inputName) noexcept -> Weapon;
	[[nodiscard]] static auto findById(ValueType id) noexcept -> Weapon;

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const Weapon& val) -> Stream& {
		return stream << val.m_value;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, Weapon& val) -> Stream& {
		constexpr auto size = Weapon::getAll().size();
		if (!(stream >> val.m_value) || val.m_value >= size) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Read invalid Weapon value \"{}\".", val.m_value);
			val.m_value = 0;
			stream.invalidate();
		}
		return stream;
	}
};

#ifndef SHARED_DATA_WEAPON_KEEP_ENUM_WEAPON_TYPES
#undef ENUM_WEAPON_TYPES
#endif

template <>
class std::hash<Weapon> {
public:
	auto operator()(const Weapon& id) const -> std::size_t {
		return m_hasher(id.value());
	}

private:
	std::hash<Weapon::ValueType> m_hasher{};
};

#endif
