#ifndef AF2_DATA_PLAYER_ID_HPP
#define AF2_DATA_PLAYER_ID_HPP

#include <cstdint> // std::uint32_t

using PlayerId = std::uint32_t;

constexpr auto PLAYER_ID_UNCONNECTED = PlayerId{0};

#endif
