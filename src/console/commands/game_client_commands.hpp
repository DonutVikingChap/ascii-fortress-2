#ifndef AF2_CONSOLE_COMMANDS_GAME_CLIENT_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_GAME_CLIENT_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarIntMinMax cl_updaterate;
extern ConVarIntMinMax cl_cmdrate;
extern ConVarBool cl_hitsound_enable;
extern ConVarBool cl_mouselook;
extern ConVarBool cl_draw_playernames_friendly;
extern ConVarBool cl_draw_playernames_enemy;
extern ConVarBool cl_draw_playernames_spectator;
extern ConVarBool cl_chat_enable;
extern ConVarBool cl_showscores;
extern ConVarBool cl_showping;
extern ConVarFloatMinMax cl_timeout;
extern ConVarIntMinMax cl_throttle_limit;
extern ConVarIntMinMax cl_throttle_max_period;
extern ConVarBool cl_allow_resource_download;
extern ConVarIntMinMax cl_max_resource_download_size;
extern ConVarIntMinMax cl_max_resource_total_download_size;
extern ConVarString address;
extern ConVarIntMinMax port;
extern ConVarIntMinMax cl_port;
extern ConVarString username;
extern ConVarString password;
extern ConVarString cl_config_file;
extern ConVarString cl_autoexec_file;
extern ConVarBool cl_crosshair_enable;
extern ConVarChar cl_crosshair;
extern ConVarColor cl_crosshair_color;
extern ConVarBool cl_crosshair_use_team_color;
extern ConVarBool cl_crosshair_distance_follow_cursor;
extern ConVarFloatMinMax cl_crosshair_min_distance;
extern ConVarFloatMinMax cl_crosshair_max_distance;
extern ConVarBool cl_crosshair_collide_world;
extern ConVarBool cl_crosshair_collide_viewport;
extern ConVarIntMinMax cl_crosshair_viewport_border;

extern ConVarString cl_chars_explosion;
extern ConVarString cl_gun_sentry;

extern ConVarChar cl_char_player;
extern ConVarChar cl_char_corpse;
extern ConVarChar cl_char_sentry;
extern ConVarChar cl_char_medkit;
extern ConVarChar cl_char_ammopack;
extern ConVarChar cl_char_flag;
extern ConVarChar cl_char_respawnvis;
extern ConVarChar cl_char_resupply;
extern ConVarChar cl_char_cart;
extern ConVarChar cl_char_track;

extern ConVarColor cl_color_world;
extern ConVarColor cl_color_non_solid;
extern ConVarColor cl_color_respawnvis;
extern ConVarColor cl_color_resupply;
extern ConVarColor cl_color_track;
extern ConVarColor cl_color_name;
extern ConVarColor cl_color_medkit;
extern ConVarColor cl_color_ammopack;
extern ConVarColor cl_color_timer;
extern ConVarColor cl_color_health;
extern ConVarColor cl_color_low_health;
extern ConVarColor cl_color_ammo;

CON_COMMAND_EXTERN(cl_player_id);

CON_COMMAND_EXTERN(team_menu);
CON_COMMAND_EXTERN(class_menu);

CON_COMMAND_EXTERN(team);
CON_COMMAND_EXTERN(class);

CON_COMMAND_EXTERN(fwd);

CON_COMMAND_EXTERN(is_connected);

CON_COMMAND_EXTERN(cl_writeconfig);

#endif
