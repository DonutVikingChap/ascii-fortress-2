#include "virtual_machine_commands.hpp"

#include "../../utilities/string.hpp" // util::contains
#include "../call_frame_handle.hpp"   // CallFrameHandle
#include "../command.hpp"             // cmd::...
#include "../command_utilities.hpp"   // cmd::...
#include "../convar.hpp"              // ConVar...
#include "../process.hpp"             // Process
#include "../suggestions.hpp"         // Suggestions
#include "../virtual_machine.hpp"     // VirtualMachine

#include <any> // std::any, std::any_cast

CON_COMMAND(current_time, "", ConCommand::NO_FLAGS, "Get the current timestamp in milliseconds.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(vm.getTime() * 1000.0f);
}

CON_COMMAND(random_int, "<min> <max>", ConCommand::NO_FLAGS, "Generate a random integer in the range [min, max].", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto min = cmd::parseNumber<int>(parseError, argv[1], "min value");
	const auto max = cmd::parseNumber<int>(parseError, argv[2], "max value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (min > max) {
		return cmd::error("{}: Minimum value must be less than or equal to the maximum value.", self.getName());
	}

	return cmd::done(vm.randomInt(min, max));
}

CON_COMMAND(random_float, "<min> <max>", ConCommand::NO_FLAGS, "Generate a random float in the range [min, max].", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto min = cmd::parseNumber<float>(parseError, argv[1], "min value");
	const auto max = cmd::parseNumber<float>(parseError, argv[2], "max value");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (min > max) {
		return cmd::error("{}: Minimum value must be less than or equal to the maximum value.", self.getName());
	}

	return cmd::done(vm.randomFloat(min, max));
}

CON_COMMAND(wait, "[frames]", ConCommand::NO_FLAGS, "Delay remaining buffered commands by one or more frames.", {}, nullptr) {
	if (frame.progress() == 0) {
		if (argv.size() > 2) {
			return cmd::error(self.getUsage());
		}

		auto nFrames = 1u;
		if (argv.size() > 1) {
			auto parseError = cmd::ParseError{};
			nFrames = cmd::parseNumber<unsigned>(parseError, argv[1], "frame count");
			if (parseError) {
				return cmd::error("{}: {}", self.getName(), *parseError);
			}
		}

		if (nFrames != 0) {
			return cmd::deferToNextFrame(nFrames);
		}
	} else {
		if (frame.progress() != 1) {
			return cmd::deferToNextFrame(frame.progress() - 1);
		}
	}
	return cmd::done();
}

CON_COMMAND(sleep, "<milliseconds>", ConCommand::NO_FLAGS, "Delay remaining commands by a given number of milliseconds.", {}, nullptr) {
	if (frame.progress() == 0) {
		if (argv.size() != 2) {
			return cmd::error(self.getUsage());
		}

		auto parseError = cmd::ParseError{};

		const auto milliseconds = cmd::parseNumber<float, cmd::NumberConstraint::NON_NEGATIVE>(parseError,
		                                                                                       argv[1],
		                                                                                       "number of milliseconds");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}

		const auto endTime = vm.getTime() * 1000.0f + milliseconds;
		data.emplace<float>(endTime);
		return cmd::deferToNextFrame(1);
	}

	if (const auto endTime = std::any_cast<float>(data); vm.getTime() * 1000.0f < endTime) {
		return cmd::deferToNextFrame(1);
	}

	return cmd::done();
}

CON_COMMAND(done, "<process>", ConCommand::NO_FLAGS, "Check if a process is done.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if ((frame.process()->getUserFlags() & Process::ADMIN) != 0) {
		if (const auto handle = vm.findProcess(id)) {
			return cmd::done(handle->done());
		}
		return cmd::done(true);
	}

	if (const auto handle = vm.findProcess(id); handle && handle->parent().lock() == frame.process()) {
		return cmd::done(handle->done());
	}
	return cmd::done(true);
}

