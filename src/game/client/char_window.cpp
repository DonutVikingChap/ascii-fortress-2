#include "char_window.hpp"

#include "../../graphics/framebuffer.hpp" // gfx::Framebuffer
#include "../../graphics/opengl.hpp"      // GL..., gl...
#include "../../utilities/algorithm.hpp"  // util::fill

#include <cstddef>  // std::size_t
#include <optional> // std::optional, std::nullopt
#include <utility>  // std::move

namespace {

struct LineSegment final {
	Vec2::Length offset = 0;
	Vec2::Length length = 0;
};

[[nodiscard]] auto clipNegative(Vec2::Length offset, Vec2::Length length) -> std::optional<LineSegment> {
	if (length < 0) {
		offset = static_cast<Vec2::Length>(offset + length);
		length = static_cast<Vec2::Length>(-length);
	}

	if (offset < 0) {
		if (length > static_cast<Vec2::Length>(-offset)) {
			length = static_cast<Vec2::Length>(length + offset);
			offset = 0;
		} else {
			return std::nullopt;
		}
	}
	return LineSegment{offset, length};
}

} // namespace

CharWindow::CharWindow() {
	const auto gridSize = m_renderer.getGridSize();
	const auto width = static_cast<std::size_t>(gridSize.x);
	const auto height = static_cast<std::size_t>(gridSize.y);
	m_tileBuffer.resize(width, height, Renderer::Tile{static_cast<char32_t>(' '), Color::transparent()});
}

auto CharWindow::update(float deltaTime) -> void {
	m_renderer.update(deltaTime);
}

auto CharWindow::setVertexShaderFilepath(std::string filepath) -> void {
	m_renderer.setVertexShaderFilepath(std::move(filepath));
}

auto CharWindow::setFragmentShaderFilepath(std::string filepath) -> void {
	m_renderer.setFragmentShaderFilepath(std::move(filepath));
}

auto CharWindow::setFontFilepath(std::string filepath) -> void {
	m_renderer.setFontFilepath(std::move(filepath));
}

auto CharWindow::setFontStaticSize(unsigned staticSize) -> void {
	m_renderer.setFontStaticSize(staticSize);
}

auto CharWindow::setFontMatchSize(bool matchSize) -> void {
	m_renderer.setFontMatchSize(matchSize);
}

auto CharWindow::setFontMatchSizeCoefficient(float fontMatchSizeCoefficient) -> void {
	m_renderer.setFontMatchSizeCoefficient(fontMatchSizeCoefficient);
}

auto CharWindow::setGridRatio(float ratio) -> void {
	m_renderer.setGridRatio(ratio);
}

auto CharWindow::setWindowSize(Vec2 windowSize) -> void {
	m_renderer.setWindowSize(windowSize);
}

auto CharWindow::setGridSize(Vec2 gridSize) -> void {
	const auto width = static_cast<std::size_t>(gridSize.x);
	const auto height = static_cast<std::size_t>(gridSize.y);
	m_tileBuffer.resize(width, height);
	this->clear();
	m_renderer.setGridSize(gridSize);
}

auto CharWindow::setGlyphOffset(Vec2 glyphOffset) -> void {
	m_renderer.setGlyphOffset(glyphOffset);
}

auto CharWindow::getFontSize() const noexcept -> unsigned {
	return m_renderer.getFontSize();
}

auto CharWindow::getGridSize() const noexcept -> Vec2 {
	return m_renderer.getGridSize();
}

auto CharWindow::getViewport() const noexcept -> Rect {
	return m_renderer.getViewport();
}

auto CharWindow::getTileSpacing() const noexcept -> Vec2 {
	return m_renderer.getTileSpacing();
}

auto CharWindow::screenToGridCoordinates(Vec2 position) const noexcept -> Vec2 {
	return m_renderer.screenToGridCoordinates(position);
}

auto CharWindow::screenToGridSize(Vec2 size) const noexcept -> Vec2 {
	return m_renderer.screenToGridSize(size);
}

auto CharWindow::gridToScreenCoordinates(Vec2 position) const noexcept -> Vec2 {
	return m_renderer.gridToScreenCoordinates(position);
}

auto CharWindow::gridToScreenSize(Vec2 size) const noexcept -> Vec2 {
	return m_renderer.gridToScreenSize(size);
}

auto CharWindow::draw(Vec2 position, char ch, Color color) -> void {
	if (ch != '\0') {
		m_tileBuffer.set(static_cast<std::size_t>(position.x), static_cast<std::size_t>(position.y), Renderer::Tile{static_cast<char32_t>(ch), color});
	}
}

auto CharWindow::draw(Vec2 position, Color color) -> void {
	m_tileBuffer.set(static_cast<std::size_t>(position.x), static_cast<std::size_t>(position.y), Renderer::Tile{static_cast<char32_t>('\0'), color});
}

