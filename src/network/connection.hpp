#ifndef AF2_NETWORK_CONNECTION_HPP
#define AF2_NETWORK_CONNECTION_HPP

#include "../debug.hpp"               // Msg, DEBUG_MSG, DEBUG_MSG_INDENT, INFO_MSG, DEBUG_TYPE_NAME
#include "../utilities/crc.hpp"       // util::CRC32
#include "../utilities/reference.hpp" // util::Reference
#include "../utilities/ring_map.hpp"  // util::RingMap
#include "../utilities/span.hpp"      // util::Span
#include "../utilities/type_list.hpp" // util::TypeList, util::typelist_..._v, util::typelist_..._t
#include "byte_stream.hpp"            // net::ByteInputStream, net::ByteOutputStream, net::ByteCountStream
#include "config.hpp"                 // net::Clock, net::TimePoint, net::Duration, net::...
#include "crypto.hpp"                 // crypto::...
#include "endpoint.hpp"               // net::IpEndpoint, net::IpAddress, net::PortNumber
#include "message.hpp"                // net::MessageDirection, net::MessageCategory, net::ReliableMessage, net::is_..._v
#include "message_layout.hpp"         // net::Big, net::String, net::List
#include "socket.hpp"                 // net::UDPSocket

#include <algorithm>     // std::min, std::max
#include <array>         // std::array
#include <bitset>        // std::bitset
#include <cassert>       // assert
#include <chrono>        // std::chrono::...
#include <climits>       // CHAR_BIT
#include <cstddef>       // std::byte, std::size_t
#include <cstdint>       // std::uint32_t, std::uint16_t, std::uint8_t
#include <deque>         // std::deque
#include <fmt/core.h>    // fmt::format
#include <fmt/ostream.h> // operator<<
#include <limits>        // std::numeric_limits
#include <new>           // std::bad_alloc
#include <ostream>       // std::ostream
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <type_traits>   // std::make_signed_t, std::is_signed_v, std::is_unsigned_v, std::enable_if_t, std::integral_constant
#include <utility>       // std::move, std::forward
#include <vector>        // std::vector

namespace net {

[[nodiscard]] auto sanitizeMessage(std::string_view message, bool allowNewlines = false) -> std::string;

using SequenceNumber = std::uint16_t;
using SequenceDistance = std::make_signed_t<SequenceNumber>;

static_assert(std::is_unsigned_v<SequenceNumber>); // Needed for defined overflow.
static_assert(std::is_signed_v<SequenceDistance> && sizeof(SequenceDistance) == sizeof(SequenceNumber)); // Needed to get the appropriate distance between two SequenceNumbers by subtracting and casting to SequenceDistance.

using PacketMask = std::bitset<PACKET_MASK_BYTES * CHAR_BIT>;
using MessageType = std::uint8_t;

struct Acknowledgement final {
	SequenceNumber ack = 0; // See: PacketHeader::ack.
	PacketMask mask = 0;    // See: PacketHeader::mask.

	[[nodiscard]] friend constexpr auto operator==(const Acknowledgement& lhs, const Acknowledgement& rhs) noexcept -> bool {
		return lhs.ack == rhs.ack && lhs.mask == rhs.mask;
	}

	[[nodiscard]] friend constexpr auto operator!=(const Acknowledgement& lhs, const Acknowledgement& rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] friend constexpr auto operator<(const Acknowledgement& lhs, const Acknowledgement& rhs) noexcept -> bool {
		if (lhs.ack == rhs.ack) {
			return lhs.mask.count() < rhs.mask.count();
		}
		return static_cast<SequenceDistance>(lhs.ack - rhs.ack) < 0;
	}

	[[nodiscard]] friend constexpr auto operator<=(const Acknowledgement& lhs, const Acknowledgement& rhs) noexcept -> bool {
		return !(rhs < lhs);
	}

	[[nodiscard]] friend constexpr auto operator>(const Acknowledgement& lhs, const Acknowledgement& rhs) noexcept -> bool {
		return rhs < lhs;
	}

