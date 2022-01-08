#include "inventory_client_commands.hpp"

#include "../../game/client/game_client.hpp" // GameClient
#include "../../game/data/hat.hpp"           // Hat
#include "../../network/endpoint.hpp"        // net::IpEndpoint
#include "../../utilities/algorithm.hpp"     // util::transform, util::collect, util::copy
#include "../command.hpp"                    // cmd::...
#include "../command_utilities.hpp"          // cmd::...
#include "../suggestions.hpp"                // Suggestions, SUGGESTIONS

#include <string>       // std::string
#include <system_error> // std::error_code

namespace {

CONVAR_CALLBACK(onNewHat) {
	if (client) {
		const auto value = std::string{self.getRaw()};
		self.restoreLocalValueSilent();
		self.setSilent(value);
		self.overrideLocalValueSilent(oldVal);
		if (!client->writeInventoryEquipHatRequest(Hat::findByName(value))) {
			return cmd::error("Failed to write equip hat request to server!");
		}
	}
	return cmd::done();
}

} // namespace

// clang-format off
ConVarString cvar_hat{"hat", "", ConVar::CLIENT_SETTING, "Currently equipped hat.", onNewHat};
// clang-format on

namespace {

SUGGESTIONS(suggestInventoryIp) {
	if (i == 1 && client) {
		return client->getInventoryIps() | util::transform(cmd::formatIpEndpoint) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

} // namespace

CON_COMMAND(cl_inventory_add, "<server_ip> <inventory_id> <token>", ConCommand::CLIENT | ConCommand::ADMIN_ONLY,
            "Add inventory credentials for a certain server.", {}, nullptr) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};
	const auto endpoint = net::IpEndpoint::resolve(argv[1].c_str(), ec);
	if (ec) {
		return cmd::error("{}: Couldn't resolve server ip \"{}\": {}", argv[1], ec.message());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[2], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	auto token = InventoryToken{};
	if (argv[3].size() != token.size()) {
		return cmd::error("{}: Invalid token size ({}/{}).", self.getName(), argv[3].size(), token.size());
	}
	util::copy(argv[3], token.begin());

	assert(client);
	if (!client->addInventory(endpoint, id, token)) {
		return cmd::error("{}: Failed to add inventory!", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(cl_inventory_remove, "<server_ip>", ConCommand::CLIENT | ConCommand::ADMIN_ONLY,
            "Remove inventory credentials for a certain server.", {}, suggestInventoryIp) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};
	const auto endpoint = net::IpEndpoint::resolve(argv[1].c_str(), ec);
	if (ec) {
		return cmd::error("{}: Couldn't resolve server ip \"{}\": {}", argv[1], ec.message());
	}

	assert(client);
	if (!client->removeInventory(endpoint)) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(cl_inventory_list, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY, "List all inventories on the client.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(client);
	return cmd::done(client->getInventoryList());
}

CON_COMMAND(cl_inventory_exists, "<server_ip>", ConCommand::CLIENT | ConCommand::ADMIN_ONLY, "Check if a certain inventory exists.", {}, suggestInventoryIp) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto ec = std::error_code{};
	const auto endpoint = net::IpEndpoint::resolve(argv[1].c_str(), ec);
	if (ec) {
		return cmd::error("{}: Invalid server ip \"{}\": {}", argv[1], ec.message());
	}

	assert(client);
	return cmd::done(client->hasInventory(endpoint));
}
