#include "utility_commands.hpp"

#include "../../utilities/algorithm.hpp" // util::transform, util::subview, util::allOf, util::anyOf
#include "../../utilities/string.hpp"    // util::join, util::icontains, util::tokenize
#include "../command.hpp"                // cmd::...
#include "../command_options.hpp"        // cmd::...
#include "../convar.hpp"                 // ConVar...
#include "../process.hpp"                // Process, CallFrameHandle
#include "../suggestions.hpp"            // Suggestions, SUGGESTIONS

#include <algorithm>  // std::sort
#include <array>      // std::array
#include <fmt/core.h> // fmt::format
#include <vector>     // std::vector

namespace {

auto formatElem(const ConVar& cvar, bool flags, bool description, bool, bool limits, bool admin, bool rcon) -> std::string {
	return cvar.format(admin, rcon, limits, limits, flags, description);
}

auto formatElem(const ConCommand& cmd, bool flags, bool description, bool options, bool, bool, bool) -> std::string {
	return cmd.format(flags, description, options);
}

template <typename T>
auto makeList(const std::vector<std::string_view>& args, bool nameOnly, bool all, bool flags, bool description, bool options, bool limits,
              bool includeHidden, std::string_view countText, bool admin, bool rcon) -> std::string {
	flags = all || flags;
	description = all || description;
	options = all || options;
	limits = all || limits;

	auto elements = std::vector<const T*>{};
	if (!args.empty()) {
		for (const auto& [name, element] : T::all()) {
			if (!includeHidden && !name.empty() && name.front() == '_') {
				continue;
			}
			if (util::allOf(args, [&, &name = name, &element = element](const auto& arg) {
					return util::anyOf(arg | util::tokenize('/'), [&](const auto& alternative) {
						if (alternative.empty()) {
							return true;
						}
						if (alternative.front() != '!') {
							return util::icontains(name, alternative) || (!nameOnly && util::icontains(element.getDescription(), alternative));
						}
						const auto str = std::string_view{alternative}.substr(1);
						return !util::icontains(name, str) && (nameOnly || !util::icontains(name, str));
					});
				})) {
				elements.push_back(&element);
			}
		}
	} else {
		for (const auto& [name, element] : T::all()) {
			if (!includeHidden && !name.empty() && name.front() == '_') {
				continue;
			}
			elements.push_back(&element);
		}
	}

	std::sort(elements.begin(), elements.end(), [](const T* lhs, const T* rhs) { return lhs->getName() < rhs->getName(); });

	if (elements.empty()) {
		return fmt::format("0 {}", countText);
	}

	const auto formatListElem = [&](const T* elem) {
		if (nameOnly) {
			return fmt::format("  {}", elem->getName());
		}
		return fmt::format("  {}", formatElem(*elem, flags, description, options, limits, admin, rcon));
	};

	return fmt::format(
		"{}\n"
		"{} {}",
		elements | util::transform(formatListElem) | util::join('\n'),
		elements.size(),
		countText);
}

SUGGESTIONS(suggestCvarsAndCommands) {
	auto suggestions = Suggestions{};
	if (i == 1 && i < command.size()) {
		for (const auto& [name, cmd] : ConCommand::all()) {
			if (name.compare(0, command[i].value.size(), command[i].value) == 0) {
				suggestions.emplace_back(name);
			}
		}
		for (const auto& [name, cvar] : ConVar::all()) {
			if (name.compare(0, command[i].value.size(), command[i].value) == 0) {
				suggestions.emplace_back(name);
			}
		}
	}
	return suggestions;
}

} // namespace

CON_COMMAND(cmdlist, "[options...] [query...]", ConCommand::NO_FLAGS, "List commands.",
            cmd::opts(cmd::opt('n', "name-only", "Don't search descriptions."), cmd::opt('a', "all", "Show all info."),
                      cmd::opt('f', "flags", "Show flags."), cmd::opt('d', "description", "Show description."),
                      cmd::opt('o', "options", "Show options."),
                      cmd::opt('i', "include-hidden", "Include names beginning with an underscore.")),
            nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	return cmd::done(
		"{}\n"
		"Use \"help <name>\" for more info.",
		makeList<ConCommand>(args,
	                         options['n'],
	                         options['a'],
	                         options['f'],
	                         options['d'],
	                         options['o'],
	                         false,
	                         options['i'],
	                         "commands",
	                         (frame.process()->getUserFlags() & Process::ADMIN) != 0,
	                         (frame.process()->getUserFlags() & Process::REMOTE) != 0));
}

