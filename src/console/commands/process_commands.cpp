#include "process_commands.hpp"

#include "../../utilities/algorithm.hpp" // util::subview
#include "../../utilities/file.hpp"      // util::readFile
#include "../../utilities/string.hpp"    // util::join, util::contains, util::stringTo, util::toString
#include "../command.hpp"                // cmd::...
#include "../command_utilities.hpp"      // cmd::...
#include "../process.hpp"                // Process, CallFrameHandle
#include "../suggestions.hpp"            // Suggestions
#include "file_commands.hpp"             // data_dir, data_subdir_cfg, data_subdir_downloads

#include <any>        // std::any, std::any_cast
#include <cassert>    // assert
#include <fmt/core.h> // fmt::format

// clang-format off
ConVarIntMinMax	await_limit{"await_limit",	10000,	ConVar::WRITE_ADMIN_ONLY | ConVar::NO_RCON_WRITE,	"Default await limit.", 0, -1};
#ifdef NDEBUG
ConVarBool 		cvar_debug{	"debug",		false,	ConVar::WRITE_ADMIN_ONLY | ConVar::NO_RCON,			"Debug mode."};
#else
ConVarBool		cvar_debug{	"debug",		true,	ConVar::WRITE_ADMIN_ONLY | ConVar::NO_RCON,			"Debug mode."};
#endif
// clang-format on

CON_COMMAND(void, "", ConCommand::NO_FLAGS, "Return nothing.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done();
}

CON_COMMAND(return, "[value]", ConCommand::NO_FLAGS, "Return the argument and exit the current function.", {}, nullptr) {
	if (argv.size() == 2) {
		return cmd::returned(util::subview(argv, 1) | util::join(';'));
	}
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::returned();
}

CON_COMMAND(break, "", ConCommand::NO_FLAGS, "Break from the current loop.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::broke();
}

CON_COMMAND(continue, "", ConCommand::NO_FLAGS, "Continue the current loop.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::continued();
}

CON_COMMAND(if, "<condition> <script>", ConCommand::NO_FLAGS, "Conditionally execute script (see also: elif, else).", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (argv[1] == "0") {
		return cmd::failedCondition();
	}

	if (argv[1] != "1") {
		return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), argv[1]);
	}

	if (!frame.tailCall(std::make_shared<Environment>(frame.env()), argv[2])) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(elif, "<condition> <script>", ConCommand::NO_FLAGS,
            "Conditionally execute script if the previous condition failed (see also: if, else).", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (frame.status() != cmd::Status::CONDITION_FAILED) {
		return cmd::done();
	}

	if (argv[1] == "0") {
		return cmd::failedCondition();
	}

	if (argv[1] != "1") {
		return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), argv[1]);
	}

	if (!frame.tailCall(std::make_shared<Environment>(frame.env()), argv[2])) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(else, "<script>", ConCommand::NO_FLAGS, "Execute script if the previous condition failed (see also: if, elif).", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (frame.status() == cmd::Status::CONDITION_FAILED) {
		if (!frame.tailCall(std::make_shared<Environment>(frame.env()), argv[1])) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
	}
	return cmd::done();
}

CON_COMMAND(while, "<condition_script> <script>", ConCommand::NO_FLAGS, "Execute script while a condition holds true.", {}, nullptr) {
	struct State final {
		Script condition;
		Script body;
	};

	switch (frame.progress()) {
		case 0: {
			if (argv.size() != 3) {
				return cmd::error(self.getUsage());
			}

			auto& state = data.emplace<State>();
			state.condition = Script::parse(argv[1]);
			state.body = Script::parse(argv[2]);

			assert(frame.arguments().size() == 3);
			frame.arguments().pop_back();
			frame.arguments()[1].reset();
			if (!frame.call(1, frame.env(), state.condition)) {
				return cmd::error("{}: Stack overflow.", self.getName());
			}

			return cmd::notDone(1);
		}
		case 1: {
			assert(argv.size() == 2);
			if (argv[1] == "0") {
				return cmd::done();
			}

			if (argv[1] != "1") {
				return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), argv[1]);
			}

			const auto& state = std::any_cast<const State&>(data);
			assert(frame.arguments().size() == 2);
			frame.arguments()[1].reset();
			if (const auto bodyFrame = frame.call(1, std::make_shared<Environment>(frame.env()), state.body)) {
				bodyFrame->makeSection();
			} else {
				return cmd::error("{}: Stack overflow.", self.getName());
			}
			return cmd::notDone(2);
		}
		case 2: {
			assert(frame.arguments().size() == 2);
			if (frame.arguments()[1].status == cmd::Status::BREAK) {
				return cmd::done();
			}

			if (frame.arguments()[1].status == cmd::Status::RETURN) {
				return cmd::returned();
			}

			if (frame.arguments()[1].status == cmd::Status::RETURN_VALUE) {
				return cmd::returned(std::move(frame.arguments()[1].value));
			}

			const auto& state = std::any_cast<const State&>(data);
			frame.arguments()[1].reset();
			if (!frame.call(1, frame.env(), state.condition)) {
				return cmd::error("{}: Stack overflow.", self.getName());
			}
			return cmd::notDone(1);
		}
		default: break;
	}
	return cmd::done();
}

