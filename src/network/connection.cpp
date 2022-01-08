#include "connection.hpp"

#include <stdexcept>    // std::length_error
#include <system_error> // std::error_code

namespace net {

auto sanitizeMessage(std::string_view message, bool allowNewlines) -> std::string {
	auto result = std::string{};
	const auto clampedLength = std::min(message.size(), net::MAX_CHAT_MESSAGE_LENGTH);
	result.reserve(clampedLength);
	for (const auto ch : message.substr(0, clampedLength)) {
		if (ch == '\n') {
			result.push_back((allowNewlines) ? '\n' : ' ');
		} else if (ch >= ' ' && ch <= '~') {
			result.push_back(ch);
		}
	}
	return result;
}

NetChannel::NetChannel(util::Span<const MessageHandler> messageHandlers, ConnectedCallback onConnected, UDPSocket& socket, Duration timeout,
                       int throttleMaxSendBufferSize, int throttleMaxPeriod)
	: m_messageHandlers(messageHandlers)
	, m_onConnected(onConnected)
	, m_socket(socket)
	, m_timeout(timeout)
	, m_throttleMaxSendBufferSize(throttleMaxSendBufferSize)
	, m_throttleMaxPeriod(throttleMaxPeriod) {}

auto NetChannel::getRemoteAddress() const noexcept -> IpAddress {
	return m_endpoint.getAddress();
}

auto NetChannel::getRemotePort() const noexcept -> PortNumber {
	return m_endpoint.getPort();
}

auto NetChannel::getRemoteEndpoint() const noexcept -> IpEndpoint {
	return m_endpoint;
}

auto NetChannel::getLatestMeasuredPingDuration() const noexcept -> Duration {
	return m_latestMeasuredPingDuration;
}

auto NetChannel::getTimeout() const noexcept -> Duration {
	return m_timeout;
}

auto NetChannel::getThrottleMaxSendBufferSize() const noexcept -> int {
	return m_throttleMaxSendBufferSize;
}

auto NetChannel::getThrottleMaxPeriod() const noexcept -> int {
	return m_throttleMaxPeriod;
}

auto NetChannel::connecting() const noexcept -> bool {
	return m_state == State::HANDSHAKE_PART1 || m_state == State::HANDSHAKE_PART2 || m_state == State::HANDSHAKE_PART3 || m_state == State::CONNECTING;
}

auto NetChannel::connected() const noexcept -> bool {
	return m_state == State::CONNECTED;
}

auto NetChannel::disconnecting() const noexcept -> bool {
	return m_state == State::DISCONNECTING_MYSELF || m_state == State::DISCONNECTING;
}

auto NetChannel::disconnected() const noexcept -> bool {
	return m_state == State::DISCONNECTED;
}

auto NetChannel::getDisconnectMessage() const noexcept -> std::string_view {
	return m_disconnectMessage;
}

auto NetChannel::getStats() const noexcept -> const ConnectionStats& {
	return m_stats;
}

auto NetChannel::setTimeout(Duration timeout) noexcept -> void {
	m_timeout = timeout;
}

auto NetChannel::setThrottleMaxSendBufferSize(int throttleMaxSendBufferSize) noexcept -> void {
	m_throttleMaxSendBufferSize = throttleMaxSendBufferSize;
}

auto NetChannel::setThrottleMaxPeriod(int throttleMaxPeriod) noexcept -> void {
	m_throttleMaxPeriod = throttleMaxPeriod;
}

auto NetChannel::connect(IpEndpoint endpoint) -> bool {
	return this->initializeConnection(false, endpoint);
}

auto NetChannel::accept(IpEndpoint endpoint) -> bool {
	return this->initializeConnection(true, endpoint);
}

auto NetChannel::reset() noexcept -> void {
	this->resetStats();
	m_receiveBuffer.clear();
	m_sendBuffer.clear();
	m_receivedPackets.clear();
	m_bufferedMessages.clear();
	m_pingTimeBuffer.clear();
	m_disconnectMessage.clear();
	m_latestSeqSent = 0;
	m_latestSeqHandled = 0;
	m_latestAckReceived = Acknowledgement{};
	m_state = State::DISCONNECTED;
}

auto NetChannel::resetStats() noexcept -> void {
	m_stats = ConnectionStats{};
}

auto NetChannel::close() -> bool {
	if (!this->disconnected()) {
		INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" closed.", std::string{this->getRemoteEndpoint()});
		m_state = State::DISCONNECTED;
		m_disconnectTime = Clock::now();
		return true;
	}
	return false;
}

auto NetChannel::close(std::string_view message) -> bool {
	if (!this->disconnected()) {
		INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" closed. Message: {}", std::string{this->getRemoteEndpoint()}, message);
		m_state = State::DISCONNECTED;
		m_disconnectTime = Clock::now();
		m_disconnectMessage = message;
		return true;
	}
	INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" tried to close when it was already closed. Message: {}", std::string{this->getRemoteEndpoint()}, message);
	return false;
}