CON_COMMAND(cvarlist, "[options...] [query...]", ConCommand::NO_FLAGS, "List cvars.",
            cmd::opts(cmd::opt('n', "name-only", "Don't search descriptions."), cmd::opt('a', "all", "Show all info."),
                      cmd::opt('f', "flags", "Show flags."), cmd::opt('d', "description", "Show description."),
                      cmd::opt('l', "limits", "Show default and min/max values."),
                      cmd::opt('i', "include-hidden", "Include names beginning with an underscore.")),
            nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	return cmd::done(
		"{}\n"
		"Use \"help <name>\" for more info.",
		makeList<ConVar>(args,
	                     options['n'],
	                     options['a'],
	                     options['f'],
	                     options['d'],
	                     false,
	                     options['l'],
	                     options['i'],
	                     "cvars",
	                     (frame.process()->getUserFlags() & Process::ADMIN) != 0,
	                     (frame.process()->getUserFlags() & Process::REMOTE) != 0));
}

CON_COMMAND(find, "[options...] <query...>", ConCommand::NO_FLAGS, "Find commands/cvars that match a search string.",
            cmd::opts(cmd::opt('n', "name-only", "Don't search descriptions."), cmd::opt('a', "all", "Show all info."),
                      cmd::opt('f', "flags", "Show flags."), cmd::opt('d', "description", "Show description."),
                      cmd::opt('o', "options", "Show options."), cmd::opt('l', "limits", "Show default and min/max values."),
                      cmd::opt('i', "include-hidden", "Include names beginning with an underscore.")),
            nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.empty()) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	const auto nameOnly = options['n'].isSet();
	const auto all = options['a'].isSet();
	const auto flags = options['f'].isSet();
	const auto description = options['d'].isSet();
	const auto includeHidden = options['i'].isSet();

	return cmd::done(
		"{}\n"
		"{}\n"
		"Use \"help <name>\" for more info.",
		makeList<ConCommand>(args,
	                         nameOnly,
	                         all,
	                         flags,
	                         description,
	                         options['o'],
	                         false,
	                         includeHidden,
	                         "commands",
	                         (frame.process()->getUserFlags() & Process::ADMIN) != 0,
	                         (frame.process()->getUserFlags() & Process::REMOTE) != 0),
		makeList<ConVar>(args,
	                     nameOnly,
	                     all,
	                     flags,
	                     description,
	                     false,
	                     options['l'],
	                     includeHidden,
	                     "cvars",
	                     (frame.process()->getUserFlags() & Process::ADMIN) != 0,
	                     (frame.process()->getUserFlags() & Process::REMOTE) != 0));
}

CON_COMMAND(help, "[options...] [name]", ConCommand::NO_FLAGS, "Learn about the console or a command/cvar.",
            cmd::opts(cmd::opt('a', "all", "Show all info."), cmd::opt('f', "flags", "Show flags.")), suggestCvarsAndCommands) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	if (!args.empty()) {
		const auto all = options['a'].isSet();
		const auto flags = options['f'].isSet();

		if (const auto* const cmd = ConCommand::find(args[0])) {
			return cmd::done(cmd->format(all || flags, true, true));
		}

		if (const auto* const cvar = ConVar::find(args[0])) {
			return cmd::done(cvar->format((frame.process()->getUserFlags() & Process::ADMIN) != 0,
			                              (frame.process()->getUserFlags() & Process::REMOTE) != 0,
			                              true,
			                              true,
			                              all || flags,
			                              true));
		}

		if (!frame.tailCall(frame.env(), GET_COMMAND(find), std::array{cmd::Value{args[0]}})) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
		return cmd::done();
	}

	return cmd::done(
		"Welcome to the Console.\n"
		"Here are some tips to get you started:\n"
		"> {:<16} List all commands.\n"
		"> {:<16} Search for commands.\n"
		"> {:<16} Learn more about a command.\n"
		"> {:<16} Get the value of x.\n"
		"> {:<16} Set the value of x.\n"
		"Use $x, x() or (x) to evaluate command arguments.\n"
		"Use {{braces}} or \"quotes\" to put spaces in strings.\n"
		"Press TAB while typing a command to auto-complete.",
		GET_COMMAND(cmdlist).getName(),
		fmt::format("{} <name>", GET_COMMAND(find).getName()),
		"x",
		"x <value>",
		self.getName());
}

