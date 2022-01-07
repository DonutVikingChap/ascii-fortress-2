#include "endpoint.hpp"

#include "../utilities/string.hpp" // util::stringTo
#include "socket.hpp"              // Socket

#include <fmt/core.h> // fmt::format

namespace net {

auto IpAddressErrorCategory::name() const noexcept -> const char* {
	return "addrinfo";
}

auto IpAddressErrorCategory::message(int condition) const -> std::string {
#ifdef _WIN32
	return WSAErrorCategory::instance().message(condition);
#else
	return gai_strerror(condition);
#endif
}

auto IpAddress::parse(std::string_view str, std::error_code& ec) noexcept -> IpAddress {
	const auto dotPos1 = str.find('.');
	if (dotPos1 == std::string_view::npos) {
		ec = make_error_code(std::errc::invalid_argument);
		return IpAddress{};
	}
	const auto dotPos2 = str.find('.', dotPos1 + 1);
	if (dotPos2 == std::string_view::npos) {
		ec = make_error_code(std::errc::invalid_argument);
		return IpAddress{};
	}
	const auto dotPos3 = str.find('.', dotPos2 + 1);
	if (dotPos3 == std::string_view::npos) {
		ec = make_error_code(std::errc::invalid_argument);
		return IpAddress{};
	}
	const auto bytePos0 = std::size_t{0};
	const auto byte0 = util::stringTo<std::uint8_t>(str.substr(bytePos0, dotPos1 - bytePos0));
	if (!byte0) {
		ec = make_error_code(std::errc::invalid_argument);
		return IpAddress{};
	}
	const auto bytePos1 = dotPos1 + 1;
	const auto byte1 = util::stringTo<std::uint8_t>(str.substr(bytePos1, dotPos2 - bytePos1));
	if (!byte1) {
		ec = make_error_code(std::errc::invalid_argument);
		return IpAddress{};
	}
	const auto bytePos2 = dotPos2 + 1;
	const auto byte2 = util::stringTo<std::uint8_t>(str.substr(bytePos2, dotPos3 - bytePos2));
	if (!byte2) {
		ec = make_error_code(std::errc::invalid_argument);
		return IpAddress{};
	}
	const auto bytePos3 = dotPos3 + 1;
	const auto byte3 = util::stringTo<std::uint8_t>(str.substr(bytePos3, str.size() - bytePos3));
	if (!byte3) {
		ec = make_error_code(std::errc::invalid_argument);
		return IpAddress{};
	}
	ec.clear();
	return IpAddress{*byte0, *byte1, *byte2, *byte3};
}

auto IpAddress::resolve(const char* hostName, const char* service, std::error_code& ec) -> IpAddress {
	return IpEndpoint::resolve(hostName, service, ec).getAddress();
}

auto IpAddress::resolve(const char* host, std::error_code& ec) -> IpAddress {
	return IpEndpoint::resolve(host, ec).getAddress();
}

auto IpAddress::getLocalAddress(std::error_code& ec) -> IpAddress {
	auto socket = Socket{};
	socket.create(Socket::ProtocolType::UDP, ec);
	if (ec) {
		return IpAddress{};
	}
	socket.connect(IpEndpoint{IpAddress{10, 255, 255, 255}, 9}, ec);
	if (ec) {
		return IpAddress{};
	}
	return socket.getLocalEndpoint(ec).getAddress();
}

IpAddress::operator std::string() const {
	return fmt::format("{}.{}.{}.{}", static_cast<int>(m_bytes[0]), static_cast<int>(m_bytes[1]), static_cast<int>(m_bytes[2]), static_cast<int>(m_bytes[3]));
}

auto IpEndpoint::parse(std::string_view str, std::error_code& ec) noexcept -> IpEndpoint {
	if (const auto colonPos = str.rfind(':'); colonPos != std::string::npos) {
		if (const auto port = util::stringTo<net::PortNumber>(str.substr(colonPos + 1))) {
			const auto addr = IpAddress::parse(str.substr(0, colonPos), ec);
			return (ec) ? IpEndpoint{} : IpEndpoint{addr, *port};
		}
		ec = make_error_code(std::errc::invalid_argument);
		return IpEndpoint{};
	}
	const auto addr = IpAddress::parse(str, ec);
	return (ec) ? IpEndpoint{} : IpEndpoint{addr, 0};
}

auto IpEndpoint::resolve(const char* hostName, const char* service, std::error_code& ec) -> IpEndpoint {
	auto hints = addrinfo{};
	hints.ai_family = AF_INET;
	addrinfo* info = nullptr;
	const auto dnsResult = getaddrinfo(hostName, service, &hints, &info);
	if (dnsResult != 0) {
#ifdef _WIN32
		ec = make_error_code(static_cast<IpAddressError>(dnsResult));
#else
		if (dnsResult == EAI_SYSTEM && errno != 0) {
			ec = std::error_code{errno, std::generic_category()};
		} else {
			ec = make_error_code(static_cast<IpAddressError>(dnsResult));
		}
#endif
		return IpEndpoint{};
	}
	if (!info) {
		ec = make_error_code(IpAddressError::FAIL);
		return IpEndpoint{};
	}
	auto result = IpEndpoint{};
	result.m_addr = *reinterpret_cast<const sockaddr_in*>(info->ai_addr);
	freeaddrinfo(info);
	ec.clear();
	return result;
}

auto IpEndpoint::resolve(const char* host, std::error_code& ec) -> IpEndpoint {
	const auto hostString = std::string_view{host};
	if (const auto lastColonPos = hostString.rfind(':'); lastColonPos != std::string_view::npos) {
		const auto hostName = std::string{hostString.substr(0, lastColonPos)};
		const auto service = host + lastColonPos + 1;
		return IpEndpoint::resolve(hostName.c_str(), service, ec);
	}
	return IpEndpoint::resolve(host, nullptr, ec);
}

IpEndpoint::operator std::string() const {
	const auto address = this->getAddress();
	const auto port = this->getPort();
	return (port == 0) ? std::string{address} : fmt::format("{}:{}", std::string{address}, port);
}

} // namespace net
