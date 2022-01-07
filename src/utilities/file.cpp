#include "file.hpp"

#include <algorithm>    // std::equal
#include <filesystem>   // std::filesystem::...
#include <fmt/core.h>   // fmt::format
#include <fstream>      // std::ifstream, std::ofstream
#include <iterator>     // std::distance
#include <sstream>      // std::ostringstream
#include <system_error> // std::error_code

namespace util {

auto readFile(std::string_view filepath, std::ios::openmode mode) noexcept -> std::optional<std::string> {
	try {
		if (auto file = std::ifstream{std::string{filepath}, mode}) {
			auto ss = std::ostringstream{};
			ss << file.rdbuf();
			return ss.str();
		}
	} catch (...) {
	}
	return std::nullopt;
}

auto dumpFile(std::string_view filepath, std::string_view text, std::ios::openmode mode) noexcept -> bool {
	try {
		auto ec = std::error_code{};

		const auto path = std::filesystem::path{filepath};
		std::filesystem::create_directories(path.parent_path(), ec);
		if (ec) {
			return false;
		}

		if (auto file = std::ofstream{path, mode}) {
			file.write(text.data(), static_cast<std::streamsize>(text.size()));
			return true;
		}
	} catch (...) {
	}
	return false;
}

auto uniqueFilePath(std::string_view path, std::string_view extension) -> std::string {
	auto ec = std::error_code{};

	const auto originalPath = std::filesystem::path{path};
	const auto desiredExtension = std::filesystem::path{extension};

	auto result = originalPath;
	result.replace_extension(desiredExtension);
	for (auto i = 1u; std::filesystem::exists(result, ec) && !ec; ++i) {
		// Try the same path, but with an increasing number appended to the filename.
		// Note: We could have calculated an "originalPathWithoutExtension" before the loop,
		// but that would only optimize for this rare case of the file already existing.
		result = originalPath;
		result.replace_extension();
		result += fmt::format("_{}", i);
		result.replace_extension(desiredExtension);
	}
	return result.string();
}

auto pathIsBelowDirectory(std::string_view path, std::string_view directory) -> bool {
	auto ec = std::error_code{};

	auto canonicalDir = (std::filesystem::weakly_canonical(directory, ec) / "").parent_path();
	if (ec) {
		return false;
	}

	const auto canonicalPath = (std::filesystem::weakly_canonical(path, ec) / "").parent_path();
	if (ec) {
		return false;
	}

	if (std::distance(canonicalDir.begin(), canonicalDir.end()) >= std::distance(canonicalPath.begin(), canonicalPath.end())) {
		return false;
	}
	return std::equal(canonicalDir.begin(), canonicalDir.end(), canonicalPath.begin());
}

} // namespace util
