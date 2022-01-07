#ifndef AF2_GUI_CONSOLE_HPP
#define AF2_GUI_CONSOLE_HPP

#include "../game/data/color.hpp"       // Color
#include "../utilities/tile_matrix.hpp" // util::TileMatrix
#include "element.hpp"

#include <SDL.h>       // SDL_...
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace gui {

class Console final : public Element {
public:
	Console(Vec2 position, Vec2 size, Color color, std::size_t maxRows);

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void override;

	auto update(float deltaTime) -> void override;

	auto draw(CharWindow& charWindow) const -> void override;

	auto setMaxRows(std::size_t maxRows) -> void;

	[[nodiscard]] auto getMaxRows() const -> std::size_t;

	auto print(std::string_view text, Color color) -> void;
	auto clear() -> void;

	auto resetScroll() -> void;
	auto scrollUp() -> void;
	auto scrollDown() -> void;
	auto scroll(std::ptrdiff_t scrolls) -> void;

private:
	auto eraseExcessRows() -> void;

	struct ColoredChar final {
		char ch = '\0';
		Color color{};
	};
	using TextRow = std::vector<ColoredChar>;

	Color m_color;
	std::vector<TextRow> m_text;
	std::size_t m_maxRows = 0;
	util::TileMatrix<char> m_matrix{};
	std::ptrdiff_t m_scrollRow = 0;
	bool m_mouseOver = false;
};

} // namespace gui

#endif
