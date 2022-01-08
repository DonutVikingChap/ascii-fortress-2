#ifndef AF2_NETWORK_ENDPOINT_HPP
#define AF2_NETWORK_ENDPOINT_HPP

#include "platform.hpp" // net::htonl, net::ntohl, net::htons, net::ntohs, net::WSAErrorCategory, ...

#include <array>        // std::array
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint32_t, std::uint16_t, std::uint8_t
#include <functional>   // std::hash<std::uint32_t>, std::hash<std::uint16_t>
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <system_error> // std::error_code, std::error_category, std::errc, std::make_error_code(std::errc)
#include <utility>      // std::make_pair

namespace net {

enum class IpAddressError : int {
	AGAIN = EAI_AGAIN,
	BADFLAGS = EAI_BADFLAGS,
	FAIL = EAI_FAIL,
	FAMILY = EAI_FAMILY,
	MEMORY = EAI_MEMORY,
	NODATA = EAI_NODATA,
	NONAME = EAI_NONAME,
	SERVICE = EAI_SERVICE,
	SOCKTYPE = EAI_SOCKTYPE,
#ifndef _WIN32
	ADDRFAMILY = EAI_ADDRFAMILY,
	SYSTEM = EAI_SYSTEM,
	INPROGRESS = EAI_INPROGRESS,
	CANCELED = EAI_CANCELED,
	NOTCANCELED = EAI_NOTCANCELED,
	ALLDONE = EAI_ALLDONE,
	INTR = EAI_INTR,
	IDN_ENCODE = EAI_IDN_ENCODE,
#endif
};

} // namespace net

template <>
struct std::is_error_code_enum<net::IpAddressError> : std::true_type {};

namespace net {

struct IpAddressErrorCategory final : std::error_category {
	[[nodiscard]] auto name() const noexcept -> const char* override;
	[[nodiscard]] auto message(int condition) const -> std::string override;

	[[nodiscard]] static auto instance() noexcept -> const IpAddressErrorCategory& {
		static const auto category = IpAddressErrorCategory{};
		return category;
	}
};

[[nodiscard]] inline auto make_error_code(IpAddressError err) -> std::error_code {
	return std::error_code{static_cast<int>(err), IpAddressErrorCategory::instance()};
}

class IpAddress final {
public:
	[[nodiscard]] static constexpr auto any() noexcept -> IpAddress {
		return IpAddress{INADDR_ANY};
	}

	[[nodiscard]] static constexpr auto none() noexcept -> IpAddress {
		return IpAddress{INADDR_NONE};
	}

	[[nodiscard]] static constexpr auto localhost() noexcept -> IpAddress {
		return IpAddress{INADDR_LOOPBACK};
	}

	[[nodiscard]] static constexpr auto broadcast() noexcept -> IpAddress {
		return IpAddress{INADDR_BROADCAST};
	}

	[[nodiscard]] static auto parse(std::string_view str, std::error_code& ec) noexcept -> IpAddress;
	[[nodiscard]] static auto resolve(const char* hostName, const char* service, std::error_code& ec) -> IpAddress;
	[[nodiscard]] static auto resolve(const char* host, std::error_code& ec) -> IpAddress;

	[[nodiscard]] static auto getLocalAddress(std::error_code& ec) -> IpAddress;

	constexpr IpAddress() noexcept = default;

	constexpr IpAddress(std::uint8_t byte0, std::uint8_t byte1, std::uint8_t byte2, std::uint8_t byte3) noexcept
		: m_bytes({byte0, byte1, byte2, byte3}) {}

	constexpr explicit IpAddress(std::uint32_t address) noexcept
		: m_bytes({
			  static_cast<std::uint8_t>((address >> 24) & 0xFF),
			  static_cast<std::uint8_t>((address >> 16) & 0xFF),
			  static_cast<std::uint8_t>((address >> 8) & 0xFF),
			  static_cast<std::uint8_t>((address >> 0) & 0xFF),
		  }) {}

	constexpr explicit operator std::uint32_t() const noexcept {
		return (m_bytes[0] << 24) | (m_bytes[1] << 16) | (m_bytes[2] << 8) | (m_bytes[3] << 0);
	}

	explicit operator std::string() const;

	[[nodiscard]] constexpr auto isLoopback() const noexcept -> bool {
		return m_bytes[0] == 127;
	}

	[[nodiscard]] constexpr auto isPrivate() const noexcept -> bool {
		return m_bytes[0] == 10 || (m_bytes[0] == 172 && m_bytes[1] >= 16 && m_bytes[1] < 32) || (m_bytes[0] == 192 && m_bytes[1] == 168);
	}

	[[nodiscard]] friend constexpr auto operator==(IpAddress lhs, IpAddress rhs) noexcept -> bool {
		return static_cast<std::uint32_t>(lhs) == static_cast<std::uint32_t>(rhs);
	}

