#ifndef AF2_GRAPHICS_BUFFER_HPP
#define AF2_GRAPHICS_BUFFER_HPP

#include "../utilities/resource.hpp" // util::Resource
#include "handle.hpp"                // gfx::Handle

namespace gfx {

class Buffer final {
public:
	Buffer();

	[[nodiscard]] auto get() const noexcept -> Handle;

private:
	struct BufferDeleter final {
		auto operator()(Handle handle) const noexcept -> void;
	};
	using BufferObject = util::Resource<Handle, BufferDeleter>;

	[[nodiscard]] static auto makeBufferObject() -> BufferObject;

	BufferObject m_vbo;
};

} // namespace gfx

#endif
