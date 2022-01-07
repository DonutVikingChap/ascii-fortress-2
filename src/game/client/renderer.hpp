#ifndef AF2_CLIENT_RENDERER_HPP
#define AF2_CLIENT_RENDERER_HPP

#include "../../graphics/buffer.hpp"       // gfx::Buffer
#include "../../graphics/font.hpp"         // gfx::Font
#include "../../graphics/glsl.hpp"         // gfx::vec2, gfx::vec4
#include "../../graphics/shader.hpp"       // gfx::ShaderProgram, gfx::ShaderUniform
#include "../../graphics/vertex_array.hpp" // gfx::VertexArray
#include "../../gui/layout.hpp"            // gui::GRID_SIZE_...
#include "../../utilities/span.hpp"        // util::Span
#include "../../utilities/tile_matrix.hpp" // util::TileMatrix
#include "../data/color.hpp"               // Color
#include "../data/rectangle.hpp"           // Rect
#include "../data/vector.hpp"              // Vec2

#include <string> // std::string
#include <vector> // std::vector

namespace gfx {
class Framebuffer;
} // namespace gfx

class Renderer final {
public:
	struct Tile final {
		char32_t ch{};
		Color color{};
	};

	struct Text final {
		std::string str{};
		Vec2 position{};
		Color color{};
		float scaleX = 0.0f;
		float scaleY = 0.0f;
	};

	static constexpr auto DEFAULT_WINDOW_WIDTH = Vec2::Length{826};
	static constexpr auto DEFAULT_WINDOW_HEIGHT = Vec2::Length{732};
	static constexpr auto DEFAULT_GRID_WIDTH = Vec2::Length{gui::GRID_SIZE_X};
	static constexpr auto DEFAULT_GRID_HEIGHT = Vec2::Length{gui::GRID_SIZE_Y};
	static constexpr auto DEFAULT_FONT_STATIC_SIZE = 20u;
	static constexpr auto DEFAULT_FONT_MATCH_SIZE = true;
	static constexpr auto DEFAULT_FONT_MATCH_SIZE_COEFFICIENT = 1.2f;
	static constexpr auto DEFAULT_GRID_RATIO = 1.1f;

	Renderer();

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

	auto render(const util::TileMatrix<Tile>& tileGrid, util::Span<const Text> texts, gfx::Framebuffer& framebuffer) -> void;

private:
	struct GlyphInstance final {
		GlyphInstance(gfx::vec2 offset, gfx::vec2 scale, gfx::vec2 textureOffset, gfx::vec2 textureScale, gfx::vec4 color) noexcept
			: offset(offset)
			, scale(scale)
			, textureOffset(textureOffset)
			, textureScale(textureScale)
			, color(color) {}

		gfx::vec2 offset{};
		gfx::vec2 scale{};
		gfx::vec2 textureOffset{};
		gfx::vec2 textureScale{};
		gfx::vec4 color{};
	};

	struct GlyphShader final {
		explicit GlyphShader(const char* vertexShaderFilepath = nullptr, const char* fragmentShaderFilepath = nullptr)
			: program(vertexShaderFilepath, fragmentShaderFilepath) {}

		gfx::ShaderProgram program;
		gfx::ShaderUniform offset{program, "offset"};
		gfx::ShaderUniform scale{program, "scale"};
		gfx::ShaderUniform time{program, "time"};
		gfx::ShaderUniform atlasTexture{program, "atlasTexture"};
	};

	auto updateShaders() -> void;
	auto updateDimensions() -> void;
	auto updateFontSize() -> void;
	auto updateFont() -> void;
	auto updateGlyphOffset() -> void;

	std::string m_vertexShaderFilepath{};
	std::string m_fragmentShaderFilepath{};
	std::string m_fontFilepath{};
	unsigned m_fontStaticSize = DEFAULT_FONT_STATIC_SIZE;
	bool m_fontMatchSize = DEFAULT_FONT_MATCH_SIZE;
	float m_fontMatchSizeCoefficient = DEFAULT_FONT_MATCH_SIZE_COEFFICIENT;
	float m_gridRatio = DEFAULT_GRID_RATIO;
	Vec2 m_windowSize{DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT};
	Vec2 m_gridSize{DEFAULT_GRID_WIDTH, DEFAULT_GRID_HEIGHT};
	GlyphShader m_glyphShader{};
	gfx::Font m_font{};
	unsigned m_fontSize{};
	Rect m_viewport{};
	Vec2 m_tileSpacing{};
	Vec2 m_baseGlyphOffset{};
	Vec2 m_glyphOffset{};
	gfx::VertexArray m_vertexArray{};
	gfx::Buffer m_vertexBuffer{};
	gfx::Buffer m_instanceBuffer{};
	std::vector<GlyphInstance> m_glyphInstances{};
	float m_time = 0.0f;
};

#endif
