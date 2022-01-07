#ifndef AF2_UTILITIES_TIME_HPP
#define AF2_UTILITIES_TIME_HPP

#include <ctime>        // std::time, std::tm, localtime_s
#include <fmt/chrono.h> // fmt::format
#include <string>       // std::string
#include <string_view>  // std::string_view
#ifndef _WIN32
#include <time.h> // localtime_r // NOLINT(modernize-deprecated-headers)
#endif

namespace util {

[[nodiscard]] inline auto getLocalTimeStr(std::string_view format) -> std::string {
	const auto t = std::time(nullptr);

	auto localTime = std::tm{};
#ifdef _WIN32
	localtime_s(&localTime, &t);
#else
	localtime_r(&t, &localTime);
#endif
	return fmt::format(fmt::format("{{:{}}}", format), localTime);
}

} // namespace util

#endif
