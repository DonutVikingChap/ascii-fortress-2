#include "renderer.hpp"

#include "../../graphics/error.hpp"       // gfx::Error
#include "../../graphics/framebuffer.hpp" // gfx::Framebuffer
#include "../../graphics/opengl.hpp"      // GL..., gl...

#include <cmath>   // std::floor, std::ceil, std::round
#include <cstddef> // std::size_t, offsetof
#include <utility> // std::move

namespace {

constexpr auto ATLAS_TEXTURE_UNIT = GLint{0};

struct Vertex final {
	gfx::vec2 coordinates{};
};

constexpr auto PRIMITIVE_TYPE = GLenum{GL_TRIANGLE_STRIP};
constexpr auto VERTICES = std::array<Vertex, 4>{
	Vertex{gfx::vec2{0.0f, 1.0f}},
	Vertex{gfx::vec2{0.0f, 0.0f}},
	Vertex{gfx::vec2{1.0f, 1.0f}},
	Vertex{gfx::vec2{1.0f, 0.0f}},
};

[[nodiscard]] constexpr auto toFloatColor(Color color) noexcept -> gfx::vec4 {
	return gfx::vec4{static_cast<float>(color.r), static_cast<float>(color.g), static_cast<float>(color.b), static_cast<float>(color.a)} *
	       (1.0f / 255.0f);
};

} // namespace

Renderer::Renderer() {
	this->updateDimensions();

	GLint oldVertexArrayBinding = 0;
	GLint oldArrayBufferBinding = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVertexArrayBinding);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldArrayBufferBinding);

	glBindVertexArray(m_vertexArray.get());

	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.get());
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(VERTICES.size() * sizeof(Vertex)), VERTICES.data(), GL_STATIC_DRAW);
	{
		const auto coordinatesLocation = GLint{0};
		glEnableVertexAttribArray(coordinatesLocation);
		glVertexAttribPointer(coordinatesLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, coordinates)));
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer.get());
	{
		const auto offsetLocation = GLint{1};
		glEnableVertexAttribArray(offsetLocation);
		glVertexAttribPointer(offsetLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), reinterpret_cast<const void*>(offsetof(GlyphInstance, offset)));
		glVertexAttribDivisor(offsetLocation, 1);

		const auto scaleLocation = GLint{2};
		glEnableVertexAttribArray(scaleLocation);
		glVertexAttribPointer(scaleLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), reinterpret_cast<const void*>(offsetof(GlyphInstance, scale)));
		glVertexAttribDivisor(scaleLocation, 1);

		const auto textureOffsetLocation = GLint{3};
		glEnableVertexAttribArray(textureOffsetLocation);
		glVertexAttribPointer(textureOffsetLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), reinterpret_cast<const void*>(offsetof(GlyphInstance, textureOffset)));
		glVertexAttribDivisor(textureOffsetLocation, 1);

		const auto textureScaleLocation = GLint{4};
		glEnableVertexAttribArray(textureScaleLocation);
		glVertexAttribPointer(textureScaleLocation, 2, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), reinterpret_cast<const void*>(offsetof(GlyphInstance, textureScale)));
		glVertexAttribDivisor(textureScaleLocation, 1);

		const auto colorLocation = GLint{5};
		glEnableVertexAttribArray(colorLocation);
		glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), reinterpret_cast<const void*>(offsetof(GlyphInstance, color)));
		glVertexAttribDivisor(colorLocation, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, oldArrayBufferBinding);
	glBindVertexArray(oldVertexArrayBinding);
}

auto Renderer::update(float deltaTime) -> void {
	m_time += deltaTime;
}

auto Renderer::setVertexShaderFilepath(std::string filepath) -> void {
	m_vertexShaderFilepath = std::move(filepath);
	this->updateShaders();
}

auto Renderer::setFragmentShaderFilepath(std::string filepath) -> void {
	m_fragmentShaderFilepath = std::move(filepath);
	this->updateShaders();
}

auto Renderer::setFontFilepath(std::string filepath) -> void {
	auto oldFontFilepath = std::move(m_fontFilepath);
	m_fontFilepath = std::move(filepath);
	try {
		this->updateFont();
	} catch (const gfx::Error&) {
		m_fontFilepath = std::move(oldFontFilepath);
		throw;
	}
}

auto Renderer::setFontStaticSize(unsigned staticSize) -> void {
	if (m_fontStaticSize != staticSize) {
		m_fontStaticSize = staticSize;
		this->updateFontSize();
	}
}

auto Renderer::setFontMatchSize(bool matchSize) -> void {
	if (m_fontMatchSize != matchSize) {
		m_fontMatchSize = matchSize;
		this->updateFontSize();
	}
}

auto Renderer::setFontMatchSizeCoefficient(float fontMatchSizeCoefficient) -> void {
	if (m_fontMatchSizeCoefficient != fontMatchSizeCoefficient) {
		m_fontMatchSizeCoefficient = fontMatchSizeCoefficient;
		this->updateFontSize();
	}
}

auto Renderer::setGridRatio(float ratio) -> void {
	if (m_gridRatio != ratio) {
		m_gridRatio = ratio;
		this->updateDimensions();
	}
}

