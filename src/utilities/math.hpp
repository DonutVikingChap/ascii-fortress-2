#ifndef AF2_UTILITIES_MATH_HPP
#define AF2_UTILITIES_MATH_HPP

#include <cmath>       // std::round
#include <limits>      // std::numeric_limits
#include <type_traits> // std::is_..._v

namespace util {

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
[[nodiscard]] constexpr auto sqrtNewtonRaphson(T x, T current, T previous) noexcept -> T {
	return current == previous ? current : sqrtNewtonRaphson(x, T{0.5} * (current + x / current), current);
}

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
[[nodiscard]] constexpr auto sqrtNewtonRaphson(T x) noexcept -> T {
	return x >= T{0} && x < std::numeric_limits<T>::infinity() ? sqrtNewtonRaphson(x, x, T{0}) : std::numeric_limits<T>::quiet_NaN();
}

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
[[nodiscard]] constexpr auto constSqrt(T x) noexcept -> T {
	return sqrtNewtonRaphson(x);
}

template <typename T>
[[nodiscard]] constexpr auto nearestMultiple(T number, T multiple) noexcept -> T {
	if constexpr (std::is_floating_point_v<T>) {
		return std::round(number / multiple) * multiple;
	} else if constexpr (std::is_integral_v<T>) {
		return ((number + (multiple / T{2})) / multiple) * multiple;
	}
}

} // namespace util

static_assert(util::nearestMultiple(6, 5) == 5);
static_assert(util::nearestMultiple(7, 5) == 5);
static_assert(util::nearestMultiple(8, 5) == 10);
static_assert(util::nearestMultiple(9, 5) == 10);
static_assert(util::nearestMultiple(10, 5) == 10);
static_assert(util::nearestMultiple(11, 5) == 10);
static_assert(util::nearestMultiple(12, 5) == 10);
static_assert(util::nearestMultiple(13, 5) == 15);
static_assert(util::nearestMultiple(149, 10) == 150);

#endif
