#include "suggestions.hpp"

#include "../utilities/algorithm.hpp" // util::collect, util::transform, util::filter, util::view
#include "commands/file_commands.hpp" // data_dir, data_subdir_cfg, data_subdir_maps, data_subdir_sounds, data_subdir_shaders
#include "con_command.hpp"            // ConCommand
#include "convar.hpp"                 // ConVar...

#include <filesystem> // std::filesystem::...
#include <fmt/core.h> // fmt::format

auto Suggestions::getFiles(std::string_view directory, std::string_view relativeTo) -> Suggestions {
	static constexpr auto isRegularFile = [](const auto& entry) {
		return entry.is_regular_file();
	};

	const auto getRelativePathString = [&](const auto& entry) {
		return std::filesystem::relative(entry.path(), relativeTo).generic_string();
	};

	try {
		const auto files = util::view(std::filesystem::recursive_directory_iterator{directory}, std::filesystem::recursive_directory_iterator{});
		return files | util::filter(isRegularFile) | util::transform(getRelativePathString) | util::collect<Suggestions>();
	} catch (const std::filesystem::filesystem_error&) {
	}
	return Suggestions{};
}

auto Suggestions::getDataFiles() -> Suggestions {
	return Suggestions::getFiles(data_dir, data_dir);
}

auto Suggestions::getScriptFilenames() -> Suggestions {
	const auto cfgDir = fmt::format("{}/{}", data_dir, data_subdir_cfg);
	const auto cfgDirInDownloads = fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, data_subdir_cfg);

	auto result = Suggestions::getFiles(cfgDir, cfgDir);
	util::append(result, Suggestions::getFiles(cfgDirInDownloads, cfgDirInDownloads));
	return result;
}

auto Suggestions::getMapFilenames() -> Suggestions {
	const auto mapDir = fmt::format("{}/{}", data_dir, data_subdir_maps);
	const auto mapDirInDownloads = fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, data_subdir_maps);

	auto result = Suggestions::getFiles(mapDir, mapDir);
	util::append(result, Suggestions::getFiles(mapDirInDownloads, mapDirInDownloads));
	return result;
}

auto Suggestions::getSoundFilenames() -> Suggestions {
	const auto soundDir = fmt::format("{}/{}", data_dir, data_subdir_sounds);
	return Suggestions::getFiles(soundDir, soundDir);
}

auto Suggestions::getShaderFilenames() -> Suggestions {
	const auto shaderDir = fmt::format("{}/{}", data_dir, data_subdir_shaders);
	return Suggestions::getFiles(shaderDir, shaderDir);
}

auto Suggestions::getCommandNames() -> Suggestions {
	static constexpr auto getCommandName = [](const auto& kv) {
		return kv.first;
	};
	return ConCommand::all() | util::transform(getCommandName) | util::collect<Suggestions>();
}

auto Suggestions::getCvarNames() -> Suggestions {
	static constexpr auto getCvarName = [](const auto& kv) {
		return kv.first;
	};
	return ConVar::all() | util::transform(getCvarName) | util::collect<Suggestions>();
}
