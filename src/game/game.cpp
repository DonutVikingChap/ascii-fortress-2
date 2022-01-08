#include "game.hpp"

#include "../console/command.hpp"                // cmd::...
#include "../console/commands/file_commands.hpp" // data_dir, data_subdir_images, data_subdir_fonts, data_subdir_shaders
#include "../console/commands/game_commands.hpp" // console_max_rows, host_..., r_..., fps_max, cvar_main, cvar_game, cmd_host_writeconfig, cmd_quit, cmd_say, cmd_say_team, headless
#include "../console/commands/process_commands.hpp"       // cmd_import, cmd_file, cmd_script
#include "../console/commands/sound_manager_commands.hpp" // volume, snd_rolloff, snd_max_simultaneous
#include "../console/con_command.hpp"                     // ConCommand, GET_COMMAND
#include "../console/convar.hpp"                          // ConVar...
#include "../console/environment.hpp"                     // Environment
#include "../console/script.hpp"                          // Script
#include "../console/suggestions.hpp"                     // Suggestions
#include "../debug.hpp"                                   // Msg, INFO_MSG
#include "../graphics/error.hpp"                          // gfx::Error
#include "../graphics/framebuffer.hpp"                    // gfx::Framebuffer
#include "../graphics/image.hpp"                          // gfx::Image
#include "../graphics/opengl.hpp"                         // GL..., gl...
#include "../gui/layout.hpp"                              // gui::CONSOLE_..., gui::CONSOLE_INPUT_...
#include "../utilities/algorithm.hpp"                     // util::subview, util::anyOf, util::append
#include "../utilities/file.hpp"                          // util::readFile
#include "../utilities/match.hpp"                         // util::match
#include "../utilities/reference.hpp"                     // util::Reference
#include "../utilities/string.hpp"                        // util::join, util::toString
#include "../utilities/time.hpp"                          // util::getLocalTimeStr
#include "client/game_client.hpp"                         // GameClient
#include "logger.hpp"                                     // logger::...
#include "meta/meta_client.hpp"                           // MetaClient
#include "meta/meta_server.hpp"                           // MetaServer
#include "server/game_server.hpp"                         // GameServer
#include "state/game_state.hpp"                           // GameState

#include <GL/glew.h> // GLEW_..., glew...
#include <algorithm> // std::sort, std::mismatch
#include <array>     // std::array
#include <atomic>    // std::atmoic_bool
#include <cassert>   // assert
#include <iostream>  // std::cout, std::cerr, std::endl, std::flush, std::cin
#include <thread>    // std::this_thread, std::thread

#ifdef NDEBUG
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // FreeConsole

#endif
#endif

using namespace std::literals::chrono_literals; // operator""s, operator""ms

namespace {

#ifndef NDEBUG
static void GLAPIENTRY debugOutputCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/,
                                           const GLchar* message, const void* /*userParam*/) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		if (type == GL_DEBUG_TYPE_ERROR) {
			std::cerr << "OpenGL ERROR: " << message << std::endl;
		} else {
			std::cerr << "OpenGL: " << message << std::endl;
		}
	}
}
#endif

} // namespace

auto Game::getConfigHeader() -> std::string {
	return fmt::format(
		"// This file is auto-generated every time the game is closed, and loaded every time the game is started.\n"
		"// Do not modify this file manually. Use the autoexec file instead.\n"
		"// Last generated {}.",
		util::getLocalTimeStr("%c"));
}

Game::Game(int argc, char* argv[])
	: m_filename(argv[0])
	, m_console(Vec2{gui::CONSOLE_X, gui::CONSOLE_Y}, Vec2{gui::CONSOLE_W, gui::CONSOLE_H}, Color::white(), static_cast<std::size_t>(console_max_rows))
	, m_consoleTextInput(Vec2{gui::CONSOLE_INPUT_X, gui::CONSOLE_INPUT_Y}, Vec2{gui::CONSOLE_INPUT_W, gui::CONSOLE_INPUT_H}, Color::white(),
                         "", nullptr, nullptr, nullptr, nullptr) {
	if (!m_consoleProcess) {
		this->fatalError("Failed to launch console process!");
		return;
	}

	this->updateFrameInterval();
	this->setConsoleModeConsole();

	// Execute command line commands.
	for (auto i = 0; i < argc; ++i) {
		if (argv[i][0] == '+') {
			auto command = Script::command({std::string_view{argv[i] + 1}});
			for (++i; i < argc && argv[i][0] != '+' && argv[i][0] != '-'; ++i) {
				command.emplace_back(argv[i]);
			}
			if (const auto result = this->consoleCommand(std::move(command)); result.status == cmd::Status::ERROR_MSG) {
				this->fatalError(result.value);
				return;
			}
		} else if (argv[i][0] == '-') {
			if (std::string_view{argv[i] + 1} == "headless") {
				if (const auto result = this->consoleCommand(Script::command({"headless", "1"})); result.status == cmd::Status::ERROR_MSG) {
					this->fatalError(result.value);
					return;
				}
			}
		}
	}

	// Execute init script.
	if (auto buf = util::readFile("init.cfg")) {
		if (const auto result = this->consoleCommand(GET_COMMAND(import),
		                                             std::array{cmd::Value{GET_COMMAND(script).getName()}, cmd::Value{std::move(*buf)}});
		    result.status == cmd::Status::ERROR_MSG) {
			this->fatalError(fmt::format("Init script failed!\n{}", result.value));
			return;
		}
	}

	// Execute game script.
	if (auto buf = util::readFile(fmt::format("{}/game.cfg", data_dir))) {
		if (const auto result = this->consoleCommand(GET_COMMAND(import),
		                                             std::array{cmd::Value{GET_COMMAND(script).getName()}, cmd::Value{std::move(*buf)}});
		    result.status == cmd::Status::ERROR_MSG) {
			this->fatalError(fmt::format("Game script failed!\n{}", result.value));
			return;
		}
	} else {
		this->fatalError("Failed to read game script!\nMake sure the game directory contains a game.cfg file.");
		return;
	}

	// Execute config script.
	if (const auto result = this->consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{host_config_file}});
	    result.status == cmd::Status::ERROR_MSG) {
		this->fatalError(fmt::format("Config failed!\n{}", result.value));
		return;
	}

	// Execute autoexec script.
	if (const auto result = this->consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{host_autoexec_file}});
	    result.status == cmd::Status::ERROR_MSG) {
		this->fatalError(fmt::format("Autoexec failed!\n{}", result.value));
		return;
	}

	// Open log file.
	if (!logger::open()) {
		this->warning("Failed to open log file!");
	}

	this->applyWindowMode();
	this->applyConsoleRows();

	m_vm.start();
	m_running = true;
	INFO_MSG(Msg::GENERAL, "Game initialized.");
}

