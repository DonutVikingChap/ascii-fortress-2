#include "inventory_server.hpp"

#include "../../console/commands/inventory_server_commands.hpp" // sv_inventory_...
#include "../../console/script.hpp"                             // Script
#include "../../utilities/algorithm.hpp"                        // util::transform, util::collect, util::erase, util::contains
#include "../../utilities/string.hpp"                           // util::join

#include <algorithm>   // std::max, std::lower_bound
#include <cassert>     // assert
#include <fmt/core.h>  // fmt::format
#include <string_view> // std::string_view

auto InventoryServer::initInventoryServer() noexcept -> bool { // NOLINT(readability-convert-member-functions-to-static)
	return crypto::init();
}

auto InventoryServer::createInventory(net::IpAddress address, std::string username) -> std::pair<InventoryId, InventoryToken> {
	const auto id = m_latestId + 1;

	auto token = InventoryToken{};
	crypto::generateAccessToken(token);
	if (!m_inventories.contains<INVENTORY_ID>(id)) {
		m_inventories.erase<INVENTORY_ADDRESS>(address);
		if (auto tokenHash = crypto::FastHash{}; crypto::fastHash(tokenHash, util::asBytes(util::Span{token}))) {
			m_inventories.emplace_back(Inventory{std::move(username), tokenHash}, id, address);
			++m_latestId;
			return {id, token};
		}
	}
	token.fill(0);
	return {INVENTORY_ID_INVALID, token};
}

auto InventoryServer::accessInventory(InventoryId id, const InventoryToken& token, net::IpAddress address, std::string username) -> bool {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		auto& [inventory, inventoryId, inventoryAddress] = *it;
		if (crypto::verifyFastHash(inventory.tokenHash, util::asBytes(util::Span{token}))) {
			inventory.username = std::move(username);
			m_inventories.set<INVENTORY_ADDRESS>(it, address);
			return true;
		}
	}
	return false;
}

auto InventoryServer::addInventory(InventoryId id, net::IpAddress address, std::string username, const crypto::FastHash& tokenHash) -> bool {
	if (!m_inventories.contains<INVENTORY_ID>(id) && !m_inventories.contains<INVENTORY_ADDRESS>(address)) {
		m_inventories.emplace_back(Inventory{std::move(username), tokenHash}, id, address);
		m_latestId = std::max(m_latestId, id);
		return true;
	}
	return false;
}

auto InventoryServer::hasInventory(InventoryId id) const -> bool {
	return m_inventories.contains<INVENTORY_ID>(id);
}

auto InventoryServer::removeInventory(InventoryId id) -> bool {
	return m_inventories.erase<INVENTORY_ID>(id) != 0;
}

auto InventoryServer::getInventoryList() const -> std::string {
	using InventoryRef = util::Reference<const Inventories::value_type>;
	using Refs = std::vector<InventoryRef>;

	static constexpr auto compareInventoryRefs = [](const auto& lhs, const auto& rhs) {
		return lhs->template get<INVENTORY_ID>() < rhs->template get<INVENTORY_ID>();
	};

	static constexpr auto formatInventory = [](const auto& elem) {
		static constexpr auto formatHat = [](const auto& hat) {
			return fmt::format("    {}", hat.getName());
		};

		const auto& [inventory, inventoryId, inventoryAddress] = *elem;
		return fmt::format(
			"{}. {} ({}):\n"
			"  Points: {}\n"
			"  Level: {}\n"
			"  Hats:\n"
			"{}",
			inventoryId,
			Script::escapedString(std::string{inventoryAddress}),
			Script::escapedString(inventory.username),
			inventory.points,
			inventory.level,
			inventory.hats | util::transform(formatHat) | util::join('\n'));
	};

	return util::collect<Refs>(m_inventories) | util::sort(compareInventoryRefs) | util::transform(formatInventory) | util::join('\n');
}

