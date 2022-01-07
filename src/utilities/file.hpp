#ifndef AF2_UTILITIES_FILE_HPP
#define AF2_UTILITIES_FILE_HPP

#include <ios>         // std::ios, std::streamsize
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace util {

[[nodiscard]] auto readFile(std::string_view filepath, std::ios::openmode mode = std::ios::in) noexcept -> std::optional<std::string>;

[[nodiscard]] auto dumpFile(std::string_view filepath, std::string_view text, std::ios::openmode mode = std::ios::trunc) noexcept -> bool;

[[nodiscard]] auto uniqueFilePath(std::string_view path, std::string_view extension) -> std::string;

[[nodiscard]] auto pathIsBelowDirectory(std::string_view path, std::string_view directory) -> bool;

} // namespace util

#endif
