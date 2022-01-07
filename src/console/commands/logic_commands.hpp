#ifndef AF2_CONSOLE_COMMANDS_LOGIC_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_LOGIC_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar...

extern ConVarBool cvar_true;
extern ConVarBool cvar_false;

CON_COMMAND_EXTERN(not );
CON_COMMAND_EXTERN(and);
CON_COMMAND_EXTERN(or);
CON_COMMAND_EXTERN(xor);

#endif