auto InventoryServer::getInventoryConfig() const -> std::string {
	using InventoryRef = util::Reference<const Inventories::value_type>;
	using Refs = std::vector<InventoryRef>;

	static constexpr auto compareInventoryRefs = [](const auto& lhs, const auto& rhs) {
		return lhs->template get<INVENTORY_ID>() < rhs->template get<INVENTORY_ID>();
	};

	static constexpr auto getInventoryCommands = [](const auto& elem) {
		const auto getHatCommand = [&](const auto& hat) {
			return fmt::format("{} {} {}",
			                   GET_COMMAND(sv_inventory_give_hat).getName(),
			                   elem->template get<INVENTORY_ID>(),
			                   Script::escapedString(hat.getName()));
		};

		const auto& [inventory, inventoryId, inventoryAddress] = *elem;
		return fmt::format(
			"{} {} {} {} {}\n"
			"{} {} {}\n"
			"{} {} {}\n"
			"{}\n",
			GET_COMMAND(sv_inventory_add).getName(),
			inventoryId,
			Script::escapedString(std::string{inventoryAddress}),
			Script::escapedString(inventory.username),
			Script::escapedString(std::string_view{reinterpret_cast<const char*>(inventory.tokenHash.data()), inventory.tokenHash.size()}),
			GET_COMMAND(sv_inventory_set_points).getName(),
			inventoryId,
			inventory.points,
			GET_COMMAND(sv_inventory_set_level).getName(),
			inventoryId,
			inventory.level,
			inventory.hats | util::transform(getHatCommand) | util::join('\n'));
	};

	return util::collect<Refs>(m_inventories) | util::sort(compareInventoryRefs) | util::transform(getInventoryCommands) | util::join('\n');
}

auto InventoryServer::getInventoryIds() const -> std::vector<InventoryId> {
	static constexpr auto getInventoryId = [](const auto& elem) {
		return elem.template get<INVENTORY_ID>();
	};
	return m_inventories | util::transform(getInventoryId) | util::collect<std::vector<InventoryId>>();
}

auto InventoryServer::equipInventoryHat(InventoryId id, Hat hat) -> bool {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		if (hat == Hat::none() || util::contains((*it)->hats, hat)) {
			this->equipHat(id, hat);
			return true;
		}
	}
	return false;
}

auto InventoryServer::unequipInventoryHat(InventoryId id, Hat hat) -> bool {
	if (!m_inventories.contains<INVENTORY_ID>(id) || this->getEquippedHat(id) != hat) {
		return false;
	}

	this->unequipHat(id, hat);
	return true;
}

auto InventoryServer::getEquippedInventoryHat(InventoryId id) const -> Hat {
	return this->getEquippedHat(id);
}

auto InventoryServer::giveInventoryHat(InventoryId id, Hat hat) -> bool {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		if (const auto hatIt = std::lower_bound((*it)->hats.begin(),
		                                        (*it)->hats.end(),
		                                        hat,
		                                        [](const auto& lhs, const auto& rhs) { return lhs.getName() < rhs.getName(); });
		    hatIt == (*it)->hats.end() || *hatIt != hat) {
			(*it)->hats.insert(hatIt, hat);
		}
		return true;
	}
	return false;
}

auto InventoryServer::removeInventoryHat(InventoryId id, Hat hat) -> bool {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		this->unequipHat(id, hat);
		util::erase((*it)->hats, hat);
		return true;
	}
	return false;
}

auto InventoryServer::getInventoryHats(InventoryId id) const -> const std::vector<Hat>* {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		return &((*it)->hats);
	}
	return nullptr;
}

auto InventoryServer::inventoryPoints(InventoryId id) -> Score* {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		return &((*it)->points);
	}
	return nullptr;
}

auto InventoryServer::inventoryPoints(InventoryId id) const -> const Score* {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		return &((*it)->points);
	}
	return nullptr;
}

auto InventoryServer::inventoryLevel(InventoryId id) -> Score* {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		return &((*it)->level);
	}
	return nullptr;
}

auto InventoryServer::inventoryLevel(InventoryId id) const -> const Score* {
	if (const auto it = m_inventories.find<INVENTORY_ID>(id); it != m_inventories.end()) {
		return &((*it)->level);
	}
	return nullptr;
}

auto InventoryServer::handleMessage(msg::sv::in::InventoryEquipHatRequest&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	if (const auto id = this->getCurrentClientInventoryId(); id != INVENTORY_ID_INVALID) {
		if (!this->equipInventoryHat(id, msg.hat)) {
			this->reply(msg::cl::out::InventoryEquipHat{{}, this->getEquippedInventoryHat(id)});
		}
	} else {
		this->reply(msg::cl::out::InventoryEquipHat{{}, Hat::none()});
	}
}
