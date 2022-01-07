#include "logger.hpp"

#include "console/commands/file_commands.hpp"   // data_dir, data_subdir_logs
#include "console/commands/game_commands.hpp"   // cvar_game, game_version
#include "console/commands/logger_commands.hpp" // log_file_limit, log_debug_output, log_max_size, log_flush, log_debug_break_on_error
#include "debug.hpp"                            // Msg, INFO_MSG
#include "utilities/time.hpp"                   // util::getLocalTimeStr

#include <algorithm>  // std::sort
#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <filesystem> // std::filesystem::...
#include <fmt/core.h> // fmt::format
#include <fstream>    // std::ofstream
#include <ios>        // std::ios
#include <limits>     // std::numeric_limits
#include <string>     // std::string
#include <utility>    // std::move
#include <vector>     // std::vector

#ifndef NDEBUG
#include <iostream> // std::clog
#endif

#ifdef _WIN32
#include <windows.h> // MessageBoxA, MB_OK, MB_ICONERROR
#endif

class Logger final {
public:
	[[nodiscard]] auto open(std::string directory, std::string name) -> bool {
		m_directory = std::move(directory);
		m_name = std::move(name);
		return this->open();
	}

	auto close() -> void {
		m_file.close();
	}

	auto output(std::string_view str) -> void {
		if (m_file) {
			this->write(str);
		} else {
			m_buffer.append(str);
		}
	}

	[[nodiscard]] static auto global() -> Logger& {
		static auto globalLogger = Logger{};
		return globalLogger;
	}

private:
	auto open() -> bool {
		if (log_file_limit > 0) {
			auto logFiles = std::vector<std::filesystem::path>{};
			try {
				for (const auto& logFile : std::filesystem::directory_iterator{m_directory}) {
					if (auto path = logFile.path(); path.filename().generic_string().compare(0, m_name.size(), m_name) == 0) {
						logFiles.push_back(std::move(path));
					}
				}
			} catch (const std::filesystem::filesystem_error&) {
				return false;
			}

			std::sort(logFiles.begin(), logFiles.end(), [](const auto& lhs, const auto& rhs) { return lhs.filename() > rhs.filename(); });

			while (logFiles.size() >= static_cast<std::size_t>(log_file_limit)) {
				if (std::error_code ec; std::filesystem::remove(logFiles.back(), ec)) {
					INFO_MSG(Msg::GENERAL, "Removed old log file \"{}\".", logFiles.back().filename().generic_string());
				}
				logFiles.pop_back();
			}
		}

#ifndef NDEBUG
		const auto filepath = fmt::format("{}/{}_debug_{}.txt", m_directory, m_name, util::getLocalTimeStr("%Y-%m-%dT%H.%M.%S"));
#else
		const auto filepath = fmt::format("{}/{}_{}.txt", m_directory, m_name, util::getLocalTimeStr("%Y-%m-%dT%H.%M.%S"));
#endif

		const auto wasOpen = m_file.is_open();
		if (wasOpen) {
			m_file.close();
			m_written = 0;
		}

		auto ec = std::error_code{};
		std::filesystem::create_directories(std::filesystem::path{filepath}.parent_path(), ec);
		if (ec) {
			return false;
		}

		m_file.open(filepath, std::ios::trunc);
		if (!m_file) {
			return false;
		}

		if (!wasOpen) {
			this->write(m_buffer);
			m_buffer.clear();
		}
		return true;
	}

	auto write(std::string_view str) -> void {
		assert(m_file);
#ifndef NDEBUG
		if (log_debug_output) {
			std::clog << str;
		}
#endif
		if (log_max_size > 0 && m_written + str.size() > static_cast<std::size_t>(log_max_size)) {
			this->open();
		}
		m_written += str.size();
		m_file << str;
		if (log_flush) {
			m_file.flush();
		}
	}

	std::ofstream m_file{};
	std::string m_buffer{};
	std::string m_directory{};
	std::string m_name{};
	std::size_t m_written = 0;
};

namespace logger {

auto open() -> bool {
	return Logger::global().open(fmt::format("{}/{}", data_dir, data_subdir_logs), fmt::format("log_{}_{}", cvar_game, game_version));
}

auto close() -> void {
	Logger::global().close();
}

auto logInfo(std::string_view str) -> void {
	const auto message = fmt::format("[{} INFO]: {}\n", util::getLocalTimeStr("%Y-%m-%dT%H:%M:%S"), str);
	Logger::global().output(message);
}

auto logWarning(std::string_view str) -> void {
	const auto message = fmt::format("[{} WARNING]: {}\n", util::getLocalTimeStr("%Y-%m-%dT%H:%M:%S"), str);
	Logger::global().output(message);
}

auto logError(std::string_view str) -> void {
#ifdef _WIN32
	if (log_show_error_message) {
		MessageBoxA(nullptr, std::string{str.begin(), str.end()}.c_str(), "Error", MB_OK | MB_ICONERROR);
	}
#endif

	const auto message = fmt::format("[{} ERROR]: {}\n", util::getLocalTimeStr("%Y-%m-%dT%H:%M:%S"), str);
	Logger::global().output(message);

#ifndef NDEBUG
	if (log_debug_break_on_error) {
		assert(false && "An error message was logged.");
	}
#endif
}

auto logFatalError(std::string_view str) -> void {
#ifdef _WIN32
	if (log_show_error_message) {
		MessageBoxA(nullptr, std::string{str.begin(), str.end()}.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
	}
#endif

	const auto message = fmt::format("[{} FATAL]: {}\n", util::getLocalTimeStr("%F %T"), str);
	Logger::global().output(message);

#ifndef NDEBUG
	if (log_debug_break_on_error) {
		assert(false && "A fatal error message was logged.");
	}
#endif
}

} // namespace logger
