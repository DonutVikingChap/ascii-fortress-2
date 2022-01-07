#ifndef AF2_SHARED_GAME_CLIENT_MESSAGES_HPP
#define AF2_SHARED_GAME_CLIENT_MESSAGES_HPP

#include "../../console/command.hpp"        // cmd::...
#include "../../network/crypto.hpp"         // crypto::...
#include "../../network/message.hpp"        // net::MessageDirection, net::...Message
#include "../../network/message_layout.hpp" // net::String, net::Big, net::List
#include "../../utilities/type_list.hpp"    // util::TypeList
#include "../data/hat.hpp"                  // Hat
#include "../data/health.hpp"               // Health
#include "../data/inventory.hpp"            // InventoryId, InventoryToken, INVENTORY_ID_INVALID
#include "../data/player_class.hpp"         // PlayerClass
#include "../data/player_id.hpp"            // PlayerId
#include "../data/score.hpp"                // Score
#include "../data/sound_id.hpp"             // SoundId
#include "../data/team.hpp"                 // Team
#include "../data/tickrate.hpp"             // Tickrate
#include "../data/vector.hpp"               // Vec2
#include "convar_update.hpp"                // ConVarUpdate
#include "resource_info.hpp"                // ResourceInfo
#include "snapshot.hpp"                     // Snapshot

#include <cstddef> // std::byte
#include <cstdint> // std::uint32_t, std::uint64_t
#include <tuple>   // std::tie

namespace msg {
namespace cl {

/**
 * Server info message.
 * Contains info about the server, such as version, current player count and capacity,
 * as well as the password data required to connect to the server.
 */
template <net::MessageDirection DIR>
struct ServerInfo final : net::ReliableMessage<ServerInfo<DIR>, DIR> {
	Tickrate tickrate = 0;
	std::uint32_t playerCount = 0;
	std::uint32_t botCount = 0;
	std::uint32_t maxPlayerCount = 0;
	net::String<DIR> mapName{};
	net::String<DIR> hostName{};
	net::String<DIR> gameVersion{};
	net::Big<crypto::pw::Salt, DIR> passwordSalt{};
	crypto::pw::HashType passwordHashType{};
	net::List<ResourceInfo, DIR> resources{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(tickrate, playerCount, botCount, maxPlayerCount, mapName, hostName, gameVersion, passwordSalt, passwordHashType, resources);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(tickrate, playerCount, botCount, maxPlayerCount, mapName, hostName, gameVersion, passwordSalt, passwordHashType, resources);
	}
};

/**
 * Joined message.
 * Sent to clients to confirm that they have successfully joined the game.
 * Contains the player id, inventory info and the server's message of the day.
 */
template <net::MessageDirection DIR>
struct Joined final : net::SecretMessage<Joined<DIR>, DIR> {
	PlayerId playerId = 0;
	InventoryId inventoryId = INVENTORY_ID_INVALID;
	net::Big<InventoryToken, DIR> inventoryToken{};
	net::String<DIR> motd{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(playerId, inventoryId, inventoryToken, motd);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(playerId, inventoryId, inventoryToken, motd);
	}
};

/**
 * Full snapshot message.
 * Contains all data required in order to replicate the server's current world state
 * on the client.
 */
template <net::MessageDirection DIR>
struct Snapshot final : net::UnreliableMessage<Snapshot<DIR>, DIR> {
	net::Big<::Snapshot, DIR> snapshot{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(snapshot);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(snapshot);
	}
};

/**
 * Partial snapshot message.
 * Contains the data required in order to transform from a previously received snapshot
 * to the server's current world state.
 */
template <net::MessageDirection DIR>
struct SnapshotDelta final : net::UnreliableMessage<SnapshotDelta<DIR>, DIR> {
	TickCount source = 0;
	net::List<std::byte, DIR> data{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(source, data);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(source, data);
	}
};

/**
 * ConVar change message.
 * Sent to inform the client that one of the server's cvars has changed.
 */
template <net::MessageDirection DIR>
struct CvarMod final : net::SecretMessage<CvarMod<DIR>, DIR> {
	net::List<ConVarUpdate, DIR> cvars{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(cvars);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(cvars);
	}
};

/**
 * Server event message.
 * Sent when a server-wide event occurs and contains a message
 * that should be shown to the user.
 */
template <net::MessageDirection DIR>
struct ServerEventMessage final : net::SecretMessage<ServerEventMessage<DIR>, DIR> {
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(message);
	}
};

/**
 * Personal server event message.
 * Sent when a server event occurs specifically concerning the recipient and contains a message
 * that should be shown to the user.
 */
template <net::MessageDirection DIR>
struct ServerEventMessagePersonal final : net::SecretMessage<ServerEventMessagePersonal<DIR>, DIR> {
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(message);
	}
};