auto NetChannel::disconnect(std::string_view message, Duration delay) -> bool {
	assert(delay > Duration::zero());

	if (!this->disconnected()) {
		if (m_state != State::DISCONNECTING_MYSELF) {
			INFO_MSG_INDENT(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" disconnected. Message: {}", std::string{this->getRemoteEndpoint()}, message) {
				m_state = State::DISCONNECTING_MYSELF;
				m_disconnectTime = Clock::now() + std::min(delay, m_timeout);
				m_disconnectMessage = message;
				if (!this->write(msg::out::Disconnect{{}, message})) {
					this->close();
					return false;
				}
			}
			return true;
		}
		INFO_MSG(Msg::CONNECTION_EVENT,
		         "NetChannel to \"{}\" tried to disconnect when it was already disconnecting. Message: {}",
		         std::string{this->getRemoteEndpoint()},
		         message);
		return false;
	}
	INFO_MSG(Msg::CONNECTION_EVENT,
	         "NetChannel to \"{}\" tried to disconnect when it was already closed. Message: {}",
	         std::string{this->getRemoteEndpoint()},
	         message);
	return false;
}

auto NetChannel::update() -> bool {
	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Updating connection.") {
		// Check if we should disconnect.
		if (!this->checkConnection()) {
			return false;
		}

		// Process all the packets we received since the last update.
		this->processReceivedPackets();
		return true;
	}
}

auto NetChannel::sendPackets() -> void {
	if (this->disconnected()) {
		return;
	}

	// Write pings periodically.
	if (const auto now = Clock::now(); now >= m_nextPingMeasureTime) {
		if (this->connected()) {
			if (this->write(msg::out::Ping{{}})) {
				m_pingTimeBuffer.push_back(now);
			} else {
				DEBUG_MSG(Msg::CONNECTION_EVENT, "Failed to write ping.");
			}
		}
		do {
			m_nextPingMeasureTime += PING_INTERVAL;
		} while (now >= m_nextPingMeasureTime);
	}

	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Sending packets.") {
		switch (this->send()) {
			case SendStatus::SUCCESS: break;
			case SendStatus::PACKET_SEND_FAILED: this->close("Failed to send packets."); break;
			case SendStatus::SEND_BUFFER_OVERFLOW: this->close("Send buffer overflow."); break;
			case SendStatus::ENCRYPTION_FAILED: this->close("Failed to encrypt packet."); break;
		}
	}
}

auto NetChannel::receivePacket(std::vector<std::byte> data) -> bool {
	if (this->disconnected()) {
		return false;
	}

	try {
		++m_stats.packetsReceived;
		m_stats.bytesReceived += static_cast<decltype(m_stats.bytesReceived)>(data.size());
		m_receivedPackets.push_back(std::move(data));
	} catch ([[maybe_unused]] const std::bad_alloc& e) {
		DEBUG_MSG(Msg::CONNECTION_EVENT, "Failed to receive packet ({} bytes) ({})!", data.size(), e.what());
		++m_stats.allocationErrorCount;
		return false;
	}
	return true;
}

auto NetChannel::handleMessage(msg::in::HandshakePart1&& msg) -> void {
	assert(!this->disconnected());

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
	                "NetChannel to \"{}\" received remote public key and handshake token.",
	                std::string{this->getRemoteEndpoint()}) {
		if (m_state != State::HANDSHAKE_PART1) {
			++m_stats.invalidMessageOrderCount;
			this->disconnect("Invalid handshake sequence.");
			return;
		}
	}

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "NetChannel to \"{}\" generating session keys.", std::string{this->getRemoteEndpoint()}) {
		if (m_serverSide) {
			if (!crypto::kx::generateSessionKeysServerSide(m_receiveKey, m_sendKey, m_publicKey, m_secretKey, msg.publicKey)) {
				this->disconnect("Suspicious client public key.");
				return;
			}
		} else {
			if (!crypto::kx::generateSessionKeysClientSide(m_receiveKey, m_sendKey, m_publicKey, m_secretKey, msg.publicKey)) {
				this->disconnect("Suspicious server public key.");
				return;
			}
		}
	}

	auto header = crypto::Stream::Header{};
	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "NetChannel to \"{}\" initializing send stream.", std::string{this->getRemoteEndpoint()}) {
		if (!m_sendStream.init(header, m_sendKey)) {
			this->disconnect("Failed to initialize secret stream header.");
			return;
		}
	}

	if (!this->write(msg::out::HandshakePart2{{}, header})) {
		this->close("Failed to write handshake message.");
		return;
	}

	m_remoteHandshakeToken = msg.token;

	m_state = State::HANDSHAKE_PART2;
}

