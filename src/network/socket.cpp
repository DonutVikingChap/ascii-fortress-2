#include "socket.hpp"

#include <chrono>  // std::chrono::...
#include <utility> // std::exchange

namespace net {
namespace {

constexpr auto UDP_RECEIVE_FLAGS = 0;
constexpr auto UDP_SEND_FLAGS = 0;
#ifdef _WIN32
constexpr auto TCP_RECEIVE_FLAGS = 0;
constexpr auto TCP_SEND_FLAGS = 0;
#else
constexpr auto TCP_RECEIVE_FLAGS = int{MSG_NOSIGNAL};
constexpr auto TCP_SEND_FLAGS = int{MSG_NOSIGNAL};
#endif

[[nodiscard]] auto getErrorStatus() noexcept -> std::error_code {
#ifdef _WIN32
	return make_error_code(WSAError{static_cast<int>(WSAGetLastError())});
#else
	return std::error_code{errno, std::generic_category()};
#endif
}

} // namespace

auto SocketErrorCategory::name() const noexcept -> const char* {
	return "socket";
}

auto SocketErrorCategory::message(int condition) const -> std::string {
	switch (static_cast<SocketError>(condition)) {
		case SocketError::WAIT: return "Wait";
		case SocketError::PARTIAL: return "Partial";
		case SocketError::DISCONNECTED: return "Disconnected";
		case SocketError::FAILED: return "Failed";
	}
	return "Unknown";
}

auto SocketErrorCategory::equivalent(const std::error_code& code, int condition) const noexcept -> bool {
	if (code.category() == *this && code.value() == condition) {
		return true;
	}

#ifdef _WIN32
	if (code.category() == WSAErrorCategory::instance()) {
		switch (code.value()) {
			case WSAEWOULDBLOCK: return condition == static_cast<int>(SocketError::WAIT);
			case WSAEALREADY: return condition == static_cast<int>(SocketError::WAIT);
			case WSAECONNABORTED: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case WSAECONNRESET: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case WSAETIMEDOUT: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case WSAENETRESET: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case WSAENOTCONN: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case WSAEISCONN: return condition == 0;
			default: break;
		}
		return condition == static_cast<int>(SocketError::FAILED);
	}
#endif

	if (code.category() == std::generic_category()) {
		if ((code.value() == EAGAIN) || (code.value() == EINPROGRESS)) {
			return condition == static_cast<int>(SocketError::WAIT);
		}
		switch (code.value()) {
			case EWOULDBLOCK: return condition == static_cast<int>(SocketError::WAIT);
			case ECONNABORTED: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case ECONNRESET: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case ETIMEDOUT: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case ENETRESET: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case ENOTCONN: return condition == static_cast<int>(SocketError::DISCONNECTED);
			case EPIPE: return condition == static_cast<int>(SocketError::DISCONNECTED);
			default: break;
		}
		return condition == static_cast<int>(SocketError::FAILED);
	}
	return false;
}

Socket::Socket(SOCKET handle) noexcept
	: m_socket(handle) {}

Socket::operator bool() const noexcept {
	return static_cast<bool>(m_socket);
}

auto Socket::close() noexcept -> void {
	m_socket.reset();
}

auto Socket::release() noexcept -> SOCKET {
	return m_socket.release();
}

auto Socket::create(ProtocolType protocol, std::error_code& ec) -> void {
	const auto type = (protocol == ProtocolType::UDP) ? SOCK_DGRAM : SOCK_STREAM;
	auto handle = socket(PF_INET, type, 0);
	if (handle == INVALID_SOCKET) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
	const auto optionValue = 1;
	setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optionValue), sizeof(optionValue));
	m_socket.reset(handle);
	ec.clear();
}

auto Socket::enableBlocking(std::error_code& ec) -> void {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
#ifdef _WIN32
	auto nonBlock = u_long{0};
	ioctlsocket(m_socket.get(), FIONBIO, &nonBlock);
#else
	if (fcntl(m_socket.get(), F_SETFL, fcntl(m_socket.get(), F_GETFL) & ~O_NONBLOCK) == -1) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
#endif
	ec.clear();
}

