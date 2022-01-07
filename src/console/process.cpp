#include "process.hpp"

#include "../debug.hpp"                        // Msg, DEBUG_MSG, DEBUG_MSG_INDENT
#include "../game/game.hpp"                    // Game
#include "../utilities/algorithm.hpp"          // util::subview, util::transform, util::collect, util::take, util::eraseIf
#include "../utilities/match.hpp"              // util::match
#include "../utilities/scope_guard.hpp"        // util::ScopeGuard
#include "../utilities/string.hpp"             // util::join, util::stringTo
#include "command_utilities.hpp"               // cmd::...
#include "commands/game_commands.hpp"          // cmd_open_chat
#include "commands/game_server_commands.hpp"   // sv_cheats
#include "commands/input_manager_commands.hpp" // cmd_actionlist
#include "commands/process_commands.hpp"       // await_limit
#include "commands/utility_commands.hpp"       // help
#include "con_command.hpp"                     // ConCommand, GET_COMMAND
#include "convar.hpp"                          // ConVar...
#include "virtual_machine.hpp"                 // VirtualMachine

#include <algorithm>  // std::minmax
#include <cassert>    // assert
#include <fmt/core.h> // fmt::format
#include <variant>    // std::holds_alternative

namespace {

auto getChatBoundInput(Game& game) -> std::string {
	auto input = std::string{GET_COMMAND(open_chat).getName()};
	for (const auto& bind : game.inputManager().getBinds()) {
		if (bind.output == GET_COMMAND(open_chat).getName()) {
			input = bind.input;
			break;
		}
	}
	return input;
}

} // namespace

Process::Process(VirtualMachine& vm, Id id, float startTime, UserFlags userFlags)
	: m_vm(vm)
	, m_startTime(startTime)
	, m_id(id)
	, m_userFlags(userFlags) {}

auto Process::clearLatestError() noexcept -> void {
	m_latestError.reset();
}

auto Process::getLatestError() noexcept -> std::optional<std::string_view> {
	return (m_latestError) ? std::optional<std::string_view>{*m_latestError} : std::nullopt;
}

auto Process::parent() noexcept -> std::weak_ptr<Process> {
	return m_parent;
}

auto Process::getId() const noexcept -> Id {
	return m_id;
}

auto Process::getUserFlags() const noexcept -> UserFlags {
	return m_userFlags;
}

auto Process::input() noexcept -> const std::shared_ptr<IOBuffer>& {
	return m_input;
}

auto Process::hasOutput() const noexcept -> bool {
	return m_output.has_value();
}

auto Process::getOutput() const noexcept -> std::optional<std::weak_ptr<IOBuffer>> {
	return m_output;
}

auto Process::getStartTime() const noexcept -> float {
	return m_startTime;
}

auto Process::running() const noexcept -> bool {
	return m_running;
}

auto Process::done() const noexcept -> bool {
	return m_callStack.empty() && m_children.empty();
}

auto Process::getProgress() const -> float {
	auto total = std::size_t{0};
	auto done = std::size_t{0};
	for (const auto& frame : m_callStack) {
		total += frame.commands.size();
		done += std::min(frame.programCounter, total);
	}
	return (total == 0) ? 1.0f : static_cast<float>(done) / static_cast<float>(total);
}

auto Process::defined(const std::shared_ptr<Environment>& env, const std::string& name) const -> bool { // NOLINT(readability-convert-member-functions-to-static)
	if (ConCommand::find(name)) {
		return true;
	}
	if (ConVar::find(name)) {
		return true;
	}
	for (auto* pEnv = env.get(); pEnv != nullptr; pEnv = pEnv->parent.get()) {
		if (pEnv->objects.count(name) != 0) {
			return true;
		}
		if (pEnv->aliases.count(name) != 0) {
			return true;
		}
	}
	return false;
}

auto Process::findObject(const std::shared_ptr<Environment>& env, const std::string& name) const // NOLINT(readability-convert-member-functions-to-static)
	-> Environment::Object* {
	auto aliasDepth = std::size_t{0};

	const auto* pName = &name;
	do {
		for (auto* pEnv = env.get();; pEnv = pEnv->parent.get()) {
			if (!pEnv) {
				return nullptr;
			}

			if (const auto it = pEnv->aliases.find(*pName); it != pEnv->aliases.end()) {
				if (const auto& cmd = it->second; cmd.size() == 1) {
					pName = &cmd.front().value;
					++aliasDepth;
					break;
				}
			}

			if (const auto it = pEnv->objects.find(*pName); it != pEnv->objects.end()) {
				return &it->second;
			}
		}
	} while (aliasDepth < MAX_ALIAS_DEPTH);
	return nullptr;
}

