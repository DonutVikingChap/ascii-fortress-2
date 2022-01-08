#include "game_commands.hpp"

#include "../../game/client/game_client.hpp" // GameClient
#include "../../game/client/renderer.hpp"    // Renderer
#include "../../game/game.hpp"               // Game
#include "../../game/meta/meta_client.hpp"   // MetaClient
#include "../../game/meta/meta_server.hpp"   // MetaServer
#include "../../game/server/game_server.hpp" // GameServer
#include "../../graphics/error.hpp"          // gfx::Error
#include "../../graphics/image.hpp"          // gfx::ImageView, gfx::ImageOptions..., gfx::save...
#include "../../utilities/algorithm.hpp"     // util::filter, util::collect, util::transform, util::enumerate, util::sort, util::contains
#include "../../utilities/file.hpp"          // util::uniqueFilePath, util::readFile, util::dumpFile
#include "../../utilities/string.hpp"        // util::toUpper, util::join
#include "../../utilities/time.hpp"          // util::getLocalTimeStr
#include "../command.hpp"                    // cmd::...
#include "../command_options.hpp"            // cmd::...
#include "../command_utilities.hpp"          // cmd::...
#include "../process.hpp"                    // Process
#include "../script.hpp"                     // Script::escapedString
#include "../suggestions.hpp"                // Suggestions
#include "environment_commands.hpp"          // cmd_...
#include "file_commands.hpp"                 // data_dir, data_subdir_...
#include "game_client_commands.hpp"          // cl_autoexec_file, cl_config_file, cmd_fwd
#include "game_server_commands.hpp"          // sv_autoexec_file, sv_config_file
#include "input_manager_commands.hpp"        // bind
#include "logic_commands.hpp"                // cmd_..., cvar_true, cvar_false
#include "math_commands.hpp"                 // cmd_..., cvar_e, cvar_pi
#include "meta_server_commands.hpp"          // meta_sv_autoexec_file, meta_sv_config_file
#include "process_commands.hpp"              // cmd_...
#include "stream_commands.hpp"               // cmd_...
#include "virtual_machine_commands.hpp"      // cmd_...

#include <array>        // std::array
#include <filesystem>   // std::filesystem::...
#include <fmt/core.h>   // fmt::format
#include <optional>     // std::optional
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <system_error> // std::error_code
#include <vector>       // std::vector

namespace {

CONVAR_CALLBACK(updateVertexShaderFilepath) {
	game.updateVertexShaderFilepath();
	return cmd::done();
}

CONVAR_CALLBACK(updateFragmentShaderFilepath) {
	game.updateFragmentShaderFilepath();
	return cmd::done();
}

CONVAR_CALLBACK(updateFontFilepath) {
	game.updateFontFilepath();
	return cmd::done();
}

CONVAR_CALLBACK(updateFontStaticSize) {
	game.updateFontStaticSize();
	return cmd::done();
}

CONVAR_CALLBACK(updateFontMatchSize) {
	game.updateFontMatchSize();
	return cmd::done();
}

CONVAR_CALLBACK(updateFontMatchSizeCoefficient) {
	game.updateFontMatchSizeCoefficient();
	return cmd::done();
}

CONVAR_CALLBACK(updateGlyphOffset) {
	game.updateGlyphOffset();
	return cmd::done();
}

CONVAR_CALLBACK(updateGridRatio) {
	game.updateGridRatio();
	return cmd::done();
}

CONVAR_CALLBACK(updateWindowIcon) {
	game.updateWindowIcon();
	return cmd::done();
}

CONVAR_CALLBACK(updateWindowMode) {
	game.updateWindowMode();
	return cmd::done();
}

CONVAR_CALLBACK(updateWindowTitle) {
	game.updateWindowTitle();
	return cmd::done();
}

CONVAR_CALLBACK(updateWindowVSync) {
	game.updateWindowVSync();
	return cmd::done();
}

CONVAR_CALLBACK(updateBackgroundColor) {
	game.updateBackgroundColor();
	return cmd::done();
}

CONVAR_CALLBACK(updateFrameInterval) {
	game.updateFrameInterval();
	return cmd::done();
}

CONVAR_CALLBACK(updateConsoleRows) {
	game.updateConsoleRows();
	return cmd::done();
}

} // namespace