	[[nodiscard]] friend constexpr auto operator>=(const Acknowledgement& lhs, const Acknowledgement& rhs) noexcept -> bool {
		return !(lhs < rhs);
	}
};

struct PacketHeader final {
	static constexpr auto PROTOCOL_ID = util::CRC32{std::array{
		std::byte{'A'},
		std::byte{'F'},
		std::byte{'2'},
		std::byte{'V'},
		std::byte{'2'},
	}};

	using Flags = std::uint8_t;
	enum Flag : Flags {
		NONE = 0,
		COMPRESSED = 1 << 1, // The packet payload data is compressed.
		RELIABLE = 1 << 2,   // This packet is reliable and contains a unique sequence number.
		SPLIT = 1 << 3,      // This packet is split into multiple pieces. This is one of the pieces. SPLIT implies RELIABLE.
		LAST_PIECE = 1 << 4, // This is the last piece of the split packet whose other pieces came before. LAST_PIECE implies SPLIT.
		EARLY_ACKS = 1 << 5, // This header contains a mask of packets received after ack. If this is not set, ack is the latest packet received.
	};

	// Always included:
	util::CRC32 checksum{}; // 32-bit checksum of PROTOCOL_ID and packet payload. Packet is dropped immediately if this is not correct.
	Flags flags = NONE;     // Flags that determine the structure of the rest of the header.
	SequenceNumber ack = 0; // The latest packet we have fully handled.
	// Included if EARLY_ACKS is set:
	PacketMask mask = 0; // Bitmask of packets received after ack. LSB is ack+2.
	// Included if RELIABLE is set:
	SequenceNumber seq = 0; // The sequence number of this packet.

	static constexpr auto MAX_SIZE = sizeof(checksum) + sizeof(flags) + sizeof(ack) + PACKET_MASK_BYTES + sizeof(seq);

	friend auto operator<<(std::ostream& stream, const PacketHeader& header) -> std::ostream& {
		return stream << fmt::format("{{checksum: {}, flags: {:#0{}x}, ack: {{{}, {:#0{}x}}}, seq: {}}}",
		                             header.checksum,
		                             header.flags,
		                             2 + 2 * sizeof(Flags),
		                             header.ack,
		                             header.mask,
		                             2 + 2 * sizeof(PacketMask),
		                             header.seq);
	}

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const PacketHeader& header) -> Stream& {
		stream << header.checksum << header.flags << header.ack;
		if (header.flags & EARLY_ACKS) {
			stream << header.mask;
		}
		if (header.flags & RELIABLE) {
			stream << header.seq;
		}
		return stream;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, PacketHeader& header) -> Stream& {
		stream >> header.checksum >> header.flags >> header.ack;
		if (header.flags & EARLY_ACKS) {
			stream >> header.mask;
		}
		if (header.flags & RELIABLE) {
			stream >> header.seq;
		}
		return stream;
	}

	[[nodiscard]] static auto calculateChecksum(util::Span<const std::byte> payload) -> util::CRC32 {
		return PROTOCOL_ID + payload;
	}
};

static_assert(MAX_PACKET_SIZE > PacketHeader::MAX_SIZE, "Packets must always be able to fit a payload.");

inline constexpr auto MAX_PACKET_PAYLOAD_SIZE = MAX_PACKET_SIZE - PacketHeader::MAX_SIZE;
inline constexpr auto MAX_MESSAGE_SIZE = std::size_t{MAX_PACKET_PAYLOAD_SIZE * (std::numeric_limits<SequenceNumber>::max() / 2)};

/**
 * Reserved NetChannel message types.
 */