auto Process::format(float currentTime) const -> std::string {
	return fmt::format(
		"#{} ({:g}%) {}s:\n"
		"{}",
		static_cast<int>(m_id),
		this->getProgress() * 100.0f,
		static_cast<unsigned>(currentTime - m_startTime),
		m_callStack | util::transform([](const CallFrame& frame) {
			static constexpr auto formatCommand = [](const Script::Command& command) {
				if (command.empty()) {
					return std::string{};
				}
				if (command.size() == 1) {
					return command.front().value;
				}
				return fmt::format("{}({})", command.front().value, util::subview(command, 1) | util::transform(Script::argumentString) | util::join(", "));
			};

			return fmt::format((frame.commands.size() <= 2) ? "  {}" : "  {}...",
							   frame.commands | util::take(2) | util::transform(formatCommand) | util::join("; "));
		}) | util::join('\n'));
}

auto Process::launchChildProcess(Process::UserFlags userFlags) -> std::shared_ptr<Process> {
	if (auto process = m_vm->launchProcess(userFlags)) {
		process->m_parent = this->weak_from_this();
		m_children.push_back(process);
		return process;
	}
	return nullptr;
}

auto Process::setErrorHandler(ErrorHandler errorHandler) -> void {
	m_errorHandler = std::move(errorHandler);
}

auto Process::resetOutput() noexcept -> void {
	m_output.reset();
}

auto Process::setOutput(const std::shared_ptr<IOBuffer>& output) noexcept -> void {
	assert(output);
	DEBUG_MSG(Msg::CONSOLE_DETAILED, "Process {} adding output buffer.", this->getId());
	output->setDone(false);
	m_output = output;
}

auto Process::setOutputDone() noexcept -> void {
	if (m_output) {
		if (auto ptr = m_output->lock()) {
			ptr->setDone(true);
		}
	}
}

auto Process::output(std::string_view str) -> bool {
	if (m_output) {
		if (auto ptr = m_output->lock()) {
			ptr->write(str);
		}
		return true;
	}
	return false;
}

auto Process::outputln(std::string_view str) -> bool {
	if (m_output) {
		if (auto ptr = m_output->lock()) {
			ptr->writeln(str);
		}
		return true;
	}
	return false;
}

auto Process::release() noexcept -> bool {
	DEBUG_MSG(Msg::CONSOLE_DETAILED, "Process {} was released.", this->getId());

	if (m_vm->adoptProcess(this->shared_from_this())) {
		if (auto parent = m_parent.lock()) {
			util::eraseIf(parent->m_children, [this](const auto& child) { return child.get() == this; });
		}
		m_parent.reset();
		return true;
	}
	return false;
}

auto Process::end() noexcept -> void {
	DEBUG_MSG(Msg::CONSOLE_DETAILED, "Process {} ended.", this->getId());

	for (auto& frame : m_callStack) {
		frame.programCounter = frame.commands.size();
	}

	if (m_output) {
		if (auto ptr = m_output->lock()) {
			ptr->setDone(true);
		}
		m_output.reset();
	}

	m_children.clear();
}

