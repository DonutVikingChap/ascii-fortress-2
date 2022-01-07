#include "con_command.hpp"

#include "../utilities/algorithm.hpp" // util::noneOf, util::transform
#include "../utilities/string.hpp"    // util::join
#include "convar.hpp"                 // ConVar...

#include <cassert>    // assert
#include <fmt/core.h> // fmt::format
#include <utility>    // std::move

auto ConCommand::all() -> std::unordered_map<std::string_view, ConCommand&>& {
	static std::unordered_map<std::string_view, ConCommand&> commands;
	return commands;
}

auto ConCommand::find(std::string_view name) -> ConCommand* {
	auto& commands = ConCommand::all();

	const auto it = commands.find(name);
	return (it == commands.end()) ? nullptr : &it->second;
}

ConCommand::ConCommand(std::string name, std::string parameters, Flags flags, std::string description, std::vector<cmd::OptionSpec> options,
					   Suggestions::Function suggestionFunc, Function function)
	: m_name(std::move(name))
	, m_parameters(std::move(parameters))
	, m_flags(flags)
	, m_description(std::move(description))
	, m_options(std::move(options))
	, m_suggestionFunc(suggestionFunc)
	, m_function(function) {
	assert(m_function);
	assert(util::noneOf(this->getName(), Script::isWhitespace));
	assert(ConVar::all().count(this->getName()) == 0);
	assert(ConCommand::all().count(this->getName()) == 0);
	ConCommand::all().emplace(this->getName(), *this);
}

ConCommand::~ConCommand() {
	ConCommand::all().erase(this->getName());
}

auto ConCommand::getName() const noexcept -> std::string_view {
	return m_name;
}

auto ConCommand::getParameters() const noexcept -> std::string_view {
	return m_parameters;
}

auto ConCommand::getFlags() const noexcept -> ConCommand::Flags {
	return m_flags;
}

auto ConCommand::getDescription() const noexcept -> std::string_view {
	return m_description;
}

auto ConCommand::getOptions() const noexcept -> util::Span<const cmd::OptionSpec> {
	return m_options;
}

auto ConCommand::getUsage() const -> std::string {
	if (m_parameters.empty()) {
		return fmt::format("Usage: {}", m_name);
	}
	return fmt::format("Usage: {} {}", m_name, m_parameters);
}

auto ConCommand::formatFlags() const -> std::string {
	auto flags = std::vector<std::string>{};
	if ((m_flags & CHEAT) != 0) {
		flags.emplace_back("cheat");
	}
	if ((m_flags & ADMIN_ONLY) != 0) {
		flags.emplace_back("admin only");
	}
	if ((m_flags & NO_RCON) != 0) {
		flags.emplace_back("no rcon");
	}
	if ((m_flags & SERVER) != 0) {
		flags.emplace_back("server");
	}
	if ((m_flags & CLIENT) != 0) {
		flags.emplace_back("client");
	}
	if ((m_flags & META_SERVER) != 0) {
		flags.emplace_back("meta server");
	}
	if ((m_flags & META_CLIENT) != 0) {
		flags.emplace_back("meta client");
	}
	return flags | util::join(", ");
}

auto ConCommand::formatOptions() const -> std::string {
	static constexpr auto formatOption = [](const auto& opt) {
		if (opt.type == cmd::OptionType::ARGUMENT_REQUIRED) {
			return fmt::format("  -{} --{:<16} {}", opt.name, fmt::format("{} <value>", opt.longName), opt.description);
		}
		return fmt::format("  -{} --{:<16} {}", opt.name, opt.longName, opt.description);
	};

	return m_options | util::transform(formatOption) | util::join('\n');
}

auto ConCommand::format(bool flags, bool description, bool options) const -> std::string {
	auto str = std::string{m_name};
	if (!m_parameters.empty()) {
		str.append(fmt::format(" {}", m_parameters));
	}
	if (flags && m_flags != NO_FLAGS) {
		str.append(fmt::format(" ({})", this->formatFlags()));
	}
	if (description) {
		str.append(fmt::format(": {}", m_description));
	}
	if (options && !m_options.empty()) {
		str.append(
			fmt::format("\n"
						"Options:\n"
						"{}",
						this->formatOptions()));
	}
	return str;
}

auto ConCommand::getSuggestions(const Script::Command& command, std::size_t i, Game& game, GameServer* server, GameClient* client,
								MetaServer* metaServer, MetaClient* metaClient, VirtualMachine& vm) const -> Suggestions {
	return (m_suggestionFunc) ? m_suggestionFunc(*this, command, i, game, server, client, metaServer, metaClient, vm) : Suggestions{};
}

auto ConCommand::execute(cmd::CommandView argv, std::any& data, const CallFrameHandle& frame, Game& game, GameServer* server,
						 GameClient* client, MetaServer* metaServer, MetaClient* metaClient, VirtualMachine& vm) -> cmd::Result {
	assert(m_function);
	return m_function(*this, argv, data, frame, game, server, client, metaServer, metaClient, vm);
}
