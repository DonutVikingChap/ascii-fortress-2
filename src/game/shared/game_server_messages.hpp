#ifndef AF2_SHARED_GAME_SERVER_MESSAGES_HPP
#define AF2_SHARED_GAME_SERVER_MESSAGES_HPP

#include "../../network/crypto.hpp"         // crypto::...
#include "../../network/message.hpp"        // net::MessageDirection, net::...Message
#include "../../network/message_layout.hpp" // net::String, net::Big, net::List
#include "../../utilities/crc.hpp"          // util::CRC32
#include "../../utilities/type_list.hpp"    // util::TypeList
#include "../data/actions.hpp"              // Actions, Action
#include "../data/hat.hpp"                  // Hat
#include "../data/health.hpp"               // Health
#include "../data/inventory.hpp"            // InventoryId, InventoryToken, INVENTORY_ID_INVALID
#include "../data/player_class.hpp"         // PlayerClass
#include "../data/team.hpp"                 // Team
#include "../data/tick_count.hpp"           // TickCount
#include "../data/tickrate.hpp"             // Tickrate

#include <string> // std::string
#include <tuple>  // std::tie

namespace msg {
namespace sv {

/**
 * Server info request message.
 * Sent to acquire the password data required in order to connect to the server.
 */
template <net::MessageDirection DIR>
struct ServerInfoRequest final : net::ReliableMessage<ServerInfoRequest<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

/**
 * Join request message.
 * Contains client info and the key to the password we're trying to connect with.
 */
template <net::MessageDirection DIR>
struct JoinRequest final : net::SecretMessage<JoinRequest<DIR>, DIR> {
	util::CRC32 mapHash{};
	net::String<DIR> gameVersion{};
	net::String<DIR> username{};
	Tickrate updateRate = 0;
	net::Big<crypto::pw::Key, DIR> passwordKey{};
	InventoryId inventoryId = INVENTORY_ID_INVALID;
	net::Big<InventoryToken, DIR> inventoryToken{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(mapHash, gameVersion, username, updateRate, passwordKey, inventoryId, inventoryToken);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(mapHash, gameVersion, username, updateRate, passwordKey, inventoryId, inventoryToken);
	}
};

/**
 * User command message.
 * Sent periodically at a fast rate in order to update the actions that are being pressed.
 */
template <net::MessageDirection DIR>
struct UserCmd final : net::UnreliableMessage<UserCmd<DIR>, DIR> {
	TickCount number = 0;
	TickCount latestSnapshotReceived = 0;
	Actions actions = Action::NONE;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(number, latestSnapshotReceived, actions);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(number, latestSnapshotReceived, actions);
	}
};

template <net::MessageDirection DIR>
struct ChatMessage final : net::SecretMessage<ChatMessage<DIR>, DIR> {
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(message);
	}
};

template <net::MessageDirection DIR>
struct TeamChatMessage final : net::SecretMessage<TeamChatMessage<DIR>, DIR> {
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(message);
	}
};

template <net::MessageDirection DIR>
struct TeamSelect final : net::SecretMessage<TeamSelect<DIR>, DIR> {
	Team team = Team::spectators();
	PlayerClass playerClass = PlayerClass::scout();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(team, playerClass);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(team, playerClass);
	}
};

template <net::MessageDirection DIR>
struct ResourceDownloadRequest final : net::SecretMessage<ResourceDownloadRequest<DIR>, DIR> {
	util::CRC32 nameHash{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(nameHash);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(nameHash);
	}
};

template <net::MessageDirection DIR>
struct UpdateRateChange final : net::SecretMessage<UpdateRateChange<DIR>, DIR> {
	Tickrate newUpdateRate = 0;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(newUpdateRate);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(newUpdateRate);
	}
};

template <net::MessageDirection DIR>
struct UsernameChange final : net::SecretMessage<UsernameChange<DIR>, DIR> {
	net::String<DIR> newUsername{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(newUsername);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(newUsername);
	}
};