CON_COMMAND(for, "<parameter> <start> <end> [step] <script>", ConCommand::NO_FLAGS, "Execute script a given number of times.", {}, nullptr) {
	struct State final {
		Script body;
		cmd::Progress i{};
		cmd::Progress end{};
		cmd::Progress step{};
	};

	switch (frame.progress()) {
		case 0: {
			if (argv.size() < 5 || argv.size() > 6) {
				return cmd::error(self.getUsage());
			}

			auto& state = data.emplace<State>();
			state.body = Script::parse(argv.back());

			auto parseError = cmd::ParseError{};
			state.i = cmd::parseNumber<cmd::Progress>(parseError, argv[2], "start value");
			state.end = cmd::parseNumber<cmd::Progress>(parseError, argv[3], "end value");
			state.step = cmd::Progress{1};
			if (argv.size() == 6) {
				state.step = cmd::parseNumber<cmd::Progress, cmd::NumberConstraint::POSITIVE>(parseError, argv[4], "step value");
			}

			if (parseError) {
				return cmd::error("{}: {}", self.getName(), *parseError);
			}

			if (state.i >= state.end) {
				return cmd::done();
			}

			auto env = std::make_shared<Environment>(frame.env());
			env->objects.try_emplace(argv[1], Environment::Variable{util::toString(state.i)});
			frame.arguments().resize(3, cmd::done());
			frame.arguments()[2].reset();
			if (const auto bodyFrame = frame.call(2, std::move(env), state.body)) {
				bodyFrame->makeSection();
			} else {
				return cmd::error("{}: Stack overflow.", self.getName());
			}
			return cmd::notDone(1);
		}
		case 1: {
			assert(frame.arguments().size() == 3);
			if (frame.arguments()[2].status == cmd::Status::BREAK) {
				return cmd::done();
			}

			if (frame.arguments()[2].status == cmd::Status::RETURN) {
				return cmd::returned();
			}

			if (frame.arguments()[2].status == cmd::Status::RETURN_VALUE) {
				return cmd::returned(std::move(frame.arguments()[2].value));
			}

			auto& state = std::any_cast<State&>(data);
			state.i += state.step;
			if (state.i >= state.end) {
				return cmd::done();
			}

			auto env = std::make_shared<Environment>(frame.env());
			env->objects.try_emplace(argv[1], Environment::Variable{util::toString(state.i)});
			frame.arguments()[2].reset();
			if (const auto bodyFrame = frame.call(2, std::move(env), state.body)) {
				bodyFrame->makeSection();
			} else {
				return cmd::error("{}: Stack overflow.", self.getName());
			}
			return cmd::notDone(1);
		}
		default: break;
	}
	return cmd::done();
}

CON_COMMAND(script, "<script>", ConCommand::NO_FLAGS, "Execute a script in the current environment.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.tailCall(frame.env(), argv[1])) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(scope, "<script>", ConCommand::NO_FLAGS, "Execute a script in its own environment.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.tailCall(std::make_shared<Environment>(frame.env()), argv[1])) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(exec, "<filename>", ConCommand::NO_FLAGS, "Execute a script file in the current environment.", {}, Suggestions::suggestScriptFile<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	const auto fileSubpath = fmt::format((util::contains(argv[1], '.')) ? "{}/{}" : "{}/{}.cfg", data_subdir_cfg, argv[1]);

	auto buf = util::readFile(fmt::format("{}/{}", data_dir, fileSubpath));
	if (!buf) {
		buf = util::readFile(fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, fileSubpath));
		if (!buf) {
			return cmd::error("{}: Couldn't read \"{}\".", self.getName(), argv[1]);
		}
	}
	if (!frame.tailCall(frame.env(), *buf)) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(file, "<filename>", ConCommand::NO_FLAGS, "Execute a script file in its own environment.", {}, Suggestions::suggestScriptFile<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	const auto fileSubpath = fmt::format((util::contains(argv[1], '.')) ? "{}/{}" : "{}/{}.cfg", data_subdir_cfg, argv[1]);

	auto buf = util::readFile(fmt::format("{}/{}", data_dir, fileSubpath));
	if (!buf) {
		buf = util::readFile(fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, fileSubpath));
		if (!buf) {
			return cmd::error("{}: Couldn't read \"{}\".", self.getName(), argv[1]);
		}
	}
	if (!frame.tailCall(std::make_shared<Environment>(frame.env()), *buf)) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(import, "<command...>", ConCommand::NO_FLAGS, "Execute a command with the current environment as its export target.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (const auto importFrame = frame.tailCall(frame.env(), argv.subCommand(1))) {
		importFrame->setExportTarget(frame.env());
		return cmd::done();
	}
	return cmd::error("{}: Stack overflow.", self.getName());
}

