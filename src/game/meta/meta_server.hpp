#ifndef AF2_META_META_SERVER_HPP
#define AF2_META_META_SERVER_HPP

#include "../../network/config.hpp"           // net::Duration, net::MAX_PACKET_SIZE
#include "../../network/connection.hpp"       // net::Connection, net::msg::in::Connect, net::sanitizeMessage
#include "../../network/endpoint.hpp"         // net::IpAddress
#include "../../network/socket.hpp"           // net::UDPSocket
#include "../../utilities/countdown.hpp"      // util::CountupLoop, util::Countup
#include "../../utilities/reference.hpp"      // util::Reference
#include "../shared/game_server_messages.hpp" // GameServerOutputMessages, msg::sv::out::...
#include "meta_client_messages.hpp"           // MetaClientOutputMessages, msg::meta::cl::out::...
#include "meta_server_messages.hpp"           // MetaServerInputMessages, msg::meta::sv::in::...

#include <cstddef>       // std::size_t, std::byte
#include <string>        // std::string
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

class Game;

class MetaServer final {
public:
	using BannedClients = std::unordered_set<net::IpAddress>;

	[[nodiscard]] static auto getConfigHeader() -> std::string;

	explicit MetaServer(Game& game);

	[[nodiscard]] auto init() -> bool;

	auto shutDown() -> void;

	auto stop(std::string_view message = {}) -> bool;

	[[nodiscard]] auto update(float deltaTime) -> bool;

	auto updateTimeout() -> void;
	auto updateThrottle() -> void;
	auto updateSpamLimit() -> void;
	auto updateTickrate() -> void;
	auto updateConfigAutoSaveInterval() -> void;
	auto updatePrivateAddressOverride() -> void;

	[[nodiscard]] auto getStatusString() const -> std::string;

	[[nodiscard]] auto kickClient(net::IpAddress ip) -> bool;
	auto banClient(net::IpAddress ip) -> void;
	[[nodiscard]] auto unbanClient(net::IpAddress ip) -> bool;

	[[nodiscard]] auto getBannedClients() const -> const BannedClients&;

private:
	struct MessageHandler final {
		util::Reference<MetaServer> server;

		template <typename Message>
		auto operator()(Message&& msg) const -> void {
			server->handleMessage(std::forward<Message>(msg));
		}
	};

	using Connection = net::Connection<MetaServerInputMessages, MessageHandler>;

	struct ClientInfo final {
		Connection connection;
		bool connecting = true;
		int spamCounter = 0;
		util::Countup<float> afkTimer{};
		net::IpEndpoint listedEndpoint{};
		bool heartbeatReceived = false;

		ClientInfo(net::UDPSocket& socket, net::Duration timeout, int throttleMaxSendBufferSize, int throttleMaxPeriod, MetaServer& server)
			: connection(socket, timeout, throttleMaxSendBufferSize, throttleMaxPeriod, MessageHandler{server}) {}
	};

	using Clients = std::vector<ClientInfo>;

	auto handleMessage(net::msg::in::Connect&& msg) -> void;
	auto handleMessage(msg::meta::sv::in::Heartbeat&& msg) -> void;
	auto handleMessage(msg::meta::sv::in::GameServerAddressListRequest&& msg) -> void;

	[[nodiscard]] auto testSpam() -> bool;

	auto tick() -> void;

	auto updateConfigAutoSave(float deltaTime) -> void;
	auto receivePackets() -> void;
	auto updateConnections() -> void;
	auto updateTicks(float deltaTime) -> void;
	auto updateClients(float deltaTime) -> void;
	auto sendPackets() -> void;

	auto findClient(net::IpAddress ip) noexcept -> Clients::iterator;
	auto findClient(net::IpEndpoint endpoint) noexcept -> Clients::iterator;

	auto updateClient(Clients::iterator it, float deltaTime, int spamUpdates) -> void;
	auto updateClientSpamCounter(Clients::iterator it, int spamUpdates) -> void;
	auto updateClientAfkTimer(Clients::iterator it, float deltaTime) -> void;

	auto disconnectClient(Clients::iterator it, std::string_view reason) -> void;

	auto dropClient(Clients::iterator it) -> void;

	[[nodiscard]] auto countClientsWithIp(net::IpAddress ip) const noexcept -> std::size_t;

	Game& m_game;
	net::UDPSocket m_socket{};
	Clients m_clients{};
	Clients::iterator m_currentClient;
	float m_spamInterval = 0.0f;
	float m_tickInterval = 0.0f;
	float m_configAutoSaveInterval = 0.0f;
	util::CountupLoop<float> m_spamTimer{};
	util::CountupLoop<float> m_tickTimer{};
	util::CountupLoop<float> m_heartbeatRequestTimer{};
	util::CountupLoop<float> m_configAutoSaveTimer{};
	BannedClients m_bannedClients{};
	std::vector<net::IpEndpoint> m_gameServerAddressList{};
	net::IpAddress m_privateAddressOverride{};
	std::size_t m_connectingClients = 0;
	bool m_stopping = false;
	bool m_hasPrivateAddressOverride = false;
};

#endif