/**
 * Chat message.
 * Contains a player-sent chat message that should be shown to the user.
 */
template <net::MessageDirection DIR>
struct ChatMessage final : net::ReliableMessage<ChatMessage<DIR>, DIR> {
	PlayerId sender = 0;
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(sender, message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(sender, message);
	}
};

/**
 * Team chat message.
 * Contains a team-specific player-sent chat message that should be shown to the user.
 */
template <net::MessageDirection DIR>
struct TeamChatMessage final : net::SecretMessage<TeamChatMessage<DIR>, DIR> {
	PlayerId sender = 0;
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(sender, message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(sender, message);
	}
};

/**
 * Server chat message.
 * Contains a server-sent chat message that should be shown to the user.
 */
template <net::MessageDirection DIR>
struct ServerChatMessage final : net::ReliableMessage<ServerChatMessage<DIR>, DIR> {
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(message);
	}
};

/**
 * Team select request.
 * Sent to inform the client that the user needs to choose a team.
 */
template <net::MessageDirection DIR>
struct PleaseSelectTeam final : net::ReliableMessage<PleaseSelectTeam<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct PlaySoundUnreliable final : net::UnreliableMessage<PlaySoundUnreliable<DIR>, DIR> {
	SoundId id = SoundId::none();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(id);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(id);
	}
};

template <net::MessageDirection DIR>
struct PlaySoundReliable final : net::ReliableMessage<PlaySoundReliable<DIR>, DIR> {
	SoundId id = SoundId::none();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(id);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(id);
	}
};

template <net::MessageDirection DIR>
struct PlaySoundPositionalUnreliable final : net::UnreliableMessage<PlaySoundPositionalUnreliable<DIR>, DIR> {
	SoundId id = SoundId::none();
	Vec2 position{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(id, position);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(id, position);
	}
};

template <net::MessageDirection DIR>
struct PlaySoundPositionalReliable final : net::ReliableMessage<PlaySoundPositionalReliable<DIR>, DIR> {
	SoundId id = SoundId::none();
	Vec2 position{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(id, position);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(id, position);
	}
};

template <net::MessageDirection DIR>
struct ResourceDownloadPart final : net::SecretMessage<ResourceDownloadPart<DIR>, DIR> {
	util::CRC32 nameHash{};
	net::String<DIR> part{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(nameHash, part);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(nameHash, part);
	}
};

template <net::MessageDirection DIR>
struct ResourceDownloadLast final : net::SecretMessage<ResourceDownloadLast<DIR>, DIR> {
	util::CRC32 nameHash{};
	net::String<DIR> part{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(nameHash, part);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(nameHash, part);
	}
};

template <net::MessageDirection DIR>
struct PlayerTeamSelected final : net::ReliableMessage<PlayerTeamSelected<DIR>, DIR> {
	Team oldTeam = Team::none();
	Team newTeam = Team::none();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(oldTeam, newTeam);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(oldTeam, newTeam);
	}
};

template <net::MessageDirection DIR>
struct PlayerClassSelected final : net::ReliableMessage<PlayerClassSelected<DIR>, DIR> {
	PlayerClass oldPlayerClass = PlayerClass::none();
	PlayerClass newPlayerClass = PlayerClass::none();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(oldPlayerClass, newPlayerClass);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(oldPlayerClass, newPlayerClass);
	}
};

template <net::MessageDirection DIR>
struct CommandOutput final : net::SecretMessage<CommandOutput<DIR>, DIR> {
	bool error = false;
	net::String<DIR> str{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(error, str);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(error, str);
	}
};

