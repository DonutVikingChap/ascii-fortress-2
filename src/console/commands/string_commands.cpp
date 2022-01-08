#include "string_commands.hpp"

#include "../../utilities/algorithm.hpp" // util::subview
#include "../../utilities/string.hpp"    // util::concat, util::iequals, util::icompare, util::ifind, util::icontains
#include "../command.hpp"                // cmd::...
#include "../command_utilities.hpp"      // cmd::...
#include "../script.hpp"                 // Script

CON_COMMAND(empty, "<x>", ConCommand::NO_FLAGS, "Check if a string is empty.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(argv[1].empty());
}

CON_COMMAND(streq, "<x> <y>", ConCommand::NO_FLAGS, "Compare equality between two strings.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(argv[1] == argv[2]);
}

CON_COMMAND(istreq, "<x> <y>", ConCommand::NO_FLAGS, "Case-insensitive version of streq.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(util::iequals(argv[1], argv[2]));
}

CON_COMMAND(strcmp, "<x> <y>", ConCommand::NO_FLAGS, "Compare two strings.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(argv[1].compare(argv[2]));
}

CON_COMMAND(istrcmp, "<x> <y>", ConCommand::NO_FLAGS, "Case-insensitive version of strcmp.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(util::icompare(argv[1], argv[2]));
}

CON_COMMAND(strfind, "<string> <substr>", ConCommand::NO_FLAGS, "Search for a substring in a string.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	const auto i = argv[1].find(argv[2]);
	return (i == std::string::npos) ? cmd::done(-1) : cmd::done(i);
}

CON_COMMAND(istrfind, "<string> <substr>", ConCommand::NO_FLAGS, "Case-insensitive version of strfind.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	const auto i = util::ifind(argv[1], argv[2]);
	return (i == std::string::npos) ? cmd::done(-1) : cmd::done(i);
}

CON_COMMAND(strcontains, "<string> <substr>", ConCommand::NO_FLAGS, "Check if a string contains a substring.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(argv[1].find(argv[2]) != std::string::npos);
}

CON_COMMAND(istrcontains, "<string> <substr>", ConCommand::NO_FLAGS, "Case-insensitive version of strcontains.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(util::icontains(argv[1], argv[2]));
}

CON_COMMAND(strlen, "<x>", ConCommand::NO_FLAGS, "Get the length of a string.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(argv[1].size());
}

CON_COMMAND(concat, "<string> <strings...>", ConCommand::NO_FLAGS, "Concatenate strings.", {}, nullptr) {
	if (argv.size() < 3) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(util::subview(argv, 1) | util::concat());
}

CON_COMMAND(substr, "<string> <index> [count]", ConCommand::NO_FLAGS,
            "Get a substring of a string, starting at index and ending at index + count (or at the end of the string, whichever comes "
            "first).",
            {}, nullptr) {
	if (argv.size() != 3 && argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto index = cmd::parseNumber<std::size_t>(parseError, argv[2], "index");

	auto count = std::string::npos;
	if (argv.size() == 4) {
		count = cmd::parseNumber<std::size_t>(parseError, argv[3], "count");
	}

	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (index > argv[1].size()) {
		return cmd::error("{}: Index out of range ({}/{}).", self.getName(), index, argv[1].size());
	}

	return cmd::done(argv[1].substr(index, count));
}

CON_COMMAND(char_at, "<string> <index>", ConCommand::NO_FLAGS, "Get the character at a certain index of a string.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto index = cmd::parseNumber<std::size_t>(parseError, argv[2], "index");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (index >= argv[1].size()) {
		return cmd::error("{}: Index out of range ({}/{}).", self.getName(), index, argv[1].size());
	}

	return cmd::done(argv[1].substr(index, 1));
}

CON_COMMAND(escaped, "<string>", ConCommand::NO_FLAGS, "Return a printable escaped version of a string.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(Script::escapedString(argv[1]));
}