auto NetChannel::handleMessage(msg::in::HandshakePart2&& msg) -> void {
	assert(!this->disconnected());

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
	                "NetChannel to \"{}\" received receive stream header.",
	                std::string{this->getRemoteEndpoint()}) {
		if (m_state != State::HANDSHAKE_PART2) {
			++m_stats.invalidMessageOrderCount;
			this->disconnect("Invalid handshake sequence.");
			return;
		}
	}

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
	                "NetChannel to \"{}\" initializing receive stream.",
	                std::string{this->getRemoteEndpoint()}) {
		if (!m_receiveStream.init(msg.header, m_receiveKey)) {
			this->disconnect("Invalid secret stream header.");
			return;
		}
	}

	if (!this->write(msg::out::HandshakePart3{{}, m_remoteHandshakeToken})) {
		this->close("Failed to write handshake message.");
		return;
	}

	m_state = State::HANDSHAKE_PART3;
}

auto NetChannel::handleMessage(msg::in::HandshakePart3&& msg) -> void {
	assert(!this->disconnected());

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "NetChannel to \"{}\" received handshake token.", std::string{this->getRemoteEndpoint()}) {
		if (m_state != State::HANDSHAKE_PART3) {
			++m_stats.invalidMessageOrderCount;
			this->disconnect("Invalid handshake sequence.");
			return;
		}
	}

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
	                "NetChannel to \"{}\" verifying handshake token.",
	                std::string{this->getRemoteEndpoint()}) {
		if (!crypto::verifyAccessToken(m_localHandshakeToken, msg.token)) {
			this->disconnect("Invalid handshake token.");
			return;
		}
	}

	if (!this->write(msg::out::Connect{{}})) {
		this->close("Failed to write handshake message.");
		return;
	}

	m_state = State::CONNECTING;
}

auto NetChannel::handleMessage(msg::in::Connect&& msg) -> void {
	assert(!this->disconnected());

	if (m_state != State::CONNECTING) {
		++m_stats.invalidMessageOrderCount;
		this->disconnect("Invalid handshake sequence.");
		return;
	}

	m_disconnectMessage.clear();
	m_state = State::CONNECTED;
	assert(m_onConnected);
	m_onConnected(*this, std::move(msg)); // NOLINT(performance-move-const-arg)

	INFO_MSG(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
	         "NetChannel to \"{}\" handshake completed successfully.",
	         std::string{this->getRemoteEndpoint()});
}

auto NetChannel::handleMessage(msg::in::Disconnect&& msg) -> void {
	assert(!this->disconnected());

	if (!this->disconnecting()) {
		INFO_MSG_INDENT(Msg::CONNECTION_EVENT,
		                "NetChannel to \"{}\" received disconnect message. Message: {}",
		                std::string{this->getRemoteEndpoint()},
		                msg.message) {
			m_state = State::DISCONNECTING;
			m_disconnectTime = Clock::now() + std::min(DISCONNECT_DURATION, m_timeout);
			m_disconnectMessage = sanitizeMessage(msg.message);
		}
	} else {
		INFO_MSG(Msg::CONNECTION_EVENT,
		         "NetChannel to \"{}\" received disconnect message when it was already disconnecting. Message: {}",
		         std::string{this->getRemoteEndpoint()},
		         msg.message);
		++m_stats.invalidMessageOrderCount;
	}
	if (!this->write(msg::out::Close{{}})) {
		INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" failed to write close message.", std::string{this->getRemoteEndpoint()});
		this->close();
	}
}

auto NetChannel::handleMessage(msg::in::Close&&) -> void {
	assert(!this->disconnected());

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" received close message.", std::string{this->getRemoteEndpoint()}) {
		if (m_serverSide) {
			if (m_state != State::DISCONNECTING_MYSELF) {
				m_state = State::DISCONNECTING_MYSELF;
				m_disconnectTime = Clock::now() + std::min(DISCONNECT_DURATION, m_timeout);
			}
			if (!this->write(msg::out::Close{{}})) {
				INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" failed to write close message.", std::string{this->getRemoteEndpoint()});
				this->close();
			}
		} else {
			m_state = State::DISCONNECTING;
			m_disconnectTime = Clock::now();
		}
	}
}

auto NetChannel::handleMessage(msg::in::Ping&&) -> void {
	assert(!this->disconnected());

	if (!this->write(msg::out::Pong{{}})) {
		this->close("Failed to write pong.");
	}
}

auto NetChannel::handleMessage(msg::in::Pong&&) -> void {
	assert(!this->disconnected());

	if (m_pingTimeBuffer.empty()) {
		INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" received invalid pong.", std::string{this->getRemoteEndpoint()});
		++m_stats.invalidMessageOrderCount;
		return;
	}

	const auto now = Clock::now();

	m_latestMeasuredPingDuration = now - m_pingTimeBuffer.front();
	m_pingTimeBuffer.erase(m_pingTimeBuffer.begin());
}

