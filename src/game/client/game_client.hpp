#ifndef AF2_CLIENT_GAME_CLIENT_HPP
#define AF2_CLIENT_GAME_CLIENT_HPP

#include "../../console/command.hpp"          // cmd::...
#include "../../network/connection.hpp"       // net::Connection, net::msg::in::Connect, net::sanitizeMessage
#include "../../network/crypto.hpp"           // crypto::...
#include "../../network/endpoint.hpp"         // net::IpAddress, net::PortNumber
#include "../../network/socket.hpp"           // net::UDPSocket
#include "../../utilities/countdown.hpp"      // util::Countup, util::CountupLoop
#include "../../utilities/crc.hpp"            // util::CRC32
#include "../../utilities/reference.hpp"      // util::Reference
#include "../../utilities/tile_matrix.hpp"    // util::TileMatrix
#include "../data/color.hpp"                  // Color
#include "../data/direction.hpp"              // Direction
#include "../data/hat.hpp"                    // Hat
#include "../data/health.hpp"                 // Health
#include "../data/player_class.hpp"           // PlayerClass
#include "../data/player_id.hpp"              // PlayerId, PLAYER_ID_UNCONNECTED
#include "../data/projectile_type.hpp"        // ProjectileType
#include "../data/rectangle.hpp"              // Rect
#include "../data/team.hpp"                   // Team
#include "../data/tick_count.hpp"             // TickCount
#include "../data/tickrate.hpp"               // Tickrate
#include "../data/vector.hpp"                 // Vec2, Vector2
#include "../shared/game_client_messages.hpp" // GameClientInputMessages, msg::cl::in::...
#include "../shared/game_server_messages.hpp" // GameServerOutputMessages, msg::sv::out::...
#include "../shared/snapshot.hpp"             // Snapshot
#include "inventory_client.hpp"               // InventoryClient
#include "remote_console_client.hpp"          // RemoteConsoleClient

#include <SDL.h>       // SDL_...
#include <cstddef>     // std::byte, std::size_t
#include <deque>       // std::deque
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::forward
#include <vector>      // std::vector

class Game;
class VirtualMachine;
class SoundManager;
class InputManager;
class CharWindow;

