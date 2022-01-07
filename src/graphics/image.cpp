#include "image.hpp"

#include "error.hpp" // gfx::Error

#include <fmt/core.h>        // fmt::format
#include <stb_image.h>       // stbi_...
#include <stb_image_write.h> // stbi_write_...
#include <utility>           // std::move, std::exchange

namespace gfx {

Image::Image(const char* filepath, const ImageOptions& options) {
	stbi_set_flip_vertically_on_load_thread(options.flipVertically ? 1 : 0);
	auto width = int{};
	auto height = int{};
	auto channelCount = int{};
	if (options.highDynamicRange) {
		m_pixels.reset(stbi_loadf(filepath, &width, &height, &channelCount, options.desiredChannelCount));
		if (!m_pixels) {
			throw Error{fmt::format("Failed to load HDR image \"{}\": {}", filepath, stbi_failure_reason())};
		}
	} else {
		m_pixels.reset(stbi_load(filepath, &width, &height, &channelCount, options.desiredChannelCount));
		if (!m_pixels) {
			throw Error{fmt::format("Failed to load image \"{}\": {}", filepath, stbi_failure_reason())};
		}
	}
	m_width = static_cast<std::size_t>(width);
	m_height = static_cast<std::size_t>(height);
	m_channelCount = static_cast<std::size_t>(channelCount);
}

Image::operator ImageView() const noexcept {
	return ImageView{m_pixels.get(), m_width, m_height, m_channelCount};
}

auto Image::getPixels() noexcept -> void* {
	return m_pixels.get();
}

auto Image::getPixels() const noexcept -> const void* {
	return m_pixels.get();
}

auto Image::getWidth() const noexcept -> std::size_t {
	return m_width;
}

auto Image::getHeight() const noexcept -> std::size_t {
	return m_height;
}

auto Image::getChannelCount() const noexcept -> std::size_t {
	return m_channelCount;
}

auto Image::PixelsDeleter::operator()(void* p) const noexcept -> void {
	stbi_image_free(p);
}

auto savePNG(ImageView image, const char* filepath, const ImageOptionsPNG& options) -> void {
	stbi_flip_vertically_on_write(options.flipVertically ? 1 : 0);
	stbi_write_png_compression_level = options.compressionLevel;
	const auto width = static_cast<int>(image.getWidth());
	const auto height = static_cast<int>(image.getHeight());
	const auto channelCount = static_cast<int>(image.getChannelCount());
	const auto* const pixels = static_cast<const stbi_uc*>(image.getPixels());
	if (stbi_write_png(filepath, width, height, channelCount, pixels, 0) == 0) {
		throw Error{fmt::format("Failed to save PNG image \"{}\"!", filepath)};
	}
}

auto saveBMP(ImageView image, const char* filepath, const ImageOptionsBMP& options) -> void {
	stbi_flip_vertically_on_write(options.flipVertically ? 1 : 0);
	const auto width = static_cast<int>(image.getWidth());
	const auto height = static_cast<int>(image.getHeight());
	const auto channelCount = static_cast<int>(image.getChannelCount());
	const auto* const pixels = static_cast<const stbi_uc*>(image.getPixels());
	if (stbi_write_bmp(filepath, width, height, channelCount, pixels) == 0) {
		throw Error{fmt::format("Failed to save BMP image \"{}\"!", filepath)};
	}
}

auto saveTGA(ImageView image, const char* filepath, const ImageOptionsTGA& options) -> void {
	stbi_flip_vertically_on_write(options.flipVertically ? 1 : 0);
	stbi_write_tga_with_rle = (options.useRLECompression) ? 1 : 0;
	const auto width = static_cast<int>(image.getWidth());
	const auto height = static_cast<int>(image.getHeight());
	const auto channelCount = static_cast<int>(image.getChannelCount());
	const auto* const pixels = static_cast<const stbi_uc*>(image.getPixels());
	if (stbi_write_tga(filepath, width, height, channelCount, pixels) == 0) {
		throw Error{fmt::format("Failed to save TGA image \"{}\"!", filepath)};
	}
}

auto saveJPG(ImageView image, const char* filepath, const ImageOptionsJPG& options) -> void {
	stbi_flip_vertically_on_write(options.flipVertically ? 1 : 0);
	const auto width = static_cast<int>(image.getWidth());
	const auto height = static_cast<int>(image.getHeight());
	const auto channelCount = static_cast<int>(image.getChannelCount());
	const auto* const pixels = static_cast<const stbi_uc*>(image.getPixels());
	if (stbi_write_jpg(filepath, width, height, channelCount, pixels, options.quality) == 0) {
		throw Error{fmt::format("Failed to save JPG image \"{}\"!", filepath)};
	}
}

auto saveHDR(ImageView image, const char* filepath, const ImageOptionsHDR& options) -> void {
	stbi_flip_vertically_on_write(options.flipVertically ? 1 : 0);
	const auto width = static_cast<int>(image.getWidth());
	const auto height = static_cast<int>(image.getHeight());
	const auto channelCount = static_cast<int>(image.getChannelCount());
	const auto* const pixels = static_cast<const float*>(image.getPixels());
	if (stbi_write_hdr(filepath, width, height, channelCount, pixels) == 0) {
		throw Error{fmt::format("Failed to save HDR image \"{}\"!", filepath)};
	}
}

} // namespace gfx