auto NetChannel::handleMessage(msg::in::EncryptedMessage&& msg) -> void {
	assert(!this->disconnected());

	const auto messageSize = msg.cipherText.size() - crypto::Stream::MESSAGE_ADDED_BYTES;
	DEBUG_MSG_INDENT(Msg::CONNECTION_CRYPTO, "Received encrypted message ({} bytes).", messageSize) {
		auto secretMessage = std::vector<std::byte>{};
		try {
			secretMessage.resize(messageSize);
		} catch ([[maybe_unused]] const std::bad_alloc& e) {
			DEBUG_MSG(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "Failed to allocate secret message: {}", e.what());
			++m_stats.allocationErrorCount;
			return;
		}

		if (!m_receiveStream.pull(secretMessage, util::Span{msg.cipherText})) {
			INFO_MSG(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
			         "NetChannel to \"{}\" failed to decrypt message: Invalid/incomplete/corrupted ciphertext ({} bytes).",
			         std::string{this->getRemoteEndpoint()},
			         messageSize);
			++m_stats.invalidEncryptedMessageCount;
			return;
		}

		auto secretMessageStream = ByteInputStream{secretMessage};
		if (auto type = MessageType{}; secretMessageStream >> type) {
			if (type >= m_messageHandlers.size()) {
				INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" received invalid secret message type.", std::string{this->getRemoteEndpoint()});
				++m_stats.invalidMessageTypeCount;
				return;
			}

			if (type == message_type_of_v<msg::in::EncryptedMessage, NetChannelInputMessages>) {
				INFO_MSG(Msg::CONNECTION_EVENT,
				         "NetChannel to \"{}\" received recursively encrypted message. Ignoring.",
				         std::string{this->getRemoteEndpoint()});
				++m_stats.invalidMessageTypeCount;
				return;
			}

			m_messageHandlers[type](*this, secretMessageStream);
		} else {
			INFO_MSG(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
			         "NetChannel to \"{}\" failed to read secret message type ({} bytes).",
			         std::string{this->getRemoteEndpoint()},
			         messageSize);
			++m_stats.invalidEncryptedMessageCount;
			return;
		}
	}
}

auto NetChannel::encryptMessage(std::vector<std::byte>& output, util::Span<const std::byte> secretMessage) -> bool {
	assert(!this->disconnected());
	assert(secretMessage.size() <= crypto::Stream::MAX_MESSAGE_SIZE);

	try {
		const auto cipherTextSize = secretMessage.size() + crypto::Stream::MESSAGE_ADDED_BYTES;
		output.resize(cipherTextSize);
		if (!m_sendStream.push(output, secretMessage)) {
			return false;
		}
		DEBUG_MSG(Msg::CONNECTION_CRYPTO, "Encrypted secret message ({} bytes).", secretMessage.size());
	} catch ([[maybe_unused]] const std::bad_alloc& e) {
		DEBUG_MSG(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "Failed to encrypt secret message: {}", e.what());
		++m_stats.encryptionErrorCount;
		return false;
	}
	return true;
}

auto NetChannel::initializeConnection(bool serverSide, IpEndpoint endpoint) -> bool {
	this->reset();

	m_serverSide = serverSide;
	m_endpoint = endpoint;

	const auto now = Clock::now();

	m_latestPacketReceiveTime = now;
	m_nextPingMeasureTime = now + PING_INTERVAL;
	m_latestMeasuredPingDuration = Duration::zero();

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "NetChannel to \"{}\" initializing...", std::string{this->getRemoteEndpoint()}) {
		if (!crypto::init()) {
			this->close("Failed to initialize crypto library.");
			return false;
		}
	}

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "NetChannel to \"{}\" generating keypair.", std::string{this->getRemoteEndpoint()}) {
		if (!crypto::kx::generateKeypair(m_publicKey, m_secretKey)) {
			this->close("Failed to create cryptographic keys.");
			return false;
		}
	}

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO,
	                "NetChannel to \"{}\" generating handshake token.",
	                std::string{this->getRemoteEndpoint()}) {
		crypto::generateAccessToken(m_localHandshakeToken);
	}

	m_state = State::HANDSHAKE_PART1;

	INFO_MSG_INDENT(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "NetChannel to \"{}\" initiating handshake.", std::string{this->getRemoteEndpoint()}) {
		if (!this->write(msg::out::HandshakePart1{{}, m_publicKey, m_localHandshakeToken})) {
			this->close("Failed to write handshake message.");
			return false;
		}
	}

	m_disconnectTime = now + std::min(CONNECT_DURATION, m_timeout);
	m_disconnectMessage = HANDSHAKE_TIMED_OUT_MESSAGE;

	INFO_MSG(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "NetChannel to \"{}\" initialized.", std::string{this->getRemoteEndpoint()});
	return true;
}

auto NetChannel::checkConnection() -> bool {
	if (this->disconnected()) {
		return false;
	}

	const auto now = Clock::now();

	if (!this->connected() && now >= m_disconnectTime) {
		this->close();
		return false;
	}

	if (m_receivedPackets.empty()) {
		if (now >= m_latestPacketReceiveTime + m_timeout) {
			INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" timed out because we stopped receiving packets.", std::string{this->getRemoteEndpoint()});
			this->close("Connection timed out (not receiving packets).");
			return false;
		}
	} else {
		m_latestPacketReceiveTime = now;
	}

	if (m_pingTimeBuffer.size() * PING_INTERVAL > std::max(m_timeout, PING_INTERVAL * 2)) {
		INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" timed out because we stopped receiving pongs.", std::string{this->getRemoteEndpoint()});
		this->close("Connection timed out (not receiving ping responses).");
		return false;
	}
	return true;
}

