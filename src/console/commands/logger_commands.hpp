#ifndef AF2_CONSOLE_COMMANDS_LOGGER_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_LOGGER_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar...

extern ConVarBool log_flush;
extern ConVarIntMinMax log_file_limit;
extern ConVarIntMinMax log_max_size;
extern ConVarBool log_show_error_message;
extern ConVarBool log_debug_output;
extern ConVarBool log_debug_break_on_error;

CON_COMMAND_EXTERN(log_open);
CON_COMMAND_EXTERN(log_close);

CON_COMMAND_EXTERN(log_info);
CON_COMMAND_EXTERN(log_warning);
CON_COMMAND_EXTERN(log_error);
CON_COMMAND_EXTERN(log_fatal);

#endif