auto Process::run(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient, std::size_t targetFrameIndex)
	-> cmd::Result {
	auto result = cmd::done();
	auto guard = util::ScopeGuard{[this] {
		m_running = false;
	}};

	m_running = true;

	DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED, "Process {} running...", this->getId()) {
		auto iteration = 0;
		while (m_callStack.size() > targetFrameIndex) {
			if (m_callStack.back().executing) {
				break;
			}

			if (m_callStack.back().programCounter >= m_callStack.back().commands.size()) {
				m_callStack.pop_back();
				continue;
			}

			const auto frameIndex = m_callStack.size() - 1;
			auto frame = util::Reference{m_callStack[frameIndex]};
			const auto& command = frame->commands[frame->programCounter];
			auto& commandState = frame->commandStates[frame->programCounter];

			if (await_limit > 0 && iteration > await_limit) {
				result = cmd::deferToNextFrame(commandState.progress);
				break;
			}

			++iteration;
			if ((command.back().flags & Script::Argument::PIPE) != 0) {
				this->setupPipeline(result, frame);
				frame = m_callStack[frameIndex];
			} else if (!commandState.argsExpanded) {
				if (this->expandArgs(frameIndex)) {
					continue; // Don't update status.
				}

				frame = m_callStack[frameIndex];
				result = cmd::error("{}: Stack overflow.", frame->commands[frame->programCounter].front().value);
			} else if (commandState.arguments.front().value.empty()) {
				result = cmd::error("Empty command name.");
			} else {
				frame->executing = true;
				DEBUG_MSG_INDENT(
					Msg::CONSOLE_DETAILED,
					"Executing process {} stack[{}]: {}",
					this->getId(),
					frameIndex,
					(commandState.arguments.size() == 1) ?
						commandState.arguments.front().value :
                        fmt::format("{} {}",
									commandState.arguments.front().value,
									util::subview(commandState.arguments, 1) | util::transform([](const auto& arg) { return arg.value; }) |
										util::transform(Script::escapedString) | util::join(' '))) {
					if (!this->checkAliases(result, frame->env, commandState.arguments, frame->returnFrameIndex, frame->returnArgumentIndex) &&
						!this->checkObjects(result, frame->env, commandState.arguments, frame->returnFrameIndex, frame->returnArgumentIndex) &&
						!this->checkGlobals(result, commandState.arguments, commandState.data, frameIndex, game, server, client, metaServer, metaClient)) {
						if (client) {
							result = cmd::error(
								"Unknown command: \"{}\". Try \"{}\".\n"
								"Tip: Press esc to close the console.\n"
								"Tip: Chat is bound to {}.",
								commandState.arguments.front().value,
								GET_COMMAND(help).getName(),
								getChatBoundInput(game));
						} else {
							result = cmd::error("Unknown command: \"{}\". Try \"{}\".",
												commandState.arguments.front().value,
												GET_COMMAND(help).getName());
						}
						const auto& cmds = ConCommand::all();
						const auto& cvars = ConVar::all();

						const auto isSimilar = [&](const auto& cmd) {
							const auto lengths = std::minmax(cmd.second.getName().size(), commandState.arguments.front().value.size());
							return (lengths.second - lengths.first) <= 3 &&
								   util::ifind(cmd.second.getName(), commandState.arguments.front().value) == 0;
						};

						if (const auto itCmd = util::findIf(cmds, isSimilar); itCmd != cmds.end()) {
							result.value.append(fmt::format("\nDid you mean: {}?", itCmd->second.getName()));
						} else if (const auto itCvar = util::findIf(cvars, isSimilar); itCvar != cvars.end()) {
							result.value.append(fmt::format("\nDid you mean: {}?", itCvar->second.getName()));
						}
					}

					if (frameIndex < m_callStack.size()) {
						frame = m_callStack[frameIndex];
						frame->executing = false;
					} else {
						DEBUG_MSG(Msg::CONSOLE_DETAILED, "Stack frame died.");
						break;
					}
				}
			}

			frame->status = result.status;
			if (frame->returnFrameIndex != NO_FRAME) {
				auto& retFrame = m_callStack[frame->returnFrameIndex];
				auto& arguments = retFrame.commandStates[retFrame.programCounter].arguments;
				if (frame->returnArgumentIndex < arguments.size()) {
					arguments[frame->returnArgumentIndex] = result;
				}
			}

			switch (result.status) {
				case cmd::Status::RETURN: [[fallthrough]];
				case cmd::Status::RETURN_VALUE: [[fallthrough]];
				case cmd::Status::BREAK: [[fallthrough]];
				case cmd::Status::CONTINUE:
					while (m_callStack.size() > targetFrameIndex) {
						if (m_callStack.back().firstInSection) {
							m_callStack.pop_back();
							break;
						}
						m_callStack.pop_back();
					}
					continue;
				case cmd::Status::NOT_DONE: [[fallthrough]];
				case cmd::Status::DEFER_TO_NEXT_FRAME:
					if (frame->programCounter < frame->commandStates.size()) {
						auto& state = frame->commandStates[frame->programCounter];
						auto& progress = state.progress;
						const auto newProgress = util::stringTo<cmd::Progress>(result.value);
						assert(newProgress);
						DEBUG_MSG(Msg::CONSOLE_DETAILED, "Command did not finish. Progress: {} -> {}.", progress, *newProgress);
						progress = *newProgress;
					}
					result.value.clear();
					break;
				case cmd::Status::ERROR_MSG:
					DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED, "Error: {}", result.value) {
						if (!this->handleError(result)) {
							m_vm->outputError(result.value);
						}
					}
					break;
				default:
					if (frame->programCounter < frame->commandStates.size()) {
						auto& state = frame->commandStates[frame->programCounter];
						state.data.reset();
						state.arguments.clear();
						++frame->programCounter;
					}
					break;
			}

			if (result.status == cmd::Status::DEFER_TO_NEXT_FRAME) {
				break;
			}
		}

		DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED, "Running child processes of process {}...", this->getId()) {
			m_vm->runProcesses(m_children, game, server, client, metaServer, metaClient);
		}
	}
	DEBUG_MSG(Msg::CONSOLE_DETAILED, "Process {} {}.", this->getId(), (this->done()) ? "done" : "not done");
	return result;
}

