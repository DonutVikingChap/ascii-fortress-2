#ifndef AF2_DATA_DIRECTION_HPP
#define AF2_DATA_DIRECTION_HPP

#include "../../utilities/math.hpp" // util::constSqrt
#include "data_type.hpp"            // DataType
#include "vector.hpp"               // Vector2

#include <cmath>       // std::abs
#include <cstdint>     // std::uint8_t
#include <type_traits> // std::is_integral_v, std::is_floating_point_v

// clang-format off
// X(name, bits)
#define ENUM_DIRECTIONS(X) \
	X(none,		0) \
	X(left,		1 << 0) \
	X(right,	1 << 1) \
	X(up,		1 << 2) \
	X(down,		1 << 3)
// clang-format on

class Direction final : public DataType<Direction, std::uint8_t> {
private:
	template <typename T>
	static constexpr auto DIAGONAL_RATIO = static_cast<T>(util::constSqrt(2.0) - 1.0);

	using DataType::DataType;

public:
#define X(name, bits) \
	[[nodiscard]] static constexpr auto name() noexcept->Direction { \
		return Direction{(bits)}; \
	}
	ENUM_DIRECTIONS(X)
#undef X

	constexpr Direction() noexcept
		: Direction(Direction::none()) {}

	[[nodiscard]] friend constexpr auto operator|(Direction lhs, Direction rhs) noexcept -> Direction {
		return Direction{static_cast<ValueType>(lhs.m_value | rhs.m_value)};
	}

	constexpr auto operator|=(Direction other) noexcept -> Direction& {
		m_value |= other.m_value;
		return *this;
	}

	template <typename T>
	constexpr Direction(T diffX, T diffY) noexcept
		: Direction(Direction::none()) {
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
		if constexpr (std::is_integral_v<T>) {
			if (std::abs(static_cast<float>(diffX) / static_cast<float>(diffY)) > DIAGONAL_RATIO<float>) {
				*this |= ((diffX < 0) ? Direction::left() : Direction::right());
			}
			if (std::abs(static_cast<float>(diffY) / static_cast<float>(diffX)) > DIAGONAL_RATIO<float>) {
				*this |= ((diffY < 0) ? Direction::up() : Direction::down());
			}
		} else {
			if (std::abs(diffX / diffY) > DIAGONAL_RATIO<T>) {
				*this |= ((diffX < 0) ? Direction::left() : Direction::right());
			}
			if (std::abs(diffY / diffX) > DIAGONAL_RATIO<T>) {
				*this |= ((diffY < 0) ? Direction::up() : Direction::down());
			}
		}
	}

	template <typename T>
	explicit Direction(Vector2<T> diff) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
		: Direction(diff.x, diff.y) {}

	constexpr Direction(bool left, bool right, bool up, bool down) noexcept
		: Direction(Direction::none()) {
		if (left) {
			*this |= Direction::left();
		}
		if (right) {
			*this |= Direction::right();
		}
		if (up) {
			*this |= Direction::up();
		}
		if (down) {
			*this |= Direction::down();
		}
	}

	[[nodiscard]] constexpr auto hasLeft() const noexcept -> bool {
		return (m_value & Direction::left().m_value) != 0;
	}

	[[nodiscard]] constexpr auto hasRight() const noexcept -> bool {
		return (m_value & Direction::right().m_value) != 0;
	}

	[[nodiscard]] constexpr auto hasUp() const noexcept -> bool {
		return (m_value & Direction::up().m_value) != 0;
	}

	[[nodiscard]] constexpr auto hasDown() const noexcept -> bool {
		return (m_value & Direction::down().m_value) != 0;
	}

	[[nodiscard]] constexpr auto hasHorizontal() const noexcept -> bool {
		return this->hasLeft() || this->hasRight();
	}

	[[nodiscard]] constexpr auto hasVertical() const noexcept -> bool {
		return this->hasUp() || this->hasDown();
	}

	[[nodiscard]] constexpr auto hasAny() const noexcept -> bool {
		return *this != Direction::none();
	}

	[[nodiscard]] constexpr auto isLeft() const noexcept -> bool {
		return this->hasLeft() && !this->hasRight();
	}

	[[nodiscard]] constexpr auto isRight() const noexcept -> bool {
		return this->hasRight() && !this->hasLeft();
	}

	[[nodiscard]] constexpr auto isUp() const noexcept -> bool {
		return this->hasUp() && !this->hasDown();
	}

	[[nodiscard]] constexpr auto isDown() const noexcept -> bool {
		return this->hasDown() && !this->hasUp();
	}

	[[nodiscard]] constexpr auto isHorizontal() const noexcept -> bool {
		return this->hasLeft() != this->hasRight();
	}

	[[nodiscard]] constexpr auto isVertical() const noexcept -> bool {
		return this->hasUp() != this->hasDown();
	}

	[[nodiscard]] constexpr auto isAny() const noexcept -> bool {
		return this->isHorizontal() || this->isVertical();
	}

	[[nodiscard]] constexpr auto getHorizontal() const noexcept -> Direction {
		return Direction{this->hasLeft(), this->hasRight(), false, false};
	}

	[[nodiscard]] constexpr auto getVertical() const noexcept -> Direction {
		return Direction{false, false, this->hasUp(), this->hasDown()};
	}

	[[nodiscard]] constexpr auto getOpposite() const noexcept -> Direction {
		return Direction{!this->hasLeft(), !this->hasRight(), !this->hasUp(), !this->hasDown()};
	}

	[[nodiscard]] static constexpr auto getX(bool left, bool right) noexcept -> Vec2::Length {
		return (left) ? (right ? Vec2::Length{0} : Vec2::Length{-1}) : (right ? Vec2::Length{1} : Vec2::Length{0});
	}

	[[nodiscard]] static constexpr auto getY(bool up, bool down) noexcept -> Vec2::Length {
		return (up) ? (down ? Vec2::Length{0} : Vec2::Length{-1}) : (down ? Vec2::Length{1} : Vec2::Length{0});
	}

	[[nodiscard]] constexpr auto getX() const noexcept -> Vec2::Length {
		return getX(this->hasLeft(), this->hasRight());
	}

	[[nodiscard]] constexpr auto getY() const noexcept -> Vec2::Length {
		return getY(this->hasUp(), this->hasDown());
	}

	[[nodiscard]] static constexpr auto getVector(bool left, bool right, bool up, bool down) noexcept -> Vec2 {
		return Vec2{getX(left, right), getY(up, down)};
	}

	[[nodiscard]] constexpr auto getVector() const noexcept -> Vec2 {
		return getVector(this->hasLeft(), this->hasRight(), this->hasUp(), this->hasDown());
	}

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const Direction& val) -> Stream& {
		return stream << val.m_value;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, Direction& val) -> Stream& {
		return stream >> val.m_value;
	}
};

#undef ENUM_DIRECTIONS

template <>
class std::hash<Direction> {
public:
	auto operator()(const Direction& direction) const -> std::size_t {
		return m_hasher(direction.value());
	}

private:
	std::hash<Direction::ValueType> m_hasher{};
};

#endif
