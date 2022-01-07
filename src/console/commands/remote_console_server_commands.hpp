#ifndef AF2_CONSOLE_COMMANDS_REMOTE_CONSOLE_SERVER_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_REMOTE_CONSOLE_SERVER_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarBool sv_rcon_enable;
extern ConVarFloatMinMax sv_rcon_session_timeout;

CON_COMMAND_EXTERN(sv_rcon_add_user);
CON_COMMAND_EXTERN(sv_rcon_add_user_hashed);
CON_COMMAND_EXTERN(sv_rcon_remove_user);

CON_COMMAND_EXTERN(sv_rcon_userlist);

CON_COMMAND_EXTERN(sv_rcon_has_user);
CON_COMMAND_EXTERN(sv_rcon_logged_in);
CON_COMMAND_EXTERN(sv_rcon_running);

CON_COMMAND_EXTERN(sv_rcon_end);
CON_COMMAND_EXTERN(sv_rcon_kill);

#endif
