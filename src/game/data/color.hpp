#ifndef AF2_DATA_COLOR_HPP
#define AF2_DATA_COLOR_HPP

#include <algorithm>   // std::min, std::max
#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <functional>  // std::hash<std::uint32_t>
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <tuple>       // std::make_tuple

// clang-format off
// X(name, str, r, g, b, a)
#define ENUM_COLORS( X ) \
X(transparent,		"Transparent",	0,		0,		0,		0) \
X(black,			"Black",		0,		0,		0,		255) \
X(white,			"White",		255,	255,	255,	255) \
X(red,				"Red",			255,	0,		0,		255) \
X(lime,				"Lime",			0,		255,	0,		255) \
X(blue,				"Blue",			0,		0,		255,	255) \
X(yellow,			"Yellow",		255,	255,	0,		255) \
X(magenta,			"Magenta",		255,	0,		255,	255) \
X(cyan,				"Cyan",			0,		255,	255,	255) \
X(silver,			"Silver",		192,	192,	192,	255) \
X(gray,				"Gray",			128,	128,	128,	255) \
X(maroon,			"Maroon",		128,	0,		0,		255) \
X(olive,			"Olive",		128,	128,	0,		255) \
X(green,			"Green",		0,		128,	0,		255) \
X(purple,			"Purple",		128,	0,		128,	255) \
X(teal,				"Teal",			0,		128,	128,	255) \
X(navy,				"Navy",			0,		0,		128,	255) \
X(dim_gray,			"Dim Gray",		105,	105,	105,	255) \
X(dark_gray,		"Dark Gray",	64,		64,		64,		255) \
X(dark_red,			"Dark Red",		139,	0,		0,		255) \
X(orange,			"Orange",		255,	165,	0,		255) \
X(dark_orange,		"Dark Orange",	255,	140,	0,		255) \
X(gold,				"Gold",			255,	215,	0,		255) \
X(dark_green,		"Dark Green",	0,		100,	0,		255) \
X(turquoise,		"Turquoise",	64,		224,	208,	255) \
X(dark_blue,		"Dark Blue",	0,		0,		139,	255) \
X(violet,			"Violet",		238,	130,	238,	255) \
X(indigo,			"Indigo",		75,		0,		130,	255) \
X(pink,				"Pink",			255,	192,	203,	255) \
X(hot_pink,			"Hot Pink",		255,	105,	180,	255) \
X(deep_pink,		"Deep Pink",	255,	20,		147,	255) \
X(light_blue,		"Light Blue",	173,	216,	230,	255) \
X(light_green,		"Light Green",	144,	238,	144,	255) \
X(light_gray,		"Light Gray",	211,	211,	211,	255) \
X(light_yellow,		"Light Yellow",	255,	255,	224,	255) \
X(brown,			"Brown",		165,	42,		42,		255) \
X(dark_brown,		"Dark Brown",	139,	69,		19,		255) \
X(team_red,			"Team Red",		255,	64,		64,		255) \
X(team_blue,		"Team Blue",	64,		64,		255,	255) \
X(team_gray,		"Team Gray",	100,	100,	100,	255)
// clang-format on

struct Color final {
#define X(name, str, r, g, b, a) \
	[[nodiscard]] static constexpr auto name() noexcept { \
		return Color{r, g, b, a}; \
	}
	ENUM_COLORS(X)
#undef X

	static auto parse(std::string_view str) noexcept -> std::optional<Color>;

	constexpr Color() noexcept = default;

	constexpr Color(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255) noexcept
		: r(red)
		, g(green)
		, b(blue)
		, a(alpha) {}

	constexpr explicit Color(std::uint32_t integer) noexcept
		: r((integer & 0xFF000000) >> 24)
		, g((integer & 0x00FF0000) >> 16)
		, b((integer & 0x0000FF00) >> 8)
		, a(integer & 0x000000FF) {}

	constexpr explicit operator std::uint32_t() const noexcept {
		return (static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) | (static_cast<std::uint32_t>(b) << 8) |
			   static_cast<std::uint32_t>(a);
	}

	[[nodiscard]] friend constexpr auto operator==(Color lhs, Color rhs) noexcept -> bool {
		return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
	}

