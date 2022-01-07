#include "debug.hpp"

#ifndef NDEBUG
namespace debug::detail {

auto indent() noexcept -> std::size_t& {
	static auto i = std::size_t{0};
	return i;
}

} // namespace debug::detail
#endif