template <net::MessageDirection DIR>
struct ForwardedCommand final : net::SecretMessage<ForwardedCommand<DIR>, DIR> {
	net::List<std::string, DIR> command{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(command);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(command);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleLoginInfoRequest final : net::SecretMessage<RemoteConsoleLoginInfoRequest<DIR>, DIR> {
	net::String<DIR> username{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(username);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(username);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleLoginRequest final : net::SecretMessage<RemoteConsoleLoginRequest<DIR>, DIR> {
	net::String<DIR> username{};
	net::Big<crypto::pw::Key, DIR> passwordKey{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(username, passwordKey);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(username, passwordKey);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleCommand final : net::SecretMessage<RemoteConsoleCommand<DIR>, DIR> {
	net::String<DIR> command{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(command);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(command);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleAbortCommand final : net::SecretMessage<RemoteConsoleAbortCommand<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleLogout final : net::SecretMessage<RemoteConsoleLogout<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct InventoryEquipHatRequest final : net::SecretMessage<InventoryEquipHatRequest<DIR>, DIR> {
	Hat hat = Hat::none();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(hat);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(hat);
	}
};

template <net::MessageDirection DIR>
struct HeartbeatRequest final : net::SecretMessage<HeartbeatRequest<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct MetaInfoRequest final : net::SecretMessage<MetaInfoRequest<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

namespace in {

using ServerInfoRequest = msg::sv::ServerInfoRequest<net::MessageDirection::INPUT>;
using JoinRequest = msg::sv::JoinRequest<net::MessageDirection::INPUT>;
using UserCmd = msg::sv::UserCmd<net::MessageDirection::INPUT>;
using ChatMessage = msg::sv::ChatMessage<net::MessageDirection::INPUT>;
using TeamSelect = msg::sv::TeamSelect<net::MessageDirection::INPUT>;
using TeamChatMessage = msg::sv::TeamChatMessage<net::MessageDirection::INPUT>;
using ResourceDownloadRequest = msg::sv::ResourceDownloadRequest<net::MessageDirection::INPUT>;
using UpdateRateChange = msg::sv::UpdateRateChange<net::MessageDirection::INPUT>;
using UsernameChange = msg::sv::UsernameChange<net::MessageDirection::INPUT>;
using ForwardedCommand = msg::sv::ForwardedCommand<net::MessageDirection::INPUT>;
using RemoteConsoleLoginInfoRequest = msg::sv::RemoteConsoleLoginInfoRequest<net::MessageDirection::INPUT>;
using RemoteConsoleLoginRequest = msg::sv::RemoteConsoleLoginRequest<net::MessageDirection::INPUT>;
using RemoteConsoleCommand = msg::sv::RemoteConsoleCommand<net::MessageDirection::INPUT>;
using RemoteConsoleAbortCommand = msg::sv::RemoteConsoleAbortCommand<net::MessageDirection::INPUT>;
using RemoteConsoleLogout = msg::sv::RemoteConsoleLogout<net::MessageDirection::INPUT>;
using InventoryEquipHatRequest = msg::sv::InventoryEquipHatRequest<net::MessageDirection::INPUT>;
using HeartbeatRequest = msg::sv::HeartbeatRequest<net::MessageDirection::INPUT>;
using MetaInfoRequest = msg::sv::MetaInfoRequest<net::MessageDirection::INPUT>;

} // namespace in

namespace out {

using ServerInfoRequest = msg::sv::ServerInfoRequest<net::MessageDirection::OUTPUT>;
using JoinRequest = msg::sv::JoinRequest<net::MessageDirection::OUTPUT>;
using UserCmd = msg::sv::UserCmd<net::MessageDirection::OUTPUT>;
using ChatMessage = msg::sv::ChatMessage<net::MessageDirection::OUTPUT>;
using TeamSelect = msg::sv::TeamSelect<net::MessageDirection::OUTPUT>;
using TeamChatMessage = msg::sv::TeamChatMessage<net::MessageDirection::OUTPUT>;
using ResourceDownloadRequest = msg::sv::ResourceDownloadRequest<net::MessageDirection::OUTPUT>;
using UpdateRateChange = msg::sv::UpdateRateChange<net::MessageDirection::OUTPUT>;
using UsernameChange = msg::sv::UsernameChange<net::MessageDirection::OUTPUT>;
using ForwardedCommand = msg::sv::ForwardedCommand<net::MessageDirection::OUTPUT>;
using RemoteConsoleLoginInfoRequest = msg::sv::RemoteConsoleLoginInfoRequest<net::MessageDirection::OUTPUT>;
using RemoteConsoleLoginRequest = msg::sv::RemoteConsoleLoginRequest<net::MessageDirection::OUTPUT>;
using RemoteConsoleCommand = msg::sv::RemoteConsoleCommand<net::MessageDirection::OUTPUT>;
using RemoteConsoleAbortCommand = msg::sv::RemoteConsoleAbortCommand<net::MessageDirection::OUTPUT>;
using RemoteConsoleLogout = msg::sv::RemoteConsoleLogout<net::MessageDirection::OUTPUT>;
using InventoryEquipHatRequest = msg::sv::InventoryEquipHatRequest<net::MessageDirection::OUTPUT>;
using HeartbeatRequest = msg::sv::HeartbeatRequest<net::MessageDirection::OUTPUT>;
using MetaInfoRequest = msg::sv::MetaInfoRequest<net::MessageDirection::OUTPUT>;

} // namespace out

} // namespace sv
} // namespace msg

template <net::MessageDirection DIR>
using GameServerMessages = util::TypeList<       //
	msg::sv::ServerInfoRequest<DIR>,             // 0
	msg::sv::JoinRequest<DIR>,                   // 1
	msg::sv::UserCmd<DIR>,                       // 2
	msg::sv::ChatMessage<DIR>,                   // 3
	msg::sv::TeamSelect<DIR>,                    // 4
	msg::sv::TeamChatMessage<DIR>,               // 5
	msg::sv::ResourceDownloadRequest<DIR>,       // 6
	msg::sv::UpdateRateChange<DIR>,              // 7
	msg::sv::UsernameChange<DIR>,                // 8
	msg::sv::ForwardedCommand<DIR>,              // 9
	msg::sv::RemoteConsoleLoginInfoRequest<DIR>, // 10
	msg::sv::RemoteConsoleLoginRequest<DIR>,     // 11
	msg::sv::RemoteConsoleCommand<DIR>,          // 12
	msg::sv::RemoteConsoleAbortCommand<DIR>,     // 13
	msg::sv::RemoteConsoleLogout<DIR>,           // 14
	msg::sv::InventoryEquipHatRequest<DIR>,      // 15
	msg::sv::HeartbeatRequest<DIR>,              // 16
	msg::sv::MetaInfoRequest<DIR>                // 17
	>;

using GameServerInputMessages = GameServerMessages<net::MessageDirection::INPUT>;
using GameServerOutputMessages = GameServerMessages<net::MessageDirection::OUTPUT>;

#endif