// clang-format off
ConVarString		cvar_main{						"main",								"",														ConVar::INIT | ConVar::ADMIN_ONLY | ConVar::NO_RCON,		"Main game command."};
ConVarString		cvar_game{						"game",								"",														ConVar::INIT | ConVar::WRITE_ADMIN_ONLY,					"Short game name."};
ConVarString		game_version{					"game_version",						"",														ConVar::INIT | ConVar::WRITE_ADMIN_ONLY,					"Game version."};
ConVarString		game_name{						"game_name",						"",														ConVar::INIT | ConVar::WRITE_ADMIN_ONLY,					"Game name."};
ConVarString		game_author{					"game_author",						"",														ConVar::INIT | ConVar::WRITE_ADMIN_ONLY,					"Game author."};
ConVarString		game_year{						"game_year",						"",														ConVar::INIT | ConVar::WRITE_ADMIN_ONLY,					"Game year."};
ConVarString		game_url{						"game_url",							"",														ConVar::HOST_VARIABLE,										"Game URL."};
ConVarBool			headless{						"headless",							false,													ConVar::INIT | ConVar::WRITE_ADMIN_ONLY | ConVar::NO_RCON,	"Whether or not to run in headless mode (no window/graphics)."};
ConVarString		r_shader_vert{					"r_shader_vert",					"default.vert",											ConVar::CLIENT_SETTING,										"What vertex shader to use.", updateVertexShaderFilepath};
ConVarString		r_shader_frag{					"r_shader_frag",					"default.frag",											ConVar::CLIENT_SETTING,										"What fragment shader to use.", updateFragmentShaderFilepath};
ConVarString		r_font{							"r_font",							"liberation/LiberationMono-Regular.ttf",				ConVar::CLIENT_SETTING,										"What main font to use.", updateFontFilepath};
ConVarString		r_icon{							"r_icon",							"",														ConVar::CLIENT_SETTING,										"What icon to use for the main window.", updateWindowIcon};
ConVarIntMinMax		r_fullscreen_mode{				"r_fullscreen_mode",				0,														ConVar::CLIENT_SETTING,										"What fullscreen mode to use. 0 = windowed.", 0, -1, updateWindowMode};
ConVarIntMinMax		r_width_windowed{				"r_width_windowed",					static_cast<int>(Renderer::DEFAULT_WINDOW_WIDTH),		ConVar::CLIENT_SETTING,										"What window width to use when switching to windowed mode.", 0, -1};
ConVarIntMinMax		r_height_windowed{				"r_height_windowed",				static_cast<int>(Renderer::DEFAULT_WINDOW_HEIGHT),		ConVar::CLIENT_SETTING,										"What window height to use when switching to windowed mode.", 0, -1};
ConVarIntMinMax		r_width{						"r_width",							static_cast<int>(Renderer::DEFAULT_WINDOW_WIDTH),		ConVar::CLIENT_SETTING,										"Width of the main window.", 0, -1, updateWindowMode};
ConVarIntMinMax		r_height{						"r_height",							static_cast<int>(Renderer::DEFAULT_WINDOW_HEIGHT),		ConVar::CLIENT_SETTING,										"Height of the main window.", 0, -1, updateWindowMode};
ConVarString		r_window_title{					"r_window_title",					"",														ConVar::CLIENT_VARIABLE,									"Title of the main window.", updateWindowTitle};
ConVarBool			r_vsync{						"r_vsync",							false,													ConVar::CLIENT_SETTING,										"Whether or not to use vertical sync.", updateWindowVSync};
ConVarColor			r_background_color{				"r_background_color",				Color::black(),											ConVar::CLIENT_SETTING,										"Main background color.", updateBackgroundColor};
ConVarFloatMinMax	r_ratio{						"r_ratio",							Renderer::DEFAULT_GRID_RATIO,							ConVar::CLIENT_SETTING,										"The y/x ratio used for spacing characters. A value of 1 is equal spacing, <1 is more stretched horizontally, >1 is more stretched vertically.", 0.1f, 10.0f, updateGridRatio};
ConVarBool			r_font_match_size{				"r_font_match_size",				Renderer::DEFAULT_FONT_MATCH_SIZE,						ConVar::CLIENT_SETTING,										"Automatically determine font size based on window resolution.", updateFontMatchSize};
ConVarFloatMinMax	r_font_match_size_coefficient{	"r_font_match_size_coefficient",	Renderer::DEFAULT_FONT_MATCH_SIZE_COEFFICIENT,			ConVar::CLIENT_SETTING,										"Coefficient to use when automatically determining font size based on window resolution.", 0.001f, -1.0f, updateFontMatchSizeCoefficient};
ConVarIntMinMax		r_font_size{					"r_font_size",						static_cast<int>(Renderer::DEFAULT_FONT_STATIC_SIZE),	ConVar::CLIENT_SETTING,										fmt::format("Main font size. Ignored if {} is enabled.", r_font_match_size.cvar().getName()), 1, 100, updateFontStaticSize};
ConVarInt			r_glyph_offset_x{				"r_glyph_offset_x",					0,														ConVar::CLIENT_SETTING,										"Offset of glyphs on the X axis.", updateGlyphOffset};
ConVarInt			r_glyph_offset_y{				"r_glyph_offset_y",					0,														ConVar::CLIENT_SETTING,										"Offset of glyphs on the Y axis.", updateGlyphOffset};
ConVarInt			r_debug_text_offset_x{			"r_debug_text_offset_x",			14,														ConVar::CLIENT_SETTING,										"Position of debug text on the X axis."};
ConVarInt			r_debug_text_offset_y{			"r_debug_text_offset_y",			28,														ConVar::CLIENT_SETTING,										"Position of debug text on the Y axis."};
ConVarFloatMinMax	r_debug_text_scale_x{			"r_debug_text_scale_x",				0.8f,													ConVar::CLIENT_SETTING,										"Scale of debug text on the X axis.", 0.001f, 1000.0f};
ConVarFloatMinMax	r_debug_text_scale_y{			"r_debug_text_scale_y",				0.8f,													ConVar::CLIENT_SETTING,										"Scale of debug text on the Y axis.", 0.001f, 1000.0f};
ConVarColor			r_debug_text_color{				"r_debug_text_color",				Color::orange(),										ConVar::CLIENT_SETTING,										"Color of debug text."};
ConVarBool			r_showfps{						"r_showfps",						false,													ConVar::CLIENT_SETTING,										"Whether or not to draw the FPS counter."};
ConVarFloatMinMax	fps_max{						"fps_max",							60.0f,													ConVar::CLIENT_SETTING,										"Maximum FPS limit. 0 = unlimited.", 0.0f, -1.0f, updateFrameInterval};
ConVarIntMinMax		console_max_rows{				"console_max_rows",					1000,													ConVar::CLIENT_SETTING,										"Number of rows in the console buffer.", 0, -1, updateConsoleRows};
ConVarFloatMinMax	host_timescale{					"host_timescale",					1.0f,													ConVar::HOST_VARIABLE,										"Time scale factor.", 0.001f, 1000.0f};
ConVarString		host_config_file{				"host_config_file",					"config.cfg",											ConVar::HOST_VARIABLE,										"Main config file to read at startup and save to at shutdown."};
ConVarString		host_autoexec_file{				"host_autoexec_file",				"autoexec.cfg",											ConVar::HOST_VARIABLE,										"Autoexec file to read at startup."};
ConVarBool			host_server_admin{				"host_server_admin",				false,													ConVar::HOST_SETTING | ConVar::NOT_RUNNING_GAME_SERVER,		"Give admin privileges to the server process when launched. Handle with care."};
// clang-format on

