#ifndef AF2_UTILITIES_RESOURCE_HPP
#define AF2_UTILITIES_RESOURCE_HPP

#include <utility> // std::exchange

namespace util {

template <typename Handle, typename Deleter, Handle NULL_HANDLE = Handle{}>
class Resource final {
public:
	constexpr explicit Resource(Handle handle = NULL_HANDLE) noexcept
		: m_handle(handle) {}

	~Resource() {
		Deleter{}(m_handle);
	}

	Resource(const Resource&) = delete;

	constexpr Resource(Resource&& other) noexcept
		: m_handle(other.release()) {}

	auto operator=(const Resource&) -> Resource& = delete;

	constexpr auto operator=(Resource&& other) noexcept -> Resource& {
		reset(other.release());
		return *this;
	}

	[[nodiscard]] constexpr explicit operator bool() const noexcept {
		return get() != NULL_HANDLE;
	}

	[[nodiscard]] friend constexpr auto operator==(const Resource& a, const Resource& b) noexcept -> bool {
		return a.get() == b.get();
	}

	[[nodiscard]] friend constexpr auto operator!=(const Resource& a, const Resource& b) noexcept -> bool {
		return a.get() != b.get();
	}

	constexpr auto reset(Handle handle = NULL_HANDLE) noexcept -> void {
		Deleter{}(std::exchange(m_handle, handle));
	}

	constexpr auto release() noexcept -> Handle {
		return std::exchange(m_handle, NULL_HANDLE);
	}

	[[nodiscard]] constexpr auto get() const noexcept -> Handle {
		return m_handle;
	}

private:
	Handle m_handle;
};

} // namespace util

#endif
