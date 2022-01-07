#ifndef AF2_DATA_RECTANGLE_HPP
#define AF2_DATA_RECTANGLE_HPP

#include "vector.hpp" // Vector2

#include <algorithm> // std::min, std::max
#include <cstdint>   // std::int32_t

template <typename T>
struct Rectangle2 final {
	using Length = T;

	T x{};
	T y{};
	T w{};
	T h{};

	constexpr Rectangle2() noexcept = default;

	template <typename X, typename Y, typename W, typename H>
	constexpr Rectangle2(X x, Y y, W w, H h) noexcept
		: x(static_cast<T>(x))
		, y(static_cast<T>(y))
		, w(static_cast<T>(w))
		, h(static_cast<T>(h)) {}

	constexpr Rectangle2(Vector2<T> position, Vector2<T> size) noexcept
		: x(position.x)
		, y(position.y)
		, w(size.x)
		, h(size.y) {}

	template <typename U>
	constexpr explicit operator Rectangle2<U>() const noexcept {
		return Rectangle2<U>{static_cast<U>(x), static_cast<U>(y), static_cast<U>(w), static_cast<U>(h)};
	}

	[[nodiscard]] friend constexpr auto operator==(const Rectangle2& lhs, const Rectangle2& rhs) noexcept -> bool {
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.w == rhs.w && lhs.h == rhs.h;
	}

	[[nodiscard]] friend constexpr auto operator!=(const Rectangle2& lhs, const Rectangle2& rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] constexpr auto getPosition() const noexcept -> Vector2<T> {
		return Vector2<T>{x, y};
	}

	[[nodiscard]] constexpr auto getSize() const noexcept -> Vector2<T> {
		return Vector2<T>{w, h};
	}

	template <typename X, typename Y>
	[[nodiscard]] constexpr auto contains(X pX, Y pY) const noexcept -> bool {
		return (pX >= x) && (pX < x + w) && (pY >= y) && (pY < y + h);
	}

	[[nodiscard]] constexpr auto contains(Vector2<T> point) const noexcept -> bool {
		return this->contains(point.x, point.y);
	}

	[[nodiscard]] constexpr auto intersects(const Rectangle2& other) const noexcept -> bool {
		const auto iLeft = std::max(x, other.x);
		const auto iTop = std::max(y, other.y);
		const auto iRight = std::min(x + w, other.x + other.w);
		const auto iBottom = std::min(y + h, other.y + other.h);

		return iLeft < iRight && iTop < iBottom;
	}

	[[nodiscard]] constexpr auto intersection(const Rectangle2& other) const noexcept -> Rectangle2 {
		const auto iLeft = std::max(x, other.x);
		const auto iTop = std::max(y, other.y);
		const auto iRight = std::min(x + w, other.x + other.w);
		const auto iBottom = std::min(y + h, other.y + other.h);

		if (iLeft < iRight && iTop < iBottom) {
			return Rectangle2{iLeft, iTop, iRight - iLeft, iBottom - iTop};
		}
		return Rectangle2{};
	}

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const Rectangle2& rect) -> Stream& {
		return stream << rect.x << rect.y << rect.w << rect.h;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, Rectangle2& rect) -> Stream& {
		return stream >> rect.x >> rect.y >> rect.w >> rect.h;
	}
};

using Rect = Rectangle2<Vec2::Length>;

#endif