namespace msg {

/**
 * First message sent to initiate a connection handshake.
 * Contains our public key and a randomly generated access token.
 * We expect to receive an encrypted message containing the same
 * access token at the end of the handshake sequence.
 */
template <net::MessageDirection DIR>
struct HandshakePart1 final : net::ReliableMessage<HandshakePart1<DIR>, DIR> {
	net::Big<crypto::kx::PublicKey, DIR> publicKey{};
	net::Big<crypto::AccessToken, DIR> token{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(publicKey, token);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(publicKey, token);
	}
};

/**
 * Second message sent in a connection handshake.
 * Contains the header generated when we initialized our send stream,
 * which the endpoint needs in order to initialize their receive stream.
 */
template <net::MessageDirection DIR>
struct HandshakePart2 final : net::ReliableMessage<HandshakePart2<DIR>, DIR> {
	net::Big<crypto::Stream::Header, DIR> header{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(header);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(header);
	}
};

/**
 * Third message sent in a connection handshake.
 * This is the first time an encrypted message is exchanged.
 * It contains the access token we received from the very first message
 * in order for the endpoint to verify that our encryption works.
 */
template <net::MessageDirection DIR>
struct HandshakePart3 final : net::SecretMessage<HandshakePart3<DIR>, DIR> {
	net::Big<crypto::AccessToken, DIR> token{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(token);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(token);
	}
};

/**
 * Final message sent in a connection handshake.
 * Sent in order to confirm to the endpoint that they are now
 * successfully connected.
 */
template <net::MessageDirection DIR>
struct Connect final : net::ReliableMessage<Connect<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

/**
 * Disconnect message.
 * Sent to inform that we are about to close our connection,
 * including a human-readable message explaining why.
 */
template <net::MessageDirection DIR>
struct Disconnect final : net::ReliableMessage<Disconnect<DIR>, DIR> {
	net::String<DIR> message{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(message);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(message);
	}
};

/**
 * Close message.
 * Sent to confirm that we have received the endpoint's disconnect message,
 * or to ask the endpoint to close immediately.
 */
template <net::MessageDirection DIR>
struct Close final : net::ReliableMessage<Close<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

/**
 * Ping message.
 * Sent periodically to measure the latency between us and the endpoint.
 */
template <net::MessageDirection DIR>
struct Ping final : net::ReliableMessage<Ping<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

/**
 * Pong message.
 * Sent as a response as soon as possible when a ping message is received.
 */
template <net::MessageDirection DIR>
struct Pong final : net::ReliableMessage<Pong<DIR>, DIR> {
	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie();
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie();
	}
};

/**
 * Encrypted message.
 * Contains an encrypted cipher text that must be decrypted before reading
 * the message type and message contents inside.
 */
template <net::MessageDirection DIR>
struct EncryptedMessage final : net::ReliableMessage<EncryptedMessage<DIR>, DIR> {
	net::List<std::byte, DIR> cipherText{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(cipherText);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(cipherText);
	}
};

namespace in {

using HandshakePart1 = net::msg::HandshakePart1<net::MessageDirection::INPUT>;
using HandshakePart2 = net::msg::HandshakePart2<net::MessageDirection::INPUT>;
using HandshakePart3 = net::msg::HandshakePart3<net::MessageDirection::INPUT>;
using Connect = net::msg::Connect<net::MessageDirection::INPUT>;
using Disconnect = net::msg::Disconnect<net::MessageDirection::INPUT>;
using Close = net::msg::Close<net::MessageDirection::INPUT>;
using Ping = net::msg::Ping<net::MessageDirection::INPUT>;
using Pong = net::msg::Pong<net::MessageDirection::INPUT>;
using EncryptedMessage = net::msg::EncryptedMessage<net::MessageDirection::INPUT>;

} // namespace in

namespace out {

using HandshakePart1 = net::msg::HandshakePart1<net::MessageDirection::OUTPUT>;
using HandshakePart2 = net::msg::HandshakePart2<net::MessageDirection::OUTPUT>;
using HandshakePart3 = net::msg::HandshakePart3<net::MessageDirection::OUTPUT>;
using Connect = net::msg::Connect<net::MessageDirection::OUTPUT>;
using Disconnect = net::msg::Disconnect<net::MessageDirection::OUTPUT>;
using Close = net::msg::Close<net::MessageDirection::OUTPUT>;
using Ping = net::msg::Ping<net::MessageDirection::OUTPUT>;
using Pong = net::msg::Pong<net::MessageDirection::OUTPUT>;
using EncryptedMessage = net::msg::EncryptedMessage<net::MessageDirection::OUTPUT>;

} // namespace out

} // namespace msg