CON_COMMAND(r_size, "[width] [height]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Set or query the main window width and height simultaneously.", {}, nullptr) {
	if (argv.size() == 1) {
		return cmd::done("{}x{}", r_width, r_height);
	}
	if (argv.size() == 3) {
		r_width.setSilent(argv[1]);
		r_height.setSilent(argv[2]);
		game.updateWindowMode();
		return cmd::done();
	}
	if (argv.size() == 2) {
		if (const auto xPos = argv[1].find('x'); xPos != std::string::npos) {
			r_width.setSilent(argv[1].substr(0, xPos));
			r_height.setSilent(argv[1].substr(xPos + 1));
			game.updateWindowMode();
			return cmd::done();
		}
	}
	return cmd::error(
		"Usage:\n"
		"  {0}: Query width/height.\n"
		"  {0} <w>x<h>: Set width/height to w/h.\n"
		"  {0} <w> <h>: Set width/height to w/h.",
		self.getName());
}

CON_COMMAND(r_desktop_width, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get the width (in pixels) of the host's desktop resolution.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(game.getDesktopMode().w);
}

CON_COMMAND(r_desktop_height, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Get the height (in pixels) of the host's desktop resolution.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(game.getDesktopMode().h);
}

CON_COMMAND(r_desktop_size, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Get the size <width>x<height> (in pixels) of the host's desktop resolution.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	const auto& mode = game.getDesktopMode();
	return cmd::done("{}x{}", mode.w, mode.h);
}

CON_COMMAND(r_fullscreen_list, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "List available fullscreen modes.", {}, nullptr) {
	static constexpr auto formatFullscreenMode = [](const auto& iv) {
		const auto& [i, mode] = iv;
		return fmt::format("{}. {}x{}px {}bpp {}Hz", i + 1, mode.w, mode.h, SDL_BITSPERPIXEL(mode.format), mode.refresh_rate);
	};
	return cmd::done(game.getFullscreenModes() | util::enumerate() | util::transform(formatFullscreenMode) | util::join('\n'));
}

CON_COMMAND(say, "<text...>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Send a chat message.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	const auto text = util::subview(argv, 1) | util::join(' ');
	if (client) {
		if (!client->writeChatMessage(text)) {
			return cmd::error("{}: Failed to write chat message.", self.getName());
		}
	} else if (server) {
		server->writeServerChatMessage(text);
	} else {
		return cmd::error("{}: Not connected.", self.getName());
	}

	if (!text.empty() && (text.front() == '!' || text.front() == '/')) {
		const auto forwardCommand = fmt::format("{} {}", GET_COMMAND(fwd).getName(), std::string_view{text}.substr(1));
		if (!frame.tailCall(frame.env(), forwardCommand)) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
	}
	return cmd::done();
}

CON_COMMAND(say_team, "<text...>", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Send a chat message to your team.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	const auto text = util::subview(argv, 1) | util::join(' ');

	assert(client);
	if (!client->writeTeamChatMessage(text)) {
		return cmd::error("{}: Failed to write chat message.", self.getName());
	}
	if (!text.empty() && (text.front() == '!' || text.front() == '/')) {
		const auto forwardCommand = fmt::format("{} {}", GET_COMMAND(fwd).getName(), std::string_view{text}.substr(1));
		if (!frame.tailCall(frame.env(), forwardCommand)) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
	}
	return cmd::done();
}

CON_COMMAND(say_server, "<text...>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Send a chat message as the server.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	server->writeServerChatMessage(util::subview(argv, 1) | util::join(' '));
	return cmd::done();
}

CON_COMMAND(clear_console, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Clear the console.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	game.clearConsole();
	return cmd::done();
}

CON_COMMAND(open_console, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Make console text input active.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}
	game.setConsoleModeConsole();
	game.activateConsole();
	return cmd::done();
}

CON_COMMAND(open_chat, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Make chat text input active.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}
	game.setConsoleModeChat();
	game.activateConsole();
	return cmd::done();
}

CON_COMMAND(open_teamchat, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Make team chat text input active.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}
	game.setConsoleModeTeamChat();
	game.activateConsole();
	return cmd::done();
}

CON_COMMAND(open_textinput, "<script>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Make text input active and execute a script when the text is submitted. The script receives a parameter named text.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	game.setConsoleModeTextInput([&game, script = Script::parse(argv[1])](std::string_view text) {
		auto func = Environment::Function{};
		func.body = script;
		func.parameters.emplace_back("text");
		game.consoleCommand(func, std::array{cmd::Value{text}});
	});
	game.activateConsole();
	return cmd::done();
}

