#include "meta_client.hpp"

#include "../../console/commands/meta_client_commands.hpp" // meta_address, meta_port, meta_cl_...
#include "../../debug.hpp"                                 // Msg, INFO_MSG, INFO_MSG_INDENT, DEBUG_MSG, DEBUG_MSG_INDENT
#include "../../utilities/algorithm.hpp"                   // util::erase, util::eraseIf, util::findIf
#include "../game.hpp"                                     // Game

#include <algorithm>    // std::remove_if
#include <chrono>       // std::chrono::..
#include <cstddef>      // std::byte, std::size_t, std::ptrdiff_t
#include <fmt/core.h>   // fmt::format
#include <system_error> // std::error_code

MetaClient::MetaClient(Game& game)
	: m_game(game)
	, m_metaServerConnection(m_socket, net::Duration{}, 0, 0, MessageHandler{*this})
	, m_currentGameServer(m_gameServerConnections.end()) {
	this->updateTimeout();
	this->updateThrottle();
	this->updateSendInterval();
}

auto MetaClient::init() -> bool {
	INFO_MSG(Msg::CLIENT, "Meta client: Initializing...");

	auto ec = std::error_code{};

	// Bind socket.
	m_socket.bind(net::IpEndpoint{net::IpAddress::any(), static_cast<net::PortNumber>(meta_cl_port)}, ec);
	if (ec) {
		if (meta_cl_port == 0) {
			m_game.warning(fmt::format("Failed to bind client socket: {}", ec.message()));
		} else {
			m_game.warning(fmt::format("Failed to bind client socket using port {}: {}", meta_cl_port, ec.message()));
		}
		return false;
	}

	return true;
}

auto MetaClient::shutDown() -> void { // NOLINT(readability-convert-member-functions-to-static)
	INFO_MSG(Msg::CLIENT, "Meta client shutting down.");
}

auto MetaClient::stop(std::string_view message) -> bool {
	if (!m_stopping) {
		INFO_MSG_INDENT(Msg::CLIENT, "Meta client: Shutting down. Message: \"{}\".", message) {
			m_stopping = true;
			if (message.empty()) {
				message = "Meta client shutting down.";
			}
			if (m_metaServerConnection.connecting()) {
				m_metaServerConnection.close(message);
			} else {
				m_metaServerConnection.disconnect(message);
			}
			for (auto& gameServer : m_gameServerConnections) {
				gameServer.connection.disconnect("Meta client shutting down.");
			}
		}
		return true;
	}
	INFO_MSG(Msg::CLIENT, "Meta client: Tried to stop when already stopping. Message: \"{}\".", message);
	return false;
}

auto MetaClient::update(float deltaTime) -> bool {
	DEBUG_MSG_INDENT(Msg::CLIENT_TICK | Msg::CONNECTION_DETAILED, "META CLIENT @ {} ms", deltaTime * 1000.0f) {
		if (m_stopping) {
			// Wait for all connections to close.
			if (m_metaServerConnection.disconnected() && m_gameServerConnections.empty()) {
				return false;
			}
		}
		this->receivePackets();
		this->updateConnections();

		if (m_sendTimer.advance(deltaTime, m_sendInterval)) {
			DEBUG_MSG_INDENT(Msg::CLIENT_TICK | Msg::CONNECTION_DETAILED, "Meta client: Performing send.") {
				const auto now = net::Clock::now();
				for (auto& gameServer : m_gameServerConnections) {
					if (gameServer.metaInfoRequestSent && now - gameServer.metaInfoRequestSendTime > gameServer.connection.getTimeout()) {
						gameServer.connection.disconnect("Meta info request timed out.");
					}
				}

				util::eraseIf(m_gameServerCooldowns, [now](const auto& kv) { return now >= kv.second.endTime; });

				const auto currentFetchCount = m_gameServerConnections.size();
				const auto maxFetchCount = static_cast<std::size_t>(static_cast<int>(meta_cl_max_server_connections));
				if (currentFetchCount < maxFetchCount) {
					const auto remainingFetchCount = maxFetchCount - currentFetchCount;
					const auto pendingEndpointCount = m_pendingGameServerEndpoints.size();
					const auto fetchCount = std::min(remainingFetchCount, pendingEndpointCount);
					const auto fetchBegin = m_pendingGameServerEndpoints.begin();
					const auto fetchEnd = fetchBegin + static_cast<std::ptrdiff_t>(fetchCount);
					m_pendingGameServerEndpoints.erase(
						std::remove_if(fetchBegin, fetchEnd, [&](const auto endpoint) { return this->connectPending(endpoint); }),
						fetchEnd);
				}

				this->sendPackets();
			}
		}
	}
	return true;
}