template <net::MessageDirection DIR>
using NetChannelMessages = util::TypeList< //
	msg::HandshakePart1<DIR>,              //
	msg::HandshakePart2<DIR>,              //
	msg::HandshakePart3<DIR>,              //
	msg::Connect<DIR>,                     //
	msg::Disconnect<DIR>,                  //
	msg::Close<DIR>,                       //
	msg::Ping<DIR>,                        //
	msg::Pong<DIR>,                        //
	msg::EncryptedMessage<DIR>>;

using NetChannelInputMessages = NetChannelMessages<net::MessageDirection::INPUT>;
using NetChannelOutputMessages = NetChannelMessages<net::MessageDirection::OUTPUT>;

[[nodiscard]] constexpr auto isNetChannelMessage(MessageType type) noexcept -> bool {
	return type < util::typelist_size_v<NetChannelInputMessages>;
}

template <typename Message, typename MessageList>
struct message_type_of : std::integral_constant<MessageType, util::typelist_index_v<Message, MessageList>> {};

template <typename Message, typename MessageList>
inline constexpr auto message_type_of_v = message_type_of<Message, MessageList>::value;

struct ConnectionStats final {
	std::uint32_t packetsSent = 0;
	std::uint32_t packetsReceived = 0;
	std::uint32_t bytesSent = 0;
	std::uint32_t bytesReceived = 0;
	std::uint32_t reliablePacketsWritten = 0;
	std::uint32_t reliablePacketsReceived = 0;
	std::uint32_t reliablePacketsReceivedOutOfOrder = 0;
	std::uint32_t sendRateThrottleCount = 0;
	std::uint32_t packetSendErrorCount = 0;
	std::uint32_t encryptionErrorCount = 0;
	std::uint32_t invalidMessageTypeCount = 0;
	std::uint32_t invalidMessagePayloadCount = 0;
	std::uint32_t invalidMessageOrderCount = 0;
	std::uint32_t invalidPacketHeaderCount = 0;
	std::uint32_t invalidOutgoingMessageSizeCount = 0;
	std::uint32_t invalidOutgoingSecretMessageSizeCount = 0;
	std::uint32_t invalidEncryptedMessageCount = 0;
	std::uint32_t invalidPacketChecksumCount = 0;
	std::uint32_t sendBufferOverflowCount = 0;
	std::uint32_t receiveBufferOverflowCount = 0;
	std::uint32_t allocationErrorCount = 0;
};

class NetChannel {
public:
	static constexpr auto HANDSHAKE_TIMED_OUT_MESSAGE = std::string_view{"Connection handshake timed out."};

protected:
	using MessageHandler = void (*)(NetChannel&, ByteInputStream&);
	using ConnectedCallback = void (*)(NetChannel&, msg::in::Connect&&);

	NetChannel(util::Span<const MessageHandler> messageHandlers, ConnectedCallback onConnected, UDPSocket& socket, Duration timeout,
	           int throttleMaxSendBufferSize, int throttleMaxPeriod);

public:
	~NetChannel() = default;

	NetChannel(const NetChannel&) = delete;
	NetChannel(NetChannel&&) noexcept = default;

	auto operator=(const NetChannel&) -> NetChannel& = delete;
	auto operator=(NetChannel&&) noexcept -> NetChannel& = default;

	[[nodiscard]] auto getRemoteAddress() const noexcept -> IpAddress;
	[[nodiscard]] auto getRemotePort() const noexcept -> PortNumber;
	[[nodiscard]] auto getRemoteEndpoint() const noexcept -> IpEndpoint;
	[[nodiscard]] auto getLatestMeasuredPingDuration() const noexcept -> Duration;
	[[nodiscard]] auto getTimeout() const noexcept -> Duration;
	[[nodiscard]] auto getThrottleMaxSendBufferSize() const noexcept -> int;
	[[nodiscard]] auto getThrottleMaxPeriod() const noexcept -> int;
	[[nodiscard]] auto connecting() const noexcept -> bool;
	[[nodiscard]] auto connected() const noexcept -> bool;
	[[nodiscard]] auto disconnecting() const noexcept -> bool;
	[[nodiscard]] auto disconnected() const noexcept -> bool;
	[[nodiscard]] auto getDisconnectMessage() const noexcept -> std::string_view;
	[[nodiscard]] auto getStats() const noexcept -> const ConnectionStats&;