CON_COMMAND(open_password, "<script>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Make password text input active and execute a script when the password is submitted. The script receives a parameter named "
            "password.",
            {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}
	game.setConsoleModePassword([&game, script = Script::parse(argv[1])](std::string_view password) {
		auto func = Environment::Function{};
		func.body = script;
		func.parameters.emplace_back("password");
		game.consoleCommand(func, std::array{cmd::Value{password}});
	});
	game.activateConsole();
	return cmd::done();
}

CON_COMMAND(maplist, "", ConCommand::ADMIN_ONLY, "List all available maps.", {}, nullptr) {
	return cmd::done(Suggestions::getMapFilenames() | util::join('\n'));
}

CON_COMMAND(status, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Get connection status.", {}, nullptr) {
	if (server) {
		if (client) {
			return cmd::done(
				"{}\n"
				"{}",
				server->getStatusString(),
				client->getStatusString());
		}
		return cmd::done(server->getStatusString());
	}
	if (client) {
		return cmd::done(client->getStatusString());
	}
	if (metaServer) {
		return cmd::done(metaServer->getStatusString());
	}
	if (metaClient) {
		return cmd::done(metaClient->getStatusString());
	}
	return cmd::error("{}: Not connected.", self.getName());
}

CON_COMMAND(print, "<text...>", ConCommand::NO_FLAGS, "Print text to the console.", {}, nullptr) {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}
	game.print(util::subview(argv, 1) | util::join(' '));
	return cmd::done();
}