auto MetaClient::updateTimeout() -> void {
	const auto timeout = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(meta_cl_timeout)});
	m_metaServerConnection.setTimeout(timeout);
	for (auto& gameServer : m_gameServerConnections) {
		gameServer.connection.setTimeout(timeout);
	}
}

auto MetaClient::updateThrottle() -> void {
	m_metaServerConnection.setThrottleMaxSendBufferSize(meta_cl_throttle_limit);
	m_metaServerConnection.setThrottleMaxPeriod(meta_cl_throttle_max_period);
	for (auto& gameServer : m_gameServerConnections) {
		gameServer.connection.setThrottleMaxSendBufferSize(meta_cl_throttle_limit);
		gameServer.connection.setThrottleMaxPeriod(meta_cl_throttle_max_period);
	}
}

auto MetaClient::updateSendInterval() -> void {
	m_sendInterval = 1.0f / static_cast<float>(meta_cl_sendrate);
	m_sendTimer.reset();
}

auto MetaClient::getStatusString() const -> std::string {
	static constexpr auto formatConnection = [](const auto& connection) {
		const auto pingMilliseconds =
			std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(connection.getLatestMeasuredPingDuration()).count();
		return fmt::format(
			"\"{}\"\n"
			"  Latency: {} ms\n"
			"  Packets sent: {}\n"
			"  Packets received: {}\n"
			"  Reliable packets written: {}\n"
			"  Reliable packets received: {}\n"
			"  Reliable packets received out of order: {}\n"
			"  Send rate throttled: {}\n"
			"  Packet send errors: {}\n"
			"  Invalid message types received: {}\n"
			"  Invalid message payloads received: {}\n"
			"  Invalid packet headers received: {}",
			std::string{connection.getRemoteEndpoint()},
			pingMilliseconds,
			connection.getStats().packetsSent,
			connection.getStats().packetsReceived,
			connection.getStats().reliablePacketsWritten,
			connection.getStats().reliablePacketsReceived,
			connection.getStats().reliablePacketsReceivedOutOfOrder,
			connection.getStats().sendRateThrottleCount,
			connection.getStats().packetSendErrorCount,
			connection.getStats().invalidMessageTypeCount,
			connection.getStats().invalidMessagePayloadCount,
			connection.getStats().invalidPacketHeaderCount);
	};

	static constexpr auto formatGameServer = [](const auto& gameServer) {
		return formatConnection(gameServer.connection);
	};

	auto ec = std::error_code{};
	return fmt::format(
		"=== META CLIENT STATUS ===\n"
		"Local address: \"{}:{}\"\n"
		"Send rate: {} Hz\n"
		"Meta server connection:\n"
		"{}\n"
		"\n"
		"Game server connections:\n"
		"{}\n"
		"==========================",
		std::string{net::IpAddress::getLocalAddress(ec)},
		m_socket.getLocalEndpoint(ec).getPort(),
		meta_cl_sendrate,
		formatConnection(m_metaServerConnection),
		m_gameServerConnections | util::transform(formatGameServer) | util::join("\n\n"));
}

