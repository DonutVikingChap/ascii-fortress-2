#include "file_commands.hpp"

#include "../../utilities/file.hpp" // util::readFile, util::dumpFile
#include "../command.hpp"           // cmd::...
#include "../suggestions.hpp"       // Suggestions

#include <filesystem>   // std::filesystem::exists, std::filesystem::create_directories
#include <fmt/core.h>   // fmt::format
#include <ios>          // std::ios
#include <system_error> // std::error_code

// clang-format off
ConVarString data_dir{					"data_dir",					"af2",			ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Main data file directory."};
ConVarString data_subdir_cfg{			"data_subdir_cfg",			"cfg",			ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Config file subdirectory."};
ConVarString data_subdir_logs{			"data_subdir_logs",			"logs",			ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Log file subdirectory."};
ConVarString data_subdir_maps{			"data_subdir_maps",			"maps",			ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Map file subdirectory."};
ConVarString data_subdir_fonts{			"data_subdir_fonts",		"fonts",		ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Font file subdirectory."};
ConVarString data_subdir_images{		"data_subdir_images",		"images",		ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Image file subdirectory."};
ConVarString data_subdir_sounds{		"data_subdir_sounds",		"sounds",		ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Sound file subdirectory."};
ConVarString data_subdir_shaders{		"data_subdir_shaders",		"shaders",		ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Shader file subdirectory."};
ConVarString data_subdir_screens{		"data_subdir_screens",		"screens",		ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Screen file subdirectory."};
ConVarString data_subdir_screenshots{	"data_subdir_screenshots",	"screenshots",	ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Screenshot file subdirectory."};
ConVarString data_subdir_downloads{		"data_subdir_downloads",	"downloads",	ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,	"Downloaded data subdirectory."};
// clang-format on

CON_COMMAND(file_read, "<filepath>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get the entire contents of a file.", {},
			Suggestions::suggestFile<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	const auto filepath = fmt::format("{}/{}", data_dir, argv[1]);
	if (auto buf = util::readFile(filepath)) {
		return cmd::done(std::move(*buf));
	}
	return cmd::error("{}: Couldn't open \"{}\" for writing.", self.getName(), filepath);
}

CON_COMMAND(file_append, "<filepath> <text>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Write text to the end of a file.", {},
			Suggestions::suggestFile<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	const auto filepath = fmt::format("{}/{}", data_dir, argv[1]);
	if (!util::dumpFile(filepath, argv[2], std::ios::app)) {
		return cmd::error("{}: Couldn't open \"{}\" for writing.", self.getName(), filepath);
	}

	return cmd::done();
}

CON_COMMAND(file_dump, "<filepath> <text>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Write text to a file. Replaces the entie file contents or creates it if it didn't already exist.", {}, Suggestions::suggestFile<1>) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	const auto filepath = fmt::format("{}/{}", data_dir, argv[1]);
	if (!util::dumpFile(filepath, argv[2])) {
		return cmd::error("{}: Couldn't open \"{}\" for writing.", self.getName(), filepath);
	}

	return cmd::done();
}

CON_COMMAND(file_exists, "<filepath>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Check if a file exists.", {}, Suggestions::suggestFile<1>) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};

	const auto result = std::filesystem::exists(fmt::format("{}/{}", data_dir, argv[1]), ec);
	if (ec) {
		return cmd::error("{}: Couldn't check if file exists: {}", self.getName(), ec.message());
	}

	return cmd::done(result);
}

CON_COMMAND(file_create_path, "<filepath>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Create a path of directories if they don't already exist.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	const auto filepath = fmt::format("{}/{}", data_dir, argv[1]);

	auto ec = std::error_code{};

	std::filesystem::create_directories(filepath, ec);
	if (ec) {
		return cmd::error("{}: Couldn't create path \"{}\": {}", self.getName(), filepath, ec.message());
	}

	return cmd::done();
}
