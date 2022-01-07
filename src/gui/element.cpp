#include "element.hpp"

#include "../game/client/char_window.hpp" // CharWindow

namespace gui {

Element::Element(Vec2 position, Vec2 size)
	: m_position(position)
	, m_size(size) {}

auto Element::activate() -> void {
	if (!m_activated) {
		m_activated = true;
		this->onActivate();
	}
}

auto Element::deactivate() -> void {
	if (m_activated) {
		m_activated = false;
		this->onDeactivate();
	}
}

auto Element::isActivated() const -> bool {
	return m_activated;
}

auto Element::getPosition() const -> Vec2 {
	return m_position;
}

auto Element::getSize() const -> Vec2 {
	return m_size;
}

auto Element::getScreenRect(const CharWindow& charWindow) const -> Rect {
	return Rect{charWindow.gridToScreenCoordinates(m_position), charWindow.gridToScreenSize(m_size)};
}

} // namespace gui