auto Renderer::setWindowSize(Vec2 windowSize) -> void {
	if (m_windowSize != windowSize) {
		m_windowSize = windowSize;
		this->updateDimensions();
	}
}

auto Renderer::setGridSize(Vec2 gridSize) -> void {
	if (m_gridSize != gridSize) {
		m_gridSize = gridSize;
		this->updateDimensions();
	}
}

auto Renderer::setGlyphOffset(Vec2 glyphOffset) -> void {
	if (m_baseGlyphOffset != glyphOffset) {
		m_baseGlyphOffset = glyphOffset;
		this->updateGlyphOffset();
	}
}

auto Renderer::getFontSize() const noexcept -> unsigned {
	return m_fontSize;
}

auto Renderer::getGridSize() const noexcept -> Vec2 {
	return m_gridSize;
}

auto Renderer::getViewport() const noexcept -> Rect {
	return m_viewport;
}

auto Renderer::getTileSpacing() const noexcept -> Vec2 {
	return m_tileSpacing;
}

auto Renderer::screenToGridCoordinates(Vec2 position) const noexcept -> Vec2 {
	return this->screenToGridSize(position - m_viewport.getPosition());
}

auto Renderer::screenToGridSize(Vec2 size) const noexcept -> Vec2 {
	return size / m_tileSpacing;
}

auto Renderer::gridToScreenCoordinates(Vec2 position) const noexcept -> Vec2 {
	return m_viewport.getPosition() + this->gridToScreenSize(position);
}

auto Renderer::gridToScreenSize(Vec2 size) const noexcept -> Vec2 {
	return size * m_tileSpacing;
}

auto Renderer::render(const util::TileMatrix<Tile>& tileGrid, util::Span<const Text> texts, gfx::Framebuffer& framebuffer) -> void {
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());

	glUseProgram(m_glyphShader.program.get());

	glUniform1f(m_glyphShader.time.getLocation(), m_time);

	glBindVertexArray(m_vertexArray.get());

	const auto& atlasTexture = m_font.getAtlasTexture();
	glActiveTexture(GL_TEXTURE0 + ATLAS_TEXTURE_UNIT);
	glBindTexture(GL_TEXTURE_2D, atlasTexture.get());
	glUniform1i(m_glyphShader.atlasTexture.getLocation(), ATLAS_TEXTURE_UNIT);

	glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer.get());

	{
		const auto screenSize = gfx::vec2{static_cast<float>(m_viewport.w), static_cast<float>(m_viewport.h)};
		const auto glyphOffset = gfx::vec2{static_cast<float>(m_glyphOffset.x), static_cast<float>(m_glyphOffset.y)};
		glViewport(static_cast<GLint>(m_viewport.x),
		           static_cast<GLint>(m_viewport.y),
		           static_cast<GLsizei>(m_viewport.w),
		           static_cast<GLsizei>(m_viewport.h));
		const auto offset = glyphOffset - screenSize * 0.5f;
		const auto scale = 2.0f / screenSize;
		glUniform2f(m_glyphShader.offset.getLocation(), offset.x, offset.y);
		glUniform2f(m_glyphShader.scale.getLocation(), scale.x, scale.y);

		const auto tileSpacing = gfx::vec2{static_cast<float>(m_tileSpacing.x), static_cast<float>(m_tileSpacing.y)};
		m_glyphInstances.clear();
		for (auto y = std::size_t{0}; y < tileGrid.getHeight(); ++y) {
			for (auto x = std::size_t{0}; x < tileGrid.getWidth(); ++x) {
				const auto& tile = tileGrid.getUnchecked(x, y);
				const auto& glyph = m_font.loadGlyph(tile.ch);
				const auto scale = gfx::vec2{std::round(glyph.size.x), -std::round(glyph.size.y)};
				const auto offset = gfx::vec2{
					std::floor(static_cast<float>(x) * tileSpacing.x + glyph.bearing.x),
					std::floor(screenSize.y - static_cast<float>(y) * tileSpacing.y + glyph.bearing.y),
				};
				m_glyphInstances.emplace_back(offset, scale, glyph.textureOffset, glyph.textureScale, toFloatColor(tile.color));
			}
		}
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_glyphInstances.size() * sizeof(GlyphInstance)), m_glyphInstances.data(), GL_STREAM_DRAW);
		glDrawArraysInstanced(PRIMITIVE_TYPE, 0, VERTICES.size(), static_cast<GLsizei>(m_glyphInstances.size()));
	}

	{
		const auto windowSize = gfx::vec2{static_cast<float>(m_windowSize.x), static_cast<float>(m_windowSize.y)};
		glViewport(0, 0, static_cast<GLsizei>(m_windowSize.x), static_cast<GLsizei>(m_windowSize.y));
		const auto offset = windowSize * -0.5f;
		const auto scale = 2.0f / windowSize;
		glUniform2f(m_glyphShader.offset.getLocation(), offset.x, offset.y);
		glUniform2f(m_glyphShader.scale.getLocation(), scale.x, scale.y);

		m_glyphInstances.clear();
		for (const auto& text : texts) {
			const auto lineStartX = static_cast<float>(text.position.x);
			auto x = lineStartX;
			auto y = static_cast<float>(text.position.y);
			for (auto it = text.str.begin(); it != text.str.end();) {
				if (const auto ch = *it++; ch == '\n') {
					x = lineStartX;
					y += std::floor(m_font.getLineMetrics().height * text.scaleY);
				} else {
					const auto& glyph = m_font.loadGlyph(static_cast<char32_t>(ch));
					const auto scale = gfx::vec2{
						std::round(glyph.size.x * text.scaleX),
						-std::round(glyph.size.y * text.scaleY),
					};
					const auto offset = gfx::vec2{
						std::floor(x + std::round(glyph.bearing.x * text.scaleX)),
						std::floor(windowSize.y - y + std::round(glyph.bearing.y * text.scaleY)),
					};
					m_glyphInstances.emplace_back(offset, scale, glyph.textureOffset, glyph.textureScale, toFloatColor(text.color));
					x += (glyph.advance + m_font.getKerning(ch, (it == text.str.end()) ? char32_t{0} : static_cast<char32_t>(*it))) * text.scaleX;
				}
			}
		}
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_glyphInstances.size() * sizeof(GlyphInstance)), m_glyphInstances.data(), GL_STREAM_DRAW);
		glDrawArraysInstanced(PRIMITIVE_TYPE, 0, VERTICES.size(), static_cast<GLsizei>(m_glyphInstances.size()));
	}
}

