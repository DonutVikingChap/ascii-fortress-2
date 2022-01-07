#ifndef AF2_CONSOLE_CON_COMMAND_HPP
#define AF2_CONSOLE_CON_COMMAND_HPP

#include "../utilities/span.hpp" // util::Span
#include "command.hpp"           // cmd::...
#include "command_options.hpp"   // cmd::OptionSpec
#include "script.hpp"            // Script
#include "suggestions.hpp"       // Suggestions

#include <any>           // std::any
#include <cstddef>       // std::size_t
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

struct Environment;
class VirtualMachine;
class CallFrameHandle;
class Game;
class GameServer;
class GameClient;
class MetaServer;
class MetaClient;
class Process;

class ConCommand final {
public:
	[[nodiscard]] static auto all() -> std::unordered_map<std::string_view, ConCommand&>&;
	[[nodiscard]] static auto find(std::string_view name) -> ConCommand*;

	using Flags = std::uint8_t;
	enum Flag : Flags {
		NO_FLAGS = 0,
		ALL_FLAGS = static_cast<Flags>(~0),
		CHEAT = 1 << 0,       // Using the command is considered a cheat.
		ADMIN_ONLY = 1 << 1,  // Command may only be used by admins.
		NO_RCON = 1 << 2,     // Command may not be used remotely.
		SERVER = 1 << 3,      // Command can only be used if the host is running a game server.
		CLIENT = 1 << 4,      // Command can only be used if the host is running a game client.
		META_SERVER = 1 << 5, // Command can only be used if the host is running a meta server.
		META_CLIENT = 1 << 6, // Command can only be used if the host is running a meta client.
	};
	using Function = cmd::Result (*)(ConCommand& self, cmd::CommandView argv, std::any& data, const CallFrameHandle& frame, Game& game,
									 GameServer* server, GameClient* client, MetaServer* metaServer, MetaClient* metaClient, VirtualMachine& vm);

	ConCommand(std::string name, std::string parameters, Flags flags, std::string description, std::vector<cmd::OptionSpec> options,
			   Suggestions::Function suggestionFunc, Function function);

	~ConCommand();

	ConCommand(const ConCommand&) = delete;
	ConCommand(ConCommand&&) = delete;

	auto operator=(const ConCommand&) -> ConCommand& = delete;
	auto operator=(ConCommand &&) -> ConCommand& = delete;

	[[nodiscard]] auto getName() const noexcept -> std::string_view;
	[[nodiscard]] auto getParameters() const noexcept -> std::string_view;
	[[nodiscard]] auto getFlags() const noexcept -> Flags;
	[[nodiscard]] auto getDescription() const noexcept -> std::string_view;
	[[nodiscard]] auto getOptions() const noexcept -> util::Span<const cmd::OptionSpec>;

	[[nodiscard]] auto getUsage() const -> std::string;

	[[nodiscard]] auto formatFlags() const -> std::string;
	[[nodiscard]] auto formatOptions() const -> std::string;
	[[nodiscard]] auto format(bool flags, bool description, bool options) const -> std::string;

	[[nodiscard]] auto getSuggestions(const Script::Command& command, std::size_t i, Game& game, GameServer* server, GameClient* client,
									  MetaServer* metaServer, MetaClient* metaClient, VirtualMachine& vm) const -> Suggestions;

private:
	friend Process;
	[[nodiscard]] auto execute(cmd::CommandView argv, std::any& data, const CallFrameHandle& frame, Game& game, GameServer* server,
							   GameClient* client, MetaServer* metaServer, MetaClient* metaClient, VirtualMachine& vm) -> cmd::Result;

	std::string m_name;
	std::string m_parameters;
	Flags m_flags;
	std::string m_description;
	std::vector<cmd::OptionSpec> m_options;
	Suggestions::Function m_suggestionFunc;
	Function m_function;
};

#define CON_COMMAND(name, params, flags, description, options, suggestions) \
	auto cmd_##name##_f(ConCommand& self, \
						cmd::CommandView argv, \
						std::any& data, \
						const CallFrameHandle& frame, \
						Game& game, \
						GameServer* server, \
						GameClient* client, \
						MetaServer* metaServer, \
						MetaClient* metaClient, \
						VirtualMachine& vm) \
		->cmd::Result; \
	ConCommand cmd_##name{#name, params, flags, description, options, suggestions, cmd_##name##_f}; \
	auto cmd_##name##_f([[maybe_unused]] ConCommand& self, \
						[[maybe_unused]] cmd::CommandView argv, \
						[[maybe_unused]] std::any& data, \
						[[maybe_unused]] const CallFrameHandle& frame, \
						[[maybe_unused]] Game& game, \
						[[maybe_unused]] GameServer* server, \
						[[maybe_unused]] GameClient* client, \
						[[maybe_unused]] MetaServer* metaServer, \
						[[maybe_unused]] MetaClient* metaClient, \
						[[maybe_unused]] VirtualMachine& vm) \
		->cmd::Result

#define CON_COMMAND_EXTERN(name) extern ConCommand cmd_##name
#define GET_COMMAND(name)        cmd_##name

#endif
