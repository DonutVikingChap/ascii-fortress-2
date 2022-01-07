#ifndef AF2_GRAPHICS_FONT_HPP
#define AF2_GRAPHICS_FONT_HPP

#include "glsl.hpp"    // gfx::vec2
#include "texture.hpp" // gfx::Texture, gfx::TextureInternalFormat, gfx::TextureFormat, gfx::TextureComponentType

#include <array>         // std::array
#include <cstddef>       // std::byte, std::size_t
#include <memory>        // std::unique_ptr
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace gfx {

struct FontGlyph final {
	vec2 textureOffset{};
	vec2 textureScale{};
	vec2 position{};
	vec2 size{};
	vec2 bearing{};
	float advance = 0.0f;
};

struct FontLineMetrics final {
	float ascender = 0.0f;
	float descender = 0.0f;
	float height = 0.0f;
};

class Font final {
public:
	Font() noexcept = default;

	Font(const char* filepath, unsigned size);

	explicit operator bool() const noexcept;

	[[nodiscard]] auto findGlyph(char32_t ch) const noexcept -> const FontGlyph*;
	auto loadGlyph(char32_t ch) -> const FontGlyph&;

	[[nodiscard]] auto getLineMetrics() const noexcept -> FontLineMetrics;
	[[nodiscard]] auto getKerning(char32_t left, char32_t right) const noexcept -> float;
	[[nodiscard]] auto getAtlasTexture() const noexcept -> const Texture&;

private:
	class GlyphAtlas final {
	public:
		static constexpr auto INITIAL_RESOLUTION = std::size_t{128};
		static constexpr auto GROWTH_FACTOR = std::size_t{2};
		static constexpr auto PADDING = std::size_t{6};

		struct InsertResult final {
			std::size_t x;
			std::size_t y;
			bool resized;
		};

		[[nodiscard]] auto insert(std::size_t width, std::size_t height) -> InsertResult;

		[[nodiscard]] auto getResolution() const noexcept -> std::size_t;

	private:
		struct AtlasRow final {
			AtlasRow(std::size_t top, std::size_t height) noexcept
				: top(top)
				, height(height) {}

			std::size_t top;
			std::size_t width = 0;
			std::size_t height;
		};

		std::vector<AtlasRow> m_rows{};
		std::size_t m_resolution = INITIAL_RESOLUTION;
	};

	struct LibraryDeleter final {
		auto operator()(void* p) const noexcept -> void;
	};
	using LibraryPtr = std::unique_ptr<void, LibraryDeleter>;

	struct FaceDeleter final {
		auto operator()(void* p) const noexcept -> void;
	};
	using FacePtr = std::unique_ptr<void, FaceDeleter>;

	static constexpr auto ATLAS_TEXTURE_INTERNAL_FORMAT = TextureInternalFormat::R8;
	static constexpr auto ATLAS_TEXTURE_FLAGS = Texture::Flags{Texture::USE_LINEAR_FILTERING};

	[[nodiscard]] static auto getLibrary() -> void*;

	auto resizeAtlasTexture() -> void;

	[[nodiscard]] auto renderFilledRectangleGlyph() -> FontGlyph;
	[[nodiscard]] auto renderGlyph(char32_t ch) -> FontGlyph;

	FacePtr m_face{};
	GlyphAtlas m_atlas{};
	Texture m_atlasTexture{};
	std::unordered_map<char32_t, FontGlyph> m_otherGlyphs{};
	std::array<FontGlyph, 128> m_asciiGlyphs{};
};

} // namespace gfx

#endif
