#ifndef AF2_SHARED_RESOURCE_INFO_HPP
#define AF2_SHARED_RESOURCE_INFO_HPP

#include "../../network/message.hpp" // net::TieInputOutputStreamableBase
#include "../../utilities/crc.hpp"   // util::CRC32

#include <cstdint> // std::uint64_t
#include <string>  // std::string
#include <tuple>   // std::tie
#include <utility> // std::move

struct ResourceInfo final : net::TieInputOutputStreamableBase<ResourceInfo> {
	std::string name{};
	util::CRC32 nameHash{};
	util::CRC32 fileHash{};
	std::uint64_t size = 0;
	bool isText = false;
	bool canDownload = false;

	ResourceInfo() noexcept = default;

	ResourceInfo(std::string name, util::CRC32 nameHash, util::CRC32 fileHash, std::uint64_t size, bool isText, bool canDownload)
		: name(std::move(name))
		, nameHash(nameHash)
		, fileHash(fileHash)
		, size(size)
		, isText(isText)
		, canDownload(canDownload) {}

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(name, nameHash, fileHash, size, isText, canDownload);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(name, nameHash, fileHash, size, isText, canDownload);
	}
};

#endif
