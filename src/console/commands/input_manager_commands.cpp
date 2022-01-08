#include "input_manager_commands.hpp"

#include "../../game/client/input_manager.hpp" // InputManager
#include "../../game/game.hpp"                 // Game
#include "../../utilities/algorithm.hpp"       // util::collect, util::transform, util::sort
#include "../../utilities/string.hpp"          // util::join
#include "../command.hpp"                      // cmd::...
#include "../command_utilities.hpp"            // cmd::...
#include "../suggestions.hpp"                  // Suggestions, SUGGESTIONS

#include <cstddef>    // std::size_t
#include <fmt/core.h> // fmt::format
#include <string>     // std::string

namespace {

SUGGESTIONS(suggestInputAndAction) {
	static constexpr auto formatAction = [](const auto& kv) {
		return fmt::format("+{}", kv.first);
	};

	if (i == 1) {
		return Suggestions{InputManager::validInputs()};
	}
	if (i == 2) {
		return InputManager::actionMap() | util::transform(formatAction) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

SUGGESTIONS(suggestBoundInput) {
	static constexpr auto getBindInput = [](const auto& bind) {
		return bind.input;
	};

	if (i == 1) {
		return game.inputManager().getBinds() | util::transform(getBindInput) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

} // namespace

CON_COMMAND(mouse_x, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get current absolute mouse pixel X coordinate.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(game.inputManager().getMousePositionX());
}

CON_COMMAND(mouse_y, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get current absolute mouse pixel X coordinate.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(game.inputManager().getMousePositionY());
}

CON_COMMAND(joystick_axis, "<axis>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get current value [-1, 1] of an axis on a joystick.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto axisIndex = cmd::parseNumber<std::size_t>(parseError, argv[1], "axis");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(game.inputManager().getJoystickAxis(axisIndex));
}

CON_COMMAND(joystick_hat_x, "<hat>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get current X value [-1, 1] of a hat on a joystick.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto hatIndex = cmd::parseNumber<std::size_t>(parseError, argv[1], "hat");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(game.inputManager().getJoystickHatX(hatIndex));
}

CON_COMMAND(joystick_hat_y, "<hat>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get current Y value [-1, 1] of a hat on a joystick.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto hatIndex = cmd::parseNumber<std::size_t>(parseError, argv[1], "hat");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(game.inputManager().getJoystickHatY(hatIndex));
}

CON_COMMAND(joystick_info, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get info about the active joystick.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(game.inputManager().getJoystickInfo());
}

CON_COMMAND(actionlist, "", ConCommand::NO_FLAGS, "List all available in-game actions to bind.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	static constexpr auto formatAction = [](const auto& kv) {
		return fmt::format("+{}", kv.first);
	};

	return cmd::done(InputManager::actionMap() | util::transform(formatAction) | util::collect<std::vector<std::string>>() | util::sort() |
	                 util::join('\n'));
}

CON_COMMAND(inputlist, "", ConCommand::NO_FLAGS, "Get a list of all valid input names for binding keys.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(InputManager::validInputs() | util::join('\n'));
}

CON_COMMAND(bind, "<key> [command]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Bind an input to a command.", {}, suggestInputAndAction) {
	if (argv.size() == 2) {
		const auto& output = game.inputManager().getBind(argv[1]);
		return (output.empty()) ? cmd::error("{}: {} is not bound.", self.getName(), argv[1]) : cmd::done(std::string{output});
	}

	if (argv.size() == 3) {
		if (!game.inputManager().bind(argv[1], argv[2])) {
			return cmd::error("{}: Invalid input \"{}\". Try \"{}\".", self.getName(), argv[1], GET_COMMAND(inputlist).getName());
		}
		return cmd::done();
	}

	return cmd::error(
		"Usage:\n"
		"  {0} <key>: Get what command a certain key is bound to.\n"
		"  {0} <key> <command>: Bind a key to a command.",
		self.getName());
}

CON_COMMAND(unbind, "<key>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Unbind an input.", {}, suggestBoundInput) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!game.inputManager().unbind(argv[1])) {
		return cmd::error("{}: {} was not bound.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(unbindall, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Unbind all inputs.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	game.inputManager().unbindAll();
	return cmd::done();
}

CON_COMMAND(allunbound, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Check if all inputs are unbound.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(!game.inputManager().hasAnyBinds());
}

CON_COMMAND(bindlist, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "List all current keybinds.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	static constexpr auto compareBinds = [](const auto& lhs, const auto& rhs) {
		return lhs.input < rhs.input;
	};

	static constexpr auto formatBind = [](const auto& bind) {
		return fmt::format("{} {:<10} {}", GET_COMMAND(bind).getName(), bind.input, Script::escapedString(bind.output));
	};

	return cmd::done(game.inputManager().getBinds() | util::sort(compareBinds) | util::transform(formatBind) | util::join('\n'));
}
