#include "inventory_server_commands.hpp"

#include "../../game/data/hat.hpp"           // Hat
#include "../../game/server/game_server.hpp" // GameServer
#include "../../utilities/algorithm.hpp"     // util::transform, util::collect, util::copy, util::contains
#include "../../utilities/string.hpp"        // util::join, util::stringTo
#include "../command.hpp"                    // cmd::...
#include "../command_utilities.hpp"          // cmd::...
#include "../suggestions.hpp"                // Suggestions, SUGGESTIONS

#include <cassert> // assert

namespace {

SUGGESTIONS(suggestInventoryId) {
	static constexpr auto formatId = [](const auto& id) {
		return util::toString(id);
	};
	if (i == 1 && server) {
		return server->getInventoryIds() | util::transform(formatId) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

SUGGESTIONS(suggestInventoryIdAndHat) {
	static constexpr auto formatId = [](const auto& id) {
		return util::toString(id);
	};
	if (i == 1 && server) {
		return server->getInventoryIds() | util::transform(formatId) | util::collect<Suggestions>();
	}

	static constexpr auto isValidHat = [](const auto& hat) {
		return hat != Hat::none();
	};
	static constexpr auto getHatName = [](const auto& hat) {
		return hat.getName();
	};
	if (i == 2) {
		return Hat::getAll() | util::filter(isValidHat) | util::transform(getHatName) | util::collect<Suggestions>();
	}
	return Suggestions{};
}

} // namespace

CON_COMMAND(sv_inventory_add, "<inventory_id> <ip> <username> <tokenhash>", ConCommand::SERVER, "Add an inventory with a pre-hashed token.", {}, nullptr) {
	if (argv.size() != 5) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	const auto ip = cmd::parseIpAddress(parseError, argv[2], "ip");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	auto tokenHash = crypto::FastHash{};
	if (argv[4].size() != tokenHash.size()) {
		return cmd::error("{}: Invalid token hash size ({}/{}).", self.getName(), argv[4].size(), tokenHash.size());
	}
	util::copy(argv[4], tokenHash.begin());

	assert(server);
	if (!server->addInventory(id, ip, argv[3], tokenHash)) {
		return cmd::error("{}: Failed to add inventory!", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(sv_inventory_remove, "<inventory_id>", ConCommand::SERVER, "Remove an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	if (!server->removeInventory(id)) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done();
}

CON_COMMAND(sv_inventory_list, "", ConCommand::SERVER, "List all inventories on the server.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(server);
	return cmd::done(server->getInventoryList());
}

CON_COMMAND(sv_inventory_exists, "<inventory_id>", ConCommand::SERVER, "Check if a certain inventory exists.", {}, suggestInventoryId) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->hasInventory(id));
}

CON_COMMAND(sv_inventory_get_points, "<inventory_id>", ConCommand::SERVER, "Get the number of points of an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	const auto* const score = server->inventoryPoints(id);
	if (!score) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done(*score);
}

CON_COMMAND(sv_inventory_set_points, "<inventory_id> <points>", ConCommand::SERVER, "Set the points of an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	const auto points = cmd::parseNumber<Score>(parseError, argv[2], "number of points");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	auto* const score = server->inventoryPoints(id);
	if (!score) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	*score = points;
	return cmd::done();
}

CON_COMMAND(sv_inventory_add_points, "<inventory_id> <points>", ConCommand::SERVER, "Add points to an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	const auto points = cmd::parseNumber<Score>(parseError, argv[2], "number of points");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	auto* const score = server->inventoryPoints(id);
	if (!score) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	*score += points;
	return cmd::done();
}

CON_COMMAND(sv_inventory_get_level, "<inventory_id>", ConCommand::SERVER, "Get the level of an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	const auto* const level = server->inventoryLevel(id);
	if (!level) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done(*level);
}

CON_COMMAND(sv_inventory_set_level, "<inventory_id> <level>", ConCommand::SERVER, "Set the level of an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	const auto levels = cmd::parseNumber<Score>(parseError, argv[2], "level");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	auto* const level = server->inventoryLevel(id);
	if (!level) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	*level = levels;
	return cmd::done();
}

CON_COMMAND(sv_inventory_add_level, "<inventory_id> <levels>", ConCommand::SERVER, "Add levels to an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	const auto levels = cmd::parseNumber<Score>(parseError, argv[2], "level");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	auto* const level = server->inventoryLevel(id);
	if (!level) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	*level += levels;
	return cmd::done();
}

CON_COMMAND(sv_inventory_get_hats, "<inventory_id>", ConCommand::SERVER, "List the hats in an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	const auto* const hats = server->getInventoryHats(id);
	if (!hats) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done(*hats | util::transform([](const auto& hat) { return hat.getName(); }) | util::join('\n'));
}

CON_COMMAND(sv_inventory_equip_hat, "<inventory_id> <hat>", ConCommand::SERVER, "Make an inventory equip a certain hat.", {}, suggestInventoryIdAndHat) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	const auto hat = Hat::findByName(argv[2]);
	if (hat == Hat::none()) {
		return cmd::error("{}: Invalid hat \"{}\".", self.getName(), argv[2]);
	}

	assert(server);
	if (!server->equipInventoryHat(id, hat)) {
		return cmd::error("{}: Couldn't equip hat for inventory \"{}\".", self.getName(), argv[1]);
	}
	return cmd::done();
}

CON_COMMAND(sv_inventory_unequip_hat, "<inventory_id> [hat]", ConCommand::SERVER, "Make an inventory unequip a certain hat.", {}, suggestInventoryIdAndHat) {
	if (argv.size() != 2 && argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (argv.size() == 3) {
		const auto hat = Hat::findByName(argv[2]);
		if (hat == Hat::none()) {
			return cmd::error("{}: Invalid hat \"{}\".", self.getName(), argv[2]);
		}

		assert(server);
		if (!server->unequipInventoryHat(id, hat)) {
			return cmd::error("{}: Couldn't unequip hat for inventory \"{}\".", self.getName(), argv[1]);
		}
	} else {
		assert(server);
		if (!server->equipInventoryHat(id, Hat::none())) {
			return cmd::error("{}: Couldn't unequip hat for inventory \"{}\".", self.getName(), argv[1]);
		}
	}
	return cmd::done();
}

CON_COMMAND(sv_inventory_get_equipped_hat, "<inventory_id>", ConCommand::SERVER, "Get the equipped hat of an inventory.", {}, suggestInventoryId) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	assert(server);
	return cmd::done(server->getEquippedInventoryHat(id).getName());
}

CON_COMMAND(sv_inventory_has_hat, "<inventory_id> <hat>", ConCommand::SERVER, "Check if an inventory has a certain hat.", {}, suggestInventoryIdAndHat) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	const auto hat = Hat::findByName(argv[2]);
	if (hat == Hat::none()) {
		return cmd::error("{}: Invalid hat \"{}\".", self.getName(), argv[2]);
	}

	assert(server);
	const auto* const hats = server->getInventoryHats(id);
	if (!hats) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done(util::contains(*hats, hat));
}

CON_COMMAND(sv_inventory_give_hat, "<inventory_id> <hat>", ConCommand::SERVER, "Add a hat to an inventory.", {}, suggestInventoryIdAndHat) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto id = INVENTORY_ID_INVALID;
	if (!util::stringTo<InventoryId>(id, argv[1])) {
		return cmd::error("{}: Invalid id \"{}\".", argv[1]);
	}

	const auto hat = Hat::findByName(argv[2]);
	if (hat == Hat::none()) {
		return cmd::error("{}: Invalid hat \"{}\".", self.getName(), argv[2]);
	}

	assert(server);
	if (!server->giveInventoryHat(id, hat)) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done();
}

CON_COMMAND(sv_inventory_remove_hat, "<inventory_id> <hat>", ConCommand::SERVER, "Remove a hat from an inventory.", {}, suggestInventoryIdAndHat) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto id = cmd::parseNumber<InventoryId>(parseError, argv[1], "inventory id");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	const auto hat = Hat::findByName(argv[2]);
	if (hat == Hat::none()) {
		return cmd::error("{}: Invalid hat \"{}\".", self.getName(), argv[2]);
	}

	assert(server);
	if (!server->removeInventoryHat(id, hat)) {
		return cmd::error("{}: Inventory \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done();
}
