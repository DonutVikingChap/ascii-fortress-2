#ifndef AF2_CONSOLE_COMMANDS_WORLD_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_WORLD_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN
#include "../convar.hpp"      // ConVar...

extern ConVarFloatMinMax mp_player_respawn_time;
extern ConVarFloatMinMax mp_round_end_time;
extern ConVarFloatMinMax mp_flag_return_time;
extern ConVarFloatMinMax mp_medkit_respawn_time;
extern ConVarFloatMinMax mp_ammopack_respawn_time;
extern ConVarIntMinMax mp_ctf_capture_limit;
extern ConVarIntMinMax mp_limitteams;
extern ConVarFloatMinMax mp_explosion_disappear_time;
extern ConVarFloatMinMax mp_sentry_despawn_time;
extern ConVarIntMinMax mp_sentry_health;
extern ConVarIntMinMax mp_sentry_range;
extern ConVarFloatMinMax mp_sentry_build_time;
extern ConVarFloatMinMax mp_roundtime_ctf;
extern ConVarFloatMinMax mp_roundtime_payload;
extern ConVarFloatMinMax mp_roundtime_tdm;
extern ConVarFloatMinMax mp_payload_cart_push_time;
extern ConVarFloatMinMax mp_payload_defense_respawn_time_coefficient;
extern ConVarFloatMinMax mp_payload_defense_respawn_time_threshold;
extern ConVarFloatMinMax mp_spy_kill_disguise_cooldown;
extern ConVarIntMinMax mp_sniper_rifle_range;
extern ConVarFloatMinMax mp_blast_jump_move_interval;
extern ConVarFloatMinMax mp_blast_jump_duration;
extern ConVarFloatMinMax mp_blast_jump_chain_duration;
extern ConVarFloat mp_blast_jump_chain_move_interval_coefficient;
extern ConVarFloat mp_self_damage_coefficient;
extern ConVarInt mp_score_objective;
extern ConVarInt mp_score_win;
extern ConVarInt mp_score_lose;
extern ConVarInt mp_score_kill;
extern ConVarInt mp_score_kill_sentry;
extern ConVarInt mp_score_heal;
extern ConVarBool mp_shotgun_use_legacy_spread;
extern ConVarBool mp_enable_round_time;
extern ConVarBool mp_switch_teams_between_rounds;
extern ConVarIntMinMax sv_max_shots_per_frame;
extern ConVarIntMinMax sv_max_move_steps_per_frame;
extern ConVarIntMinMax mp_winlimit;
extern ConVarIntMinMax mp_roundlimit;
extern ConVarFloatMinMax mp_timelimit;

CON_COMMAND_EXTERN(mp_get_team_id_by_name);
CON_COMMAND_EXTERN(mp_get_class_id_by_name);
CON_COMMAND_EXTERN(mp_get_projectile_type_id_by_name);
CON_COMMAND_EXTERN(mp_get_weapon_id_by_name);

CON_COMMAND_EXTERN(mp_get_team_name);
CON_COMMAND_EXTERN(mp_get_class_name);
CON_COMMAND_EXTERN(mp_get_projectile_type_name);
CON_COMMAND_EXTERN(mp_get_weapon_name);

CON_COMMAND_EXTERN(mp_get_player_id_by_ip);
CON_COMMAND_EXTERN(mp_get_player_ip);
CON_COMMAND_EXTERN(mp_get_player_inventory_id);

CON_COMMAND_EXTERN(mp_award_player_points);

CON_COMMAND_EXTERN(mp_is_player_bot);

CON_COMMAND_EXTERN(mp_play_world_sound);
CON_COMMAND_EXTERN(mp_play_world_sound_from_player);
CON_COMMAND_EXTERN(mp_play_team_sound);
CON_COMMAND_EXTERN(mp_play_team_sound_separate);
CON_COMMAND_EXTERN(mp_play_game_sound);

CON_COMMAND_EXTERN(mp_write_event);
CON_COMMAND_EXTERN(mp_write_event_team);
CON_COMMAND_EXTERN(mp_write_event_player);

CON_COMMAND_EXTERN(mp_write_chat);
CON_COMMAND_EXTERN(mp_write_chat_team);
CON_COMMAND_EXTERN(mp_write_chat_player);

CON_COMMAND_EXTERN(mp_end_round);
CON_COMMAND_EXTERN(mp_reset_round);

CON_COMMAND_EXTERN(mp_reset_map);

CON_COMMAND_EXTERN(mp_get_team_wins);
CON_COMMAND_EXTERN(mp_time_played);
CON_COMMAND_EXTERN(mp_rounds_played);

#endif
