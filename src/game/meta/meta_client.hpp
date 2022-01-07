#ifndef AF2_META_META_CLIENT_HPP
#define AF2_META_META_CLIENT_HPP

#include "../../network/config.hpp"           // net::Clock, net::TimePoint, net::Duration, net::DISCONNECT_DURATION, net::MAX_PACKET_SIZE
#include "../../network/connection.hpp"       // net::Connection, net::msg::in::Connect, net::sanitizeMessage
#include "../../network/endpoint.hpp"         // net::IpEndpoint, net::IpAddress, net::PortNumber
#include "../../network/socket.hpp"           // net::UDPSocket
#include "../../utilities/countdown.hpp"      // util::CountupLoop
#include "../../utilities/reference.hpp"      // util::Reference
#include "../../utilities/span.hpp"           // util::Span
#include "../shared/game_server_messages.hpp" // GameServerOutputMessages, msg::sv::out::...
#include "meta_client_messages.hpp"           // MetaClientInputMessages, msg::meta::cl::in::...
#include "meta_server_messages.hpp"           // MetaServerOutputMessages, msg::meta::sv::out::...

#include <cstdint>       // std::uint8_t
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move, std::forward
#include <vector>        // std::vector

class Game;

class MetaClient final {
public:
	struct ReceivedMetaInfo final {
		msg::meta::cl::in::MetaInfo info{};
		net::IpEndpoint endpoint{};
		net::Duration ping = net::Duration::zero();

		ReceivedMetaInfo() noexcept = default;
		ReceivedMetaInfo(msg::meta::cl::in::MetaInfo&& info, net::IpEndpoint endpoint, net::Duration ping)
			: info(std::move(info))
			, endpoint(endpoint)
			, ping(ping) {}
	};

	explicit MetaClient(Game& game);

	[[nodiscard]] auto init() -> bool;

	auto shutDown() -> void;

	auto stop(std::string_view message = {}) -> bool;

	[[nodiscard]] auto update(float deltaTime) -> bool;

	auto updateTimeout() -> void;
	auto updateThrottle() -> void;
	auto updateSendInterval() -> void;

	[[nodiscard]] auto getStatusString() const -> std::string;

	[[nodiscard]] auto refresh() noexcept -> bool;

	[[nodiscard]] auto isConnecting() const noexcept -> bool;
	[[nodiscard]] auto hasReceivedGameServerEndpoints() const noexcept -> bool;

	[[nodiscard]] auto metaInfo() const noexcept -> util::Span<const ReceivedMetaInfo>;
	[[nodiscard]] auto gameServerEndpoints() const noexcept -> util::Span<const net::IpEndpoint>;

private:
	struct MessageHandler final {
		util::Reference<MetaClient> client;

		template <typename Message>
		auto operator()(Message&& msg) const -> void {
			client->handleMessage(std::forward<Message>(msg));
		}
	};

	using Connection = net::Connection<MetaClientInputMessages, MessageHandler>;

	struct GameServerInfo final {
		Connection connection;
		net::TimePoint metaInfoRequestSendTime{};
		bool metaInfoRequestWritten = false;
		bool metaInfoRequestSent = false;

		GameServerInfo(net::UDPSocket& socket, net::Duration duration, int throttleMaxSendBufferSize, int throttleMaxPeriod, MetaClient& client)
			: connection(socket, duration, throttleMaxSendBufferSize, throttleMaxPeriod, MessageHandler{client}) {}

		template <typename Message>
		[[nodiscard]] auto write(const Message& msg) -> bool {
			return connection.write<GameServerOutputMessages>(msg);
		}
	};

	using GameServers = std::vector<GameServerInfo>;

	struct GameServerCooldown final {
		net::TimePoint endTime{};
	};

	using GameServerCooldowns = std::unordered_map<net::IpEndpoint, GameServerCooldown>;

	template <typename Message>
	[[nodiscard]] auto writeToMetaServer(const Message& msg) -> bool {
		return m_metaServerConnection.write<MetaServerOutputMessages>(msg);
	}

	auto handleMessage(net::msg::in::Connect&& msg) -> void;
	auto handleMessage(msg::meta::cl::in::GameServerAddressList&& msg) -> void;
	auto handleMessage(msg::meta::cl::in::MetaInfo&& msg) -> void;

	auto receivePackets() -> void;
	auto updateConnections() -> void;
	auto sendPackets() -> void;

	[[nodiscard]] auto findGameServer(net::IpEndpoint endpoint) noexcept -> GameServers::iterator;

	[[nodiscard]] auto connectPending(net::IpEndpoint endpoint) -> bool;

	Game& m_game;
	net::UDPSocket m_socket{};
	Connection m_metaServerConnection;
	GameServers m_gameServerConnections{};
	GameServers::iterator m_currentGameServer;
	std::vector<net::IpEndpoint> m_gameServerEndpoints{};
	std::vector<net::IpEndpoint> m_pendingGameServerEndpoints{};
	std::vector<ReceivedMetaInfo> m_metaInfo{};
	GameServerCooldowns m_gameServerCooldowns{};
	float m_sendInterval = 0.0f;
	util::CountupLoop<float> m_sendTimer{};
	bool m_stopping = false;
	bool m_hasReceivedGameServerEndpoints = false;
};

#endif
