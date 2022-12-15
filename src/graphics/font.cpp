#include "font.hpp"

#include "error.hpp"       // gfx::Error
#include "framebuffer.hpp" // gfx::Framebuffer
#include "opengl.hpp"      // GL..., gl...

#include <fmt/core.h>  // fmt::format
#include <string_view> // std::string_view
#include <utility>     // std::move

// clang-format off
#include <ft2build.h>  // FT_FREETYPE_H
#include FT_FREETYPE_H // FT_...
// clang-format on

namespace gfx {
namespace {

[[nodiscard]] auto getErrorString(std::string_view message, FT_Error err) noexcept -> std::string {
	const auto* const str = FT_Error_String(err);
	return fmt::format("{}: {}", message, (str) ? str : "Unknown error!");
}

} // namespace

Font::Font(const char* filepath, unsigned size)
	: m_face([&] {
		auto* face = FT_Face{};
		if (const auto err = FT_New_Face(static_cast<FT_Library>(getLibrary()), filepath, 0, &face); err != FT_Err_Ok) {
			throw Error{getErrorString(fmt::format("Failed to load font \"{}\"", filepath), err)};
		}
		return face;
	}())
	, m_atlasTexture(ATLAS_TEXTURE_INTERNAL_FORMAT, m_atlas.getResolution(), m_atlas.getResolution(), ATLAS_TEXTURE_FLAGS) {
	auto* const face = static_cast<FT_Face>(m_face.get());
	if (const auto err = FT_Select_Charmap(face, FT_ENCODING_UNICODE); err != FT_Err_Ok) {
		throw Error{getErrorString(fmt::format("Failed to load unicode charmap for font \"{}\"", filepath), err)};
	}
	if (const auto err = FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(size)); err != FT_Err_Ok) {
		throw Error{getErrorString(fmt::format("Failed to load font \"{}\" at size {}", filepath, size), err)};
	}
	m_asciiGlyphs[0] = this->renderFilledRectangleGlyph();
	for (auto i = std::size_t{1}; i < m_asciiGlyphs.size(); ++i) {
		m_asciiGlyphs[i] = this->renderGlyph(static_cast<char32_t>(i));
	}
}

Font::operator bool() const noexcept {
	return static_cast<bool>(m_face);
}

auto Font::findGlyph(char32_t ch) const noexcept -> const FontGlyph* {
	if (const auto index = static_cast<std::size_t>(ch); index < m_asciiGlyphs.size()) {
		return &m_asciiGlyphs[index];
	}
	if (const auto it = m_otherGlyphs.find(ch); it != m_otherGlyphs.end()) {
		return &it->second;
	}
	return nullptr;
}

auto Font::loadGlyph(char32_t ch) -> const FontGlyph& {
	if (const auto index = static_cast<std::size_t>(ch); index < m_asciiGlyphs.size()) {
		return m_asciiGlyphs[index];
	}
	const auto [it, inserted] = m_otherGlyphs.try_emplace(ch);
	if (inserted) {
		it->second = this->renderGlyph(ch);
	}
	return it->second;
}

auto Font::getLineMetrics() const noexcept -> FontLineMetrics {
	auto* const face = static_cast<FT_Face>(m_face.get());
	if (!face) {
		return FontLineMetrics{};
	}
	return FontLineMetrics{
		static_cast<float>(face->size->metrics.ascender) * 0.015625f,
		static_cast<float>(face->size->metrics.descender) * 0.015625f,
		static_cast<float>(face->size->metrics.height) * 0.015625f,
	};
}

auto Font::getKerning(char32_t left, char32_t right) const noexcept -> float {
	auto* const face = static_cast<FT_Face>(m_face.get());
	if (!face || !FT_HAS_KERNING(face)) {
		return 0.0f;
	}
	if (left == 0 || right == 0) {
		return 0.0f;
	}
	const auto leftIndex = FT_Get_Char_Index(face, FT_ULong{left});
	const auto rightIndex = FT_Get_Char_Index(face, FT_ULong{right});
	auto kerning = FT_Vector{};
	FT_Get_Kerning(face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &kerning);
	const auto kerningX = static_cast<float>(kerning.x);
	return (FT_IS_SCALABLE(face)) ? kerningX * 0.015625f : kerningX;
}

auto Font::getAtlasTexture() const noexcept -> const Texture& {
	return m_atlasTexture;
}

auto Font::getLibrary() -> void* {
	static auto library = LibraryPtr{[] {
		auto* lib = FT_Library{};
		if (const auto err = FT_Init_FreeType(&lib); err != FT_Err_Ok) {
			throw Error{getErrorString("Failed to initialize FreeType library", err)};
		}
		return lib;
	}()};
	return library.get();
}

auto Font::resizeAtlasTexture() -> void {
	GLint oldFramebufferBinding = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebufferBinding);
	try {
		auto newAtlas = Texture{ATLAS_TEXTURE_INTERNAL_FORMAT, m_atlas.getResolution(), m_atlas.getResolution(), ATLAS_TEXTURE_FLAGS};
		auto fbo = Framebuffer{};
		glBindFramebuffer(GL_FRAMEBUFFER, fbo.get());
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_atlasTexture.get(), 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, newAtlas.get(), 0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		const auto drawAttachments = std::array<GLenum, 1>{GL_COLOR_ATTACHMENT1};
		glDrawBuffers(1, drawAttachments.data());
		const auto width = static_cast<GLint>(m_atlasTexture.getWidth());
		const auto height = static_cast<GLint>(m_atlasTexture.getHeight());
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
		m_atlasTexture = std::move(newAtlas);
		const auto textureSize = vec2{static_cast<float>(m_atlasTexture.getWidth()), static_cast<float>(m_atlasTexture.getHeight())};
		for (auto& glyph : m_asciiGlyphs) {
			glyph.textureOffset = glyph.position / textureSize;
			glyph.textureScale = glyph.size / textureSize;
		}
		for (auto& [ch, glyph] : m_otherGlyphs) {
			glyph.textureOffset = glyph.position / textureSize;
			glyph.textureScale = glyph.size / textureSize;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, oldFramebufferBinding);
	} catch (...) {
		glBindFramebuffer(GL_FRAMEBUFFER, oldFramebufferBinding);
		throw;
	}
}

