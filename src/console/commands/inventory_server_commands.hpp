#ifndef AF2_CONSOLE_COMMANDS_INVENTORY_SERVER_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_INVENTORY_SERVER_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN

CON_COMMAND_EXTERN(sv_inventory_add);
CON_COMMAND_EXTERN(sv_inventory_remove);

CON_COMMAND_EXTERN(sv_inventory_list);

CON_COMMAND_EXTERN(sv_inventory_exists);

CON_COMMAND_EXTERN(sv_inventory_get_points);
CON_COMMAND_EXTERN(sv_inventory_set_points);
CON_COMMAND_EXTERN(sv_inventory_add_points);

CON_COMMAND_EXTERN(sv_inventory_get_level);
CON_COMMAND_EXTERN(sv_inventory_set_level);
CON_COMMAND_EXTERN(sv_inventory_add_level);

CON_COMMAND_EXTERN(sv_inventory_get_hats);

CON_COMMAND_EXTERN(sv_inventory_equip_hat);
CON_COMMAND_EXTERN(sv_inventory_unequip_hat);

CON_COMMAND_EXTERN(sv_inventory_get_equipped_hat);

CON_COMMAND_EXTERN(sv_inventory_has_hat);

CON_COMMAND_EXTERN(sv_inventory_give_hat);
CON_COMMAND_EXTERN(sv_inventory_remove_hat);

#endif
