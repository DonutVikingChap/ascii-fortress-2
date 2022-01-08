#include "input_manager.hpp"

#include "../../debug.hpp"               // Msg, INFO_MSG, DEBUG_MSG
#include "../../utilities/algorithm.hpp" // util::transform, util::append

#include <array>      // std::array
#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <fmt/core.h> // fmt::format
#include <utility>    // std::move

namespace {

const auto INPUT_KEYS = std::unordered_map<SDL_Scancode, std::string_view>{
	{SDL_SCANCODE_A, "a"},
	{SDL_SCANCODE_B, "b"},
	{SDL_SCANCODE_C, "c"},
	{SDL_SCANCODE_D, "d"},
	{SDL_SCANCODE_E, "e"},
	{SDL_SCANCODE_F, "f"},
	{SDL_SCANCODE_G, "g"},
	{SDL_SCANCODE_H, "h"},
	{SDL_SCANCODE_I, "i"},
	{SDL_SCANCODE_J, "j"},
	{SDL_SCANCODE_K, "k"},
	{SDL_SCANCODE_L, "l"},
	{SDL_SCANCODE_M, "m"},
	{SDL_SCANCODE_N, "n"},
	{SDL_SCANCODE_O, "o"},
	{SDL_SCANCODE_P, "p"},
	{SDL_SCANCODE_Q, "q"},
	{SDL_SCANCODE_R, "r"},
	{SDL_SCANCODE_S, "s"},
	{SDL_SCANCODE_T, "t"},
	{SDL_SCANCODE_U, "u"},
	{SDL_SCANCODE_V, "v"},
	{SDL_SCANCODE_W, "w"},
	{SDL_SCANCODE_X, "x"},
	{SDL_SCANCODE_Y, "y"},
	{SDL_SCANCODE_Z, "z"},
	{SDL_SCANCODE_0, "0"},
	{SDL_SCANCODE_1, "1"},
	{SDL_SCANCODE_2, "2"},
	{SDL_SCANCODE_3, "3"},
	{SDL_SCANCODE_4, "4"},
	{SDL_SCANCODE_5, "5"},
	{SDL_SCANCODE_6, "6"},
	{SDL_SCANCODE_7, "7"},
	{SDL_SCANCODE_8, "8"},
	{SDL_SCANCODE_9, "9"},
	{SDL_SCANCODE_ESCAPE, "esc"},
	{SDL_SCANCODE_LCTRL, "lctrl"},
	{SDL_SCANCODE_LSHIFT, "lshift"},
	{SDL_SCANCODE_LALT, "lalt"},
	{SDL_SCANCODE_RCTRL, "rctrl"},
	{SDL_SCANCODE_RSHIFT, "rshift"},
	{SDL_SCANCODE_RALT, "ralt"},
	{SDL_SCANCODE_MENU, "menu"},
	{SDL_SCANCODE_LEFTBRACKET, "["},
	{SDL_SCANCODE_RIGHTBRACKET, "]"},
	{SDL_SCANCODE_SEMICOLON, ";"},
	{SDL_SCANCODE_COMMA, ","},
	{SDL_SCANCODE_PERIOD, "."},
	{SDL_SCANCODE_APOSTROPHE, "'"},
	{SDL_SCANCODE_SLASH, "/"},
	{SDL_SCANCODE_BACKSLASH, "\\"},
	{SDL_SCANCODE_GRAVE, "~"},
	{SDL_SCANCODE_EQUALS, "="},
	{SDL_SCANCODE_MINUS, "-"},
	{SDL_SCANCODE_SPACE, "space"},
	{SDL_SCANCODE_RETURN, "return"},
	{SDL_SCANCODE_BACKSPACE, "backspace"},
	{SDL_SCANCODE_TAB, "tab"},
	{SDL_SCANCODE_PAGEUP, "pgup"},
	{SDL_SCANCODE_PAGEDOWN, "pgdn"},
	{SDL_SCANCODE_END, "end"},
	{SDL_SCANCODE_HOME, "home"},
	{SDL_SCANCODE_INSERT, "insert"},
	{SDL_SCANCODE_DELETE, "delete"},
	{SDL_SCANCODE_LEFT, "leftarrow"},
	{SDL_SCANCODE_RIGHT, "rightarrow"},
	{SDL_SCANCODE_UP, "uparrow"},
	{SDL_SCANCODE_DOWN, "downarrow"},
	{SDL_SCANCODE_KP_PLUS, "kp_plus"},
	{SDL_SCANCODE_KP_MINUS, "kp_minus"},
	{SDL_SCANCODE_KP_MULTIPLY, "kp_multiply"},
	{SDL_SCANCODE_KP_DIVIDE, "kp_divide"},
	{SDL_SCANCODE_KP_0, "kp_0"},
	{SDL_SCANCODE_KP_1, "kp_1"},
	{SDL_SCANCODE_KP_2, "kp_2"},
	{SDL_SCANCODE_KP_3, "kp_3"},
	{SDL_SCANCODE_KP_4, "kp_4"},
	{SDL_SCANCODE_KP_5, "kp_5"},
	{SDL_SCANCODE_KP_6, "kp_6"},
	{SDL_SCANCODE_KP_7, "kp_7"},
	{SDL_SCANCODE_KP_8, "kp_8"},
	{SDL_SCANCODE_KP_9, "kp_9"},
	{SDL_SCANCODE_F1, "f1"},
	{SDL_SCANCODE_F2, "f2"},
	{SDL_SCANCODE_F3, "f3"},
	{SDL_SCANCODE_F4, "f4"},
	{SDL_SCANCODE_F5, "f5"},
	{SDL_SCANCODE_F6, "f6"},
	{SDL_SCANCODE_F7, "f7"},
	{SDL_SCANCODE_F8, "f8"},
	{SDL_SCANCODE_F9, "f9"},
	{SDL_SCANCODE_F10, "f10"},
	{SDL_SCANCODE_F11, "f11"},
	{SDL_SCANCODE_F12, "f12"},
	{SDL_SCANCODE_F13, "f13"},
	{SDL_SCANCODE_F14, "f14"},
	{SDL_SCANCODE_F15, "f15"},
	{SDL_SCANCODE_PAUSE, "pause"},
};

constexpr auto MAX_JOYSTICK_BUTTON_COUNT = std::size_t{32};
constexpr auto MAX_JOYSTICK_AXIS_COUNT = std::size_t{16};
constexpr auto MAX_JOYSTICK_HAT_COUNT = std::size_t{8};

const auto INPUT_JOYSTICK_BUTTONS = [] {
	auto joystickButtonTable = std::array<std::string, MAX_JOYSTICK_BUTTON_COUNT>{};
	for (auto i = std::size_t{0}; i < MAX_JOYSTICK_BUTTON_COUNT; ++i) {
		joystickButtonTable[i] = fmt::format("joy_button_{}", i);
	}
	return joystickButtonTable;
}();

const auto INPUT_JOYSTICK_AXES = [] {
	auto joystickAxisTable = std::array<std::string, MAX_JOYSTICK_AXIS_COUNT>{};
	for (auto i = std::size_t{0}; i < MAX_JOYSTICK_AXIS_COUNT; ++i) {
		joystickAxisTable[i] = fmt::format("joy_axis_{}", i);
	}
	return joystickAxisTable;
}();

const auto INPUT_JOYSTICK_HATS = [] {
	auto joystickHatTable = std::array<std::string, MAX_JOYSTICK_HAT_COUNT>{};
	for (auto i = std::size_t{0}; i < MAX_JOYSTICK_HAT_COUNT; ++i) {
		joystickHatTable[i] = fmt::format("joy_hat_{}", i);
	}
	return joystickHatTable;
}();

constexpr auto INPUT_MOUSE_CLICK_LEFT = std::string_view{"mouse1"};
constexpr auto INPUT_MOUSE_CLICK_RIGHT = std::string_view{"mouse2"};
constexpr auto INPUT_MOUSE_CLICK_MIDDLE = std::string_view{"mouse3"};
constexpr auto INPUT_MOUSE_CLICK_BACK = std::string_view{"mouse4"};
constexpr auto INPUT_MOUSE_CLICK_FORWARD = std::string_view{"mouse5"};
constexpr auto INPUT_MOUSE_SCROLL_UP = std::string_view{"mwheelup"};
constexpr auto INPUT_MOUSE_SCROLL_DOWN = std::string_view{"mwheeldown"};
constexpr auto INPUT_MOUSE_SCROLL_LEFT = std::string_view{"mwheelleft"};
constexpr auto INPUT_MOUSE_SCROLL_RIGHT = std::string_view{"mwheelright"};
constexpr auto INPUT_MOUSE_LOOK_UP = std::string_view{"mlookup"};
constexpr auto INPUT_MOUSE_LOOK_DOWN = std::string_view{"mlookdown"};
constexpr auto INPUT_MOUSE_LOOK_LEFT = std::string_view{"mlookleft"};
constexpr auto INPUT_MOUSE_LOOK_RIGHT = std::string_view{"mlookright"};

} // namespace