CON_COMMAND(wtf, "", ConCommand::NO_FLAGS, "Try to get help about the latest error.", {}, nullptr) {
	const auto error = frame.process()->getLatestError();
	if (!error) {
		return cmd::error("{}: No error.", self.getName());
	}

	constexpr auto UNKNOWN_COMMAND_PREFIX = std::string_view{"Unknown command: \""};
	constexpr auto USAGE_PREFIX = std::string_view{"Usage: "};

	if (const auto len = UNKNOWN_COMMAND_PREFIX.size(); error->compare(0, len, UNKNOWN_COMMAND_PREFIX) == 0) {
		return cmd::done(
			"Couldn't find an alias, object, command or cvar named \"{}\".\n"
			"Try \"{}\", \"{}\" or \"{}\".",
			error->substr(len, error->find('\"', len) - len),
			GET_COMMAND(cmdlist).getName(),
			GET_COMMAND(cvarlist).getName(),
			GET_COMMAND(find).getName());
	}

	auto offset = std::size_t{0};
	if (const auto len = USAGE_PREFIX.size(); error->compare(0, len, USAGE_PREFIX) == 0) {
		offset = len;
	}

	const auto name = error->substr(offset, error->find_first_of(": ", offset) - offset);

	if (const auto* const cmd = ConCommand::find(name)) {
		return cmd::done(cmd->format(false, true, false));
	}

	if (const auto* const cvar = ConVar::find(name)) {
		return cmd::done(
			cvar->format((frame.process()->getUserFlags() & Process::ADMIN) != 0, (frame.process()->getUserFlags() & Process::REMOTE) != 0, true, true, false, true));
	}

	return cmd::error("{}: Sorry, I got nothing.", self.getName());
}

CON_COMMAND(reset, "<cvar>", ConCommand::NO_FLAGS, "Set a cvar to its default value.", {}, Suggestions::suggestCvar<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (auto&& cvar = ConVar::find(argv[1])) {
		if (!frame.tailCall(frame.env(), *cvar, std::string{cvar->getDefaultValue()})) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
		return cmd::done();
	}
	return cmd::error("{}: Unknown cvar \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(default, "<cvar>", ConCommand::NO_FLAGS, "Get the default value of a cvar.", {}, Suggestions::suggestCvar<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const cvar = ConVar::find(argv[1])) {
		if ((cvar->getFlags() & ConVar::READ_ADMIN_ONLY) != 0 && (frame.process()->getUserFlags() & Process::ADMIN) == 0) {
			return cmd::error("{}: {} is admin-only.", self.getName(), cvar->getName());
		}
		if ((cvar->getFlags() & ConVar::NO_RCON_READ) != 0 && (frame.process()->getUserFlags() & Process::REMOTE) != 0) {
			return cmd::error("{}: {} may not be read remotely.", self.getName(), cvar->getName());
		}
		return cmd::done(cvar->getDefaultValue());
	}
	return cmd::error("{}: Unknown cvar \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(cvar_type, "<cvar>", ConCommand::NO_FLAGS, "Get the type of a cvar.", {}, Suggestions::suggestCvar<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const cvar = ConVar::find(argv[1])) {
		switch (cvar->getType()) {
			case ConVar::Type::BOOL: return cmd::done("bool");
			case ConVar::Type::CHAR: return cmd::done("char");
			case ConVar::Type::COLOR: return cmd::done("color");
			case ConVar::Type::FLOAT: return cmd::done("float");
			case ConVar::Type::HASH: return cmd::done("hash");
			case ConVar::Type::INT: return cmd::done("int");
			case ConVar::Type::STRING: return cmd::done("string");
		}
	}
	return cmd::error("{}: Unknown cvar \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(cvar_min, "<cvar>", ConCommand::NO_FLAGS, "Get the minimum value of a cvar.", {}, Suggestions::suggestCvar<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const cvar = ConVar::find(argv[1])) {
		return cmd::done(cvar->getMinValue());
	}
	return cmd::error("{}: Unknown cvar \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(cvar_max, "<cvar>", ConCommand::NO_FLAGS, "Get the maximum value of a cvar.", {}, Suggestions::suggestCvar<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const cvar = ConVar::find(argv[1])) {
		return cmd::done(cvar->getMaxValue());
	}
	return cmd::error("{}: Unknown cvar \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(get_raw, "<cvar>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get the raw value of a secret cvar (admin only).", {},
            Suggestions::suggestCvar<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const cvar = ConVar::find(argv[1])) {
		return cmd::done(std::string{cvar->getRaw()});
	}
	return cmd::error("{}: Unkown cvar \"{}\".", self.getName(), argv[1]);
}