class GameClient final
	: public InventoryClient
	, public RemoteConsoleClient {
public:
	[[nodiscard]] static auto getConfigHeader() -> std::string;

	GameClient(Game& game, VirtualMachine& vm, CharWindow& charWindow, SoundManager& soundManager, InputManager& inputManager);

	[[nodiscard]] auto init() -> bool;

	auto shutDown() -> void;

	auto handleEvent(const SDL_Event& e) -> void;

	[[nodiscard]] auto update(float deltaTime) -> bool;

	auto draw() const -> void;

	auto toggleTeamSelect() -> void;
	auto toggleClassSelect() -> void;

	auto disconnect() -> void;

	auto updateTimeout() -> void;
	auto updateThrottle() -> void;
	auto updateCommandInterval() -> void;
	[[nodiscard]] auto updateUpdateRate() -> bool;
	[[nodiscard]] auto updateUsername() -> bool;

	[[nodiscard]] auto writeChatMessage(std::string_view message) -> bool;
	[[nodiscard]] auto writeTeamChatMessage(std::string_view message) -> bool;

	[[nodiscard]] auto teamSelect(Team team) -> bool;
	[[nodiscard]] auto teamSelectAuto() -> bool;
	[[nodiscard]] auto teamSelectRandom() -> bool;

	[[nodiscard]] auto classSelect(PlayerClass playerClass) -> bool;
	[[nodiscard]] auto classSelectAuto() -> bool;
	[[nodiscard]] auto classSelectRandom() -> bool;

	[[nodiscard]] auto forwardCommand(cmd::CommandView argv) -> bool;

	[[nodiscard]] auto hasJoinedGame() const -> bool;
	[[nodiscard]] auto hasSelectedTeam() const -> bool;

	[[nodiscard]] auto getPlayerId() const -> PlayerId;

	[[nodiscard]] auto worldToGridCoordinates(Vec2 position) const -> Vec2;
	[[nodiscard]] auto gridToWorldCoordinates(Vec2 position) const -> Vec2;

	[[nodiscard]] auto getStatusString() const -> std::string;

private:
	struct MessageHandler final {
		util::Reference<GameClient> client;

		template <typename Message>
		auto operator()(Message&& msg) const -> void {
			client->handleMessage(std::forward<Message>(msg));
		}
	};

	using Connection = net::Connection<GameClientInputMessages, MessageHandler>;

	using Screen = util::TileMatrix<char>;

	struct ResourceDownload final {
		ResourceDownload() noexcept = default;
		ResourceDownload(std::string name, util::CRC32 nameHash, util::CRC32 fileHash, std::size_t size, bool isText)
			: name(std::move(name))
			, nameHash(nameHash)
			, fileHash(fileHash)
			, size(size)
			, isText(isText) {}

		std::string data;
		std::string name;
		util::CRC32 nameHash;
		util::CRC32 fileHash;
		std::size_t size = 0;
		bool isText = false;
	};
	using ResourceDownloadQueue = std::deque<ResourceDownload>;

	template <typename Message>
	[[nodiscard]] auto writeToGameServer(const Message& msg) -> bool {
		return m_connection.write<GameServerOutputMessages>(msg);
	}

	auto handleMessage(net::msg::in::Connect&& msg) -> void;
	auto handleMessage(msg::cl::in::ServerInfo&& msg) -> void;
	auto handleMessage(msg::cl::in::Joined&& msg) -> void;
	auto handleMessage(msg::cl::in::Snapshot&& msg) -> void;
	auto handleMessage(msg::cl::in::SnapshotDelta&& msg) -> void;
	auto handleMessage(msg::cl::in::CvarMod&& msg) -> void;
	auto handleMessage(msg::cl::in::ServerEventMessage&& msg) -> void;
	auto handleMessage(msg::cl::in::ServerEventMessagePersonal&& msg) -> void;
	auto handleMessage(msg::cl::in::ChatMessage&& msg) -> void;
	auto handleMessage(msg::cl::in::TeamChatMessage&& msg) -> void;
	auto handleMessage(msg::cl::in::ServerChatMessage&& msg) -> void;
	auto handleMessage(msg::cl::in::PleaseSelectTeam&& msg) -> void;
	auto handleMessage(msg::cl::in::PlaySoundUnreliable&& msg) -> void;
	auto handleMessage(msg::cl::in::PlaySoundReliable&& msg) -> void;
	auto handleMessage(msg::cl::in::PlaySoundPositionalUnreliable&& msg) -> void;
	auto handleMessage(msg::cl::in::PlaySoundPositionalReliable&& msg) -> void;
	auto handleMessage(msg::cl::in::ResourceDownloadPart&& msg) -> void;
	auto handleMessage(msg::cl::in::ResourceDownloadLast&& msg) -> void;
	auto handleMessage(msg::cl::in::PlayerTeamSelected&& msg) -> void;
	auto handleMessage(msg::cl::in::PlayerClassSelected&& msg) -> void;
	auto handleMessage(msg::cl::in::CommandOutput&& msg) -> void;
	auto handleMessage(msg::cl::in::HitConfirmed&& msg) -> void;

	using InventoryClient::handleMessage;
	using RemoteConsoleClient::handleMessage;

	auto vm() -> VirtualMachine& override;

	auto write(msg::sv::out::InventoryEquipHatRequest&& msg) -> bool override;

	auto write(msg::sv::out::RemoteConsoleLoginInfoRequest&& msg) -> bool override;
	auto write(msg::sv::out::RemoteConsoleLoginRequest&& msg) -> bool override;
	auto write(msg::sv::out::RemoteConsoleCommand&& msg) -> bool override;
	auto write(msg::sv::out::RemoteConsoleAbortCommand&& msg) -> bool override;
	auto write(msg::sv::out::RemoteConsoleLogout&& msg) -> bool override;

	auto downloadNextResourceInQueue() -> void;
	auto joinGame() -> void;
	auto receivePackets() -> void;
	auto loadScreen(Screen& screen, std::string_view filename) -> void;

	auto drawChar(Vec2 position, Color color, char ch) const -> void;
	auto drawString(Vec2 position, Color color, std::string_view str) const -> void;
	auto drawGun(Vec2 position, Color color, Direction direction, std::string_view gun) const -> void;
	auto drawCorpse(Vec2 position, Color color) const -> void;
	auto drawPlayer(Vec2 position, Color color, Direction aimDirection, PlayerClass playerClass, Hat hat) const -> void;
	auto drawProjectile(Vec2 position, Color color, ProjectileType type) const -> void;
	auto drawExplosion(Vec2 position, Color color) const -> void;
	auto drawSentryGun(Vec2 position, Color color, Direction aimDirection) const -> void;
	auto drawGenericEntity(Vec2 position, Color color, const util::TileMatrix<char>& matrix) const -> void;
	auto drawMedkit(Vec2 position) const -> void;
	auto drawAmmopack(Vec2 position) const -> void;
	auto drawFlag(Vec2 position, Color color) const -> void;
	auto drawCart(Vec2 position, Color color) const -> void;

	Game& m_game;
	VirtualMachine& m_vm;
	CharWindow& m_charWindow;
	SoundManager& m_soundManager;
	InputManager& m_inputManager;
	net::UDPSocket m_socket{};
	Connection m_connection;
	crypto::pw::Salt m_serverPasswordSalt{};
	crypto::pw::HashType m_serverPasswordHashType{};
	Tickrate m_serverTickrate;
	std::string m_serverMapName{};
	Snapshot m_snapshot{};
	TickCount m_userCmdNumber = 0;
	float m_commandInterval = 0.0f;
	util::CountupLoop<float> m_commandTimer{};
	PlayerId m_playerId = PLAYER_ID_UNCONNECTED;
	Screen m_screen{};
	Screen m_teamSelectScreen{};
	Screen m_classSelectScreen{};
	Screen m_scoreboardScreen{};
	Vec2 m_viewPosition;
	Rect m_viewport;
	Vec2 m_mousePosition{};
	bool m_teamSelected = true;
	bool m_classSelected = true;
	Team m_selectedTeam = Team::spectators();
	ResourceDownloadQueue m_resourceDownloadQueue;
	bool m_aimingLeft = false;
	bool m_aimingRight = false;
	bool m_aimingUp = false;
	bool m_aimingDown = false;
};

#endif