auto InputManager::actionMap() -> const std::unordered_map<std::string_view, Actions>& {
	static const auto map = std::unordered_map<std::string_view, Actions>{
		{"left", Action::MOVE_LEFT},
		{"right", Action::MOVE_RIGHT},
		{"up", Action::MOVE_UP},
		{"down", Action::MOVE_DOWN},
		{"aimleft", Action::AIM_LEFT},
		{"aimright", Action::AIM_RIGHT},
		{"aimup", Action::AIM_UP},
		{"aimdown", Action::AIM_DOWN},
		{"attack1", Action::ATTACK1},
		{"attack2", Action::ATTACK2},
	};
	return map;
}

auto InputManager::validInputs() -> const std::vector<std::string>& {
	static const auto validInputs = [] {
		auto validInputs = std::vector<std::string>{};
		validInputs.reserve(INPUT_KEYS.size() + INPUT_JOYSTICK_BUTTONS.size() + INPUT_JOYSTICK_AXES.size() + INPUT_JOYSTICK_HATS.size() + 13);
		util::append(validInputs, INPUT_KEYS | util::transform([](const auto& kv) { return kv.second; }));
		util::append(validInputs, INPUT_JOYSTICK_BUTTONS);
		util::append(validInputs, INPUT_JOYSTICK_AXES);
		util::append(validInputs, INPUT_JOYSTICK_HATS);
		validInputs.emplace_back(INPUT_MOUSE_CLICK_LEFT);
		validInputs.emplace_back(INPUT_MOUSE_CLICK_RIGHT);
		validInputs.emplace_back(INPUT_MOUSE_CLICK_MIDDLE);
		validInputs.emplace_back(INPUT_MOUSE_CLICK_BACK);
		validInputs.emplace_back(INPUT_MOUSE_CLICK_FORWARD);
		validInputs.emplace_back(INPUT_MOUSE_SCROLL_UP);
		validInputs.emplace_back(INPUT_MOUSE_SCROLL_DOWN);
		validInputs.emplace_back(INPUT_MOUSE_SCROLL_LEFT);
		validInputs.emplace_back(INPUT_MOUSE_SCROLL_RIGHT);
		validInputs.emplace_back(INPUT_MOUSE_LOOK_UP);
		validInputs.emplace_back(INPUT_MOUSE_LOOK_DOWN);
		validInputs.emplace_back(INPUT_MOUSE_LOOK_LEFT);
		validInputs.emplace_back(INPUT_MOUSE_LOOK_RIGHT);
		return validInputs;
	}();
	return validInputs;
}