auto CharWindow::drawLineHorizontal(Vec2 position, Vec2::Length length, char ch, Color color) -> void {
	if (ch != '\0') {
		if (const auto clipped = clipNegative(position.x, length)) {
			m_tileBuffer.drawLineHorizontal(static_cast<std::size_t>(clipped->offset),
			                                static_cast<std::size_t>(position.y),
			                                static_cast<std::size_t>(clipped->length),
			                                Renderer::Tile{static_cast<char32_t>(ch), color});
		}
	}
}

auto CharWindow::drawLineHorizontal(Vec2 position, Vec2::Length length, Color color) -> void {
	if (const auto clipped = clipNegative(position.x, length)) {
		m_tileBuffer.drawLineHorizontal(static_cast<std::size_t>(clipped->offset),
		                                static_cast<std::size_t>(position.y),
		                                static_cast<std::size_t>(clipped->length),
		                                Renderer::Tile{static_cast<char32_t>('\0'), color});
	}
}

auto CharWindow::drawLineVertical(Vec2 position, Vec2::Length length, char ch, Color color) -> void {
	if (ch != '\0') {
		if (const auto clipped = clipNegative(position.y, length)) {
			m_tileBuffer.drawLineHorizontal(static_cast<std::size_t>(position.x),
			                                static_cast<std::size_t>(clipped->offset),
			                                static_cast<std::size_t>(clipped->length),
			                                Renderer::Tile{static_cast<char32_t>(ch), color});
		}
	}
}

auto CharWindow::drawLineVertical(Vec2 position, Vec2::Length length, Color color) -> void {
	if (const auto clipped = clipNegative(position.y, length)) {
		m_tileBuffer.drawLineHorizontal(static_cast<std::size_t>(position.x),
		                                static_cast<std::size_t>(clipped->offset),
		                                static_cast<std::size_t>(clipped->length),
		                                Renderer::Tile{static_cast<char32_t>('\0'), color});
	}
}

auto CharWindow::drawRect(const Rect& area, char ch, Color color) -> void {
	if (ch != '\0') {
		if (const auto horizontal = clipNegative(area.x, area.w)) {
			if (const auto vertical = clipNegative(area.y, area.h)) {
				m_tileBuffer.drawRect(static_cast<std::size_t>(horizontal->offset),
				                      static_cast<std::size_t>(vertical->offset),
				                      static_cast<std::size_t>(horizontal->length),
				                      static_cast<std::size_t>(vertical->length),
				                      Renderer::Tile{static_cast<char32_t>(ch), color});
			}
		}
	}
}

auto CharWindow::drawRect(const Rect& area, Color color) -> void {
	if (const auto horizontal = clipNegative(area.x, area.w)) {
		if (const auto vertical = clipNegative(area.y, area.h)) {
			m_tileBuffer.drawRect(static_cast<std::size_t>(horizontal->offset),
			                      static_cast<std::size_t>(vertical->offset),
			                      static_cast<std::size_t>(horizontal->length),
			                      static_cast<std::size_t>(vertical->length),
			                      Renderer::Tile{static_cast<char32_t>('\0'), color});
		}
	}
}

auto CharWindow::fillRect(const Rect& area, char ch, Color color) -> void {
	if (ch != '\0') {
		if (const auto horizontal = clipNegative(area.x, area.w)) {
			if (const auto vertical = clipNegative(area.y, area.h)) {
				m_tileBuffer.fillRect(static_cast<std::size_t>(horizontal->offset),
				                      static_cast<std::size_t>(vertical->offset),
				                      static_cast<std::size_t>(horizontal->length),
				                      static_cast<std::size_t>(vertical->length),
				                      Renderer::Tile{static_cast<char32_t>(ch), color});
			}
		}
	}
}

auto CharWindow::fillRect(const Rect& area, Color color) -> void {
	if (const auto horizontal = clipNegative(area.x, area.w)) {
		if (const auto vertical = clipNegative(area.y, area.h)) {
			m_tileBuffer.fillRect(static_cast<std::size_t>(horizontal->offset),
			                      static_cast<std::size_t>(vertical->offset),
			                      static_cast<std::size_t>(horizontal->length),
			                      static_cast<std::size_t>(vertical->length),
			                      Renderer::Tile{static_cast<char32_t>('\0'), color});
		}
	}
}

auto CharWindow::draw(Vec2 position, std::string_view str, Color color) -> void {
	const auto startX = position.x;
	for (const auto ch : str) {
		if (ch == '\n') {
			position.x = startX;
			++position.y;
		} else {
			this->draw(position, ch, color);
			++position.x;
		}
	}
}

auto CharWindow::draw(Vec2 position, const util::TileMatrix<char>& matrix, Color color) -> void {
	this->draw(position, matrix, Rect{0, 0, static_cast<decltype(Rect::w)>(matrix.getWidth()), static_cast<decltype(Rect::h)>(matrix.getHeight())}, color);
}

