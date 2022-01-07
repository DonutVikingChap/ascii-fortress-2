#include "button.hpp"

#include "../console/commands/gui_button_commands.hpp" // gui_button_...
#include "../game/client/char_window.hpp"              // CharWindow
#include "../utilities/algorithm.hpp"                  // util::replace

#include <utility> // std::move

namespace gui {

Button::Button(Vec2 position, Vec2 size, Color color, std::string text, Function function)
	: Element(position, size)
	, m_color(color)
	, m_function(std::move(function)) {
	const auto w = static_cast<std::size_t>(m_size.x);
	const auto h = static_cast<std::size_t>(m_size.y);

	m_normalMatrix.resize(w, h);
	m_hoverMatrix.resize(w, h);
	m_pressedMatrix.resize(w, h);

	this->setText(std::move(text));
}

auto Button::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	switch (e.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (e.button.button == SDL_BUTTON_LEFT && this->getScreenRect(charWindow).contains(Vec2{e.button.x, e.button.y})) {
				m_state = State::PRESSED;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (e.button.button == SDL_BUTTON_LEFT) {
				if (this->getScreenRect(charWindow).contains(Vec2{e.button.x, e.button.y})) {
					if (m_state == State::PRESSED) {
						if (!m_activated) {
							this->activate();
						} else {
							m_state = State::HOVER;
						}

						if (m_function) {
							m_function(*this);
						}
					} else {
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
			if (m_activated && m_state == State::HOVER && (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE)) {
				m_state = State::PRESSED;
			}
			break;
		case SDL_KEYUP:
			if (m_activated && m_state == State::PRESSED && (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE)) {
				m_state = State::HOVER;
				if (m_function) {
					m_function(*this);
				}
			}
			break;
		case SDL_MOUSEMOTION:
			if (!m_activated) {
				if (this->getScreenRect(charWindow).contains(Vec2{e.motion.x, e.motion.y})) {
					if (m_state != State::PRESSED) {
						m_state = State::HOVER;
					}
				} else {
					m_state = State::NORMAL;
				}
			}
			break;
		default: break;
	}
}

auto Button::update(float) -> void {}

auto Button::draw(CharWindow& charWindow) const -> void {
	if (m_activated) {
		switch (m_state) {
			case State::NORMAL: [[fallthrough]];
			case State::HOVER: charWindow.draw(m_position, m_hoverMatrix, m_color); break;
			case State::PRESSED: charWindow.draw(m_position, m_pressedMatrix, m_color); break;
		}
	} else {
		switch (m_state) {
			case State::NORMAL: charWindow.draw(m_position, m_normalMatrix, m_color); break;
			case State::HOVER: charWindow.draw(m_position, m_hoverMatrix, m_color); break;
			case State::PRESSED: charWindow.draw(m_position, m_pressedMatrix, m_color); break;
		}
	}
}

auto Button::setFunction(Function function) -> void {
	m_function = std::move(function);
}

auto Button::setText(std::string text) -> void {
	m_text = std::move(text);

	const auto w = static_cast<std::size_t>(m_size.x);
	const auto h = static_cast<std::size_t>(m_size.y);
	const auto x = (m_text.size() >= w) ? std::size_t{0} : static_cast<std::size_t>((w - m_text.size()) / 2);
	const auto y = static_cast<std::size_t>(h / 2);

	auto tempText = m_text;
	util::replace(tempText, ' ', static_cast<char>(gui_button_fill_char));
	m_normalMatrix.fillRect(1, 1, w - 2, h - 2, gui_button_fill_char);
	m_normalMatrix.set(0, 0, gui_button_corner_char);
	m_normalMatrix.set(w - 1, 0, gui_button_corner_char);
	m_normalMatrix.set(0, h - 1, gui_button_corner_char);
	m_normalMatrix.set(w - 1, h - 1, gui_button_corner_char);
	m_normalMatrix.drawLineVertical(0, 1, h - 2, gui_button_edge_left_char);
	m_normalMatrix.drawLineVertical(w - 1, 1, h - 2, gui_button_edge_right_char);
	m_normalMatrix.drawLineHorizontal(1, 0, w - 2, gui_button_edge_top_char);
	m_normalMatrix.drawLineHorizontal(1, h - 1, w - 2, gui_button_edge_bottom_char);
	m_normalMatrix.draw(x, y, tempText.begin(), tempText.end());

	tempText = m_text;
	util::replace(tempText, ' ', static_cast<char>(gui_button_hover_char));
	m_hoverMatrix.fillRect(1, 1, w - 2, h - 2, gui_button_hover_char);
	m_hoverMatrix.set(0, 0, gui_button_corner_char);
	m_hoverMatrix.set(w - 1, 0, gui_button_corner_char);
	m_hoverMatrix.set(0, h - 1, gui_button_corner_char);
	m_hoverMatrix.set(w - 1, h - 1, gui_button_corner_char);
	m_hoverMatrix.drawLineVertical(0, 1, h - 2, gui_button_edge_left_char);
	m_hoverMatrix.drawLineVertical(w - 1, 1, h - 2, gui_button_edge_right_char);
	m_hoverMatrix.drawLineHorizontal(1, 0, w - 2, gui_button_edge_top_char);
	m_hoverMatrix.drawLineHorizontal(1, h - 1, w - 2, gui_button_edge_bottom_char);
	m_hoverMatrix.draw(x, y, tempText.begin(), tempText.end());

	tempText = m_text;
	util::replace(tempText, ' ', static_cast<char>(gui_button_press_char));
	m_pressedMatrix.fillRect(1, 1, w - 2, h - 2, gui_button_press_char);
	m_pressedMatrix.set(0, 0, gui_button_corner_char);
	m_pressedMatrix.set(w - 1, 0, gui_button_corner_char);
	m_pressedMatrix.set(0, h - 1, gui_button_corner_char);
	m_pressedMatrix.set(w - 1, h - 1, gui_button_corner_char);
	m_pressedMatrix.drawLineVertical(0, 1, h - 2, gui_button_edge_left_char);
	m_pressedMatrix.drawLineVertical(w - 1, 1, h - 2, gui_button_edge_right_char);
	m_pressedMatrix.drawLineHorizontal(1, 0, w - 2, gui_button_edge_top_char);
	m_pressedMatrix.drawLineHorizontal(1, h - 1, w - 2, gui_button_edge_bottom_char);
	m_pressedMatrix.draw(x, y, tempText.begin(), tempText.end());
}

auto Button::setState(State state) noexcept -> void {
	m_state = state;
}

auto Button::setColor(Color color) noexcept -> void {
	m_color = color;
}

auto Button::getText() const noexcept -> std::string_view {
	return m_text;
}

auto Button::getState() const noexcept -> Button::State {
	return m_state;
}

auto Button::getColor() const noexcept -> Color {
	return m_color;
}

auto Button::onActivate() -> void {
	m_state = State::HOVER;
}

auto Button::onDeactivate() -> void {
	m_state = State::NORMAL;
}

} // namespace gui