auto Font::renderFilledRectangleGlyph() -> FontGlyph {
	auto* const face = static_cast<FT_Face>(m_face.get());
	if (!face) {
		return FontGlyph{};
	}
	if (const auto err = FT_Load_Char(face, FT_ULong{'0'}, FT_LOAD_RENDER); err != FT_Err_Ok) {
		throw Error{getErrorString("Failed to render font glyph for char \'0\'", err)};
	}
	const auto width = static_cast<std::size_t>(face->glyph->bitmap.width);
	const auto height = static_cast<std::size_t>(face->glyph->bitmap.rows);
	const auto [x, y, resized] = m_atlas.insert(width, height);
	if (resized) {
		this->resizeAtlasTexture();
	}
	const auto pixels = std::vector<std::byte>(width * height, std::byte{255});
	m_atlasTexture.paste(width, height, TextureFormat::R, TextureComponentType::BYTE, pixels.data(), x, y);
	const auto textureSize = vec2{static_cast<float>(m_atlasTexture.getWidth()), static_cast<float>(m_atlasTexture.getHeight())};
	const auto position = vec2{static_cast<float>(x), static_cast<float>(y)};
	const auto size = vec2{static_cast<float>(width), static_cast<float>(height)};
	const auto bearing = vec2{static_cast<float>(face->glyph->bitmap_left), static_cast<float>(face->glyph->bitmap_top)};
	const auto advance = static_cast<float>(face->glyph->advance.x) * 0.015625f;
	return FontGlyph{position / textureSize, size / textureSize, position, size, bearing, advance};
}

auto Font::renderGlyph(char32_t ch) -> FontGlyph {
	auto* const face = static_cast<FT_Face>(m_face.get());
	if (!face) {
		return FontGlyph{};
	}
	if (const auto err = FT_Load_Char(face, FT_ULong{ch}, FT_LOAD_RENDER); err != FT_Err_Ok) {
		throw Error{getErrorString(fmt::format("Failed to render font glyph for char \'{}\'", FT_ULong{ch}), err)};
	}
	auto x = std::size_t{0};
	auto y = std::size_t{0};
	const auto width = static_cast<std::size_t>(face->glyph->bitmap.width);
	const auto height = static_cast<std::size_t>(face->glyph->bitmap.rows);
	if (const auto* const pixels = face->glyph->bitmap.buffer) {
		if (face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
			throw Error{fmt::format("Invalid font glyph pixel mode for char {}!", FT_ULong{ch})};
		}
		const auto [atlasX, atlasY, resized] = m_atlas.insert(width, height);
		if (resized) {
			this->resizeAtlasTexture();
		}
		x = atlasX;
		y = atlasY;
		m_atlasTexture.paste(width, height, TextureFormat::R, TextureComponentType::BYTE, pixels, x, y);
	}
	const auto textureSize = vec2{static_cast<float>(m_atlasTexture.getWidth()), static_cast<float>(m_atlasTexture.getHeight())};
	const auto position = vec2{static_cast<float>(x), static_cast<float>(y)};
	const auto size = vec2{static_cast<float>(width), static_cast<float>(height)};
	const auto bearing = vec2{static_cast<float>(face->glyph->bitmap_left), static_cast<float>(face->glyph->bitmap_top)};
	const auto advance = static_cast<float>(face->glyph->advance.x) * 0.015625f;
	return FontGlyph{position / textureSize, size / textureSize, position, size, bearing, advance};
}

auto Font::GlyphAtlas::insert(std::size_t width, std::size_t height) -> InsertResult {
	const auto paddedWidth = width + PADDING * std::size_t{2};
	const auto paddedHeight = height + PADDING * std::size_t{2};
	AtlasRow* rowPtr = nullptr;
	for (auto& row : m_rows) {
		if (const auto heightRatio = static_cast<float>(paddedHeight) / static_cast<float>(row.height);
		    heightRatio >= 0.7f && heightRatio <= 1.0f && paddedWidth <= m_resolution - row.width) {
			rowPtr = &row;
			break;
		}
	}
	auto resized = false;
	if (!rowPtr) {
		const auto newRowTop = (m_rows.empty()) ? std::size_t{0} : m_rows.back().top + m_rows.back().height;
		const auto newRowHeight = paddedHeight + paddedHeight / std::size_t{10};
		while (m_resolution < newRowTop + newRowHeight || m_resolution < paddedWidth) {
			m_resolution *= GROWTH_FACTOR;
			resized = true;
		}
		rowPtr = &m_rows.emplace_back(newRowTop, paddedHeight);
	}
	const auto x = rowPtr->width + PADDING;
	const auto y = rowPtr->top + PADDING;
	rowPtr->width += paddedWidth;
	return InsertResult{x, y, resized};
}

auto Font::GlyphAtlas::getResolution() const noexcept -> std::size_t {
	return m_resolution;
}

auto Font::LibraryDeleter::operator()(void* p) const noexcept -> void {
	FT_Done_FreeType(static_cast<FT_Library>(p));
}

auto Font::FaceDeleter::operator()(void* p) const noexcept -> void {
	FT_Done_Face(static_cast<FT_Face>(p));
}

} // namespace gfx