	[[nodiscard]] friend constexpr auto operator!=(Color lhs, Color rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] friend constexpr auto operator<(Color lhs, Color rhs) noexcept -> bool {
		return std::make_tuple(lhs.r, lhs.g, lhs.b, lhs.a) < std::make_tuple(rhs.r, rhs.g, rhs.b, rhs.a);
	}

	[[nodiscard]] friend constexpr auto operator<=(Color lhs, Color rhs) noexcept -> bool {
		return !(rhs < lhs);
	}

	[[nodiscard]] friend constexpr auto operator>(Color lhs, Color rhs) noexcept -> bool {
		return rhs < lhs;
	}

	[[nodiscard]] friend constexpr auto operator>=(Color lhs, Color rhs) noexcept -> bool {
		return !(lhs < rhs);
	}

	[[nodiscard]] friend constexpr auto operator+(Color lhs, Color rhs) noexcept -> Color {
		return {
			static_cast<std::uint8_t>(std::min(static_cast<int>(lhs.r) + rhs.r, 0)),
			static_cast<std::uint8_t>(std::min(static_cast<int>(lhs.g) + rhs.g, 0)),
			static_cast<std::uint8_t>(std::min(static_cast<int>(lhs.b) + rhs.b, 0)),
			static_cast<std::uint8_t>(std::min(static_cast<int>(lhs.a) + rhs.a, 0)),
		};
	}

	[[nodiscard]] friend constexpr auto operator-(Color lhs, Color rhs) noexcept -> Color {
		return {
			static_cast<std::uint8_t>(std::max(static_cast<int>(lhs.r) - rhs.r, 0)),
			static_cast<std::uint8_t>(std::max(static_cast<int>(lhs.g) - rhs.g, 0)),
			static_cast<std::uint8_t>(std::max(static_cast<int>(lhs.b) - rhs.b, 0)),
			static_cast<std::uint8_t>(std::max(static_cast<int>(lhs.a) - rhs.a, 0)),
		};
	}

	[[nodiscard]] friend constexpr auto operator*(Color lhs, Color rhs) noexcept -> Color {
		return {
			static_cast<std::uint8_t>(static_cast<int>(lhs.r) * rhs.r / 255),
			static_cast<std::uint8_t>(static_cast<int>(lhs.g) * rhs.g / 255),
			static_cast<std::uint8_t>(static_cast<int>(lhs.b) * rhs.b / 255),
			static_cast<std::uint8_t>(static_cast<int>(lhs.a) * rhs.a / 255),
		};
	}

	auto operator+=(Color other) noexcept -> Color& {
		return *this = *this + other;
	}

	auto operator-=(Color other) noexcept -> Color& {
		return *this = *this - other;
	}

	auto operator*=(Color other) noexcept -> Color& {
		return *this = *this * other;
	}

	[[nodiscard]] static constexpr auto getAll() noexcept {
		return std::array{
#define X(name, str, r, g, b, a) Color::name(),
			ENUM_COLORS(X)
#undef X
		};
	}

	[[nodiscard]] constexpr auto getName() const noexcept {
		switch (static_cast<std::uint32_t>(*this)) {
#define X(name, str, r, g, b, a) \
	case static_cast<std::uint32_t>(Color::name()): return std::string_view{str};
			ENUM_COLORS(X)
#undef X
		}
		return std::string_view{};
	}

	[[nodiscard]] constexpr auto getCodeName() const noexcept {
		switch (static_cast<std::uint32_t>(*this)) {
#define X(name, str, r, g, b, a) \
	case static_cast<std::uint32_t>(Color::name()): return std::string_view{#name};
			ENUM_COLORS(X)
#undef X
		}
		return std::string_view{};
	}

	[[nodiscard]] auto getString() const -> std::string;

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, Color color) -> Stream& {
		return stream << color.r << color.g << color.b << color.a;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, Color& color) -> Stream& {
		return stream >> color.r >> color.g >> color.b >> color.a;
	}

	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;
	std::uint8_t a = 255;
};

#undef ENUM_COLORS

template <>
class std::hash<Color> {
public:
	auto operator()(const Color& color) const -> std::size_t {
		return m_hasher(static_cast<std::uint32_t>(color));
	}

private:
	std::hash<std::uint32_t> m_hasher{};
};

#endif