auto Renderer::updateShaders() -> void {
	if (!m_vertexShaderFilepath.empty() && !m_fragmentShaderFilepath.empty()) {
		m_glyphShader = GlyphShader{m_vertexShaderFilepath.c_str(), m_fragmentShaderFilepath.c_str()};
	}
}

auto Renderer::updateDimensions() -> void {
	const auto unadjustedTileWidth = static_cast<Vec2::Length>(m_windowSize.x / m_gridSize.x);
	const auto unadjustedTileHeight = static_cast<Vec2::Length>(m_windowSize.y / m_gridSize.y);

	const auto adjustedTileWidth = static_cast<Vec2::Length>(std::round(static_cast<float>(unadjustedTileHeight) / m_gridRatio));
	const auto adjustedTileHeight = static_cast<Vec2::Length>(std::round(static_cast<float>(unadjustedTileWidth) * m_gridRatio));

	if (unadjustedTileHeight < adjustedTileHeight) {
		m_tileSpacing.x = adjustedTileWidth;
		m_tileSpacing.y = unadjustedTileHeight;
	} else {
		m_tileSpacing.x = unadjustedTileWidth;
		m_tileSpacing.y = adjustedTileHeight;
	}

	m_viewport.w = static_cast<Vec2::Length>(m_tileSpacing.x * m_gridSize.x);
	m_viewport.h = static_cast<Vec2::Length>(m_tileSpacing.y * m_gridSize.y);
	m_viewport.x = static_cast<Vec2::Length>(std::floor((static_cast<float>(m_windowSize.x) * 0.5f) - (static_cast<float>(m_viewport.w) * 0.5f)));
	m_viewport.y = static_cast<Vec2::Length>(std::floor((static_cast<float>(m_windowSize.y) * 0.5f) - (static_cast<float>(m_viewport.h) * 0.5f)));

	const auto fontSize = (m_fontMatchSize) ? static_cast<unsigned>(static_cast<float>(m_tileSpacing.y) * m_fontMatchSizeCoefficient) : m_fontStaticSize;
	if (m_fontSize != fontSize) {
		m_fontSize = fontSize;
		if (!m_fontFilepath.empty() && m_fontSize != 0) {
			m_font = gfx::Font{m_fontFilepath.c_str(), m_fontSize};
		}
	}
	this->updateGlyphOffset();
}

auto Renderer::updateFontSize() -> void {
	const auto fontSize = (m_fontMatchSize) ? static_cast<unsigned>(static_cast<float>(m_tileSpacing.y) * m_fontMatchSizeCoefficient) : m_fontStaticSize;
	if (m_fontSize != fontSize) {
		m_fontSize = fontSize;
		this->updateFont();
	}
}

auto Renderer::updateFont() -> void {
	if (!m_fontFilepath.empty() && m_fontSize != 0) {
		m_font = gfx::Font{m_fontFilepath.c_str(), m_fontSize};
		this->updateGlyphOffset();
	}
}

auto Renderer::updateGlyphOffset() -> void {
	m_glyphOffset = Vec2{
		m_baseGlyphOffset.x + static_cast<Vec2::Length>(std::floor(static_cast<float>(m_tileSpacing.x) * 0.5f - m_font.loadGlyph('0').size.x * 0.5f)),
		m_baseGlyphOffset.y + static_cast<Vec2::Length>(std::floor(static_cast<float>(m_tileSpacing.y) * -0.5f -
	                                                               (m_font.getLineMetrics().ascender + m_font.getLineMetrics().descender) * 0.5f)),
	};
}
