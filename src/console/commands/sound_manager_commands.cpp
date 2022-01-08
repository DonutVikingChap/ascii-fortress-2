#include "sound_manager_commands.hpp"

#include "../../game/client/sound_manager.hpp" // SoundManager
#include "../../game/game.hpp"                 // Game
#include "../command.hpp"                      // cmd::...
#include "../command_utilities.hpp"            // cmd::...
#include "../suggestions.hpp"                  // Suggestions
#include "file_commands.hpp"                   // data_dir, data_subdir_...

#include <fmt/core.h> // fmt::format

namespace {

CONVAR_CALLBACK(updateGlobalVolume) {
	game.updateGlobalVolume();
	return cmd::done();
}

CONVAR_CALLBACK(updateRolloffFactor) {
	game.updateRolloffFactor();
	return cmd::done();
}

CONVAR_CALLBACK(updateMaxSimultaneous) {
	game.updateMaxSimultaneouslyPlayingSounds();
	return cmd::done();
}

} // namespace

// clang-format off
ConVarFloatMinMax	snd_attenuation{		"snd_attenuation",				0.1f,	ConVar::CLIENT_SETTING,	"Coefficient for the positional audio sound stage size.", 0.0f, -1.0f};
ConVarFloatMinMax	snd_rolloff{			"snd_rolloff",					1.0f,	ConVar::CLIENT_SETTING,	"Sound rolloff factor.", 0.001f, -1.0f, updateRolloffFactor};
ConVarFloat			snd_distance{			"snd_distance",					2.0f,	ConVar::CLIENT_SETTING,	"How far away on the Z axis sounds should play in the 2D world."};
ConVarIntMinMax		snd_max_simultaneous{	"snd_max_simultaneous",			32,		ConVar::CLIENT_SETTING,	"Maximum number of sounds that can be playing simultaneously.", 1, 1024, updateMaxSimultaneous};
ConVarFloatMinMax	volume{					"volume",						25.0f,	ConVar::CLIENT_SETTING,	"Sound volume in percent.", 0.0f, 100.0f, updateGlobalVolume};
// clang-format on

CON_COMMAND(play_music, "<filename> [volume]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Play a music file once.", {},
            Suggestions::suggestSoundFile<1>) {
	if (argv.size() < 2 || argv.size() > 3) {
		return cmd::error(self.getUsage());
	}

	auto volume = 100.0f;
	if (argv.size() >= 3) {
		auto parseError = cmd::ParseError{};

		volume = cmd::parseNumber<float>(parseError, argv[2], "volume");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}
	}

	if (game.soundManager()) {
		const auto filepath = fmt::format("{}/{}/{}", data_dir, data_subdir_sounds, argv[1]);
		if (!game.soundManager()->playMusic(filepath.c_str(), volume * 0.01f, false)) {
			return cmd::error("{}: Failed to open \"{}\"!", self.getName(), filepath);
		}
	}
	return cmd::done();
}

CON_COMMAND(loop_music, "<filename> [volume]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Play a music file in a loop.", {},
            Suggestions::suggestSoundFile<1>) {
	if (argv.size() < 2 || argv.size() > 3) {
		return cmd::error(self.getUsage());
	}

	auto volume = 100.0f;
	if (argv.size() >= 3) {
		auto parseError = cmd::ParseError{};

		volume = cmd::parseNumber<float>(parseError, argv[2], "volume");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}
	}

	if (game.soundManager()) {
		const auto filepath = fmt::format("{}/{}/{}", data_dir, data_subdir_sounds, argv[1]);
		if (!game.soundManager()->playMusic(filepath.c_str(), volume * 0.01f, true)) {
			return cmd::error("{}: Failed to open \"{}\"!", self.getName(), filepath);
		}
	}
	return cmd::done();
}

CON_COMMAND(music_playing, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Check if there is any music currently playing.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	if (game.soundManager()) {
		return cmd::done(game.soundManager()->isMusicPlaying());
	}
	return cmd::done(false);
}

CON_COMMAND(stop_music, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Stop any currently playing music.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	if (game.soundManager()) {
		game.soundManager()->stopMusic();
	}
	return cmd::done();
}
