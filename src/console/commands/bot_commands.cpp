#include "bot_commands.hpp"

#include "../../game/server/bot.hpp" // Bot

namespace {

CONVAR_CALLBACK(updateSpyCheckProbability) {
	Bot::updateSpyCheckProbability();
	return cmd::done();
}

CONVAR_CALLBACK(updateHealthProbability) {
	Bot::updateHealthProbability();
	return cmd::done();
}

CONVAR_CALLBACK(updateClassWeights) {
	Bot::updateClassWeights();
	return cmd::done();
}

CONVAR_CALLBACK(updateGoalWeights) {
	Bot::updateGoalWeights();
	return cmd::done();
}

} // namespace

// clang-format off
ConVarFloatMinMax	bot_range_shotgun{					"bot_range_shotgun",				0.5f,	ConVar::SERVER_VARIABLE,	"Fraction of bot_range below which soldier bots use their shotgun.", 0.0f, 1.0f};
ConVarIntMinMax		bot_range{							"bot_range",						18,		ConVar::SERVER_VARIABLE,	"The radius of the circle in which bots can see enemy players.", 0, -1};
ConVarFloatMinMax	bot_defend_time{					"bot_defend_time",					4.0f,	ConVar::SERVER_VARIABLE,	"How many seconds bots wait while defending.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_heal_time{						"bot_heal_time",					2.0f,	ConVar::SERVER_VARIABLE,	"How many seconds bots spend healing.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_heal_cooldown{					"bot_heal_cooldown",				2.0f,	ConVar::SERVER_VARIABLE,	"How many seconds bots wait before healing again.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_spycheck_time{					"bot_spycheck_time",				4.5f,	ConVar::SERVER_VARIABLE,	"How many seconds bots spend spychecking.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_spycheck_cooldown{				"bot_spycheck_cooldown",			7.0f,	ConVar::SERVER_VARIABLE,	"How many seconds bots wait before spychecking again.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_spycheck_cooldown_spawn{		"bot_spycheck_cooldown_spawn",		5.0f,	ConVar::SERVER_VARIABLE,	"How many seconds bots wait before spychecking after spawning.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_spycheck_panic_time{			"bot_spycheck_panic_time",			8.0f,	ConVar::SERVER_VARIABLE,	"How many seconds bots spend spychecking after seeing an enemy spy.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_spycheck_reaction_time{			"bot_spycheck_reaction_time",		0.3f,	ConVar::SERVER_VARIABLE,	"How many seconds bots wait before spychecking a friendly.", 0.0f, -1.0f};
ConVarFloatMinMax	bot_probability_spycheck{			"bot_probability_spycheck",			0.9f,	ConVar::SERVER_VARIABLE,	"Probability [0, 1] that defines how likely a bot is to spycheck a teammate.", 0.0f, 1.0f, updateSpyCheckProbability};
ConVarFloatMinMax	bot_probability_get_health{			"bot_probability_get_health",		0.5f,	ConVar::SERVER_VARIABLE,	"Probability [0, 1] that defines how likely a bot is to get health after taking damage.", 0.0f, 1.0f, updateHealthProbability};
ConVarFloatMinMax	bot_class_weight_scout{				"bot_class_weight_scout",			2.0f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Scout.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_soldier{			"bot_class_weight_soldier",			2.0f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Soldier.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_pyro{				"bot_class_weight_pyro",			0.8f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Pyro.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_demoman{			"bot_class_weight_demoman",			0.8f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Demoman.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_heavy{				"bot_class_weight_heavy",			1.0f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Heavy.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_engineer{			"bot_class_weight_engineer",		1.0f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Engineer.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_medic{				"bot_class_weight_medic",			0.6f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Medic.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_sniper{			"bot_class_weight_sniper",			0.8f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Sniper.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_class_weight_spy{				"bot_class_weight_spy",				0.5f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to go Spy.", 0.0f, -1.0f, updateClassWeights};
ConVarFloatMinMax	bot_decision_weight_do_objective{	"bot_decision_weight_do_objective",	15.0f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to decide to do the objective.", 0.0f, -1.0f, updateGoalWeights};
ConVarFloatMinMax	bot_decision_weight_roam{			"bot_decision_weight_roam",			20.0f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to decide to roam.", 0.0f, -1.0f, updateGoalWeights};
ConVarFloatMinMax	bot_decision_weight_defend{			"bot_decision_weight_defend",		5.0f,	ConVar::SERVER_VARIABLE,	"Weight that defines how likely a bot is to choose to defend.", 0.0f, -1.0f, updateGoalWeights};
// clang-format on
