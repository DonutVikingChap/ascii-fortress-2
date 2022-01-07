#include "meta_client_commands.hpp"

#include "../../game/data/latency.hpp"     // Latency
#include "../../game/meta/meta_client.hpp" // MetaClient
#include "../../network/connection.hpp"    // net::sanitizeMessage
#include "../../utilities/algorithm.hpp"   // util::collect, util::transform, util::sort
#include "../../utilities/reference.hpp"   // util::Reference
#include "../../utilities/string.hpp"      // util::join
#include "../command.hpp"                  // cmd::...
#include "../command_utilities.hpp"        // cmd::...
#include "../script.hpp"                   // Script

#include <cassert>    // assert
#include <chrono>     // std::chrono::...
#include <fmt/core.h> // fmt::format
#include <ratio>      // std::milli

namespace {

CONVAR_CALLBACK(updateTimeout) {
	if (metaClient) {
		metaClient->updateTimeout();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateThrottle) {
	if (metaClient) {
		metaClient->updateThrottle();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateSendInterval) {
	if (metaClient) {
		metaClient->updateSendInterval();
	}
	return cmd::done();
}

} // namespace

// clang-format off
ConVarString		meta_address{					"meta_address",						"",		ConVar::CLIENT_SETTING | ConVar::NOT_RUNNING_META_CLIENT,	"Remote meta server address to connect to."};
ConVarIntMinMax		meta_port{						"meta_port",						0,		ConVar::CLIENT_SETTING | ConVar::NOT_RUNNING_META_CLIENT,	"Remote meta server port to connect to.", 0, 65535};
ConVarIntMinMax		meta_cl_port{					"meta_cl_port",						0,		ConVar::CLIENT_SETTING | ConVar::NOT_RUNNING_META_CLIENT,	"Port used by the meta client. Set to 0 to choose automatically.", 0, 65535};
ConVarFloatMinMax	meta_cl_timeout{				"meta_cl_timeout",					10.0f,	ConVar::CLIENT_SETTING,										"How many seconds to wait before we assume that the meta server is not responding.", 0.0f, -1.0f, updateTimeout};
ConVarIntMinMax		meta_cl_throttle_limit{			"meta_cl_throttle_limit",			6,		ConVar::CLIENT_SETTING,										"How many packets are allowed to be queued in the meta client send buffer before throttling the outgoing send rate.", 0, -1, updateThrottle};
ConVarIntMinMax		meta_cl_throttle_max_period{	"meta_cl_throttle_max_period",		6,		ConVar::CLIENT_SETTING,										"Maximum number of packet sends to skip in a row while the meta client send rate is throttled.", 0, -1, updateThrottle};
ConVarIntMinMax		meta_cl_max_server_connections{	"meta_cl_max_server_connections",	32,		ConVar::CLIENT_SETTING,										"Maximum number of simultaneous connections to open to game servers received from the meta server.", 1, 1000};
ConVarIntMinMax		meta_cl_sendrate{				"meta_cl_cmdrate",					10,		ConVar::CLIENT_SETTING,										"The rate (in Hz) at which to send packets to the server.", 1, 1000, updateSendInterval};
// clang-format on

CON_COMMAND(meta_is_connecting, "", ConCommand::META_CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Check if the meta client is connecting to the meta server.", {}, nullptr) {
	assert(metaClient);
	return cmd::done(metaClient->isConnecting());
}

CON_COMMAND(meta_refresh, "", ConCommand::META_CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Refresh the meta client's server list.", {}, nullptr) {
	assert(metaClient);
	if (!metaClient->refresh()) {
		return cmd::error("{}: Failed to refresh server list!", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(meta_has_received_ip_list, "", ConCommand::META_CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Check if the server ip list has been retrieved by the meta client.", {}, nullptr) {
	assert(metaClient);
	return cmd::done(metaClient->hasReceivedGameServerEndpoints());
}

CON_COMMAND(meta_ip_count, "", ConCommand::META_CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Get the current number of server ips retrieved by the meta client.", {}, nullptr) {
	assert(metaClient);
	return cmd::done(metaClient->gameServerEndpoints().size());
}

CON_COMMAND(meta_ip_list, "", ConCommand::META_CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Get the current list of server ips retrieved by the meta client.", {}, nullptr) {
	assert(metaClient);
	return cmd::done(metaClient->gameServerEndpoints() | util::transform(cmd::formatIpEndpoint) | util::join('\n'));
}

CON_COMMAND(meta_info_count, "", ConCommand::META_CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Get the current number of server infos retrieved by the meta client.", {}, nullptr) {
	assert(metaClient);
	return cmd::done(metaClient->metaInfo().size());
}

CON_COMMAND(meta_info, "", ConCommand::META_CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
			"Get the current server info retrieved by the meta client.", {}, nullptr) {
	assert(metaClient);
	static constexpr auto compareMetaInfo = [](const auto& lhs, const auto& rhs) {
		return lhs->ping < rhs->ping;
	};

	static constexpr auto formatMetaInfo = [](const auto& metaInfo) {
		return fmt::format(
			"{{\n"
			"  ip {}\n"
			"  hostname {}\n"
			"  version {}\n"
			"  map {}\n"
			"  players {}\n"
			"  bots {}\n"
			"  maxplayers {}\n"
			"  ping {}\n"
			"  tickrate {}\n"
			"}}",
			Script::escapedString(std::string{metaInfo->endpoint}),
			Script::escapedString(net::sanitizeMessage(metaInfo->info.hostName)),
			Script::escapedString(net::sanitizeMessage(metaInfo->info.gameVersion)),
			Script::escapedString(net::sanitizeMessage(metaInfo->info.mapName)),
			metaInfo->info.playerCount,
			metaInfo->info.botCount,
			metaInfo->info.maxPlayerCount,
			std::chrono::duration_cast<std::chrono::duration<Latency, std::milli>>(metaInfo->ping).count(),
			metaInfo->info.tickrate);
	};

	using Refs = std::vector<util::Reference<const MetaClient::ReceivedMetaInfo>>;

	return cmd::done(metaClient->metaInfo() | util::collect<Refs>() | util::sort(compareMetaInfo) | util::transform(formatMetaInfo) | util::join('\n'));
}
