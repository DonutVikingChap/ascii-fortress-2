#ifndef AF2_SERVER_GAME_SERVER_HPP
#define AF2_SERVER_GAME_SERVER_HPP

#include "../../console/command.hpp"          // cmd::...
#include "../../console/environment.hpp"      // Environment
#include "../../console/script.hpp"           // Script
#include "../../network/config.hpp"           // net::Duration, net::MAX_...
#include "../../network/connection.hpp"       // net::Connection, net::msg::in::Connect, net::sanitizeMessage
#include "../../network/crypto.hpp"           // crypto::...
#include "../../network/endpoint.hpp"         // net::IpEndpoint, net::IpAddress, net::PortNumber
#include "../../network/socket.hpp"           // net::UDPSocket
#include "../../utilities/countdown.hpp"      // util::CountupLoop
#include "../../utilities/crc.hpp"            // util::CRC32
#include "../../utilities/multi_hash.hpp"     // util::MultiHash
#include "../../utilities/reference.hpp"      // util::Reference
#include "../../utilities/span.hpp"           // util::Span, util::asBytes
#include "../data/hat.hpp"                    // Hat
#include "../data/health.hpp"                 // Health
#include "../data/inventory.hpp"              // InventoryId, InventoryToken, INVENTORY_ID_INVALID
#include "../data/player_class.hpp"           // PlayerClass
#include "../data/player_id.hpp"              // PlayerId, PLAYER_ID_UNCONNECTED
#include "../data/score.hpp"                  // Score
#include "../data/team.hpp"                   // Team
#include "../data/tick_count.hpp"             // TickCount
#include "../data/tickrate.hpp"               // Tickrate
#include "../shared/convar_update.hpp"        // ConVarUpdate
#include "../shared/game_client_messages.hpp" // GameClientOutputMessages, msg::cl::out::...
#include "../shared/game_server_messages.hpp" // GameServerInputMessages, msg::sv::in::...
#include "../shared/resource_info.hpp"        // ResourceInfo
#include "../shared/snapshot.hpp"             // Snapshot
#include "bot.hpp"                            // Bot
#include "inventory_server.hpp"               // InventoryServer
#include "remote_console_server.hpp"          // RemoteConsoleServer
#include "world.hpp"                          // World

#include <array>         // std::array
#include <cstddef>       // std::size_t
#include <deque>         // std::deque
#include <memory>        // std::shared_ptr, std::unique_ptr, std::make_unique
#include <optional>      // std::optional, std::nullopt
#include <random>        // std::discrete_distribution
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::forward
#include <vector>        // std::vector

class Game;
class ConVar;
struct Environment;

