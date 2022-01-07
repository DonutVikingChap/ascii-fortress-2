#include "menu.hpp"

#include "../game/client/char_window.hpp" // CharWindow
#include "../utilities/algorithm.hpp"     // util::contains, util::find, util::contains, util::noneOf
#include "element.hpp"                    // gui::Element

#include <cassert> // assert
#include <cmath>   // std::remquo, std::signbit
#include <utility> // std::move

namespace gui {

Menu::Menu(std::vector<Element*> elements, Function onSelectNone, Function onEscape, DirectionFunction onDirection, ClickFunction onClick,
		   ScrollFunction onScroll, HoverFunction onHover)
	: m_elements(std::move(elements))
	, m_onSelectNone(std::move(onSelectNone))
	, m_onEscape(std::move(onEscape))
	, m_onDirection(std::move(onDirection))
	, m_onClick(std::move(onClick))
	, m_onScroll(std::move(onScroll))
	, m_onHover(std::move(onHover)) {
	assert(!util::contains(m_elements, nullptr));
}

auto Menu::elements() -> util::Span<Element* const> {
	return m_elements;
}

auto Menu::elements() const -> util::Span<const Element* const> {
	return m_elements;
}

auto Menu::addElement(Element& element) -> void {
	m_elements.push_back(&element);
}

auto Menu::hasElement(const Element& element) const -> bool {
	return util::contains(m_elements, &element);
}

auto Menu::removeElement(Element& element) -> bool {
	for (auto it = m_elements.begin(); it != m_elements.end(); ++it) {
		if (*it == &element) {
			if (m_activeElement == static_cast<std::size_t>(it - m_elements.begin())) {
				m_activeElement = NO_ELEMENT;
			}
			m_elements.erase(it);
			return true;
		}
	}
	return false;
}

auto Menu::getActiveElement() -> Element* {
	return (m_activeElement == NO_ELEMENT) ? nullptr : m_elements[m_activeElement];
}

auto Menu::getActiveElement() const -> const Element* {
	return (m_activeElement == NO_ELEMENT) ? nullptr : m_elements[m_activeElement];
}

auto Menu::activateElement(Element& element) -> void {
	for (auto& element : m_elements) {
		element->deactivate();
	}

	const auto it = util::find(m_elements, &element);
	assert(it != m_elements.end());
	const auto i = it - m_elements.begin();
	m_activeElement = i;
	(*it)->activate();
}

auto Menu::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	if (!m_elements.empty()) {
		if (e.type == SDL_MOUSEBUTTONDOWN) {
			if (e.button.button == SDL_BUTTON_LEFT) {
				const auto mousePosition = Vec2{e.button.x, e.button.y};
				for (auto i = std::size_t{0}; i < m_elements.size(); ++i) {
					if (const auto& element = m_elements[i]; element->getScreenRect(charWindow).contains(mousePosition)) {
						m_activeElement = i;
						break;
					}
				}
				if (m_onClick) {
					m_onClick(*this, charWindow.screenToGridCoordinates(mousePosition));
				}
			}
		} else if (e.type == SDL_MOUSEWHEEL) {
			if (m_onScroll) {
				auto scrollsX = int{};
				std::remquo(e.wheel.preciseX, 1.0f, &scrollsX);
				if (scrollsX == 0) {
					scrollsX = (std::signbit(e.wheel.preciseX)) ? -1 : 1;
				}
				auto scrollsY = int{};
				std::remquo(e.wheel.preciseY, 1.0f, &scrollsY);
				if (scrollsY == 0) {
					scrollsY = (std::signbit(e.wheel.preciseY)) ? -1 : 1;
				}
				auto mouseX = int{};
				auto mouseY = int{};
				SDL_GetMouseState(&mouseX, &mouseY);
				m_onScroll(*this,
						   charWindow.screenToGridCoordinates(Vec2{mouseX, mouseY}),
						   Vec2{static_cast<Vec2::Length>(scrollsX), static_cast<Vec2::Length>(-scrollsY)});
			}
		} else if (e.type == SDL_MOUSEMOTION) {
			if (m_onHover) {
				m_onHover(*this, charWindow.screenToGridCoordinates(Vec2{e.motion.x, e.motion.y}));
			}
		} else if (e.type == SDL_KEYDOWN) {
			switch (e.key.keysym.sym) {
				case SDLK_TAB:
					for (auto& element : m_elements) {
						element->deactivate();
					}
					if ((e.key.keysym.mod & KMOD_SHIFT) != KMOD_NONE) {
						if (m_activeElement == 0 || m_activeElement == NO_ELEMENT) {
							m_activeElement = m_elements.size() - 1;
						} else {
							--m_activeElement;
						}
					} else {
						if (m_activeElement == NO_ELEMENT || m_activeElement == m_elements.size() - 1) {
							m_activeElement = 0;
						} else {
							++m_activeElement;
						}
					}
					m_elements[m_activeElement]->activate();
					break;
				case SDLK_ESCAPE:
					if (m_activeElement == NO_ELEMENT && m_onEscape) {
						m_onEscape(*this);
					} else {
						for (auto& element : m_elements) {
							element->deactivate();
						}
						m_activeElement = NO_ELEMENT;
					}
					break;
				case SDLK_RETURN:
					if (m_onSelectNone && util::noneOf(m_elements, [](const auto& element) { return element->isActivated(); })) {
						m_onSelectNone(*this);
					}
					break;
				case SDLK_LEFT: [[fallthrough]];
				case SDLK_RIGHT: [[fallthrough]];
				case SDLK_UP: [[fallthrough]];
				case SDLK_DOWN:
					if (m_onDirection && util::noneOf(m_elements, [](const auto& element) { return element->isActivated(); })) {
						const auto x = static_cast<Vec2::Length>(static_cast<int>(e.key.keysym.sym == SDLK_RIGHT) -
																 static_cast<int>(e.key.keysym.sym == SDLK_LEFT));
						const auto y = static_cast<Vec2::Length>(static_cast<int>(e.key.keysym.sym == SDLK_DOWN) -
																 static_cast<int>(e.key.keysym.sym == SDLK_UP));
						m_onDirection(*this, Vec2{x, y});
					}
					break;
				default: break;
			}
		}
	}
}

auto Menu::deactivate() -> void {
	m_activeElement = NO_ELEMENT;
}

} // namespace gui
