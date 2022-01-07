#include "gui_dropdown_commands.hpp"

// clang-format off
ConVarChar gui_dropdown_left_char{			"gui_dropdown_edge_char",			'[',	ConVar::CLIENT_VARIABLE, "How to draw the left edges of dropdowns."};
ConVarChar gui_dropdown_right_char{			"gui_dropdown_right_char",			']',	ConVar::CLIENT_VARIABLE, "How to draw the right edges of dropdowns."};
ConVarChar gui_dropdown_background_char{	"gui_dropdown_background_char",		' ',	ConVar::CLIENT_VARIABLE, "How to draw the inside of dropdowns."};
ConVarChar gui_dropdown_hover_char{			"gui_dropdown_hover_char",			'-',	ConVar::CLIENT_VARIABLE, "How to draw the inside of dropdowns when hovered over."};
ConVarChar gui_dropdown_press_char{			"gui_dropdown_press_char",			'=',	ConVar::CLIENT_VARIABLE, "How to draw the inside of dropdowns when pressed."};
ConVarChar gui_dropdown_selected_char{		"gui_dropdown_selected_char",		'>',	ConVar::CLIENT_VARIABLE, "How to draw the character before the selected item in a dropdown."};
ConVarChar gui_dropdown_separator_char{		"gui_dropdown_separator_char",		'|',	ConVar::CLIENT_VARIABLE, "How to draw the separator before the arrow in a dropwdown."};
ConVarChar gui_dropdown_arrow_up_char{		"gui_dropdown_arrow_up_char",		'^',	ConVar::CLIENT_VARIABLE, "How to draw the up arrow in a dropwdown."};
ConVarChar gui_dropdown_arrow_down_char{	"gui_dropdown_arrow_down_char",		'v',	ConVar::CLIENT_VARIABLE, "How to draw the down arrow in a dropwdown."};
// clang-format on
