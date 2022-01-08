#ifndef AF2_CONSOLE_CALL_FRAME_HANDLE_HPP
#define AF2_CONSOLE_CALL_FRAME_HANDLE_HPP

#include "../utilities/span.hpp" // util::Span
#include "command.hpp"           // cmd::...
#include "environment.hpp"       // Environment
#include "script.hpp"            // Script

#include <any>         // std::any
#include <cstddef>     // std::size_t
#include <memory>      // std::shared_ptr
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

class Game;
class GameServer;
class GameClient;
class MetaServer;
class MetaClient;
class ConVar;
class ConCommand;
class Process;
struct Environment;

class CallFrameHandle final {
public:
	CallFrameHandle(std::shared_ptr<Process> process, std::size_t frameIndex) noexcept;

	[[nodiscard]] auto process() const noexcept -> const std::shared_ptr<Process>&;
	[[nodiscard]] auto index() const noexcept -> std::size_t;
	[[nodiscard]] auto env() const noexcept -> const std::shared_ptr<Environment>&;
	[[nodiscard]] auto retFrame() const noexcept -> std::size_t;
	[[nodiscard]] auto retArg() const noexcept -> std::size_t;
	[[nodiscard]] auto status() const noexcept -> cmd::Status;
	[[nodiscard]] auto progress() const noexcept -> cmd::Progress;
	[[nodiscard]] auto arguments() const noexcept -> cmd::CommandArguments&;
	[[nodiscard]] auto data() const noexcept -> std::any&;
	[[nodiscard]] auto getExportTarget() const noexcept -> std::shared_ptr<Environment>;

	auto makeTryBlock() const noexcept -> void;
	auto makeSection() const noexcept -> void;
	auto resetExportTarget() const noexcept -> void;
	auto setExportTarget(const std::shared_ptr<Environment>& exportTarget) const noexcept -> void;

	[[nodiscard]] auto run(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) const -> cmd::Result;
	[[nodiscard]] auto await(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) const -> cmd::Result;
	[[nodiscard]] auto awaitUnlimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient) const
		-> cmd::Result;
	[[nodiscard]] auto awaitLimited(Game& game, GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient,
	                                int limit) const -> cmd::Result;

	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, std::string_view script) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, cmd::CommandView argv) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, Script::Command command) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, Script commands) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, const Environment::Function& function) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, const Environment::Function& function,
	                        util::Span<const cmd::Value> args) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConCommand& cmd) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConCommand& cmd,
	                        util::Span<const cmd::Value> args) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConVar& cvar) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto call(std::size_t returnArgumentIndex, std::shared_ptr<Environment> env, ConVar& cvar, std::string value) const
		-> std::optional<CallFrameHandle>;

	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, std::string_view script) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, cmd::CommandView argv) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, Script::Command command) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, Script commands) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, const Environment::Function& function) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, const Environment::Function& function, util::Span<const cmd::Value> args) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, ConCommand& cmd) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, ConCommand& cmd, util::Span<const cmd::Value> args) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, ConVar& cvar) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto tailCall(std::shared_ptr<Environment> env, ConVar& cvar, std::string value) const -> std::optional<CallFrameHandle>;

	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, std::string_view script) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, cmd::CommandView argv) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, Script::Command command) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, Script commands) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, const Environment::Function& function) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, const Environment::Function& function, util::Span<const cmd::Value> args) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, ConCommand& cmd) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, ConCommand& cmd, util::Span<const cmd::Value> args) const
		-> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, ConVar& cvar) const -> std::optional<CallFrameHandle>;
	[[nodiscard]] auto callDiscard(std::shared_ptr<Environment> env, ConVar& cvar, std::string value) const -> std::optional<CallFrameHandle>;

private:
	friend Process;
	std::shared_ptr<Process> m_process;
	std::size_t m_frameIndex;
};

#endif
