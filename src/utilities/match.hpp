#ifndef AF2_UTILITIES_MATCH_HPP
#define AF2_UTILITIES_MATCH_HPP

#include "overloaded.hpp" // util::Overloaded

#include <utility> // std::forward
#include <variant> // std::visit

namespace util {

namespace detail {

template <typename Variant>
struct Matcher {
	Variant variant;

	template <typename... Functors>
	constexpr auto operator()(Functors&&... functors) const -> decltype(auto) {
		return std::visit(util::Overloaded{std::forward<Functors>(functors)...}, variant);
	}
};

} // namespace detail

template <typename Variant>
[[nodiscard]] constexpr auto match(Variant&& variant) -> detail::Matcher<Variant> {
	return detail::Matcher<Variant>{std::forward<Variant>(variant)};
}

} // namespace util

#endif
