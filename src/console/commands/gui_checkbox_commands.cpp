#include "gui_checkbox_commands.hpp"

// clang-format off
ConVarChar gui_checkbox_left_char{				"gui_checkbox_edge_char",			'[',	ConVar::CLIENT_VARIABLE, "How to draw the left edges of checkboxes."};
ConVarChar gui_checkbox_right_char{				"gui_checkbox_right_char",			']',	ConVar::CLIENT_VARIABLE, "How to draw the right edges of checkboxes."};
ConVarChar gui_checkbox_hover_enabled_char{		"gui_checkbox_hover_enabled_char",	'0',	ConVar::CLIENT_VARIABLE, "How to draw the inside of checkboxes when enabled and hovered over."};
ConVarChar gui_checkbox_hover_disabled_char{	"gui_checkbox_hover_disabled_char",	'-',	ConVar::CLIENT_VARIABLE, "How to draw the inside of checkboxes when disabled and hovered over."};
ConVarChar gui_checkbox_press_char{				"gui_checkbox_press_char",			'x',	ConVar::CLIENT_VARIABLE, "How to draw the inside of checkboxes when pressed."};
ConVarChar gui_checkbox_enabled_char{			"gui_checkbox_enabled_char",		'O',	ConVar::CLIENT_VARIABLE, "How to draw the inside of checkboxes when enabled."};
ConVarChar gui_checkbox_disabled_char{			"gui_checkbox_disabled_char",		' ',	ConVar::CLIENT_VARIABLE, "How to draw the inside of checkboxes when disabled."};
// clang-format on
