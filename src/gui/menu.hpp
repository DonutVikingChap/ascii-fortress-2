#ifndef AF2_GUI_MENU_HPP
#define AF2_GUI_MENU_HPP

#include "../game/data/vector.hpp" // Vec2
#include "../utilities/span.hpp"   // util::Span

#include <SDL.h>      // SDL_..., KMOD_...
#include <cstddef>    // std::size_t
#include <functional> // std::function
#include <limits>     // std::numeric_limits
#include <vector>     // std::vector

class CharWindow;

namespace gui {

class Element;

class Menu final {
public:
	using Function = std::function<void(Menu& menu)>;
	using DirectionFunction = std::function<void(Menu& menu, Vec2 offset)>;
	using ClickFunction = std::function<void(Menu& menu, Vec2 position)>;
	using ScrollFunction = std::function<void(Menu& menu, Vec2 position, Vec2 offset)>;
	using HoverFunction = std::function<void(Menu& menu, Vec2 position)>;

	explicit Menu(std::vector<Element*> elements, Function onSelectNone = nullptr, Function onEscape = nullptr, DirectionFunction onDirection = nullptr,
	              ClickFunction onClick = nullptr, ScrollFunction onScroll = nullptr, HoverFunction onHover = nullptr);

	[[nodiscard]] auto elements() -> util::Span<Element* const>;
	[[nodiscard]] auto elements() const -> util::Span<const Element* const>;

	auto addElement(Element& element) -> void;
	[[nodiscard]] auto hasElement(const Element& element) const -> bool;
	auto removeElement(Element& element) -> bool;

	[[nodiscard]] auto getActiveElement() -> Element*;
	[[nodiscard]] auto getActiveElement() const -> const Element*;

	auto activateElement(Element& element) -> void;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void;

	auto deactivate() -> void;

private:
	static constexpr auto NO_ELEMENT = std::numeric_limits<std::size_t>::max();

	std::vector<Element*> m_elements;
	Function m_onSelectNone;
	Function m_onEscape;
	DirectionFunction m_onDirection;
	ClickFunction m_onClick;
	ScrollFunction m_onScroll;
	HoverFunction m_onHover;
	std::size_t m_activeElement = NO_ELEMENT;
};

} // namespace gui

#endif
