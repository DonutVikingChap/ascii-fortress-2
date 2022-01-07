#include "platform.hpp"

#ifdef _WIN32

#include <array>          // std::array
#include <basetsd.h>      // DWORD
#include <cstddef>        // std::size_t
#include <stringapiset.h> // WideCharToMultiByte
#include <winbase.h>      // FormatMessageW, FORMAT_MESSAGE_FROM_SYSTEM
#include <windef.h>       // MAKEWORD
#include <winnt.h>        // MAKELANGID, LANG_NEUTRAL, SUBLANG_DEFAULT

namespace net {
namespace {

struct WSAManager final {
	[[nodiscard]] WSAManager() noexcept {
		auto wsaData = WSADATA{};
		WSAStartup(MAKEWORD(2, 2), &wsaData);
	}

	~WSAManager() {
		WSACleanup();
	}

	WSAManager(const WSAManager&) = delete;
	WSAManager(WSAManager&&) = delete;
	auto operator=(const WSAManager&) -> WSAManager& = delete;
	auto operator=(WSAManager&&) -> WSAManager& = delete;
} wsaInitializer{};

} // namespace

auto WSAErrorCategory::name() const noexcept -> const char* {
	return "WSA";
}

auto WSAErrorCategory::message(int condition) const -> std::string {
	auto buffer = std::array<wchar_t, 256>{};
	auto size = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
							   nullptr,
							   static_cast<DWORD>(condition),
							   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							   buffer.data(),
							   static_cast<DWORD>(buffer.size()),
							   nullptr);
	while (size > 0 && (buffer[size - 1] == '\r' || buffer[size - 1] == '\n')) {
		--size;
	}
	if (size == 0) {
		return "Unknown";
	}
	auto str = std::string(std::size_t{512}, '\0');
	const auto length = WideCharToMultiByte(CP_UTF8, 0, buffer.data(), static_cast<int>(size), str.data(), static_cast<int>(str.size()), nullptr, nullptr);
	if (length >= 0) {
		str.resize(static_cast<std::size_t>(length));
	} else {
		str.clear();
	}
	return str;
}

} // namespace net

#endif
