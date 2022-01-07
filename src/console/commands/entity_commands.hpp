#ifndef AF2_CONSOLE_COMMANDS_ENTITY_COMMANDS_HPP
#define AF2_CONSOLE_COMMANDS_ENTITY_COMMANDS_HPP

#include "../con_command.hpp" // ConCommand, CON_COMMAND, CON_COMMAND_EXTERN

CON_COMMAND_EXTERN(mp_create_projectile);
CON_COMMAND_EXTERN(mp_create_explosion);
CON_COMMAND_EXTERN(mp_create_sentry);
CON_COMMAND_EXTERN(mp_create_medkit);
CON_COMMAND_EXTERN(mp_create_ammopack);
CON_COMMAND_EXTERN(mp_create_ent);
CON_COMMAND_EXTERN(mp_create_flag);

CON_COMMAND_EXTERN(mp_spawn_player);
CON_COMMAND_EXTERN(mp_spawn_medkit);
CON_COMMAND_EXTERN(mp_spawn_ammopack);

CON_COMMAND_EXTERN(mp_hurt_player);
CON_COMMAND_EXTERN(mp_hurt_sentry);

CON_COMMAND_EXTERN(mp_kill_player);
CON_COMMAND_EXTERN(mp_kill_sentry);
CON_COMMAND_EXTERN(mp_kill_medkit);
CON_COMMAND_EXTERN(mp_kill_ammopack);

CON_COMMAND_EXTERN(mp_delete_projectile);
CON_COMMAND_EXTERN(mp_delete_explosion);
CON_COMMAND_EXTERN(mp_delete_sentry);
CON_COMMAND_EXTERN(mp_delete_medkit);
CON_COMMAND_EXTERN(mp_delete_ammopack);
CON_COMMAND_EXTERN(mp_delete_ent);
CON_COMMAND_EXTERN(mp_delete_flag);

CON_COMMAND_EXTERN(mp_has_player);
CON_COMMAND_EXTERN(mp_has_projectile);
CON_COMMAND_EXTERN(mp_has_explosion);
CON_COMMAND_EXTERN(mp_has_sentry);
CON_COMMAND_EXTERN(mp_has_medkit);
CON_COMMAND_EXTERN(mp_has_ammopack);
CON_COMMAND_EXTERN(mp_has_ent);
CON_COMMAND_EXTERN(mp_has_flag);
CON_COMMAND_EXTERN(mp_has_cart);

CON_COMMAND_EXTERN(mp_count_player);
CON_COMMAND_EXTERN(mp_count_projectile);
CON_COMMAND_EXTERN(mp_count_explosion);
CON_COMMAND_EXTERN(mp_count_sentry);
CON_COMMAND_EXTERN(mp_count_medkit);
CON_COMMAND_EXTERN(mp_count_ammopack);
CON_COMMAND_EXTERN(mp_count_ent);
CON_COMMAND_EXTERN(mp_count_flag);
CON_COMMAND_EXTERN(mp_count_cart);

CON_COMMAND_EXTERN(mp_player_list);
CON_COMMAND_EXTERN(mp_projectile_list);
CON_COMMAND_EXTERN(mp_explosion_list);
CON_COMMAND_EXTERN(mp_sentry_list);
CON_COMMAND_EXTERN(mp_medkit_list);
CON_COMMAND_EXTERN(mp_ammopack_list);
CON_COMMAND_EXTERN(mp_ent_list);
CON_COMMAND_EXTERN(mp_flag_list);
CON_COMMAND_EXTERN(mp_cart_list);

CON_COMMAND_EXTERN(mp_teleport_player);
CON_COMMAND_EXTERN(mp_teleport_projectile);
CON_COMMAND_EXTERN(mp_teleport_explosion);
CON_COMMAND_EXTERN(mp_teleport_sentry);
CON_COMMAND_EXTERN(mp_teleport_medkit);
CON_COMMAND_EXTERN(mp_teleport_ammopack);
CON_COMMAND_EXTERN(mp_teleport_ent);
CON_COMMAND_EXTERN(mp_teleport_flag);

CON_COMMAND_EXTERN(mp_get_player_id_by_name);
CON_COMMAND_EXTERN(mp_is_player_carrying_flag);
CON_COMMAND_EXTERN(mp_player_team_select);
CON_COMMAND_EXTERN(mp_resupply_player);