auto Process::await(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient, std::size_t targetFrameIndex)
	-> cmd::Result {
	if (await_limit > 0) {
		return this->awaitLimited(game, server, client, metaServer, metaClient, await_limit, targetFrameIndex);
	}
	return this->awaitUnlimited(game, server, client, metaServer, metaClient, targetFrameIndex);
}

auto Process::awaitUnlimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient,
							 std::size_t targetFrameIndex) -> cmd::Result {
	return this->awaitLimited(game, server, client, metaServer, metaClient, 0, targetFrameIndex);
}

auto Process::awaitLimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient, int limit,
						   std::size_t targetFrameIndex) -> cmd::Result {
	auto result = cmd::done();
	DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED, "Process {} awaiting result...", this->getId()) {
		for (auto iteration = 0; m_callStack.size() > targetFrameIndex; ++iteration) {
			if (limit > 0 && iteration > limit) {
				DEBUG_MSG(Msg::CONSOLE_DETAILED, "Reached await limit.");
				return cmd::error("Reached await limit.");
			}
			result = this->run(game, server, client, metaServer, metaClient, targetFrameIndex);
		}
	}
	DEBUG_MSG(Msg::CONSOLE_DETAILED, "Process {} await done.", this->getId());
	return result;
}

auto Process::call(std::shared_ptr<Environment> env, std::string_view script, std::size_t returnFrameIndex, std::size_t returnArgumentIndex,
				   const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	return this->call(std::move(env), Script::parse(script), returnFrameIndex, returnArgumentIndex, exportTarget);
}

auto Process::call(std::shared_ptr<Environment> env, cmd::CommandView argv, std::size_t returnFrameIndex, std::size_t returnArgumentIndex,
				   const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	auto command = Script::Command{};
	command.reserve(argv.size());
	for (const auto& arg : argv) {
		command.emplace_back(arg, Script::Argument::NO_FLAGS);
	}
	return this->call(std::move(env), Script{std::move(command)}, returnFrameIndex, returnArgumentIndex, exportTarget);
}

auto Process::call(std::shared_ptr<Environment> env, Script::Command command, std::size_t returnFrameIndex, std::size_t returnArgumentIndex,
				   const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	return this->call(std::move(env), Script{std::move(command)}, returnFrameIndex, returnArgumentIndex, exportTarget);
}

auto Process::call(std::shared_ptr<Environment> env, Script commands, std::size_t returnFrameIndex, std::size_t returnArgumentIndex,
				   const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	const auto frameIndex = m_callStack.size();
	assert(returnFrameIndex == NO_FRAME || returnFrameIndex < frameIndex);
	DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED,
					 "Process {} called {}.",
					 this->getId(),
					 (commands.empty())    ? "no commands" :
					 (commands.size() > 1) ? "several commands" :
                                             commands.front().front().value) {
		if (frameIndex == Process::MAX_STACK_SIZE) {
			DEBUG_MSG(Msg::CONSOLE_DETAILED, "Stack overflow!");
			return std::nullopt;
		}

		m_callStack.emplace_back(std::move(env), std::move(commands), returnFrameIndex, returnArgumentIndex, exportTarget);
	}
	return CallFrameHandle{this->shared_from_this(), frameIndex};
}

auto Process::call(std::shared_ptr<Environment> env, const Environment::Function& function, std::size_t returnFrameIndex,
				   std::size_t returnArgumentIndex, const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	return this->call(std::move(env), function, util::Span<const cmd::Value>{}, returnFrameIndex, returnArgumentIndex, exportTarget);
}