	auto setTimeout(Duration timeout) noexcept -> void;
	auto setThrottleMaxSendBufferSize(int throttleMaxSendBufferSize) noexcept -> void;
	auto setThrottleMaxPeriod(int throttleMaxPeriod) noexcept -> void;

	[[nodiscard]] auto connect(IpEndpoint endpoint) -> bool;
	[[nodiscard]] auto accept(IpEndpoint endpoint) -> bool;

	auto reset() noexcept -> void;
	auto resetStats() noexcept -> void;

	auto close() -> bool;
	auto close(std::string_view message) -> bool;
	auto disconnect(std::string_view message, Duration delay = DISCONNECT_DURATION) -> bool;

	[[nodiscard]] auto update() -> bool;

	auto sendPackets() -> void;
	auto receivePacket(std::vector<std::byte> data) -> bool;

protected:
	template <typename MessageList, typename Message, typename = std::enable_if_t<net::is_message_v<Message>>>
	[[nodiscard]] auto bufferMessage(const Message& msg) noexcept -> bool {
		constexpr auto type = message_type_of_v<Message, MessageList>;

		auto countStream = ByteCountStream{};
		countStream << type << msg;

		auto data = std::vector<std::byte>{};
		data.reserve(countStream.capacity());
		auto dataStream = ByteOutputStream{data};
		dataStream << type << msg;
		if (data.size() > MAX_MESSAGE_SIZE) {
			DEBUG_MSG(Msg::CONNECTION_EVENT, "Failed to buffer {} (greater than max message size ({}/{}))!", DEBUG_TYPE_NAME_ONLY(Message), data.size(), MAX_MESSAGE_SIZE);
			++m_stats.invalidOutgoingMessageSizeCount;
			return false;
		}

		try {
			if constexpr (net::is_secret_message_v<Message>) {
				if (data.size() > crypto::Stream::MAX_MESSAGE_SIZE) {
					DEBUG_MSG(Msg::CONNECTION_EVENT,
					          "Failed to buffer {} (greater than max secret message size ({}/{}))!",
					          DEBUG_TYPE_NAME_ONLY(Message),
					          data.size(),
					          crypto::Stream::MAX_MESSAGE_SIZE);
					++m_stats.invalidOutgoingSecretMessageSizeCount;
					return false;
				}
			}
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Buffering {} ({} bytes).", DEBUG_TYPE_NAME_ONLY(Message), data.size());
			m_bufferedMessages.emplace_back(std::move(data), message_category_of_v<Message>);
		} catch ([[maybe_unused]] const std::bad_alloc& e) {
			DEBUG_MSG(Msg::CONNECTION_EVENT, "Failed to buffer {} ({} bytes) ({})!", DEBUG_TYPE_NAME_ONLY(Message), data.size(), e.what());
			++m_stats.allocationErrorCount;
			return false;
		}
		return true;
	}

	auto handleMessage(msg::in::HandshakePart1&& msg) -> void;
	auto handleMessage(msg::in::HandshakePart2&& msg) -> void;
	auto handleMessage(msg::in::HandshakePart3&& msg) -> void;
	auto handleMessage(msg::in::Connect&& msg) -> void;
	auto handleMessage(msg::in::Disconnect&& msg) -> void;
	auto handleMessage(msg::in::Close&& msg) -> void;
	auto handleMessage(msg::in::Ping&& msg) -> void;
	auto handleMessage(msg::in::Pong&& msg) -> void;
	auto handleMessage(msg::in::EncryptedMessage&& msg) -> void;

private:
	enum class State : std::uint8_t {
		DISCONNECTED,
		HANDSHAKE_PART1,
		HANDSHAKE_PART2,
		HANDSHAKE_PART3,
		CONNECTING,
		CONNECTED,
		DISCONNECTING_MYSELF,
		DISCONNECTING,
	};

	struct BufferedMessage final {
		BufferedMessage() = default;

