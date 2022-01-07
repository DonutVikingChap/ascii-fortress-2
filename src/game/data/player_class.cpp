#define SHARED_PLAYER_CLASS_KEEP_ENUM_PLAYERCLASSES
#include "player_class.hpp"

#include "../../console/convar.hpp"   // ConVar...
#include "../../utilities/string.hpp" // util::ifind

#include <fmt/core.h> // fmt::format
#include <limits>     // std::numeric_limits

// clang-format off
// X(name, health, moveInterval, gun)
#define ENUM_PLAYERCLASS_STATS(X) \
	X(spectator,	0,		0.06f,	"") \
	X(scout,		125,	0.11f,	"********") \
	X(soldier,		200,	0.17f,	"########") \
	X(pyro,			175,	0.16f,	"&&&&&&&&") \
	X(demoman,		175,	0.16f,	"xxxxxxxx") \
	X(heavy,		300,	0.23f,	"HHHHHHHH") \
	X(engineer,		125,	0.15f,	"eeeeeeee") \
	X(medic,		150,	0.13f,	"mmmmmmmm") \
	X(sniper,		125,	0.15f,	"\\|/_\\|/_") \
	X(spy,			125,	0.15f,	"ssssssss")
// clang-format on

#define STRINGIFY(x) #x

// clang-format off
#define X(name, health, moveInterval, gun) \
	ConVarIntMinMax   mp_class_health_##name{			STRINGIFY(mp_class_health_##name),			health,			ConVar::SHARED_VARIABLE,	fmt::format("Max health of the {}.", PlayerClass::name().getName()), 0, -1}; \
	ConVarFloatMinMax mp_class_move_interval_##name{	STRINGIFY(mp_class_move_interval_##name),	moveInterval,	ConVar::SHARED_VARIABLE,	fmt::format("Time taken for the {} to move one unit.", PlayerClass::name().getName()), 0.0f, -1.0f}; \
	ConVarIntMinMax   mp_class_limit_##name{			STRINGIFY(mp_class_limit_##name),			100,			ConVar::SHARED_VARIABLE,	fmt::format("Number of players allowed to select {} on the same team.", PlayerClass::name().getName()), 0, -1}; \
	ConVarString      cl_gun_##name{					STRINGIFY(cl_gun_##name),					gun,			ConVar::CLIENT_VARIABLE,	fmt::format("How to draw the gun for the {}.", PlayerClass::name().getName())};
// clang-format on

ENUM_PLAYERCLASS_STATS(X)

#undef X

auto PlayerClass::getGun() const noexcept -> std::string_view {
	switch (*this) {
#define X(name, health, moveInterval, gun) \
	case PlayerClass::name(): return cl_gun_##name;
		ENUM_PLAYERCLASS_STATS(X)
#undef X
		case PlayerClass::none(): break;
	}
	return "";
}

auto PlayerClass::getHealth() const noexcept -> Health {
	switch (*this) {
#define X(name, health, moveInterval, gun) \
	case PlayerClass::name(): return static_cast<Health>(mp_class_health_##name);
		ENUM_PLAYERCLASS_STATS(X)
#undef X
		case PlayerClass::none(): break;
	}
	return 0;
}

auto PlayerClass::getMoveInterval() const noexcept -> float {
	switch (*this) {
#define X(name, health, moveInterval, gun) \
	case PlayerClass::name(): return mp_class_move_interval_##name;
		ENUM_PLAYERCLASS_STATS(X)
#undef X
		case PlayerClass::none(): break;
	}
	return 0.0f;
}

auto PlayerClass::getLimit() const noexcept -> std::size_t {
	switch (*this) {
#define X(name, health, moveInterval, gun) \
	case PlayerClass::name(): return static_cast<std::size_t>(mp_class_limit_##name);
		ENUM_PLAYERCLASS_STATS(X)
#undef X
		case PlayerClass::none(): break;
	}
	return std::numeric_limits<std::size_t>::max();
}

auto PlayerClass::findByName(std::string_view inputName) noexcept -> PlayerClass {
	if (inputName.empty()) {
		return PlayerClass::none();
	}

	if (const auto id = util::stringTo<ValueType>(inputName)) {
		return PlayerClass::findById(*id);
	}

#define X(name, str, script, primaryWeapon, secondaryWeapon) \
	if constexpr (PlayerClass::name() != PlayerClass::none()) { \
		if (util::ifind(std::string_view{str}, inputName) == 0) { \
			return PlayerClass::name(); \
		} \
	}
	ENUM_PLAYERCLASSES(X)
#undef X

	return PlayerClass::none();
}

auto PlayerClass::findById(ValueType id) noexcept -> PlayerClass {
	constexpr auto size = PlayerClass::getAll().size();
	if (id >= size) {
		return PlayerClass::none();
	}
	return PlayerClass{id};
}
