#ifndef AF2_UTILITIES_INTEGER_HPP
#define AF2_UTILITIES_INTEGER_HPP

#include <cassert>     // assert
#include <climits>     // CHAR_BIT
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int..._t, std::uint..._t
#include <limits>      // std::numeric_limits
#include <type_traits> // std::conditional_t, std::is_integral_v, std::is_signed_v, std::common_type_t, std::make_unsigned_t, std::enable_if_t

namespace util {

// clang-format off
template <std::size_t BITS>
using int_t = std::conditional_t<
	BITS <= 8, std::int8_t, std::conditional_t<
	BITS <= 16, std::int16_t, std::conditional_t<
	BITS <= 32, std::int32_t, std::conditional_t<
	BITS <= 64, std::int64_t, void>>>
>;

template <std::size_t BITS>
using uint_t = std::conditional_t<
	BITS <= 8, std::uint8_t, std::conditional_t<
	BITS <= 16, std::uint16_t, std::conditional_t<
	BITS <= 32, std::uint32_t, std::conditional_t<
	BITS <= 64, std::uint64_t, void>>>
>;

template <std::size_t SIZE>
using sized_int_t = std::conditional_t<
	SIZE <= 1, std::int8_t, std::conditional_t<
	SIZE <= 2, std::int16_t, std::conditional_t<
	SIZE <= 4, std::int32_t, std::conditional_t<
	SIZE <= 8, std::int64_t, void>>>
>;

template <std::size_t SIZE>
using sized_uint_t = std::conditional_t<
	SIZE <= 1, std::uint8_t, std::conditional_t<
	SIZE <= 2, std::uint16_t, std::conditional_t<
	SIZE <= 4, std::uint32_t, std::conditional_t<
	SIZE <= 8, std::uint64_t, void>>>
>;
// clang-format on

template <typename T>
[[nodiscard]] constexpr auto setBit(T number, sized_uint_t<sizeof(T)> bit) noexcept -> T {
	assert(bit < std::numeric_limits<T>::digits);
	return number | (T{1} << bit);
}

template <typename T>
[[nodiscard]] constexpr auto clearBit(T number, sized_uint_t<sizeof(T)> bit) noexcept -> T {
	assert(bit < std::numeric_limits<T>::digits);
	return number & ~(T{1} << bit);
}

template <typename T>
[[nodiscard]] constexpr auto toggleBit(T number, sized_uint_t<sizeof(T)> bit) noexcept -> T {
	assert(bit < std::numeric_limits<T>::digits);
	return number ^ (T{1} << bit);
}

template <typename T>
[[nodiscard]] constexpr auto checkBit(T number, sized_uint_t<sizeof(T)> bit) noexcept -> bool {
	assert(bit < std::numeric_limits<T>::digits);
	return ((number >> bit) & T{1}) != 0;
}

template <typename T>
[[nodiscard]] constexpr auto setBitValue(T number, sized_uint_t<sizeof(T)> bit, bool value) noexcept -> T {
	assert(bit < std::numeric_limits<T>::digits);
	return number ^ ((-((value) ? T{1} : T{0}) ^ number) & (T{1} << bit));
}

template <typename T>
[[nodiscard]] constexpr auto rotateBitsLeft(T value, unsigned count) noexcept -> T {
	using Promoted = std::make_unsigned_t<std::common_type_t<int, T>>;

	static_assert(std::is_integral_v<T>, "Cannot rotate non-integer types.");
	static_assert(!std::is_signed_v<T>, "Cannot rotate signed integer types.");
	constexpr auto bits = static_cast<unsigned>(std::numeric_limits<T>::digits);
	static_assert((bits & (bits - 1)) == 0, "Value bits must be a power of 2.");
	constexpr auto mask = bits - 1;

	const auto mb = count & mask;
	const auto promoted = static_cast<Promoted>(value);
	return static_cast<T>((promoted << mb) | (promoted >> ((-mb) & mask)));
}

template <typename T>
[[nodiscard]] constexpr auto rotateBitsRight(T value, unsigned count) noexcept -> T {
	using Promoted = std::make_unsigned_t<std::common_type_t<int, T>>;

	static_assert(std::is_integral_v<T>, "Cannot rotate non-integer types.");
	static_assert(!std::is_signed_v<T>, "Cannot rotate signed integer types.");
	constexpr auto bits = static_cast<unsigned>(std::numeric_limits<T>::digits);
	static_assert((bits & (bits - 1)) == 0, "Value bits must be a power of 2.");
	constexpr auto mask = bits - 1;

	const auto mb = count & mask;
	const auto promoted = static_cast<Promoted>(value);
	return static_cast<T>((promoted >> mb) | (promoted << ((-mb) & mask)));
}

template <typename T>
[[nodiscard]] constexpr auto countSetBits(T value) noexcept -> std::size_t {
	auto count = std::size_t{0};
	while (value != T{0}) {
		if (value & T{1}) {
			++count;
		}
		value >>= 1;
	}
	return count;
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr auto ceil2(T number) noexcept -> T {
	--number;
	for (auto i = std::size_t{1}; i < sizeof(number) * CHAR_BIT; i *= 2) {
		number |= number >> i;
	}
	++number;
	return number;
}

static_assert(sizeof(int_t<0>) == 1);
static_assert(sizeof(int_t<32>) == 4);
static_assert(sizeof(int_t<31>) == 4);
static_assert(sizeof(int_t<8>) == 1);
static_assert(sizeof(int_t<16>) == 2);
static_assert(sizeof(int_t<13>) == 2);
static_assert(sizeof(int_t<33>) == 8);
static_assert(sizeof(int_t<64>) == 8);

static_assert(sizeof(uint_t<0>) == 1);
static_assert(sizeof(uint_t<32>) == 4);
static_assert(sizeof(uint_t<31>) == 4);
static_assert(sizeof(uint_t<8>) == 1);
static_assert(sizeof(uint_t<16>) == 2);
static_assert(sizeof(uint_t<13>) == 2);
static_assert(sizeof(uint_t<33>) == 8);
static_assert(sizeof(uint_t<64>) == 8);

static_assert(sizeof(sized_int_t<0>) == 1);
static_assert(sizeof(sized_int_t<1>) == 1);
static_assert(sizeof(sized_int_t<2>) == 2);
static_assert(sizeof(sized_int_t<3>) == 4);
static_assert(sizeof(sized_int_t<4>) == 4);
static_assert(sizeof(sized_int_t<5>) == 8);
static_assert(sizeof(sized_int_t<8>) == 8);

static_assert(sizeof(sized_uint_t<0>) == 1);
static_assert(sizeof(sized_uint_t<1>) == 1);
static_assert(sizeof(sized_uint_t<2>) == 2);
static_assert(sizeof(sized_uint_t<3>) == 4);
static_assert(sizeof(sized_uint_t<4>) == 4);
static_assert(sizeof(sized_uint_t<5>) == 8);
static_assert(sizeof(sized_uint_t<8>) == 8);

static_assert(ceil2(-123) == 0);
static_assert(ceil2(-4) == 0);
static_assert(ceil2(-3) == 0);
static_assert(ceil2(-2) == 0);
static_assert(ceil2(-1) == 0);
static_assert(ceil2(0) == 0);
static_assert(ceil2(1) == 1);
static_assert(ceil2(2) == 2);
static_assert(ceil2(3) == 4);
static_assert(ceil2(4) == 4);
static_assert(ceil2(5) == 8);
static_assert(ceil2(6) == 8);
static_assert(ceil2(7) == 8);
static_assert(ceil2(8) == 8);
static_assert(ceil2(9) == 16);
static_assert(ceil2(123) == 128);
static_assert(ceil2(0u) == 0u);
static_assert(ceil2(1u) == 1u);
static_assert(ceil2(2u) == 2u);
static_assert(ceil2(3u) == 4u);
static_assert(ceil2(4u) == 4u);
static_assert(ceil2(5u) == 8u);
static_assert(ceil2(6u) == 8u);
static_assert(ceil2(7u) == 8u);
static_assert(ceil2(8u) == 8u);
static_assert(ceil2(9u) == 16u);
static_assert(ceil2(123u) == 128u);
static_assert(ceil2(std::uint32_t{4294967295}) == std::uint32_t{0});

} // namespace util

#endif