auto Process::call(std::shared_ptr<Environment> env, const Environment::Function& function, util::Span<const cmd::Value> args, std::size_t returnFrameIndex,
				   std::size_t returnArgumentIndex, const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	auto frame = this->call(std::make_shared<Environment>(std::move(env)), function.body, returnFrameIndex, returnArgumentIndex, exportTarget);
	if (frame) {
		frame->makeSection();
		if (args.empty()) {
			frame->env()->objects.try_emplace("@", Environment::Array{});
		} else {
			auto i = std::size_t{0};
			for (; i < args.size(); ++i) {
				if (i < function.parameters.size()) {
					const auto& param = function.parameters[i];
					if (param == "...") {
						frame->env()->objects["@"] = Environment::Array{args[i]};
					} else {
						frame->env()->objects[param] = Environment::Variable{args[i]};
					}
				} else {
					auto& obj = frame->env()->objects.try_emplace("@", Environment::Array{}).first->second;
					if (auto* const arr = std::get_if<Environment::Array>(&obj)) {
						arr->push_back(args[i]);
					} else {
						assert(std::holds_alternative<Environment::Variable>(obj));
						auto value = std::move(std::get_if<Environment::Variable>(&obj)->value);

						obj = Environment::Array{std::move(value), args[i]};
					}
				}
			}

			if (i + 1 == function.parameters.size() && function.parameters.back() == "...") {
				frame->env()->objects.try_emplace("@", Environment::Array{});
			}
		}
	}
	return frame;
}

auto Process::call(std::shared_ptr<Environment> env, ConCommand& cmd, std::size_t returnFrameIndex, std::size_t returnArgumentIndex,
				   const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	return this->call(std::move(env), cmd, util::Span<const cmd::Value>{}, returnFrameIndex, returnArgumentIndex, exportTarget);
}

auto Process::call(std::shared_ptr<Environment> env, ConCommand& cmd, util::Span<const cmd::Value> args, std::size_t returnFrameIndex,
				   std::size_t returnArgumentIndex, const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	auto command = Script::Command{Script::Argument{std::string{cmd.getName()}, Script::Argument::NO_FLAGS}};
	for (const auto& arg : args) {
		command.emplace_back(arg, Script::Argument::NO_FLAGS);
	}
	return this->call(std::move(env), std::move(command), returnFrameIndex, returnArgumentIndex, exportTarget);
}

auto Process::call(std::shared_ptr<Environment> env, ConVar& cvar, std::size_t returnFrameIndex, std::size_t returnArgumentIndex,
				   const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	return this->call(std::move(env),
					  Script::Command{Script::Argument{std::string{cvar.getName()}, Script::Argument::NO_FLAGS}},
					  returnFrameIndex,
					  returnArgumentIndex,
					  exportTarget);
}

auto Process::call(std::shared_ptr<Environment> env, ConVar& cvar, std::string value, std::size_t returnFrameIndex,
				   std::size_t returnArgumentIndex, const std::shared_ptr<Environment>& exportTarget) -> std::optional<CallFrameHandle> {
	return this->call(std::move(env),
					  Script::Command{Script::Argument{std::string{cvar.getName()}, Script::Argument::NO_FLAGS},
									  Script::Argument{std::move(value), Script::Argument::NO_FLAGS}},
					  returnFrameIndex,
					  returnArgumentIndex,
					  exportTarget);
}

auto operator==(const Process& lhs, const Process& rhs) noexcept -> bool {
	return lhs.getId() == rhs.getId();
}

auto operator!=(const Process& lhs, const Process& rhs) noexcept -> bool {
	return !(lhs == rhs);
}

auto Process::handleError(cmd::Result error) -> bool {
	DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED, "Unwinding stack.") {
		while (!m_callStack.empty()) {
			if (m_callStack.back().firstInTryBlock) {
				m_callStack.pop_back();
				DEBUG_MSG(Msg::CONSOLE_DETAILED, "Reached beginning of try block. {} stack frames left.", m_callStack.size());
				m_latestError = std::move(error.value);
				return true;
			}
			m_callStack.pop_back();
		}
	}

	util::eraseIf(m_children, [](const auto& child) { return !child->running(); });

	if (m_errorHandler && m_errorHandler(error)) {
		return true;
	}

	if (auto parent = m_parent.lock()) {
		return parent->handleError(std::move(error));
	}

	m_latestError = std::move(error.value);
	return false;
}

