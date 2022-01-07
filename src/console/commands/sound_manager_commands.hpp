#ifndef AF2_CONSOLE_COMMANDS_SOUND_MANAGER_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_SOUND_MANAGER_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar..., CONVAR_CALLBACK

extern ConVarFloatMinMax snd_attenuation;
extern ConVarFloatMinMax snd_rolloff;
extern ConVarFloat snd_distance;
extern ConVarIntMinMax snd_max_simultaneous;
extern ConVarFloatMinMax volume;

CON_COMMAND_EXTERN(play_music);
CON_COMMAND_EXTERN(loop_music);
CON_COMMAND_EXTERN(music_playing);
CON_COMMAND_EXTERN(stop_music);

#endif
