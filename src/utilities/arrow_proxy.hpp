#ifndef AF2_UTILITIES_ARROW_PROXY_HPP
#define AF2_UTILITIES_ARROW_PROXY_HPP

#include <memory>      // std::addressof
#include <type_traits> // std::is_class_v

namespace util {

template <typename Reference>
struct ArrowProxy final {
	Reference ref;

	[[nodiscard]] constexpr auto operator->() noexcept -> Reference* {
		return std::addressof(ref);
	}
};

template <typename Iterator>
[[nodiscard]] constexpr auto arrowOf(Iterator it) -> decltype(auto) {
	if constexpr (std::is_class_v<Iterator>) {
		return it.operator->();
	} else {
		return std::addressof(*it);
	}
}

} // namespace util

#endif