CON_COMMAND(run, "<process>", ConCommand::NO_FLAGS, "Run one tick of a launched process.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if ((frame.process()->getUserFlags() & Process::ADMIN) != 0) {
		if (const auto handle = vm.findProcess(id)) {
			return handle->run(game, server, client, metaServer, metaClient);
		}
		return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
	}

	if (const auto handle = vm.findProcess(id); handle && handle->parent().lock() == frame.process()) {
		return handle->run(game, server, client, metaServer, metaClient);
	}
	return cmd::error("{}: Couldn't find child process \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(await, "<process>", ConCommand::NO_FLAGS, "Wait for a process to finish.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if ((frame.process()->getUserFlags() & Process::ADMIN) != 0) {
		if (const auto handle = vm.findProcess(id)) {
			return handle->await(game, server, client, metaServer, metaClient);
		}
		return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
	}

	if (const auto handle = vm.findProcess(id); handle && handle->parent().lock() == frame.process()) {
		return handle->await(game, server, client, metaServer, metaClient);
	}
	return cmd::error("{}: Couldn't find child process \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(await_unlimited, "<process>", ConCommand::ADMIN_ONLY, "Wait an unlimited number of ticks for a process to finish.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (const auto handle = vm.findProcess(id)) {
		return handle->awaitUnlimited(game, server, client, metaServer, metaClient);
	}
	return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(await_limited, "<limit> <process>", ConCommand::ADMIN_ONLY, "Wait a given number of ticks for a process to finish.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto limit = cmd::parseNumber<int>(parseError, argv[1], "limit");
	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[2], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (const auto handle = vm.findProcess(id)) {
		return handle->awaitLimited(game, server, client, metaServer, metaClient, limit);
	}
	return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[2]);
}

CON_COMMAND(global, "<command...>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Execute a command in the global environment.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.tailCall(vm.globalEnv(), argv.subCommand(1))) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(parent_id, "[process]", ConCommand::NO_FLAGS,
            "Get the parent id of this or another process. Returns an empty string if there is no parent.", {}, VirtualMachine::suggestProcessId<1>) {
	if (argv.size() != 1 && argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (argv.size() == 2) {
		if ((frame.process()->getUserFlags() & Process::ADMIN) == 0) {
			return cmd::error("{}: Only admins can get the parent id of other processes.", self.getName());
		}

		auto parseError = cmd::ParseError{};

		const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}

		if (const auto handle = vm.findProcess(id)) {
			if (const auto parent = handle->parent().lock()) {
				return cmd::done(parent->getId());
			}
			return cmd::done("");
		}
		return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
	}

	if (const auto parent = frame.process()->parent().lock()) {
		return cmd::done(parent->getId());
	}
	return cmd::done("");
}

CON_COMMAND(stop, "[process]", ConCommand::NO_FLAGS, "End one or all currently running script processes.", {}, VirtualMachine::suggestProcessId<1>) {
	if (argv.size() != 1 && argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (argv.size() == 2) {
		auto parseError = cmd::ParseError{};

		const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}

		if ((frame.process()->getUserFlags() & Process::ADMIN) != 0) {
			if (const auto handle = vm.findProcess(id)) {
				handle->end();
				return cmd::done();
			}
			return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
		}

		if (const auto handle = vm.findProcess(id); handle && handle->parent().lock() == frame.process()) {
			handle->end();
			return cmd::done();
		}
		return cmd::error("{}: Couldn't find child process \"{}\".", self.getName(), argv[1]);
	}

	if ((frame.process()->getUserFlags() & Process::ADMIN) == 0) {
		return cmd::error("{}: Only admins may end all processes.", self.getName());
	}

	vm.endAllProcesses();
	return cmd::done();
}

CON_COMMAND(ps, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "List all currently running script processes.", {}, nullptr) {
	return cmd::done(vm.processSummary());
}

CON_COMMAND(release, "<process>", ConCommand::ADMIN_ONLY, "Release a process and have it run independently in the background.", {},
            VirtualMachine::suggestProcessId<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (const auto handle = vm.findProcess(id)) {
		if (handle->release()) {
			return cmd::done();
		}
		return cmd::error("{}: Couldn't release process.", self.getName());
	}
	return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(launch, "<command...>", ConCommand::NO_FLAGS, "Launch a new child process to run in its own environment, starting next frame.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto handle = frame.process()->launchChildProcess(frame.process()->getUserFlags())) {
		if (handle->call(frame.env(), argv.subCommand(1))) {
			return cmd::done(handle->getId());
		}
		return cmd::error("{}: Stack overflow.", self.getName());
	}
	return cmd::error("{}: Couldn't launch process!", self.getName());
}
