#include "texture.hpp"

#include "error.hpp"  // gfx::Error
#include "opengl.hpp" // GL..., gl...

#include <fmt/core.h> // fmt::format

namespace gfx {

auto Texture::getChannelCount(TextureFormat format) -> std::size_t {
	switch (format) {
		case TextureFormat::R: return 1;
		case TextureFormat::RG: return 2;
		case TextureFormat::RGB: return 3;
		case TextureFormat::RGBA: return 4;
	}
	return 0;
}

auto Texture::getInternalChannelCount(TextureInternalFormat internalFormat) -> std::size_t {
	switch (internalFormat) {
		case TextureInternalFormat::R8: [[fallthrough]];
		case TextureInternalFormat::R16F: [[fallthrough]];
		case TextureInternalFormat::R32F: return 1;
		case TextureInternalFormat::RG8: [[fallthrough]];
		case TextureInternalFormat::RG16F: [[fallthrough]];
		case TextureInternalFormat::RG32F: return 2;
		case TextureInternalFormat::RGB8: [[fallthrough]];
		case TextureInternalFormat::RGB16F: [[fallthrough]];
		case TextureInternalFormat::RGB32F: return 3;
		case TextureInternalFormat::RGBA8: [[fallthrough]];
		case TextureInternalFormat::RGBA16F: [[fallthrough]];
		case TextureInternalFormat::RGBA32F: return 4;
	}
	return 0;
}

auto Texture::getPixelFormat(std::size_t channelCount) -> TextureFormat {
	switch (channelCount) {
		case 1: return TextureFormat::R;
		case 2: return TextureFormat::RG;
		case 3: return TextureFormat::RGB;
		case 4: return TextureFormat::RGBA;
		default: break;
	}
	throw Error{fmt::format("Invalid texture channel count \"{}\"!", channelCount)};
}

auto Texture::getInternalPixelFormat8BitColor(std::size_t channelCount) -> TextureInternalFormat {
	switch (channelCount) {
		case 1: return TextureInternalFormat::R8;
		case 2: return TextureInternalFormat::RG8;
		case 3: return TextureInternalFormat::RGB8;
		case 4: return TextureInternalFormat::RGBA8;
		default: break;
	}
	throw Error{fmt::format("Invalid texture channel count \"{}\"!", channelCount)};
}

Texture::Texture(TextureInternalFormat internalFormat, std::size_t width, std::size_t height, TextureFormat format,
                 TextureComponentType type, const void* pixels, Flags flags)
	: m_texture(Texture::makeTextureObject())
	, m_internalFormat(internalFormat)
	, m_width(width)
	, m_height(height) {
	GLint oldUnpackAlignment = 0;
	GLint oldTextureBinding2D = 0;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpackAlignment);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureBinding2D);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, m_texture.get());
	glTexImage2D(GL_TEXTURE_2D,
	             0,
	             static_cast<GLint>(internalFormat),
	             static_cast<GLsizei>(width),
	             static_cast<GLsizei>(height),
	             0,
	             static_cast<GLenum>(format),
	             static_cast<GLenum>(type),
	             pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ((flags & REPEAT) != 0) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ((flags & REPEAT) != 0) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ((flags & USE_LINEAR_FILTERING) != 0) ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ((flags & USE_LINEAR_FILTERING) != 0) ? GL_LINEAR : GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, oldTextureBinding2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpackAlignment);
}

Texture::Texture(TextureInternalFormat internalFormat, std::size_t width, std::size_t height, Flags flags)
	: Texture(internalFormat, width, height, TextureFormat::R, TextureComponentType::BYTE, nullptr, flags) {}

auto Texture::paste(std::size_t width, std::size_t height, TextureFormat format, TextureComponentType type, const void* pixels,
                    std::size_t x, std::size_t y) -> void {
	GLint unpackAlignment = 0;
	GLint textureBinding2D = 0;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBinding2D);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, m_texture.get());
	glTexSubImage2D(GL_TEXTURE_2D,
	                0,
	                static_cast<GLint>(x),
	                static_cast<GLint>(y),
	                static_cast<GLsizei>(width),
	                static_cast<GLsizei>(height),
	                static_cast<GLenum>(format),
	                static_cast<GLenum>(type),
	                pixels);

	glBindTexture(GL_TEXTURE_2D, textureBinding2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
}

auto Texture::readPixels8BitColor(TextureFormat format) const -> std::vector<std::byte> {
	auto result = std::vector<std::byte>(m_width * m_height * Texture::getChannelCount(format));
	GLint oldPackAlignment = 0;
	GLint oldTextureBinding2D = 0;
	glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureBinding2D);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, m_texture.get());
	glGetTexImage(GL_TEXTURE_2D, 0, static_cast<GLenum>(format), GL_UNSIGNED_BYTE, result.data());

	glBindTexture(GL_TEXTURE_2D, oldTextureBinding2D);
	glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
	return result;
}

auto Texture::getInternalFormat() const noexcept -> TextureInternalFormat {
	return m_internalFormat;
}

auto Texture::getWidth() const noexcept -> std::size_t {
	return m_width;
}

auto Texture::getHeight() const noexcept -> std::size_t {
	return m_height;
}

auto Texture::get() const noexcept -> Handle {
	return m_texture.get();
}

auto Texture::makeTextureObject() -> TextureObject {
	auto handle = Handle{};
	glGenTextures(1, &handle);
	if (!handle) {
		throw Error{"Failed to create texture object!"};
	}
	return TextureObject{handle};
}

auto Texture::TextureDeleter::operator()(Handle handle) const noexcept -> void {
	glDeleteTextures(1, &handle);
}

} // namespace gfx
