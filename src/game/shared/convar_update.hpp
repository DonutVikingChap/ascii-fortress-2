#ifndef AF2_SHARED_CONVAR_UPDATE_HPP
#define AF2_SHARED_CONVAR_UPDATE_HPP

#include "../../network/message.hpp" // net::TieInputOutputStreamableBase

#include <string> // std::string
#include <tuple>  // std::tie

struct ConVarUpdate final : net::TieInputOutputStreamableBase<ConVarUpdate> {
	std::string name{};
	std::string newValue{};

	[[nodiscard]] constexpr auto tie() noexcept {
		return std::tie(name, newValue);
	}

	[[nodiscard]] constexpr auto tie() const noexcept {
		return std::tie(name, newValue);
	}
};

#endif