CON_COMMAND(print_colored, "<color> <text...>", ConCommand::NO_FLAGS, "Print colored text to the console.", {}, cmd::suggestColor<1>) {
	if (argv.size() < 3) {
		return cmd::error(self.getUsage());
	}
	if (const auto color = Color::parse(argv[1])) {
		game.print(util::subview(argv, 2) | util::join(' '), *color);
		return cmd::done();
	}
	return cmd::error("{}: Invalid color \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(println, "[text...]", ConCommand::NO_FLAGS, "Print a line of text to the console.", {}, nullptr) {
	if (argv.empty()) {
		return cmd::error(self.getUsage());
	}
	if (argv.size() == 1) {
		game.println();
	} else {
		game.println(util::subview(argv, 1) | util::join(' '));
	}
	return cmd::done();
}

CON_COMMAND(println_colored, "<color> <text...>", ConCommand::NO_FLAGS, "Print a line of colored text to the console.", {}, cmd::suggestColor<1>) {
	if (argv.size() < 3) {
		return cmd::error(self.getUsage());
	}
	if (const auto color = Color::parse(argv[1])) {
		game.println(util::subview(argv, 2) | util::join(' '), *color);
		return cmd::done();
	}
	return cmd::error("{}: Invalid color \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(is_running_client, "", ConCommand::NO_FLAGS, "Check if the game is currently running a game client.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(static_cast<bool>(game.gameClient()));
}

CON_COMMAND(is_running_server, "", ConCommand::NO_FLAGS, "Check if the game is currently running a game server.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(static_cast<bool>(game.gameServer()));
}

CON_COMMAND(is_running_meta_client, "", ConCommand::NO_FLAGS, "Check if the game is currently running a meta client.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(static_cast<bool>(game.metaClient()));
}

CON_COMMAND(is_running_meta_server, "", ConCommand::NO_FLAGS, "Check if the game is currently running a meta server.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(static_cast<bool>(game.metaServer()));
}

CON_COMMAND(is_running, "", ConCommand::NO_FLAGS, "Check if the game is currently running a game/meta server/client.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(game.gameServer() || game.gameClient() || game.metaServer() || game.metaClient());
}

CON_COMMAND(disconnect, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Disconnect from the current game.", {}, nullptr) {
	if (!server && !client && !metaServer && !metaClient) {
		return cmd::error("{}: Not connected.", self.getName());
	}
	if (metaClient) {
		metaClient->stop();
	}
	if (metaServer) {
		metaServer->stop();
	}
	if (client) {
		client->disconnect();
	}
	if (server) {
		server->stop();
	}
	return cmd::done();
}

CON_COMMAND(quit, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Quit the game.", {}, nullptr) {
	if (frame.progress() == 0) {
		if (argv.size() != 1) {
			return cmd::error(self.getUsage());
		}

		if (!server && !client) {
			game.quit();
			return cmd::done();
		}
		if (client) {
			client->disconnect();
		}
		if (server) {
			server->stop();
		}
		return cmd::notDone(1);
	}

	if (game.gameServer() || game.gameClient() || game.metaServer() || game.metaClient()) {
		return cmd::notDone(1);
	}

	game.quit();
	return cmd::done();
}

CON_COMMAND(host_publish_game, "<outdir>", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Create a copy of the game folder containing all necessary game files with the default configs.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	const auto dataPath = std::filesystem::path{std::string_view{data_dir}};
	const auto dataFilename = dataPath.filename();
	const auto outPath = dataPath / std::string_view{argv[1]};
	const auto outDataPath = outPath / dataFilename;

	const auto cfgSubpath = std::filesystem::path{std::string_view{data_subdir_cfg}};
	const auto logsSubpath = std::filesystem::path{std::string_view{data_subdir_logs}};
	const auto mapsSubpath = std::filesystem::path{std::string_view{data_subdir_maps}};
	const auto fontsSubpath = std::filesystem::path{std::string_view{data_subdir_fonts}};
	const auto imagesSubpath = std::filesystem::path{std::string_view{data_subdir_images}};
	const auto soundsSubpath = std::filesystem::path{std::string_view{data_subdir_sounds}};
	const auto shadersSubpath = std::filesystem::path{std::string_view{data_subdir_shaders}};
	const auto screensSubpath = std::filesystem::path{std::string_view{data_subdir_screens}};
	const auto screenshotsSubpath = std::filesystem::path{std::string_view{data_subdir_screenshots}};
	const auto downloadsSubpath = std::filesystem::path{std::string_view{data_subdir_downloads}};

	const auto exePath = std::filesystem::path{game.getFilename()};
	const auto binPath = exePath.parent_path();
	const auto exeFilename = exePath.filename();

	auto ec = std::error_code{};

	std::filesystem::create_directory(outPath, ec);
	if (ec) {
		return cmd::error("{}: Failed to create directory \"{}\": {}", self.getName(), outPath.string(), ec.message());
	}
	std::filesystem::create_directory(outDataPath, ec);
	if (ec) {
		return cmd::error("{}: Failed to create data directory \"{}\": {}", self.getName(), outDataPath.string(), ec.message());
	}
	std::filesystem::create_directory(outDataPath / cfgSubpath, ec);
	if (ec) {
		return cmd::error("{}: Failed to create cfg directory: {}", self.getName(), ec.message());
	}
	std::filesystem::create_directory(outDataPath / logsSubpath, ec);
	if (ec) {
		return cmd::error("{}: Failed to create logs directory: {}", self.getName(), ec.message());
	}
	std::filesystem::create_directory(outDataPath / screenshotsSubpath, ec);
	if (ec) {
		return cmd::error("{}: Failed to create screenshots directory: {}", self.getName(), ec.message());
	}
	std::filesystem::create_directory(outDataPath / downloadsSubpath, ec);
	if (ec) {
		return cmd::error("{}: Failed to create downloads directory: {}", self.getName(), ec.message());
	}

	static constexpr auto readmeFilename = "readme.txt";
	if (auto buf = util::readFile((dataPath / ".." / readmeFilename).string())) {
		if (!util::dumpFile((outPath / readmeFilename).string(), *buf)) {
			return cmd::error("{}: Failed to write readme file.", self.getName());
		}
	} else {
		return cmd::error("{}: Failed to read readme file.", self.getName());
	}

	static constexpr auto getElementPtr = [](const auto& kv) {
		return &kv.second;
	};
	static constexpr auto compareName = [](const auto* lhs, const auto* rhs) {
		return lhs->getName() < rhs->getName();
	};
	static constexpr auto formatCommand = [](const auto* pCmd) {
		const auto& cmd = *pCmd;
		auto str = std::string{cmd.getName()};
		if (const auto& parameters = cmd.getParameters(); !parameters.empty()) {
			str.append(fmt::format(" {}", parameters));
		}
		str.append(fmt::format("\n  Description: {}\n", cmd.getDescription()));
		if (cmd.getFlags() != ConCommand::NO_FLAGS) {
			str.append(fmt::format("  Flags: {}\n", cmd.formatFlags()));
		}
		if (!cmd.getOptions().empty()) {
			str.append(fmt::format("  Options:\n{}\n", cmd.formatOptions()));
		}
		return str;
	};
	static constexpr auto formatCvar = [](const auto* pCvar) {
		const auto& cvar = *pCvar;
		auto str = std::string{cvar.getName()};
		if (const auto& defaultValue = cvar.getDefaultValue(); !defaultValue.empty()) {
			str.append(fmt::format(" (default: \"{}\")", defaultValue));
		}
		str.append(fmt::format("\n  Description: {}\n", cvar.getDescription()));
		const auto minValue = cvar.getMinValue();
		const auto maxValue = cvar.getMaxValue();
		if (minValue != maxValue) {
			str.append(fmt::format("  Minimum: {:g}\n", minValue));
			if (minValue < maxValue) {
				str.append(fmt::format("  Maximum: {:g}\n", maxValue));
			}
		}
		if (cvar.getFlags() != ConVar::NO_FLAGS) {
			str.append(fmt::format("  Flags: {}\n", cvar.formatFlags()));
		}
		return str;
	};
	const auto sortedCommands = ConCommand::all() | util::transform(getElementPtr) | util::collect<std::vector<ConCommand*>>() | util::sort(compareName);
	const auto sortedCvars = ConVar::all() | util::transform(getElementPtr) | util::collect<std::vector<ConVar*>>() | util::sort(compareName);
	static constexpr auto omakeFilename = "omake.txt";
	if (!util::dumpFile((outPath / omakeFilename).string(),
	                    fmt::format("-------------------------------------------------------------------\n"
	                                "* {} ~ {}.\n"
	                                "\n"
	                                "  Console Commands & Variables\n"
	                                "\n"
	                                "                               Generated for Version {} at\n"
	                                "                                                     {}\n"
	                                "-------------------------------------------------------------------\n"
	                                "\n"
	                                "====================================================================\n"
	                                "#1. Console Commands\n"
	                                "====================================================================\n"
	                                "\n"
	                                "{}\n"
	                                "\n"
	                                "====================================================================\n"
	                                "#2. Console Variables\n"
	                                "====================================================================\n"
	                                "\n"
	                                "{}\n"
	                                "\n",
	                                util::toUpper(cvar_game),
	                                game_name,
	                                game_version,
	                                util::getLocalTimeStr("%Y-%m-%d"),
	                                sortedCommands | util::transform(formatCommand) | util::join('\n'),
	                                sortedCvars | util::transform(formatCvar) | util::join('\n')))) {
		return cmd::error("{}: Failed to write omake file!", self.getName());
	}

	static constexpr auto getName = [](const auto* p) {
		return p->getName();
	};
	const auto controlKeywordCommands = std::array {
		&GET_COMMAND(scope),
		&GET_COMMAND(if),
		&GET_COMMAND(else),
		&GET_COMMAND(elif),
		&GET_COMMAND(while),
		&GET_COMMAND(for),
		&GET_COMMAND(foreach),
		&GET_COMMAND(throw),
		&GET_COMMAND(try),
		&GET_COMMAND(catch),
		&GET_COMMAND(return),
		&GET_COMMAND(break),
		&GET_COMMAND(continue),
		&GET_COMMAND(await),
		&GET_COMMAND(await_limited),
		&GET_COMMAND(await_unlimited),
		&GET_COMMAND(echo),
		&GET_COMMAND(wait),
		&GET_COMMAND(sleep),
		&GET_COMMAND(breakpoint),
	};
	const auto keywordCommands = std::array{
		&GET_COMMAND(void),   &GET_COMMAND(delete), &GET_COMMAND(alias), &GET_COMMAND(unalias),  &GET_COMMAND(global), &GET_COMMAND(inline),
		&GET_COMMAND(enum),   &GET_COMMAND(var),    &GET_COMMAND(const), &GET_COMMAND(function), &GET_COMMAND(array),  &GET_COMMAND(table),
		&GET_COMMAND(eq),     &GET_COMMAND(ne),     &GET_COMMAND(lt),    &GET_COMMAND(le),       &GET_COMMAND(gt),     &GET_COMMAND(ge),
		&GET_COMMAND(neg),    &GET_COMMAND(add),    &GET_COMMAND(sub),   &GET_COMMAND(mul),      &GET_COMMAND(div),    &GET_COMMAND(mod),
		&GET_COMMAND(pow),    &GET_COMMAND(not ),   &GET_COMMAND(and),   &GET_COMMAND(or),       &GET_COMMAND(xor),    &GET_COMMAND(export),
		&GET_COMMAND(import), &GET_COMMAND(script), &GET_COMMAND(file),
	};
	const auto keywordCvars = std::array{
		&cvar_true.cvar(),
		&cvar_false.cvar(),
		&cvar_e.cvar(),
		&cvar_pi.cvar(),
	};
	const auto isRegularCommand = [&](const auto* pCmd) {
		return !util::contains(controlKeywordCommands, pCmd) && !util::contains(keywordCommands, pCmd);
	};
	const auto isRegularCvar = [&](const auto* pCvar) {
		return !util::contains(keywordCvars, pCvar);
	};
	static constexpr auto tmLanguageFilename = "af2script.tmLanguage.json";
	if (!util::dumpFile((outDataPath / cfgSubpath / tmLanguageFilename).string(),
	                    fmt::format(R"JSON({{
	"$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json",
	"name": "AF2Script",
	"patterns": [{{"include": "#keywords"}}, {{"include": "#strings"}}],
	"repository": {{
		"keywords": {{
			"patterns": [
				{{"name": "keyword.control.af2script", "match": "\\b({})\\b"}},
				{{"name": "variable.af2script", "match": "\\$[a-z|A-Z|0-9|_|+|-|@|\\$]+"}},
				{{"name": "comment.line.double-slash.af2script", "match": "//.*"}},
				{{"name": "keyword.af2script", "match": "\\b({})\\b"}},
				{{"name": "entity.name.function.af2script", "match": "\\b({})\\b"}},
				{{"name": "entity.name.class.af2script", "match": "\\b({})\\b"}},
				{{"name": "constant.numeric.af2script", "match": "-?\\b([0-9][0-9|']*\\.?[0-9|']*)\\b"}}
			]
		}},
		"strings": {{
			"name": "string.quoted.double.af2script",
			"begin": "\"",
			"end": "\"",
			"patterns": [
				{{"name": "constant.character.escape.af2script", "match": "\\\\."}}
			]
		}}
	}},
	"scopeName": "source.af2"
}})JSON",
	                                controlKeywordCommands | util::transform(getName) | util::join('|'),
	                                (keywordCommands | util::transform(getName) | util::join('|')) + "|" +
	                                    (keywordCvars | util::transform(getName) | util::join('|')),
	                                sortedCommands | util::filter(isRegularCommand) | util::transform(getName) | util::join('|'),
	                                sortedCvars | util::filter(isRegularCvar) | util::transform(getName) | util::join('|')))) {
		return cmd::error("{}: Failed to write tmLanguage file!", self.getName());
	}

	std::filesystem::copy_file(exePath, outPath / exeFilename, std::filesystem::copy_options::update_existing, ec);
	if (ec) {
		return cmd::error("{}: Failed to copy game executable \"{}\" to \"{}\": {}",
		                  self.getName(),
		                  game.getFilename(),
		                  (outPath / exeFilename).string(),
		                  ec.message());
	}