template <net::MessageDirection DIR>
struct HitConfirmed final : net::SecretMessage<HitConfirmed<DIR>, DIR> {
	Health damage = 0;

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(damage);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(damage);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleLoginInfo final : net::SecretMessage<RemoteConsoleLoginInfo<DIR>, DIR> {
	net::Big<crypto::pw::Salt, DIR> passwordSalt{};
	crypto::pw::HashType passwordHashType{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(passwordSalt, passwordHashType);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(passwordSalt, passwordHashType);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleLoginGranted final : net::SecretMessage<RemoteConsoleLoginGranted<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleLoginDenied final : net::SecretMessage<RemoteConsoleLoginDenied<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleResult final : net::SecretMessage<RemoteConsoleResult<DIR>, DIR> {
	cmd::Result value = cmd::done();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(value.status, value.value);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(value.status, value.value);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleOutput final : net::SecretMessage<RemoteConsoleOutput<DIR>, DIR> {
	net::String<DIR> value{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(value);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(value);
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleDone final : net::SecretMessage<RemoteConsoleDone<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct RemoteConsoleLoggedOut final : net::SecretMessage<RemoteConsoleLoggedOut<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

template <net::MessageDirection DIR>
struct InventoryEquipHat final : net::SecretMessage<InventoryEquipHat<DIR>, DIR> {
	Hat hat = Hat::none();

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(hat);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(hat);
	}
};

namespace in {

using ServerInfo = msg::cl::ServerInfo<net::MessageDirection::INPUT>;
using Joined = msg::cl::Joined<net::MessageDirection::INPUT>;
using Snapshot = msg::cl::Snapshot<net::MessageDirection::INPUT>;
using SnapshotDelta = msg::cl::SnapshotDelta<net::MessageDirection::INPUT>;
using CvarMod = msg::cl::CvarMod<net::MessageDirection::INPUT>;
using ServerEventMessage = msg::cl::ServerEventMessage<net::MessageDirection::INPUT>;
using ServerEventMessagePersonal = msg::cl::ServerEventMessagePersonal<net::MessageDirection::INPUT>;
using ChatMessage = msg::cl::ChatMessage<net::MessageDirection::INPUT>;
using PleaseSelectTeam = msg::cl::PleaseSelectTeam<net::MessageDirection::INPUT>;
using TeamChatMessage = msg::cl::TeamChatMessage<net::MessageDirection::INPUT>;
using ServerChatMessage = msg::cl::ServerChatMessage<net::MessageDirection::INPUT>;
using PlaySoundUnreliable = msg::cl::PlaySoundUnreliable<net::MessageDirection::INPUT>;
using PlaySoundReliable = msg::cl::PlaySoundReliable<net::MessageDirection::INPUT>;
using PlaySoundPositionalUnreliable = msg::cl::PlaySoundPositionalUnreliable<net::MessageDirection::INPUT>;
using PlaySoundPositionalReliable = msg::cl::PlaySoundPositionalReliable<net::MessageDirection::INPUT>;
using ResourceDownloadPart = msg::cl::ResourceDownloadPart<net::MessageDirection::INPUT>;
using ResourceDownloadLast = msg::cl::ResourceDownloadLast<net::MessageDirection::INPUT>;
using PlayerTeamSelected = msg::cl::PlayerTeamSelected<net::MessageDirection::INPUT>;
using PlayerClassSelected = msg::cl::PlayerClassSelected<net::MessageDirection::INPUT>;
using CommandOutput = msg::cl::CommandOutput<net::MessageDirection::INPUT>;
using HitConfirmed = msg::cl::HitConfirmed<net::MessageDirection::INPUT>;
using RemoteConsoleLoginInfo = msg::cl::RemoteConsoleLoginInfo<net::MessageDirection::INPUT>;
using RemoteConsoleLoginGranted = msg::cl::RemoteConsoleLoginGranted<net::MessageDirection::INPUT>;
using RemoteConsoleLoginDenied = msg::cl::RemoteConsoleLoginDenied<net::MessageDirection::INPUT>;
using RemoteConsoleResult = msg::cl::RemoteConsoleResult<net::MessageDirection::INPUT>;
using RemoteConsoleOutput = msg::cl::RemoteConsoleOutput<net::MessageDirection::INPUT>;
using RemoteConsoleDone = msg::cl::RemoteConsoleDone<net::MessageDirection::INPUT>;
using RemoteConsoleLoggedOut = msg::cl::RemoteConsoleLoggedOut<net::MessageDirection::INPUT>;
using InventoryEquipHat = msg::cl::InventoryEquipHat<net::MessageDirection::INPUT>;

} // namespace in

namespace out {

using ServerInfo = msg::cl::ServerInfo<net::MessageDirection::OUTPUT>;
using Joined = msg::cl::Joined<net::MessageDirection::OUTPUT>;
using Snapshot = msg::cl::Snapshot<net::MessageDirection::OUTPUT>;
using SnapshotDelta = msg::cl::SnapshotDelta<net::MessageDirection::OUTPUT>;
using CvarMod = msg::cl::CvarMod<net::MessageDirection::OUTPUT>;
using ServerEventMessage = msg::cl::ServerEventMessage<net::MessageDirection::OUTPUT>;
using ServerEventMessagePersonal = msg::cl::ServerEventMessagePersonal<net::MessageDirection::OUTPUT>;
using ChatMessage = msg::cl::ChatMessage<net::MessageDirection::OUTPUT>;
using PleaseSelectTeam = msg::cl::PleaseSelectTeam<net::MessageDirection::OUTPUT>;
using TeamChatMessage = msg::cl::TeamChatMessage<net::MessageDirection::OUTPUT>;
using ServerChatMessage = msg::cl::ServerChatMessage<net::MessageDirection::OUTPUT>;
using PlaySoundUnreliable = msg::cl::PlaySoundUnreliable<net::MessageDirection::OUTPUT>;
using PlaySoundReliable = msg::cl::PlaySoundReliable<net::MessageDirection::OUTPUT>;
using PlaySoundPositionalUnreliable = msg::cl::PlaySoundPositionalUnreliable<net::MessageDirection::OUTPUT>;
using PlaySoundPositionalReliable = msg::cl::PlaySoundPositionalReliable<net::MessageDirection::OUTPUT>;
using ResourceDownloadPart = msg::cl::ResourceDownloadPart<net::MessageDirection::OUTPUT>;
using ResourceDownloadLast = msg::cl::ResourceDownloadLast<net::MessageDirection::OUTPUT>;
using PlayerTeamSelected = msg::cl::PlayerTeamSelected<net::MessageDirection::OUTPUT>;
using PlayerClassSelected = msg::cl::PlayerClassSelected<net::MessageDirection::OUTPUT>;
using CommandOutput = msg::cl::CommandOutput<net::MessageDirection::OUTPUT>;
using HitConfirmed = msg::cl::HitConfirmed<net::MessageDirection::OUTPUT>;
using RemoteConsoleLoginInfo = msg::cl::RemoteConsoleLoginInfo<net::MessageDirection::OUTPUT>;
using RemoteConsoleLoginGranted = msg::cl::RemoteConsoleLoginGranted<net::MessageDirection::OUTPUT>;
using RemoteConsoleLoginDenied = msg::cl::RemoteConsoleLoginDenied<net::MessageDirection::OUTPUT>;
using RemoteConsoleResult = msg::cl::RemoteConsoleResult<net::MessageDirection::OUTPUT>;
using RemoteConsoleOutput = msg::cl::RemoteConsoleOutput<net::MessageDirection::OUTPUT>;
using RemoteConsoleDone = msg::cl::RemoteConsoleDone<net::MessageDirection::OUTPUT>;
using RemoteConsoleLoggedOut = msg::cl::RemoteConsoleLoggedOut<net::MessageDirection::OUTPUT>;
using InventoryEquipHat = msg::cl::InventoryEquipHat<net::MessageDirection::OUTPUT>;

} // namespace out

} // namespace cl
} // namespace msg

template <net::MessageDirection DIR>
using GameClientMessages = util::TypeList<       //
	msg::cl::ServerInfo<DIR>,                    // 0
	msg::cl::Joined<DIR>,                        // 1
	msg::cl::Snapshot<DIR>,                      // 2
	msg::cl::SnapshotDelta<DIR>,                 // 3
	msg::cl::CvarMod<DIR>,                       // 4
	msg::cl::ServerEventMessage<DIR>,            // 5
	msg::cl::ServerEventMessagePersonal<DIR>,    // 6
	msg::cl::ChatMessage<DIR>,                   // 7
	msg::cl::PleaseSelectTeam<DIR>,              // 8
	msg::cl::TeamChatMessage<DIR>,               // 9
	msg::cl::ServerChatMessage<DIR>,             // 10
	msg::cl::PlaySoundUnreliable<DIR>,           // 11
	msg::cl::PlaySoundReliable<DIR>,             // 12
	msg::cl::PlaySoundPositionalUnreliable<DIR>, // 13
	msg::cl::PlaySoundPositionalReliable<DIR>,   // 14
	msg::cl::ResourceDownloadPart<DIR>,          // 15
	msg::cl::ResourceDownloadLast<DIR>,          // 16
	msg::cl::PlayerTeamSelected<DIR>,            // 17
	msg::cl::PlayerClassSelected<DIR>,           // 18
	msg::cl::CommandOutput<DIR>,                 // 19
	msg::cl::HitConfirmed<DIR>,                  // 20
	msg::cl::RemoteConsoleLoginInfo<DIR>,        // 21
	msg::cl::RemoteConsoleLoginGranted<DIR>,     // 22
	msg::cl::RemoteConsoleLoginDenied<DIR>,      // 23
	msg::cl::RemoteConsoleResult<DIR>,           // 24
	msg::cl::RemoteConsoleOutput<DIR>,           // 25
	msg::cl::RemoteConsoleDone<DIR>,             // 26
	msg::cl::RemoteConsoleLoggedOut<DIR>,        // 27
	msg::cl::InventoryEquipHat<DIR>              // 28
	>;

using GameClientInputMessages = GameClientMessages<net::MessageDirection::INPUT>;
using GameClientOutputMessages = GameClientMessages<net::MessageDirection::OUTPUT>;

#endif