auto InputManager::isValidInput(std::string_view input) -> bool {
	static const auto validInputSet = [] {
		auto validInputSet = std::unordered_set<std::string_view>{};
		for (const auto& input : InputManager::validInputs()) {
			validInputSet.emplace(input);
		}
		return validInputSet;
	}();
	return validInputSet.count(input) != 0;
}

auto InputManager::update() -> void {
	m_previousActions = m_actions;
}

auto InputManager::handleEvent(const SDL_Event& e, bool active) -> Script {
	switch (e.type) {
		case SDL_WINDOWEVENT:
			if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				return this->releaseAll();
			}
			break;
		case SDL_KEYDOWN:
			if (active) {
				if (const auto it = INPUT_KEYS.find(e.key.keysym.scancode); it != INPUT_KEYS.end()) {
					return this->press(it->second);
				}
				DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown key scancode \"{}\" pressed.", e.key.keysym.scancode);
			}
			break;
		case SDL_KEYUP:
			if (active) {
				if (const auto it = INPUT_KEYS.find(e.key.keysym.scancode); it != INPUT_KEYS.end()) {
					return this->release(it->second);
				}
				DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown key scancode \"{}\" released.", e.key.keysym.scancode);
			} else {
				if (const auto it = INPUT_KEYS.find(e.key.keysym.scancode); it != INPUT_KEYS.end()) {
					this->releaseActionsOnly(it->second);
				} else {
					DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown key scancode \"{}\" released.", e.key.keysym.scancode);
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (active) {
				switch (e.button.button) {
					case SDL_BUTTON_LEFT: return this->press(INPUT_MOUSE_CLICK_LEFT);
					case SDL_BUTTON_RIGHT: return this->press(INPUT_MOUSE_CLICK_RIGHT);
					case SDL_BUTTON_MIDDLE: return this->press(INPUT_MOUSE_CLICK_MIDDLE);
					case SDL_BUTTON_X1: return this->press(INPUT_MOUSE_CLICK_BACK);
					case SDL_BUTTON_X2: return this->press(INPUT_MOUSE_CLICK_FORWARD);
					default: DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown mouse button \"{}\" pressed.", e.button.button); break;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (active) {
				switch (e.button.button) {
					case SDL_BUTTON_LEFT: return this->release(INPUT_MOUSE_CLICK_LEFT);
					case SDL_BUTTON_RIGHT: return this->release(INPUT_MOUSE_CLICK_RIGHT);
					case SDL_BUTTON_MIDDLE: return this->release(INPUT_MOUSE_CLICK_MIDDLE);
					case SDL_BUTTON_X1: return this->release(INPUT_MOUSE_CLICK_BACK);
					case SDL_BUTTON_X2: return this->release(INPUT_MOUSE_CLICK_FORWARD);
					default: DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown mouse button \"{}\" released.", e.button.button); break;
				}
			} else {
				switch (e.button.button) {
					case SDL_BUTTON_LEFT: this->releaseActionsOnly(INPUT_MOUSE_CLICK_LEFT); break;
					case SDL_BUTTON_RIGHT: this->releaseActionsOnly(INPUT_MOUSE_CLICK_RIGHT); break;
					case SDL_BUTTON_MIDDLE: this->releaseActionsOnly(INPUT_MOUSE_CLICK_MIDDLE); break;
					case SDL_BUTTON_X1: this->releaseActionsOnly(INPUT_MOUSE_CLICK_BACK); break;
					case SDL_BUTTON_X2: this->releaseActionsOnly(INPUT_MOUSE_CLICK_FORWARD); break;
					default: DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown mouse button \"{}\" released.", e.button.button); break;
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			if (active) {
				auto commands = Script{};
				m_mouseWheelAccumulatorHorizontal += e.wheel.preciseX;
				while (m_mouseWheelAccumulatorHorizontal <= -1.0f) {
					util::append(commands, this->press(INPUT_MOUSE_SCROLL_RIGHT));
					util::append(commands, this->release(INPUT_MOUSE_SCROLL_RIGHT));
					m_mouseWheelAccumulatorHorizontal += 1.0f;
				}
				while (m_mouseWheelAccumulatorHorizontal >= 1.0f) {
					util::append(commands, this->press(INPUT_MOUSE_SCROLL_LEFT));
					util::append(commands, this->release(INPUT_MOUSE_SCROLL_LEFT));
					m_mouseWheelAccumulatorHorizontal -= 1.0f;
				}
				m_mousewheelAccumulatorVertical += e.wheel.preciseY;
				while (m_mousewheelAccumulatorVertical <= -1.0f) {
					util::append(commands, this->press(INPUT_MOUSE_SCROLL_DOWN));
					util::append(commands, this->release(INPUT_MOUSE_SCROLL_DOWN));
					m_mousewheelAccumulatorVertical += 1.0f;
				}
				while (m_mousewheelAccumulatorVertical >= 1.0f) {
					util::append(commands, this->press(INPUT_MOUSE_SCROLL_UP));
					util::append(commands, this->release(INPUT_MOUSE_SCROLL_UP));
					m_mousewheelAccumulatorVertical -= 1.0f;
				}
				return commands;
			}
			break;
		case SDL_JOYBUTTONDOWN:
			if (m_joystick && e.jbutton.which == SDL_JoystickInstanceID(m_joystick.get())) {
				if (const auto button = static_cast<std::size_t>(e.jbutton.button); button < MAX_JOYSTICK_BUTTON_COUNT) {
					if (active) {
						return this->press(INPUT_JOYSTICK_BUTTONS[button]);
					}
				} else {
					DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown joystick button \"{}\" pressed.", e.jbutton.button);
				}
			} else {
				DEBUG_MSG(Msg::INPUT_MANAGER, "Button \"{}\" pressed on an inactive joystick.", e.jbutton.button);
			}
			break;
		case SDL_JOYBUTTONUP:
			if (m_joystick && e.jbutton.which == SDL_JoystickInstanceID(m_joystick.get())) {
				if (const auto button = static_cast<std::size_t>(e.jbutton.button); button < MAX_JOYSTICK_BUTTON_COUNT) {
					if (active) {
						return this->release(INPUT_JOYSTICK_BUTTONS[button]);
					}
					this->releaseActionsOnly(INPUT_JOYSTICK_BUTTONS[button]);
				} else {
					DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown joystick button \"{}\" released.", e.jbutton.button);
				}
			} else {
				DEBUG_MSG(Msg::INPUT_MANAGER, "Button \"{}\" released on an inactive joystick.", e.jbutton.button);
			}
			break;
		case SDL_JOYAXISMOTION:
			if (m_joystick && e.jaxis.which == SDL_JoystickInstanceID(m_joystick.get())) {
				if (const auto axis = static_cast<std::size_t>(e.jaxis.axis); axis < MAX_JOYSTICK_AXIS_COUNT) {
					if (active) {
						const auto& input = INPUT_JOYSTICK_AXES[axis];
						auto commands = this->press(input);
						util::append(commands, this->release(input));
						return commands;
					}
				} else {
					DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown joystick axis \"{}\" moved.", e.jaxis.axis);
				}
			} else {
				DEBUG_MSG(Msg::INPUT_MANAGER, "Axis \"{}\" moved on an inactive joystick.", e.jaxis.axis);
			}
			break;
		case SDL_JOYHATMOTION:
			if (m_joystick && e.jhat.which == SDL_JoystickInstanceID(m_joystick.get())) {
				if (const auto hat = static_cast<std::size_t>(e.jhat.hat); hat < MAX_JOYSTICK_HAT_COUNT) {
					if (active) {
						const auto& input = INPUT_JOYSTICK_HATS[hat];
						auto commands = this->press(input);
						util::append(commands, this->release(input));
						return commands;
					}
				} else {
					DEBUG_MSG(Msg::INPUT_MANAGER, "Unknown joystick hat \"{}\" moved.", e.jhat.hat);
				}
			} else {
				DEBUG_MSG(Msg::INPUT_MANAGER, "Hat \"{}\" moved on an inactive joystick.", e.jhat.hat);
			}
			break;
		case SDL_JOYDEVICEADDED:
			if (m_joystick) {
				m_joystick.reset();
				INFO_MSG(Msg::INPUT_MANAGER, "Joystick disconnected.");
				m_joystick = JoystickPtr{SDL_JoystickOpen(e.jdevice.which)};
				INFO_MSG(Msg::INPUT_MANAGER, "Joystick connected.");
				return this->releaseAll();
			}
			m_joystick = JoystickPtr{SDL_JoystickOpen(e.jdevice.which)};
			INFO_MSG(Msg::INPUT_MANAGER, "Joystick connected.");
			break;
		case SDL_JOYDEVICEREMOVED:
			if (m_joystick && e.jdevice.which == SDL_JoystickInstanceID(m_joystick.get())) {
				m_joystick.reset();
				INFO_MSG(Msg::INPUT_MANAGER, "Joystick disconnected.");
				return this->releaseAll();
			}
			DEBUG_MSG(Msg::INPUT_MANAGER, "Disconnected an inactive joystick.", e.jhat.hat);
			break;
		default: break;
	}
	return Script{};
}