CON_COMMAND_EXTERN(mp_set_round_time);
CON_COMMAND_EXTERN(mp_add_round_time);
CON_COMMAND_EXTERN(mp_get_round_time);

CON_COMMAND_EXTERN(mp_get_player_name);
CON_COMMAND_EXTERN(mp_get_player_x);
CON_COMMAND_EXTERN(mp_get_player_y);
CON_COMMAND_EXTERN(mp_get_player_move_x);
CON_COMMAND_EXTERN(mp_get_player_move_y);
CON_COMMAND_EXTERN(mp_get_player_aim_x);
CON_COMMAND_EXTERN(mp_get_player_aim_y);
CON_COMMAND_EXTERN(mp_get_player_attack1);
CON_COMMAND_EXTERN(mp_get_player_attack2);
CON_COMMAND_EXTERN(mp_get_player_team);
CON_COMMAND_EXTERN(mp_get_player_class);
CON_COMMAND_EXTERN(mp_get_player_alive);
CON_COMMAND_EXTERN(mp_get_player_ping);
CON_COMMAND_EXTERN(mp_get_player_disguised);
CON_COMMAND_EXTERN(mp_get_player_health);
CON_COMMAND_EXTERN(mp_get_player_score);
CON_COMMAND_EXTERN(mp_get_player_noclip);
CON_COMMAND_EXTERN(mp_get_player_primary_ammo);
CON_COMMAND_EXTERN(mp_get_player_secondary_ammo);
CON_COMMAND_EXTERN(mp_get_player_hat);

CON_COMMAND_EXTERN(mp_set_player_ping);
CON_COMMAND_EXTERN(mp_set_player_disguised);
CON_COMMAND_EXTERN(mp_set_player_score);
CON_COMMAND_EXTERN(mp_set_player_attack1);
CON_COMMAND_EXTERN(mp_set_player_attack2);
CON_COMMAND_EXTERN(mp_set_player_move);
CON_COMMAND_EXTERN(mp_set_player_aim);
CON_COMMAND_EXTERN(mp_set_player_noclip);

CON_COMMAND_EXTERN(mp_get_sentry_x);
CON_COMMAND_EXTERN(mp_get_sentry_y);
CON_COMMAND_EXTERN(mp_get_sentry_aim_x);
CON_COMMAND_EXTERN(mp_get_sentry_aim_y);
CON_COMMAND_EXTERN(mp_get_sentry_team);
CON_COMMAND_EXTERN(mp_get_sentry_health);
CON_COMMAND_EXTERN(mp_get_sentry_owner);

CON_COMMAND_EXTERN(mp_set_sentry_owner);
CON_COMMAND_EXTERN(mp_set_sentry_aim);

CON_COMMAND_EXTERN(mp_get_projectile_x);
CON_COMMAND_EXTERN(mp_get_projectile_y);
CON_COMMAND_EXTERN(mp_get_projectile_type);
CON_COMMAND_EXTERN(mp_get_projectile_team);
CON_COMMAND_EXTERN(mp_get_projectile_move_x);
CON_COMMAND_EXTERN(mp_get_projectile_move_y);
CON_COMMAND_EXTERN(mp_get_projectile_owner);
CON_COMMAND_EXTERN(mp_get_projectile_weapon);
CON_COMMAND_EXTERN(mp_get_projectile_damage);
CON_COMMAND_EXTERN(mp_get_projectile_time_left);
CON_COMMAND_EXTERN(mp_get_projectile_move_interval);
CON_COMMAND_EXTERN(mp_get_projectile_sticky);

CON_COMMAND_EXTERN(mp_set_projectile_owner);
CON_COMMAND_EXTERN(mp_set_projectile_weapon);
CON_COMMAND_EXTERN(mp_set_projectile_damage);
CON_COMMAND_EXTERN(mp_set_projectile_time_left);
CON_COMMAND_EXTERN(mp_set_projectile_move_interval);
CON_COMMAND_EXTERN(mp_set_projectile_move);

