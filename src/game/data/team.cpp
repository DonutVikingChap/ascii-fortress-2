#define SHARED_DATA_TEAM_KEEP_ENUM_TEAMS
#include "team.hpp"

#include "../../console/convar.hpp"   // ConVar...
#include "../../utilities/string.hpp" // util::ifind, util::stringTo

#define STRINGIFY(x) #x

// clang-format off
#define X(name, str, color) \
	ConVarInt team_##name{STRINGIFY(team_##name), Team::name().getId(), ConVar::READ_ONLY, fmt::format("Team id of team {}.", Team::name().getName())};
// clang-format on

ENUM_TEAMS(X)

#undef X

auto Team::findByName(std::string_view inputName) noexcept -> Team {
	if (inputName.empty()) {
		return Team::none();
	}

	if (const auto id = util::stringTo<ValueType>(inputName)) {
		return Team::findById(*id);
	}

#define X(name, str, color) \
	if constexpr (Team::name() != Team::none()) { \
		if (util::ifind(std::string_view{#name}, inputName) == 0) { \
			return Team::name(); \
		} \
		if (util::ifind(std::string_view{str}, inputName) == 0) { \
			return Team::name(); \
		} \
	}
	ENUM_TEAMS(X)
#undef X

	return Team::none();
}

auto Team::findById(ValueType id) noexcept -> Team {
	constexpr auto size = Team::getAll().size();
	if (id >= size) {
		return Team::none();
	}
	return Team{id};
}