auto MetaClient::refresh() noexcept -> bool {
	m_pendingGameServerEndpoints.clear();
	m_gameServerEndpoints.clear();
	m_metaInfo.clear();
	m_hasReceivedGameServerEndpoints = false;
	if (m_stopping || m_metaServerConnection.disconnecting()) {
		return false;
	}

	if (m_metaServerConnection.connected()) {
		if (!this->writeToMetaServer(msg::meta::sv::out::GameServerAddressListRequest{{}})) {
			INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT, "Meta client: Failed to write game server address list request.");
			return false;
		}
	} else if (m_metaServerConnection.disconnected()) {
		auto ec = std::error_code{};
		const auto ip = net::IpAddress::resolve(std::string{meta_address}.c_str(), ec);
		if (ec) {
			m_game.warning(fmt::format("Couldn't resolve ip address \"{}\": {}", meta_address, ec.message()));
			return false;
		}
		// Initialize connection.
		const auto endpoint = net::IpEndpoint{ip, static_cast<net::PortNumber>(meta_port)};
		if (!m_metaServerConnection.connect(endpoint)) {
			m_game.error(fmt::format("Failed to intialize meta client connection to meta server: {}", m_metaServerConnection.getDisconnectMessage()));
			return false;
		}
	}
	return true;
}

auto MetaClient::isConnecting() const noexcept -> bool {
	return m_metaServerConnection.connecting();
}

auto MetaClient::hasReceivedGameServerEndpoints() const noexcept -> bool {
	return m_hasReceivedGameServerEndpoints;
}

auto MetaClient::metaInfo() const noexcept -> util::Span<const ReceivedMetaInfo> {
	return m_metaInfo;
}

auto MetaClient::gameServerEndpoints() const noexcept -> util::Span<const net::IpEndpoint> {
	return m_gameServerEndpoints;
}

auto MetaClient::handleMessage(net::msg::in::Connect&&) -> void {
	if (m_currentGameServer == m_gameServerConnections.end()) {
		INFO_MSG(Msg::CLIENT, "Meta client: Meta server \"{}\" connected.", std::string{m_metaServerConnection.getRemoteEndpoint()});
		if (!this->writeToMetaServer(msg::meta::sv::out::GameServerAddressListRequest{{}})) {
			m_metaServerConnection.disconnect("Failed to write game server address list request.");
		}
	} else {
		INFO_MSG(Msg::CLIENT, "Meta client: Game server \"{}\" connected.", std::string{m_currentGameServer->connection.getRemoteEndpoint()});
		if (!m_currentGameServer->write(msg::sv::out::MetaInfoRequest{{}})) {
			m_currentGameServer->connection.disconnect("Failed to write meta info request.");
		} else {
			m_currentGameServer->metaInfoRequestWritten = true;
		}
	}
}

auto MetaClient::handleMessage(msg::meta::cl::in::GameServerAddressList&& msg) -> void {
	if (m_currentGameServer == m_gameServerConnections.end()) {
		m_gameServerEndpoints = std::move(msg.endpoints);
		m_pendingGameServerEndpoints = m_gameServerEndpoints;
		m_hasReceivedGameServerEndpoints = true;
	} else {
		INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT,
				 "Meta client: Received unrequested game server address list from bad game server \"{}\".",
				 std::string{m_currentGameServer->connection.getRemoteEndpoint()});
		m_currentGameServer->connection.disconnect("Invalid message.");
	}
}

auto MetaClient::handleMessage(msg::meta::cl::in::MetaInfo&& msg) -> void {
	if (m_currentGameServer == m_gameServerConnections.end()) {
		INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT,
				 "Meta client: Received unrequested meta info from bad meta server \"{}\".",
				 std::string{m_metaServerConnection.getRemoteEndpoint()});
		this->stop("Invalid message received from meta server.");
	} else {
		if (!m_currentGameServer->connection.disconnecting()) {
			const auto now = net::Clock::now();
			const auto ping = now - m_currentGameServer->metaInfoRequestSendTime;
			const auto endpoint = m_currentGameServer->connection.getRemoteEndpoint();
			INFO_MSG(Msg::CLIENT, "Meta client: Received meta info from game server \"{}\".", std::string{endpoint});
			m_metaInfo.emplace_back(std::move(msg), endpoint, ping);
			util::erase(m_pendingGameServerEndpoints, endpoint);
			m_currentGameServer->connection.disconnect("Meta info fetch finished.");
		}
	}
}

