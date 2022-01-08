#ifndef AF2_NETWORK_PLATFORM_HPP
#define AF2_NETWORK_PLATFORM_HPP

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h> // AF_INET, PF_INET, SOCK_DGRAM, SOCK_STREAM, SOCKET, INVALID_SOCKET, SOCKET_ERROR, INADDR_ANY, INADDR_LOOPBACK, INADDR_BROADCAST, INADDR_NONE, SOL_SOCKET, SO_REUSEADDR, SOMAXCONN, FD_SET, WSADATA, WSAE..., FIONBIO, addrinfo, sockaddr, sockaddr_in, accept, bind, closesocket, connect, getpeername, getsockname, inet_addr, ioctlsocket, listen, recv, recvfrom, select, send, sendto, setsockopt, socket, WSACleanup, WSAGetLastError, WSAStartup, fd_set, timeval, u_long
#include <ws2tcpip.h> // EAI_..., gai_strerror, freeaddrinfo, getaddrinfo, socklen_t
#ifndef __MINGW32__
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <system_error> // std::error_code, std::error_category

#else

#include <cerrno>       // errno
#include <fcntl.h>      // fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <netdb.h>      // EAI_..., gai_strerror, addrinfo, freeaddrinfo, getaddrinfo
#include <netinet/in.h> // INADDR_ANY, INADDR_NONE, INADDR_LOOPBACK, INADDR_BROADCAST, sockaddr_in
#include <sys/select.h> // select
#include <sys/socket.h> // AF_INET, PF_INET, SOCK_DGRAM, SOCK_STREAM, SO_REUSEADDR, SOMAXCONN, MSG_NOSIGNAL, socklen_t, sockaddr, accept, bind, connect, getpeername, getsockname, listen, recv, recvfrom, send, sendto, setsockopt, socket
#include <unistd.h>     // close

using SOCKET = int;

inline constexpr auto INVALID_SOCKET = -1;
inline constexpr auto SOCKET_ERROR = -1;

inline auto closesocket(SOCKET handle) -> int {
	return close(handle);
}

#endif

#include <SDL_endian.h> // SDL_BYTEORDER, SDL_LIL_ENDIAN, SDL_BIG_ENDIAN
#include <cstdint>      // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <limits>       // std::numeric_limits
#include <string>       // std::string
#include <type_traits>  // std::true_type

#undef htonl
#undef htons
#undef ntohs
#undef ntohl

#ifdef _WIN32

namespace net {

struct WSAError final {
	int value{};
};

} // namespace net

template <>
struct std::is_error_code_enum<net::WSAError> : std::true_type {};

namespace net {

struct WSAErrorCategory final : std::error_category {
	[[nodiscard]] auto name() const noexcept -> const char* override;
	[[nodiscard]] auto message(int condition) const -> std::string override;

	[[nodiscard]] static auto instance() noexcept -> const WSAErrorCategory& {
		static const auto category = WSAErrorCategory{};
		return category;
	}
};

[[nodiscard]] inline auto make_error_code(WSAError err) -> std::error_code {
	return std::error_code{static_cast<int>(err.value), WSAErrorCategory::instance()};
}

} // namespace net

#endif

