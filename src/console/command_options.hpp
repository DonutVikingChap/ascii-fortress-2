#ifndef AF2_CONSOLE_COMMAND_OPTIONS_HPP
#define AF2_CONSOLE_COMMAND_OPTIONS_HPP

#include "../utilities/span.hpp" // util::Span
#include "command.hpp"           // cmd::Value, cmd::CommandView

#include <cstddef>       // std::size_t
#include <optional>      // std::optional
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <utility>       // std::forward, std::move, std::pair
#include <vector>        // std::vector

namespace cmd {

enum class OptionType : std::uint8_t { NO_ARGUMENT, ARGUMENT_REQUIRED };

class Option final {
public:
	constexpr Option() noexcept = default;
	constexpr explicit Option(std::optional<std::string_view> option) noexcept
		: m_option(option) {}

	[[nodiscard]] constexpr auto isSet() const noexcept -> bool {
		return m_option.has_value();
	}

	[[nodiscard]] constexpr auto valueOr(std::string_view defaultValue) const noexcept -> std::string_view {
		if (m_option) {
			return *m_option;
		}
		return defaultValue;
	}

	[[nodiscard]] constexpr auto valueOrEmpty() const noexcept -> std::string_view {
		return this->valueOr(std::string_view{});
	}

	constexpr operator bool() const noexcept {
		return m_option.has_value();
	}

	constexpr auto operator*() const -> const std::string_view& {
		return *m_option;
	}

	constexpr auto operator->() const -> const std::string_view* {
		return m_option.operator->();
	}

private:
	std::optional<std::string_view> m_option{};
};

struct OptionSpec final {
	const char name;
	const std::string_view longName;
	const std::string description;
	const OptionType type;

	OptionSpec(char name, std::string_view longName, std::string description, OptionType type)
		: name(name)
		, longName(longName)
		, description(std::move(description))
		, type(type) {}
};

class Options final {
public:
	auto clear() noexcept -> void {
		m_options.clear();
	}
	auto clear(char name) -> bool {
		return m_options.erase(name) != 0;
	}

	auto set(char name) -> void {
		m_options[name] = "1";
	}

	auto set(char name, std::string_view value) -> void {
		m_options[name] = value;
	}

	auto setError(std::string error) -> void {
		m_options.clear();
		m_error = std::move(error);
	}

	[[nodiscard]] auto error() const noexcept -> const std::optional<std::string>& {
		return m_error;
	}

	[[nodiscard]] auto operator[](char name) const -> Option {
		if (const auto it = m_options.find(name); it != m_options.end()) {
			return Option{it->second};
		}
		return Option{};
	}

private:
	std::unordered_map<char, std::string_view> m_options{};
	std::optional<std::string> m_error{};
};

[[nodiscard]] inline auto opt(char name, std::string_view longName, std::string description, OptionType type = OptionType::NO_ARGUMENT) -> OptionSpec {
	return OptionSpec{name, longName, std::move(description), type};
}

template <typename... Args>
[[nodiscard]] inline auto opts(Args&&... args) -> std::vector<OptionSpec> {
	return std::vector<OptionSpec>{std::forward<Args>(args)...};
}

[[nodiscard]] auto isOption(const Value& arg) noexcept -> bool;
[[nodiscard]] auto isSpecificOption(const Value& arg, char name, std::string_view longName) noexcept -> bool;

[[nodiscard]] auto optc(CommandView argv) noexcept -> std::size_t;
[[nodiscard]] auto argc(CommandView argv, util::Span<const OptionSpec> optionSpecs, std::size_t offset = 1) noexcept -> std::size_t;

[[nodiscard]] auto parse(CommandView argv, util::Span<const OptionSpec> optionSpecs, std::size_t offset = 1)
	-> std::pair<std::vector<std::string_view>, Options>;

} // namespace cmd

#endif