auto NetChannel::processReceivedPackets() -> void {
	assert(!this->disconnected());

	auto shouldCheckSavedPackets = false;
	auto newestAck = m_latestAckReceived;
	for (auto& packet : m_receivedPackets) {
		auto packetStream = ByteInputStream{packet};
		if (auto header = PacketHeader{}; packetStream >> header) {
			DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Received packet {}.", header) {
				// Check the checksum of the packet.
				if (header.checksum != PacketHeader::calculateChecksum(packetStream)) {
					DEBUG_MSG(Msg::CONNECTION_EVENT, "Packet checksum was invalid. Discarding.");
					++m_stats.invalidPacketChecksumCount;
					continue;
				}

				// Check if this packet contains a new acknowledgement.
				if (const auto headerAck = Acknowledgement{header.ack, header.mask}; headerAck > newestAck) {
					newestAck = headerAck; // Update the newest acknowledgement.
				}

				if ((header.flags & PacketHeader::RELIABLE) != 0) {
					// Packet is reliable. Check if it has the expected sequence number.
					++m_stats.reliablePacketsReceived;
					auto expectedSeq = static_cast<SequenceNumber>(m_latestSeqHandled + 1); // Note: Expected to overflow.
					if (header.seq == expectedSeq) {
						DEBUG_MSG(Msg::CONNECTION_DETAILED, "Packet is reliable and new (#{}).", header.seq);
						if ((header.flags & PacketHeader::SPLIT) != 0) {
							// Packet is split. Save the payload for later.
							if (this->savePacket(header, std::vector<std::byte>{packetStream.begin(), packetStream.end()})) {
								shouldCheckSavedPackets = true;
							}
						} else {
							// Packet is complete. Handle its messages now.
							this->handleMessages(packetStream);
							++m_latestSeqHandled;
							++expectedSeq;
							shouldCheckSavedPackets = true;
						}
					} else {
						// Packet is reliable and out of order. Save the payload for later.
						DEBUG_MSG(Msg::CONNECTION_DETAILED,
						          "Packet is reliable and out of order (#{}) (expected #{}). Saving payload for later.",
						          header.seq,
						          expectedSeq);
						if (this->savePacket(header, std::vector<std::byte>{packetStream.begin(), packetStream.end()})) {
							++m_stats.reliablePacketsReceivedOutOfOrder;
							shouldCheckSavedPackets = true;
						}
					}
				} else {
					// Packet is unreliable. Handle its messages now.
					DEBUG_MSG(Msg::CONNECTION_DETAILED, "Packet is unreliable.");
					this->handleMessages(packetStream);
				}
			}
		} else {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Received invalid packet header.");
			++m_stats.invalidPacketHeaderCount;
		}
	}
	m_receivedPackets.clear();

	// If we received a new acknowledgement, remove the acknowledged packets from our send buffer.
	if (newestAck != m_latestAckReceived) {
		this->acknowledge(newestAck);
		m_latestAckReceived = newestAck;
	}

	// Check the packets that were received out of order.
	if (shouldCheckSavedPackets) {
		this->processSavedPackets();
	}
}

auto NetChannel::processSavedPackets() -> void {
	assert(!this->disconnected());

	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Checking saved packets...") {
		// Look for the next packet.
		auto expectedSeq = static_cast<SequenceNumber>(m_latestSeqHandled + 1); // Note: Expected to overflow.
		for (auto it = m_receiveBuffer.find(expectedSeq); it != m_receiveBuffer.end(); it = m_receiveBuffer.find(expectedSeq)) {
			if ((it->second.header.flags & PacketHeader::SPLIT) != 0) {
				DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED,
				                 "Next partial packet #{} was found! Checking for the rest of the pieces...",
				                 it->second.header.seq) {
					auto lastSeq = static_cast<SequenceNumber>(m_latestSeqHandled + 2); // Note: Expected to overflow.
					while (true) {
						it = m_receiveBuffer.find(lastSeq);
						if (it == m_receiveBuffer.end()) {
							DEBUG_MSG(Msg::CONNECTION_DETAILED, "Couldn't find the last piece.");
							return;
						}
						if ((it->second.header.flags & PacketHeader::LAST_PIECE) != 0) {
							DEBUG_MSG(Msg::CONNECTION_DETAILED, "Found the last piece.");
							++m_latestSeqHandled;
							++expectedSeq;
							it = m_receiveBuffer.find(m_latestSeqHandled);

							auto fullPayload = std::move(it->second.payload);
							m_receiveBuffer.erase(it);
							do {
								++m_latestSeqHandled;
								++expectedSeq;
								it = m_receiveBuffer.find(m_latestSeqHandled);
								fullPayload.insert(fullPayload.end(), it->second.payload.begin(), it->second.payload.end());
								m_receiveBuffer.erase(it);
							} while (m_latestSeqHandled != lastSeq);
							this->handleMessages(fullPayload);
							break;
						}
						DEBUG_MSG(Msg::CONNECTION_DETAILED, "Found another piece...");
						++lastSeq;
					}
				}
			} else {
				DEBUG_MSG(Msg::CONNECTION_DETAILED, "Next full packet #{} was found! Handling now...", it->second.header.seq);
				this->handleMessages(it->second.payload);
				++m_latestSeqHandled;
				++expectedSeq;
				m_receiveBuffer.erase(it);
			}
		}
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Next packet not found.");
	}
}