auto MetaClient::receivePackets() -> void {
	auto buffer = std::vector<std::byte>(net::MAX_PACKET_SIZE);
	while (true) {
		auto ec = std::error_code{};
		auto remoteEndpoint = net::IpEndpoint{};
		const auto receivedBytes = m_socket.receiveFrom(remoteEndpoint, buffer, ec).size();
		if (ec) {
			if (ec != net::SocketError::WAIT) {
				DEBUG_MSG(Msg::CLIENT, "Meta client: Failed to receive packet: {}", ec.message());
			}
			break;
		}
		if (remoteEndpoint == m_metaServerConnection.getRemoteEndpoint()) {
			buffer.resize(receivedBytes);
			m_metaServerConnection.receivePacket(std::move(buffer));
			buffer.resize(net::MAX_PACKET_SIZE);
		} else if (const auto it = this->findGameServer(remoteEndpoint); it != m_gameServerConnections.end()) {
			buffer.resize(receivedBytes);
			it->connection.receivePacket(std::move(buffer));
			buffer.resize(net::MAX_PACKET_SIZE);
		}
	}
}

auto MetaClient::updateConnections() -> void {
	if (!m_metaServerConnection.disconnected()) {
		if (!m_metaServerConnection.update()) {
			if (m_metaServerConnection.getDisconnectMessage() == Connection::HANDSHAKE_TIMED_OUT_MESSAGE) {
				m_game.warning(
					"Failed to connect to the meta server.\n"
					"Cannot fetch game server list.");
			}
		}
	}

	for (auto it = m_gameServerConnections.begin(); it != m_gameServerConnections.end();) {
		m_currentGameServer = it;
		if (!it->connection.update()) {
			INFO_MSG(Msg::CLIENT,
					 "Meta client: Game server \"{}\" was dropped.{}",
					 std::string{it->connection.getRemoteEndpoint()},
					 (it->connection.getDisconnectMessage().empty()) ? "" : fmt::format(" Reason: {}", it->connection.getDisconnectMessage()));
			const auto [itCooldown, inserted] = m_gameServerCooldowns.try_emplace(it->connection.getRemoteEndpoint());
			itCooldown->second.endTime = net::Clock::now() + net::DISCONNECT_DURATION;

			it = m_gameServerConnections.erase(it);
		} else {
			++it;
		}
	}
	m_currentGameServer = m_gameServerConnections.end();
}

auto MetaClient::sendPackets() -> void {
	m_metaServerConnection.sendPackets();
	for (auto& gameServer : m_gameServerConnections) {
		if (gameServer.metaInfoRequestWritten) {
			gameServer.metaInfoRequestWritten = false;
			gameServer.metaInfoRequestSent = true;
			gameServer.metaInfoRequestSendTime = net::Clock::now();
		}
		gameServer.connection.sendPackets();
	}
}

auto MetaClient::findGameServer(net::IpEndpoint endpoint) noexcept -> GameServers::iterator {
	return util::findIf(m_gameServerConnections,
						[endpoint](const auto& gameServer) { return gameServer.connection.getRemoteEndpoint() == endpoint; });
}

auto MetaClient::connectPending(net::IpEndpoint endpoint) -> bool {
	if (m_gameServerCooldowns.count(endpoint) != 0) {
		return false;
	}
	if (this->findGameServer(endpoint) == m_gameServerConnections.end()) {
		INFO_MSG(Msg::CLIENT, "Meta client: Fetching meta info from game server \"{}\"...", std::string{endpoint});
		const auto timeout = std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(meta_cl_timeout)});
		auto& gameServer = m_gameServerConnections.emplace_back(m_socket, timeout, meta_cl_throttle_limit, meta_cl_throttle_max_period, *this);
		if (!gameServer.connection.connect(endpoint)) {
			INFO_MSG(Msg::CLIENT,
					 "Meta client: Failed to initialize connection to game server \"{}\": {}",
					 std::string{endpoint},
					 gameServer.connection.getDisconnectMessage());
			m_gameServerConnections.pop_back();
		}
	}
	return true;
}