class GameServer final
	: public InventoryServer
	, public RemoteConsoleServer {
public:
	struct BannedPlayer final {
		std::string username;

		explicit BannedPlayer(std::string username)
			: username(std::move(username)) {}
	};
	using BannedPlayers = std::unordered_map<net::IpAddress, BannedPlayer>;

	static constexpr auto USERNAME_UNCONNECTED = std::string_view{"unconnected"};
	static constexpr auto USERNAME_META_SERVER = std::string_view{"metaserver"};

	[[nodiscard]] static auto getConfigHeader() -> std::string;

	static auto updateHatDropWeights() -> void;
	static auto replicate(const ConVar& cvar) -> void;

	GameServer(Game& game, VirtualMachine& vm, std::shared_ptr<Environment> env, std::shared_ptr<Process> process);
	~GameServer() override;

	GameServer(const GameServer&) = delete;
	GameServer(GameServer&&) = delete;

	auto operator=(const GameServer&) -> GameServer& = delete;
	auto operator=(GameServer&&) -> GameServer& = delete;

	[[nodiscard]] auto init() -> bool;

	auto shutDown() -> void;

	auto stop(std::string_view message = {}) -> bool;

	[[nodiscard]] auto update(float deltaTime) -> bool;

	auto updateTimeout() -> void;
	auto updateThrottle() -> void;
	auto updateSpamLimit() -> void;
	auto updateTickrate() -> void;
	auto updateBotTickrate() -> void;
	auto updateConfigAutoSaveInterval() -> void;
	auto updateResourceUploadInterval() -> void;
	auto updateAllowResourceDownload() -> void;
	auto updateMetaSubmit() -> void;

	auto changeLevel() -> void;
	auto changeLevelToNext() -> void;

	[[nodiscard]] auto hasPlayers() const -> bool;

	[[nodiscard]] auto getStatusString() const -> std::string;

	[[nodiscard]] auto kickPlayer(std::string_view ipOrName) -> bool;
	[[nodiscard]] auto banPlayer(std::string_view ipOrName, std::optional<std::string> playerUsername) -> bool;
	[[nodiscard]] auto unbanPlayer(net::IpAddress ip) -> bool;

	[[nodiscard]] auto addBot(std::string name = "1", Team team = Team::none(), PlayerClass playerClass = PlayerClass::none()) -> bool;
	[[nodiscard]] auto kickBot(std::string_view name) -> bool;
	auto kickAllBots() -> void;

	auto freezeBots() -> void;

	[[nodiscard]] auto getBannedPlayers() const -> const BannedPlayers&;
	[[nodiscard]] auto getConnectedClientIps() const -> std::vector<net::IpEndpoint>;
	[[nodiscard]] auto getBotNames() const -> std::vector<std::string>;
	[[nodiscard]] auto getPlayerIdByIp(net::IpEndpoint endpoint) const -> std::optional<PlayerId>;
	[[nodiscard]] auto getPlayerInventoryId(PlayerId id) const -> std::optional<InventoryId>;
	[[nodiscard]] auto getPlayerIp(PlayerId id) const -> std::optional<net::IpEndpoint>;

	auto awardPlayerPoints(PlayerId id, std::string_view name, Score points) -> bool;

	[[nodiscard]] auto rockTheVote(net::IpEndpoint endpoint) -> bool;

	[[nodiscard]] auto isPlayerBot(PlayerId id) const -> bool;

	[[nodiscard]] auto world() noexcept -> World&;
	[[nodiscard]] auto world() const noexcept -> const World&;

	auto resetClients() -> void;
	auto resetEnvironment() -> void;

	[[nodiscard]] auto writeCommandOutput(net::IpEndpoint endpoint, std::string_view message) -> bool;
	[[nodiscard]] auto writeCommandError(net::IpEndpoint endpoint, std::string_view message) -> bool;

	auto writeServerChatMessage(std::string_view message) -> void;
	auto writeServerChatMessage(std::string_view message, Team team) -> void;
	auto writeServerChatMessagePersonal(std::string_view message, PlayerId playerId) -> bool;
	auto writeServerEventMessage(std::string_view message) -> void;
	auto writeServerEventMessage(std::string_view message, Team team) -> void;
	auto writeServerEventMessage(std::string_view message, util::Span<const PlayerId> relevantPlayerIds) -> void;
	auto writeServerEventMessagePersonal(std::string_view message, PlayerId playerId) -> bool;

	auto writePlayerTeamSelected(Team oldTeam, Team newTeam, PlayerId playerId) -> void;
	auto writePlayerClassSelected(PlayerClass oldPlayerClass, PlayerClass newPlayerClass, PlayerId playerId) -> void;

	auto writeHitConfirmed(Health damage, PlayerId playerId) -> void;

	auto playWorldSound(SoundId soundId, Vec2 position) -> void;
	auto playWorldSound(SoundId soundId, Vec2 position, PlayerId source) -> void;

	auto playTeamSound(SoundId soundId, Team team) -> void;
	auto playTeamSound(SoundId correctTeamId, SoundId otherTeamId, Team team) -> void;

	auto playPlayerInterfaceSound(SoundId soundId, PlayerId playerId) -> void;

	auto playGameSound(SoundId soundId) -> void;

	auto setObject(std::string name, Environment::Object object) -> void;
	auto deleteObject(const std::string& name) -> void;

	auto callIfDefined(Script::Command command) -> void;
	auto callScript(Script script) -> void;

private:
	struct MessageHandler final {
		util::Reference<GameServer> server;

		template <typename Message>
		auto operator()(Message&& msg) const -> void {
			server->handleMessage(std::forward<Message>(msg));
		}
	};

	using Connection = net::Connection<GameServerInputMessages, MessageHandler>;

	using HatDistribution = std::discrete_distribution<unsigned short>;

	struct Resource final {
		Resource() noexcept = default;
		Resource(std::string name, std::string data, bool canDownload)
			: name(std::move(name))
			, data(std::move(data))
			, canDownload(canDownload) {}

		std::string name{};
		std::string data{};
		bool canDownload = false;
	};
	using Resources = std::unordered_map<util::CRC32, Resource>;
	using ResourceInfoList = std::vector<ResourceInfo>;

	struct ClientInfo final {
		using SnapshotBuffer = std::unique_ptr<std::array<Snapshot, 32>>;
		using RconToken = std::optional<std::string_view>;

		Connection connection;
		TickCount latestUserCmdNumber = 0;
		TickCount latestSnapshotReceived = 0;
		float updateInterval = 0.0f;
		util::CountupLoop<float> updateTimer{};
		bool connecting = true;
		bool wantsToRtv = false;
		int spamCounter = 0;
		util::Countup<float> afkTimer{};
		Actions latestActions = Action::NONE;
		SnapshotBuffer snapshots = std::make_unique<std::array<Snapshot, 32>>();
		Resources::const_pointer resourceUpload = nullptr;
		std::size_t resourceUploadProgress = 0;
		util::CountupLoop<float> resourceUploadTimer{};

		ClientInfo(net::UDPSocket& socket, net::Duration duration, int throttleMaxSendBufferSize, int throttleMaxPeriod, GameServer& server)
			: connection(socket, duration, throttleMaxSendBufferSize, throttleMaxPeriod, MessageHandler{server}) {}

		template <typename Message>
		[[nodiscard]] auto write(const Message& msg) -> bool {
			return connection.write<GameClientOutputMessages>(msg);
		}
	};

	using Clients = util::MultiHash<ClientInfo,           // client
									net::IpEndpoint,      // endpoint
									net::IpAddress,       // address
									std::string,          // username
									PlayerId,             // playerId
									InventoryId,          // inventoryId
									ClientInfo::RconToken // rconToken
									>;

	static constexpr auto CLIENT_CLIENT = std::size_t{0};                           // client
	static constexpr auto CLIENT_ENDPOINT = std::size_t{CLIENT_CLIENT + 1};         // endpoint
	static constexpr auto CLIENT_ADDRESS = std::size_t{CLIENT_ENDPOINT + 1};        // address
	static constexpr auto CLIENT_USERNAME = std::size_t{CLIENT_ADDRESS + 1};        // username
	static constexpr auto CLIENT_PLAYER_ID = std::size_t{CLIENT_USERNAME + 1};      // playerId
	static constexpr auto CLIENT_INVENTORY_ID = std::size_t{CLIENT_PLAYER_ID + 1};  // inventoryId
	static constexpr auto CLIENT_RCON_TOKEN = std::size_t{CLIENT_INVENTORY_ID + 1}; // rconToken

	[[nodiscard]] static auto modifiedCvars() -> std::unordered_set<const ConVar*>&;

	auto resetClient(Clients::iterator it) -> void {
		auto& client = **it;

		client.latestUserCmdNumber = 0;
		client.latestSnapshotReceived = 0;
		client.updateInterval = 0.0f;
		client.updateTimer.reset();
		client.wantsToRtv = false;
		client.spamCounter = 0;
		client.afkTimer.reset();
		client.latestActions = Action::NONE;
		client.snapshots->fill(Snapshot{});
		client.resourceUpload = nullptr;
		client.resourceUploadProgress = 0;
		client.resourceUploadTimer.reset();
		m_clients.set<CLIENT_USERNAME>(it, std::string{});
		m_clients.set<CLIENT_PLAYER_ID>(it, PLAYER_ID_UNCONNECTED);
		m_clients.set<CLIENT_INVENTORY_ID>(it, INVENTORY_ID_INVALID);
	}

	auto handleMessage(net::msg::in::Connect&& msg) -> void;
	auto handleMessage(msg::sv::in::ServerInfoRequest&& msg) -> void;
	auto handleMessage(msg::sv::in::JoinRequest&& msg) -> void;
	auto handleMessage(msg::sv::in::UserCmd&& msg) -> void;
	auto handleMessage(msg::sv::in::ChatMessage&& msg) -> void;
	auto handleMessage(msg::sv::in::TeamChatMessage&& msg) -> void;
	auto handleMessage(msg::sv::in::TeamSelect&& msg) -> void;
	auto handleMessage(msg::sv::in::ResourceDownloadRequest&& msg) -> void;
	auto handleMessage(msg::sv::in::UpdateRateChange&& msg) -> void;
	auto handleMessage(msg::sv::in::UsernameChange&& msg) -> void;
	auto handleMessage(msg::sv::in::ForwardedCommand&& msg) -> void;
	auto handleMessage(msg::sv::in::HeartbeatRequest&& msg) -> void;
	auto handleMessage(msg::sv::in::MetaInfoRequest&& msg) -> void;

	using InventoryServer::handleMessage;
	using RemoteConsoleServer::handleMessage;

	static auto hatDistribution() -> HatDistribution&;
	auto generateHat() -> Hat;

	[[nodiscard]] auto game() -> Game& override;
	[[nodiscard]] auto gameServer() -> GameServer& override;
	[[nodiscard]] auto vm() -> VirtualMachine& override;

	[[nodiscard]] auto testSpam() -> bool override;

	auto equipHat(InventoryId id, Hat hat) -> void override;
	auto unequipHat(InventoryId id, Hat hat) -> void override;

	auto getEquippedHat(InventoryId id) const -> Hat override;

	auto reply(msg::cl::out::InventoryEquipHat&& msg) -> void override;
	auto reply(msg::cl::out::RemoteConsoleLoginInfo&& msg) -> void override;
	auto reply(msg::cl::out::RemoteConsoleLoginGranted&& msg) -> void override;
	auto reply(msg::cl::out::RemoteConsoleLoginDenied&& msg) -> void override;
	auto reply(msg::cl::out::RemoteConsoleResult&& msg) -> void override;
	auto reply(msg::cl::out::RemoteConsoleOutput&& msg) -> void override;
	auto reply(msg::cl::out::RemoteConsoleDone&& msg) -> void override;
	auto reply(msg::cl::out::RemoteConsoleLoggedOut&& msg) -> void override;

	auto registerCurrentRconClient(std::string_view username) -> void override;
	auto unregisterRconClient(std::string_view username) -> void override;

	[[nodiscard]] auto getCurrentClientInventoryId() const -> InventoryId override;

	[[nodiscard]] auto getCurrentClientRegisteredRconUsername() const -> std::optional<std::string_view> override;

	auto write(std::string_view username, msg::cl::out::RemoteConsoleResult&& msg) -> void override;
	auto write(std::string_view username, msg::cl::out::RemoteConsoleOutput&& msg) -> void override;
	auto write(std::string_view username, msg::cl::out::RemoteConsoleDone&& msg) -> void override;
	auto write(std::string_view username, msg::cl::out::RemoteConsoleLoggedOut&& msg) -> void override;

	auto tick() -> void;

	auto updateConfigAutoSave(float deltaTime) -> void;
	auto receivePackets() -> void;
	auto updateConnections() -> void;
	auto updateMetaServerConnection(float deltaTime) -> void;
	auto updateTicks(float deltaTime) -> void;
	auto updateClients(float deltaTime) -> void;
	auto sendPackets() -> void;
	auto updateProcess() -> void;

	[[nodiscard]] auto pollModifiedCvars() -> std::vector<ConVarUpdate>;

	auto updateClient(Clients::iterator it, float deltaTime, int spamUpdates, const msg::cl::out::CvarMod& modifiedCvarsMessage) -> void;
	auto updateClientSpamCounter(Clients::iterator it, int spamUpdates) -> void;
	auto updateClientAfkTimer(Clients::iterator it, float deltaTime) -> void;
	auto writeClientModifiedCvars(Clients::iterator it, const msg::cl::out::CvarMod& modifiedCvarsMessage) -> void;
	auto updateClientResourceUpload(Clients::iterator it, float deltaTime) -> void;
	auto updateClientPing(Clients::iterator it) -> void;

	auto addResource(std::string name, std::string data) -> void;

	[[nodiscard]] auto loadMap() -> bool;

	auto connectToMetaServer() -> void;

	auto disconnectClient(Clients::iterator it, std::string_view reason) -> void;

	auto dropClient(Clients::iterator it) -> void;

	[[nodiscard]] auto writeServerInfo(ClientInfo& client) -> bool;

	auto writeCommandOutput(ClientInfo& client, std::string_view message) -> void;
	auto writeCommandError(ClientInfo& client, std::string_view message) -> void;

	auto writeWorldStateToClients() -> void;

	[[nodiscard]] auto findValidUsername(std::string_view original) const -> std::string;

	[[nodiscard]] auto findClient(std::string_view ipOrName) -> Clients::iterator;
	[[nodiscard]] auto findClient(std::string_view ipOrName) const -> Clients::const_iterator;
	[[nodiscard]] auto findClientByIp(net::IpEndpoint endpoint) -> Clients::iterator;
	[[nodiscard]] auto findClientByIp(net::IpEndpoint endpoint) const -> Clients::const_iterator;

	[[nodiscard]] auto countClientsWithIp(net::IpAddress ip) const noexcept -> std::size_t;
	[[nodiscard]] auto countPlayersWithIp(net::IpAddress ip) const noexcept -> std::size_t;

	Game& m_game;
	VirtualMachine& m_vm;
	std::shared_ptr<Environment> m_env;
	std::shared_ptr<Process> m_process;
	World m_world;
	net::UDPSocket m_socket{};
	Resources m_resources{};
	ResourceInfoList m_resourceInfo{};
	Tickrate m_tickrate = 0;
	float m_spamInterval = 0.0f;
	float m_tickInterval = 0.0f;
	float m_botTickInterval = 0.0f;
	float m_configAutoSaveInterval = 0.0f;
	float m_resourceUploadInterval = 0.0f;
	util::CountupLoop<float> m_spamTimer{};
	util::CountupLoop<float> m_tickTimer{};
	util::CountupLoop<float> m_botTickTimer{};
	util::CountupLoop<float> m_configAutoSaveTimer{};
	util::CountupLoop<float> m_metaServerRetryTimer{};
	BannedPlayers m_bannedPlayers{};
	std::vector<Bot> m_bots{};
	Clients m_clients{};
	Clients::iterator m_currentClient;
	Bot::CoordinateDistributionX m_xCoordinateDistribution{};
	Bot::CoordinateDistributionY m_yCoordinateDistribution{};
	net::IpEndpoint m_metaServerEndpoint{};
	std::size_t m_currentBotIndex = 0;
	std::size_t m_connectingClients = 0;
	bool m_stopping = false;
};

#endif