auto InputManager::bind(std::string input, std::string output) -> bool {
	if (!InputManager::isValidInput(input)) {
		return false;
	}

	if (output.empty()) {
		this->unbind(input);
	} else {
		auto commands = Script::parse(output);
		if (commands.empty()) {
			this->unbind(input);
		} else {
			auto out = Output{};
			out.script = std::move(output);
			out.onPress = std::move(commands);
			for (const auto& command : out.onPress) {
				assert(!command.empty());
				if (const auto& name = command.front().value; !name.empty()) {
					if (name.front() == '+') {
						if (const auto actionIt = InputManager::actionMap().find(std::string_view{name}.substr(1));
						    actionIt != InputManager::actionMap().end()) {
							out.actionsPressed |= actionIt->second;
						}
						out.onRelease.push_back(command);
						out.onRelease.back().front().value.front() = '-';
					} else if (name.front() == '-') {
						if (const auto actionIt = InputManager::actionMap().find(std::string_view{name}.substr(1));
						    actionIt != InputManager::actionMap().end()) {
							out.actionsReleased |= actionIt->second;
						}
						out.onRelease.push_back(command);
						out.onRelease.back().front().value.front() = '+';
					}
				}
			}
			m_binds[*m_inputStrings.emplace(std::move(input)).first] = std::move(out);
		}
	}
	return true;
}

