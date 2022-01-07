#include "virtual_machine.hpp"

#include "../game/client/input_manager.hpp" // InputManager
#include "../utilities/algorithm.hpp"       // util::collect, util::transform, util::erase, util::contains, util::collect
#include "../utilities/reference.hpp"       // util::Reference
#include "../utilities/string.hpp"          // util::join, util::toString

#include <cassert>    // assert
#include <cmath>      // std::nextafter
#include <fmt/core.h> // fmt::format
#include <limits>     // std::numeric_limits
#include <utility>    // std::move, std::forward, std::pair

VirtualMachine::VirtualMachine(OutputCallback output, ErrorCallback error)
	: m_output(std::move(output))
	, m_error(std::move(error)) {}

auto VirtualMachine::globalEnv() noexcept -> const std::shared_ptr<Environment>& {
	return m_env;
}

auto VirtualMachine::getGlobalEnv() const noexcept -> const Environment& {
	return *m_env;
}

auto VirtualMachine::getTime() const noexcept -> float {
	return m_time;
}

auto VirtualMachine::getProcessCount() const noexcept -> std::size_t {
	return m_processes.size();
}

auto VirtualMachine::rng() noexcept -> std::mt19937& {
	return m_rng;
}

auto VirtualMachine::rng() const noexcept -> const std::mt19937& {
	return m_rng;
}

auto VirtualMachine::started() const noexcept -> bool {
	return m_started;
}

auto VirtualMachine::getAllProcessIds() const -> std::vector<Process::Id> {
	static constexpr auto getProcessId = [](const auto& process) {
		return process->getId();
	};
	return m_processes | util::transform(getProcessId) | util::collect<std::vector<Process::Id>>();
}

auto VirtualMachine::randomFloat(float min, float max) -> float {
	return m_floatDistribution(m_rng, decltype(m_floatDistribution)::param_type{min, std::nextafter(max, std::numeric_limits<float>::max())});
}

auto VirtualMachine::randomInt(int min, int max) -> int {
	return m_intDistribution(m_rng, decltype(m_intDistribution)::param_type{min, max});
}

auto VirtualMachine::processSummary() const -> std::string {
	const auto formatProcess = [&](const auto& kv) {
		const auto& [id, ptr] = kv;
		if (const auto process = ptr.lock()) {
			return process->format(m_time);
		}
		return fmt::format("{}: null", id);
	};

	return fmt::format(
		"Process summary:\n"
		"{}",
		m_processMap | util::transform(formatProcess) | util::join('\n'));
}

auto VirtualMachine::output(std::string message) -> void {
	assert(m_output);
	m_output(std::move(message));
}

auto VirtualMachine::outputln(std::string message) -> void {
	assert(m_output);
	message.push_back('\n');
	m_output(std::move(message));
}

auto VirtualMachine::outputError(std::string message) -> void {
	assert(m_error);
	m_error(std::move(message));
}

auto VirtualMachine::output(cmd::Result result) -> void {
	if (result.status == cmd::Status::VALUE || result.status == cmd::Status::RETURN_VALUE) {
		this->outputln(std::move(result.value));
	}
}

auto VirtualMachine::start() -> void {
	m_started = true;
}

auto VirtualMachine::run(float deltaTime, Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) -> void {
	m_time += deltaTime;
	this->runProcesses(m_processes, game, server, client, metaServer, metaClient);
}

auto VirtualMachine::runProcesses(std::vector<std::shared_ptr<Process>>& processes, Game& game, GameServer* server, GameClient* client,
								  MetaServer* metaServer, MetaClient* metaClient) -> void {
	const auto end = processes.size();
	for (auto i = std::size_t{0}; i < end; ++i) {
		auto& process = processes[i];
		assert(process);
		this->output(process->run(game, server, client, metaServer, metaClient));
		assert(processes.size() >= end);
		assert(process);
		if (process->done()) {
			process->end();
			process.reset();
		}
	}

	util::erase(processes, nullptr);
	util::eraseIf(m_processMap, [](const auto& kv) { return kv.second.expired(); });
}

auto VirtualMachine::endAllProcesses() -> void {
	for (auto&& kv : m_processMap) {
		if (const auto process = kv.second.lock()) {
			process->end();
		}
	}
}

auto VirtualMachine::findProcess(Process::Id id) -> std::shared_ptr<Process> {
	if (const auto it = m_processMap.find(id); it != m_processMap.end()) {
		return it->second.lock();
	}
	return nullptr;
}

auto VirtualMachine::launchProcess(Process::UserFlags userFlags) -> std::shared_ptr<Process> {
	const auto it = m_processMap.emplace_back();

	auto process = std::make_shared<Process>(*this, it->first, m_time, userFlags);
	if (!process) {
		m_processMap.erase(it);
		return nullptr;
	}
	it->second = process;
	return process;
}

auto VirtualMachine::adoptProcess(std::shared_ptr<Process> process) -> bool {
	assert(process);
	assert(!util::contains(m_processes, process));
	m_processes.push_back(std::move(process));
	return true;
}

auto VirtualMachine::getProcessIdSuggestions() const -> Suggestions {
	static constexpr auto formatProcessId = [](const auto& id) {
		return util::toString(id);
	};
	return this->getAllProcessIds() | util::transform(formatProcessId) | util::collect<Suggestions>();
}
