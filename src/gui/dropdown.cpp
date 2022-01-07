#include "dropdown.hpp"

#include "../console/commands/gui_dropdown_commands.hpp" // gui_dropdown_...
#include "../game/client/char_window.hpp"                // CharWindow

#include <cassert> // assert
#include <utility> // std::move

namespace gui {

Dropdown::Dropdown(Vec2 position, Vec2 size, Color color, std::vector<std::string> options, std::size_t selectedOptionIndex, Function function)
	: Element(position, size)
	, m_color(color)
	, m_options(std::move(options))
	, m_selectedOptionIndex(selectedOptionIndex)
	, m_hoverOptionIndex(selectedOptionIndex)
	, m_function(std::move(function)) {
	assert(!m_options.empty());
	assert(m_selectedOptionIndex < m_options.size());
	assert(m_hoverOptionIndex < m_options.size());
	if (m_size.x >= 3 && m_size.y >= 1) {
		const auto w = static_cast<std::size_t>(m_size.x);
		const auto h = static_cast<std::size_t>(m_size.y);
		const auto hOpen = static_cast<std::size_t>(h + m_options.size());
		const auto separatorX = static_cast<std::size_t>(w - 3);
		const auto arrowX = static_cast<std::size_t>(w - 2);
		const auto arrowY = static_cast<std::size_t>(h - 1);
		const auto r = static_cast<std::size_t>(w - 1);

		m_closedMatrix.resize(w, h, gui_dropdown_background_char);
		m_closedMatrix.drawLineVertical(0, 0, h, gui_dropdown_left_char);
		m_closedMatrix.drawLineVertical(r, 0, h, gui_dropdown_right_char);
		m_closedMatrix.drawLineVertical(separatorX, 0, h, gui_dropdown_separator_char);
		m_closedMatrix.setUnchecked(arrowX, arrowY, gui_dropdown_arrow_down_char);

		m_openMatrix.resize(w, hOpen, gui_dropdown_background_char);
		m_openMatrix.drawLineVertical(0, 0, hOpen, gui_dropdown_left_char);
		m_openMatrix.drawLineVertical(r, 0, hOpen, gui_dropdown_right_char);
		m_openMatrix.drawLineVertical(separatorX, 0, h, gui_dropdown_separator_char);
		m_openMatrix.setUnchecked(arrowX, arrowY, gui_dropdown_arrow_up_char);
	}
}

auto Dropdown::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	switch (e.type) {
		case SDL_MOUSEBUTTONDOWN:
			if (e.button.button == SDL_BUTTON_LEFT) {
				if (const auto mousePosition = Vec2{e.button.x, e.button.y}; this->getScreenRect(charWindow).contains(mousePosition)) {
					if (!m_activated) {
						this->activate();
					}
					m_hoverOptionIndex = BASE_INDEX;
					m_pressedOptionIndex = BASE_INDEX;
				} else if (m_activated && m_open) {
					for (auto i = std::size_t{0}; i < m_options.size(); ++i) {
						if (this->getOptionScreenRect(i, charWindow).contains(mousePosition)) {
							m_hoverOptionIndex = i;
							m_pressedOptionIndex = i;
							return;
						}
					}
					this->deactivate();
				} else if (m_activated) {
					this->deactivate();
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (e.button.button == SDL_BUTTON_LEFT) {
				if (m_activated) {
					if (const auto mousePosition = Vec2{e.button.x, e.button.y}; this->getScreenRect(charWindow).contains(mousePosition)) {
						m_hoverOptionIndex = BASE_INDEX;
						if (m_pressedOptionIndex == BASE_INDEX) {
							m_open = !m_open;
						}
						m_pressedOptionIndex = NO_INDEX;
					} else if (m_open) {
						for (auto i = std::size_t{0}; i < m_options.size(); ++i) {
							if (this->getOptionScreenRect(i, charWindow).contains(mousePosition)) {
								m_hoverOptionIndex = i;
								if (m_pressedOptionIndex == i) {
									m_pressedOptionIndex = NO_INDEX;
									m_selectedOptionIndex = i;
									if (m_function) {
										m_function(*this);
									}
								} else {
									m_pressedOptionIndex = NO_INDEX;
								}
								return;
							}
						}
						this->deactivate();
					} else {
						this->deactivate();
					}
				}
			}
			break;
		case SDL_KEYDOWN:
			if (m_activated) {
				switch (e.key.keysym.sym) {
					case SDLK_RETURN: [[fallthrough]];
					case SDLK_SPACE: m_pressedOptionIndex = m_hoverOptionIndex; break;
					case SDLK_UP:
						if (m_hoverOptionIndex == NO_INDEX) {
							m_hoverOptionIndex = BASE_INDEX;
						} else if (m_hoverOptionIndex != BASE_INDEX) {
							if (m_hoverOptionIndex > 0) {
								--m_hoverOptionIndex;
							} else {
								m_hoverOptionIndex = BASE_INDEX;
								m_open = false;
							}
						}
						break;
					case SDLK_DOWN:
						if (m_hoverOptionIndex == NO_INDEX) {
							m_hoverOptionIndex = BASE_INDEX;
						} else if (m_hoverOptionIndex == BASE_INDEX) {
							m_hoverOptionIndex = 0;
							m_open = true;
						} else {
							if (m_hoverOptionIndex < m_options.size() - 1) {
								++m_hoverOptionIndex;
							}
						}
						break;
					default: break;
				}
			}
			break;
		case SDL_KEYUP:
			if (m_activated) {
				switch (e.key.keysym.sym) {
					case SDLK_RETURN: [[fallthrough]];
					case SDLK_SPACE:
						if (m_pressedOptionIndex == m_hoverOptionIndex) {
							m_pressedOptionIndex = NO_INDEX;
							if (m_hoverOptionIndex == BASE_INDEX) {
								if (m_open) {
									m_open = false;
								} else {
									m_hoverOptionIndex = 0;
									m_open = true;
								}
							} else if (m_hoverOptionIndex != NO_INDEX) {
								m_selectedOptionIndex = m_hoverOptionIndex;
								if (m_function) {
									m_function(*this);
								}
							}
						}
						break;
					default: break;
				}
			}
			break;
		case SDL_MOUSEMOTION:
			if (const auto mousePosition = Vec2{e.motion.x, e.motion.y}; this->getScreenRect(charWindow).contains(mousePosition)) {
				m_hoverOptionIndex = BASE_INDEX;
			} else if (m_activated && m_open) {
				for (auto i = std::size_t{0}; i < m_options.size(); ++i) {
					if (this->getOptionScreenRect(i, charWindow).contains(mousePosition)) {
						m_hoverOptionIndex = i;
						return;
					}
				}
				m_hoverOptionIndex = NO_INDEX;
			} else {
				m_hoverOptionIndex = NO_INDEX;
			}
			break;
		default: break;
	}
}

auto Dropdown::update(float) -> void {}

auto Dropdown::draw(CharWindow& charWindow) const -> void {
	if (m_size.x >= 4 && m_size.y >= 1) {
		charWindow.draw(m_position, (m_open) ? m_openMatrix : m_closedMatrix, m_color);
		if (m_pressedOptionIndex == BASE_INDEX) {
			charWindow.drawLineHorizontal(Vec2{m_position.x + 1, m_position.y}, static_cast<Vec2::Length>(m_size.x - 4), gui_dropdown_press_char, m_color);
		} else if (m_hoverOptionIndex == BASE_INDEX) {
			charWindow.drawLineHorizontal(Vec2{m_position.x + 1, m_position.y}, static_cast<Vec2::Length>(m_size.x - 4), gui_dropdown_hover_char, m_color);
		}
		charWindow.draw(Vec2{m_position.x + 1, m_position.y}, this->getSelectedOption(), m_color);
		if (m_open) {
			for (auto i = std::size_t{0}; i < m_options.size(); ++i) {
				const auto y = static_cast<Vec2::Length>(m_position.y + 1 + i);
				if (i == m_pressedOptionIndex) {
					charWindow.drawLineHorizontal(Vec2{m_position.x + 1, y}, static_cast<Vec2::Length>(m_size.x - 2), gui_dropdown_press_char, m_color);
				} else if (i == m_hoverOptionIndex) {
					charWindow.drawLineHorizontal(Vec2{m_position.x + 1, y}, static_cast<Vec2::Length>(m_size.x - 2), gui_dropdown_hover_char, m_color);
				}

				if (i == m_selectedOptionIndex) {
					charWindow.draw(Vec2{m_position.x + 1, y}, gui_dropdown_selected_char, m_color);
				}
				charWindow.draw(Vec2{m_position.x + 2, y}, this->getOption(i), m_color);
			}
		}
	}
}

auto Dropdown::setColor(Color color) noexcept -> void {
	m_color = color;
}

auto Dropdown::setSelectedOptionIndex(std::size_t selectedOptionIndex) noexcept -> void {
	if (selectedOptionIndex < m_options.size()) {
		m_selectedOptionIndex = selectedOptionIndex;
	} else {
		m_selectedOptionIndex = m_options.size() - 1;
	}
}

auto Dropdown::setFunction(Function function) -> void {
	m_function = std::move(function);
}

auto Dropdown::getColor() const noexcept -> Color {
	return m_color;
}

auto Dropdown::getOptionCount() const noexcept -> std::size_t {
	return m_options.size();
}

auto Dropdown::getSelectedOptionIndex() const noexcept -> std::size_t {
	return m_selectedOptionIndex;
}

auto Dropdown::getSelectedOption() const noexcept -> std::string_view {
	assert(m_selectedOptionIndex < m_options.size());
	return m_options[m_selectedOptionIndex];
}

auto Dropdown::getOption(std::size_t i) const -> std::string_view {
	assert(i < m_options.size());
	return m_options[i];
}

auto Dropdown::onActivate() -> void {
	m_hoverOptionIndex = BASE_INDEX;
	m_pressedOptionIndex = NO_INDEX;
}

auto Dropdown::onDeactivate() -> void {
	m_open = false;
	m_hoverOptionIndex = NO_INDEX;
	m_pressedOptionIndex = NO_INDEX;
}

auto Dropdown::getOptionScreenRect(std::size_t i, const CharWindow& charWindow) const -> Rect {
	return Rect{charWindow.gridToScreenCoordinates(Vec2{m_position.x, static_cast<Vec2::Length>(m_position.y + m_size.y + i)}),
				charWindow.gridToScreenSize(Vec2{m_size.x, 1})};
}

} // namespace gui
