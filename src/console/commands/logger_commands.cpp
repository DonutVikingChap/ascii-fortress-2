#include "logger_commands.hpp"

#include "../../logger.hpp"              // logger::...
#include "../../utilities/algorithm.hpp" // util::subview
#include "../../utilities/string.hpp"    // util::join
#include "../command.hpp"                // cmd::...

// clang-format off
ConVarBool		log_flush{					"log_flush",				true,		ConVar::CLIENT_SETTING,		"Whether or not to flush the log file every time a new message is logged."};
ConVarIntMinMax	log_file_limit{				"log_file_limit",			50,			ConVar::CLIENT_SETTING,		"How many log files to keep before deleting the oldest ones. Set to 0 to never delete log files.", 0, -1};
ConVarIntMinMax	log_max_size{				"log_max_size",				1000000,	ConVar::CLIENT_SETTING,		"Maximum number of bytes to write before opening a new log file. Set to 0 for unlimited size.", 0, -1};
ConVarBool		log_show_error_message{		"log_show_error_message",	true,		ConVar::CLIENT_SETTING,		"Whether or not to show a message box when errors are logged."};
ConVarBool		log_debug_output{			"log_debug_output",			true,		ConVar::CLIENT_VARIABLE,	"Whether or not to output log messages to stderr."};
ConVarBool		log_debug_break_on_error{	"log_debug_break_on_error",	true,		ConVar::CLIENT_VARIABLE,	"Whether or not to break the debugger when errors are logged."};
// clang-format on

CON_COMMAND(log_open, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Start a new log file.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	if (!logger::open()) {
		return cmd::error("Failed to open log file!");
	}
	return cmd::done();
}

CON_COMMAND(log_close, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Close the current log file.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	logger::close();
	return cmd::done();
}

CON_COMMAND(log_info, "<message...>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Log an info message.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}
	logger::logInfo(util::subview(argv, 1) | util::join(' '));
	return cmd::done();
}

CON_COMMAND(log_warning, "<message...>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Log a warning message.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}
	logger::logWarning(util::subview(argv, 1) | util::join(' '));
	return cmd::done();
}

CON_COMMAND(log_error, "<message...>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Log an error message.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}
	logger::logError(util::subview(argv, 1) | util::join(' '));
	return cmd::done();
}

CON_COMMAND(log_fatal, "<message...>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Log a fatal error message.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}
	logger::logFatalError(util::subview(argv, 1) | util::join(' '));
	return cmd::done();
}
