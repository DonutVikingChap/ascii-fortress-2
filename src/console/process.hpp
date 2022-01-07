#ifndef AF2_CONSOLE_PROCESS_HPP
#define AF2_CONSOLE_PROCESS_HPP

#include "../utilities/reference.hpp" // util::Reference
#include "../utilities/span.hpp"      // util::Span
#include "call_frame_handle.hpp"      // CallFrameHandle
#include "command.hpp"                // cmd::...
#include "environment.hpp"            // Environment
#include "io_buffer.hpp"              // IOBuffer
#include "script.hpp"                 // Script

#include <any>         // std::any
#include <chrono>      // std::chrono::..., std::milli
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <cstdint>     // std::uint32_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <memory>      // std::unique_ptr, std::make_unique, std::shared_ptr, std::make_shared, std::weak_ptr, std::enable_shared_from_this
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move, std::forward
#include <vector>        // std::vector

class Game;
class GameServer;
class GameClient;
class MetaServer;
class MetaClient;
class VirtualMachine;
class ConVar;
class ConCommand;

class Process final : public std::enable_shared_from_this<Process> {
public:
	using UserFlags = std::uint8_t;
	enum : UserFlags {
		NO_FLAGS = 0,     // No flags.
		CONSOLE = 1 << 0, // Was launched from the console and is not a script file.
		SERVER = 1 << 1,  // Was launched by the game server.
		CLIENT = 1 << 2,  // Was launched by the game client.
		REMOTE = 1 << 3,  // Was launched remotely.
		ADMIN = 1 << 4,   // Has administrative privileges.
	};

	using Id = std::uint64_t;
	using ErrorHandler = std::function<bool(cmd::Result)>;

	static constexpr auto MAX_STACK_SIZE = std::size_t{1000};
	static constexpr auto MAX_ALIAS_DEPTH = std::size_t{100};
	static constexpr auto NO_FRAME = std::numeric_limits<std::size_t>::max();

	Process(VirtualMachine& vm, Id id, float startTime, UserFlags userFlags);

	auto clearLatestError() noexcept -> void;
	[[nodiscard]] auto getLatestError() noexcept -> std::optional<std::string_view>;

	[[nodiscard]] auto parent() noexcept -> std::weak_ptr<Process>;
	[[nodiscard]] auto getId() const noexcept -> Id;
	[[nodiscard]] auto getUserFlags() const noexcept -> UserFlags;
	[[nodiscard]] auto input() noexcept -> const std::shared_ptr<IOBuffer>&;
	[[nodiscard]] auto hasOutput() const noexcept -> bool;
	[[nodiscard]] auto getOutput() const noexcept -> std::optional<std::weak_ptr<IOBuffer>>;
	[[nodiscard]] auto getStartTime() const noexcept -> float;
	[[nodiscard]] auto running() const noexcept -> bool;
	[[nodiscard]] auto done() const noexcept -> bool;
	[[nodiscard]] auto getProgress() const -> float;

	[[nodiscard]] auto defined(const std::shared_ptr<Environment>& env, const std::string& name) const -> bool;
	[[nodiscard]] auto findObject(const std::shared_ptr<Environment>& env, const std::string& name) const -> Environment::Object*;

	[[nodiscard]] auto format(float currentTime) const -> std::string;

	[[nodiscard]] auto launchChildProcess(Process::UserFlags userFlags) -> std::shared_ptr<Process>;

	auto setErrorHandler(ErrorHandler errorHandler) -> void;

	auto resetOutput() noexcept -> void;
	auto setOutput(const std::shared_ptr<IOBuffer>& output) noexcept -> void;
	auto setOutputDone() noexcept -> void;

	[[nodiscard]] auto output(std::string_view str) -> bool;
	[[nodiscard]] auto outputln(std::string_view str) -> bool;

	[[nodiscard]] auto release() noexcept -> bool;
	auto end() noexcept -> void;

