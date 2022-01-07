#ifndef AF2_CONSOLE_COMMANDS_META_CLIENT_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_META_CLIENT_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarString meta_address;
extern ConVarIntMinMax meta_port;
extern ConVarIntMinMax meta_cl_port;
extern ConVarFloatMinMax meta_cl_timeout;
extern ConVarIntMinMax meta_cl_throttle_limit;
extern ConVarIntMinMax meta_cl_throttle_max_period;
extern ConVarIntMinMax meta_cl_max_server_connections;
extern ConVarIntMinMax meta_cl_sendrate;

CON_COMMAND_EXTERN(meta_is_connecting);
CON_COMMAND_EXTERN(meta_refresh);
CON_COMMAND_EXTERN(meta_has_received_ip_list);
CON_COMMAND_EXTERN(meta_ip_count);
CON_COMMAND_EXTERN(meta_ip_list);
CON_COMMAND_EXTERN(meta_info_count);
CON_COMMAND_EXTERN(meta_info);

#endif