auto Process::setupPipeline(cmd::Result& result, CallFrame& frame) -> void {
	DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED, "Pipeline setting up...") {
		auto parent = std::shared_ptr<Process>{};
		for (const auto& command : util::subview(frame.commands, frame.programCounter)) {
			assert(!command.empty());
			++frame.programCounter;
			if (auto process = (parent) ? parent->launchChildProcess(this->getUserFlags()) : this->launchChildProcess(this->getUserFlags())) {
				auto cmd = command;
				cmd.back().flags &= ~Script::Argument::PIPE;
				if (!process->call(frame.env, std::move(cmd))) {
					result = cmd::error("Failed to setup pipe: Stack overflow.");
					DEBUG_MSG(Msg::CONSOLE_DETAILED, "Failed: Stack overflow!");
					return;
				}

				if (parent) {
					parent->setOutput(process->input());
				}
				parent = std::move(process);
			} else {
				result = cmd::error("Failed to setup pipe: Couldn't launch process.");
				DEBUG_MSG(Msg::CONSOLE_DETAILED, "Failed: Couldn't launch process!");
				return;
			}

			if ((command.back().flags & Script::Argument::PIPE) == 0) {
				break;
			}
		}
	}
	DEBUG_MSG(Msg::CONSOLE_DETAILED, "Pipeline set up.");
}

auto Process::expandArgs(std::size_t frameIndex) -> bool {
	const auto iPc = m_callStack[frameIndex].programCounter;
	auto command = m_callStack[frameIndex].commands[iPc];
	for (auto i = std::size_t{0}; i < command.size();) {
		if ((command[i].flags & Script::Argument::EXPAND) != 0) {
			if (auto* const obj = this->findObject(m_callStack[frameIndex].env, command[i].value)) {
				if (auto* const arr = std::get_if<Environment::Array>(obj)) {
					if (arr->empty()) {
						command.erase(command.begin() + static_cast<Script::Command::difference_type>(i));
					} else {
						command[i].flags &= ~Script::Argument::EXPAND;
						command[i].value = arr->front();
						++i;
						for (const auto& arg : util::subview(*arr, 1)) {
							command.insert(command.begin() + static_cast<Script::Command::difference_type>(i),
										   Script::Argument{arg, Script::Argument::NO_FLAGS});
							++i;
						}
					}
					continue;
				}
			}
		}
		++i;
	}

	auto& arguments = m_callStack[frameIndex].commandStates[iPc].arguments;
	arguments.clear();
	arguments.reserve(command.size());
	for (auto& arg : command) {
		if ((arg.flags & Script::Argument::EXEC) != 0) {
			arguments.push_back(cmd::done());
		} else {
			arguments.push_back(cmd::done(std::move(arg.value)));
		}
	}

	for (auto returnArgumentIndex = command.size(); returnArgumentIndex-- > 0;) {
		if (const auto& retArg = command[returnArgumentIndex]; (retArg.flags & Script::Argument::EXEC) != 0) {
			DEBUG_MSG_INDENT(Msg::CONSOLE_DETAILED, "Expanding process {} stack[{}][{}].", this->getId(), frameIndex, returnArgumentIndex) {
				if (!this->call(m_callStack[frameIndex].env, retArg.value, frameIndex, returnArgumentIndex)) {
					return false;
				}
			}
		}
	}
	m_callStack[frameIndex].commandStates[iPc].argsExpanded = true;
	return true;
}

auto Process::checkAliases(cmd::Result& result, const std::shared_ptr<Environment>& env, cmd::CommandArguments& arguments,
						   std::size_t returnFrameIndex, std::size_t returnArgumentIndex) -> bool {
	assert(!arguments.empty());
	for (auto* pEnv = env.get(); pEnv != nullptr; pEnv = pEnv->parent.get()) {
		if (const auto it = pEnv->aliases.find(arguments.front().value); it != pEnv->aliases.end()) {
			auto cmd = it->second;
			assert(!cmd.empty());
			cmd.reserve(cmd.size() + arguments.size() - 1);
			for (auto& argument : util::subview(arguments, 1)) {
				cmd.emplace_back(std::move(argument.value));
			}

			if (this->call(env, std::move(cmd), returnFrameIndex, returnArgumentIndex)) {
				result = cmd::done();
			} else {
				result = cmd::error("{}: Stack overflow.", arguments.front().value);
			}
			return true;
		}
	}
	return false;
}

