#ifndef AF2_CONSOLE_COMMANDS_FILE_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_FILE_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar...

extern ConVarString data_dir;
extern ConVarString data_subdir_cfg;
extern ConVarString data_subdir_logs;
extern ConVarString data_subdir_maps;
extern ConVarString data_subdir_fonts;
extern ConVarString data_subdir_images;
extern ConVarString data_subdir_sounds;
extern ConVarString data_subdir_shaders;
extern ConVarString data_subdir_screens;
extern ConVarString data_subdir_screenshots;
extern ConVarString data_subdir_downloads;

CON_COMMAND_EXTERN(file_read);
CON_COMMAND_EXTERN(file_append);
CON_COMMAND_EXTERN(file_dump);
CON_COMMAND_EXTERN(file_exists);
CON_COMMAND_EXTERN(file_create_path);

#endif
