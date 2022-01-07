#ifndef AF2_DATA_INVENTORY_HPP
#define AF2_DATA_INVENTORY_HPP

#include "../../network/crypto.hpp" // crypto::AccessToken

#include <cstdint> // std::uint32_t

using InventoryId = std::uint32_t;
using InventoryToken = crypto::AccessToken;
using InventoryTokenRef = crypto::AccessTokenRef;
using InventoryTokenView = crypto::AccessTokenView;

constexpr auto INVENTORY_ID_INVALID = InventoryId{0};

#endif