auto Socket::disableBlocking(std::error_code& ec) -> void {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
#ifdef _WIN32
	auto nonBlock = u_long{1};
	ioctlsocket(m_socket.get(), FIONBIO, &nonBlock);
#else
	if (fcntl(m_socket.get(), F_SETFL, fcntl(m_socket.get(), F_GETFL) | O_NONBLOCK) == -1) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
#endif
	ec.clear();
}

auto Socket::setBlocking(BlockingMode mode, std::error_code& ec) -> void {
	switch (mode) {
		case BlockingMode::UNSPECIFIED:
			if (!m_socket) {
				ec = make_error_code(SocketError::FAILED);
				return;
			}
			ec.clear();
			break;
		case BlockingMode::BLOCK: this->enableBlocking(ec); break;
		case BlockingMode::NONBLOCK: this->disableBlocking(ec); break;
	}
}

auto Socket::connect(IpEndpoint endpoint, std::error_code& ec) -> void {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
	if (::connect(m_socket.get(), reinterpret_cast<sockaddr*>(&endpoint.get()), static_cast<socklen_t>(sizeof(endpoint.get()))) < 0) {
		ec = getErrorStatus();
		return;
	}
	ec.clear();
}

auto Socket::bind(IpEndpoint endpoint, std::error_code& ec) -> void {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
	if (::bind(m_socket.get(), reinterpret_cast<sockaddr*>(&endpoint.get()), static_cast<socklen_t>(sizeof(endpoint.get()))) == SOCKET_ERROR) {
		ec = getErrorStatus();
		return;
	}
	ec.clear();
}

auto Socket::getLocalEndpoint(std::error_code& ec) const -> IpEndpoint {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return IpEndpoint{};
	}
	auto endpoint = IpEndpoint{};
	auto size = static_cast<socklen_t>(sizeof(endpoint.get()));
	if (getsockname(m_socket.get(), reinterpret_cast<sockaddr*>(&endpoint.get()), &size) == SOCKET_ERROR) {
		ec = getErrorStatus();
		return IpEndpoint{};
	}
	ec.clear();
	return endpoint;
}

auto Socket::getRemoteEndpoint(std::error_code& ec) const -> IpEndpoint {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return IpEndpoint{};
	}
	auto endpoint = IpEndpoint{};
	auto size = static_cast<socklen_t>(sizeof(endpoint.get()));
	if (getpeername(m_socket.get(), reinterpret_cast<sockaddr*>(&endpoint.get()), &size) == SOCKET_ERROR) {
		ec = getErrorStatus();
		return IpEndpoint{};
	}
	ec.clear();
	return endpoint;
}

auto Socket::listen(std::error_code& ec) -> void {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return;
	}
	if (::listen(m_socket.get(), SOMAXCONN) == SOCKET_ERROR) {
		ec = getErrorStatus();
		return;
	}
	ec.clear();
}

auto Socket::accept(std::error_code& ec) -> Socket {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return Socket{};
	}
	auto endpoint = IpEndpoint{};
	auto size = static_cast<socklen_t>(sizeof(endpoint.get()));
	const auto remote = ::accept(m_socket.get(), reinterpret_cast<sockaddr*>(&endpoint.get()), &size);
	if (remote == INVALID_SOCKET) {
		ec = getErrorStatus();
		return Socket{};
	}
	ec.clear();
	return Socket{remote};
}

auto Socket::receive(util::Span<std::byte> buffer, int flags, std::error_code& ec) -> util::Span<std::byte> {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return util::Span<std::byte>{};
	}
	const auto bytesReceived = recv(m_socket.get(), reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), flags);
	if (bytesReceived < 0) {
		ec = getErrorStatus();
		return util::Span<std::byte>{};
	}
	ec.clear();
	return buffer.first(static_cast<std::size_t>(bytesReceived));
}

auto Socket::receiveFrom(IpEndpoint& endpoint, util::Span<std::byte> buffer, int flags, std::error_code& ec) -> util::Span<std::byte> {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return util::Span<std::byte>{};
	}
	auto size = static_cast<socklen_t>(sizeof(endpoint.get()));
	const auto bytesReceived = recvfrom(m_socket.get(),
	                                    reinterpret_cast<char*>(buffer.data()),
	                                    static_cast<int>(buffer.size()),
	                                    flags,
	                                    reinterpret_cast<sockaddr*>(&endpoint.get()),
	                                    &size);
	if (bytesReceived < 0) {
		ec = getErrorStatus();
		return util::Span<std::byte>{};
	}
	ec.clear();
	return buffer.first(static_cast<std::size_t>(bytesReceived));
}

