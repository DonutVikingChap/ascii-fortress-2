#ifndef AF2_GRAPHICS_FRAMEBUFFER_HPP
#define AF2_GRAPHICS_FRAMEBUFFER_HPP

#include "../utilities/resource.hpp" // util::Resource
#include "handle.hpp"                // gfx::Handle

namespace gfx {

class Framebuffer final {
public:
	[[nodiscard]] static auto getDefault() -> Framebuffer&;

	Framebuffer();

	[[nodiscard]] auto get() const noexcept -> Handle;

private:
	explicit Framebuffer(Handle handle) noexcept;

	struct FramebufferDeleter final {
		auto operator()(Handle handle) const noexcept -> void;
	};
	using FramebufferObject = util::Resource<Handle, FramebufferDeleter>;

	[[nodiscard]] static auto makeFramebufferObject() -> FramebufferObject;

	FramebufferObject m_fbo;
};

} // namespace gfx

#endif
