#include "slider.hpp"

#include "../console/commands/gui_slider_commands.hpp" // gui_slider_...
#include "../game/client/char_window.hpp"              // CharWindow
#include "../utilities/math.hpp"                       // util::nearestMultiple

#include <cmath>       // std::round
#include <type_traits> // std::is_integral_v
#include <utility>     // std::move

namespace gui {

Slider::Slider(Vec2 position, Vec2 size, Color color, float value, float delta, Function function)
	: Element(position, size)
	, m_color(color)
	, m_slideHoverChar(gui_slider_slide_hover_char)
	, m_slideChar(gui_slider_slide_char)
	, m_delta(delta)
	, m_function(std::move(function)) {
	if (m_size.x >= 2 && m_size.y >= 1) {
		const auto w = static_cast<std::size_t>(m_size.x);
		const auto h = static_cast<std::size_t>(m_size.y);
		const auto r = static_cast<std::size_t>(w - 1);

		m_normalMatrix.resize(w, h, gui_slider_fill_char);
		m_normalMatrix.drawLineVertical(0, 0, h, gui_slider_left_char);
		m_normalMatrix.drawLineVertical(r, 0, h, gui_slider_right_char);

		m_hoverMatrix.resize(w, h, gui_slider_hover_char);
		m_hoverMatrix.drawLineVertical(0, 0, h, gui_slider_left_char);
		m_hoverMatrix.drawLineVertical(r, 0, h, gui_slider_right_char);
	}
	this->setValue(value);
}

auto Slider::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	switch (e.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (e.button.button == SDL_BUTTON_LEFT) {
				const auto& rect = this->getActiveRect(charWindow);
				const auto mousePosition = Vec2{e.button.x, e.button.y};
				if (rect.contains(mousePosition)) {
					m_state = State::PRESSED;
					this->updateHoverValue(charWindow, mousePosition);
					this->setValueToHoverValue();
					if (m_function) {
						m_function(*this);
					}
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (e.button.button == SDL_BUTTON_LEFT) {
				const auto& rect = this->getActiveRect(charWindow);
				const auto mousePosition = Vec2{e.button.x, e.button.y};
				if (rect.contains(mousePosition)) {
					if (m_state == State::PRESSED) {
						this->updateHoverValue(charWindow, mousePosition);
						if (!m_activated) {
							this->activate();
						}
					}
					m_state = State::HOVER;
				} else if (m_activated) {
					this->deactivate();
				} else {
					m_state = State::NORMAL;
				}
			}
			break;
		case SDL_KEYDOWN:
			if (m_activated) {
				if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
					m_state = State::PRESSED;
					this->setValueToHoverValue();
					if (m_function) {
						m_function(*this);
					}
				} else if (e.key.keysym.sym == SDLK_LEFT) {
					this->slideHoverValueLeft();
					this->setValueToHoverValue();
					if (m_function) {
						m_function(*this);
					}
				} else if (e.key.keysym.sym == SDLK_RIGHT) {
					this->slideHoverValueRight();
					this->setValueToHoverValue();
					if (m_function) {
						m_function(*this);
					}
				}
			}
			break;
		case SDL_KEYUP:
			if (m_activated && m_state == State::PRESSED) {
				if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE) {
					m_state = State::HOVER;
				}
			}
			break;
		case SDL_MOUSEMOTION: {
			const auto mousePosition = Vec2{e.motion.x, e.motion.y};
			if (m_state == State::PRESSED) {
				this->updateHoverValue(charWindow, mousePosition);
				this->setValueToHoverValue();
				if (m_function) {
					m_function(*this);
				}
			} else {
				const auto& rect = this->getActiveRect(charWindow);
				if (rect.contains(mousePosition)) {
					m_state = State::HOVER;
					this->updateHoverValue(charWindow, mousePosition);
				} else {
					m_state = State::NORMAL;
					this->setHoverValueToValue();
				}
			}
			break;
		}
		default: break;
	}
}

auto Slider::update(float) -> void {}

auto Slider::draw(CharWindow& charWindow) const -> void {
	charWindow.draw(m_position, (m_state == State::NORMAL) ? m_normalMatrix : m_hoverMatrix, m_color);
	charWindow.draw(Vec2{m_position.x + m_valueOffset, m_position.y}, m_slideChar, m_color);
	if (m_state == State::HOVER) {
		charWindow.draw(Vec2{m_position.x + m_hoverOffset, m_position.y}, m_slideHoverChar, m_color);
	}
}

auto Slider::setFunction(Function function) -> void {
	m_function = std::move(function);
}

auto Slider::setState(State state) noexcept -> void {
	m_state = state;
}

auto Slider::setColor(Color color) noexcept -> void {
	m_color = color;
}

auto Slider::setValue(float value) noexcept -> void {
	m_value = util::nearestMultiple(value, m_delta);
	if (m_value <= 0.0f) {
		m_value = 0.0f;
	} else if (m_value >= 1.0f) {
		m_value = 1.0f;
	}
	m_valueOffset = static_cast<Vec2::Length>(static_cast<int>(static_cast<float>(m_size.x - 3) * m_value) + 1);
	m_hoverValue = m_value;
	m_hoverOffset = m_valueOffset;
}

auto Slider::setHoverValue(float value) noexcept -> void {
	m_hoverValue = util::nearestMultiple(value, m_delta);
	if (m_hoverValue <= 0.0f) {
		m_hoverValue = 0.0f;
	} else if (m_hoverValue >= 1.0f) {
		m_hoverValue = 1.0f;
	}
	m_hoverOffset = static_cast<Vec2::Length>(static_cast<int>(static_cast<float>(m_size.x - 3) * m_hoverValue) + 1);
}

auto Slider::setValueToHoverValue() noexcept -> void {
	m_value = m_hoverValue;
	m_valueOffset = m_hoverOffset;
}

auto Slider::setHoverValueToValue() noexcept -> void {
	m_hoverValue = m_value;
	m_hoverOffset = m_valueOffset;
}

auto Slider::getState() const noexcept -> Slider::State {
	return m_state;
}

auto Slider::getColor() const noexcept -> Color {
	return m_color;
}

auto Slider::getValue() const noexcept -> float {
	return m_value;
}

auto Slider::getActiveRect(const CharWindow& charWindow) const -> Rect {
	return Rect{charWindow.gridToScreenCoordinates(Vec2{m_position.x + 1, m_position.y}),
	            charWindow.gridToScreenSize(Vec2{m_size.x - 2, m_size.y})};
}

auto Slider::slideHoverValueLeft() -> void {
	this->setHoverValue(m_hoverValue - m_delta);
}

auto Slider::slideHoverValueRight() -> void {
	this->setHoverValue(m_hoverValue + m_delta);
}

auto Slider::updateHoverValue(const CharWindow& charWindow, const Vec2 mousePosition) -> void {
	const auto screenPosition = charWindow.gridToScreenCoordinates(Vec2{m_position.x + 1, m_position.y});
	const auto screenSize = charWindow.gridToScreenSize(Vec2{m_size.x - 3, m_size.y});
	this->setHoverValue(static_cast<float>(mousePosition.x - screenPosition.x) / static_cast<float>(screenSize.x));
}

auto Slider::onActivate() -> void {
	m_state = State::HOVER;
}

auto Slider::onDeactivate() -> void {
	m_state = State::NORMAL;
	this->setHoverValueToValue();
}

} // namespace gui