CON_COMMAND_EXTERN(mp_get_explosion_x);
CON_COMMAND_EXTERN(mp_get_explosion_y);
CON_COMMAND_EXTERN(mp_get_explosion_team);
CON_COMMAND_EXTERN(mp_get_explosion_owner);
CON_COMMAND_EXTERN(mp_get_explosion_weapon);
CON_COMMAND_EXTERN(mp_get_explosion_damage);
CON_COMMAND_EXTERN(mp_get_explosion_time_left);

CON_COMMAND_EXTERN(mp_set_explosion_owner);
CON_COMMAND_EXTERN(mp_set_explosion_weapon);
CON_COMMAND_EXTERN(mp_set_explosion_damage);
CON_COMMAND_EXTERN(mp_set_explosion_time_left);

CON_COMMAND_EXTERN(mp_get_medkit_x);
CON_COMMAND_EXTERN(mp_get_medkit_y);
CON_COMMAND_EXTERN(mp_get_medkit_alive);
CON_COMMAND_EXTERN(mp_get_medkit_respawn_time_left);

CON_COMMAND_EXTERN(mp_get_ammopack_x);
CON_COMMAND_EXTERN(mp_get_ammopack_y);
CON_COMMAND_EXTERN(mp_get_ammopack_alive);
CON_COMMAND_EXTERN(mp_get_ammopack_respawn_time_left);

CON_COMMAND_EXTERN(mp_get_flag_name);
CON_COMMAND_EXTERN(mp_get_flag_x);
CON_COMMAND_EXTERN(mp_get_flag_y);
CON_COMMAND_EXTERN(mp_get_flag_spawn_x);
CON_COMMAND_EXTERN(mp_get_flag_spawn_y);
CON_COMMAND_EXTERN(mp_get_flag_team);
CON_COMMAND_EXTERN(mp_get_flag_score);
CON_COMMAND_EXTERN(mp_get_flag_carrier);
CON_COMMAND_EXTERN(mp_get_flag_return_time_left);

CON_COMMAND_EXTERN(mp_set_flag_name);
CON_COMMAND_EXTERN(mp_set_flag_spawn);

CON_COMMAND_EXTERN(mp_get_cart_x);
CON_COMMAND_EXTERN(mp_get_cart_y);
CON_COMMAND_EXTERN(mp_get_cart_team);
CON_COMMAND_EXTERN(mp_get_cart_track_size);
CON_COMMAND_EXTERN(mp_get_cart_track_index);

CON_COMMAND_EXTERN(mp_get_ent_x);
CON_COMMAND_EXTERN(mp_get_ent_y);
CON_COMMAND_EXTERN(mp_get_ent_move_x);
CON_COMMAND_EXTERN(mp_get_ent_move_y);
CON_COMMAND_EXTERN(mp_get_ent_color);
CON_COMMAND_EXTERN(mp_get_ent_move_interval);
CON_COMMAND_EXTERN(mp_get_ent_visible);
CON_COMMAND_EXTERN(mp_get_ent_w);
CON_COMMAND_EXTERN(mp_get_ent_h);
CON_COMMAND_EXTERN(mp_get_ent_matrix);
CON_COMMAND_EXTERN(mp_get_ent_matrix_at);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_world);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_environment);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_environment);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_player);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_player);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_projectile);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_projectile);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_explosion);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_explosion);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_sentry);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_sentry);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_medkit);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_ammopack);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_flag);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_flag);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_cart);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_cart);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_ent);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_environment);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_player);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_projectile);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_explosion);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_sentry);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_flag);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_red_all);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_blue_all);
CON_COMMAND_EXTERN(mp_get_ent_solid_to_all);

CON_COMMAND_EXTERN(mp_set_ent_color);
CON_COMMAND_EXTERN(mp_set_ent_move_interval);
CON_COMMAND_EXTERN(mp_set_ent_visible);
CON_COMMAND_EXTERN(mp_set_ent_move);
CON_COMMAND_EXTERN(mp_set_ent_matrix);
CON_COMMAND_EXTERN(mp_set_ent_matrix_at);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_world);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_environment);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_environment);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_player);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_player);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_projectile);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_projectile);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_explosion);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_explosion);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_sentry);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_sentry);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_medkit);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_ammopack);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_flag);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_flag);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_cart);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_cart);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_ent);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_environment);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_player);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_projectile);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_explosion);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_sentry);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_flag);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_red_all);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_blue_all);
CON_COMMAND_EXTERN(mp_set_ent_solid_to_all);

#endif
