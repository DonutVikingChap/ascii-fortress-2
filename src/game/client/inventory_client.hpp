#ifndef AF2_CLIENT_INVENTORY_CLIENT_HPP
#define AF2_CLIENT_INVENTORY_CLIENT_HPP

#include "../../network/endpoint.hpp"         // net::IpEndpoint
#include "../data/hat.hpp"                    // Hat
#include "../data/inventory.hpp"              // InventoryId, InventoryToken, INVENTORY_ID_INVALID
#include "../data/score.hpp"                  // Score
#include "../shared/game_client_messages.hpp" // msg::cl::in::Inventory...
#include "../shared/game_server_messages.hpp" // msg::sv::out::Inventory...

#include <optional>      // std::optional
#include <string>        // std::string
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

class InventoryClient {
public:
	struct Inventory final {
		constexpr Inventory() noexcept = default;
		constexpr Inventory(InventoryId id, const InventoryToken& token) noexcept
			: id(id)
			, token(token) {}

		InventoryId id = INVENTORY_ID_INVALID;
		InventoryToken token{};
	};

	InventoryClient() = default;

	virtual ~InventoryClient();

	InventoryClient(const InventoryClient&) = delete;
	InventoryClient(InventoryClient&&) = delete;

	auto operator=(const InventoryClient&) -> InventoryClient& = delete;
	auto operator=(InventoryClient &&) -> InventoryClient& = delete;

	[[nodiscard]] auto initInventoryClient() noexcept -> bool;

	[[nodiscard]] auto addInventory(net::IpEndpoint serverEndpoint, InventoryId inventoryId, const InventoryToken& inventoryToken) -> bool;

	[[nodiscard]] auto getInventory(net::IpEndpoint serverEndpoint) -> Inventory&;
	[[nodiscard]] auto hasInventory(net::IpEndpoint serverEndpoint) const -> bool;

	[[nodiscard]] auto removeInventory(net::IpEndpoint serverEndpoint) -> bool;

	[[nodiscard]] auto getInventoryList() const -> std::string;
	[[nodiscard]] auto getInventoryConfig() const -> std::string;

	[[nodiscard]] auto getInventoryIps() const -> std::vector<net::IpEndpoint>;

	[[nodiscard]] auto writeInventoryEquipHatRequest(Hat hat) -> bool;

protected:
	auto handleMessage(msg::cl::in::InventoryEquipHat&& msg) -> void;

	virtual auto write(msg::sv::out::InventoryEquipHatRequest&& msg) -> bool = 0;

private:
	using Inventories = std::unordered_map<net::IpEndpoint, Inventory>;

	Inventories m_inventories{};
};

#endif
