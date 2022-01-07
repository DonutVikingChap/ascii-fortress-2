#ifndef AF2_CONSOLE_COMMANDS_META_SERVER_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_META_SERVER_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN, GET_COMMAND
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarIntMinMax meta_sv_port;
extern ConVarString meta_sv_config_file;
extern ConVarString meta_sv_autoexec_file;
extern ConVarFloatMinMax meta_sv_timeout;
extern ConVarIntMinMax meta_sv_throttle_limit;
extern ConVarIntMinMax meta_sv_throttle_max_period;
extern ConVarFloatMinMax meta_sv_disconnect_cooldown;
extern ConVarFloatMinMax meta_sv_heartbeat_interval;
extern ConVarFloatMinMax meta_sv_afk_autokick_time;
extern ConVarIntMinMax meta_sv_max_ticks_per_frame;
extern ConVarIntMinMax meta_sv_spam_limit;
extern ConVarIntMinMax meta_sv_tickrate;
extern ConVarIntMinMax meta_sv_max_clients;
extern ConVarIntMinMax meta_sv_max_connecting_clients;
extern ConVarIntMinMax meta_sv_config_auto_save_interval;
extern ConVarIntMinMax meta_sv_max_connections_per_ip;
extern ConVarString meta_sv_private_address_override;

CON_COMMAND_EXTERN(meta_sv_kick);

CON_COMMAND_EXTERN(meta_sv_ban);
CON_COMMAND_EXTERN(meta_sv_unban);

CON_COMMAND_EXTERN(meta_sv_ban_list);

CON_COMMAND_EXTERN(meta_sv_writeconfig);

#endif