Game::~Game() {
	INFO_MSG(Msg::GENERAL, "Game shutting down.");

	this->stopGameClient();
	this->stopGameServer();
	this->stopMetaClient();
	this->stopMetaServer();

	// Save config file.
	this->awaitConsoleCommand(GET_COMMAND(host_writeconfig));
}

auto Game::run() -> int {
	if (this->isRunning()) {
		this->reset();
		if (this->isHeadless()) {
			this->runHeadless();
		} else {
			this->runGraphical();
		}
	}
	return m_exitStatus;
}

auto Game::quit(int exitStatus) -> void {
	this->stopGameClient();
	this->stopGameServer();
	this->stopMetaClient();
	this->stopMetaServer();
	m_canvas.clear();
	m_gameState.reset();
	m_exitStatus = exitStatus;
	m_running = false;
}

auto Game::isRunning() const noexcept -> bool {
	return m_running;
}

auto Game::isHeadless() const noexcept -> bool { // NOLINT(readability-convert-member-functions-to-static)
	return static_cast<bool>(headless);
}

auto Game::isFullscreen() const noexcept -> bool {
	return m_window && (SDL_GetWindowFlags(m_window.get()) & SDL_WINDOW_FULLSCREEN) != 0;
}

auto Game::getFilename() const noexcept -> std::string_view {
	return m_filename;
}

auto Game::getWindowSize() const noexcept -> Vec2 {
	if (!m_window) {
		return Vec2{};
	}
	auto width = int{};
	auto height = int{};
	SDL_GetWindowSize(m_window.get(), &width, &height);
	return Vec2{width, height};
}

auto Game::getDesktopMode() -> SDL_DisplayMode {
	auto result = SDL_DisplayMode{};
	if (m_window) {
		if (const auto displayIndex = SDL_GetWindowDisplayIndex(m_window.get()); displayIndex >= 0) {
			if (SDL_GetDesktopDisplayMode(displayIndex, &result) != 0) {
				this->warning(fmt::format("Failed to get desktop display mode: {}", SDL_GetError()));
			}
		} else {
			this->warning(fmt::format("Failed to get window display index: {}", SDL_GetError()));
		}
	}
	return result;
}

auto Game::getFullscreenModes() -> std::vector<SDL_DisplayMode> {
	auto result = std::vector<SDL_DisplayMode>{};
	if (m_window) {
		if (const auto displayIndex = SDL_GetWindowDisplayIndex(m_window.get()); displayIndex >= 0) {
			if (const auto modeCount = SDL_GetNumDisplayModes(displayIndex); modeCount > 0) {
				result.resize(static_cast<std::size_t>(modeCount), SDL_DisplayMode{});
				for (auto i = 0; i < modeCount; ++i) {
					if (SDL_GetDisplayMode(displayIndex, i, &result[i]) != 0) {
						this->warning(fmt::format("Failed to get display mode: {}", SDL_GetError()));
						result.clear();
						break;
					}
				}
			} else {
				this->warning(fmt::format("Failed to get the number of display modes: {}", SDL_GetError()));
			}
		} else {
			this->warning(fmt::format("Failed to get window display index: {}", SDL_GetError()));
		}
	}
	return result;
}

auto Game::updateFrameInterval() -> void {
	if (fps_max > 0.0f) {
		m_frameInterval = std::chrono::duration_cast<Duration>(std::chrono::duration<float>{1.0f / static_cast<float>(fps_max)});
	} else {
		m_frameInterval = Duration::zero();
	}
}

auto Game::updateVertexShaderFilepath() -> void {
	if (this->isRunning()) {
		this->applyVertexShaderFilepath();
	}
}

auto Game::updateFragmentShaderFilepath() -> void {
	if (this->isRunning()) {
		this->applyFragmentShaderFilepath();
	}
}

auto Game::updateFontFilepath() -> void {
	if (this->isRunning()) {
		this->applyFontFilepath();
	}
}

auto Game::updateFontStaticSize() -> void {
	if (this->isRunning()) {
		this->applyFontStaticSize();
	}
}

auto Game::updateFontMatchSize() -> void {
	if (this->isRunning()) {
		this->applyFontMatchSize();
	}
}

auto Game::updateFontMatchSizeCoefficient() -> void {
	if (this->isRunning()) {
		this->applyFontMatchSizeCoefficient();
	}
}

auto Game::updateGlyphOffset() -> void {
	if (this->isRunning()) {
		this->applyGlyphOffset();
	}
}

auto Game::updateGridRatio() -> void {
	if (this->isRunning()) {
		this->applyGridRatio();
	}
}

auto Game::updateWindowMode() -> void {
	if (this->isRunning()) {
		this->applyWindowMode();
	}
}

auto Game::updateWindowTitle() -> void {
	if (this->isRunning()) {
		this->applyWindowTitle();
	}
}

auto Game::updateWindowVSync() -> void {
	if (this->isRunning()) {
		this->applyWindowVSync();
	}
}

