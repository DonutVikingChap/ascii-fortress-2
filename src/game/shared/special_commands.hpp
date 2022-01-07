#ifndef AF2_SHARED_SPECIAL_COMMANDS_HPP
#define AF2_SHARED_SPECIAL_COMMANDS_HPP

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace cmd {

constexpr auto MAX_SIZE = std::size_t{16};

constexpr auto NOCLIP = std::string_view{"noclip"};
constexpr auto HURTME = std::string_view{"hurtme"};
constexpr auto ROCK_THE_VOTE = std::string_view{"rtv"};
constexpr auto MESSAGE_OF_THE_DAY = std::string_view{"motd"};
constexpr auto POINTS = std::string_view{"points"};
constexpr auto LEVEL = std::string_view{"level"};
constexpr auto HATS = std::string_view{"hats"};

constexpr auto ALL = std::array{
	NOCLIP,
	HURTME,
	ROCK_THE_VOTE,
	MESSAGE_OF_THE_DAY,
	POINTS,
	LEVEL,
	HATS,
};

} // namespace cmd

#endif
