#include "logic_commands.hpp"

#include "../command.hpp" // cmd::...

// clang-format off
ConVarBool cvar_true{	"true",		true,	ConVar::READ_ONLY,	"Constant 1."};
ConVarBool cvar_false{	"false",	false,	ConVar::READ_ONLY,	"Constant 0."};
// clang-format on

CON_COMMAND(not, "<x>", ConCommand::NO_FLAGS, "Return 1 if x is 0, 0 if x is 1.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	if (argv[1] == "0") {
		return cmd::done(true);
	}
	if (argv[1] == "1") {
		return cmd::done(false);
	}
	return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), argv[1]);
}

CON_COMMAND(and, "<x> [y...]", ConCommand::NO_FLAGS, "Return 1 if all arguments are 1, otherwise return 0.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	for (const auto& arg : argv.subCommand(1)) {
		if (arg != "1") {
			if (arg == "0") {
				return cmd::done(false);
			}
			return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), arg);
		}
	}
	return cmd::done(true);
}

CON_COMMAND(or, "<x> [y...]", ConCommand::NO_FLAGS, "Return 1 if any of the arguments are 1, otherwise return 0.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	for (const auto& arg : argv.subCommand(1)) {
		if (arg != "0") {
			if (arg == "1") {
				return cmd::done(true);
			}
			return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), arg);
		}
	}
	return cmd::done(false);
}

CON_COMMAND(xor, "<x> [y...]", ConCommand::NO_FLAGS, "Return 1 if exactly one argument is 1, otherwise return 0.",
            {
			},
            nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	auto found = false;
	for (const auto& arg : argv.subCommand(1)) {
		if (arg != "0") {
			if (arg == "1") {
				if (found) {
					return cmd::done(false);
				}
				found = true;
			} else {
				return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), arg);
			}
		}
	}
	return cmd::done(found);
}