auto NetChannel::handleMessages(util::Span<const std::byte> payload) -> void {
	assert(!this->disconnected());

	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Reading messages...") {
		auto packetStream = ByteInputStream{payload};
		for (auto type = MessageType{}; packetStream >> type;) {
			if (type >= m_messageHandlers.size()) {
				INFO_MSG(Msg::CONNECTION_EVENT, "NetChannel to \"{}\" received invalid message type \"{}\".", std::string{this->getRemoteEndpoint()}, type);
				++m_stats.invalidMessageTypeCount;
				break;
			}
			m_messageHandlers[type](*this, packetStream);
		}
	}
}

auto NetChannel::acknowledge(Acknowledgement ack) -> void {
	assert(!this->disconnected());

	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Removing acked packets from send buffer...") {
		// Delete packets before and including the last one processed by the recepient.
		while (!m_sendBuffer.empty() && static_cast<SequenceDistance>(m_sendBuffer.front().header.seq - ack.ack) <= 0) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Removed outgoing packet #{}.", m_sendBuffer.front().header.seq);
			m_sendBuffer.pop_front();
		}

		// Mark packets that the recepient has received out of order as acked.
		auto i = SequenceNumber{1};
		while (ack.mask.any() && i < m_sendBuffer.size()) {
			if (ack.mask.test(0)) {
				if (!m_sendBuffer[i].acked) {
					DEBUG_MSG(Msg::CONNECTION_DETAILED, "Removed outgoing out-of-order packet #{}.", m_sendBuffer[i].header.seq);
				}
				m_sendBuffer[i].acked = true;
			}
			++i;
			ack.mask >>= 1;
		}
	}
}

auto NetChannel::savePacket(const PacketHeader& header, std::vector<std::byte> payload) -> bool { // NOLINT(performance-unnecessary-value-param)
	assert(!this->disconnected());

	if (m_receiveBuffer.empty()) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Trying to save packet #{}. Recv buffer is empty.", header.seq);
	} else {
		DEBUG_MSG(Msg::CONNECTION_DETAILED,
		          "Trying to save packet #{}. Recv buffer: {{size: {}, first: {}, last: {}}}.",
		          header.seq,
		          m_receiveBuffer.size(),
		          m_receiveBuffer.front().first,
		          m_receiveBuffer.back().first);
	}

	if (static_cast<SequenceDistance>(header.seq - m_latestSeqHandled) <= 0) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to save packet - already handled.");
		return false;
	}

	try {
		if (m_receiveBuffer.try_emplace(header.seq, header, std::move(payload)).second) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Successfully saved packet.");
			return true;
		}
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to save packet.");
	} catch ([[maybe_unused]] const std::bad_alloc& e) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to save packet: {}", e.what());
	} catch ([[maybe_unused]] const std::length_error& e) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to save packet: {}", e.what());
	}
	return false;
}

auto NetChannel::send() -> NetChannel::SendStatus {
	if (static_cast<int>(m_sendBuffer.size()) > m_throttleMaxSendBufferSize) {
		if (this->throttle()) {
			++m_stats.sendRateThrottleCount;
			return SendStatus::SUCCESS;
		}
	} else {
		m_throttlePeriod = 0;
		m_throttleCounter = 0;
	}

	const auto mask = this->getEarlyPacketMask();

	auto flags = PacketHeader::Flags{(mask == 0) ? PacketHeader::NONE : PacketHeader::EARLY_ACKS};
	auto payload = std::vector<std::byte>{};

	if (m_bufferedMessages.empty() && m_sendBuffer.empty()) {
		DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Nothing new to write. Sending empty packet.") {
			auto header = PacketHeader{};
			header.checksum = PacketHeader::calculateChecksum(payload);
			header.flags = flags;
			header.ack = m_latestSeqHandled;
			header.mask = mask;
			return this->sendPacket(header, payload);
		}
	}

	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Re-writing old un-acked reliable packets...") {
		for (const auto& packet : m_sendBuffer) {
			if (!packet.acked) {
				auto header = packet.header;
				if (mask != 0) {
					header.flags |= PacketHeader::EARLY_ACKS;
				} else {
					header.flags &= ~PacketHeader::EARLY_ACKS;
				}
				header.ack = m_latestSeqHandled;
				header.mask = mask;
				DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Re-writing reliable packet #{}.", header.seq) {
					if (const auto status = this->sendPacket(header, packet.payload); status != SendStatus::SUCCESS) {
						return status;
					}
				}
			}
		}
	}

	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Writing new messages...") {
		if (const auto status = this->writeMessages(flags, mask, payload, m_bufferedMessages); status != SendStatus::SUCCESS) {
			return status;
		}
	}

	if (!payload.empty()) {
		return this->sendPacket(flags, mask, std::move(payload));
	}
	return SendStatus::SUCCESS;
}