auto InputManager::unbind(std::string_view input) -> bool {
	if (const auto it = m_binds.find(input); it != m_binds.end()) {
		m_binds.erase(it);
		m_inputStrings.erase(std::string{input});
		return true;
	}
	return false;
}

auto InputManager::unbindAll() noexcept -> void {
	m_binds.clear();
	m_inputStrings.clear();
}

auto InputManager::hasAnyBinds() const noexcept -> bool {
	return !m_binds.empty();
}

auto InputManager::getMousePositionX() const noexcept -> int {
	return m_mousePositionX;
}

auto InputManager::getMousePositionY() const noexcept -> int {
	return m_mousePositionY;
}

auto InputManager::isJoystickConnected() const noexcept -> bool {
	if (!m_joystick) {
		return false;
	}
	return SDL_JoystickGetAttached(m_joystick.get());
}

auto InputManager::getJoystickAxis(std::size_t axisIndex) const noexcept -> float {
	if (!m_joystick) {
		return 0.0f;
	}
	const auto axis = static_cast<int>(axisIndex);
	if (axis >= SDL_JoystickNumAxes(m_joystick.get())) {
		return 0.0f;
	}
	const auto value = SDL_JoystickGetAxis(m_joystick.get(), axis);
	return (static_cast<float>(value) + 0.5f) / 32767.5f;
}

