#ifndef AF2_CONSOLE_SCRIPT_HPP
#define AF2_CONSOLE_SCRIPT_HPP

#include <array>            // std::array
#include <cstddef>          // std::size_t, std::ptrdiff_t
#include <cstdint>          // std::uint8_t
#include <initializer_list> // std::initializer_list
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <utility>          // std::move, std::forward
#include <vector>           // std::vector

class Script final {
public:
	struct Argument final {
		using Flags = std::uint8_t;
		enum Flag : Flags {
			NO_FLAGS = 0,
			ALL = static_cast<Flags>(~0),
			EXEC = 1 << 0,   // This argument should be executed as a command before being used.
			EXPAND = 1 << 1, // This argument should be expanded into (possibly) several arguments.
			PIPE = 1 << 2    // This argument marks a pipe between this and the following command.
		};

		Argument() noexcept = default;

		explicit Argument(std::string value, Flags flags = NO_FLAGS)
			: value(std::move(value))
			, flags(flags) {}

		std::string value{};
		Flags flags = NO_FLAGS;
	};
	using Command = std::vector<Argument>;

	static constexpr auto HEX_DIGITS = std::array<const char, 16>{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	[[nodiscard]] static constexpr auto hexValue(char ch) noexcept -> char {
		return (ch >= 'A') ? (ch >= 'a') ? static_cast<char>(ch - 'a' + 10) : static_cast<char>(ch - 'A' + 10) : static_cast<char>(ch - '0');
	}

	[[nodiscard]] static constexpr auto isPrintableChar(char ch) noexcept -> bool {
		return ch >= ' ' && ch <= '~';
	}

	[[nodiscard]] static constexpr auto isWhitespace(char ch) noexcept -> bool {
		return ch == ' ' || ch == '\t';
	}

	[[nodiscard]] static constexpr auto isCommandSeparator(char ch) noexcept -> bool {
		return ch == ';' || ch == '\n';
	}

	[[nodiscard]] static auto parse(std::string_view script) -> Script;

	[[nodiscard]] static auto escapedString(std::string_view str) -> std::string;
	[[nodiscard]] static auto argumentString(const Argument& argument) -> std::string;
	[[nodiscard]] static auto commandString(const Command& command) -> std::string;
	[[nodiscard]] static auto scriptString(const Script& commands) -> std::string;

	[[nodiscard]] static auto command(std::initializer_list<std::string_view> init) -> Command;
	[[nodiscard]] static auto subCommand(Command command, std::size_t offset, std::size_t count) -> Command;
	[[nodiscard]] static auto subCommand(Command command, std::size_t offset) -> Command;

	Script() noexcept = default;

	explicit Script(std::vector<Command> commands)
		: m_commands(std::move(commands)) {}

	Script(std::initializer_list<Command> init)
		: m_commands(init) {}

	template <typename... Args>
	auto emplace_back(Args&&... args) -> decltype(auto) {
		return m_commands.emplace_back(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto push_back(Args&&... args) -> decltype(auto) {
		return m_commands.push_back(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto insert(Args&&... args) -> decltype(auto) {
		return m_commands.insert(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto erase(Args&&... args) -> decltype(auto) {
		return m_commands.erase(std::forward<Args>(args)...);
	}

	[[nodiscard]] auto front() -> decltype(auto) {
		return m_commands.front();
	}

	[[nodiscard]] auto front() const -> decltype(auto) {
		return m_commands.front();
	}

	[[nodiscard]] auto back() -> decltype(auto) {
		return m_commands.back();
	}

	[[nodiscard]] auto back() const -> decltype(auto) {
		return m_commands.back();
	}

	[[nodiscard]] auto begin() noexcept -> decltype(auto) {
		return m_commands.begin();
	}

	[[nodiscard]] auto begin() const noexcept -> decltype(auto) {
		return m_commands.begin();
	}

	[[nodiscard]] auto end() noexcept -> decltype(auto) {
		return m_commands.end();
	}

	[[nodiscard]] auto end() const noexcept -> decltype(auto) {
		return m_commands.end();
	}

	[[nodiscard]] auto size() const noexcept -> decltype(auto) {
		return m_commands.size();
	}

	[[nodiscard]] auto empty() const noexcept -> decltype(auto) {
		return m_commands.empty();
	}

	[[nodiscard]] auto clear() noexcept -> decltype(auto) {
		return m_commands.clear();
	}

	template <typename Index>
	[[nodiscard]] auto operator[](Index&& i) -> decltype(auto) {
		return m_commands[std::forward<Index>(i)];
	}

	template <typename Index>
	[[nodiscard]] auto operator[](Index&& i) const -> decltype(auto) {
		return m_commands[std::forward<Index>(i)];
	}

private:
	std::vector<Command> m_commands{};
};

#endif
