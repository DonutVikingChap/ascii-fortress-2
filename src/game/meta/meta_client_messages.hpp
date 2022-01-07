#ifndef AF2_META_META_CLIENT_MESSAGES_HPP
#define AF2_META_META_CLIENT_MESSAGES_HPP

#include "../../network/endpoint.hpp"       // net::IpEndpoint
#include "../../network/message.hpp"        // net::MessageDirection, net::...Message
#include "../../network/message_layout.hpp" // net::List, net::String
#include "../../utilities/type_list.hpp"    // util::TypeList
#include "../data/tickrate.hpp"             // Tickrate

#include <cstdint> // std::uint32_t
#include <tuple>   // std::tie

namespace msg {
namespace meta {
namespace cl {

template <net::MessageDirection DIR>
struct GameServerAddressList final : net::SecretMessage<GameServerAddressList<DIR>, DIR> {
	net::List<net::IpEndpoint, DIR> endpoints{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(endpoints);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(endpoints);
	}
};

template <net::MessageDirection DIR>
struct MetaInfo final : net::SecretMessage<MetaInfo<DIR>, DIR> {
	Tickrate tickrate = 0;
	std::uint32_t playerCount = 0;
	std::uint32_t botCount = 0;
	std::uint32_t maxPlayerCount = 0;
	net::String<DIR> mapName{};
	net::String<DIR> hostName{};
	net::String<DIR> gameVersion{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(tickrate, playerCount, botCount, maxPlayerCount, mapName, hostName, gameVersion);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(tickrate, playerCount, botCount, maxPlayerCount, mapName, hostName, gameVersion);
	}
};

namespace in {

using GameServerAddressList = msg::meta::cl::GameServerAddressList<net::MessageDirection::INPUT>;
using MetaInfo = msg::meta::cl::MetaInfo<net::MessageDirection::INPUT>;

} // namespace in

namespace out {

using GameServerAddressList = msg::meta::cl::GameServerAddressList<net::MessageDirection::OUTPUT>;
using MetaInfo = msg::meta::cl::MetaInfo<net::MessageDirection::OUTPUT>;

} // namespace out

} // namespace cl
} // namespace meta
} // namespace msg

template <net::MessageDirection DIR>
using MetaClientMessages = util::TypeList<     //
	msg::meta::cl::GameServerAddressList<DIR>, // 0
	msg::meta::cl::MetaInfo<DIR>               // 1
	>;

using MetaClientInputMessages = MetaClientMessages<net::MessageDirection::INPUT>;
using MetaClientOutputMessages = MetaClientMessages<net::MessageDirection::OUTPUT>;

#endif
