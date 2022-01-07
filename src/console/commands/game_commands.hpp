#ifndef AF2_CONSOLE_COMMANDS_GAME_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_GAME_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN, GET_COMMAND
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarString cvar_main;
extern ConVarString cvar_game;
extern ConVarString game_version;
extern ConVarString game_name;
extern ConVarString game_author;
extern ConVarString game_year;
extern ConVarString game_url;
extern ConVarBool headless;
extern ConVarString r_shader_vert;
extern ConVarString r_shader_frag;
extern ConVarString r_font;
extern ConVarString r_icon;
extern ConVarIntMinMax r_fullscreen_mode;
extern ConVarIntMinMax r_width_windowed;
extern ConVarIntMinMax r_height_windowed;
extern ConVarIntMinMax r_width;
extern ConVarIntMinMax r_height;
extern ConVarString r_window_title;
extern ConVarBool r_vsync;
extern ConVarColor r_background_color;
extern ConVarFloatMinMax r_ratio;
extern ConVarBool r_font_match_size;
extern ConVarFloatMinMax r_font_match_size_coefficient;
extern ConVarIntMinMax r_font_size;
extern ConVarInt r_glyph_offset_x;
extern ConVarInt r_glyph_offset_y;
extern ConVarInt r_debug_text_offset_x;
extern ConVarInt r_debug_text_offset_y;
extern ConVarFloatMinMax r_debug_text_scale_x;
extern ConVarFloatMinMax r_debug_text_scale_y;
extern ConVarColor r_debug_text_color;
extern ConVarBool r_showfps;
extern ConVarFloatMinMax fps_max;
extern ConVarIntMinMax console_max_rows;
extern ConVarFloatMinMax host_timescale;
extern ConVarString host_config_file;
extern ConVarString host_autoexec_file;
extern ConVarBool host_server_admin;

CON_COMMAND_EXTERN(r_size);
CON_COMMAND_EXTERN(r_desktop_width);
CON_COMMAND_EXTERN(r_desktop_height);
CON_COMMAND_EXTERN(r_desktop_size);
CON_COMMAND_EXTERN(r_fullscreen_list);

CON_COMMAND_EXTERN(say);
CON_COMMAND_EXTERN(say_team);
CON_COMMAND_EXTERN(say_server);

CON_COMMAND_EXTERN(clear_console);
CON_COMMAND_EXTERN(open_console);
CON_COMMAND_EXTERN(open_chat);
CON_COMMAND_EXTERN(open_teamchat);
CON_COMMAND_EXTERN(open_textinput);
CON_COMMAND_EXTERN(open_password);

CON_COMMAND_EXTERN(maplist);

CON_COMMAND_EXTERN(status);

CON_COMMAND_EXTERN(print);
CON_COMMAND_EXTERN(print_colored);
CON_COMMAND_EXTERN(println);
CON_COMMAND_EXTERN(println_colored);

CON_COMMAND_EXTERN(is_running_client);
CON_COMMAND_EXTERN(is_running_server);
CON_COMMAND_EXTERN(is_running_meta_client);
CON_COMMAND_EXTERN(is_running_meta_server);
CON_COMMAND_EXTERN(is_running);

CON_COMMAND_EXTERN(disconnect);
CON_COMMAND_EXTERN(quit);

CON_COMMAND_EXTERN(host_publish_game);
CON_COMMAND_EXTERN(host_writeconfig);

CON_COMMAND_EXTERN(map_is_loaded);
CON_COMMAND_EXTERN(map_get_char);
CON_COMMAND_EXTERN(map_is_solid);
CON_COMMAND_EXTERN(map_find_path);
CON_COMMAND_EXTERN(map_width);
CON_COMMAND_EXTERN(map_height);

CON_COMMAND_EXTERN(screenshot);

#endif