auto InputManager::getJoystickHatX(std::size_t hatIndex) const noexcept -> int {
	if (!m_joystick) {
		return 0;
	}
	const auto hat = static_cast<int>(hatIndex);
	if (hat >= SDL_JoystickNumHats(m_joystick.get())) {
		return 0;
	}
	switch (SDL_JoystickGetHat(m_joystick.get(), hat)) {
		case SDL_HAT_CENTERED: [[fallthrough]];
		case SDL_HAT_UP: [[fallthrough]];
		case SDL_HAT_DOWN: return 0;
		case SDL_HAT_LEFT: [[fallthrough]];
		case SDL_HAT_LEFTUP: [[fallthrough]];
		case SDL_HAT_LEFTDOWN: return -1;
		case SDL_HAT_RIGHT: [[fallthrough]];
		case SDL_HAT_RIGHTUP: [[fallthrough]];
		case SDL_HAT_RIGHTDOWN: return 1;
		default: break;
	}
	return 0;
}

auto InputManager::getJoystickHatY(std::size_t hatIndex) const noexcept -> int {
	if (!m_joystick) {
		return 0;
	}
	const auto hat = static_cast<int>(hatIndex);
	if (hat >= SDL_JoystickNumHats(m_joystick.get())) {
		return 0;
	}
	switch (SDL_JoystickGetHat(m_joystick.get(), hat)) {
		case SDL_HAT_CENTERED: [[fallthrough]];
		case SDL_HAT_LEFT: [[fallthrough]];
		case SDL_HAT_RIGHT: return 0;
		case SDL_HAT_UP: [[fallthrough]];
		case SDL_HAT_LEFTUP: [[fallthrough]];
		case SDL_HAT_RIGHTUP: return -1;
		case SDL_HAT_DOWN: [[fallthrough]];
		case SDL_HAT_LEFTDOWN: [[fallthrough]];
		case SDL_HAT_RIGHTDOWN: return 1;
		default: break;
	}
	return 0;
}

