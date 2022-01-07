#ifndef AF2_CONSOLE_COMMANDS_GAME_SERVER_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_GAME_SERVER_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN, GET_COMMAND
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarBool sv_cheats;
extern ConVarIntMinMax sv_spam_limit;
extern ConVarIntMinMax sv_tickrate;
extern ConVarIntMinMax sv_bot_tickrate;
extern ConVarBool sv_bot_ai_enable;
extern ConVarBool sv_bot_ai_require_players;
extern ConVarIntMinMax sv_max_ticks_per_frame;
extern ConVarIntMinMax sv_playerlimit;
extern ConVarIntMinMax sv_max_username_length;
extern ConVarFloatMinMax sv_disconnect_cooldown;
extern ConVarFloatMinMax sv_timeout;
extern ConVarIntMinMax sv_throttle_limit;
extern ConVarIntMinMax sv_throttle_max_period;
extern ConVarString sv_hostname;
extern ConVarBool sv_allow_resource_download;
extern ConVarFloatMinMax sv_resource_upload_rate;
extern ConVarIntMinMax sv_resource_upload_chunk_size;
extern ConVarHashed sv_password;
extern ConVarBool sv_rtv_enable;
extern ConVarFloatMinMax sv_rtv_delay;
extern ConVarFloatMinMax sv_rtv_needed;
extern ConVarString sv_nextlevel;
extern ConVarString sv_map;
extern ConVarIntMinMax sv_bot_count;
extern ConVarIntMinMax sv_port;
extern ConVarString sv_config_file;
extern ConVarString sv_autoexec_file;
extern ConVarString sv_map_rotation;
extern ConVarString sv_motd;
extern ConVarIntMinMax sv_max_clients;
extern ConVarIntMinMax sv_max_connecting_clients;
extern ConVarIntMinMax sv_config_auto_save_interval;
extern ConVarIntMinMax sv_score_level_interval;
extern ConVarFloatMinMax sv_afk_autokick_time;
extern ConVarIntMinMax sv_max_connections_per_ip;
extern ConVarIntMinMax sv_max_players_per_ip;
extern ConVarBool sv_meta_submit;
extern ConVarBool sv_meta_submit_retry;
extern ConVarFloatMinMax sv_meta_submit_retry_interval;

CON_COMMAND_EXTERN(changelevel);

CON_COMMAND_EXTERN(bot_add);
CON_COMMAND_EXTERN(bot_kick);

CON_COMMAND_EXTERN(sv_rtv);

CON_COMMAND_EXTERN(sv_write_output);
CON_COMMAND_EXTERN(sv_write_error);

CON_COMMAND_EXTERN(sv_kick);

CON_COMMAND_EXTERN(sv_ban);
CON_COMMAND_EXTERN(sv_unban);

CON_COMMAND_EXTERN(sv_ban_list);

CON_COMMAND_EXTERN(sv_writeconfig);

#endif
