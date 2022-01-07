#include "checkbox.hpp"

#include "../console/commands/gui_checkbox_commands.hpp" // gui_checkbox_...
#include "../game/client/char_window.hpp"                // CharWindow

#include <utility> // std::move

namespace gui {

Checkbox::Checkbox(Vec2 position, Vec2 size, Color color, bool value, Function function)
	: Element(position, size)
	, m_color(color)
	, m_value(value)
	, m_function(std::move(function)) {
	if (m_size.x >= 2 && m_size.y >= 1) {
		const auto w = static_cast<std::size_t>(m_size.x);
		const auto h = static_cast<std::size_t>(m_size.y);
		const auto r = static_cast<std::size_t>(w - 1);

		m_enabledMatrix.resize(w, h, gui_checkbox_enabled_char);
		m_enabledMatrix.drawLineVertical(0, 0, h, gui_checkbox_left_char);
		m_enabledMatrix.drawLineVertical(r, 0, h, gui_checkbox_right_char);

		m_disabledMatrix.resize(w, h, gui_checkbox_disabled_char);
		m_disabledMatrix.drawLineVertical(0, 0, h, gui_checkbox_left_char);
		m_disabledMatrix.drawLineVertical(r, 0, h, gui_checkbox_right_char);

		m_hoverEnabledMatrix.resize(w, h, gui_checkbox_hover_enabled_char);
		m_hoverEnabledMatrix.drawLineVertical(0, 0, h, gui_checkbox_left_char);
		m_hoverEnabledMatrix.drawLineVertical(r, 0, h, gui_checkbox_right_char);

		m_hoverDisabledMatrix.resize(w, h, gui_checkbox_hover_disabled_char);
		m_hoverDisabledMatrix.drawLineVertical(0, 0, h, gui_checkbox_left_char);
		m_hoverDisabledMatrix.drawLineVertical(r, 0, h, gui_checkbox_right_char);

		m_pressedMatrix.resize(w, h, gui_checkbox_press_char);
		m_pressedMatrix.drawLineVertical(0, 0, h, gui_checkbox_left_char);
		m_pressedMatrix.drawLineVertical(r, 0, h, gui_checkbox_right_char);
	}
}

auto Checkbox::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	switch (e.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (e.button.button == SDL_BUTTON_LEFT && this->getScreenRect(charWindow).contains(Vec2{e.button.x, e.button.y})) {
				m_state = State::PRESSED;
				m_value = !m_value;
				if (m_function) {
					m_function(*this);
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (e.button.button == SDL_BUTTON_LEFT) {
				if (this->getScreenRect(charWindow).contains(Vec2{e.button.x, e.button.y})) {
					if (m_state == State::PRESSED) {
						if (!m_activated) {
							this->activate();
						}
						m_state = State::HOVER;
					}
				} else if (m_activated) {
					this->deactivate();
				} else {
					m_state = State::NORMAL;
				}
			}
			break;
		case SDL_KEYDOWN:
			if (m_activated && (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE)) {
				m_state = State::PRESSED;
			}
			break;
		case SDL_KEYUP:
			if (m_activated && m_state == State::PRESSED && (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE)) {
				m_state = State::HOVER;
				m_value = !m_value;
				if (m_function) {
					m_function(*this);
				}
			}
			break;
		case SDL_MOUSEMOTION:
			if (m_state != State::PRESSED) {
				if (this->getScreenRect(charWindow).contains(Vec2{e.motion.x, e.motion.y})) {
					m_state = State::HOVER;
				} else {
					m_state = State::NORMAL;
				}
			}
			break;
		default: break;
	}
}

auto Checkbox::update(float) -> void {}

auto Checkbox::draw(CharWindow& charWindow) const -> void {
	switch (m_state) {
		case State::NORMAL:
			if (m_value) {
				charWindow.draw(m_position, m_enabledMatrix, m_color);
			} else {
				charWindow.draw(m_position, m_disabledMatrix, m_color);
			}
			break;
		case State::HOVER:
			if (m_value) {
				charWindow.draw(m_position, m_hoverEnabledMatrix, m_color);
			} else {
				charWindow.draw(m_position, m_hoverDisabledMatrix, m_color);
			}
			break;
		case State::PRESSED: charWindow.draw(m_position, m_pressedMatrix, m_color); break;
	}
}

auto Checkbox::setFunction(Function function) -> void {
	m_function = std::move(function);
}

auto Checkbox::setValue(bool value) -> void {
	m_value = value;
}

auto Checkbox::setState(State state) noexcept -> void {
	m_state = state;
}

auto Checkbox::setColor(Color color) noexcept -> void {
	m_color = color;
}

auto Checkbox::getValue() const noexcept -> bool {
	return m_value;
}

auto Checkbox::getState() const noexcept -> Checkbox::State {
	return m_state;
}

auto Checkbox::getColor() const noexcept -> Color {
	return m_color;
}

auto Checkbox::onActivate() -> void {
	m_state = State::HOVER;
}

auto Checkbox::onDeactivate() -> void {
	m_state = State::NORMAL;
}

} // namespace gui
