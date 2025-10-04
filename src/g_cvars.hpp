#pragma once

#include "q_std.h"
#include "game.h"

#include <cstddef>
#include <cstdint>

enum class game_cvar_stage : uint8_t {
        PRE_INIT,
        INIT,
};

using cvar_default_fn = const char *(*)();

struct game_cvar_default {
        const char *literal = nullptr;
        cvar_default_fn compute = nullptr;

        const char *resolve() const
        {
                return compute ? compute() : literal;
        }
};

struct game_cvar_descriptor {
        cvar_t **storage;
        const char *name;
        game_cvar_default default_value;
        cvar_flags_t flags;
};

inline const char *DefaultCheatsValue()
{
#if defined(_DEBUG)
        return "1";
#else
        return "0";
#endif
}

const char *DefaultGametypeValue();
const char *DefaultRulesetValue();

#define CVAR_LITERAL(value) game_cvar_default{ value, nullptr }
#define CVAR_FMT(...) game_cvar_default{ nullptr, +[]() -> const char * { return G_Fmt(__VA_ARGS__).data(); } }
#define CVAR_VALUE(expr) game_cvar_default{ nullptr, +[]() -> const char * { return (expr); } }

#define GAME_PREINIT_CVARS(_)                                                                                             \
        _(maxclients, "maxclients", CVAR_FMT("{}", MAX_SPLIT_PLAYERS), CVAR_SERVERINFO | CVAR_LATCH)                         \
        _(minplayers, "minplayers", CVAR_LITERAL("2"), CVAR_NOFLAGS)                                                           \
        _(maxplayers, "maxplayers", CVAR_LITERAL("16"), CVAR_NOFLAGS)                                                         \
        _(deathmatch, "deathmatch", CVAR_LITERAL("1"), CVAR_LATCH)                                                            \
        _(teamplay, "teamplay", CVAR_LITERAL("0"), CVAR_SERVERINFO)                                                           \
        _(ctf, "ctf", CVAR_LITERAL("0"), CVAR_SERVERINFO)                                                                     \
        _(g_gametype, "g_gametype", CVAR_VALUE(DefaultGametypeValue()), CVAR_SERVERINFO)                                      \
        _(coop, "coop", CVAR_LITERAL("0"), CVAR_LATCH)