		template <typename Data, typename Category>
		BufferedMessage(Data&& data, Category&& category)
			: data(std::forward<Data>(data))
			, category(std::forward<Category>(category)) {}

		std::vector<std::byte> data;
		MessageCategory category{};
	};

	enum class SendStatus : std::uint8_t {
		SUCCESS,              // Packets were sent successfully.
		PACKET_SEND_FAILED,   // Failed to send packets. Connection should close.
		SEND_BUFFER_OVERFLOW, // A split reliable message was too big to fit in the send buffer. Connection should close.
		ENCRYPTION_FAILED,    // Failed to encrypt a secret message. Connection should close.
	};

	struct OutgoingPacket final {
		OutgoingPacket() = default;

		template <typename Header, typename Payload>
		OutgoingPacket(Header&& header, Payload&& payload)
			: header(std::forward<Header>(header))
			, payload(std::forward<Payload>(payload)) {}

		PacketHeader header;
		std::vector<std::byte> payload;
		bool acked = false;
	};

	struct IncomingPacket final {
		IncomingPacket() = default;

		template <typename Header, typename Payload>
		IncomingPacket(Header&& header, Payload&& payload)
			: header(std::forward<Header>(header))
			, payload(std::forward<Payload>(payload)) {}

		PacketHeader header;
		std::vector<std::byte> payload;
	};

	template <typename Message, typename = std::enable_if_t<is_output_message_v<Message>>>
	[[nodiscard]] auto write(const Message& msg) noexcept -> bool {
		return this->bufferMessage<NetChannelOutputMessages>(msg);
	}

	[[nodiscard]] auto encryptMessage(std::vector<std::byte>& output, util::Span<const std::byte> secretMessage) -> bool;

	[[nodiscard]] auto initializeConnection(bool serverSide, IpEndpoint endpoint) -> bool;

	[[nodiscard]] auto checkConnection() -> bool;

	auto processReceivedPackets() -> void;
	auto processSavedPackets() -> void;
	auto handleMessages(util::Span<const std::byte> payload) -> void;
	auto acknowledge(Acknowledgement ack) -> void;

	[[nodiscard]] auto savePacket(const PacketHeader& header, std::vector<std::byte> payload) -> bool;

	[[nodiscard]] auto send() -> SendStatus;

	[[nodiscard]] auto throttle() -> bool;

	[[nodiscard]] auto writeMessages(PacketHeader::Flags& flags, PacketMask mask, std::vector<std::byte>& payload,
	                                 std::vector<BufferedMessage>& messages) -> SendStatus;

	[[nodiscard]] auto getEarlyPacketMask() const -> PacketMask;

	[[nodiscard]] auto sendPacket(PacketHeader::Flags flags, PacketMask mask, std::vector<std::byte>&& payload) -> SendStatus;
	[[nodiscard]] auto splitAndSendMessage(std::vector<std::byte>&& payload, PacketHeader::Flags flags, PacketMask mask,
	                                       util::Span<const std::byte> message) -> SendStatus;
	[[nodiscard]] auto sendAndBufferPacket(const PacketHeader& header, std::vector<std::byte>&& payload) -> SendStatus;
	[[nodiscard]] auto sendPacket(const PacketHeader& header, util::Span<const std::byte> payload) -> SendStatus;

	crypto::kx::PublicKey m_publicKey{};
	crypto::kx::SecretKey m_secretKey{};
	crypto::kx::ReceiveKey m_receiveKey{};
	crypto::kx::SendKey m_sendKey{};
	crypto::Stream::Send m_sendStream{};
	crypto::Stream::Receive m_receiveStream{};
	crypto::AccessToken m_localHandshakeToken{};
	crypto::AccessToken m_remoteHandshakeToken{};
	std::deque<OutgoingPacket> m_sendBuffer{};
	util::RingMap<SequenceNumber, IncomingPacket> m_receiveBuffer{};
	std::vector<std::vector<std::byte>> m_receivedPackets{};
	std::vector<BufferedMessage> m_bufferedMessages{};
	std::vector<TimePoint> m_pingTimeBuffer{};
	std::string m_disconnectMessage{};
	util::Span<const MessageHandler> m_messageHandlers;
	ConnectedCallback m_onConnected;
	util::Reference<UDPSocket> m_socket;
	IpEndpoint m_endpoint{};
	Duration m_timeout;
	TimePoint m_latestPacketReceiveTime{};
	TimePoint m_disconnectTime{};
	TimePoint m_nextPingMeasureTime{};
	Duration m_latestMeasuredPingDuration = Duration::zero();
	SequenceNumber m_latestSeqSent = 0;
	SequenceNumber m_latestSeqHandled = 0;
	Acknowledgement m_latestAckReceived{};
	State m_state = State::DISCONNECTED;
	int m_throttleMaxSendBufferSize;
	int m_throttleMaxPeriod;
	int m_throttleCounter = 0;
	int m_throttlePeriod = 0;
	bool m_serverSide = false;

protected:
	ConnectionStats m_stats{};
};