auto Game::updateWindowIcon() -> void {
	if (this->isRunning()) {
		this->applyWindowIcon();
	}
}

auto Game::updateBackgroundColor() -> void {
	if (this->isRunning()) {
		this->applyBackgroundColor();
	}
}

auto Game::updateConsoleRows() -> void {
	if (this->isRunning()) {
		this->applyConsoleRows();
	}
}

auto Game::updateGlobalVolume() -> void {
	if (this->isRunning()) {
		this->applyGlobalVolume();
	}
}

auto Game::updateRolloffFactor() -> void {
	if (this->isRunning()) {
		this->applyRolloffFactor();
	}
}

auto Game::updateMaxSimultaneouslyPlayingSounds() -> void {
	if (this->isRunning()) {
		this->applyMaxSimultaneouslyPlayingSounds();
	}
}

auto Game::setState(std::unique_ptr<GameState> newState) -> bool {
	this->stopGameClient();
	this->stopGameServer();
	this->stopMetaClient();
	this->stopMetaServer();
	m_canvas.clear();
	m_gameState.reset();
	if (!newState || !newState->init()) {
		this->reset();
		return false;
	}
	m_gameState = std::move(newState);
	return true;
}

auto Game::reset() -> void {
	this->stopGameClient();
	this->stopGameServer();
	this->stopMetaClient();
	this->stopMetaServer();
	m_canvas.clear();
	m_gameState.reset();
	if (const auto result = this->consoleCommand(std::string_view{cvar_main}); result.status == cmd::Status::ERROR_MSG) {
		this->fatalError(fmt::format("Main failed!\n{}", result.value));
	}
}

auto Game::startGameServer() -> bool {
	if (auto handle = m_consoleProcess->launchChildProcess((host_server_admin) ? (Process::SERVER | Process::ADMIN) : Process::SERVER)) {
		m_server = std::make_unique<GameServer>(*this, m_vm, std::make_shared<Environment>(m_vm.globalEnv()), std::move(handle));
		if (!m_server) {
			return false;
		}

		if (!m_server->init()) {
			m_server->shutDown();
			m_server.reset();
			return false;
		}
		return true;
	}
	this->error("Couldn't launch server process!");
	return false;
}

auto Game::startGameClient() -> bool {
	if (this->isHeadless() || !m_charWindow || !m_soundManager) {
		return false;
	}

	m_client = std::make_unique<GameClient>(*this, m_vm, *m_charWindow, *m_soundManager, m_inputManager);
	if (!m_client) {
		return false;
	}

	if (!m_client->init()) {
		m_client->shutDown();
		m_client.reset();
		return false;
	}
	return true;
}

auto Game::startMetaServer() -> bool {
	m_metaServer = std::make_unique<MetaServer>(*this);
	if (!m_metaServer) {
		return false;
	}

	if (!m_metaServer->init()) {
		m_metaServer->shutDown();
		m_metaServer.reset();
		return false;
	}
	return true;
}

auto Game::startMetaClient() -> bool {
	m_metaClient = std::make_unique<MetaClient>(*this);
	if (!m_metaClient) {
		return false;
	}

	if (!m_metaClient->init()) {
		m_metaClient->shutDown();
		m_metaClient.reset();
		return false;
	}
	return true;
}

auto Game::stopGameServer() -> bool {
	if (m_server) {
		m_server->shutDown();
		m_server.reset();
		return true;
	}
	return false;
}

auto Game::stopGameClient() -> bool {
	if (m_client) {
		m_client->shutDown();
		m_client.reset();
		return true;
	}
	return false;
}

auto Game::stopMetaServer() -> bool {
	if (m_metaServer) {
		m_metaServer->shutDown();
		m_metaServer.reset();
		return true;
	}
	return false;
}

auto Game::stopMetaClient() -> bool {
	if (m_metaClient) {
		m_metaClient->shutDown();
		m_metaClient.reset();
		return true;
	}
	return false;
}

auto Game::gameServer() noexcept -> GameServer* {
	return m_server.get();
}

auto Game::gameServer() const noexcept -> const GameServer* {
	return m_server.get();
}

auto Game::gameClient() noexcept -> GameClient* {
	return m_client.get();
}

auto Game::gameClient() const noexcept -> const GameClient* {
	return m_client.get();
}

auto Game::metaServer() noexcept -> MetaServer* {
	return m_metaServer.get();
}

auto Game::metaServer() const noexcept -> const MetaServer* {
	return m_metaServer.get();
}

auto Game::metaClient() noexcept -> MetaClient* {
	return m_metaClient.get();
}

auto Game::metaClient() const noexcept -> const MetaClient* {
	return m_metaClient.get();
}

auto Game::charWindow() noexcept -> CharWindow* {
	return m_charWindow.get();
}

auto Game::charWindow() const noexcept -> const CharWindow* {
	return m_charWindow.get();
}

auto Game::canvas() noexcept -> gui::Canvas& {
	return m_canvas;
}

auto Game::canvas() const noexcept -> const gui::Canvas& {
	return m_canvas;
}

auto Game::soundManager() noexcept -> SoundManager* {
	return m_soundManager.get();
}

auto Game::soundManager() const noexcept -> const SoundManager* {
	return m_soundManager.get();
}

auto Game::inputManager() noexcept -> InputManager& {
	return m_inputManager;
}

auto Game::inputManager() const noexcept -> const InputManager& {
	return m_inputManager;
}

auto Game::map() -> Map& {
	return m_map;
}

auto Game::map() const -> const Map& {
	return m_map;
}

auto Game::clearConsole() -> void {
	m_console.clear();
}

auto Game::activateConsole() -> void {
	m_consoleTextInput.activate();
}

auto Game::deactivateConsole() -> void {
	m_consoleTextInput.deactivate();
}

auto Game::isConsoleActive() const -> bool {
	return m_consoleTextInput.isActivated();
}

