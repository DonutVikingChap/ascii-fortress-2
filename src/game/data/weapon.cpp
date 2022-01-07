#define SHARED_DATA_WEAPON_KEEP_ENUM_WEAPON_TYPES
#include "weapon.hpp"

#include "../../console/convar.hpp"   // ConVar...
#include "../../utilities/string.hpp" // util::ifind, util::stringTo

#include <fmt/core.h> // fmt::format

// clang-format off
// X(name, ammoPerShot, ammoPerClip, damage, shootInterval, reloadDelay)
#define ENUM_WEAPON_STATS( X ) \
	X(scattergun,			1,		6,		50,		0.7f,		0.7f) \
	X(rocket_launcher,		1,		4,		150,	0.8f,		0.8f) \
	X(flame_thrower,		2,		200,	40,		0.1f,		1.0f) \
	X(stickybomb_launcher,	1,		8,		150,	0.6f,		0.8f) \
	X(minigun,				2,		200,	30,		0.133333f,	1.0f) \
	X(shotgun,				1,		6,		45,		0.7f,		1.0f) \
	X(syringe_gun,			1,		40,		15,		0.12f,		0.7f) \
	X(sniper_rifle,			1,		1,		150,	2.0f,		0.0f) \
	X(knife,				1,		1,		500,	2.0f,		0.0f) \
	X(build_tool,			130,	200,	0,		1.0f,		9.0f) \
	X(medi_gun,				1,		1,		-50,	0.166667f,	0.0f) \
	X(disguise_kit,			1,		1,		0,		1.0f,		0.0f) \
	X(sentry_gun,			1,		1,		40,		0.2f,		0.0f) \
	X(sticky_detonator,		1,		1,		0,		0.001f,		0.0f)
// clang-format on

#define STRINGIFY(x) #x

// clang-format off
#define X(name, ammoPerShot, ammoPerClip, damage, shootInterval, reloadDelay) \
	ConVarIntMinMax mp_weapon_ammo_per_shot_##name{		STRINGIFY(mp_weapon_ammo_per_shot_##name),	ammoPerShot,	ConVar::SHARED_VARIABLE, fmt::format("Ammo consumed per shot by a {}.", Weapon::name().getName()), 0, -1}; \
	ConVarIntMinMax mp_weapon_ammo_per_clip_##name{		STRINGIFY(mp_weapon_ammo_per_clip_##name),	ammoPerClip,	ConVar::SHARED_VARIABLE, fmt::format("Ammo per clip in a {}.", Weapon::name().getName()), 0, -1}; \
	ConVarInt mp_weapon_damage_##name{					STRINGIFY(mp_weapon_damage_##name),			damage,			ConVar::SHARED_VARIABLE, fmt::format("Damage dealt by a {}.", Weapon::name().getName())}; \
	ConVarFloatMinMax mp_weapon_shoot_interval_##name{	STRINGIFY(mp_weapon_shoot_interval_##name),	shootInterval,	ConVar::SHARED_VARIABLE, fmt::format("Time needed between {} shots.", Weapon::name().getName()), 0.0f, -1.0f}; \
	ConVarFloatMinMax mp_weapon_reload_delay_##name{	STRINGIFY(mp_weapon_reload_delay_##name),	reloadDelay,	ConVar::SHARED_VARIABLE, fmt::format("Delay before a {} can reload after shooting.", Weapon::name().getName()), 0.0f, -1.0f};
// clang-format on

ENUM_WEAPON_STATS(X)

#undef X

auto Weapon::getAmmoPerShot() const noexcept -> Ammo {
	switch (*this) {
#define X(name, ammoPerShot, ammoPerClip, damage, shootInterval, reloadDelay) \
	case Weapon::name(): return static_cast<Ammo>(mp_weapon_ammo_per_shot_##name);
		ENUM_WEAPON_STATS(X)
#undef X
		case Weapon::none(): break;
	}
	return 0;
}

auto Weapon::getAmmoPerClip() const noexcept -> Ammo {
	switch (*this) {
#define X(name, ammoPerShot, ammoPerClip, damage, shootInterval, reloadDelay) \
	case Weapon::name(): return static_cast<Ammo>(mp_weapon_ammo_per_clip_##name);
		ENUM_WEAPON_STATS(X)
#undef X
		case Weapon::none(): break;
	}
	return 0;
}

auto Weapon::getDamage() const noexcept -> Health {
	switch (*this) {
#define X(name, ammoPerShot, ammoPerClip, damage, shootInterval, reloadDelay) \
	case Weapon::name(): return static_cast<Health>(mp_weapon_damage_##name);
		ENUM_WEAPON_STATS(X)
#undef X
		case Weapon::none(): break;
	}
	return 0;
}

auto Weapon::getShootInterval() const noexcept -> float {
	switch (*this) {
#define X(name, ammoPerShot, ammoPerClip, damage, shootInterval, reloadDelay) \
	case Weapon::name(): return mp_weapon_shoot_interval_##name;
		ENUM_WEAPON_STATS(X)
#undef X
		case Weapon::none(): break;
	}
	return 0.0f;
}

auto Weapon::getReloadDelay() const noexcept -> float {
	switch (*this) {
#define X(name, ammoPerShot, ammoPerClip, damage, shootInterval, reloadDelay) \
	case Weapon::name(): return mp_weapon_reload_delay_##name;
		ENUM_WEAPON_STATS(X)
#undef X
		case Weapon::none(): break;
	}
	return 0.0f;
}

auto Weapon::findByName(std::string_view inputName) noexcept -> Weapon {
	if (inputName.empty()) {
		return Weapon::none();
	}

	if (const auto id = util::stringTo<ValueType>(inputName)) {
		return Weapon::findById(*id);
	}

#define X(name, str, projectileType, shootSound, hurtSound, reloadSound) \
	if constexpr (Weapon::name() != Weapon::none()) { \
		if (util::ifind(std::string_view{#name}, inputName) == 0) { \
			return Weapon::name(); \
		} \
		if (util::ifind(std::string_view{str}, inputName) == 0) { \
			return Weapon::name(); \
		} \
	}
	ENUM_WEAPON_TYPES(X)
#undef X

	return Weapon::none();
}

auto Weapon::findById(ValueType id) noexcept -> Weapon {
	constexpr auto size = Weapon::getAll().size();
	if (id >= size) {
		return Weapon::none();
	}
	return Weapon{id};
}
