#ifndef AF2_GRAPHICS_TEXTURE_HPP
#define AF2_GRAPHICS_TEXTURE_HPP

#include "../utilities/resource.hpp" // util::Resource
#include "handle.hpp"                // gfx::Handle

#include <cstddef> // std::byte, std::size_t
#include <cstdint> // std::int32_t, std::uint32_t, std::uint8_t
#include <vector>  // std::vector

namespace gfx {

enum class TextureFormat : std::uint32_t {
	R = 0x1903,
	RG = 0x8227,
	RGB = 0x1907,
	RGBA = 0x1908,
};

enum class TextureInternalFormat : std::int32_t {
	R8 = 0x8229,
	RG8 = 0x822B,
	RGB8 = 0x8051,
	RGBA8 = 0x8058,
	R16F = 0x822D,
	RG16F = 0x822F,
	RGB16F = 0x881B,
	RGBA16F = 0x881A,
	R32F = 0x822E,
	RG32F = 0x8230,
	RGB32F = 0x8815,
	RGBA32F = 0x8814,
};

enum class TextureComponentType : std::uint32_t {
	BYTE = 0x1401,
	FLOAT = 0x1406,
};

class Texture final {
public:
	using Flags = std::uint8_t;
	enum Flag : Flags {
		NO_FLAGS = 0,
		REPEAT = 1 << 0,
		USE_LINEAR_FILTERING = 1 << 1,
	};

	[[nodiscard]] static auto getChannelCount(TextureFormat format) -> std::size_t;
	[[nodiscard]] static auto getInternalChannelCount(TextureInternalFormat internalFormat) -> std::size_t;

	[[nodiscard]] static auto getPixelFormat(std::size_t channelCount) -> TextureFormat;
	[[nodiscard]] static auto getInternalPixelFormat8BitColor(std::size_t channelCount) -> TextureInternalFormat;

	constexpr Texture() noexcept = default;

	constexpr explicit operator bool() const noexcept {
		return static_cast<bool>(m_texture);
	}

	Texture(TextureInternalFormat internalFormat, std::size_t width, std::size_t height, TextureFormat format, TextureComponentType type,
	        const void* pixels, Flags flags);
	Texture(TextureInternalFormat internalFormat, std::size_t width, std::size_t height, Flags flags);

	auto paste(std::size_t width, std::size_t height, TextureFormat format, TextureComponentType type, const void* pixels, std::size_t x,
	           std::size_t y) -> void;

	[[nodiscard]] auto readPixels8BitColor(TextureFormat format) const -> std::vector<std::byte>;

	[[nodiscard]] auto getInternalFormat() const noexcept -> TextureInternalFormat;
	[[nodiscard]] auto getWidth() const noexcept -> std::size_t;
	[[nodiscard]] auto getHeight() const noexcept -> std::size_t;

	[[nodiscard]] auto get() const noexcept -> Handle;

private:
	struct TextureDeleter final {
		auto operator()(Handle handle) const noexcept -> void;
	};
	using TextureObject = util::Resource<Handle, TextureDeleter>;

	[[nodiscard]] static auto makeTextureObject() -> TextureObject;

	TextureObject m_texture{};
	TextureInternalFormat m_internalFormat{};
	std::size_t m_width = 0;
	std::size_t m_height = 0;
};

} // namespace gfx

#endif
