#define SHARED_DATA_PROJECTILE_TYPE_KEEP_ENUM_PROJECTILE_TYPES
#include "projectile_type.hpp"

#include "../../console/convar.hpp"   // ConVar...
#include "../../utilities/string.hpp" // util::ifind, util::stringTo

#include <fmt/core.h> // fmt::format

// clang-format off
// X(name, moveInterval, disappearTime, ch)
#define ENUM_PROJECTILE_STATS(X) \
	X(bullet,		0.01666f,	0.6f,	'*') \
	X(rocket,		0.04f,		1.5f,	'o') \
	X(sticky,		0.082f,		0.7f,	'B') \
	X(flame,		0.06f,		0.5f,	'f') \
	X(heal_beam,	0.06f,		0.5f,	'+') \
	X(syringe,		0.03f,		0.7f,	'-') \
	X(sniper_trail,	0.0f,		0.1f,	'x')
// clang-format on

#define STRINGIFY(x) #x

// clang-format off
#define X(name, moveInterval, disappearTime, ch) \
	ConVarFloatMinMax mp_projectile_move_interval_##name{	STRINGIFY(mp_projectile_move_interval_##name),	moveInterval,	ConVar::SHARED_VARIABLE,	fmt::format("Time taken for a {} to move one unit.", ProjectileType::name().getName()), 0.0f, -1.0f}; \
	ConVarFloatMinMax mp_projectile_disappear_time_##name{	STRINGIFY(mp_projectile_disappear_time_##name),	disappearTime,	ConVar::SHARED_VARIABLE,	fmt::format("Time taken for a {} to disappear.", ProjectileType::name().getName()), 0.0f, -1.0f}; \
	ConVarChar cl_char_##name{								STRINGIFY(cl_char_##name),						ch,				ConVar::CLIENT_VARIABLE,	fmt::format("How to draw a {}.", ProjectileType::name().getName())};
// clang-format on

ENUM_PROJECTILE_STATS(X)

#undef X

auto ProjectileType::getChar() const noexcept -> char {
	switch (*this) {
#define X(name, moveInterval, disappearTime, ch) \
	case ProjectileType::name(): return cl_char_##name;
		ENUM_PROJECTILE_STATS(X)
#undef X
		case ProjectileType::none(): break;
	}
	return '\0';
}

auto ProjectileType::getMoveInterval() const noexcept -> float {
	switch (*this) {
#define X(name, moveInterval, disappearTime, ch) \
	case ProjectileType::name(): return mp_projectile_move_interval_##name;
		ENUM_PROJECTILE_STATS(X)
#undef X
		case ProjectileType::none(): break;
	}
	return 0.0f;
}

auto ProjectileType::getDisappearTime() const noexcept -> float {
	switch (*this) {
#define X(name, moveInterval, disappearTime, ch) \
	case ProjectileType::name(): return mp_projectile_disappear_time_##name;
		ENUM_PROJECTILE_STATS(X)
#undef X
		case ProjectileType::none(): break;
	}
	return 0.0f;
}

auto ProjectileType::findByName(std::string_view inputName) noexcept -> ProjectileType {
	if (inputName.empty()) {
		return ProjectileType::none();
	}

	if (const auto id = util::stringTo<ValueType>(inputName)) {
		return ProjectileType::findById(*id);
	}

#define X(name, str) \
	if constexpr (ProjectileType::name() != ProjectileType::none()) { \
		if (util::ifind(std::string_view{#name}, inputName) == 0) { \
			return ProjectileType::name(); \
		} \
		if (util::ifind(std::string_view{str}, inputName) == 0) { \
			return ProjectileType::name(); \
		} \
	}
	ENUM_PROJECTILE_TYPES(X)
#undef X

	return ProjectileType::none();
}

auto ProjectileType::findById(ValueType id) noexcept -> ProjectileType {
	constexpr auto size = ProjectileType::getAll().size();
	if (id >= size) {
		return ProjectileType::none();
	}
	return ProjectileType{id};
}