namespace net {

[[nodiscard]] constexpr auto isLittleEndian() noexcept -> bool {
	return SDL_BYTEORDER == SDL_LIL_ENDIAN;
}

[[nodiscard]] constexpr auto isBigEndian() noexcept -> bool {
	return SDL_BYTEORDER == SDL_BIG_ENDIAN;
}

[[nodiscard]] constexpr auto pack754(long double val, unsigned bits, unsigned expBits) noexcept -> std::uint64_t {
	static_assert(std::numeric_limits<long double>::is_iec559, "This implementation assumes IEEE-754 floats.");

	if (val == 0.0l) {
		return 0;
	}

	auto norm = (val < 0) ? -val : val;
	auto shift = 0;
	while (norm >= 2.0l) {
		norm /= 2.0l;
		++shift;
	}
	while (norm < 1.0l) {
		norm *= 2.0l;
		--shift;
	}
	norm -= 1.0l;

	const auto significandBits = bits - expBits - 1;
	const auto significand = static_cast<std::uint64_t>(norm * ((std::uint64_t{1} << significandBits) + 0.5l));
	const auto exp = static_cast<std::uint64_t>(shift + ((1 << (expBits - 1)) - 1));
	const auto sign = (val < 0) ? std::uint64_t{1} : std::uint64_t{0};
	return (sign << (bits - 1)) | (exp << (bits - expBits - 1)) | significand;
}

[[nodiscard]] constexpr auto unpack754(std::uint64_t val, unsigned bits, unsigned expBits) noexcept -> long double {
	static_assert(std::numeric_limits<long double>::is_iec559, "This implementation assumes IEEE-754 floats.");

	if (val == 0) {
		return 0.0l;
	}

	const auto significandBits = bits - expBits - 1;

	auto result = static_cast<long double>(val & ((std::uint64_t{1} << significandBits) - 1));
	result /= (std::uint64_t{1} << significandBits);
	result += 1.0l;

	const auto bias = static_cast<std::uint64_t>((1 << (expBits - 1)) - 1);

	auto shift = static_cast<std::int64_t>(((val >> significandBits) & ((std::uint64_t{1} << expBits) - 1)) - bias);
	while (shift > 0) {
		result *= 2.0l;
		--shift;
	}
	while (shift < 0) {
		result /= 2.0l;
		++shift;
	}

	result *= ((val >> (bits - 1)) & 1) ? -1.0l : 1.0l;
	return result;
}

[[nodiscard]] constexpr auto htons(std::uint16_t value) noexcept -> std::uint16_t {
	if constexpr (net::isBigEndian()) {
		return value;
	} else {
		return static_cast<std::uint16_t>(((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
	}
}

[[nodiscard]] constexpr auto ntohs(std::uint16_t value) noexcept -> std::uint16_t {
	return net::htons(value);
}

[[nodiscard]] constexpr auto htonl(std::uint32_t value) noexcept -> std::uint32_t {
	if constexpr (net::isBigEndian()) {
		return value;
	} else {
		return static_cast<std::uint32_t>(((value & 0x000000FF) << 24) | ((value & 0x0000FF00) << 8) | ((value & 0x00FF0000) >> 8) |
		                                  ((value & 0xFF000000) >> 24));
	}
}

[[nodiscard]] constexpr auto ntohl(std::uint32_t value) noexcept -> std::uint32_t {
	return net::htonl(value);
}

[[nodiscard]] constexpr auto htonll(std::uint64_t value) noexcept -> std::uint64_t {
	if constexpr (net::isBigEndian()) {
		return value;
	} else {
		return static_cast<std::uint64_t>(((value & 0x00000000000000FF) << 56) | ((value & 0x000000000000FF00) << 40) |
		                                  ((value & 0x0000000000FF0000) << 24) | ((value & 0x00000000FF000000) << 8) |
		                                  ((value & 0x000000FF00000000) >> 8) | ((value & 0x0000FF0000000000) >> 24) |
		                                  ((value & 0x00FF000000000000) >> 40) | ((value & 0xFF00000000000000) >> 56));
	}
}

[[nodiscard]] constexpr auto ntohll(std::uint64_t value) noexcept -> std::uint64_t {
	return net::htonll(value);
}

template <typename T>
[[nodiscard]] constexpr auto htoni(T value) noexcept {
	if constexpr (sizeof(T) == 1) {
		return static_cast<std::uint8_t>(value);
	} else if constexpr (sizeof(T) == 2) {
		return net::htons(static_cast<std::uint16_t>(value));
	} else if constexpr (sizeof(T) == 4) {
		return net::htonl(static_cast<std::uint32_t>(value));
	} else if constexpr (sizeof(T) == 8) {
		return net::htonll(static_cast<std::uint64_t>(value));
	}
}

template <typename T>
[[nodiscard]] constexpr auto ntohi(T value) noexcept {
	if constexpr (sizeof(T) == 1) {
		return static_cast<std::uint8_t>(value);
	} else if constexpr (sizeof(T) == 2) {
		return net::ntohs(static_cast<std::uint16_t>(value));
	} else if constexpr (sizeof(T) == 4) {
		return net::ntohl(static_cast<std::uint32_t>(value));
	} else if constexpr (sizeof(T) == 8) {
		return net::ntohll(static_cast<std::uint64_t>(value));
	}
}

[[nodiscard]] constexpr auto htonf(float value) noexcept -> std::uint32_t {
	static_assert(sizeof(float) == 4);
	return net::htonl(static_cast<std::uint32_t>(net::pack754(static_cast<long double>(value), 32, 8)));
}

[[nodiscard]] constexpr auto ntohf(std::uint32_t value) noexcept -> float {
	static_assert(sizeof(float) == 4);
	return static_cast<float>(net::unpack754(static_cast<std::uint64_t>(net::ntohl(value)), 32, 8));
}

[[nodiscard]] constexpr auto htond(double value) noexcept -> std::uint64_t {
	static_assert(sizeof(double) == 8);
	return net::htonll(net::pack754(static_cast<long double>(value), 64, 11));
}

[[nodiscard]] constexpr auto ntohd(std::uint64_t value) noexcept -> double {
	static_assert(sizeof(double) == 8);
	return static_cast<double>(net::unpack754(net::ntohll(value), 64, 11));
}

static_assert(net::ntohl(net::htonl(0xDEADBEEF)) == 0xDEADBEEF);

static_assert(net::ntohf(net::htonf(0.5f)) == 0.5f);
static_assert(net::ntohf(net::htonf(133.7f)) == 133.7f);
static_assert(net::ntohf(net::htonf(0.0f)) == 0.0f);
static_assert(net::ntohf(net::htonf(0.0000000000001f)) == 0.0000000000001f);
static_assert(net::ntohf(net::htonf(-4.0f)) == -4.0f);

static_assert(net::ntohd(net::htond(0.5)) == 0.5);
static_assert(net::ntohd(net::htond(133.7)) == 133.7);
static_assert(net::ntohd(net::htond(0.0)) == 0.0);
static_assert(net::ntohd(net::htond(0.0000000000001)) == 0.0000000000001);
static_assert(net::ntohd(net::htond(-4.0)) == -4.0);

} // namespace net

#endif
