#ifndef AF2_META_META_SERVER_MESSAGES_HPP
#define AF2_META_META_SERVER_MESSAGES_HPP

#include "../../network/message.hpp"     // net::MessageDirection, net::...Message
#include "../../utilities/type_list.hpp" // util::TypeList

#include <tuple> // std::tie

namespace msg {
namespace meta {
namespace sv {

template <net::MessageDirection DIR>
struct Heartbeat final : net::SecretMessage<Heartbeat<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct GameServerAddressListRequest final : net::SecretMessage<GameServerAddressListRequest<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

namespace in {

using Heartbeat = msg::meta::sv::Heartbeat<net::MessageDirection::INPUT>;
using GameServerAddressListRequest = msg::meta::sv::GameServerAddressListRequest<net::MessageDirection::INPUT>;

} // namespace in

namespace out {

using Heartbeat = msg::meta::sv::Heartbeat<net::MessageDirection::OUTPUT>;
using GameServerAddressListRequest = msg::meta::sv::GameServerAddressListRequest<net::MessageDirection::OUTPUT>;

} // namespace out

} // namespace sv
} // namespace meta
} // namespace msg

template <net::MessageDirection DIR>
using MetaServerMessages = util::TypeList<           //
	msg::meta::sv::Heartbeat<DIR>,                   // 0
	msg::meta::sv::GameServerAddressListRequest<DIR> // 1
	>;

using MetaServerInputMessages = MetaServerMessages<net::MessageDirection::INPUT>;
using MetaServerOutputMessages = MetaServerMessages<net::MessageDirection::OUTPUT>;

#endif
