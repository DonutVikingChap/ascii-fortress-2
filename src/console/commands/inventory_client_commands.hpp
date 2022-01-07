#ifndef AF2_CONSOLE_COMMANDS_INVENTORY_CLIENT_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_INVENTORY_CLIENT_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarString cvar_hat;

CON_COMMAND_EXTERN(cl_inventory_add);
CON_COMMAND_EXTERN(cl_inventory_remove);

CON_COMMAND_EXTERN(cl_inventory_list);

CON_COMMAND_EXTERN(cl_inventory_exists);

#endif