auto Game::setConsoleModeConsole() -> void {
	m_consoleTextInput.setSubmitFunction([this](gui::TextInput& textInput) {
		if (const auto text = std::string{textInput.getText()}; !text.empty()) {
			INFO_MSG(Msg::CONSOLE_EVENT, "User submitted console command {}.", Script::escapedString(text));
			this->println(fmt::format("] {}", text));
			textInput.addToHistory(text);
			textInput.clearText();
			m_console.resetScroll();
			this->consoleCommand(text); // NOTE: This needs to be called at the end of the function, since the console command might switch the console mode!
		} else {
			textInput.deactivate();
			m_console.resetScroll();
		}
	});
	m_consoleTextInput.setAutoCompleteFunction([this](gui::TextInput& textInput) { this->autoComplete(textInput); });
}

auto Game::setConsoleModeChat() -> void {
	m_consoleTextInput.setSubmitFunction([this](gui::TextInput& textInput) {
		const auto& text = textInput.getText();
		INFO_MSG(Msg::CONSOLE_EVENT | Msg::CHAT, "User submitted chat message {}.", Script::escapedString(text));
		this->consoleCommand(GET_COMMAND(say), std::array{std::string{text}});
		textInput.addToHistory(std::string{text});
		textInput.clearText();
		textInput.deactivate();
		m_console.resetScroll();
		this->setConsoleModeConsole();
	});
	m_consoleTextInput.setAutoCompleteFunction(nullptr);
}

auto Game::setConsoleModeTeamChat() -> void {
	m_consoleTextInput.setSubmitFunction([this](gui::TextInput& textInput) {
		const auto& text = textInput.getText();
		INFO_MSG(Msg::CONSOLE_EVENT | Msg::CHAT, "User submitted team chat message {}.", Script::escapedString(text));
		this->consoleCommand(GET_COMMAND(say_team), std::array{std::string{text}});
		textInput.addToHistory(std::string{text});
		textInput.clearText();
		textInput.deactivate();
		m_console.resetScroll();
		this->setConsoleModeConsole();
	});
	m_consoleTextInput.setAutoCompleteFunction(nullptr);
}

auto Game::setConsoleModeTextInput(std::function<void(std::string_view)> callback) -> void {
	m_consoleTextInput.setSubmitFunction([this, callback = std::move(callback)](gui::TextInput& textInput) {
		auto text = std::string{textInput.getText()};
		textInput.clearText();
		textInput.deactivate();
		m_console.resetScroll();
		callback(text);
		this->setConsoleModeConsole();
	});
	m_consoleTextInput.setAutoCompleteFunction(nullptr);
}

auto Game::setConsoleModePassword(std::function<void(std::string_view)> callback) -> void {
	m_consoleTextInput.setPrivate(true);
	m_consoleTextInput.setSubmitFunction([this, callback = std::move(callback)](gui::TextInput& textInput) {
		auto text = std::string{textInput.getText()};
		textInput.clearText();
		textInput.deactivate();
		textInput.setPrivate(false);
		m_console.resetScroll();
		callback(text);
		this->setConsoleModeConsole();
	});
	m_consoleTextInput.setAutoCompleteFunction(nullptr);
}

auto Game::captureScreenshotRGBA8() -> std::vector<std::byte> {
	auto result = std::vector<std::byte>{};
	if (m_window) {
		const auto windowSize = this->getWindowSize();
		const auto width = static_cast<std::size_t>(windowSize.x);
		const auto height = static_cast<std::size_t>(windowSize.y);

		result.resize(width * height * 4);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, result.data());
	}
	return result;
}

auto Game::drawDebugString(std::string str) -> void {
	if (this->isHeadless()) {
		return;
	}
	if (m_debugText.empty()) {
		m_debugText = std::move(str);
	} else {
		m_debugText.push_back('\n');
		m_debugText.append(str);
	}
}

auto Game::print(std::string_view str, Color color) -> void {
	DEBUG_MSG(Msg::CONSOLE_OUTPUT, "[CONSOLE] {}", (!str.empty() && str.back() == '\n') ? str.substr(0, str.size() - 1) : str);
	if (this->isHeadless()) {
		std::cout << str << std::flush;
	} else {
		m_console.print(str, color);
	}
}

auto Game::println(std::string str, Color color) -> void {
	DEBUG_MSG(Msg::CONSOLE_OUTPUT, "[CONSOLE] {}", str);
	if (this->isHeadless()) {
		std::cout << str << std::endl;
	} else {
		str.push_back('\n');
		m_console.print(str, color);
	}
}

auto Game::println() -> void {
	DEBUG_MSG(Msg::CONSOLE_OUTPUT, "[CONSOLE] ");
	if (this->isHeadless()) {
		std::cout << std::endl;
	} else {
		m_console.print("\n", Color{});
	}
}

auto Game::warning(std::string str) -> void {
	logger::logWarning(str);
	if (this->isHeadless()) {
		std::cout << str << std::endl;
	} else {
		str.push_back('\n');
		m_console.print(str, Color::yellow());
	}
}

auto Game::error(std::string str) -> void {
	logger::logError(str);
	if (this->isHeadless()) {
		std::cerr << str << std::endl;
	} else {
		str.push_back('\n');
		m_console.print(str, Color::red());
	}
}

auto Game::fatalError(std::string str) -> void {
	logger::logFatalError(str);
	if (this->isHeadless() || !this->isRunning()) {
		std::cerr << str << std::endl;
	} else {
		str.push_back('\n');
		m_console.print(str, Color::red());
	}
	this->quit(EXIT_FAILURE);
}

