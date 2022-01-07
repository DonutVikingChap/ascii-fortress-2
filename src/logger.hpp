#ifndef AF2_LOGGER_HPP
#define AF2_LOGGER_HPP

#include <string_view> // std::string_view

namespace logger {

[[nodiscard]] auto open() -> bool;

auto close() -> void;

auto logInfo(std::string_view str) -> void;
auto logWarning(std::string_view str) -> void;
auto logError(std::string_view str) -> void;
auto logFatalError(std::string_view str) -> void;

} // namespace logger

#endif
