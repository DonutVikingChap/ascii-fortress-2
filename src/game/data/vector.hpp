#ifndef AF2_DATA_VECTOR_HPP
#define AF2_DATA_VECTOR_HPP

#include <cmath>       // std::hypot, std::atan2
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <cstdint>     // std::int16_t
#include <functional>  // std::hash<std::int16_t>
#include <type_traits> // std::bool_constant, std::is_same_v, std::remove_cv_t, std::remove_reference_t, std::enable_if_t

namespace detail {

template <typename T, typename Scalar>
struct is_scalar
	: std::bool_constant<std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, Scalar> || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, int>> {
};

template <typename T, typename Scalar>
inline constexpr auto is_scalar_v = detail::is_scalar<T, Scalar>::value;

} // namespace detail

template <typename T>
struct Vector2 final {
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using reference = T&;
	using const_reference = T&;
	using pointer = T*;
	using const_pointer = const T*;

	using Length = T;

	T x{};
	T y{};

	constexpr Vector2() noexcept = default;

	template <typename X, typename Y, typename = std::enable_if_t<detail::is_scalar_v<X, T> && detail::is_scalar_v<Y, T>>>
	constexpr Vector2(X x, Y y) noexcept
		: x(static_cast<T>(x))
		, y(static_cast<T>(y)) {}

	template <typename U>
	constexpr explicit operator Vector2<U>() const noexcept {
		return Vector2<U>{static_cast<U>(x), static_cast<U>(y)};
	}

	[[nodiscard]] friend constexpr auto operator==(Vector2 lhs, Vector2 rhs) noexcept -> bool {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	[[nodiscard]] friend constexpr auto operator!=(Vector2 lhs, Vector2 rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] constexpr auto size() const noexcept -> size_type {
		return 2;
	}

	[[nodiscard]] constexpr auto operator[](size_type i) noexcept -> reference {
		if (i % 2 == 0) {
			return x;
		}
		return y;
	}

	[[nodiscard]] constexpr auto operator[](size_type i) const noexcept -> const_reference {
		if (i % 2 == 0) {
			return x;
		}
		return y;
	}

	[[nodiscard]] constexpr auto lengthSquared() const noexcept {
		return x * x + y * y;
	}

	[[nodiscard]] constexpr auto length() const noexcept {
		return std::hypot(x, y);
	}

	[[nodiscard]] constexpr auto angle() const noexcept {
		return std::atan2(y, x);
	}

	[[nodiscard]] constexpr auto normalized() const noexcept -> Vector2 {
		return *this / static_cast<T>(this->length());
	}

	constexpr auto normalize() noexcept -> void {
		*this /= static_cast<T>(this->length());
	}

	constexpr auto operator+=(Vector2 rhs) noexcept -> Vector2& {
		return *this = *this + rhs;
	}

	constexpr auto operator-=(Vector2 rhs) noexcept -> Vector2& {
		return *this = *this - rhs;
	}

	template <typename U, typename = std::enable_if_t<detail::is_scalar_v<U, T>>>
	constexpr auto operator*=(U rhs) noexcept -> Vector2& {
		return *this = *this * rhs;
	}

	template <typename U, typename = std::enable_if_t<detail::is_scalar_v<U, T>>>
	constexpr auto operator/=(U rhs) noexcept -> Vector2& {
		return *this = *this / rhs;
	}

	[[nodiscard]] constexpr auto operator-() const noexcept -> Vector2 {
		return Vector2{static_cast<T>(-x), static_cast<T>(-y)};
	}

	[[nodiscard]] friend constexpr auto operator+(Vector2 lhs, Vector2 rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs.x + rhs.x), static_cast<T>(lhs.y + rhs.y)};
	}

	[[nodiscard]] friend constexpr auto operator-(Vector2 lhs, Vector2 rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs.x - rhs.x), static_cast<T>(lhs.y - rhs.y)};
	}

	[[nodiscard]] friend constexpr auto operator*(Vector2 lhs, Vector2 rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs.x * rhs.x), static_cast<T>(lhs.y * rhs.y)};
	}

	[[nodiscard]] friend constexpr auto operator/(Vector2 lhs, Vector2 rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs.x / rhs.x), static_cast<T>(lhs.y / rhs.y)};
	}

	template <typename U, typename = std::enable_if_t<detail::is_scalar_v<U, T>>>
	[[nodiscard]] friend constexpr auto operator*(Vector2 lhs, U rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs.x * rhs), static_cast<T>(lhs.y * rhs)};
	}

	template <typename U, typename = std::enable_if_t<detail::is_scalar_v<U, T>>>
	[[nodiscard]] friend constexpr auto operator*(U lhs, Vector2 rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs * rhs.x), static_cast<T>(lhs * rhs.y)};
	}

	template <typename U, typename = std::enable_if_t<detail::is_scalar_v<U, T>>>
	[[nodiscard]] friend constexpr auto operator/(Vector2 lhs, U rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs.x / rhs), static_cast<T>(lhs.y / rhs)};
	}

	template <typename U, typename = std::enable_if_t<detail::is_scalar_v<U, T>>>
	[[nodiscard]] friend constexpr auto operator/(U lhs, Vector2 rhs) noexcept -> Vector2 {
		return Vector2{static_cast<T>(lhs / rhs.x), static_cast<T>(lhs / rhs.y)};
	}

	[[nodiscard]] static constexpr auto dotProduct(Vector2 lhs, Vector2 rhs) noexcept {
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}

	[[nodiscard]] static constexpr auto determinant(Vector2 lhs, Vector2 rhs) noexcept {
		return lhs.x * rhs.y - lhs.y * rhs.x;
	}

	[[nodiscard]] static constexpr auto angle(Vector2 lhs, Vector2 rhs) noexcept {
		return std::atan2(Vector2::determinant(lhs, rhs), Vector2::dotProduct(lhs, rhs));
	}

	[[nodiscard]] static constexpr auto distanceSquared(Vector2 lhs, Vector2 rhs) noexcept {
		return (rhs - lhs).lengthSquared();
	}

	[[nodiscard]] static constexpr auto distance(Vector2 lhs, Vector2 rhs) noexcept {
		return (rhs - lhs).length();
	}

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const Vector2& val) -> Stream& {
		return stream << val.x << val.y;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, Vector2& val) -> Stream& {
		return stream >> val.x >> val.y;
	}
};

using Vec2 = Vector2<std::int16_t>;

template <>
class std::hash<Vec2> {
public:
	auto operator()(const Vec2& vector) const -> std::size_t {
		return (53 + m_componentHasher(vector.x)) * 53 + m_componentHasher(vector.y);
	}

private:
	std::hash<Vec2::Length> m_componentHasher{};
};

#endif
