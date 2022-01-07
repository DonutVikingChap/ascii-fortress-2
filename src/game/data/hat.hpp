#ifndef AF2_DATA_HAT_HPP
#define AF2_DATA_HAT_HPP

#include "../../debug.hpp" // Msg, DEBUG_MSG
#include "color.hpp"       // Color
#include "data_type.hpp"   // DataType

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <functional>  // std::hash<Hat::ValueType>
#include <string_view> // std::string_view

// clang-format off
// X(name, str, ch, color)
#define ENUM_HATS(X) \
	X(none,						"",								'\0',	Color::transparent()) \
	X(ghastly_gibus,			"Ghastly Gibus",				'G',	Color::dark_gray()) \
	X(troublemakers_tossle_cap,	"Troublemaker's Tossle Cap",	'a',	Color::lime()) \
	X(towering_pillar_of_hats,	"Towering Pillar of Hats",		't',	Color::light_yellow()) \
	X(scotsmans_stove_pipe,		"Scotsman's Stove Pipe",		't',	Color::gray()) \
	X(glengarry_bonnet,			"Glengarry Bonnet",				'i',	Color::purple()) \
	X(party_hat,				"Party Hat",					'^',	Color::purple()) \
	X(charmers_chapeau,			"Charmer's Chapeau",			'~',	Color::purple()) \
	X(batters_helmet,			"Batter's Helmet",				'b',	Color::gray()) \
	X(officers_ushanka,			"Officer's Ushanka",			'w',	Color::dark_gray()) \
	X(potassium_bonnet,			"Potassium Bonnet",				'J',	Color::yellow()) \
	X(killers_kabuto,			"Killer's Kabuto",				'V',	Color::gold()) \
	X(triboniophorus_tyrannus,	"Triboniophorus Tyrannus",		'n',	Color::lime()) \
	X(vintage_tyrolean,			"Vintage Tyrolean",				'h',	Color::brown()) \
	X(anger,					"Anger",						'A',	Color::deep_pink()) \
	X(modest_pile_of_hat,		"Modest Pile of Hat",			'o',	Color::gray()) \
	X(a_rather_festive_tree,	"A Rather Festive Tree",		'A',	Color::green()) \
	X(boxcar_bomber,			"Boxcar Bomber",				'm',	Color::dark_brown()) \
	X(ellis_cap,				"Ellis' Cap",					'L',	Color::light_gray()) \
	X(texas_ten_gallon,			"Texas Ten Gallon",				'u',	Color::dark_brown())
// clang-format on

class Hat final : public DataType<Hat, std::uint8_t> {
private:
	using DataType::DataType;

	static constexpr auto NAMES = std::array{
#define X(name, str, ch, color) std::string_view{str},
		ENUM_HATS(X)
#undef X
	};

	static constexpr auto CHARS = std::array{
#define X(name, str, ch, color) ch,
		ENUM_HATS(X)
#undef X
	};

	static constexpr auto COLORS = std::array{
#define X(name, str, ch, color) color,
		ENUM_HATS(X)
#undef X
	};

public:
	enum class Index : ValueType {
#define X(name, str, ch, color) name,
		ENUM_HATS(X)
#undef X
	};

	constexpr operator Index() const noexcept {
		return static_cast<Index>(m_value);
	}

#define X(name, str, ch, color) \
	[[nodiscard]] static constexpr auto name() noexcept { \
		return Hat{static_cast<ValueType>(Index::name)}; \
	}
	ENUM_HATS(X)
#undef X

	constexpr Hat() noexcept
		: Hat(Hat::none()) {}

	[[nodiscard]] static constexpr auto getAll() noexcept {
		return std::array{
#define X(name, str, ch, color) Hat::name(),
			ENUM_HATS(X)
#undef X
		};
	}

	[[nodiscard]] constexpr auto getName() const noexcept {
		return NAMES[m_value];
	}

	[[nodiscard]] constexpr auto getChar() const noexcept {
		return CHARS[m_value];
	}

	[[nodiscard]] constexpr auto getColor() const noexcept {
		return COLORS[m_value];
	}

	[[nodiscard]] constexpr auto getId() const noexcept {
		return m_value;
	}

	[[nodiscard]] auto getDropWeight() const noexcept -> float;

	[[nodiscard]] static auto findByName(std::string_view inputName) noexcept -> Hat;
	[[nodiscard]] static auto findById(ValueType id) noexcept -> Hat;

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const Hat& val) -> Stream& {
		return stream << val.m_value;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, Hat& val) -> Stream& {
		constexpr auto size = Hat::getAll().size();
		if (!(stream >> val.m_value) || val.m_value >= size) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Read invalid Hat value \"{}\".", val.m_value);
			val.m_value = 0;
			stream.invalidate();
		}
		return stream;
	}
};

#ifndef SHARED_HAT_KEEP_ENUM_HATS
#undef ENUM_HATS
#endif

template <>
class std::hash<Hat> {
public:
	auto operator()(const Hat& id) const -> std::size_t {
		return m_hasher(id.value());
	}

private:
	std::hash<Hat::ValueType> m_hasher{};
};

#endif
