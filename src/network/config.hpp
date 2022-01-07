#ifndef AF2_NETWORK_CONFIG_HPP
#define AF2_NETWORK_CONFIG_HPP

#include <chrono>  // std::chrono::...
#include <cstddef> // std::size_t

namespace net {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

inline constexpr auto MAX_PACKET_SIZE = std::size_t{1200};
inline constexpr auto PACKET_MASK_BYTES = std::size_t{4};
inline constexpr auto PING_INTERVAL = Duration{std::chrono::seconds{1}};
inline constexpr auto CONNECT_DURATION = Duration{std::chrono::seconds{10}};
inline constexpr auto DISCONNECT_DURATION = Duration{std::chrono::seconds{3}};

inline constexpr auto MAX_CHAT_MESSAGE_LENGTH = std::size_t{256};
inline constexpr auto MAX_USERNAME_LENGTH = std::size_t{16};
inline constexpr auto MAX_SERVER_COMMAND_SIZE = std::size_t{16};

} // namespace net

#endif
