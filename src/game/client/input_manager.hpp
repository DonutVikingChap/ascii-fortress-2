#ifndef AF2_CLIENT_INPUT_MANAGER_HPP
#define AF2_CLIENT_INPUT_MANAGER_HPP

#include "../../console/script.hpp" // Script
#include "../data/actions.hpp"      // Actions, Action

#include <SDL.h>         // SDL_...
#include <cstddef>       // std::size_t
#include <memory>        // std::unique_ptr
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

class InputManager final {
public:
	struct BindView final {
		std::string_view input{};
		std::string_view output{};
	};

	[[nodiscard]] static auto actionMap() -> const std::unordered_map<std::string_view, Actions>&;
	[[nodiscard]] static auto validInputs() -> const std::vector<std::string>&;
	[[nodiscard]] static auto isValidInput(std::string_view input) -> bool;

	auto update() -> void;

	[[nodiscard]] auto handleEvent(const SDL_Event& e, bool active) -> Script;

	[[nodiscard]] auto bind(std::string input, std::string output) -> bool;

	auto unbind(std::string_view input) -> bool;
	auto unbindAll() noexcept -> void;

	[[nodiscard]] auto hasAnyBinds() const noexcept -> bool;

	[[nodiscard]] auto getMousePositionX() const noexcept -> int;
	[[nodiscard]] auto getMousePositionY() const noexcept -> int;

	[[nodiscard]] auto isJoystickConnected() const noexcept -> bool;
	[[nodiscard]] auto getJoystickAxis(std::size_t axisIndex) const noexcept -> float;
	[[nodiscard]] auto getJoystickHatX(std::size_t hatIndex) const noexcept -> int;
	[[nodiscard]] auto getJoystickHatY(std::size_t hatIndex) const noexcept -> int;
	[[nodiscard]] auto getJoystickInfo() const -> std::string;

	[[nodiscard]] auto getActions() const noexcept -> Actions;
	[[nodiscard]] auto isActionPressed(Actions action) const noexcept -> bool;
	[[nodiscard]] auto wasActionPressed(Actions action) const noexcept -> bool;
	[[nodiscard]] auto wasActionReleased(Actions action) const noexcept -> bool;

	auto pressAction(Actions action) noexcept -> void;
	auto pressAction(std::string_view actionName) noexcept -> bool;
	auto releaseAction(Actions action) noexcept -> void;
	auto releaseAction(std::string_view actionName) noexcept -> bool;

	[[nodiscard]] auto getBind(std::string_view input) const noexcept -> std::string_view;

	[[nodiscard]] auto getBinds() const -> std::vector<BindView>;

	[[nodiscard]] auto isPressed(std::string_view input) const noexcept -> bool;
	[[nodiscard]] auto press(std::string_view input) -> Script;
	[[nodiscard]] auto release(std::string_view input) -> Script;
	[[nodiscard]] auto releaseAll() -> Script;

	[[nodiscard]] auto pressMouseLookUp() -> Script;
	[[nodiscard]] auto releaseMouseLookUp() -> Script;
	[[nodiscard]] auto pressMouseLookDown() -> Script;
	[[nodiscard]] auto releaseMouseLookDown() -> Script;
	[[nodiscard]] auto pressMouseLookLeft() -> Script;
	[[nodiscard]] auto releaseMouseLookLeft() -> Script;
	[[nodiscard]] auto pressMouseLookRight() -> Script;
	[[nodiscard]] auto releaseMouseLookRight() -> Script;

private:
	struct Output final {
		std::string script{};
		Actions actionsPressed = 0;
		Actions actionsReleased = 0;
		Script onPress{};
		Script onRelease{};
		bool pressed = false;
	};

	struct JoystickDeleter final {
		auto operator()(SDL_Joystick* p) const noexcept -> void;
	};
	using JoystickPtr = std::unique_ptr<SDL_Joystick, JoystickDeleter>;

	auto pressActionsOnly(std::string_view input) -> void;
	auto releaseActionsOnly(std::string_view input) -> void;

	auto press(Output& output) -> Script;
	auto release(Output& output) -> Script;

	std::unordered_set<std::string> m_inputStrings{};
	std::unordered_set<std::string> m_actionStrings{};
	std::unordered_map<std::string_view, Output> m_binds{};
	float m_mouseWheelAccumulatorHorizontal = 0.0f;
	float m_mousewheelAccumulatorVertical = 0.0f;
	int m_mousePositionX = 0;
	int m_mousePositionY = 0;
	JoystickPtr m_joystick{};
	Actions m_actions = Action::NONE;
	Actions m_previousActions = Action::NONE;
};

#endif
