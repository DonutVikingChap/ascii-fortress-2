#ifndef AF2_GUI_ELEMENT_HPP
#define AF2_GUI_ELEMENT_HPP

#include "../game/data/rectangle.hpp" // Rect
#include "../game/data/vector.hpp"    // Vec2

#include <SDL.h> // SDL_...

class CharWindow;

namespace gui {

class Element {
public:
	Element(Vec2 position, Vec2 size);
	virtual ~Element() = default;

	Element(const Element&) = default;
	Element(Element&&) noexcept = default;

	auto operator=(const Element&) -> Element& = default;
	auto operator=(Element&&) noexcept -> Element& = default;

	virtual auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void = 0;

	virtual auto update(float deltaTime) -> void = 0;

	virtual auto draw(CharWindow& charWindow) const -> void = 0;

	auto activate() -> void;
	auto deactivate() -> void;

	[[nodiscard]] auto isActivated() const -> bool;

	[[nodiscard]] auto getPosition() const -> Vec2;
	[[nodiscard]] auto getSize() const -> Vec2;

	[[nodiscard]] auto getScreenRect(const CharWindow& charWindow) const -> Rect;

protected:
	virtual auto onActivate() -> void {}

	virtual auto onDeactivate() -> void {}

	Vec2 m_position;
	Vec2 m_size;
	bool m_activated = false;
};

} // namespace gui

#endif
