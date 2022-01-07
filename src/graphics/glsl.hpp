#ifndef AF2_GRAPHICS_GLSL_HPP
#define AF2_GRAPHICS_GLSL_HPP

#include <cstddef>   // std::size_t
#include <stdexcept> // std::out_of_range

namespace gfx {

template <std::size_t N, typename T>
struct vec;

template <typename T>
struct vec<2, T> final {
	T x{};
	T y{};

	[[nodiscard]] constexpr auto operator[](std::size_t i) -> T& {
		switch (i) {
			case 0: return x;
			case 1: return y;
			default: break;
		}
		throw std::out_of_range{"vec2 index out of range!"};
	}

	[[nodiscard]] constexpr auto operator[](std::size_t i) const -> const T& {
		switch (i) {
			case 0: return x;
			case 1: return y;
			default: break;
		}
		throw std::out_of_range{"vec2 index out of range!"};
	}

	[[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
		return 2;
	}
};

template <typename T>
struct vec<3, T> final {
	T x{};
	T y{};
	T z{};

	[[nodiscard]] constexpr auto operator[](std::size_t i) -> T& {
		switch (i) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: break;
		}
		throw std::out_of_range{"vec3 index out of range!"};
	}

	[[nodiscard]] constexpr auto operator[](std::size_t i) const -> const T& {
		switch (i) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: break;
		}
		throw std::out_of_range{"vec3 index out of range!"};
	}

	[[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
		return 3;
	}
};

template <typename T>
struct vec<4, T> final {
	T x{};
	T y{};
	T z{};
	T w{};

	[[nodiscard]] constexpr auto operator[](std::size_t i) -> T& {
		switch (i) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			case 3: return w;
			default: break;
		}
		throw std::out_of_range{"vec4 index out of range!"};
	}

	[[nodiscard]] constexpr auto operator[](std::size_t i) const -> const T& {
		switch (i) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			case 3: return w;
			default: break;
		}
		throw std::out_of_range{"vec4 index out of range!"};
	}

	[[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
		return 4;
	}
};

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator==(vec<N, T> lhs, vec<N, T> rhs) noexcept -> bool {
	for (auto i = std::size_t{0}; i < N; ++i) {
		if (lhs[i] != rhs[i]) {
			return false;
		}
	}
	return true;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator!=(vec<N, T> lhs, vec<N, T> rhs) noexcept -> bool {
	return !(lhs == rhs);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator-(vec<N, T> rhs) noexcept -> vec<2, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = -rhs[i];
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator+(vec<N, T> lhs, vec<N, T> rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs[i] + rhs[i];
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator-(vec<N, T> lhs, vec<N, T> rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs[i] - rhs[i];
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator*(vec<N, T> lhs, vec<N, T> rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs[i] * rhs[i];
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator/(vec<N, T> lhs, vec<N, T> rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs[i] / rhs[i];
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator*(vec<N, T> lhs, T rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs[i] * rhs;
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator*(T lhs, vec<N, T> rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs * rhs[i];
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator/(vec<N, T> lhs, T rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs[i] / rhs;
	}
	return result;
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr auto operator/(T lhs, vec<N, T> rhs) noexcept -> vec<N, T> {
	auto result = vec<N, T>{};
	for (auto i = std::size_t{0}; i < N; ++i) {
		result[i] = lhs / rhs[i];
	}
	return result;
}

template <std::size_t N, typename T>
constexpr auto operator+=(vec<N, T>& lhs, vec<N, T> rhs) noexcept -> vec<N, T>& {
	return lhs = lhs + rhs;
}

template <std::size_t N, typename T>
constexpr auto operator-=(vec<N, T>& lhs, vec<N, T> rhs) noexcept -> vec<N, T>& {
	return lhs = lhs - rhs;
}

template <std::size_t N, typename T>
constexpr auto operator*=(vec<N, T>& lhs, vec<N, T> rhs) noexcept -> vec<N, T>& {
	return lhs = lhs * rhs;
}

template <std::size_t N, typename T>
constexpr auto operator/=(vec<N, T>& lhs, vec<N, T> rhs) noexcept -> vec<N, T>& {
	return lhs = lhs / rhs;
}

template <std::size_t N, typename T>
constexpr auto operator*=(vec<N, T>& lhs, T rhs) noexcept -> vec<N, T>& {
	return lhs = lhs * rhs;
}

template <std::size_t N, typename T>
constexpr auto operator/=(vec<N, T>& lhs, T rhs) noexcept -> vec<N, T>& {
	return lhs = lhs / rhs;
}

using vec2 = vec<2, float>;
using vec3 = vec<3, float>;
using vec4 = vec<4, float>;

} // namespace gfx

#endif