auto Socket::send(util::Span<const std::byte> bytes, int flags, std::error_code& ec) -> std::size_t {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return 0;
	}
	const auto bytesSent = ::send(m_socket.get(), reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size()), flags);
	if (bytesSent < 0) {
		ec = getErrorStatus();
		return 0;
	}
	ec.clear();
	return static_cast<std::size_t>(bytesSent);
}

auto Socket::sendTo(IpEndpoint endpoint, util::Span<const std::byte> bytes, int flags, std::error_code& ec) -> std::size_t {
	if (!m_socket) {
		ec = make_error_code(SocketError::FAILED);
		return 0;
	}
	if (sendto(m_socket.get(),
	           reinterpret_cast<const char*>(bytes.data()),
	           static_cast<int>(bytes.size()),
	           flags,
	           reinterpret_cast<sockaddr*>(&endpoint.get()),
	           static_cast<socklen_t>(sizeof(endpoint.get()))) < 0) {
		ec = getErrorStatus();
		return 0;
	}
	ec.clear();
	return bytes.size();
}

auto Socket::get() const noexcept -> SOCKET {
	return m_socket.get();
}

auto Socket::SocketDeleter::operator()(SOCKET handle) const noexcept -> void {
	if (handle != INVALID_SOCKET) {
		closesocket(handle);
	}
}

UDPSocket::UDPSocket(SOCKET handle) noexcept
	: Socket(handle) {}

UDPSocket::operator bool() const noexcept {
	return static_cast<bool>(static_cast<const Socket&>(*this));
}

auto UDPSocket::close() noexcept -> void {
	Socket::close();
}

auto UDPSocket::release() noexcept -> SOCKET {
	return Socket::release();
}

auto UDPSocket::bind(IpEndpoint endpoint, std::error_code& ec) -> void {
	Socket::close();
	Socket::create(ProtocolType::UDP, ec);
	if (ec) {
		return;
	}
	Socket::disableBlocking(ec);
	if (ec) {
		return;
	}
	Socket::bind(endpoint, ec);
}

auto UDPSocket::getLocalEndpoint(std::error_code& ec) const -> IpEndpoint {
	return Socket::getLocalEndpoint(ec);
}

auto UDPSocket::receiveFrom(IpEndpoint& endpoint, util::Span<std::byte> buffer, std::error_code& ec) -> util::Span<std::byte> {
	return Socket::receiveFrom(endpoint, buffer, UDP_RECEIVE_FLAGS, ec);
}

auto UDPSocket::sendTo(IpEndpoint endpoint, util::Span<const std::byte> bytes, std::error_code& ec) -> std::size_t {
	return Socket::sendTo(endpoint, bytes, UDP_SEND_FLAGS, ec);
}

auto UDPSocket::get() const noexcept -> SOCKET {
	return Socket::get();
}

TCPSocket::TCPSocket(SOCKET handle) noexcept
	: Socket(handle) {}

TCPSocket::operator bool() const noexcept {
	return static_cast<bool>(static_cast<const Socket&>(*this));
}

auto TCPSocket::close() noexcept -> void {
	Socket::close();
}

auto TCPSocket::release() noexcept -> SOCKET {
	return Socket::release();
}

