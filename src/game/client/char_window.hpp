#ifndef AF2_CLIENT_CHAR_WINDOW_HPP
#define AF2_CLIENT_CHAR_WINDOW_HPP

#include "../../utilities/tile_matrix.hpp" // util::TileMatrix
#include "../data/color.hpp"               // Color
#include "../data/rectangle.hpp"           // Rect
#include "../data/vector.hpp"              // Vec2
#include "../shared/map.hpp"               // Map
#include "renderer.hpp"                    // Renderer

#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace gfx {
class Framebuffer;
} // namespace gfx

class CharWindow final {
public:
	CharWindow();

	auto update(float deltaTime) -> void;

	auto setVertexShaderFilepath(std::string filepath) -> void;
	auto setFragmentShaderFilepath(std::string filepath) -> void;
	auto setFontFilepath(std::string filepath) -> void;
	auto setFontStaticSize(unsigned staticSize) -> void;
	auto setFontMatchSize(bool matchSize) -> void;
	auto setFontMatchSizeCoefficient(float fontMatchSizeCoefficient) -> void;
	auto setGridRatio(float ratio) -> void;
	auto setWindowSize(Vec2 windowSize) -> void;
	auto setGridSize(Vec2 gridSize) -> void;
	auto setGlyphOffset(Vec2 glyphOffset) -> void;

	[[nodiscard]] auto getFontSize() const noexcept -> unsigned;
	[[nodiscard]] auto getGridSize() const noexcept -> Vec2;
	[[nodiscard]] auto getViewport() const noexcept -> Rect;
	[[nodiscard]] auto getTileSpacing() const noexcept -> Vec2;

	[[nodiscard]] auto screenToGridCoordinates(Vec2 position) const noexcept -> Vec2;
	[[nodiscard]] auto screenToGridSize(Vec2 size) const noexcept -> Vec2;

	[[nodiscard]] auto gridToScreenCoordinates(Vec2 position) const noexcept -> Vec2;
	[[nodiscard]] auto gridToScreenSize(Vec2 size) const noexcept -> Vec2;

	auto draw(Vec2 position, char ch, Color color) -> void;
	auto draw(Vec2 position, Color color) -> void;
	auto drawLineHorizontal(Vec2 position, Vec2::Length length, char ch, Color color) -> void;
	auto drawLineHorizontal(Vec2 position, Vec2::Length length, Color color) -> void;
	auto drawLineVertical(Vec2 position, Vec2::Length length, char ch, Color color) -> void;
	auto drawLineVertical(Vec2 position, Vec2::Length length, Color color) -> void;
	auto drawRect(const Rect& area, char ch, Color color) -> void;
	auto drawRect(const Rect& area, Color color) -> void;
	auto fillRect(const Rect& area, char ch, Color color) -> void;
	auto fillRect(const Rect& area, Color color) -> void;

	auto draw(Vec2 position, std::string_view str, Color color) -> void;
	auto draw(Vec2 position, const util::TileMatrix<char>& matrix, Color color) -> void;
	auto draw(Vec2 position, const util::TileMatrix<char>& matrix, const Rect& srcRect, Color color) -> void;
	auto draw(Vec2 position, const Map& map, const Rect& srcRect, Color worldColor, Color nonSolidColor, bool red, bool blue,
	          char trackChar, Color trackColor, char respawnVisChar, Color respawnVisColor, char resupplyChar, Color resupplyColor) -> void;

	auto fill(char ch, Color color) -> void;
	auto fill(Color color) -> void;

	auto clear() -> void;
	auto clear(Vec2 position) -> void;
	auto clear(const Rect& area) -> void;

	auto addText(Vec2 position, float scaleX, float scaleY, std::string str, Color color) -> void;
	auto clearText() -> void;

	auto render(gfx::Framebuffer& framebuffer) -> void;

private:
	util::TileMatrix<Renderer::Tile> m_tileBuffer{};
	std::vector<Renderer::Text> m_textBuffer{};
	Renderer m_renderer{};
};

#endif
