#ifndef AF2_GAME_HPP
#define AF2_GAME_HPP

#include "../console/process.hpp"         // Process
#include "../console/virtual_machine.hpp" // VirtualMachine
#include "../gui/canvas.hpp"              // gui::Canvas
#include "../gui/console.hpp"             // gui::Console
#include "../gui/text_input.hpp"          // gui::TextInput
#include "client/char_window.hpp"         // CharWindow
#include "client/input_manager.hpp"       // InputManager
#include "client/sound_manager.hpp"       // SoundManager
#include "data/color.hpp"                 // Color
#include "data/rectangle.hpp"             // Rect
#include "data/vector.hpp"                // Vec2
#include "shared/map.hpp"                 // Map

#include <SDL.h>       // SDL_..., Uint32
#include <chrono>      // std::chrono::...
#include <cstddef>     // std::byte, std::size_t
#include <cstdlib>     // EXIT_SUCCESS, EXIT_FAILURE
#include <fmt/core.h>  // fmt::format
#include <functional>  // std::function
#include <memory>      // std::unique_ptr, std::shared_ptr
#include <stdexcept>   // std::exception, std::runtime_error
#include <string>      // std::string, std::getline
#include <type_traits> // std::remove_pointer_t
#include <utility>     // std::forward, std::move, std::pair
#include <vector>      // std::vector

class GameState;
class GameServer;
class GameClient;
class MetaServer;
class MetaClient;

struct SDLContext final {
	[[nodiscard]] SDLContext() {
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
			throw std::runtime_error{fmt::format("Failed to initialize SDL: {}", SDL_GetError())};
		}
	}

	~SDLContext() {
		SDL_Quit();
	}

	SDLContext(const SDLContext&) = delete;
	SDLContext(SDLContext&&) = delete;
	auto operator=(const SDLContext&) -> SDLContext& = delete;
	auto operator=(SDLContext&&) -> SDLContext& = delete;
};

struct SDLManager {
	SDLManager() {
		static const auto context = SDLContext{};
	}
};

class Game final : private SDLManager {
public:
	[[nodiscard]] static auto getConfigHeader() -> std::string;

	Game(int argc, char* argv[]);
	~Game();

	Game(const Game&) = delete;
	Game(Game&&) = delete;

	auto operator=(const Game&) -> Game& = delete;
	auto operator=(Game&&) -> Game& = delete;

	[[nodiscard]] auto run() -> int; // Returns exit status.

	auto quit(int exitStatus = EXIT_SUCCESS) -> void;

	[[nodiscard]] auto isRunning() const noexcept -> bool;
	[[nodiscard]] auto isHeadless() const noexcept -> bool;
	[[nodiscard]] auto isFullscreen() const noexcept -> bool;
	[[nodiscard]] auto getFilename() const noexcept -> std::string_view;
	[[nodiscard]] auto getWindowSize() const noexcept -> Vec2;

	[[nodiscard]] auto getDesktopMode() -> SDL_DisplayMode;
	[[nodiscard]] auto getFullscreenModes() -> std::vector<SDL_DisplayMode>;

	auto updateFrameInterval() -> void;
	auto updateVertexShaderFilepath() -> void;
	auto updateFragmentShaderFilepath() -> void;
	auto updateFontFilepath() -> void;
	auto updateFontStaticSize() -> void;
	auto updateFontMatchSize() -> void;
	auto updateFontMatchSizeCoefficient() -> void;
	auto updateGlyphOffset() -> void;
	auto updateGridRatio() -> void;
	auto updateWindowMode() -> void;
	auto updateWindowTitle() -> void;
	auto updateWindowVSync() -> void;
	auto updateWindowIcon() -> void;
	auto updateBackgroundColor() -> void;
	auto updateConsoleRows() -> void;
	auto updateGlobalVolume() -> void;
	auto updateRolloffFactor() -> void;
	auto updateMaxSimultaneouslyPlayingSounds() -> void;

	[[nodiscard]] auto setState(std::unique_ptr<GameState> newState) -> bool;

	auto reset() -> void;

	[[nodiscard]] auto startGameServer() -> bool;
	[[nodiscard]] auto startGameClient() -> bool;

	[[nodiscard]] auto startMetaServer() -> bool;
	[[nodiscard]] auto startMetaClient() -> bool;

	auto stopGameServer() -> bool;
	auto stopGameClient() -> bool;

	auto stopMetaServer() -> bool;
	auto stopMetaClient() -> bool;

	[[nodiscard]] auto gameServer() noexcept -> GameServer*;
	[[nodiscard]] auto gameServer() const noexcept -> const GameServer*;

	[[nodiscard]] auto gameClient() noexcept -> GameClient*;
	[[nodiscard]] auto gameClient() const noexcept -> const GameClient*;

	[[nodiscard]] auto metaServer() noexcept -> MetaServer*;
	[[nodiscard]] auto metaServer() const noexcept -> const MetaServer*;

	[[nodiscard]] auto metaClient() noexcept -> MetaClient*;
	[[nodiscard]] auto metaClient() const noexcept -> const MetaClient*;

	[[nodiscard]] auto charWindow() noexcept -> CharWindow*;
	[[nodiscard]] auto charWindow() const noexcept -> const CharWindow*;

	[[nodiscard]] auto canvas() noexcept -> gui::Canvas&;
	[[nodiscard]] auto canvas() const noexcept -> const gui::Canvas&;