#ifdef _WIN32
	static constexpr auto libraryExtension = ".dll";
#else
	static constexpr auto libraryExtension = ".so";
#endif
	for (const auto& file : std::filesystem::directory_iterator{binPath, ec}) {
		if (file.is_regular_file()) {
			if (const auto& path = file.path(); path.filename().string().find(libraryExtension) != std::string::npos) {
				std::filesystem::copy_file(path, outPath / path.filename(), std::filesystem::copy_options::update_existing, ec);
				if (ec) {
					return cmd::error("{}: Failed to copy library file \"{}\" to \"{}\": {}",
					                  self.getName(),
					                  path,
					                  (outPath / path.filename()).string(),
					                  ec.message());
				}
			}
		}
	}
	if (ec) {
		return cmd::error("{}: Failed to iterate bin directory \"{}\": {}", self.getName(), binPath.string(), ec.message());
	}

	if (auto buf = util::readFile((dataPath / "game.cfg").string())) {
		if (!util::dumpFile((outDataPath / "game.cfg").string(), *buf)) {
			return cmd::error("{}: Failed to write game script file.", self.getName());
		}
	} else {
		return cmd::error("{}: Failed to read game script file.", self.getName());
	}

	if (cvar_game != "af2") {
		static constexpr auto initFilename = "init.cfg";
		if (!util::dumpFile((outPath / initFilename).string(),
		                    fmt::format("// Startup script for \"{}\".\n"
		                                "// Do not modify this file as a user. Use the autoexec file instead.\n"
		                                "{} {}\n",
		                                game_name,
		                                data_dir.cvar().getName(),
		                                Script::escapedString(dataFilename.string())))) {
			return cmd::error("{}: Failed to write init file!", self.getName());
		}
	}

	const auto copyData = [&](const std::filesystem::path& relativePath) -> std::optional<cmd::Result> {
		const auto inputPath = dataPath / relativePath;
		for (const auto& file : std::filesystem::recursive_directory_iterator{inputPath, ec}) {
			if (file.is_regular_file()) {
				const auto& path = file.path();
				if (path.extension() == ".txt" || path.extension() == ".cfg" || path.extension() == ".vert" || path.extension() == ".frag") {
					if (auto buf = util::readFile(path.string())) {
						if (!util::dumpFile((outDataPath / std::filesystem::relative(path, dataPath)).string(), *buf)) {
							return cmd::error("{}: Failed to write file \"{}\".", self.getName(), path.string());
						}
					} else {
						return cmd::error("{}: Failed to read file \"{}\".", self.getName(), path.string());
					}
				} else {
					const auto outputPath = outDataPath / std::filesystem::relative(path, dataPath);
					std::filesystem::create_directories(outputPath.parent_path(), ec);
					if (ec) {
						return cmd::error("{}: Failed to create directory \"{}\": {}", self.getName(), path.parent_path().string(), ec.message());
					}
					std::filesystem::copy_file(path, outputPath, std::filesystem::copy_options::update_existing, ec);
					if (ec) {
						return cmd::error("{}: Failed to copy file \"{}\": {}", self.getName(), path.string(), ec.message());
					}
				}
			}
		}
		if (ec) {
			return cmd::error("{}: Failed to iterate directory \"{}\": {}", self.getName(), inputPath.string(), ec.message());
		}
		return std::nullopt;
	};

	if (auto err = copyData(cfgSubpath)) {
		return std::move(*err);
	}
	if (auto err = copyData(mapsSubpath)) {
		return std::move(*err);
	}
	if (auto err = copyData(fontsSubpath)) {
		return std::move(*err);
	}
	if (auto err = copyData(imagesSubpath)) {
		return std::move(*err);
	}
	if (auto err = copyData(screensSubpath)) {
		return std::move(*err);
	}
	if (auto err = copyData(shadersSubpath)) {
		return std::move(*err);
	}
	if (auto err = copyData(soundsSubpath)) {
		return std::move(*err);
	}
	if (!util::dumpFile((outDataPath / cfgSubpath / std::string_view{host_config_file}).string(), Game::getConfigHeader())) {
		return cmd::error("{}: Failed to create host config file!", self.getName());
	}
	if (!util::dumpFile((outDataPath / cfgSubpath / std::string_view{cl_config_file}).string(), GameClient::getConfigHeader())) {
		return cmd::error("{}: Failed to create game client config file!", self.getName());
	}
	if (!util::dumpFile((outDataPath / cfgSubpath / std::string_view{sv_config_file}).string(), GameServer::getConfigHeader())) {
		return cmd::error("{}: Failed to create game server config file!", self.getName());
	}
	if (!util::dumpFile((outDataPath / cfgSubpath / std::string_view{meta_sv_config_file}).string(), GameServer::getConfigHeader())) {
		return cmd::error("{}: Failed to create meta server config file!", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(host_writeconfig, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Save all archive cvars to the config file.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	static constexpr auto isCvarNonDefault = [](const auto& kv) {
		return ((kv.second.getFlags() & ConVar::ARCHIVE) != 0) && kv.second.getRawLocalValue() != kv.second.getDefaultValue();
	};

	static constexpr auto compareCvarRefs = [](const auto& lhs, const auto& rhs) {
		return lhs->first < rhs->first;
	};

	static constexpr auto getCvarCommand = [](const auto& kv) {
		return fmt::format("{} {}", kv->first, Script::escapedString(kv->second.getRawLocalValue()));
	};

	static constexpr auto compareBinds = [](const auto& lhs, const auto& rhs) {
		return lhs.input < rhs.input;
	};

	static constexpr auto getBindCommand = [](const auto& bind) {
		return fmt::format("{} {} {}", GET_COMMAND(bind).getName(), bind.input, Script::escapedString(bind.output));
	};

	using CvarRef = util::Reference<const std::pair<const std::string_view, ConVar&>>;
	using Refs = std::vector<CvarRef>;

	if (!util::dumpFile(fmt::format("{}/{}/{}", data_dir, data_subdir_cfg, host_config_file),
	                    fmt::format("{}\n"
	                                "\n"
	                                "// Cvars:\n"
	                                "{}\n"
	                                "\n"
	                                "// Binds:\n"
	                                "{}\n",
	                                Game::getConfigHeader(),
	                                ConVar::all() | util::filter(isCvarNonDefault) | util::collect<Refs>() | util::sort(compareCvarRefs) |
	                                    util::transform(getCvarCommand) | util::join('\n'),
	                                game.inputManager().getBinds() | util::sort(compareBinds) | util::transform(getBindCommand) | util::join('\n')))) {
		return cmd::error("{}: Failed to save config file \"{}\"!", self.getName(), host_config_file);
	}
	return cmd::done();
}

CON_COMMAND(map_is_loaded, "", ConCommand::NO_FLAGS, "Check if the map is loaded.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(game.map().isLoaded());
}

CON_COMMAND(map_get_char, "<x> <y>", ConCommand::NO_FLAGS, "Get the character at a certain position in the map.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[1], "x");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "y");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(std::string{game.map().get(Vec2{x, y})});
}

CON_COMMAND(map_is_solid, "<x> <y> [team] [dx dy]", ConCommand::NO_FLAGS,
            "Check if the map is solid at a certain position (for the given team and in the given direction).", {}, nullptr) {
	if (argv.size() != 3 && argv.size() != 4 && argv.size() != 6) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<Vec2::Length>(parseError, argv[1], "x");
	const auto y = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "y");

	auto team = Team::none();
	auto moveVector = std::optional<Vec2>{};
	if (argv.size() >= 4) {
		team = cmd::parseTeam(parseError, argv[3], "team");
	}
	if (argv.size() == 6) {
		const auto dx = cmd::parseNumber<Vec2::Length>(parseError, argv[4], "dx");
		const auto dy = cmd::parseNumber<Vec2::Length>(parseError, argv[5], "dy");
		moveVector = Vec2{dx, dy};
	}
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	const auto red = team == Team::red();
	const auto blue = team == Team::blue();

	if (moveVector) {
		return cmd::done(game.map().isSolid(Vec2{x, y}, red, blue, Direction{*moveVector}));
	}
	return cmd::done(game.map().isSolid(Vec2{x, y}, red, blue));
}

CON_COMMAND(map_find_path, "<start_x> <start_y> <destination_x> <destination_y> [team]", ConCommand::NO_FLAGS,
            "Use a pathfinding algorithm to find the shortest path between two points on the map.", {}, nullptr) {
	if (argv.size() != 5 && argv.size() != 6) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto start_x = cmd::parseNumber<Vec2::Length>(parseError, argv[1], "start x");
	const auto start_y = cmd::parseNumber<Vec2::Length>(parseError, argv[2], "start y");
	const auto destination_x = cmd::parseNumber<Vec2::Length>(parseError, argv[3], "destination x");
	const auto destination_y = cmd::parseNumber<Vec2::Length>(parseError, argv[4], "destination y");

	auto team = Team::none();
	if (argv.size() == 6) {
		team = cmd::parseTeam(parseError, argv[5], "team");
	}

	const auto red = team == Team::red();
	const auto blue = team == Team::blue();

	const auto path = game.map().findPath(Vec2{start_x, start_y}, Vec2{destination_x, destination_y}, red, blue);

	static constexpr auto formatVec2 = [](Vec2 v) {
		return fmt::format("{{{};{}}}", v.x, v.y);
	};

	return cmd::done(path | util::transform(formatVec2) | util::join('\n'));
}

CON_COMMAND(map_width, "", ConCommand::NO_FLAGS, "Get the width of the map.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(game.map().getWidth());
}

CON_COMMAND(map_height, "", ConCommand::NO_FLAGS, "Get the height of the map.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(game.map().getHeight());
}

CON_COMMAND(screenshot, "[options...] [filename]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Capture a screenshot of the main window and save to an image file.",
            cmd::opts(cmd::opt('f', "format", "Image format to save as (bmp|png|tga|jpg). Default is png.", cmd::OptionType::ARGUMENT_REQUIRED)),
            nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() > 1) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	auto format = std::string{"png"};
	if (const auto formatOpt = options['f']) {
		format = *formatOpt;
	}

	const auto filename = (args.size() == 1) ? std::string{args[0]} : fmt::format("{}_screenshot_{}", cvar_game, util::getLocalTimeStr("%Y-%m-%d"));
	const auto filepath = util::uniqueFilePath(fmt::format("{}/{}/{}", data_dir, data_subdir_screenshots, filename), format);

	const auto pixels = game.captureScreenshotRGBA8();
	const auto windowSize = game.getWindowSize();
	const auto width = static_cast<std::size_t>(windowSize.x);
	const auto height = static_cast<std::size_t>(windowSize.y);
	const auto channelCount = std::size_t{4};
	try {
		if (format == "bmp") {
			auto options = gfx::ImageOptionsBMP{};
			options.flipVertically = true;
			gfx::saveBMP(gfx::ImageView{pixels.data(), width, height, channelCount}, filepath.c_str(), options);
		} else if (format == "png") {
			auto options = gfx::ImageOptionsPNG{};
			options.flipVertically = true;
			gfx::savePNG(gfx::ImageView{pixels.data(), width, height, channelCount}, filepath.c_str(), options);
		} else if (format == "tga") {
			auto options = gfx::ImageOptionsTGA{};
			options.flipVertically = true;
			gfx::saveTGA(gfx::ImageView{pixels.data(), width, height, channelCount}, filepath.c_str(), options);
		} else if (format == "jpg") {
			auto options = gfx::ImageOptionsJPG{};
			options.flipVertically = true;
			gfx::saveJPG(gfx::ImageView{pixels.data(), width, height, channelCount}, filepath.c_str(), options);
		} else {
			return cmd::error("{}: Invalid image format \"{}\"! Valid formats are: bmp, png, tga, jpg.", self.getName(), format);
		}
	} catch (const gfx::Error& e) {
		return cmd::error("{}: Failed to save screenshot: {}", self.getName(), e.what());
	}
	return cmd::done();
}
