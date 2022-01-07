#ifndef AF2_NETWORK_SOCKET_HPP
#define AF2_NETWORK_SOCKET_HPP

#include "../utilities/resource.hpp" // util::Resource
#include "../utilities/span.hpp"     // util::Span
#include "config.hpp"                // net::Duration
#include "endpoint.hpp"              // net::IpEndpoint, net::IpAddress
#include "platform.hpp"              // net::WSAErrorCategory, ...

#include <cstddef>      // std::byte, std::size_t
#include <cstdint>      // std::uint8_t
#include <system_error> // std::error_code, std::error_category, std::error_condition
#include <type_traits>  // std::true_type

namespace net {

enum class SocketError : int {
	// The value 0 is reserved for success.
	WAIT = 1,     // Try again later.
	PARTIAL,      // The data was partially transmitted.
	DISCONNECTED, // The socket disconnected.
	FAILED,       // The operation failed.
};

} // namespace net

template <>
struct std::is_error_condition_enum<net::SocketError> : std::true_type {};

namespace net {

struct SocketErrorCategory final : std::error_category {
	[[nodiscard]] auto name() const noexcept -> const char* override;
	[[nodiscard]] auto message(int condition) const -> std::string override;
	[[nodiscard]] auto equivalent(const std::error_code& code, int condition) const noexcept -> bool override;

	[[nodiscard]] static auto instance() noexcept -> const SocketErrorCategory& {
		static const auto category = SocketErrorCategory{};
		return category;
	}
};

[[nodiscard]] inline auto make_error_code(SocketError err) -> std::error_code {
	return std::error_code{static_cast<int>(err), SocketErrorCategory::instance()};
}

[[nodiscard]] inline auto make_error_condition(SocketError err) -> std::error_condition {
	return std::error_condition{static_cast<int>(err), SocketErrorCategory::instance()};
}

class Socket {
public:
	enum class ProtocolType : std::uint8_t {
		UDP, // User Datagram Protocol.
		TCP, // Transmission Control Protocol.
	};

	enum class BlockingMode : std::uint8_t {
		UNSPECIFIED, // Preserve current blocking mode.
		BLOCK,       // Enable blocking.
		NONBLOCK,    // Disable blocking.
	};

	explicit Socket(SOCKET handle = INVALID_SOCKET) noexcept;

	explicit operator bool() const noexcept;

	auto close() noexcept -> void;
	auto release() noexcept -> SOCKET;

	auto create(ProtocolType protocol, std::error_code& ec) -> void;

	auto enableBlocking(std::error_code& ec) -> void;
	auto disableBlocking(std::error_code& ec) -> void;
	auto setBlocking(BlockingMode mode, std::error_code& ec) -> void;

	auto connect(IpEndpoint endpoint, std::error_code& ec) -> void;
	auto bind(IpEndpoint endpoint, std::error_code& ec) -> void;

	[[nodiscard]] auto getLocalEndpoint(std::error_code& ec) const -> IpEndpoint;
	[[nodiscard]] auto getRemoteEndpoint(std::error_code& ec) const -> IpEndpoint;

	auto listen(std::error_code& ec) -> void;

	[[nodiscard]] auto accept(std::error_code& ec) -> Socket;

	[[nodiscard]] auto receive(util::Span<std::byte> buffer, int flags, std::error_code& ec) -> util::Span<std::byte>;
	[[nodiscard]] auto receiveFrom(IpEndpoint& endpoint, util::Span<std::byte> buffer, int flags, std::error_code& ec) -> util::Span<std::byte>;

	auto send(util::Span<const std::byte> bytes, int flags, std::error_code& ec) -> std::size_t;
	auto sendTo(IpEndpoint endpoint, util::Span<const std::byte> bytes, int flags, std::error_code& ec) -> std::size_t;

	[[nodiscard]] auto get() const noexcept -> SOCKET;

private:
	struct SocketDeleter final {
		auto operator()(SOCKET handle) const noexcept -> void;
	};
	using SocketObject = util::Resource<SOCKET, SocketDeleter, INVALID_SOCKET>;

	SocketObject m_socket;
};

class UDPSocket final : private Socket {
public:
	explicit UDPSocket(SOCKET handle = INVALID_SOCKET) noexcept;

	explicit operator bool() const noexcept;

	auto close() noexcept -> void;
	auto release() noexcept -> SOCKET;

	auto bind(IpEndpoint endpoint, std::error_code& ec) -> void;

	[[nodiscard]] auto getLocalEndpoint(std::error_code& ec) const -> IpEndpoint;

	[[nodiscard]] auto receiveFrom(IpEndpoint& endpoint, util::Span<std::byte> buffer, std::error_code& ec) -> util::Span<std::byte>;

	auto sendTo(IpEndpoint endpoint, util::Span<const std::byte> bytes, std::error_code& ec) -> std::size_t;

	[[nodiscard]] auto get() const noexcept -> SOCKET;
};

class TCPSocket final : private Socket {
public:
	explicit TCPSocket(SOCKET handle = INVALID_SOCKET) noexcept;

	explicit operator bool() const noexcept;

	auto close() noexcept -> void;
	auto release() noexcept -> SOCKET;

	auto connect(BlockingMode mode, IpEndpoint endpoint, Duration timeout, std::error_code& ec) -> void;

	[[nodiscard]] auto getLocalEndpoint(std::error_code& ec) const -> IpEndpoint;
	[[nodiscard]] auto getRemoteEndpoint(std::error_code& ec) const -> IpEndpoint;

	[[nodiscard]] auto receive(util::Span<std::byte> buffer, std::error_code& ec) -> util::Span<std::byte>;
	[[nodiscard]] auto receiveFrom(IpEndpoint& endpoint, util::Span<std::byte> buffer, std::error_code& ec) -> util::Span<std::byte>;

	auto send(util::Span<const std::byte> bytes, std::error_code& ec) -> std::size_t;
	auto sendTo(IpEndpoint endpoint, util::Span<const std::byte> bytes, std::error_code& ec) -> std::size_t;

	[[nodiscard]] auto get() const noexcept -> SOCKET;
};

class TCPListener final : private Socket {
public:
	explicit TCPListener(SOCKET handle = INVALID_SOCKET) noexcept;

	explicit operator bool() const noexcept;

	auto close() noexcept -> void;
	auto release() noexcept -> SOCKET;

	auto listen(BlockingMode mode, IpEndpoint endpoint, std::error_code& ec) -> void;

	[[nodiscard]] auto getLocalEndpoint(std::error_code& ec) const -> IpEndpoint;

	[[nodiscard]] auto accept(std::error_code& ec) -> TCPSocket;

	[[nodiscard]] auto get() const noexcept -> SOCKET;
};

} // namespace net

#endif
