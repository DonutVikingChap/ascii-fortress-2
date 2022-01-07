#include "vertex_array.hpp"

#include "error.hpp"  // gfx::Error
#include "opengl.hpp" // GL..., gl...

namespace gfx {

VertexArray::VertexArray()
	: m_vao(VertexArray::makeVertexArrayObject()) {}

auto VertexArray::get() const noexcept -> Handle {
	return m_vao.get();
}

auto VertexArray::makeVertexArrayObject() -> VertexArrayObject {
	auto handle = Handle{};
	glGenVertexArrays(1, &handle);
	if (!handle) {
		throw Error{"Failed to create vertex array object!"};
	}
	return VertexArrayObject{handle};
}

auto VertexArray::VertexArrayDeleter::operator()(Handle handle) const noexcept -> void {
	glDeleteVertexArrays(1, &handle);
}

} // namespace gfx
