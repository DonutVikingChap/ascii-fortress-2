#ifndef AF2_SERVER_INVENTORY_SERVER_HPP
#define AF2_SERVER_INVENTORY_SERVER_HPP

#include "../../network/crypto.hpp"           // crypto::...
#include "../../network/endpoint.hpp"         // net::IpAddress
#include "../../utilities/multi_hash.hpp"     // util::MultiHash
#include "../data/hat.hpp"                    // Hat
#include "../data/inventory.hpp"              // InventoryId, InventoryToken, INVENTORY_ID_INVALID
#include "../data/score.hpp"                  // Score
#include "../shared/game_client_messages.hpp" // msg::cl::out::Inventory...
#include "../shared/game_server_messages.hpp" // msg::sv::in::Inventory...

#include <cstddef>  // std::size_t, std::byte
#include <optional> // std::optional
#include <string>   // std::string
#include <utility>  // std::move, std::pair
#include <vector>   // std::vector

class InventoryServer {
public:
	InventoryServer() = default;

	InventoryServer(const InventoryServer&) = default;
	InventoryServer(InventoryServer&&) noexcept = default;

	virtual ~InventoryServer() = default;

	auto operator=(const InventoryServer&) -> InventoryServer& = default;
	auto operator=(InventoryServer&&) noexcept -> InventoryServer& = default;

	[[nodiscard]] auto initInventoryServer() noexcept -> bool;

	[[nodiscard]] auto createInventory(net::IpAddress address, std::string username) -> std::pair<InventoryId, InventoryToken>;

	[[nodiscard]] auto accessInventory(InventoryId id, const InventoryToken& token, net::IpAddress address, std::string username) -> bool;

	[[nodiscard]] auto addInventory(InventoryId id, net::IpAddress address, std::string username, const crypto::FastHash& tokenHash) -> bool;
	[[nodiscard]] auto hasInventory(InventoryId id) const -> bool;
	[[nodiscard]] auto removeInventory(InventoryId id) -> bool;

	[[nodiscard]] auto getInventoryList() const -> std::string;
	[[nodiscard]] auto getInventoryConfig() const -> std::string;

	[[nodiscard]] auto getInventoryIds() const -> std::vector<InventoryId>;

	[[nodiscard]] auto equipInventoryHat(InventoryId id, Hat hat) -> bool;
	[[nodiscard]] auto unequipInventoryHat(InventoryId id, Hat hat) -> bool;

	[[nodiscard]] auto getEquippedInventoryHat(InventoryId id) const -> Hat;

	[[nodiscard]] auto giveInventoryHat(InventoryId id, Hat hat) -> bool;
	[[nodiscard]] auto removeInventoryHat(InventoryId id, Hat hat) -> bool;

	[[nodiscard]] auto getInventoryHats(InventoryId id) const -> const std::vector<Hat>*;

	[[nodiscard]] auto inventoryPoints(InventoryId id) -> Score*;
	[[nodiscard]] auto inventoryPoints(InventoryId id) const -> const Score*;

	[[nodiscard]] auto inventoryLevel(InventoryId id) -> Score*;
	[[nodiscard]] auto inventoryLevel(InventoryId id) const -> const Score*;

protected:
	auto handleMessage(msg::sv::in::InventoryEquipHatRequest&& msg) -> void;

	[[nodiscard]] virtual auto testSpam() -> bool = 0;

	virtual auto equipHat(InventoryId id, Hat hat) -> void = 0;
	virtual auto unequipHat(InventoryId id, Hat hat) -> void = 0;

	[[nodiscard]] virtual auto getEquippedHat(InventoryId id) const -> Hat = 0;

	virtual auto reply(msg::cl::out::InventoryEquipHat&& msg) -> void = 0;

	[[nodiscard]] virtual auto getCurrentClientInventoryId() const -> InventoryId = 0;

private:
	struct Inventory final {
		Inventory(std::string username, const crypto::FastHash& tokenHash)
			: username(std::move(username))
			, tokenHash(tokenHash) {}

		std::string username;
		crypto::FastHash tokenHash;
		std::vector<Hat> hats{};
		Score points = 0;
		Score level = 0;
	};

	using Inventories = util::MultiHash<Inventory,     // inventory
	                                    InventoryId,   // inventoryId
	                                    net::IpAddress // inventoryAddress
	                                    >;

	static constexpr auto INVENTORY_INVENTORY = std::size_t{0};
	static constexpr auto INVENTORY_ID = std::size_t{INVENTORY_INVENTORY + 1};
	static constexpr auto INVENTORY_ADDRESS = std::size_t{INVENTORY_ID + 1};

	Inventories m_inventories{};
	InventoryId m_latestId = INVENTORY_ID_INVALID;
};

#endif