auto NetChannel::throttle() -> bool {
	if (m_throttlePeriod == 0) {
		if (m_throttlePeriod < m_throttleMaxPeriod) {
			m_throttlePeriod = std::min(static_cast<int>(m_sendBuffer.size()) / m_throttleMaxSendBufferSize, m_throttleMaxPeriod);
			return true;
		}
	} else {
		++m_throttleCounter;
		if (m_throttleCounter < m_throttlePeriod) {
			return true;
		}
		m_throttleCounter = 0;
		if (m_throttlePeriod < m_throttleMaxPeriod) {
			++m_throttlePeriod;
		}
	}
	return false;
}

auto NetChannel::writeMessages(PacketHeader::Flags& flags, PacketMask mask, std::vector<std::byte>& payload, std::vector<BufferedMessage>& messages)
	-> NetChannel::SendStatus {
	// Write messages.
	for (auto it = messages.begin(); it != messages.end();) {
		auto& message = *it;

		// Don't send non-NetChannel messages if we are not connected.
		if (!this->connected()) {
			auto type = MessageType{};
			auto messageStream = ByteInputStream{util::Span{message.data}.first(sizeof(MessageType))};
			messageStream >> type;
			if (!net::isNetChannelMessage(type)) {
				DEBUG_MSG(Msg::CONNECTION_DETAILED, "Ignoring non-NetChannel message because we are not connected ({}) bytes.", message.data.size());
				++it;
				continue;
			}
		}

		if (message.category == MessageCategory::SECRET) {
			constexpr auto type = message_type_of_v<msg::out::EncryptedMessage, NetChannelOutputMessages>;

			auto cipherText = std::vector<std::byte>{};
			if (!this->encryptMessage(cipherText, message.data)) {
				return SendStatus::ENCRYPTION_FAILED;
			}
			const auto encryptedMessage = msg::out::EncryptedMessage{{}, cipherText};
			message.data.clear();
			auto countStream = ByteCountStream{};
			countStream << type << encryptedMessage;
			message.data.reserve(countStream.capacity());
			auto messageStream = ByteOutputStream{message.data};
			messageStream << type << encryptedMessage;
			message.category = MessageCategory::RELIABLE; // Treat it as a reliable message from now on.
		}

		if (message.data.size() > MAX_PACKET_PAYLOAD_SIZE) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED,
			          "Message ({} bytes) is larger than the maximum message space of {} bytes. Splitting into multiple packets.",
			          message.data.size(),
			          MAX_PACKET_PAYLOAD_SIZE);
			if (const auto status = this->splitAndSendMessage(std::move(payload), flags, mask, message.data); status != SendStatus::SUCCESS) {
				return status;
			}
			payload.clear();
			flags &= ~PacketHeader::RELIABLE;
		} else {
			if (message.category == MessageCategory::RELIABLE) {
				flags |= PacketHeader::RELIABLE;
			}

			if (payload.size() + message.data.size() > MAX_PACKET_PAYLOAD_SIZE) {
				DEBUG_MSG(Msg::CONNECTION_DETAILED,
				          "Message ({} bytes) is too large to fit in remaining {} bytes of current packet payload. Sending another packet.",
				          message.data.size(),
				          MAX_PACKET_PAYLOAD_SIZE - payload.size());
				if (const auto status = this->sendPacket(flags, mask, std::move(payload)); status != SendStatus::SUCCESS) {
					return status;
				}
				payload.clear();
				flags &= ~PacketHeader::RELIABLE;
			}

			payload.insert(payload.end(), message.data.begin(), message.data.end());
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Wrote {} byte message.", message.data.size());
		}
		it = messages.erase(it);
	}
	return SendStatus::SUCCESS;
}

auto NetChannel::getEarlyPacketMask() const -> PacketMask {
	auto mask = PacketMask{};
	for (const auto& kv : m_receiveBuffer) {
		const auto i = static_cast<SequenceDistance>(kv.first - m_latestSeqHandled) - 1;
		if (i >= 0 && i < static_cast<SequenceDistance>(mask.size())) {
			mask.set(static_cast<std::size_t>(i));
		}
	}
	return mask;
}

auto NetChannel::sendPacket(PacketHeader::Flags flags, PacketMask mask, std::vector<std::byte>&& payload) -> NetChannel::SendStatus {
	auto header = PacketHeader{};
	header.checksum = PacketHeader::calculateChecksum(payload);
	header.flags = flags;
	header.ack = m_latestSeqHandled;
	header.mask = mask;
	if ((flags & PacketHeader::RELIABLE) != 0) {
		header.seq = ++m_latestSeqSent;
		if (const auto status = this->sendAndBufferPacket(header, std::move(payload)); status != SendStatus::SUCCESS) {
			return status;
		}
	} else {
		if (const auto status = this->sendPacket(header, payload); status != SendStatus::SUCCESS) {
			return status;
		}
	}
	return SendStatus::SUCCESS;
}

