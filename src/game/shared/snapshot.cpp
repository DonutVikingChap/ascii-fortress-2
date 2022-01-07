#include "snapshot.hpp"

#include <algorithm> // std::find_if

auto Snapshot::findPlayerInfo(PlayerId id) -> ent::sh::PlayerInfo* {
	const auto it = std::find_if(playerInfo.begin(), playerInfo.end(), [id](const auto& playerInfo) { return playerInfo.id == id; });
	return (it == playerInfo.end()) ? nullptr : &*it;
}

auto Snapshot::findPlayerInfo(PlayerId id) const -> const ent::sh::PlayerInfo* {
	const auto it = std::find_if(playerInfo.begin(), playerInfo.end(), [id](const auto& playerInfo) { return playerInfo.id == id; });
	return (it == playerInfo.end()) ? nullptr : &*it;
}