template <typename IncomingMessages, typename Handler>
class Connection final : public NetChannel {
private:
	using AllIncomingMessages = util::typelist_concat_t<NetChannelInputMessages, IncomingMessages>;

	static_assert(util::typelist_size_v<AllIncomingMessages> <= std::numeric_limits<MessageType>::max(),
	              "Too many incoming message types.");

	static_assert(is_all_input_messages_v<AllIncomingMessages>, "Incoming message list contains a type that is not an input message!");

	template <typename Message, typename = std::enable_if_t<net::is_input_message_v<Message>>>
	static auto readAndHandleMessage(NetChannel& channel, ByteInputStream& packetStream) -> void {
		auto& connection = static_cast<Connection&>(channel);

		auto msg = Message{};
		if (!(packetStream >> msg)) {
			DEBUG_MSG(Msg::CONNECTION_EVENT | Msg::CONNECTION_CRYPTO, "Read {} with invalid payload.", DEBUG_TYPE_NAME_ONLY(Message));
			++connection.m_stats.invalidMessagePayloadCount;
			return;
		}

		DEBUG_MSG_INDENT(Msg::CONNECTION_DETAILED, "Read {} successfully.", DEBUG_TYPE_NAME_ONLY(Message)) {
			if constexpr (util::typelist_contains_v<Message, NetChannelInputMessages>) {
				connection.handleMessage(std::move(msg));
			} else {
				if (connection.connected()) {
					connection.m_handler(std::move(msg));
				}
			}
		}
	}

	template <typename TypeList>
	struct message_handler_table;

	template <typename... Ts>
	struct message_handler_table<util::TypeList<Ts...>> {
		static constexpr auto value = std::array{Connection::template readAndHandleMessage<Ts>...};
	};

	static void onConnected(NetChannel& channel, msg::in::Connect&& msg) {
		auto& connection = static_cast<Connection&>(channel);
		connection.m_handler(std::move(msg)); // NOLINT(performance-move-const-arg)
	}

public:
	Connection(UDPSocket& socket, Duration timeout, int throttleMaxSendBufferSize, int throttleMaxPeriod, Handler handler)
		: NetChannel(message_handler_table<AllIncomingMessages>::value, onConnected, socket, timeout, throttleMaxSendBufferSize, throttleMaxPeriod)
		, m_handler(std::move(handler)) {}

	~Connection() = default;

	Connection(const Connection&) = delete;
	Connection(Connection&&) noexcept = default;

	auto operator=(const Connection&) -> Connection& = delete;
	auto operator=(Connection&&) noexcept -> Connection& = default;

	template <typename MessageList, typename Message, typename = std::enable_if_t<is_output_message_v<Message>>>
	[[nodiscard]] auto write(const Message& msg) noexcept -> bool {
		using AllOutgoingMessages = util::typelist_concat_t<NetChannelOutputMessages, MessageList>;

		static_assert(util::typelist_size_v<AllOutgoingMessages> <= std::numeric_limits<MessageType>::max(),
		              "Too many outgoing message types.");

		static_assert(is_all_output_messages_v<AllOutgoingMessages>, "Outgoing message list contains a type that is not an output message!");

		return this->bufferMessage<AllOutgoingMessages>(msg);
	}

private:
	Handler m_handler;
};

} // namespace net

#endif
