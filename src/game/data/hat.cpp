#define SHARED_HAT_KEEP_ENUM_HATS
#include "hat.hpp"

#include "../../console/command.hpp"         // cmd::...
#include "../../console/con_command.hpp"     // ConCommand, CON_COMMAND
#include "../../console/convar.hpp"          // ConVar...
#include "../../game/server/game_server.hpp" // GameServer
#include "../../utilities/algorithm.hpp"     // util::transform, util::filter
#include "../../utilities/string.hpp"        // util::join, util::stringTo, util::ifind

// clang-format off
// X(name, dropWeight)
#define ENUM_HAT_STATS(X) \
	X(ghastly_gibus,			200.0f) \
	X(troublemakers_tossle_cap,	50.0f) \
	X(towering_pillar_of_hats,	100.0f) \
	X(scotsmans_stove_pipe,		100.0f) \
	X(glengarry_bonnet,			100.0f) \
	X(party_hat,				150.0f) \
	X(charmers_chapeau,			75.0f) \
	X(batters_helmet,			100.0f) \
	X(officers_ushanka,			200.0f) \
	X(potassium_bonnet,			50.0f) \
	X(killers_kabuto,			100.0f) \
	X(triboniophorus_tyrannus,	100.0f) \
	X(vintage_tyrolean,			100.0f) \
	X(anger,					75.0f) \
	X(modest_pile_of_hat,		200.0f) \
	X(a_rather_festive_tree,	100.0f) \
	X(boxcar_bomber,			100.0f) \
	X(ellis_cap,				100.0f) \
	X(texas_ten_gallon,			100.0f)
// clang-format on

namespace {

CONVAR_CALLBACK(updateHatDropWeights) {
	GameServer::updateHatDropWeights();
	return cmd::done();
}

} // namespace

#define STRINGIFY(x) #x

// clang-format off
#define X(name, dropWeight) \
	ConVarFloatMinMax sv_hat_drop_weight_##name{STRINGIFY(sv_hat_drop_weight_##name), dropWeight, ConVar::SERVER_VARIABLE, fmt::format("How often {} should drop compared to other hats.", Hat::name().getName()), 0.0f, -1.0f, updateHatDropWeights};
// clang-format on

ENUM_HAT_STATS(X)

#undef X

CON_COMMAND(hatlist, "", ConCommand::NO_FLAGS, "List the names of all hats.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	static constexpr auto isValidHat = [](const auto& hat) {
		return hat != Hat::none();
	};
	static constexpr auto getHatName = [](const auto& hat) {
		return hat.getName();
	};
	return cmd::done(Hat::getAll() | util::filter(isValidHat) | util::transform(getHatName) | util::join('\n'));
}

auto Hat::findByName(std::string_view inputName) noexcept -> Hat {
	if (inputName.empty()) {
		return Hat::none();
	}

	if (const auto id = util::stringTo<ValueType>(inputName)) {
		return Hat::findById(*id);
	}

#define X(name, str, ch, color) \
	if constexpr (Hat::name() != Hat::none()) { \
		if (util::ifind(std::string_view{#name}, inputName) == 0) { \
			return Hat::name(); \
		} \
		if (util::ifind(std::string_view{str}, inputName) == 0) { \
			return Hat::name(); \
		} \
	}
	ENUM_HATS(X)
#undef X

	return Hat::none();
}

auto Hat::getDropWeight() const noexcept -> float {
	switch (*this) {
#define X(name, dropWeight) \
	case Hat::name(): return sv_hat_drop_weight_##name;
		ENUM_HAT_STATS(X)
#undef X
		case Hat::none(): break;
	}
	return 0.0f;
}

auto Hat::findById(ValueType id) noexcept -> Hat {
	constexpr auto size = Hat::getAll().size();
	if (id >= size) {
		return Hat::none();
	}
	return Hat{id};
}
