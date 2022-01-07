#include "inventory_client.hpp"

#include "../../console/commands/inventory_client_commands.hpp" // cvar_hat
#include "../../console/con_command.hpp"                        // ConCommand, CON_COMMAND
#include "../../console/convar.hpp"                             // ConVar...
#include "../../console/script.hpp"                             // Script
#include "../../network/crypto.hpp"                             // crypto::...
#include "../../utilities/algorithm.hpp"                        // util::transform, util::collect
#include "../../utilities/reference.hpp"                        // util::Reference
#include "../../utilities/string.hpp"                           // util::join

#include <fmt/core.h>  // fmt::format
#include <string_view> // std::string_view

InventoryClient::~InventoryClient() {
	cvar_hat.cvar().restoreLocalValueSilent();
}

auto InventoryClient::initInventoryClient() noexcept -> bool { // NOLINT(readability-convert-member-functions-to-static)
	return crypto::init();
}

auto InventoryClient::addInventory(net::IpEndpoint serverEndpoint, InventoryId inventoryId, const InventoryToken& inventoryToken) -> bool {
	return m_inventories.try_emplace(serverEndpoint, inventoryId, inventoryToken).second;
}

auto InventoryClient::getInventory(net::IpEndpoint serverEndpoint) -> InventoryClient::Inventory& {
	return m_inventories[serverEndpoint];
}

auto InventoryClient::hasInventory(net::IpEndpoint serverEndpoint) const -> bool {
	return m_inventories.count(serverEndpoint) != 0;
}

auto InventoryClient::removeInventory(net::IpEndpoint serverEndpoint) -> bool {
	return m_inventories.erase(serverEndpoint) != 0;
}

auto InventoryClient::getInventoryList() const -> std::string {
	static constexpr auto formatInventory = [](const auto& kv) {
		const auto& [serverEndpoint, inventory] = kv;
		return fmt::format("{}: {{id: {}, token: {}}}",
						   Script::escapedString(std::string{serverEndpoint}),
						   inventory.id,
						   Script::escapedString(std::string_view{reinterpret_cast<const char*>(inventory.token.data()), inventory.token.size()}));
	};

	return m_inventories | util::transform(formatInventory) | util::join('\n');
}

auto InventoryClient::getInventoryConfig() const -> std::string {
	using InventoryRef = util::Reference<const Inventories::value_type>;
	using Refs = std::vector<InventoryRef>;

	static constexpr auto compareInventoryRefs = [](const auto& lhs, const auto& rhs) {
		return lhs->first < rhs->first;
	};

	static constexpr auto getInventoryCommand = [](const auto& kv) {
		const auto& [serverEndpoint, inventory] = *kv;
		return fmt::format("{} {} {} {}",
						   GET_COMMAND(cl_inventory_add).getName(),
						   Script::escapedString(std::string{serverEndpoint}),
						   inventory.id,
						   Script::escapedString(std::string_view{reinterpret_cast<const char*>(inventory.token.data()), inventory.token.size()}));
	};

	return m_inventories | util::collect<Refs>() | util::sort(compareInventoryRefs) | util::transform(getInventoryCommand) | util::join('\n');
}

auto InventoryClient::getInventoryIps() const -> std::vector<net::IpEndpoint> {
	static constexpr auto getEndpoint = [](const auto& kv) {
		return kv.first;
	};
	return m_inventories | util::transform(getEndpoint) | util::collect<std::vector<net::IpEndpoint>>();
}

auto InventoryClient::writeInventoryEquipHatRequest(Hat hat) -> bool {
	return this->write(msg::sv::out::InventoryEquipHatRequest{{}, hat});
}

auto InventoryClient::handleMessage(msg::cl::in::InventoryEquipHat&& msg) -> void { // NOLINT(readability-convert-member-functions-to-static)
	cvar_hat.cvar().overrideLocalValueSilent(msg.hat.getName());
}