#define GAME_INIT_CVARS(_)                                                                                                 \
        /* General server identity */                                                                                      \
        _(hostname, "hostname", CVAR_LITERAL("Welcome to Muff Mode!"), CVAR_NOFLAGS)                                         \
        _(gamename, "gamename", CVAR_LITERAL(GAMEVERSION), CVAR_SERVERINFO | CVAR_LATCH)                                      \
        /* Weapon offsets */                                                                                                \
        _(gun_x, "gun_x", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                                   \
        _(gun_y, "gun_y", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                                   \
        _(gun_z, "gun_z", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                                   \
        /* Movement and physics */                                                                                          \
        _(g_rollspeed, "g_rollspeed", CVAR_LITERAL("200"), CVAR_NOFLAGS)                                                     \
        _(g_rollangle, "g_rollangle", CVAR_LITERAL("2"), CVAR_NOFLAGS)                                                       \
        _(g_maxvelocity, "g_maxvelocity", CVAR_LITERAL("2000"), CVAR_NOFLAGS)                                               \
        _(g_gravity, "g_gravity", CVAR_LITERAL("800"), CVAR_NOFLAGS)                                                         \
        _(g_skip_view_modifiers, "g_skip_view_modifiers", CVAR_LITERAL("0"), CVAR_NOSET)                                    \
        _(g_stopspeed, "g_stopspeed", CVAR_LITERAL("100"), CVAR_NOFLAGS)                                                     \
        _(g_horde_starting_wave, "g_horde_starting_wave", CVAR_LITERAL("1"), CVAR_SERVERINFO | CVAR_LATCH)                   \
        /* Core game modifiers */                                                                                           \
        _(g_huntercam, "g_huntercam", CVAR_LITERAL("1"), CVAR_SERVERINFO | CVAR_LATCH)                                       \
        _(g_dm_strong_mines, "g_dm_strong_mines", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_dm_random_items, "g_dm_random_items", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_instagib, "g_instagib", CVAR_LITERAL("0"), CVAR_SERVERINFO | CVAR_LATCH)                                        \
        _(g_instagib_splash, "g_instagib_splash", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_owner_auto_join, "g_owner_auto_join", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                           \
        _(g_owner_push_scores, "g_owner_push_scores", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                       \
        _(g_gametype_cfg, "g_gametype_cfg", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_quadhog, "g_quadhog", CVAR_LITERAL("0"), CVAR_SERVERINFO | CVAR_LATCH)                                          \
        _(g_nadefest, "g_nadefest", CVAR_LITERAL("0"), CVAR_SERVERINFO | CVAR_LATCH)                                        \
        _(g_frenzy, "g_frenzy", CVAR_LITERAL("0"), CVAR_SERVERINFO | CVAR_LATCH)                                            \
        _(g_vampiric_damage, "g_vampiric_damage", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_vampiric_exp_min, "g_vampiric_exp_min", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                         \
        _(g_vampiric_health_max, "g_vampiric_health_max", CVAR_LITERAL("9999"), CVAR_NOFLAGS)                                \
        _(g_vampiric_percentile, "g_vampiric_percentile", CVAR_LITERAL("0.67f"), CVAR_NOFLAGS)                               \
        /* Freeze tag */                                                                                                     \
        _(g_frozen_time, "g_frozen_time", CVAR_LITERAL("180"), CVAR_NOFLAGS)                                                 \
        /* Cooperative play */                                                                                               \
        _(g_coop_player_collision, "g_coop_player_collision", CVAR_LITERAL("0"), CVAR_LATCH)                                 \
        _(g_coop_squad_respawn, "g_coop_squad_respawn", CVAR_LITERAL("1"), CVAR_LATCH)                                       \
        _(g_coop_enable_lives, "g_coop_enable_lives", CVAR_LITERAL("0"), CVAR_LATCH)                                         \
        _(g_coop_num_lives, "g_coop_num_lives", CVAR_LITERAL("2"), CVAR_LATCH)                                               \
        _(g_coop_instanced_items, "g_coop_instanced_items", CVAR_LITERAL("1"), CVAR_LATCH)                                   \
        /* Grapple */                                                                                                        \
        _(g_allow_grapple, "g_allow_grapple", CVAR_LITERAL("auto"), CVAR_NOFLAGS)                                            \
        _(g_allow_kill, "g_allow_kill", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                     \
        _(g_grapple_offhand, "g_grapple_offhand", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_grapple_fly_speed, "g_grapple_fly_speed", CVAR_FMT("{}", 650), CVAR_NOFLAGS)                                    \
        _(g_grapple_pull_speed, "g_grapple_pull_speed", CVAR_FMT("{}", 650.0f), CVAR_NOFLAGS)                               \
        _(g_grapple_damage, "g_grapple_damage", CVAR_LITERAL("10"), CVAR_NOFLAGS)                                            \
        /* Messaging and debugging */                                                                                        \
        _(g_frag_messages, "g_frag_messages", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                               \
        _(g_debug_monster_paths, "g_debug_monster_paths", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                   \
        _(g_debug_monster_kills, "g_debug_monster_kills", CVAR_LITERAL("0"), CVAR_LATCH)                                     \
        _(bot_debug_follow_actor, "bot_debug_follow_actor", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                 \
        _(bot_debug_move_to_point, "bot_debug_move_to_point", CVAR_LITERAL("0"), CVAR_NOFLAGS)                               \
        /* Engine / noset */                                                                                                 \
        _(g_dedicated, "dedicated", CVAR_LITERAL("0"), CVAR_NOSET)                                                           \
        _(g_cheats, "cheats", CVAR_VALUE(DefaultCheatsValue()), CVAR_SERVERINFO | CVAR_LATCH)                                 \
        /* Latched */                                                                                                        \
        _(skill, "skill", CVAR_LITERAL("3"), CVAR_LATCH)                                                                     \
        _(maxentities, "maxentities", CVAR_FMT("{}", MAX_ENTITIES), CVAR_LATCH)                                              \
        /* Score limits */                                                                                                   \
        _(fraglimit, "fraglimit", CVAR_LITERAL("0"), CVAR_SERVERINFO)                                                        \
        _(timelimit, "timelimit", CVAR_LITERAL("0"), CVAR_SERVERINFO)                                                        \
        _(roundlimit, "roundlimit", CVAR_LITERAL("8"), CVAR_SERVERINFO)                                                      \
        _(roundtimelimit, "roundtimelimit", CVAR_LITERAL("2"), CVAR_SERVERINFO)                                              \
        _(capturelimit, "capturelimit", CVAR_LITERAL("8"), CVAR_SERVERINFO)                                                  \
        _(mercylimit, "mercylimit", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                         \
        _(noplayerstime, "noplayerstime", CVAR_LITERAL("10"), CVAR_NOFLAGS)                                                  \
        _(g_ruleset, "g_ruleset", CVAR_VALUE(DefaultRulesetValue()), CVAR_SERVERINFO)                                         \
        /* Access control */                                                                                                \
        _(password, "password", CVAR_LITERAL(""), CVAR_USERINFO)                                                             \
        _(spectator_password, "spectator_password", CVAR_LITERAL(""), CVAR_USERINFO)                                         \
        _(admin_password, "admin_password", CVAR_LITERAL(""), CVAR_NOFLAGS)                                                  \
        _(needpass, "needpass", CVAR_LITERAL("0"), CVAR_SERVERINFO)                                                          \
        _(filterban, "filterban", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                           \
        /* View bob */                                                                                                       \
        _(run_pitch, "run_pitch", CVAR_LITERAL("0.002"), CVAR_NOFLAGS)                                                       \
        _(run_roll, "run_roll", CVAR_LITERAL("0.005"), CVAR_NOFLAGS)                                                         \
        _(bob_up, "bob_up", CVAR_LITERAL("0.005"), CVAR_NOFLAGS)                                                             \
        _(bob_pitch, "bob_pitch", CVAR_LITERAL("0.002"), CVAR_NOFLAGS)                                                       \
        _(bob_roll, "bob_roll", CVAR_LITERAL("0.002"), CVAR_NOFLAGS)                                                         \
        /* Flood control */                                                                                                  \
        _(flood_msgs, "flood_msgs", CVAR_LITERAL("4"), CVAR_NOFLAGS)                                                         \
        _(flood_persecond, "flood_persecond", CVAR_LITERAL("4"), CVAR_NOFLAGS)                                               \
        _(flood_waitdelay, "flood_waitdelay", CVAR_LITERAL("10"), CVAR_NOFLAGS)                                              \
        /* AI modifiers */                                                                                                   \
        _(ai_allow_dm_spawn, "ai_allow_dm_spawn", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(ai_damage_scale, "ai_damage_scale", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                               \
        _(ai_model_scale, "ai_model_scale", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                 \
        _(ai_movement_disabled, "ai_movement_disabled", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                     \
        /* Core gameplay flags */                                                                                            \
        _(g_airaccelerate, "g_airaccelerate", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                               \
        _(g_allow_admin, "g_allow_admin", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                   \
        _(g_allow_custom_skins, "g_allow_custom_skins", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                     \
        _(g_allow_forfeit, "g_allow_forfeit", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                               \
        _(g_allow_mymap, "g_allow_mymap", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                   \
        _(g_allow_spec_vote, "g_allow_spec_vote", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_allow_techs, "g_allow_techs", CVAR_LITERAL("auto"), CVAR_NOFLAGS)                                                \
        _(g_allow_vote_midgame, "g_allow_vote_midgame", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                     \
        _(g_allow_voting, "g_allow_voting", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_arena_dmg_armor, "g_arena_dmg_armor", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_arena_start_armor, "g_arena_start_armor", CVAR_LITERAL("200"), CVAR_NOFLAGS)                                     \
        _(g_arena_start_health, "g_arena_start_health", CVAR_LITERAL("200"), CVAR_NOFLAGS)                                   \
        _(g_coop_health_scaling, "g_coop_health_scaling", CVAR_LITERAL("0"), CVAR_LATCH)                                     \
        _(g_corpse_sink_time, "g_corpse_sink_time", CVAR_LITERAL("15"), CVAR_NOFLAGS)                                        \
        _(g_damage_scale, "g_damage_scale", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_disable_player_collision, "g_disable_player_collision", CVAR_LITERAL("0"), CVAR_NOFLAGS)                         \
        /* Deathmatch controls */                                                                                            \
        _(g_dm_allow_exit, "g_dm_allow_exit", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                               \
        _(g_dm_allow_no_humans, "g_dm_allow_no_humans", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                     \
        _(g_dm_auto_join, "g_dm_auto_join", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_dm_crosshair_id, "g_dm_crosshair_id", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                           \
        _(g_dm_do_readyup, "g_dm_do_readyup", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                               \
        _(g_dm_do_warmup, "g_dm_do_warmup", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_dm_exec_level_cfg, "g_dm_exec_level_cfg", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                       \
        _(g_dm_force_join, "g_dm_force_join", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                               \
        _(g_dm_force_respawn, "g_dm_force_respawn", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                         \
        _(g_dm_force_respawn_time, "g_dm_force_respawn_time", CVAR_LITERAL("3"), CVAR_NOFLAGS)                               \
        _(g_dm_holdable_adrenaline, "g_dm_holdable_adrenaline", CVAR_LITERAL("1"), CVAR_NOFLAGS)                             \
        _(g_dm_instant_items, "g_dm_instant_items", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                         \
        _(g_dm_intermission_shots, "g_dm_intermission_shots", CVAR_LITERAL("0"), CVAR_NOFLAGS)                               \
        _(g_dm_item_respawn_rate, "g_dm_item_respawn_rate", CVAR_LITERAL("1.0"), CVAR_NOFLAGS)                               \
        _(g_dm_no_fall_damage, "g_dm_no_fall_damage", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                       \
        _(g_dm_no_quad_drop, "g_dm_no_quad_drop", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        _(g_dm_no_self_damage, "g_dm_no_self_damage", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                       \
        _(g_dm_no_stack_double, "g_dm_no_stack_double", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                     \
        _(g_dm_overtime, "g_dm_overtime", CVAR_LITERAL("120"), CVAR_NOFLAGS)                                                 \
        _(g_dm_powerup_drop, "g_dm_powerup_drop", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                           \
        _(g_dm_powerups_minplayers, "g_dm_powerups_minplayers", CVAR_LITERAL("0"), CVAR_NOFLAGS)                             \
        _(g_dm_respawn_delay_min, "g_dm_respawn_delay_min", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                 \
        _(g_dm_respawn_point_min_dist, "g_dm_respawn_point_min_dist", CVAR_LITERAL("256"), CVAR_NOFLAGS)                     \
        _(g_dm_respawn_point_min_dist_debug, "g_dm_respawn_point_min_dist_debug", CVAR_LITERAL("0"), CVAR_NOFLAGS)           \
        _(g_dm_same_level, "g_dm_same_level", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                               \
        _(g_dm_player_spawn_rule, "g_dm_player_spawn_rule", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                 \
        _(g_dm_spawnpads, "g_dm_spawnpads", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_dm_timeout_length, "g_dm_timeout_length", CVAR_LITERAL("120"), CVAR_NOFLAGS)                                     \
        _(g_dm_weapons_stay, "g_dm_weapons_stay", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                           \
        /* Entity overrides */                                                                                               \
        _(g_drop_cmds, "g_drop_cmds", CVAR_LITERAL("7"), CVAR_NOFLAGS)                                                       \
        _(g_entity_override_dir, "g_entity_override_dir", CVAR_LITERAL("maps"), CVAR_NOFLAGS)                               \
        _(g_entity_override_load, "g_entity_override_load", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                 \
        _(g_entity_override_save, "g_entity_override_save", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                 \
        /* Misc flags */                                                                                                     \
        _(g_eyecam, "g_eyecam", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                             \
        _(g_fast_doors, "g_fast_doors", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                     \
        _(g_frames_per_frame, "g_frames_per_frame", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                         \
        _(g_friendly_fire, "g_friendly_fire", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                               \
        _(g_inactivity, "g_inactivity", CVAR_LITERAL("120"), CVAR_NOFLAGS)                                                   \
        _(g_infinite_ammo, "g_infinite_ammo", CVAR_LITERAL("0"), CVAR_LATCH)                                                 \
        _(g_instant_weapon_switch, "g_instant_weapon_switch", CVAR_LITERAL("0"), CVAR_LATCH)                                 \
        _(g_item_bobbing, "g_item_bobbing", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_knockback_scale, "g_knockback_scale", CVAR_LITERAL("1.0"), CVAR_NOFLAGS)                                         \
        _(g_ladder_steps, "g_ladder_steps", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_lag_compensation, "g_lag_compensation", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                         \
        _(g_map_list, "g_map_list", CVAR_LITERAL(""), CVAR_NOFLAGS)                                                          \
        _(g_map_list_shuffle, "g_map_list_shuffle", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                         \
        _(g_map_pool, "g_map_pool", CVAR_LITERAL(""), CVAR_NOFLAGS)                                                          \
        _(g_mapspawn_no_bfg, "g_no_bfg", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                    \
        _(g_mapspawn_no_plasmabeam, "g_no_plasmabeam", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                      \
        _(g_match_lock, "g_match_lock", CVAR_LITERAL("0"), CVAR_SERVERINFO)                                                  \
        _(g_matchstats, "g_matchstats", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                     \
        _(g_motd_filename, "g_motd_filename", CVAR_LITERAL("motd.txt"), CVAR_NOFLAGS)                                        \
        _(g_mover_debug, "g_mover_debug", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                   \
        _(g_mover_speed_scale, "g_mover_speed_scale", CVAR_LITERAL("1.0f"), CVAR_NOFLAGS)                                    \
        _(g_no_armor, "g_no_armor", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                         \
        _(g_no_health, "g_no_health", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                       \
        _(g_no_items, "g_no_items", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                         \
        _(g_no_mines, "g_no_mines", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                         \
        _(g_no_nukes, "g_no_nukes", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                         \
        _(g_no_powerups, "g_no_powerups", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                   \
        _(g_no_spheres, "g_no_spheres", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                     \
        _(g_quick_weapon_switch, "g_quick_weapon_switch", CVAR_LITERAL("1"), CVAR_LATCH)                                     \
        _(g_round_countdown, "g_round_countdown", CVAR_LITERAL("10"), CVAR_NOFLAGS)                                          \
        _(g_select_empty, "g_select_empty", CVAR_LITERAL("0"), CVAR_ARCHIVE)                                                 \
        _(g_showhelp, "g_showhelp", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                         \
        _(g_showmotd, "g_showmotd", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                         \
        _(g_start_items, "g_start_items", CVAR_LITERAL(""), CVAR_NOFLAGS)                                                    \
        _(g_starting_health, "g_starting_health", CVAR_LITERAL("100"), CVAR_NOFLAGS)                                         \
        _(g_starting_health_bonus, "g_starting_health_bonus", CVAR_LITERAL("25"), CVAR_NOFLAGS)                              \
        _(g_starting_armor, "g_starting_armor", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                             \
        _(g_strict_saves, "g_strict_saves", CVAR_LITERAL("1"), CVAR_NOFLAGS)                                                 \
        _(g_teamplay_allow_team_pick, "g_teamplay_allow_team_pick", CVAR_LITERAL("0"), CVAR_NOFLAGS)                         \
        _(g_teamplay_armor_protect, "g_teamplay_armor_protect", CVAR_LITERAL("0"), CVAR_NOFLAGS)                             \
        _(g_teamplay_auto_balance, "g_teamplay_auto_balance", CVAR_LITERAL("1"), CVAR_NOFLAGS)                               \
        _(g_teamplay_force_balance, "g_teamplay_force_balance", CVAR_LITERAL("0"), CVAR_NOFLAGS)                             \
        _(g_teamplay_item_drop_notice, "g_teamplay_item_drop_notice", CVAR_LITERAL("1"), CVAR_NOFLAGS)                       \
        _(g_teleporter_freeze, "g_teleporter_freeze", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                       \
        _(g_verbose, "g_verbose", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                           \
        _(g_vote_flags, "g_vote_flags", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                                     \
        _(g_vote_limit, "g_vote_limit", CVAR_LITERAL("3"), CVAR_NOFLAGS)                                                     \
        _(g_warmup_countdown, "g_warmup_countdown", CVAR_LITERAL("10"), CVAR_NOFLAGS)                                        \
        _(g_warmup_ready_percentage, "g_warmup_ready_percentage", CVAR_LITERAL("0.51f"), CVAR_NOFLAGS)                       \
        _(g_weapon_projection, "g_weapon_projection", CVAR_LITERAL("0"), CVAR_NOFLAGS)                                       \
        _(g_weapon_respawn_time, "g_weapon_respawn_time", CVAR_LITERAL("30"), CVAR_NOFLAGS)                                  \
        /* Bots */                                                                                                           \
        _(bot_name_prefix, "bot_name_prefix", CVAR_LITERAL("B|"), CVAR_NOFLAGS)

#define DECLARE_GAME_CVAR(identifier, name, default_value, flags) inline cvar_t *identifier = nullptr;
GAME_PREINIT_CVARS(DECLARE_GAME_CVAR)
GAME_INIT_CVARS(DECLARE_GAME_CVAR)
#undef DECLARE_GAME_CVAR

inline const game_cvar_descriptor g_preinit_cvars[] = {
#define DEFINE_GAME_CVAR(identifier, name, default_value, flags) { &identifier, name, default_value, flags },
        GAME_PREINIT_CVARS(DEFINE_GAME_CVAR)
#undef DEFINE_GAME_CVAR
};

inline const game_cvar_descriptor g_init_cvars[] = {
#define DEFINE_GAME_CVAR(identifier, name, default_value, flags) { &identifier, name, default_value, flags },
        GAME_INIT_CVARS(DEFINE_GAME_CVAR)
#undef DEFINE_GAME_CVAR
};

inline constexpr size_t g_preinit_cvars_count = sizeof(g_preinit_cvars) / sizeof(g_preinit_cvars[0]);
inline constexpr size_t g_init_cvars_count = sizeof(g_init_cvars) / sizeof(g_init_cvars[0]);

struct game_cvar_range {
        const game_cvar_descriptor *entries;
        size_t count;
};

inline game_cvar_range GetGameCvars(game_cvar_stage stage)
{
        switch (stage) {
        case game_cvar_stage::PRE_INIT:
                return { g_preinit_cvars, g_preinit_cvars_count };
        case game_cvar_stage::INIT:
                return { g_init_cvars, g_init_cvars_count };
        }

        return { nullptr, 0 };
}

template<typename Fn>
inline void ForEachGameCvar(game_cvar_stage stage, Fn &&fn)
{
        auto range = GetGameCvars(stage);
        for (size_t i = 0; i < range.count; ++i) {
                fn(range.entries[i]);
        }
}

#undef GAME_INIT_CVARS
#undef GAME_PREINIT_CVARS
#undef CVAR_VALUE
#undef CVAR_FMT
#undef CVAR_LITERAL