auto NetChannel::splitAndSendMessage(std::vector<std::byte>&& payload, PacketHeader::Flags flags, PacketMask mask,
                                     util::Span<const std::byte> message) -> NetChannel::SendStatus {
	auto i = MAX_PACKET_PAYLOAD_SIZE - payload.size();

	const auto pieces = 2 + ((message.size() - i - 1) / MAX_PACKET_PAYLOAD_SIZE);
	DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Splitting message into {} pieces.", pieces) {
		if (pieces > m_receiveBuffer.max_size()) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Sending {} pieces would cause a send buffer overflow.", pieces);
			++m_stats.packetSendErrorCount;
			return SendStatus::SEND_BUFFER_OVERFLOW;
		}

		payload.insert(payload.end(), message.begin(), message.begin() + i);
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Wrote first {} byte piece of split message.", i);

		auto header = PacketHeader{};
		header.checksum = PacketHeader::calculateChecksum(payload);
		header.flags = flags | PacketHeader::RELIABLE | PacketHeader::SPLIT;
		header.ack = m_latestSeqHandled;
		header.mask = mask;
		header.seq = ++m_latestSeqSent;
		if (const auto status = this->sendAndBufferPacket(header, std::move(payload)); status != SendStatus::SUCCESS) {
			return status;
		}
		payload.clear();

		for (; i + MAX_PACKET_PAYLOAD_SIZE < message.size(); i += MAX_PACKET_PAYLOAD_SIZE) {
			payload.insert(payload.end(), message.begin() + i, message.begin() + i + MAX_PACKET_PAYLOAD_SIZE);
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Wrote {} byte piece of split message.", MAX_PACKET_PAYLOAD_SIZE);

			header.checksum = PacketHeader::calculateChecksum(payload);
			header.seq = ++m_latestSeqSent;
			if (const auto status = this->sendAndBufferPacket(header, std::move(payload)); status != SendStatus::SUCCESS) {
				return status;
			}
			payload.clear();
		}

		payload.insert(payload.end(), message.begin() + i, message.end());
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Wrote final {} byte piece of split message.", message.size() - i);

		header.checksum = PacketHeader::calculateChecksum(payload);
		header.flags |= PacketHeader::LAST_PIECE;
		header.seq = ++m_latestSeqSent;
		if (const auto status = this->sendAndBufferPacket(header, std::move(payload)); status != SendStatus::SUCCESS) {
			return status;
		}
	}
	return SendStatus::SUCCESS;
}

auto NetChannel::sendAndBufferPacket(const PacketHeader& header, std::vector<std::byte>&& payload) -> NetChannel::SendStatus {
	try {
		const auto& packet = m_sendBuffer.emplace_back(header, std::move(payload));
		++m_stats.reliablePacketsWritten;
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Added packet to send buffer.");
		return this->sendPacket(packet.header, packet.payload);
	} catch ([[maybe_unused]] const std::bad_alloc& e) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to buffer and send reliable packet: {}", e.what());
		++m_stats.allocationErrorCount;
		++m_stats.sendBufferOverflowCount;
		++m_stats.packetSendErrorCount;
		return SendStatus::SEND_BUFFER_OVERFLOW;
	} catch ([[maybe_unused]] const std::length_error& e) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to buffer and send reliable packet: {}", e.what());
		++m_stats.sendBufferOverflowCount;
		++m_stats.packetSendErrorCount;
		return SendStatus::SEND_BUFFER_OVERFLOW;
	}
	++m_stats.packetSendErrorCount;
	DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to buffer and send reliable packet.");
	return SendStatus::PACKET_SEND_FAILED;
}

auto NetChannel::sendPacket(const PacketHeader& header, util::Span<const std::byte> payload) -> NetChannel::SendStatus {
	auto packet = std::vector<std::byte>{};
	auto countStream = ByteCountStream{};
	countStream << header;
	countStream.write(payload);
	packet.reserve(countStream.capacity());
	auto packetStream = ByteOutputStream{packet};
	packetStream << header;
	packetStream.write(payload);
	++m_stats.packetsSent;
	m_stats.bytesSent += static_cast<decltype(m_stats.bytesSent)>(packet.size());
	auto ec = std::error_code{};
	const auto bytesSent = m_socket->sendTo(m_endpoint, packet, ec);
	if (ec || bytesSent != packet.size()) {
		++m_stats.packetSendErrorCount;
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Failed to send packet.");
		return SendStatus::PACKET_SEND_FAILED;
	}
	DEBUG_MSG(Msg::CONNECTION_DETAILED, "Sent packet successfully.");
	return SendStatus::SUCCESS;
}

} // namespace net