CON_COMMAND(export, "<command...>", ConCommand::NO_FLAGS, "Execute a command with the current export target as the environment.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (auto exportTarget = frame.getExportTarget()) {
		if (!frame.tailCall(std::move(exportTarget), argv.subCommand(1))) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
		return cmd::done();
	}
	return cmd::error("{}: No export target!", self.getName());
}

CON_COMMAND(exit, "", ConCommand::NO_FLAGS, "End the current script process (see also: quit).", {}, nullptr) {
	if ((frame.process()->getUserFlags() & Process::CONSOLE) != 0) {
		return cmd::error("{}: Cannot exit the console process. Use \"quit\" to quit the game.", self.getName());
	}
	frame.process()->end();
	return cmd::done();
}

CON_COMMAND(error, "", ConCommand::NO_FLAGS, "Get the message of the last error.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	if (const auto error = frame.process()->getLatestError()) {
		return cmd::done(*error);
	}

	return cmd::error("{}: No error!", self.getName());
}

CON_COMMAND(error_clear, "", ConCommand::NO_FLAGS, "Clear the last error message.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	frame.process()->clearLatestError();
	return cmd::done();
}

CON_COMMAND(try, "<script>", ConCommand::NO_FLAGS,
            "Execute a script in its own environment, but don't end the process if there is an error.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	frame.process()->clearLatestError();
	if (const auto tryFrame = frame.tailCall(std::make_shared<Environment>(frame.env()), argv[1])) {
		tryFrame->makeTryBlock();
		return cmd::done();
	}
	return cmd::error("{}: Stack overflow.", self.getName());
}

CON_COMMAND(catch, "<script>", ConCommand::NO_FLAGS, "Execute a script in its own environment if there is an error.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (frame.process()->getLatestError()) {
		if (!frame.callDiscard(frame.env(), GET_COMMAND(error_clear))) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}

		if (!frame.tailCall(std::make_shared<Environment>(frame.env()), argv[1])) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
	}
	return cmd::done();
}

CON_COMMAND(throw, "[message...]", ConCommand::NO_FLAGS, "Raise an error. If no message is provided, the last error message is re-thrown.", {}, nullptr) {
	if (argv.size() == 1) {
		if (const auto error = frame.process()->getLatestError()) {
			return cmd::error(*error);
		}
		return cmd::error("{}: No error!", self.getName());
	}
	return cmd::error(util::subview(argv, 1) | util::join(' '));
}

CON_COMMAND(assert, "<condition> [message...]", ConCommand::NO_FLAGS, "Raise an error if a condition fails.", {}, nullptr) {
	if (frame.progress() == 0) {
		if (argv.size() < 2) {
			return cmd::error(self.getUsage());
		}

		frame.arguments().insert(frame.arguments().begin() + 1, cmd::done());
		if (!frame.call(1, frame.env(), frame.arguments()[2].value)) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
		return cmd::notDone(1);
	}

	if (argv[1] == "0") {
		if (argv.size() > 3) {
			return cmd::error("Assertion failed: {{{}}}! ({})", argv[2], util::subview(argv, 3) | util::join(' '));
		}
		return cmd::error("Assertion failed: {{{}}}!", argv[2]);
	}
	if (argv[1] != "1") {
		return cmd::error("{}: \"{}\" is not a boolean value.", self.getName(), argv[1]);
	}
	return cmd::done();
}

CON_COMMAND(breakpoint, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Breaks the debugger (in debug builds).", {}, nullptr) {
	assert(false && "Breakpoint hit!");
	return cmd::done();
}