auto CharWindow::draw(Vec2 position, const util::TileMatrix<char>& matrix, const Rect& srcRect, Color color) -> void {
	const auto startX = position.x;
	const auto srcRectX = static_cast<std::size_t>(srcRect.x);
	const auto srcRectY = static_cast<std::size_t>(srcRect.y);
	const auto srcRectW = static_cast<std::size_t>(srcRect.w);
	const auto srcRectH = static_cast<std::size_t>(srcRect.h);
	const auto gridSize = this->getGridSize();
	for (auto y = std::size_t{0}; y < srcRectH && y < matrix.getHeight() && position.y < gridSize.y; ++y, ++position.y) {
		for (auto x = std::size_t{0}; x < srcRectW && x < matrix.getWidth() && position.x < gridSize.x; ++x, ++position.x) {
			this->draw(position, matrix.getUnchecked(srcRectX + x, srcRectY + y), color);
		}
		position.x = startX;
	}
}

auto CharWindow::draw(Vec2 position, const Map& map, const Rect& srcRect, Color worldColor, Color nonSolidColor, bool red, bool blue, char trackChar,
                      Color trackColor, char respawnVisChar, Color respawnVisColor, char resupplyChar, Color resupplyColor) -> void {
	const auto viewport = Rect{position.x, position.y, srcRect.w, srcRect.h};

	const auto worldToGridCoordinates = [position, viewPosition = Vec2{srcRect.x, srcRect.y}](Vec2 p) {
		return position + p - viewPosition;
	};

	const auto drawChar = [&](Vec2 p, char ch, Color color) {
		if (const auto tilePosition = worldToGridCoordinates(p); viewport.contains(tilePosition)) {
			this->draw(tilePosition, ch, color);
		}
	};

	// World.
	const auto startX = position.x;
	const auto srcRectX = static_cast<std::size_t>(srcRect.x);
	const auto srcRectY = static_cast<std::size_t>(srcRect.y);
	const auto srcRectW = static_cast<std::size_t>(srcRect.w);
	const auto srcRectH = static_cast<std::size_t>(srcRect.h);
	const auto gridSize = this->getGridSize();
	for (auto y = std::size_t{0}; y < srcRectH && y < map.getMatrix().getHeight() && position.y < gridSize.y; ++y, ++position.y) {
		for (auto x = std::size_t{0}; x < srcRectW && x < map.getMatrix().getWidth() && position.x < gridSize.x; ++x, ++position.x) {
			const auto& ch = map.getMatrix().getUnchecked(srcRectX + x, srcRectY + y);
			this->draw(position, ch, (Map::isSolidChar(ch)) ? worldColor : nonSolidColor);
		}
		position.x = startX;
	}

	// Cart tracks.
	for (const auto& p : map.getBlueCartPath()) {
		drawChar(p, trackChar, trackColor);
	}
	for (const auto& p : map.getRedCartPath()) {
		drawChar(p, trackChar, trackColor);
	}

	// Respawnroom visualizers.
	if (!blue) {
		for (const auto& p : map.getBlueRespawnRoomVisualizers()) {
			drawChar(p, respawnVisChar, respawnVisColor);
		}
	}
	if (!red) {
		for (const auto& p : map.getRedRespawnRoomVisualizers()) {
			drawChar(p, respawnVisChar, respawnVisColor);
		}
	}

	// Resupply lockers.
	for (const auto& p : map.getResupplyLockers()) {
		drawChar(p, resupplyChar, resupplyColor);
	}
}

auto CharWindow::fill(char ch, Color color) -> void {
	if (ch != '\0') {
		util::fill(m_tileBuffer, Renderer::Tile{static_cast<char32_t>(ch), color});
	}
}

auto CharWindow::fill(Color color) -> void {
	util::fill(m_tileBuffer, Renderer::Tile{static_cast<char32_t>('\0'), color});
}

auto CharWindow::clear() -> void {
	util::fill(m_tileBuffer, Renderer::Tile{static_cast<char32_t>(' '), Color::transparent()});
}

auto CharWindow::clear(Vec2 position) -> void {
	m_tileBuffer.set(static_cast<std::size_t>(position.x),
	                 static_cast<std::size_t>(position.y),
	                 Renderer::Tile{static_cast<char32_t>(' '), Color::transparent()});
}

auto CharWindow::clear(const Rect& area) -> void {
	if (const auto horizontal = clipNegative(area.x, area.w)) {
		if (const auto vertical = clipNegative(area.y, area.h)) {
			m_tileBuffer.fillRect(static_cast<std::size_t>(horizontal->offset),
			                      static_cast<std::size_t>(vertical->offset),
			                      static_cast<std::size_t>(horizontal->length),
			                      static_cast<std::size_t>(vertical->length),
			                      Renderer::Tile{static_cast<char32_t>(' '), Color::transparent()});
		}
	}
}

auto CharWindow::addText(Vec2 position, float scaleX, float scaleY, std::string str, Color color) -> void {
	m_textBuffer.push_back(Renderer::Text{std::move(str), position, color, scaleX, scaleY});
}

auto CharWindow::clearText() -> void {
	m_textBuffer.clear();
}

auto CharWindow::render(gfx::Framebuffer& framebuffer) -> void {
	m_renderer.render(m_tileBuffer, m_textBuffer, framebuffer);
}
