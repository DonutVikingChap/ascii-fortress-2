#ifndef AF2_CONSOLE_VIRTUAL_MACHINE_HPP
#define AF2_CONSOLE_VIRTUAL_MACHINE_HPP

#include "../utilities/registry.hpp" // util::Registry
#include "../utilities/span.hpp"     // util::Span
#include "command.hpp"               // cmd::...
#include "environment.hpp"           // Environment
#include "process.hpp"               // Process
#include "suggestions.hpp"           // Suggestions, SUGGESTIONS

#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <functional>  // std::function
#include <memory>      // std::shared_ptr, std::make_shared, std::weak_ptr
#include <optional>    // std::optional, std::nullopt
#include <random>      // std::mt19937, std::random_device, std::uniform_int_distribution, std::uniform_real_distribution
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

class Game;
class GameServer;
class GameClient;
class MetaServer;
class MetaClient;
class ConVar;

class VirtualMachine final {
public:
	using OutputCallback = std::function<void(std::string&&)>;
	using ErrorCallback = std::function<void(std::string&&)>;

	template <std::size_t INDEX>
	static SUGGESTIONS(suggestProcessId) {
		if (i == INDEX) {
			return vm.getProcessIdSuggestions();
		}
		return Suggestions{};
	}

	VirtualMachine(OutputCallback output, ErrorCallback error);

	[[nodiscard]] auto globalEnv() noexcept -> const std::shared_ptr<Environment>&;
	[[nodiscard]] auto getGlobalEnv() const noexcept -> const Environment&;
	[[nodiscard]] auto getTime() const noexcept -> float;
	[[nodiscard]] auto getProcessCount() const noexcept -> std::size_t;
	[[nodiscard]] auto rng() noexcept -> std::mt19937&;
	[[nodiscard]] auto rng() const noexcept -> const std::mt19937&;
	[[nodiscard]] auto started() const noexcept -> bool;
	[[nodiscard]] auto getAllProcessIds() const -> std::vector<Process::Id>;

	[[nodiscard]] auto randomFloat(float min, float max) -> float;
	[[nodiscard]] auto randomInt(int min, int max) -> int;

	[[nodiscard]] auto processSummary() const -> std::string;

	auto output(std::string message) -> void;
	auto outputln(std::string message) -> void;
	auto outputError(std::string message) -> void;
	auto output(cmd::Result result) -> void;

	auto start() -> void;

	auto run(float deltaTime, Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) -> void;
	auto runProcesses(std::vector<std::shared_ptr<Process>>& processes, Game& game, GameServer* server, GameClient* client,
					  MetaServer* metaServer, MetaClient* metaClient) -> void;

	auto endAllProcesses() -> void;

	[[nodiscard]] auto findProcess(Process::Id id) -> std::shared_ptr<Process>;
	[[nodiscard]] auto launchProcess(Process::UserFlags userFlags) -> std::shared_ptr<Process>;

	[[nodiscard]] auto adoptProcess(std::shared_ptr<Process> process) -> bool;

private:
	[[nodiscard]] auto getProcessIdSuggestions() const -> Suggestions;

	std::shared_ptr<Environment> m_env = std::make_shared<Environment>();
	std::vector<std::shared_ptr<Process>> m_processes{};
	util::Registry<std::weak_ptr<Process>, Process::Id> m_processMap{};
	OutputCallback m_output;
	ErrorCallback m_error;
	std::mt19937 m_rng{std::random_device{}()};
	std::uniform_int_distribution<int> m_intDistribution{};
	std::uniform_real_distribution<float> m_floatDistribution{};
	float m_time = 0.0f;
	bool m_started = false;
};

#endif
