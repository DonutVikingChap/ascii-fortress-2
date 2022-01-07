#include "buffer.hpp"

#include "error.hpp"  // gfx::Error
#include "opengl.hpp" // GL..., gl...

namespace gfx {

Buffer::Buffer()
	: m_vbo(Buffer::makeBufferObject()) {}

auto Buffer::get() const noexcept -> Handle {
	return m_vbo.get();
}

auto Buffer::makeBufferObject() -> BufferObject {
	auto handle = Handle{};
	glGenBuffers(1, &handle);
	if (!handle) {
		throw Error{"Failed to create buffer object!"};
	}
	return BufferObject{handle};
}

auto Buffer::BufferDeleter::operator()(Handle handle) const noexcept -> void {
	glDeleteBuffers(1, &handle);
}

} // namespace gfx