	[[nodiscard]] auto run(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient,
						   std::size_t targetFrameIndex = 0) -> cmd::Result;
	[[nodiscard]] auto await(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient,
							 std::size_t targetFrameIndex = 0) -> cmd::Result;
	[[nodiscard]] auto awaitUnlimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient,
									  std::size_t targetFrameIndex = 0) -> cmd::Result;
	[[nodiscard]] auto awaitLimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient,
									int limit, std::size_t targetFrameIndex = 0) -> cmd::Result;

	[[nodiscard]] auto call(std::shared_ptr<Environment> env, std::string_view script, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, cmd::CommandView argv, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, Script::Command command, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, Script commands, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, const Environment::Function& function, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, const Environment::Function& function, util::Span<const cmd::Value> args,
							std::size_t returnFrameIndex = NO_FRAME, std::size_t returnArgumentIndex = 0,
							const std::shared_ptr<Environment>& exportTarget = nullptr) -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, ConCommand& cmd, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, ConCommand& cmd, util::Span<const cmd::Value> args,
							std::size_t returnFrameIndex = NO_FRAME, std::size_t returnArgumentIndex = 0,
							const std::shared_ptr<Environment>& exportTarget = nullptr) -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, ConVar& cvar, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::shared_ptr<Environment> env, ConVar& cvar, std::string value, std::size_t returnFrameIndex = NO_FRAME,
							std::size_t returnArgumentIndex = 0, const std::shared_ptr<Environment>& exportTarget = nullptr)
		-> std::optional<CallFrameHandle>;

	friend auto operator==(const Process& lhs, const Process& rhs) noexcept -> bool;
	friend auto operator!=(const Process& lhs, const Process& rhs) noexcept -> bool;

private:
	friend CallFrameHandle;
	struct CallFrame final {
		CallFrame(std::shared_ptr<Environment> env, Script commands, std::size_t returnFrameIndex, std::size_t returnArgumentIndex,
				  const std::shared_ptr<Environment>& exportTarget)
			: commands(std::move(commands))
			, commandStates(this->commands.size())
			, env(std::move(env))
			, returnFrameIndex(returnFrameIndex)
			, returnArgumentIndex(returnArgumentIndex)
			, exportTarget(exportTarget) {}

		struct CommandState final {
			cmd::CommandArguments arguments;
			cmd::Progress progress = 0;
			std::any data;
			bool argsExpanded = false;
		};

		Script commands;                         // Commands to be executed.
		std::vector<CommandState> commandStates; // State of each command.
		std::shared_ptr<Environment> env;        // Local environment.
		std::size_t returnFrameIndex;            // Which call frame to return to.
		std::size_t returnArgumentIndex;         // Which argument in the return frame to return to.
		std::weak_ptr<Environment> exportTarget;
		std::size_t programCounter = 0;         // Current command.
		cmd::Status status = cmd::Status::NONE; // Current status.
		bool firstInTryBlock = false;
		bool firstInSection = false;
		bool executing = false;
	};

	[[nodiscard]] auto handleError(cmd::Result error) -> bool;
	auto setupPipeline(cmd::Result& result, CallFrame& frame) -> void;
	[[nodiscard]] auto expandArgs(std::size_t frameIndex) -> bool;
	[[nodiscard]] auto checkAliases(cmd::Result& result, const std::shared_ptr<Environment>& env, cmd::CommandArguments& arguments,
									std::size_t returnFrameIndex, std::size_t returnArgumentIndex) -> bool;
	[[nodiscard]] auto checkObjects(cmd::Result& result, const std::shared_ptr<Environment>& env, cmd::CommandArguments& arguments,
									std::size_t returnFrameIndex, std::size_t returnArgumentIndex) -> bool;
	[[nodiscard]] auto checkGlobals(cmd::Result& result, cmd::CommandArguments& arguments, std::any& data, std::size_t frameIndex, Game& game,
									GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) -> bool;

	util::Reference<VirtualMachine> m_vm;
	std::vector<CallFrame> m_callStack{};                             // Current call stack.
	std::shared_ptr<IOBuffer> m_input = std::make_shared<IOBuffer>(); // Received input.
	std::optional<std::weak_ptr<IOBuffer>> m_output{};                // If nullopt, output to virtual machine.
	float m_startTime;
	Id m_id;
	UserFlags m_userFlags;
	std::weak_ptr<Process> m_parent{};
	std::vector<std::shared_ptr<Process>> m_children{};
	std::optional<std::string> m_latestError{};
	ErrorHandler m_errorHandler{};
	bool m_running = false;
};

#endif
