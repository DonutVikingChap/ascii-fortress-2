#ifndef AF2_CONSOLE_COMMANDS_GUI_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_GUI_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN

CON_COMMAND_EXTERN(colorlist);

CON_COMMAND_EXTERN(gui_size_x);
CON_COMMAND_EXTERN(gui_size_y);

CON_COMMAND_EXTERN(gui_is_clear);
CON_COMMAND_EXTERN(gui_clear);

CON_COMMAND_EXTERN(gui_is_activated);
CON_COMMAND_EXTERN(gui_activate);
CON_COMMAND_EXTERN(gui_deactivate);

CON_COMMAND_EXTERN(gui_has_element);

CON_COMMAND_EXTERN(gui_remove);

CON_COMMAND_EXTERN(gui_set_text);
CON_COMMAND_EXTERN(gui_get_text);

CON_COMMAND_EXTERN(gui_set_color);
CON_COMMAND_EXTERN(gui_get_color);

CON_COMMAND_EXTERN(gui_set_value);
CON_COMMAND_EXTERN(gui_get_value);

CON_COMMAND_EXTERN(gui_screen_get);
CON_COMMAND_EXTERN(gui_screen_set);

CON_COMMAND_EXTERN(gui_button);
CON_COMMAND_EXTERN(gui_input);
CON_COMMAND_EXTERN(gui_slider);
CON_COMMAND_EXTERN(gui_checkbox);
CON_COMMAND_EXTERN(gui_dropdown);
CON_COMMAND_EXTERN(gui_screen);
CON_COMMAND_EXTERN(gui_screen_matrix);
CON_COMMAND_EXTERN(gui_screen_file);
CON_COMMAND_EXTERN(gui_text);

CON_COMMAND_EXTERN(gui_push_menu);
CON_COMMAND_EXTERN(gui_pop_menu);

CON_COMMAND_EXTERN(gui_dump);

#endif
