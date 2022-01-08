#ifndef AF2_NETWORK_MESSAGE_LAYOUT_HPP
#define AF2_NETWORK_MESSAGE_LAYOUT_HPP

#include "../utilities/span.hpp" // util::Span
#include "message.hpp"           // net::MessageDirection

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::conditional_t
#include <vector>      // std::vector

namespace net {

template <typename T, MessageDirection DIR>
using Big = std::conditional_t<DIR == MessageDirection::INPUT, //
                               T,                              //
                               const T&>;

template <MessageDirection DIR>
using String = std::conditional_t<DIR == MessageDirection::INPUT, //
                                  std::string,                    //
                                  std::string_view>;

template <typename T, std::size_t N, MessageDirection DIR>
using Array = std::conditional_t<DIR == MessageDirection::INPUT, //
                                 std::array<T, N>,               //
                                 util::Span<const T, N>>;

template <typename T, MessageDirection DIR>
using List = std::conditional_t<DIR == MessageDirection::INPUT, //
                                std::vector<T>,                 //
                                util::Span<const T>>;

} // namespace net

#endif
