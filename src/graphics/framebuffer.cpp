#include "framebuffer.hpp"

#include "error.hpp"  // gfx::Error
#include "opengl.hpp" // GL..., gl...

namespace gfx {

auto Framebuffer::getDefault() -> Framebuffer& {
	static auto fbo = Framebuffer{Handle{}};
	return fbo;
}

Framebuffer::Framebuffer()
	: m_fbo(Framebuffer::makeFramebufferObject()) {}

auto Framebuffer::get() const noexcept -> Handle {
	return m_fbo.get();
}

Framebuffer::Framebuffer(Handle handle) noexcept
	: m_fbo(handle) {}

auto Framebuffer::makeFramebufferObject() -> FramebufferObject {
	auto handle = Handle{};
	glGenFramebuffers(1, &handle);
	if (!handle) {
		throw Error{"Failed to create framebuffer object!"};
	}
	return FramebufferObject{handle};
}

auto Framebuffer::FramebufferDeleter::operator()(Handle handle) const noexcept -> void {
	glDeleteFramebuffers(1, &handle);
}

} // namespace gfx
