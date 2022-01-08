#include "meta_server_commands.hpp"

#include "../../game/meta/meta_server.hpp" // MetaServer
#include "../../network/config.hpp"        // net::DISCONNECT_DURATION
#include "../../utilities/algorithm.hpp"   // util::collect, util::sort, util::transform
#include "../../utilities/file.hpp"        // util::dumpFile
#include "../../utilities/reference.hpp"   // util::Reference
#include "../../utilities/string.hpp"      // util::join
#include "../command.hpp"                  // cmd::...
#include "../command_utilities.hpp"        // cmd::...
#include "../script.hpp"                   // Script
#include "file_commands.hpp"               // data_dir, data_subdir_maps, data_subdir_cfg

#include <cassert>      // assert
#include <chrono>       // std::chrono::...
#include <fmt/core.h>   // fmt::format
#include <optional>     // std::optional
#include <string>       // std::string
#include <system_error> // std::error_code

namespace {

CONVAR_CALLBACK(updateTimeout) {
	if (metaServer) {
		metaServer->updateTimeout();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateThrottle) {
	if (metaServer) {
		metaServer->updateThrottle();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateSpamLimit) {
	if (metaServer) {
		metaServer->updateSpamLimit();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateTickrate) {
	if (metaServer) {
		metaServer->updateTickrate();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updateConfigAutoSaveInterval) {
	if (metaServer) {
		metaServer->updateConfigAutoSaveInterval();
	}
	return cmd::done();
}

CONVAR_CALLBACK(updatePrivateAddressOverride) {
	if (metaServer) {
		metaServer->updatePrivateAddressOverride();
	}
	return cmd::done();
}

constexpr auto DISCONNECT_DURATION_SECONDS = std::chrono::duration_cast<std::chrono::duration<float>>(net::DISCONNECT_DURATION).count();

} // namespace

// clang-format off
ConVarIntMinMax		meta_sv_port{						"meta_sv_port",							25600,							ConVar::SERVER_SETTING | ConVar::NOT_RUNNING_META_SERVER,	"Local port to use when starting a meta server.", 0, 65535};
ConVarString		meta_sv_config_file{				"meta_sv_config_file",					"meta_sv_config.cfg",			ConVar::HOST_SETTING,										"Main meta server config file to read at startup and save to at shutdown."};
ConVarString		meta_sv_autoexec_file{				"meta_sv_autoexec_file",				"meta_sv_autoexec.cfg",			ConVar::HOST_SETTING,										"Meta server autoexec file to read at startup."};
ConVarFloatMinMax	meta_sv_timeout{					"meta_sv_timeout",						10.0f,							ConVar::SERVER_SETTING,										"How many seconds to wait before booting a meta client that isn't sending messages.", 0.0f, -1.0f, updateTimeout};
ConVarIntMinMax		meta_sv_throttle_limit{				"meta_sv_throttle_limit",				6,								ConVar::SERVER_SETTING,										"How many packets are allowed to be queued in the meta server send buffer before throttling the outgoing send rate.", 0, -1, updateThrottle};
ConVarIntMinMax		meta_sv_throttle_max_period{		"meta_sv_throttle_max_period",			6,								ConVar::SERVER_SETTING,										"Maximum number of packet sends to skip in a row while the meta server send rate is throttled.", 0, -1, updateThrottle};
ConVarFloatMinMax	meta_sv_disconnect_cooldown{		"meta_sv_disconnect_cooldown",			DISCONNECT_DURATION_SECONDS,	ConVar::SERVER_SETTING,										"How many seconds to wait before letting a meta client connect again after disconnecting.", 0.0f, -1.0f};
ConVarFloatMinMax	meta_sv_heartbeat_interval{ 		"meta_sv_heartbeat_interval",   		1.0f,							ConVar::SERVER_SETTING,										"Interval at which the meta server should request heartbeats from connected game servers.", 0.01f, -1.0f};
ConVarFloatMinMax	meta_sv_afk_autokick_time{			"meta_sv_afk_autokick_time",			60.0f,							ConVar::SERVER_SETTING,										"Automatically kick meta clients if they haven't done anything for this many seconds (0 = unlimited).", 0.0f, -1.0f};
ConVarIntMinMax		meta_sv_max_ticks_per_frame{		"meta_sv_max_ticks_per_frame",			10,								ConVar::SERVER_SETTING,										"How many ticks that are allowed to run on one meta server frame.", 1, -1};
ConVarIntMinMax		meta_sv_spam_limit{					"meta_sv_spam_limit",					4,								ConVar::SERVER_SETTING,										"Maximum number of potential spam messages per second for the meta server to receive before kicking the sender. 0 = unlimited.", 0, -1, updateSpamLimit};
ConVarIntMinMax		meta_sv_tickrate{					"meta_sv_tickrate",						5,								ConVar::SERVER_SETTING,										"The rate (in Hz) at which the meta server updates.", 1, 1000, updateTickrate};
ConVarIntMinMax		meta_sv_max_clients{				"meta_sv_max_clients",					255,							ConVar::SERVER_SETTING,										"Maximum number of connections to handle simultaneously on the meta server. When the limit is hit, any remaining packets received from unconnected addresses will be ignored.", 0, -1};
ConVarIntMinMax		meta_sv_max_connecting_clients{		"meta_sv_max_connecting_clients",		10,								ConVar::SERVER_SETTING,										"Maximum number of new connections to handle simultaneously on the meta server. When the limit is hit, any remaining packets received from unconnected addresses will be ignored.", 0, -1};
ConVarIntMinMax		meta_sv_config_auto_save_interval{	"meta_sv_config_auto_save_interval",	5,								ConVar::SERVER_SETTING,										"Minutes between automatic meta server config saves. 0 = Disable autosave.", 0, -1, updateConfigAutoSaveInterval};
ConVarIntMinMax		meta_sv_max_connections_per_ip{		"meta_sv_max_connections_per_ip",		10,								ConVar::SERVER_SETTING,										"Maximum number of connections to accept from the same IP address on the meta server (0 = unlimited).", 0, -1};
ConVarString		meta_sv_private_address_override{	"meta_sv_private_address_override",		"",								ConVar::SERVER_SETTING,										"If non-empty, the meta server will advertise servers with private addresses as this address instead.", updatePrivateAddressOverride};
// clang-format on

CON_COMMAND(meta_sv_kick, "<ip>", ConCommand::META_SERVER | ConCommand::ADMIN_ONLY, "Kick a client from the meta server.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};
	const auto ip = net::IpAddress::parse(argv[1], ec);
	if (ec) {
		return cmd::error("{}: Couldn't parse ip address \"{}\": {}", self.getName(), argv[1], ec.message());
	}
	assert(metaServer);
	if (!metaServer->kickClient(ip)) {
		return cmd::error("{}: Client not found.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(meta_sv_ban, "<ip>", ConCommand::META_SERVER | ConCommand::ADMIN_ONLY, "Ban a client from the meta server.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};
	const auto ip = net::IpAddress::parse(argv[1], ec);
	if (ec) {
		return cmd::error("{}: Couldn't parse ip address \"{}\": {}", self.getName(), argv[1], ec.message());
	}
	assert(metaServer);
	metaServer->banClient(ip);

	return cmd::done();
}

CON_COMMAND(meta_sv_unban, "<ip>", ConCommand::META_SERVER | ConCommand::ADMIN_ONLY,
            "Remove an ip address from the meta server's banned client list.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};
	const auto ip = net::IpAddress::parse(argv[1], ec);
	if (ec) {
		return cmd::error("{}: Couldn't parse ip address \"{}\": {}", self.getName(), argv[1], ec.message());
	}
	assert(metaServer);
	if (!metaServer->unbanClient(ip)) {
		return cmd::error("{}: Ip address \"{}\" is not banned. Use \"{}\" for a list of banned ips.",
		                  self.getName(),
		                  std::string{ip},
		                  GET_COMMAND(meta_sv_ban_list).getName());
	}

	return cmd::done();
}

CON_COMMAND(meta_sv_ban_list, "", ConCommand::META_SERVER | ConCommand::ADMIN_ONLY, "List all banned ips on the meta server.", {}, nullptr) {
	assert(metaServer);
	return cmd::done(metaServer->getBannedClients() | util::transform(cmd::formatIpAddress) | util::join('\n'));
}

CON_COMMAND(meta_sv_writeconfig, "", ConCommand::META_SERVER | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Save the current meta server config.", {}, nullptr) {
	using BannedClientRef = util::Reference<const MetaServer::BannedClients::value_type>;
	using Refs = std::vector<BannedClientRef>;

	static constexpr auto compareBannedClientRefs = [](const auto& lhs, const auto& rhs) {
		return *lhs < *rhs;
	};

	static constexpr auto getBannedClientCommand = [](const auto& ip) {
		return fmt::format("{} {}", GET_COMMAND(meta_sv_ban).getName(), Script::escapedString(std::string{*ip}));
	};

	assert(metaServer);
	if (!util::dumpFile(fmt::format("{}/{}/{}", data_dir, data_subdir_cfg, meta_sv_config_file),
	                    fmt::format("{}\n"
	                                "\n"
	                                "// Banned IPs:\n"
	                                "{}\n",
	                                MetaServer::getConfigHeader(),
	                                metaServer->getBannedClients() | util::collect<Refs>() | util::sort(compareBannedClientRefs) |
	                                    util::transform(getBannedClientCommand) | util::join('\n')))) {
		return cmd::error("{}: Failed to save config file \"{}\"!", self.getName(), meta_sv_config_file);
	}
	return cmd::done();
}
