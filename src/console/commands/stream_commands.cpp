#include "stream_commands.hpp"

#include "../../utilities/algorithm.hpp" // util::subview
#include "../../utilities/string.hpp"    // util::join
#include "../command.hpp"                // cmd::...
#include "../command_utilities.hpp"      // cmd::...
#include "../process.hpp"                // Process
#include "../virtual_machine.hpp"        // VirtualMachine

CON_COMMAND(echo, "[strings...]", ConCommand::NO_FLAGS,
            "Write a line of space-separated strings to the process output, or to the virtual machine if the process has no output. Echoes "
            "piped input if no arguments are provided.",
            {}, nullptr) {
	if (argv.size() == 1) {
		auto wrote = false;
		while (auto result = frame.process()->input()->readln()) {
			if (!frame.process()->outputln(*result)) {
				vm.outputln(std::move(*result));
			}
			wrote = true;
		}

		if (!frame.process()->input()->isDone()) {
			return cmd::deferToNextFrame((wrote) ? 1 : 0);
		}

		if (frame.progress() == 0 && !wrote) {
			if (!frame.process()->outputln("")) {
				vm.outputln("");
			}
		}
		return cmd::done();
	}

	auto text = util::subview(argv, 1) | util::join(' ');
	if (!frame.process()->outputln(text)) {
		vm.outputln(std::move(text));
	}
	return cmd::done();
}

CON_COMMAND(write, "<string>", ConCommand::NO_FLAGS, "Write a string to the process output.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.process()->output(argv[1])) {
		return cmd::error("{}: Process has no output.", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(write_to, "<process> <string>", ConCommand::NO_FLAGS, "Write a string to a certain process.", {}, VirtualMachine::suggestProcessId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if ((frame.process()->getUserFlags() & Process::ADMIN) != 0) {
		if (const auto handle = vm.findProcess(id)) {
			handle->input()->write(argv[2]);
			return cmd::done();
		}
		return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
	}

	if (const auto handle = vm.findProcess(id); handle && handle->parent().lock() == frame.process()) {
		handle->input()->write(argv[2]);
		return cmd::done();
	}
	return cmd::error("{}: Couldn't find child process \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(writeln, "<string>", ConCommand::NO_FLAGS, "Write a line to the process output.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.process()->outputln(argv[1])) {
		return cmd::error("{}: Process has no output.", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(writeln_to, "<process> <string>", ConCommand::NO_FLAGS, "Write a line to a certain process.", {}, VirtualMachine::suggestProcessId<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<Process::Id>(parseError, argv[1], "process id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if ((frame.process()->getUserFlags() & Process::ADMIN) != 0) {
		if (const auto handle = vm.findProcess(id)) {
			handle->input()->writeln(argv[2]);
			return cmd::done();
		}
		return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
	}

	if (const auto handle = vm.findProcess(id); handle && handle->parent().lock() == frame.process()) {
		handle->input()->writeln(argv[2]);
		return cmd::done();
	}
	return cmd::error("{}: Couldn't find child process \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(write_done, "", ConCommand::NO_FLAGS, "Tell our output process that we are done writing to it.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	frame.process()->setOutputDone();
	return cmd::done();
}

CON_COMMAND(write_done_to, "<process>", ConCommand::NO_FLAGS, "Tell a certain process that we are done writing to it.", {},
            VirtualMachine::suggestProcessId<1>) {
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
			handle->input()->setDone(true);
			return cmd::done();
		}
		return cmd::error("{}: Couldn't find process \"{}\".", self.getName(), argv[1]);
	}

	if (const auto handle = vm.findProcess(id); handle && handle->parent().lock() == frame.process()) {
		handle->input()->setDone(true);
		return cmd::done();
	}
	return cmd::error("{}: Couldn't find child process \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(write_has_output, "", ConCommand::NO_FLAGS, "Check if there is an output buffer to write to.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(frame.process()->hasOutput());
}

CON_COMMAND(input_received, "", ConCommand::NO_FLAGS, "Check if our process has received new input.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(frame.process()->input()->canRead());
}

CON_COMMAND(input_done, "", ConCommand::NO_FLAGS, "Check if our process's input has ended.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(frame.process()->input()->isDone());
}

CON_COMMAND(read_has_input, "", ConCommand::NO_FLAGS, "Check if there is or may be more input to read.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(frame.process()->input()->canRead() || !frame.process()->input()->isDone());
}

CON_COMMAND(read, "", ConCommand::NO_FLAGS, "Read a string from the process input.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	if (auto result = frame.process()->input()->read()) {
		return cmd::done(std::move(*result));
	}

	return cmd::deferToNextFrame(0);
}

CON_COMMAND(readln, "", ConCommand::NO_FLAGS, "Read a line from the process input.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	if (auto result = frame.process()->input()->readln()) {
		return cmd::done(std::move(*result));
	}

	return cmd::deferToNextFrame(0);
}