auto TCPSocket::connect(BlockingMode mode, IpEndpoint endpoint, Duration timeout, std::error_code& ec) -> void {
	Socket::close();
	Socket::create(ProtocolType::TCP, ec);
	if (ec) {
		return;
	}
	if (timeout <= Duration::zero()) {
		Socket::setBlocking(mode, ec);
		if (ec) {
			return;
		}
		Socket::connect(endpoint, ec);
		return;
	}
	Socket::disableBlocking(ec);
	if (ec) {
		return;
	}
	Socket::connect(endpoint, ec);
	if (ec) {
		if (mode != BlockingMode::BLOCK || ec != SocketError::WAIT) {
			return;
		}
		auto fdset = fd_set{};
		FD_ZERO(&fdset);
		FD_SET(Socket::get(), &fdset);
		const auto timeoutSeconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
		const auto timeoutMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout - timeoutSeconds);
		auto tv = timeval{};
		tv.tv_sec = static_cast<decltype(tv.tv_sec)>(timeoutSeconds.count());
		tv.tv_usec = static_cast<decltype(tv.tv_usec)>(timeoutMicroseconds.count());
		if (select(static_cast<int>(Socket::get() + 1), nullptr, &fdset, nullptr, &tv) < 1) {
			ec = getErrorStatus();
			return;
		}
		const auto remoteAddress = Socket::getRemoteEndpoint(ec).getAddress();
		if (ec) {
			return;
		}
		if (remoteAddress == IpAddress::none()) {
			ec = getErrorStatus();
			return;
		}
		ec.clear();
	}
	if (mode == BlockingMode::BLOCK) {
		Socket::enableBlocking(ec);
	}
}

auto TCPSocket::getLocalEndpoint(std::error_code& ec) const -> IpEndpoint {
	return Socket::getLocalEndpoint(ec);
}

auto TCPSocket::getRemoteEndpoint(std::error_code& ec) const -> IpEndpoint {
	return Socket::getRemoteEndpoint(ec);
}

auto TCPSocket::receive(util::Span<std::byte> buffer, std::error_code& ec) -> util::Span<std::byte> {
	return Socket::receive(buffer, TCP_RECEIVE_FLAGS, ec);
}

auto TCPSocket::receiveFrom(IpEndpoint& endpoint, util::Span<std::byte> buffer, std::error_code& ec) -> util::Span<std::byte> {
	return Socket::receiveFrom(endpoint, buffer, TCP_RECEIVE_FLAGS, ec);
}

auto TCPSocket::send(util::Span<const std::byte> bytes, std::error_code& ec) -> std::size_t {
	auto bytesSent = std::size_t{0};
	while (bytesSent < bytes.size()) {
		const auto chunkBytesSent = Socket::send(bytes.subspan(bytesSent), TCP_SEND_FLAGS, ec);
		if (ec) {
			if (ec == SocketError::WAIT && bytesSent > 0) {
				ec = make_error_code(SocketError::PARTIAL);
			}
			return bytesSent;
		}
		bytesSent += chunkBytesSent;
	}
	ec.clear();
	return bytesSent;
}

auto TCPSocket::sendTo(IpEndpoint endpoint, util::Span<const std::byte> bytes, std::error_code& ec) -> std::size_t {
	auto bytesSent = std::size_t{0};
	while (bytesSent < bytes.size()) {
		const auto chunkBytesSent = Socket::sendTo(endpoint, bytes.subspan(bytesSent), TCP_SEND_FLAGS, ec);
		if (ec) {
			if (ec == SocketError::WAIT && bytesSent > 0) {
				ec = make_error_code(SocketError::PARTIAL);
			}
			return bytesSent;
		}
		bytesSent += chunkBytesSent;
	}
	ec.clear();
	return bytesSent;
}

auto TCPSocket::get() const noexcept -> SOCKET {
	return Socket::get();
}

TCPListener::TCPListener(SOCKET handle) noexcept
	: Socket(handle) {}

TCPListener::operator bool() const noexcept {
	return static_cast<bool>(static_cast<const Socket&>(*this));
}

auto TCPListener::close() noexcept -> void {
	Socket::close();
}

auto TCPListener::release() noexcept -> SOCKET {
	return Socket::release();
}

auto TCPListener::listen(BlockingMode mode, IpEndpoint endpoint, std::error_code& ec) -> void {
	Socket::close();
	Socket::create(ProtocolType::TCP, ec);
	if (ec) {
		return;
	}
	Socket::setBlocking(mode, ec);
	if (ec) {
		return;
	}
	Socket::bind(endpoint, ec);
	if (ec) {
		return;
	}
	Socket::listen(ec);
}

auto TCPListener::getLocalEndpoint(std::error_code& ec) const -> IpEndpoint {
	return Socket::getLocalEndpoint(ec);
}

auto TCPListener::accept(std::error_code& ec) -> TCPSocket {
	return TCPSocket{Socket::accept(ec).release()};
}

auto TCPListener::get() const noexcept -> SOCKET {
	return Socket::get();
}

} // namespace net
