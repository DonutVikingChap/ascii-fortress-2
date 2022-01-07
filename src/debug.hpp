#ifndef AF2_DEBUG_HPP
#define AF2_DEBUG_HPP

#include "logger.hpp" // logger::logInfo

#include <cstddef>    // std::size_t
#include <fmt/core.h> // fmt::...
#include <utility>    // std::forward

struct Msg final {
	Msg() = delete;

	using Filter = int;
	enum : Filter {
		NONE = 0,                     // No messages.
		ALL = ~0,                     // All messages.
		GENERAL = 1 << 0,             // General info.
		CONNECTION_EVENT = 1 << 1,    // Connection-related messages.
		CONNECTION_DETAILED = 1 << 2, // Detailed connection-related messages.
		CONNECTION_DELTA = 1 << 3,    // Delta-encoding-related messages.
		CONNECTION_CRYPTO = 1 << 4,   // Encryption/decryption-related messages.
		SERVER_TICK = 1 << 5,         // Messages related to server updates.
		CLIENT_TICK = 1 << 6,         // Messages related to client updates.
		INPUT_MANAGER = 1 << 7,       // Input-related messages.
		CHAT = 1 << 8,                // User chat messages.
		CLIENT = 1 << 9,              // Client info messages.
		SERVER = 1 << 10,             // Server info messages.
		CONSOLE_EVENT = 1 << 11,      // Console-related messages.
		CONSOLE_OUTPUT = 1 << 12,     // Messages printed to the console.
		CONSOLE_DETAILED = 1 << 13,   // Detailed console-related messages.
		CONVAR_EVENT = 1 << 14,       // Cvar-related messages.
		RCON = 1 << 15,               // Remote console-related messages.
	};

#ifndef NDEBUG
	static constexpr auto ACTIVE_FILTER = Filter{GENERAL | CONNECTION_EVENT | INPUT_MANAGER | CHAT | CLIENT | SERVER | CONSOLE_EVENT |
												 CONSOLE_OUTPUT | RCON};
#else
	static constexpr auto ACTIVE_FILTER = Filter{ALL};
#endif
};

#ifndef NDEBUG
#include <filesystem>  // std::filesystem::path
#include <string_view> // std::string_view
#include <typeinfo>    // std::type_info

namespace debug {
namespace detail {

auto indent() noexcept -> std::size_t&;

inline auto debugMsgImpl(const char* func, const char* file, int line, std::string_view format_str, fmt::format_args args) {
	logger::logInfo(fmt::format("{:<40}: {:<{}}{}",
								fmt::format("({}, {}:{})", func, std::filesystem::path{file}.filename().generic_string(), line),
								"",
								indent() * 4,
								fmt::vformat(format_str, args)));
}

template <Msg::Filter MESSAGE_FILTER, std::size_t N, typename... Args>
inline auto debugMsg([[maybe_unused]] const char* func, [[maybe_unused]] const char* file, [[maybe_unused]] int line,
					 [[maybe_unused]] const char (&format_str)[N], [[maybe_unused]] Args&&... args) -> void {
	if constexpr ((MESSAGE_FILTER & Msg::ACTIVE_FILTER) != Msg::NONE) {
		debug::detail::debugMsgImpl(func, file, line, format_str, fmt::make_format_args(std::forward<Args>(args)...));
	}
}

template <Msg::Filter MESSAGE_FILTER>
struct Indenter final {
	Indenter() noexcept {
		if constexpr ((MESSAGE_FILTER & Msg::ACTIVE_FILTER) != Msg::NONE) {
			++debug::detail::indent();
		}
	}

	~Indenter() {
		if constexpr ((MESSAGE_FILTER & Msg::ACTIVE_FILTER) != Msg::NONE) {
			--debug::detail::indent();
		}
	}

	Indenter(const Indenter&) = delete;
	Indenter(Indenter&&) = delete;

	auto operator=(const Indenter&) -> Indenter& = delete;
	auto operator=(Indenter &&) -> Indenter& = delete;
};

} // namespace detail
} // namespace debug

#define DEBUG_MSG_INDENT(filter, ...) \
	DEBUG_MSG(filter, __VA_ARGS__); \
	if (const auto debugMsgIndenter = debug::detail::Indenter<(filter)>{}; true)

#define DEBUG_MSG(filter, ...) debug::detail::debugMsg<(filter)>(__func__, __FILE__, __LINE__, __VA_ARGS__)

#define INFO_MSG(filter, ...) DEBUG_MSG(filter, __VA_ARGS__)

#define INFO_MSG_INDENT(filter, ...) \
	INFO_MSG(filter, __VA_ARGS__); \
	if (const auto debugMsgIndenter = debug::detail::Indenter<(filter)>{}; true)

#define DEBUG_TYPE_NAME(type) \
	[]() -> std::string_view { \
		const auto str = std::string_view{typeid(type).name()}; \
		if (str.compare(0, 7, "struct ") == 0) { \
			return str.substr(7); \
		} \
		if (str.compare(0, 6, "class ") == 0) { \
			return str.substr(6); \
		} \
		return str; \
	}()

#define DEBUG_TYPE_NAME_ONLY(type) \
	[]() -> std::string_view { \
		const auto str = DEBUG_TYPE_NAME(type); \
		const auto iLastColon = str.rfind(':'); \
		if (iLastColon != std::string_view::npos && iLastColon + 1 < str.size()) { \
			return str.substr(iLastColon + 1); \
		} \
		return str; \
	}()
#else
namespace detail {

inline auto infoMsgImpl(std::string_view format_str, fmt::format_args args) -> void {
	logger::logInfo(fmt::vformat(format_str, args));
}

template <Msg::Filter MESSAGE_FILTER, std::size_t N, typename... Args>
inline auto infoMsg([[maybe_unused]] const char (&format_str)[N], [[maybe_unused]] Args&&... args) -> void {
	if constexpr ((MESSAGE_FILTER & Msg::ACTIVE_FILTER) != Msg::NONE) {
		detail::infoMsgImpl(format_str, fmt::make_format_args(std::forward<Args>(args)...));
	}
}

} // namespace detail
#define DEBUG_MSG_INDENT(filter, ...) if constexpr (true)

#define DEBUG_MSG(filter, ...) ((void)0)

#define INFO_MSG(filter, ...) detail::infoMsg<(filter)>(__VA_ARGS__)

#define INFO_MSG_INDENT(filter, ...) \
	INFO_MSG(filter, __VA_ARGS__); \
	if constexpr (true)

#define DEBUG_TYPE_NAME(type) ""

#define DEBUG_TYPE_NAME_ONLY(type) ""

#endif

#endif