auto Process::checkObjects(cmd::Result& result, const std::shared_ptr<Environment>& env, cmd::CommandArguments& arguments,
						   std::size_t returnFrameIndex, std::size_t returnArgumentIndex) -> bool {
	assert(!arguments.empty());
	for (auto* pEnv = env.get(); pEnv != nullptr; pEnv = pEnv->parent.get()) {
		if (const auto it = pEnv->objects.find(arguments.front().value); it != pEnv->objects.end()) {
			result = util::match(it->second)(
				[&](Environment::Variable& var) {
					if (arguments.size() == 1) {
						return cmd::done(var.value);
					}
					if (arguments.size() == 2) {
						var.value = std::move(arguments[1].value);
						return cmd::done();
					}
					return cmd::error("Usage: {0} or {0} <value>", arguments.front().value);
				},
				[&](Environment::Constant& constant) {
					if (arguments.size() == 1) {
						return cmd::done(constant.value);
					}
					return cmd::error("Usage: {0}", arguments.front().value);
				},
				[&](Environment::Function& function) {
					if (arguments.size() == function.parameters.size() + 1 ||
						(!function.parameters.empty() && arguments.size() >= function.parameters.size() && function.parameters.back() == "...")) {
						auto args = std::vector<cmd::Value>{};
						args.reserve(arguments.size() - 1);
						for (auto& argument : util::subview(arguments, 1)) {
							args.emplace_back(std::move(argument.value));
						}
						if (!this->call(env, function, args, returnFrameIndex, returnArgumentIndex)) {
							return cmd::error("{}: Stack overflow.", arguments.front().value);
						}
						return cmd::done();
					}
					return cmd::error("Usage: {} {}", arguments.front().value, function.parameters | util::transform([](const auto& param) {
																				   return fmt::format("<{}>", param);
																			   }) | util::join(' '));
				},
				[&](Environment::Array& arr) {
					if (arguments.size() == 1) {
						return cmd::done(Environment::arrayString(arr));
					}
					if (arguments.size() == 2) {
						auto parseError = cmd::ParseError{};

						auto index = cmd::parseNumber<int>(parseError, arguments[1].value, "array index");
						if (parseError) {
							return cmd::error("{}: {}", arguments.front().value, *parseError);
						}

						if (index < 0) {
							index += static_cast<int>(arr.size());
						}

						const auto i = static_cast<std::size_t>(index);
						if (i < arr.size()) {
							return cmd::done(arr[i]);
						}
						return cmd::error("{}: Array index out of range ({}/{}).", arguments.front().value, i, arr.size());
					}
					if (arguments.size() == 3) {
						auto parseError = cmd::ParseError{};

						auto index = cmd::parseNumber<int>(parseError, arguments[1].value, "array index");
						if (parseError) {
							return cmd::error("{}: {}", arguments.front().value, *parseError);
						}

						if (index < 0) {
							index += static_cast<int>(arr.size());
						}

						const auto i = static_cast<std::size_t>(index);
						if (i < arr.size()) {
							arr[i] = std::move(arguments[2].value);
							return cmd::done();
						}
						return cmd::error("{}: Array index out of range ({}/{}).", arguments.front().value, i, arr.size());
					}
					return cmd::error("Usage: {0} or {0} <index> or {0} <index> <value>", arguments.front().value);
				},
				[&](Environment::Table& table) {
					if (arguments.size() == 1) {
						return cmd::done(Environment::tableString(table));
					}
					if (arguments.size() == 2) {
						if (const auto elemIt = table.find(arguments[1].value); elemIt != table.end()) {
							return cmd::done(elemIt->second);
						}
						return cmd::done();
					}
					if (arguments.size() == 3) {
						table[arguments[1].value] = std::move(arguments[2].value);
						return cmd::done();
					}
					return cmd::error("Usage: {0} or {0} <key> or {0} <key> <value>", arguments.front().value);
				});
			return true;
		}
	}
	return false;
}

