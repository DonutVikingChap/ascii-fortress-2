#include "gui_commands.hpp"

#include "../../game/data/color.hpp"       // Color
#include "../../game/data/vector.hpp"      // Vec2
#include "../../game/game.hpp"             // Game
#include "../../gui/canvas.hpp"            // gui::Canvas
#include "../../gui/layout.hpp"            // gui::GRID_SIZE_...
#include "../../utilities/algorithm.hpp"   // util::collect, util::transform, util::enumerate
#include "../../utilities/file.hpp"        // util::readFile
#include "../../utilities/match.hpp"       // util::match
#include "../../utilities/string.hpp"      // util::toString, util::join
#include "../../utilities/tile_matrix.hpp" // util::TileMatrix
#include "../command.hpp"                  // cmd::...
#include "../command_options.hpp"          // cmd::...
#include "../command_utilities.hpp"        // cmd::...
#include "../convar.hpp"                   // ConVar...
#include "../suggestions.hpp"              // Suggestions
#include "file_commands.hpp"               // data_dir, data_subdir_screens, data_subdir_downloads

#include <cstddef>    // std::size_t
#include <fmt/core.h> // fmt::format
#include <limits>     // std::numeric_limits
#include <string>     // std::string
#include <vector>     // std::vector

namespace {

auto formatId(gui::Canvas::Id id) -> std::string {
	return util::toString(id);
}

template <std::size_t INDEX>
SUGGESTIONS(suggestGuiId) {
	if (i == INDEX) {
		return game.canvas().getElementIds() | util::transform(formatId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

SUGGESTIONS(suggestGuiIds) {
	return game.canvas().getElementIds() | util::transform(formatId) | util::collect<Suggestions>();
}

} // namespace

CON_COMMAND(colorlist, "", ConCommand::NO_FLAGS, "List all available color names.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(Color::getAll() | util::transform(cmd::formatColorName) | util::join('\n'));
}

CON_COMMAND(gui_size_x, "", ConCommand::NO_FLAGS, "Get the number of columns in the GUI window.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(gui::GRID_SIZE_X);
}

CON_COMMAND(gui_size_y, "", ConCommand::NO_FLAGS, "Get the number of rows in the GUI window.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(gui::GRID_SIZE_Y);
}

CON_COMMAND(gui_is_clear, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Check if the GUI is clear.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(game.canvas().isClear());
}

CON_COMMAND(gui_clear, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Clear the GUI.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	game.canvas().clear();
	return cmd::done();
}

CON_COMMAND(gui_is_activated, "<id>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Check if an element in the GUI is activated.", {},
            suggestGuiId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().hasElement(id)) {
		return cmd::error("{}: GUI contains no element with id \"{}\".", self.getName(), argv[1]);
	}

	return cmd::done(game.canvas().isElementActivated(id));
}

CON_COMMAND(gui_activate, "<id>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Select an element in the GUI.", {}, suggestGuiId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().activateElement(id)) {
		return cmd::error("{}: Couldn't activate id \"{}\".", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_deactivate, "[id]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Deselect one or all activated elements in the GUI.", {},
            suggestGuiId<1>) {
	if (argv.empty() || argv.size() > 2) {
		return cmd::error(self.getUsage());
	}

	if (argv.size() == 2) {
		auto parseError = cmd::ParseError{};

		const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}

		if (!game.canvas().deactivateElement(id)) {
			return cmd::error("{}: Couldn't deactivate id \"{}\".", self.getName(), argv[1]);
		}
	} else {
		game.canvas().deactivate();
	}
	return cmd::done();
}

CON_COMMAND(gui_has_element, "<id>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Check if a GUI element with a certain id exists.", {},
            suggestGuiId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(game.canvas().hasElement(id));
}

CON_COMMAND(gui_remove, "<id>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Remove a GUI element.", {}, suggestGuiId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().removeElement(id)) {
		return cmd::error("{}: Couldn't remove element at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_set_text, "<id> <text>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Set the text of a GUI element.", {}, suggestGuiId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().setElementText(id, argv[2])) {
		return cmd::error("{}: Couldn't set text of element at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_get_text, "<id>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get the text of a GUI element.", {}, suggestGuiId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (const auto text = game.canvas().getElementText(id)) {
		return cmd::done(*text);
	}

	return cmd::error("{}: Couldn't get text of element at id {}.", self.getName(), argv[1]);
}

CON_COMMAND(gui_set_color, "<id> <color>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Set the color of a GUI element.", {},
            SUGGEST(suggestGuiId<1>, cmd::suggestColor<2>)) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto color = cmd::parseColor(parseError, argv[2], "color");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().setElementColor(id, color)) {
		return cmd::error("{}: Couldn't set color of element at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_get_color, "<id>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get the color of a GUI element.", {}, suggestGuiId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (const auto color = game.canvas().getElementColor(id)) {
		return cmd::done(color->getString());
	}

	return cmd::error("{}: Couldn't get color of element at id {}.", self.getName(), argv[1]);
}

CON_COMMAND(gui_set_value, "<id> <value>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Set the value of a GUI element.", {}, suggestGuiId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto value = cmd::parseNumber<float>(parseError, argv[2], "value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().setElementValue(id, value)) {
		return cmd::error("{}: Couldn't set value of element at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_get_value, "<id>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get the value of a GUI element.", {}, suggestGuiId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (const auto value = game.canvas().getElementValue(id)) {
		return cmd::done(*value);
	}

	return cmd::error("{}: Couldn't get value of element at id {}.", self.getName(), argv[1]);
}

CON_COMMAND(gui_screen_get, "<id> <x> <y> [default]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Get the character at a certain position on a screen in the GUI.", {}, suggestGuiId<1>) {
	if (argv.size() != 4 && argv.size() != 5) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<std::size_t>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<std::size_t>(parseError, argv[3], "y coordinate");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	auto defaultVal = '\0';
	if (argv.size() == 5) {
		if (argv[4].size() != 1) {
			return cmd::error("{}: Multiple default characters specified \"{}\" (should only be one).", self.getName(), argv[4]);
		}
		defaultVal = argv[4].front();
	}

	if (const auto ch = game.canvas().getScreenChar(id, x, y, defaultVal)) {
		return cmd::done(std::string{*ch});
	}

	return cmd::error("{}: Couldn't get character of element at id {}.", self.getName(), argv[1]);
}

CON_COMMAND(gui_screen_set, "<id> <x> <y> <char>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Set the character at a certain position on a screen in the GUI.", {}, suggestGuiId<1>) {
	if (argv.size() != 5) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<std::size_t>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<std::size_t>(parseError, argv[3], "y coordinate");

	if (argv[4].size() != 1) {
		return cmd::error("{}: Multiple characters specified \"{}\" (should only be one).", self.getName(), argv[4]);
	}

	if (!game.canvas().setScreenChar(id, x, y, argv[4].front())) {
		return cmd::error("{}: Couldn't set character of element at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_button, "<id> <x> <y> <w> <h> <color> <text> <script>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a button that executes a script when clicked.", {}, cmd::suggestColor<6>) {
	if (argv.size() != 9) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto w = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[4], "width");
	const auto h = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[5], "height");
	const auto color = cmd::parseColor(parseError, argv[6], "color");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().addButton(id, Vec2{x, y}, Vec2{w, h}, color, argv[7], frame.env(), frame.process(), argv[8])) {
		return cmd::error("{}: Couldn't add button at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_input, "<id> <x> <y> <w> <h> <color> <text> <script> [options...]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a text input box that controls a cvar.",
            cmd::opts(cmd::opt('p', "private", "Show text as asterisks (*)."),
                      cmd::opt('r', "replace-mode", "Start with replace mode (insert) enabled."),
                      cmd::opt('l', "length", "Maximum number of characters allowed.", cmd::OptionType::ARGUMENT_REQUIRED),
                      cmd::opt('t', "type", "Type of value (bool/char/int/float/string).", cmd::OptionType::ARGUMENT_REQUIRED)),
            cmd::suggestColor<6>) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 8) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, args[0], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, args[1], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, args[2], "y coordinate");
	const auto w = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, args[3], "width");
	const auto h = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, args[4], "height");
	const auto color = cmd::parseColor(parseError, args[5], "color");

	auto maxLength = std::numeric_limits<std::size_t>::max();
	if (const auto& lengthStr = options['l']) {
		maxLength = cmd::parseNumber<std::size_t>(parseError, *lengthStr, "length");
	} else if (const auto& typeStr = options['t']) {
		if (*typeStr == "bool" || *typeStr == "char") {
			maxLength = 1;
		}
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().addInput(id, Vec2{x, y}, Vec2{w, h}, color, std::string{args[6]}, frame.env(), frame.process(), args[7], maxLength, options['p'], options['r'])) {
		return cmd::error("{}: Couldn't add input at id {}.", self.getName(), args[0]);
	}

	return cmd::done();
}