auto Game::autoComplete(gui::TextInput& textInput) -> void {
	if (const auto& text = textInput.getText(); !text.empty()) {
		if (const auto commands = Script::parse(text); !commands.empty()) {
			const auto& command = commands.back();
			assert(!command.empty());
			const auto i = (text.compare(text.size() - command.back().value.size(), command.back().value.size(), command.back().value) == 0) ?
			                   command.size() - 1 :
                               command.size();

			const auto doAutoCompleteRaw = [&](std::string_view str) {
				if (i == command.size()) {
					textInput.setText(fmt::format("{}{}", text, str));
				} else {
					textInput.setText(fmt::format("{}{}", text.substr(0, text.size() - command.back().value.size()), str));
				}
			};

			const auto doAutoComplete = [&](std::string_view str) {
				if (util::anyOf(str, Script::isWhitespace)) {
					const auto escapedStr = Script::escapedString(str);
					doAutoCompleteRaw(escapedStr);
				} else {
					doAutoCompleteRaw(str);
				}
			};

			auto candidates = Suggestions{};

			// Check aliases.
			for (const auto& [name, alias] : m_vm.getGlobalEnv().aliases) {
				if (name == command.front().value) {
					this->println(fmt::format("alias {} {{{}}}", name, Script::commandString(alias)), Color::gray());
					m_console.resetScroll();
					return;
				}
				if (i == 0 && name.compare(0, command.front().value.size(), command.front().value) == 0) {
					candidates.emplace_back(name);
				}
			}

			// Check local objects.
			for (const auto& [name, obj] : m_vm.getGlobalEnv().objects) {
				if (name == command.front().value) {
					this->println(util::match(obj)(
									  [&, &name = name](const Environment::Variable& var) {
										  return fmt::format("var {} {}", name, Script::escapedString(var.value));
									  },
									  [&, &name = name](const Environment::Constant& constant) {
										  return fmt::format("const {} {}", name, Script::escapedString(constant.value));
									  },
									  [&, &name = name](const Environment::Function& function) {
										  if (function.parameters.empty()) {
											  return fmt::format("function {} {{...}}", name);
										  }
										  return fmt::format("function {} {} {{...}}", name, function.parameters | util::join(' '));
									  },
									  [&, &name = name](const Environment::Array& arr) {
										  return fmt::format("array {} {{\n{}}}", name, Environment::arrayString(arr));
									  },
									  [&, &name = name](const Environment::Table& table) {
										  return fmt::format("table {} {{\n{}}}", name, Environment::tableString(table));
									  }),
					              Color::gray());
					m_console.resetScroll();
					return;
				}
				if (i == 0 && name.compare(0, command.front().value.size(), command.front().value) == 0) {
					candidates.emplace_back(name);
				}
			}

			// Check commands.
			for (const auto& [name, cmd] : ConCommand::all()) {
				if (name == command.front().value) {
					auto suggestions =
						cmd.getSuggestions(command, i, *this, m_server.get(), m_client.get(), m_metaServer.get(), m_metaClient.get(), m_vm);
					if (i != command.size()) {
						util::eraseIf(suggestions, [&](const auto& suggestion) {
							return suggestion.compare(0, command.back().value.size(), command.back().value) != 0;
						});
					}

					if (suggestions.empty()) {
						this->println(fmt::format("{} {}", name, cmd.getParameters()), Color::gray());
						m_console.resetScroll();
						return;
					}

					if (suggestions.size() == 1) {
						doAutoComplete(suggestions.front());
						return;
					}

					candidates.insert(candidates.end(), suggestions.begin(), suggestions.end());
				} else if (i == 0 && name.compare(0, command.front().value.size(), command.front().value) == 0) {
					candidates.emplace_back(name);
				}
			}

			// Check cvars.
			for (const auto& [name, cvar] : ConVar::all()) {
				if (name == command.front().value) {
					this->println(cvar.format((m_consoleProcess->getUserFlags() & Process::ADMIN) != 0,
					                          (m_consoleProcess->getUserFlags() & Process::REMOTE) != 0,
					                          false,
					                          false,
					                          false,
					                          false),
					              Color::gray());
					m_console.resetScroll();
					return;
				}
				if (i == 0 && name.compare(0, command.front().value.size(), command.front().value) == 0) {
					candidates.emplace_back(name);
				}
			}

			// Perform auto-complete.
			if (candidates.empty()) {
				return;
			}

			if (candidates.size() == 1) {
				doAutoComplete(candidates.front());
				return;
			}

			auto common = std::string{candidates.front()};
			for (const auto& candidate : util::subview(candidates, 1)) {
				const auto its = std::mismatch(candidate.begin(), candidate.end(), common.begin(), common.end());
				common.erase(its.second, common.end());
			}

			if (!common.empty() && command.back().value != common) {
				doAutoComplete(std::move(common));
			} else {
				std::sort(candidates.begin(), candidates.end());
				this->println(candidates | util::join(' '), Color::gray());
				m_console.resetScroll();
			}
		}
	}
}