auto Process::checkGlobals(cmd::Result& result, cmd::CommandArguments& arguments, std::any& data, std::size_t frameIndex, Game& game,
						   GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) -> bool {
	assert(!arguments.empty());
	if (auto&& cmd = ConCommand::find(arguments.front().value)) /* Check commands. */ {
		if ((cmd->getFlags() & ConCommand::CHEAT) != 0 && !sv_cheats) {
			result = cmd::error("{} cannot be used because cheats are disabled.", cmd->getName());
		} else if ((cmd->getFlags() & ConCommand::ADMIN_ONLY) != 0 && (m_userFlags & Process::ADMIN) == 0) {
			result = cmd::error("{} requires admin privileges.", cmd->getName());
		} else if ((cmd->getFlags() & ConCommand::NO_RCON) != 0 && (m_userFlags & Process::REMOTE) != 0) {
			result = cmd::error("{} cannot be used remotely.", cmd->getName());
		} else if ((cmd->getFlags() & ConCommand::SERVER) != 0 && !server) {
			result = cmd::error("{}: Not running a server.", cmd->getName());
		} else if ((cmd->getFlags() & ConCommand::CLIENT) != 0 && !client) {
			result = cmd::error("{}: Not connected to a server.", cmd->getName());
		} else if ((cmd->getFlags() & ConCommand::META_SERVER) != 0 && !metaServer) {
			result = cmd::error("{}: Not running a meta server.", cmd->getName());
		} else if ((cmd->getFlags() & ConCommand::META_CLIENT) != 0 && !metaClient) {
			result = cmd::error("{}: Not running a meta client.", cmd->getName());
		} else {
			result = cmd->execute(cmd::CommandView{arguments}, data, CallFrameHandle{this->shared_from_this(), frameIndex}, game, server, client, metaServer, metaClient, m_vm);
		}
	} else if (auto&& cvar = ConVar::find(arguments.front().value)) /* Check cvars. */ {
		if (arguments.size() == 1) {
			if ((cvar->getFlags() & ConVar::READ_ADMIN_ONLY) != 0 && (m_userFlags & Process::ADMIN) == 0) {
				result = cmd::error("{} can only be read by admin processes.", cvar->getName());
			} else if ((cvar->getFlags() & ConVar::NO_RCON_READ) != 0 && (m_userFlags & Process::REMOTE) != 0) {
				result = cmd::error("{} cannot be read remotely.", cvar->getName());
			} else {
				result = cmd::done(cvar->getString());
			}
		} else if ((cvar->getFlags() & ConVar::READ_ONLY) != 0) {
			result = cmd::error("{} is read-only.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::INIT) != 0 && m_vm->started()) {
			result = cmd::error("{} cannot be changed after startup.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::CHEAT) != 0 && !sv_cheats) {
			result = cmd::error("{} cannot be changed because cheats are disabled.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::REPLICATED) != 0 && client && !server) {
			result = cmd::error("{} cannot be changed because you are not the server.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::NOT_RUNNING_GAME_SERVER) != 0 && server) {
			result = cmd::error("{} cannot be changed while running a game server.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::NOT_RUNNING_GAME_CLIENT) != 0 && client) {
			result = cmd::error("{} cannot be changed while running a game client.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::NOT_RUNNING_META_SERVER) != 0 && metaServer) {
			result = cmd::error("{} cannot be changed while running a meta server.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::NOT_RUNNING_META_CLIENT) != 0 && metaClient) {
			result = cmd::error("{} cannot be changed while running a meta client.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::WRITE_ADMIN_ONLY) != 0 && (m_userFlags & Process::ADMIN) == 0) {
			result = cmd::error("{} can only be changed by admin processes.", cvar->getName());
		} else if ((cvar->getFlags() & ConVar::NO_RCON_WRITE) != 0 && (m_userFlags & Process::REMOTE) != 0) {
			result = cmd::error("{} cannot be changed remotely.", cvar->getName());
		} else {
			result = cvar->set(util::subview(arguments, 1) | util::transform([](const auto& arg) { return arg.value; }) | util::join(' '),
							   game,
							   server,
							   client,
							   metaServer,
							   metaClient);
		}
	} else if (!arguments.front().value.empty() && arguments.front().value.front() == '+') /* Check input manager press. */ {
		if ((m_userFlags & Process::ADMIN) == 0) {
			result = cmd::error("{} requires admin privileges.", arguments.front().value);
		} else if (game.inputManager().pressAction(std::string_view{arguments.front().value}.substr(1))) {
			result = cmd::done();
		} else {
			result = cmd::error("Unknown action: \"{}\". Try \"{}\".", arguments.front().value, GET_COMMAND(actionlist).getName());
		}
	} else if (!arguments.front().value.empty() && arguments.front().value.front() == '-') /* Check input manager release. */ {
		if ((m_userFlags & Process::ADMIN) == 0) {
			result = cmd::error("{} requires admin privileges.", arguments.front().value);
		} else if (game.inputManager().releaseAction(std::string_view{arguments.front().value}.substr(1))) {
			result = cmd::done();
		} else {
			result = cmd::error("Unknown action: \"{}\". Try \"{}\".", arguments.front().value, GET_COMMAND(actionlist).getName());
		}
	} else {
		return false;
	}
	return true;
}