	[[nodiscard]] auto soundManager() noexcept -> SoundManager*;
	[[nodiscard]] auto soundManager() const noexcept -> const SoundManager*;

	[[nodiscard]] auto inputManager() noexcept -> InputManager&;
	[[nodiscard]] auto inputManager() const noexcept -> const InputManager&;

	[[nodiscard]] auto map() -> Map&;
	[[nodiscard]] auto map() const -> const Map&;

	auto clearConsole() -> void;

	auto activateConsole() -> void;
	auto deactivateConsole() -> void;

	[[nodiscard]] auto isConsoleActive() const -> bool;

	auto setConsoleModeConsole() -> void;
	auto setConsoleModeChat() -> void;
	auto setConsoleModeTeamChat() -> void;
	auto setConsoleModeTextInput(std::function<void(std::string_view)> callback) -> void;
	auto setConsoleModePassword(std::function<void(std::string_view)> callback) -> void;

	[[nodiscard]] auto captureScreenshotRGBA8() -> std::vector<std::byte>;

	auto drawDebugString(std::string str) -> void;

	auto print(std::string_view str, Color color = Color::white()) -> void;
	auto println(std::string str, Color color = Color::white()) -> void;
	auto println() -> void;

	auto warning(std::string str) -> void;
	auto error(std::string str) -> void;
	auto fatalError(std::string str) -> void;

	template <typename... Args>
	auto consoleCommand(Args&&... args) -> cmd::Result {
		if (const auto frame = m_consoleProcess->call(m_vm.globalEnv(), std::forward<Args>(args)..., Process::NO_FRAME, 0, nullptr)) {
			auto result = frame->run(*this, m_server.get(), m_client.get(), m_metaServer.get(), m_metaClient.get());
			m_vm.output(result);
			return result;
		}
		auto result = cmd::error("Console command stack overflow!");
		m_vm.output(result);
		return result;
	}

	template <typename... Args>
	auto awaitConsoleCommand(Args&&... args) -> cmd::Result {
		if (const auto frame = m_consoleProcess->call(m_vm.globalEnv(), std::forward<Args>(args)..., Process::NO_FRAME, 0, nullptr)) {
			auto result = frame->await(*this, m_server.get(), m_client.get(), m_metaServer.get(), m_metaClient.get());
			m_vm.output(result);
			if (!m_consoleProcess->done()) {
				return cmd::error("Console command await did not finish executing!");
			}
			return result;
		}
		auto result = cmd::error("Console command stack overflow!");
		m_vm.output(result);
		return result;
	}

private:
	using Clock = std::chrono::steady_clock;
	using TimePoint = Clock::time_point;
	using Duration = Clock::duration;

	struct WindowDeleter final {
		auto operator()(SDL_Window* p) const noexcept -> void {
			SDL_DestroyWindow(p);
		}
	};
	using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;

	struct GLContextDeleter final {
		auto operator()(SDL_GLContext p) const noexcept -> void {
			SDL_GL_DeleteContext(p);
		}
	};
	using GLContextPtr = std::unique_ptr<std::remove_pointer_t<SDL_GLContext>, GLContextDeleter>;

	struct SurfaceDeleter final {
		auto operator()(SDL_Surface* p) const noexcept -> void {
			SDL_FreeSurface(p);
		}
	};
	using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;

	auto autoComplete(gui::TextInput& textInput) -> void;

	auto createWindow(int width, int height, bool fullscreen) -> void;
	auto applyVertexShaderFilepath() -> void;
	auto applyFragmentShaderFilepath() -> void;
	auto applyFontFilepath() -> void;
	auto applyFontStaticSize() -> void;
	auto applyFontMatchSize() -> void;
	auto applyFontMatchSizeCoefficient() -> void;
	auto applyGlyphOffset() -> void;
	auto applyGridRatio() -> void;
	auto applyWindowMode() -> void;
	auto applyWindowTitle() -> void;
	auto applyWindowVSync() -> void;
	auto applyWindowIcon() -> void;
	auto applyBackgroundColor() -> void;
	auto applyConsoleRows() -> void;
	auto applyGlobalVolume() -> void;
	auto applyRolloffFactor() -> void;
	auto applyMaxSimultaneouslyPlayingSounds() -> void;

	auto runHeadless() -> void;
	auto runGraphical() -> void;

	int m_exitStatus = EXIT_SUCCESS;
	bool m_running = false;
	Duration m_frameInterval = Duration::zero();
	std::string_view m_filename;
	VirtualMachine m_vm{
		[this](std::string&& str) { this->print(std::move(str)); },
		[this](std::string&& str) { this->println(std::move(str), Color::yellow()); },
	};
	std::shared_ptr<Process> m_consoleProcess = m_vm.launchProcess(Process::CONSOLE | Process::ADMIN);
	WindowPtr m_window{};
	GLContextPtr m_glContext{};
	std::unique_ptr<CharWindow> m_charWindow{};
	gui::Canvas m_canvas{*this, m_vm};
	gui::Console m_console;
	gui::TextInput m_consoleTextInput;
	std::unique_ptr<SoundManager> m_soundManager{};
	InputManager m_inputManager{};
	Map m_map{};
	std::unique_ptr<GameServer> m_server{};
	std::unique_ptr<GameClient> m_client{};
	std::unique_ptr<MetaServer> m_metaServer{};
	std::unique_ptr<MetaClient> m_metaClient{};
	std::unique_ptr<GameState> m_gameState{};
	std::string m_debugText{};
};

#endif
