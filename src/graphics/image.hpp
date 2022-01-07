#ifndef AF2_GRAPHICS_IMAGE_HPP
#define AF2_GRAPHICS_IMAGE_HPP

#include <cstddef> // std::size_t
#include <memory>  // std::unique_ptr

namespace gfx {

class ImageView final {
public:
	constexpr ImageView() noexcept = default;

	constexpr ImageView(const void* pixels, std::size_t width, std::size_t height, std::size_t channelCount) noexcept
		: m_pixels(pixels)
		, m_width(width)
		, m_height(height)
		, m_channelCount(channelCount) {}

	[[nodiscard]] constexpr auto getPixels() const noexcept -> const void* {
		return m_pixels;
	}

	[[nodiscard]] constexpr auto getWidth() const noexcept -> std::size_t {
		return m_width;
	}

	[[nodiscard]] constexpr auto getHeight() const noexcept -> std::size_t {
		return m_height;
	}

	[[nodiscard]] constexpr auto getChannelCount() const noexcept -> std::size_t {
		return m_channelCount;
	}

private:
	const void* m_pixels = nullptr;
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channelCount = 0;
};

struct ImageOptions final {
	int desiredChannelCount = 0;
	bool highDynamicRange = false;
	bool flipVertically = false;
};

class Image final {
public:
	Image() noexcept = default;
	explicit Image(const char* filepath, const ImageOptions& options = {});

	operator ImageView() const noexcept;

	[[nodiscard]] auto getPixels() noexcept -> void*;
	[[nodiscard]] auto getPixels() const noexcept -> const void*;
	[[nodiscard]] auto getWidth() const noexcept -> std::size_t;
	[[nodiscard]] auto getHeight() const noexcept -> std::size_t;
	[[nodiscard]] auto getChannelCount() const noexcept -> std::size_t;

private:
	struct PixelsDeleter final {
		auto operator()(void* p) const noexcept -> void;
	};
	using PixelsPtr = std::unique_ptr<void, PixelsDeleter>;

	PixelsPtr m_pixels{};
	std::size_t m_width = 0;
	std::size_t m_height = 0;
	std::size_t m_channelCount = 0;
};

struct ImageOptionsPNG final {
	int compressionLevel = 8;
	bool flipVertically = false;
};

auto savePNG(ImageView image, const char* filepath, const ImageOptionsPNG& options = {}) -> void;

struct ImageOptionsBMP final {
	bool flipVertically = false;
};

auto saveBMP(ImageView image, const char* filepath, const ImageOptionsBMP& options = {}) -> void;

struct ImageOptionsTGA final {
	bool useRLECompression = true;
	bool flipVertically = false;
};

auto saveTGA(ImageView image, const char* filepath, const ImageOptionsTGA& options = {}) -> void;

struct ImageOptionsJPG final {
	int quality = 90;
	bool flipVertically = false;
};

auto saveJPG(ImageView image, const char* filepath, const ImageOptionsJPG& options = {}) -> void;

struct ImageOptionsHDR final {
	bool flipVertically = false;
};

auto saveHDR(ImageView image, const char* filepath, const ImageOptionsHDR& options = {}) -> void;

} // namespace gfx

#endif