auto InputManager::getJoystickInfo() const -> std::string {
	if (!m_joystick) {
		return "Unavailable";
	}
	auto guid = std::string(33, '\0');
	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(m_joystick.get()), guid.data(), static_cast<int>(guid.size()));
	const auto* power = "Unknown";
	switch (SDL_JoystickCurrentPowerLevel(m_joystick.get())) {
		case SDL_JOYSTICK_POWER_EMPTY: power = "Empty"; break;
		case SDL_JOYSTICK_POWER_LOW: power = "Low"; break;
		case SDL_JOYSTICK_POWER_MEDIUM: power = "Medium"; break;
		case SDL_JOYSTICK_POWER_FULL: power = "Full"; break;
		case SDL_JOYSTICK_POWER_WIRED: power = "Wired"; break;
		case SDL_JOYSTICK_POWER_MAX: power = "Max"; break;
		default: break;
	}
	return fmt::format(
		"GUID: {}\n"
		"Name: {}\n"
		"Power: {}\n"
		"Number of buttons: {}\n"
		"Number of axes: {}\n"
		"Number of hats: {}\n",
		guid.c_str(),
		SDL_JoystickName(m_joystick.get()),
		power,
		SDL_JoystickNumButtons(m_joystick.get()),
		SDL_JoystickNumAxes(m_joystick.get()),
		SDL_JoystickNumHats(m_joystick.get()));
}

auto InputManager::getActions() const noexcept -> Actions {
	return m_actions;
}

auto InputManager::isActionPressed(Actions action) const noexcept -> bool {
	return (m_actions & action) != 0;
}

auto InputManager::wasActionPressed(Actions action) const noexcept -> bool {
	return (m_actions & action) != 0 && (m_previousActions & action) == 0;
}

auto InputManager::wasActionReleased(Actions action) const noexcept -> bool {
	return (m_previousActions & action) != 0 && (m_actions & action) == 0;
}

auto InputManager::pressAction(Actions action) noexcept -> void {
	m_actions |= action;
}

auto InputManager::pressAction(std::string_view actionName) noexcept -> bool {
	if (const auto actionIt = InputManager::actionMap().find(actionName); actionIt != InputManager::actionMap().end()) {
		this->pressAction(actionIt->second);
		return true;
	}
	return false;
}

auto InputManager::releaseAction(Actions action) noexcept -> void {
	m_actions &= ~action;
}