	[[nodiscard]] friend constexpr auto operator!=(IpAddress lhs, IpAddress rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] friend constexpr auto operator<(IpAddress lhs, IpAddress rhs) noexcept -> bool {
		return static_cast<std::uint32_t>(lhs) < static_cast<std::uint32_t>(rhs);
	}

	[[nodiscard]] friend constexpr auto operator<=(IpAddress lhs, IpAddress rhs) noexcept -> bool {
		return !(rhs < lhs);
	}

	[[nodiscard]] friend constexpr auto operator>(IpAddress lhs, IpAddress rhs) noexcept -> bool {
		return rhs < lhs;
	}

	[[nodiscard]] friend constexpr auto operator>=(IpAddress lhs, IpAddress rhs) noexcept -> bool {
		return !(lhs < rhs);
	}

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, IpAddress address) -> Stream& {
		return stream << static_cast<std::uint32_t>(address);
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, IpAddress& address) -> Stream& {
		auto value = std::uint32_t{};
		stream >> value;
		address = IpAddress{value};
		return stream;
	}

private:
	std::array<std::uint8_t, 4> m_bytes{};
};

} // namespace net

template <>
class std::hash<net::IpAddress> {
public:
	auto operator()(const net::IpAddress& address) const -> std::size_t {
		return m_hasher(static_cast<std::uint32_t>(address));
	}

private:
	std::hash<std::uint32_t> m_hasher{};
};

namespace net {

using PortNumber = std::uint16_t;

class IpEndpoint final {
public:
	[[nodiscard]] static auto parse(std::string_view str, std::error_code& ec) noexcept -> IpEndpoint;
	[[nodiscard]] static auto resolve(const char* hostName, const char* service, std::error_code& ec) -> IpEndpoint;
	[[nodiscard]] static auto resolve(const char* host, std::error_code& ec) -> IpEndpoint;

	constexpr IpEndpoint() noexcept = default;

	constexpr explicit IpEndpoint(IpAddress address, PortNumber port = 0) {
		m_addr.sin_family = AF_INET;
#ifdef __APPLE__
		m_addr.sin_len = sizeof(m_addr);
#endif
		m_addr.sin_addr.s_addr = net::htonl(static_cast<std::uint32_t>(address));
		m_addr.sin_port = net::htons(port);
	}

	explicit operator std::string() const;

	[[nodiscard]] friend constexpr auto operator==(IpEndpoint lhs, IpEndpoint rhs) noexcept -> bool {
		return lhs.getAddress() == rhs.getAddress() && lhs.getPort() == rhs.getPort();
	}

	[[nodiscard]] friend constexpr auto operator!=(IpEndpoint lhs, IpEndpoint rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] friend constexpr auto operator<(IpEndpoint lhs, IpEndpoint rhs) noexcept -> bool {
		return std::make_pair(lhs.getAddress(), lhs.getPort()) < std::make_pair(rhs.getAddress(), rhs.getPort());
	}

	[[nodiscard]] friend constexpr auto operator<=(IpEndpoint lhs, IpEndpoint rhs) noexcept -> bool {
		return !(rhs < lhs);
	}

	[[nodiscard]] friend constexpr auto operator>(IpEndpoint lhs, IpEndpoint rhs) noexcept -> bool {
		return rhs < lhs;
	}

	[[nodiscard]] friend constexpr auto operator>=(IpEndpoint lhs, IpEndpoint rhs) noexcept -> bool {
		return !(lhs < rhs);
	}

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, IpEndpoint endpoint) -> Stream& {
		return stream << endpoint.getAddress() << endpoint.getPort();
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, IpEndpoint& endpoint) -> Stream& {
		auto address = IpAddress{};
		auto port = PortNumber{};
		stream >> address >> port;
		endpoint = IpEndpoint{address, port};
		return stream;
	}

	[[nodiscard]] constexpr auto getAddress() const noexcept -> IpAddress {
		return IpAddress{net::ntohl(m_addr.sin_addr.s_addr)};
	}

	[[nodiscard]] constexpr auto getPort() const noexcept -> PortNumber {
		return PortNumber{net::ntohs(m_addr.sin_port)};
	}

	[[nodiscard]] constexpr auto get() noexcept -> sockaddr_in& {
		return m_addr;
	}

	[[nodiscard]] constexpr auto get() const noexcept -> const sockaddr_in& {
		return m_addr;
	}

private:
	sockaddr_in m_addr{};
};

} // namespace net

template <>
class std::hash<net::IpEndpoint> {
public:
	auto operator()(const net::IpEndpoint& endpoint) const -> std::size_t {
		return (53 + m_addressHasher(endpoint.getAddress())) * 53 + m_portHasher(endpoint.getPort());
	}

private:
	std::hash<net::IpAddress> m_addressHasher{};
	std::hash<net::PortNumber> m_portHasher{};
};

#endif
