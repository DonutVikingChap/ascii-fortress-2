#ifndef AF2_DATA_TEAM_HPP
#define AF2_DATA_TEAM_HPP

#include "../../debug.hpp" // Msg, DEBUG_MSG
#include "color.hpp"       // Color
#include "data_type.hpp"   // DataType

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <functional>  // std::hash<Team::ValueType>
#include <string_view> // std::string_view

// clang-format off
// X(name, str, color)
#define ENUM_TEAMS(X) \
	X(none,			"",				Color::transparent()) \
	X(red,			"RED",			Color::team_red()) \
	X(blue,			"BLU",			Color::team_blue()) \
	X(spectators,	"Spectators",	Color::team_gray())
// clang-format on

class Team final : public DataType<Team, std::uint8_t> {
private:
	using DataType::DataType;

	static constexpr auto NAMES = std::array{
#define X(name, str, color) std::string_view{str},
		ENUM_TEAMS(X)
#undef X
	};

	static constexpr auto COLORS = std::array{
#define X(name, str, color) color,
		ENUM_TEAMS(X)
#undef X
	};

public:
	enum class Index : ValueType {
#define X(name, str, color) name,
		ENUM_TEAMS(X)
#undef X
	};

	constexpr operator Index() const noexcept {
		return static_cast<Index>(m_value);
	}

#define X(name, str, color) \
	[[nodiscard]] static constexpr auto name() noexcept { \
		return Team{static_cast<ValueType>(Index::name)}; \
	}
	ENUM_TEAMS(X)
#undef X

	constexpr Team() noexcept
		: Team(Team::none()) {}

	[[nodiscard]] static constexpr auto getAll() noexcept {
		return std::array{
#define X(name, str, color) Team::name(),
			ENUM_TEAMS(X)
#undef X
		};
	}

	[[nodiscard]] constexpr auto getName() const noexcept {
		return NAMES[m_value];
	}

	[[nodiscard]] constexpr auto getColor() const noexcept {
		return COLORS[m_value];
	}

	[[nodiscard]] constexpr auto getId() const noexcept {
		return m_value;
	}

	[[nodiscard]] constexpr auto getOppositeTeam() const noexcept -> Team {
		return (*this == Team::red()) ? Team::blue() : ((*this == Team::blue()) ? Team::red() : Team::spectators());
	}

	[[nodiscard]] static auto findByName(std::string_view inputName) noexcept -> Team;
	[[nodiscard]] static auto findById(ValueType id) noexcept -> Team;

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const Team& val) -> Stream& {
		return stream << val.m_value;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, Team& val) -> Stream& {
		constexpr auto size = Team::getAll().size();
		if (!(stream >> val.m_value) || val.m_value >= size) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Read invalid Team value \"{}\".", val.m_value);
			val.m_value = 0;
			stream.invalidate();
		}
		return stream;
	}
};

#ifndef SHARED_DATA_TEAM_KEEP_ENUM_TEAMS
#undef ENUM_TEAMS
#endif

template <>
class std::hash<Team> {
public:
	auto operator()(const Team& team) const -> std::size_t {
		return m_hasher(team.value());
	}

private:
	std::hash<Team::ValueType> m_hasher{};
};

#endif