auto InputManager::releaseAction(std::string_view actionName) noexcept -> bool {
	if (const auto actionIt = InputManager::actionMap().find(actionName); actionIt != InputManager::actionMap().end()) {
		this->releaseAction(actionIt->second);
		return true;
	}
	return false;
}

auto InputManager::getBind(std::string_view input) const noexcept -> std::string_view {
	const auto it = m_binds.find(input);
	return (it == m_binds.end()) ? std::string_view{} : it->second.script;
}

auto InputManager::getBinds() const -> std::vector<InputManager::BindView> {
	std::vector<BindView> binds;
	for (const auto& [input, output] : m_binds) {
		binds.push_back(BindView{input, output.script});
	}
	return binds;
}

auto InputManager::isPressed(std::string_view input) const noexcept -> bool {
	if (const auto it = m_binds.find(input); it != m_binds.end()) {
		return it->second.pressed;
	}
	return false;
}

auto InputManager::press(std::string_view input) -> Script {
	if (const auto it = m_binds.find(input); it != m_binds.end()) {
		return this->press(it->second);
	}
	return Script{};
}

auto InputManager::release(std::string_view input) -> Script {
	if (const auto it = m_binds.find(input); it != m_binds.end()) {
		return this->release(it->second);
	}
	return Script{};
}

auto InputManager::releaseAll() -> Script {
	auto commands = Script{};
	for (auto&& [input, output] : m_binds) {
		util::append(commands, this->release(output));
	}
	return commands;
}

auto InputManager::pressMouseLookUp() -> Script {
	return this->press(INPUT_MOUSE_LOOK_UP);
}

auto InputManager::releaseMouseLookUp() -> Script {
	return this->release(INPUT_MOUSE_LOOK_UP);
}

auto InputManager::pressMouseLookDown() -> Script {
	return this->press(INPUT_MOUSE_LOOK_DOWN);
}

auto InputManager::releaseMouseLookDown() -> Script {
	return this->release(INPUT_MOUSE_LOOK_DOWN);
}

auto InputManager::pressMouseLookLeft() -> Script {
	return this->press(INPUT_MOUSE_LOOK_LEFT);
}

auto InputManager::releaseMouseLookLeft() -> Script {
	return this->release(INPUT_MOUSE_LOOK_LEFT);
}

auto InputManager::pressMouseLookRight() -> Script {
	return this->press(INPUT_MOUSE_LOOK_RIGHT);
}

auto InputManager::releaseMouseLookRight() -> Script {
	return this->release(INPUT_MOUSE_LOOK_RIGHT);
}

auto InputManager::JoystickDeleter::operator()(SDL_Joystick* p) const noexcept -> void {
	SDL_JoystickClose(p);
}

auto InputManager::pressActionsOnly(std::string_view input) -> void {
	if (const auto it = m_binds.find(input); it != m_binds.end()) {
		if (!it->second.pressed) {
			it->second.pressed = true;
			this->pressAction(it->second.actionsPressed);
			this->releaseAction(it->second.actionsReleased);
		}
	}
}

auto InputManager::releaseActionsOnly(std::string_view input) -> void {
	if (const auto it = m_binds.find(input); it != m_binds.end()) {
		if (it->second.pressed) {
			it->second.pressed = false;
			this->releaseAction(it->second.actionsPressed);
			this->pressAction(it->second.actionsReleased);
		}
	}
}

auto InputManager::press(Output& output) -> Script {
	if (!output.pressed) {
		output.pressed = true;
		this->pressAction(output.actionsPressed);
		this->releaseAction(output.actionsReleased);
		if (!output.onPress.empty()) {
			return output.onPress;
		}
	}
	return Script{};
}

auto InputManager::release(Output& output) -> Script {
	if (output.pressed) {
		output.pressed = false;
		this->releaseAction(output.actionsPressed);
		this->pressAction(output.actionsReleased);
		if (!output.onRelease.empty()) {
			return output.onRelease;
		}
	}
	return Script{};
}
