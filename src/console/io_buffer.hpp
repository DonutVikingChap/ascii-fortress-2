#ifndef AF2_CONSOLE_IO_BUFFER_HPP
#define AF2_CONSOLE_IO_BUFFER_HPP

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

class IOBuffer final {
public:
	[[nodiscard]] auto canRead() const noexcept -> bool;
	[[nodiscard]] auto isDone() const noexcept -> bool;

	auto setDone(bool done) noexcept -> void;

	auto write(std::string_view str) -> void;
	auto writeln(std::string_view str) -> void;

	[[nodiscard]] auto read() -> std::optional<std::string>;
	[[nodiscard]] auto readln() -> std::optional<std::string>;

private:
	std::optional<std::string> m_str{};
	bool                       m_done = true;
};

#endif