CON_COMMAND(gui_slider, "<id> <x> <y> <w> <h> <color> <value> <delta> <script>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a slider that executes a script when the value is changed.", {}, nullptr) {
	if (argv.size() != 10) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto w = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[4], "width");
	const auto h = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[5], "height");
	const auto color = cmd::parseColor(parseError, argv[6], "color");
	const auto value = cmd::parseNumber<float>(parseError, argv[7], "value");
	const auto delta = cmd::parseNumber<float>(parseError, argv[8], "delta");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().addSlider(id, Vec2{x, y}, Vec2{w, h}, color, value, delta, frame.env(), frame.process(), argv[9])) {
		return cmd::error("{}: Couldn't add slider at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_checkbox, "<id> <x> <y> <w> <h> <color> <value> <script>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a checkbox that executes a script when changed.", {}, nullptr) {
	if (argv.size() != 9) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto w = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[4], "width");
	const auto h = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[5], "height");
	const auto color = cmd::parseColor(parseError, argv[6], "color");
	const auto value = cmd::parseBool(parseError, argv[7], "value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().addCheckbox(id, Vec2{x, y}, Vec2{w, h}, color, value, frame.env(), frame.process(), argv[8])) {
		return cmd::error("{}: Couldn't add checkbox at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_dropdown, "<id> <x> <y> <w> <h> <color> <value> <script> <alternatives...>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a dropdown menu that executes a script when modified.", {}, nullptr) {
	if (argv.size() < 10) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto w = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[4], "width");
	const auto h = cmd::parseNumber<Vec2::Length, cmd::NumberConstraint::NON_NEGATIVE>(parseError, argv[5], "height");
	const auto color = cmd::parseColor(parseError, argv[6], "color");
	const auto value = cmd::parseNumber<std::size_t>(parseError, argv[7], "value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	auto alternatives = std::vector<std::string>{};
	alternatives.reserve(argv.size() - 9);
	for (auto i = std::size_t{9}; i < argv.size(); ++i) {
		alternatives.push_back(argv[i]);
	}

	if (value >= alternatives.size()) {
		return cmd::error("{}: Value must be less than the number of alternatives ({}/{}).", self.getName(), value, alternatives.size());
	}

	if (!game.canvas().addDropdown(id, Vec2{x, y}, Vec2{w, h}, color, std::move(alternatives), value, frame.env(), frame.process(), argv[8])) {
		return cmd::error("{}: Couldn't add dropdown at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_screen, "<id> <x> <y> <w> <h> <color> [char]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a character matrix filled with a certain character.", {}, nullptr) {
	if (argv.size() != 7 && argv.size() != 8) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto w = cmd::parseNumber<std::size_t>(parseError, argv[4], "width");
	const auto h = cmd::parseNumber<std::size_t>(parseError, argv[5], "height");
	const auto color = cmd::parseColor(parseError, argv[6], "color");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	auto matrix = util::TileMatrix<char>{};
	if (argv.size() == 8) {
		if (argv[7].size() != 1) {
			return cmd::error("{}: Multiple characters specified (should only be one) \"{}\".", self.getName(), argv[7]);
		}
		matrix = util::TileMatrix<char>{w, h, argv[7].front()};
	} else {
		matrix = util::TileMatrix<char>{w, h};
	}

	if (!game.canvas().addScreen(id, Vec2{x, y}, color, std::move(matrix))) {
		return cmd::error("{}: Couldn't add screen at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_screen_matrix, "<id> <x> <y> <color> <chars>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a character matrix parsed from a screen string.", {}, nullptr) {
	if (argv.size() != 6) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto color = cmd::parseColor(parseError, argv[4], "color");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	auto matrix = util::TileMatrix<char>{argv[5]};
	if (matrix.empty()) {
		return cmd::error("{}: Failed to parse character matrix.", self.getName());
	}

	if (!game.canvas().addScreen(id, Vec2{x, y}, color, std::move(matrix))) {
		return cmd::error("{}: Couldn't add screen at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_screen_file, "<id> <x> <y> <color> <filename>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Place a character matrix loaded from a screen file.", {}, nullptr) {
	if (argv.size() != 6) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto color = cmd::parseColor(parseError, argv[4], "color");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	const auto fileSubpath = fmt::format("{}/{}", data_subdir_screens, argv[5]);
	auto buf = util::readFile(fmt::format("{}/{}", data_dir, fileSubpath));
	if (!buf) {
		buf = util::readFile(fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, fileSubpath));
		if (!buf) {
			return cmd::error("{}: Couldn't read screen file \"{}\".", self.getName(), argv[5]);
		}
	}

	if (!game.canvas().addScreen(id, Vec2{x, y}, color, util::TileMatrix<char>{*buf})) {
		return cmd::error("{}: Couldn't add screen at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_text, "<id> <x> <y> <color> <text>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Place text.", {}, nullptr) {
	if (argv.size() != 6) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, argv[1], "id");
	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "x coordinate");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "y coordinate");
	const auto color = cmd::parseColor(parseError, argv[4], "color");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (!game.canvas().addText(id, Vec2{x, y}, color, argv[5])) {
		return cmd::error("{}: Couldn't add text at id {}.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(gui_push_menu, "[ids...] [options...]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Add a menu of the elements with the given ids onto the menu stack.",
            cmd::opts(cmd::opt('x', "on_select_none", "Executed when the menu is clicked outside of any elements.", cmd::OptionType::ARGUMENT_REQUIRED),
                      cmd::opt('e', "on_escape", "Executed when escape is pressed.", cmd::OptionType::ARGUMENT_REQUIRED),
                      cmd::opt('d', "on_direction", "Executed when a direction key is pressed.", cmd::OptionType::ARGUMENT_REQUIRED),
                      cmd::opt('c', "on_click", "Executed when the menu is clicked.", cmd::OptionType::ARGUMENT_REQUIRED),
                      cmd::opt('s', "on_scroll", "Executed when the menu is scrolled.", cmd::OptionType::ARGUMENT_REQUIRED),
                      cmd::opt('h', "on_hover", "Executed when the cursor moves.", cmd::OptionType::ARGUMENT_REQUIRED)),
            suggestGuiIds) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.empty()) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{} {}", self.getName(), *error);
	}

	auto ids = std::vector<gui::Canvas::Id>{};
	for (const auto& arg : args) {
		auto parseError = cmd::ParseError{};

		const auto id = cmd::parseNumber<gui::Canvas::Id>(parseError, arg, "id");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}

		if (!game.canvas().hasElement(id)) {
			return cmd::error("{}: No element with id \"{}\".", self.getName(), arg);
		}

		ids.push_back(id);
	}

	const auto onSelectNone = options['x'];
	const auto onEscape = options['e'];
	const auto onDirection = options['d'];
	const auto onClick = options['c'];
	const auto onScroll = options['s'];
	const auto onHover = options['h'];

	if (!game.canvas().pushMenu(ids,
	                            frame.env(),
	                            frame.process(),
	                            onSelectNone.valueOrEmpty(),
	                            onEscape.valueOrEmpty(),
	                            onDirection.valueOrEmpty(),
	                            onClick.valueOrEmpty(),
	                            onScroll.valueOrEmpty(),
	                            onHover.valueOrEmpty())) {
		return cmd::error("{}: Couldn't add menu.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(gui_pop_menu, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Remove the last added menu from the menu stack.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	if (!game.canvas().popMenu()) {
		return cmd::error("{}: Couldn't remove menu.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(gui_dump, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get a formatted summary of everything currently in the GUI.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	static constexpr auto formatElementInfo = [](const auto& elementInfo) {
		return util::match(elementInfo)(
			[](const gui::Canvas::ButtonInfoView& buttonInfo) {
				return fmt::format("{:>2}: button   (x={:>2}, y={:>2}, w={:>2}, h={:>2}, color={}, text=\"{}\"){}",
			                       buttonInfo.id,
			                       buttonInfo.position.x,
			                       buttonInfo.position.y,
			                       buttonInfo.size.x,
			                       buttonInfo.size.y,
			                       buttonInfo.color.getString(),
			                       buttonInfo.text,
			                       (buttonInfo.activated) ? " (activated)" : "");
			},
			[](const gui::Canvas::InputInfoView& inputInfo) {
				return fmt::format("{:>2}: input    (x={:>2}, y={:>2}, w={:>2}, h={:>2}, color={}, text=\"{}\"){}",
			                       inputInfo.id,
			                       inputInfo.position.x,
			                       inputInfo.position.y,
			                       inputInfo.size.x,
			                       inputInfo.size.y,
			                       inputInfo.color.getString(),
			                       inputInfo.text,
			                       (inputInfo.activated) ? " (activated)" : "");
			},
			[](const gui::Canvas::SliderInfoView& sliderInfo) {
				return fmt::format("{:>2}: slider   (x={:>2}, y={:>2}, w={:>2}, h={:>2}, color={}){}",
			                       sliderInfo.id,
			                       sliderInfo.position.x,
			                       sliderInfo.position.y,
			                       sliderInfo.size.x,
			                       sliderInfo.size.y,
			                       sliderInfo.color.getString(),
			                       (sliderInfo.activated) ? " (activated)" : "");
			},
			[](const gui::Canvas::CheckboxInfoView& checkboxInfo) {
				return fmt::format("{:>2}: checkbox (x={:>2}, y={:>2}, w={:>2}, h={:>2}, color={}){}",
			                       checkboxInfo.id,
			                       checkboxInfo.position.x,
			                       checkboxInfo.position.y,
			                       checkboxInfo.size.x,
			                       checkboxInfo.size.y,
			                       checkboxInfo.color.getString(),
			                       (checkboxInfo.activated) ? " (activated)" : "");
			},
			[](const gui::Canvas::DropdownInfoView& dropdownInfo) {
				return fmt::format("{:>2}: dropdown (x={:>2}, y={:>2}, w={:>2}, h={:>2}, color={}){}",
			                       dropdownInfo.id,
			                       dropdownInfo.position.x,
			                       dropdownInfo.position.y,
			                       dropdownInfo.size.x,
			                       dropdownInfo.size.y,
			                       dropdownInfo.color.getString(),
			                       (dropdownInfo.activated) ? " (activated)" : "");
			},
			[](const gui::Canvas::ScreenInfoView& screenInfo) {
				return fmt::format("{:>2}: screen   (x={:>2}, y={:>2}, w={:>2}, h={:>2}, color={})",
			                       screenInfo.id,
			                       screenInfo.position.x,
			                       screenInfo.position.y,
			                       screenInfo.size.x,
			                       screenInfo.size.y,
			                       screenInfo.color.getString());
			},
			[](const gui::Canvas::TextInfoView& textInfo) {
				return fmt::format("{:>2}: text     (x={:>2}, y={:>2},             color={}, text=\"{}\")",
			                       textInfo.id,
			                       textInfo.position.x,
			                       textInfo.position.y,
			                       textInfo.color.getString(),
			                       textInfo.text);
			});
	};

	static constexpr auto formatMenuInfo = [](const auto& iv) {
		const auto& [i, menuInfo] = iv;
		return fmt::format("menu[{}]: {:<14} (active id: {}){}",
		                   i,
		                   menuInfo.ids | util::transform([](const auto& id) { return util::toString(id); }) | util::join(' '),
		                   (menuInfo.activeId) ? util::toString(*menuInfo.activeId) : "none",
		                   (menuInfo.activated) ? " (activated)" : "");
	};

	auto elements = game.canvas().getElementInfo() | util::transform(formatElementInfo) | util::join('\n');
	if (const auto menus = game.canvas().getMenuInfo() | util::enumerate() | util::transform(formatMenuInfo) | util::join('\n'); !menus.empty()) {
		return cmd::done("{}\n{}", elements, menus);
	}
	return cmd::done(std::move(elements));
}
