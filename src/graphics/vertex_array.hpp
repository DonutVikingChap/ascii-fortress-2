#ifndef AF2_GRAPHICS_VERTEX_ARRAY_HPP
#define AF2_GRAPHICS_VERTEX_ARRAY_HPP

#include "../utilities/resource.hpp" // util::Resource
#include "handle.hpp"                // gfx::Handle

namespace gfx {

class VertexArray final {
public:
	VertexArray();

	[[nodiscard]] auto get() const noexcept -> Handle;

private:
	struct VertexArrayDeleter final {
		auto operator()(Handle handle) const noexcept -> void;
	};
	using VertexArrayObject = util::Resource<Handle, VertexArrayDeleter>;

	[[nodiscard]] static auto makeVertexArrayObject() -> VertexArrayObject;

	VertexArrayObject m_vao;
};

} // namespace gfx

#endif