auto Game::createWindow(int width, int height, bool fullscreen) -> void {
	m_glContext.reset();
	m_window.reset();

	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0) {
		this->warning(fmt::format("Failed to enable double buffering: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1) != 0) {
		this->warning(fmt::format("Failed to enable hardware acceleration: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8) != 0) {
		this->warning(fmt::format("Failed to enable 8-bit red color channel: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8) != 0) {
		this->warning(fmt::format("Failed to enable 8-bit green color channel: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8) != 0) {
		this->warning(fmt::format("Failed to enable 8-bit blue color channel: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8) != 0) {
		this->warning(fmt::format("Failed to enable 8-bit alpha color channel: {}", SDL_GetError()));
	}
#ifdef __EMSCRIPTEN__
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0) {
		this->warning(fmt::format("Failed to set OpenGL context major version to 3: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) != 0) {
		this->warning(fmt::format("Failed to set OpenGL context minor version to 0: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) != 0) {
		this->warning(fmt::format("Failed to set OpenGL context profile to ES: {}", SDL_GetError()));
	}
#else
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0) {
		this->warning(fmt::format("Failed to set OpenGL context major version to 3: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0) {
		this->warning(fmt::format("Failed to set OpenGL context minor version to 3: {}", SDL_GetError()));
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) != 0) {
		this->warning(fmt::format("Failed to set OpenGL context profile to core: {}", SDL_GetError()));
	}
#endif

	auto flags = Uint32{SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE};
	if (fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	m_window.reset(SDL_CreateWindow(std::string{r_window_title}.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags));
	if (!m_window) {
		throw std::runtime_error{fmt::format("Failed to create window: {}", SDL_GetError())};
	}

	m_glContext.reset(SDL_GL_CreateContext(m_window.get()));
	if (!m_glContext) {
		throw std::runtime_error{fmt::format("Failed to create OpenGL context: {}", SDL_GetError())};
	}

	glewExperimental = GL_TRUE;
	if (const auto err = glewInit(); err != GLEW_OK) {
		throw std::runtime_error{fmt::format("Failed to initialize GLEW: {}", glewGetErrorString(err))};
	}

#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(debugOutputCallback, nullptr);
#endif
}

auto Game::applyVertexShaderFilepath() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	try {
		if (r_shader_vert.empty()) {
			m_charWindow->setVertexShaderFilepath(std::string{});
		} else {
			m_charWindow->setVertexShaderFilepath(fmt::format("{}/{}/{}", data_dir, data_subdir_shaders, r_shader_vert));
		}
	} catch (const gfx::Error& e) {
		this->warning(fmt::format("Failed to set vertex shader: {}", e.what()));
	}
}

auto Game::applyFragmentShaderFilepath() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	try {
		if (r_shader_frag.empty()) {
			m_charWindow->setFragmentShaderFilepath(std::string{});
		} else {
			m_charWindow->setFragmentShaderFilepath(fmt::format("{}/{}/{}", data_dir, data_subdir_shaders, r_shader_frag));
		}
	} catch (const gfx::Error& e) {
		this->warning(fmt::format("Failed to set fragment shader: {}", e.what()));
	}
}

auto Game::applyFontFilepath() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	try {
		if (r_font.empty()) {
			m_charWindow->setFontFilepath(std::string{});
		} else {
			m_charWindow->setFontFilepath(fmt::format("{}/{}/{}", data_dir, data_subdir_fonts, r_font));
		}
	} catch (const gfx::Error& e) {
		this->warning(fmt::format("Failed to set font: {}", e.what()));
	}
}

auto Game::applyFontStaticSize() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	m_charWindow->setFontStaticSize(r_font_size);
}

auto Game::applyFontMatchSize() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	m_charWindow->setFontMatchSize(r_font_match_size);
}

auto Game::applyFontMatchSizeCoefficient() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	m_charWindow->setFontMatchSizeCoefficient(r_font_match_size_coefficient);
}

auto Game::applyGlyphOffset() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	m_charWindow->setGlyphOffset(Vec2{static_cast<int>(r_glyph_offset_x), static_cast<int>(r_glyph_offset_y)});
}

auto Game::applyGridRatio() -> void {
	if (this->isHeadless() || !m_charWindow) {
		return;
	}
	m_charWindow->setGridRatio(r_ratio);
}

auto Game::applyWindowMode() -> void {
	if (this->isHeadless()) {
		return;
	}
	if (!m_window) {
		this->createWindow(r_width, r_height, r_fullscreen_mode > 0);
		this->applyWindowIcon();
		this->applyWindowVSync();
		this->applyBackgroundColor();
		m_charWindow = std::make_unique<CharWindow>();
		this->applyVertexShaderFilepath();
		this->applyFragmentShaderFilepath();
		this->applyFontFilepath();
		this->applyFontStaticSize();
		this->applyFontMatchSize();
		this->applyFontMatchSizeCoefficient();
		this->applyGridRatio();
		m_soundManager = std::make_unique<SoundManager>();
		this->applyGlobalVolume();
		this->applyRolloffFactor();
		this->applyMaxSimultaneouslyPlayingSounds();
	}
	const auto& fullscreenModes = this->getFullscreenModes();
	if (const auto modeIndex = static_cast<std::size_t>(r_fullscreen_mode); modeIndex > 0 && modeIndex <= fullscreenModes.size()) {
		INFO_MSG(Msg::GENERAL, "Setting fullscreen mode to {}/{}.", modeIndex, fullscreenModes.size());
		const auto& mode = fullscreenModes[modeIndex - 1];
		r_width.setSilent(util::toString(mode.w));
		r_height.setSilent(util::toString(mode.h));
		SDL_SetWindowDisplayMode(m_window.get(), &mode);
		if (!this->isFullscreen()) {
			if (SDL_SetWindowFullscreen(m_window.get(), SDL_GetWindowFlags(m_window.get()) | SDL_WINDOW_FULLSCREEN) != 0) {
				this->warning(fmt::format("Failed to enable fullscreen: {}", SDL_GetError()));
			}
		}
	} else if (this->isFullscreen()) {
		r_width.setSilent(r_width_windowed.cvar().getRaw());
		r_height.setSilent(r_height_windowed.cvar().getRaw());
		if (SDL_SetWindowFullscreen(m_window.get(), SDL_GetWindowFlags(m_window.get()) & ~SDL_WINDOW_FULLSCREEN) != 0) {
			this->warning(fmt::format("Failed to disable fullscreen: {}", SDL_GetError()));
		}
		SDL_RestoreWindow(m_window.get());
		SDL_SetWindowSize(m_window.get(), r_width, r_height);
	} else {
		r_width_windowed.setSilent(r_width.cvar().getRaw());
		r_height_windowed.setSilent(r_height.cvar().getRaw());
		SDL_RestoreWindow(m_window.get());
		SDL_SetWindowSize(m_window.get(), r_width, r_height);
	}
	assert(m_charWindow);
	assert(m_consoleProcess);
	m_charWindow->setWindowSize(Vec2{static_cast<int>(r_width), static_cast<int>(r_height)});
	if (m_consoleProcess->defined(m_vm.globalEnv(), "on_window_resize")) {
		if (!m_consoleProcess->call(m_vm.globalEnv(), Script::command({"on_window_resize"}))) {
			this->warning("Console command stack overflow!");
		}
	}
}

auto Game::applyWindowTitle() -> void {
	if (!m_window) {
		return;
	}
	SDL_SetWindowTitle(m_window.get(), std::string{r_window_title}.c_str());
}

auto Game::applyWindowVSync() -> void {
	if (!m_window) {
		return;
	}
	if (r_vsync) {
		if (SDL_GL_SetSwapInterval(-1) != 0 && SDL_GL_SetSwapInterval(1) != 0) {
			this->warning(fmt::format("Failed to enable V-Sync: {}", SDL_GetError()));
		}
	} else {
		if (SDL_GL_SetSwapInterval(0) != 0) {
			this->warning(fmt::format("Failed to disable V-Sync: {}", SDL_GetError()));
		}
	}
}

auto Game::applyWindowIcon() -> void {
	if (!m_window) {
		return;
	}
	try {
		auto image = gfx::Image{fmt::format("{}/{}/{}", data_dir, data_subdir_images, r_icon).c_str()};
		const auto width = static_cast<int>(image.getWidth());
		const auto height = static_cast<int>(image.getHeight());
		const auto channelCount = static_cast<int>(image.getChannelCount());
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		const auto shift = (4 - channelCount) * 8;
		const auto rMask = Uint32{0xFF000000} >> shift;
		const auto gMask = Uint32{0x00FF0000} >> shift;
		const auto bMask = Uint32{0x0000FF00} >> shift;
		const auto aMask = Uint32{0x000000FF} >> shift;
#else
		const auto rMask = (channelCount >= 1) ? Uint32{0x000000FF} : Uint32{0x00000000};
		const auto gMask = (channelCount >= 2) ? Uint32{0x0000FF00} : Uint32{0x00000000};
		const auto bMask = (channelCount >= 3) ? Uint32{0x00FF0000} : Uint32{0x00000000};
		const auto aMask = (channelCount >= 4) ? Uint32{0xFF000000} : Uint32{0x00000000};
#endif
		const auto surface = SurfacePtr{
			SDL_CreateRGBSurfaceFrom(image.getPixels(), width, height, channelCount * 8, channelCount * width, rMask, gMask, bMask, aMask)};
		SDL_SetWindowIcon(m_window.get(), surface.get());
	} catch (const gfx::Error& e) {
		this->warning(fmt::format("Failed to set window icon: {}", e.what()));
	}
}

auto Game::applyBackgroundColor() -> void {
	const auto backgroundColor = Color{r_background_color};
	const auto backgroundR = static_cast<float>(backgroundColor.r) * (1.0f / 255.0f);
	const auto backgroundG = static_cast<float>(backgroundColor.g) * (1.0f / 255.0f);
	const auto backgroundB = static_cast<float>(backgroundColor.b) * (1.0f / 255.0f);
	const auto backgroundA = static_cast<float>(backgroundColor.a) * (1.0f / 255.0f);
	glClearColor(backgroundR, backgroundG, backgroundB, backgroundA);
}

auto Game::applyConsoleRows() -> void {
	m_console.setMaxRows(static_cast<std::size_t>(console_max_rows));
}

auto Game::applyGlobalVolume() -> void {
	if (m_soundManager) {
		m_soundManager->setGlobalVolume(volume * 0.01f);
	}
}

auto Game::applyRolloffFactor() -> void {
	if (m_soundManager) {
		m_soundManager->setRolloffFactor(snd_rolloff);
	}
}

auto Game::applyMaxSimultaneouslyPlayingSounds() -> void {
	if (m_soundManager) {
		m_soundManager->setMaxSimultaneouslyPlayingSounds(static_cast<std::size_t>(snd_max_simultaneous));
	}
}

auto Game::runHeadless() -> void {
	auto commandQueue = Script{};
	auto accessingCommandQueue = std::atomic_bool{};
	accessingCommandQueue.store(false);

	auto inputThread = std::thread{[&] {
		try {
			while (m_running) {
				auto command = std::string{};
				if (std::getline(std::cin, command)) {
					auto expected = false;
					while (!accessingCommandQueue.compare_exchange_weak(expected, true)) {
						std::this_thread::yield();
					}
					auto script = Script::parse(command);
					command.clear();

					const auto wasQuit = !script.empty() && script.front().size() == 1 && script.front().front().value == GET_COMMAND(quit).getName();

					util::append(commandQueue, std::move(script));
					accessingCommandQueue.store(false);
					if (wasQuit) {
						break;
					}
				}
				std::cin.clear();
			}
		} catch (const std::exception& e) {
			this->fatalError(fmt::format("Exception thrown in input thread: {}", e.what()));
		} catch (...) {
			this->fatalError("Unknown exception thrown in input thread!");
		}
	}};

	try {
		auto lastFrameTime = Clock::now();
		while (m_running) {
			// Get time delta since last "real" frame.
			const auto thisTime = Clock::now();
			const auto clockDeltaTime = thisTime - lastFrameTime;

			// Check if we're under the framerate limit.
			if (clockDeltaTime < m_frameInterval) {
				// Sleep until next frame. Headless mode is not as timing critical as graphical mode.
				if (const auto timeUntilNextFrame = m_frameInterval - clockDeltaTime; timeUntilNextFrame > 1ms) {
					std::this_thread::sleep_for(timeUntilNextFrame);
				} else {
					std::this_thread::yield();
				}
				continue;
			}
			lastFrameTime = thisTime;

			// Get delta time.
			const auto frameDeltaTime = std::chrono::duration<float>(clockDeltaTime).count();
			const auto deltaTime = frameDeltaTime * host_timescale;

			// Execute queued commands.
			if (auto expected = false; accessingCommandQueue.compare_exchange_strong(expected, true)) {
				this->consoleCommand(std::move(commandQueue));
				commandQueue.clear();
				accessingCommandQueue.store(false);
			}

			// Run the virtual machine.
			m_vm.run(deltaTime, *this, m_server.get(), m_client.get(), m_metaServer.get(), m_metaClient.get());

			// Run the console process.
			m_vm.output(m_consoleProcess->run(*this, m_server.get(), m_client.get(), m_metaServer.get(), m_metaClient.get()));

			// Update the game state.
			if (m_gameState) {
				m_gameState->update(deltaTime);
			}
		}
	} catch (...) {
		m_running = false;
		inputThread.join();
		throw;
	}
	inputThread.join();
}

auto Game::runGraphical() -> void {
	assert(m_window);
	assert(m_charWindow);
#ifdef NDEBUG
#ifdef _WIN32
	FreeConsole();
#endif
#endif
	auto lastFrameTime = Clock::now();
	auto lastFpsTime = lastFrameTime;
	auto frameCounter = 0u;
	auto lastCountedFrames = 0u;
	while (m_running) {
		// Get time delta since last "real" frame.
		const auto thisTime = Clock::now();
		const auto clockDeltaTime = thisTime - lastFrameTime;

		// Check if we're under the framerate limit.
		if (clockDeltaTime < m_frameInterval) {
			continue;
		}
		lastFrameTime = thisTime;

		// Update frame counter.
		++frameCounter;
		if (thisTime - lastFpsTime >= 1s) {
			lastFpsTime = thisTime;
			lastCountedFrames = frameCounter;
			frameCounter = 0;
		}

		// Get delta time.
		const auto actualDeltaTime = std::chrono::duration<float>(clockDeltaTime).count();
		const auto deltaTime = actualDeltaTime * host_timescale;

		// Update framerate indicator and counter strings.
		if (r_showfps) {
			this->drawDebugString(fmt::format("FPS: {} Hz\nFT: {} ms\nFrames: {}", 1.0f / actualDeltaTime, actualDeltaTime * 1000.0f, lastCountedFrames));
		}

		// Update input manager.
		m_inputManager.update();

		// Handle events.
		{
			auto inputCommands = Script{};
			for (auto e = SDL_Event{}; SDL_PollEvent(&e) != 0;) {
				if (e.type == SDL_QUIT) {
					if (this->consoleCommand(GET_COMMAND(quit)).status == cmd::Status::ERROR_MSG) {
						this->quit();
					}
				} else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
					r_width.setSilent(util::toString(e.window.data1));
					r_height.setSilent(util::toString(e.window.data2));
					if (!this->isFullscreen()) {
						r_width_windowed.setSilent(r_width.cvar().getRaw());
						r_height_windowed.setSilent(r_height.cvar().getRaw());
					}
					INFO_MSG(Msg::GENERAL, "Window resized to {}x{}.", e.window.data1, e.window.data2);
					m_charWindow->setWindowSize(Vec2{e.window.data1, e.window.data2});
					if (m_consoleProcess->defined(m_vm.globalEnv(), "on_window_resize")) {
						if (!m_consoleProcess->call(m_vm.globalEnv(), Script::command({"on_window_resize"}))) {
							this->warning("Console command stack overflow!");
						}
					}
				}

				// Let the input manager handle the event.
				{
					const auto active = (m_server || m_client || m_metaServer) && !m_consoleTextInput.isActivated() && !m_canvas.hasMenu();
					util::append(inputCommands, m_inputManager.handleEvent(e, active));
				}

				// Let the gui handle the event.
				if (!m_consoleTextInput.isActivated()) {
					m_canvas.handleEvent(e, *m_charWindow);
				}

				// Let the game state handle the event.
				if (!m_consoleTextInput.isActivated() && m_gameState) {
					m_gameState->handleEvent(e, *m_charWindow);
				}

				// Let the console handle the event.
				m_console.handleEvent(e, *m_charWindow);
				m_consoleTextInput.handleEvent(e, *m_charWindow);
			}
			if (!inputCommands.empty()) {
				this->consoleCommand(std::move(inputCommands));
			}
		}

		// Run the virtual machine.
		m_vm.run(deltaTime, *this, m_server.get(), m_client.get(), m_metaServer.get(), m_metaClient.get());

		// Run the console process.
		m_vm.output(m_consoleProcess->run(*this, m_server.get(), m_client.get(), m_metaServer.get(), m_metaClient.get()));

		// Update the GUI.
		m_canvas.update(deltaTime);

		// Update the char window.
		m_charWindow->update(deltaTime);

		// Update the game state.
		if (m_gameState) {
			m_gameState->update(deltaTime);
		}

		// Update the console.
		m_console.update(deltaTime);
		m_consoleTextInput.update(deltaTime);

		// Draw the console.
		m_console.draw(*m_charWindow);
		m_consoleTextInput.draw(*m_charWindow);

		// Draw the game state.
		if (m_gameState) {
			m_gameState->draw(*m_charWindow);
		}

		// Draw the GUI.
		m_canvas.draw(*m_charWindow);

		// Draw debug text.
		m_charWindow->addText(Vec2{static_cast<int>(r_debug_text_offset_x), static_cast<int>(r_debug_text_offset_y)},
		                      r_debug_text_scale_x,
		                      r_debug_text_scale_y,
		                      std::move(m_debugText),
		                      r_debug_text_color);
		m_debugText.clear();

		// Clear the framebuffer.
		auto& framebuffer = gfx::Framebuffer::getDefault();
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
		glClear(GL_COLOR_BUFFER_BIT);

		// Render the char window onto the framebuffer.
		m_charWindow->render(framebuffer);

		// Clear the char window for next frame.
		m_charWindow->clear();
		m_charWindow->clearText();

		// Flip the screen buffer.
		SDL_GL_SwapWindow(m_window.get());
	}
}
