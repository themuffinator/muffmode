// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "bots/bot_includes.h"

CHECK_GCLIENT_INTEGRITY;
CHECK_ENTITY_INTEGRITY;

constexpr int32_t DEFAULT_GRAPPLE_SPEED = 650; // speed of grapple in flight
constexpr float	  DEFAULT_GRAPPLE_PULL_SPEED = 650; // speed player is pulled at

std::mt19937 mt_rand;

game_locals_t  game;
level_locals_t level;

local_game_import_t  gi;

/*static*/ char local_game_import_t::print_buffer[0x10000];

/*static*/ std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers;
/*static*/ std::array<const char *, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs;

game_export_t  globals;
spawn_temp_t   st;

cached_modelindex		sm_meat_index;
cached_soundindex		snd_fry;

gentity_t *g_entities;

cvar_t *hostname;

cvar_t *deathmatch;
cvar_t *ctf;
cvar_t *teamplay;
cvar_t *g_gametype;

cvar_t *coop;

cvar_t *skill;
cvar_t *fraglimit;
cvar_t *capturelimit;
cvar_t *timelimit;
cvar_t *roundlimit;
cvar_t *roundtimelimit;
cvar_t *mercylimit;
cvar_t *noplayerstime;

cvar_t *g_ruleset;

cvar_t *password;
cvar_t *spectator_password;
cvar_t *admin_password;
cvar_t *needpass;
cvar_t *filterban;

static cvar_t *maxclients;
static cvar_t *maxentities;
cvar_t *maxplayers;
cvar_t *minplayers;

cvar_t *ai_allow_dm_spawn;
cvar_t *ai_damage_scale;
cvar_t *ai_model_scale;
cvar_t *ai_movement_disabled;
cvar_t *bob_pitch;
cvar_t *bob_roll;
cvar_t *bob_up;
cvar_t *bot_debug_follow_actor;
cvar_t *bot_debug_move_to_point;
cvar_t *flood_msgs;
cvar_t *flood_persecond;
cvar_t *flood_waitdelay;
cvar_t *gun_x, *gun_y, *gun_z;
cvar_t *run_pitch;
cvar_t *run_roll;

cvar_t *g_airaccelerate;
cvar_t *g_allow_admin;
cvar_t *g_allow_custom_skins;
cvar_t *g_allow_forfeit;
cvar_t *g_allow_grapple;
cvar_t *g_allow_kill;
cvar_t *g_allow_mymap;
cvar_t *g_allow_spec_vote;
cvar_t *g_allow_techs;
cvar_t *g_allow_vote_midgame;
cvar_t *g_allow_voting;
cvar_t *g_arena_dmg_armor;
cvar_t *g_arena_start_armor;
cvar_t *g_arena_start_health;
cvar_t *g_cheats;
cvar_t *g_coop_enable_lives;
cvar_t *g_coop_health_scaling;
cvar_t *g_coop_instanced_items;
cvar_t *g_coop_num_lives;
cvar_t *g_coop_player_collision;
cvar_t *g_coop_squad_respawn;
cvar_t *g_damage_scale;
cvar_t *g_debug_monster_kills;
cvar_t *g_debug_monster_paths;
cvar_t *g_dedicated;
cvar_t *g_disable_player_collision;
cvar_t *g_dm_allow_no_humans;
cvar_t *g_dm_auto_join;
cvar_t *g_dm_crosshair_id;
cvar_t *g_dm_do_readyup;
cvar_t *g_dm_do_warmup;
cvar_t *g_dm_exec_level_cfg;
cvar_t *g_dm_force_join;
cvar_t *g_dm_force_respawn;
cvar_t *g_dm_force_respawn_time;
cvar_t *g_dm_holdable_adrenaline;
cvar_t *g_dm_instant_items;
cvar_t *g_dm_intermission_shots;
cvar_t *g_dm_item_respawn_rate;
cvar_t *g_dm_no_fall_damage;
cvar_t *g_dm_no_quad_drop;
cvar_t *g_dm_no_self_damage;
cvar_t *g_dm_overtime;
cvar_t *g_dm_powerup_drop;
cvar_t *g_dm_powerups_minplayers;
cvar_t *g_dm_random_items;
cvar_t *g_dm_respawn_delay_min;
cvar_t *g_dm_respawn_point_min_dist;
cvar_t *g_dm_respawn_point_min_dist_debug;
cvar_t *g_dm_same_level;
cvar_t *g_dm_spawnpads;
cvar_t *g_dm_strong_mines;
cvar_t *g_dm_telepads;
cvar_t *g_dm_timeout_length;
cvar_t *g_dm_weapons_stay;
cvar_t *g_drop_cmds;
cvar_t *g_entity_override_dir;
cvar_t *g_entity_override_load;
cvar_t *g_entity_override_save;
cvar_t *g_eyecam;
cvar_t *g_fast_doors;
cvar_t *g_frag_messages;
cvar_t *g_frenzy;
cvar_t *g_friendly_fire;
cvar_t *g_frozen_time;
cvar_t *g_grapple_damage;
cvar_t *g_grapple_fly_speed;
cvar_t *g_grapple_offhand;
cvar_t *g_grapple_pull_speed;
cvar_t *g_gravity;
cvar_t *g_horde_starting_wave;
cvar_t *g_huntercam;
cvar_t *g_inactivity;
cvar_t *g_infinite_ammo;
cvar_t *g_instagib;
cvar_t *g_instagib_splash;
cvar_t *g_instant_weapon_switch;
cvar_t *g_item_bobbing;
cvar_t *g_knockback_scale;
cvar_t *g_ladder_steps;
cvar_t *g_lag_compensation;
cvar_t *g_level_ruleset;
cvar_t *g_map_list;
cvar_t *g_map_list_shuffle;
cvar_t *g_map_pool;
cvar_t *g_match_lock;
cvar_t *g_matchstats;
cvar_t *g_maxvelocity;
cvar_t *g_motd_filename;
cvar_t *g_mover_debug;
cvar_t *g_mover_speed_scale;
cvar_t *g_nadefest;
cvar_t *g_no_armor;
cvar_t *g_no_bfg;
cvar_t *g_no_health;
cvar_t *g_no_items;
cvar_t *g_no_mines;
cvar_t *g_no_nukes;
cvar_t *g_no_powerups;
cvar_t *g_no_spheres;
cvar_t *g_owner_auto_join;
cvar_t *g_owner_push_scores;
cvar_t *g_gametype_cfg;
cvar_t *g_quadhog;
cvar_t *g_quick_weapon_switch;
cvar_t *g_rollangle;
cvar_t *g_rollspeed;
cvar_t *g_select_empty;
cvar_t *g_showhelp;
cvar_t *g_showmotd;
cvar_t *g_skip_view_modifiers;
cvar_t *g_start_items;
cvar_t *g_starting_health;
cvar_t *g_starting_health_bonus;
cvar_t *g_starting_armor;
cvar_t *g_stopspeed;
cvar_t *g_strict_saves;
cvar_t *g_teamplay_allow_team_pick;
cvar_t *g_teamplay_armor_protect;
cvar_t *g_teamplay_auto_balance;
cvar_t *g_teamplay_force_balance;
cvar_t *g_teamplay_item_drop_notice;
cvar_t *g_vampiric_damage;
cvar_t *g_vampiric_exp_min;
cvar_t *g_vampiric_health_max;
cvar_t *g_vampiric_percentile;
cvar_t *g_verbose;
cvar_t *g_vote_flags;
cvar_t *g_vote_limit;
cvar_t *g_warmup_countdown;
cvar_t *g_warmup_ready_percentage;
cvar_t *g_weapon_projection;
cvar_t *g_weapon_respawn_time;

cvar_t *g_maps_pool_file;
cvar_t *g_maps_cycle_file;
cvar_t *g_maps_selector;
cvar_t *g_maps_mymap;

cvar_t *bot_name_prefix;

static cvar_t *g_frames_per_frame;

int ii_duel_header;
int ii_highlight;
int ii_ctf_red_dropped;
int ii_ctf_blue_dropped;
int ii_ctf_red_taken;
int ii_ctf_blue_taken;
int ii_teams_red_default;
int ii_teams_blue_default;
int ii_teams_red_tiny;
int ii_teams_blue_tiny;
int ii_teams_header_red;
int ii_teams_header_blue;
int mi_ctf_red_flag, mi_ctf_blue_flag; // [Paril-KEX]

void ClientThink(gentity_t *ent, usercmd_t *cmd);
gentity_t *ClientChooseSlot(const char *userInfo, const char *socialID, bool isBot, gentity_t **ignore, size_t num_ignore, bool cinematic);
bool ClientConnect(gentity_t *ent, char *userInfo, const char *socialID, bool isBot);
char *WriteGameJson(bool autosave, size_t *out_size);
void ReadGameJson(const char *jsonString);
char *WriteLevelJson(bool transition, size_t *out_size);
void ReadLevelJson(const char *jsonString);
bool CanSave();
void ClientDisconnect(gentity_t *ent);
void ClientBegin(gentity_t *ent);
void ClientCommand(gentity_t *ent);
void G_RunFrame(bool main_loop);
void G_PrepFrame();
void G_InitSave();

#include <chrono>

// =================================================

void LoadMotd() {
	// load up ent override
	const char *name = G_Fmt("baseq2/{}", g_motd_filename->string[0] ? g_motd_filename->string : "motd.txt").data();
	FILE *f = fopen(name, "rb");
	bool valid = true;
	if (f != NULL) {
		char *buffer = nullptr;
		size_t length;
		size_t read_length;

		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (length > 0x40000) {
			gi.Com_PrintFmt("{}: MoTD file length exceeds maximum: \"{}\"\n", __FUNCTION__, name);
			valid = false;
		}
		if (valid) {
			buffer = (char *)gi.TagMalloc(length + 1, '\0');
			if (length) {
				read_length = fread(buffer, 1, length, f);

				if (length != read_length) {
					gi.Com_PrintFmt("{}: MoTD file read error: \"{}\"\n", __FUNCTION__, name);
					valid = false;
				}
			}
		}
		fclose(f);
		
		if (valid) {
			game.motd = (const char *)buffer;
			game.motd_mod_count++;
			if (g_verbose->integer)
				gi.Com_PrintFmt("{}: MotD file verified and loaded: \"{}\"\n", __FUNCTION__, name);
		} else {
			gi.Com_PrintFmt("{}: MotD file load error for \"{}\", discarding.\n", __FUNCTION__, name);
		}
	}
}

int check_ruleset = -1;
static void CheckRuleset() {
	if (game.ruleset && check_ruleset == g_ruleset->modified_count)
		return;

	game.ruleset = (ruleset_t)clamp(g_ruleset->integer, (int)RS_NONE + 1, (int)RS_NUM_RULESETS - 1);

	if ((int)game.ruleset != g_ruleset->integer)
		gi.cvar_forceset("g_ruleset", G_Fmt("{}", (int)game.ruleset).data());

	check_ruleset = g_ruleset->modified_count;

	gi.LocBroadcast_Print(PRINT_HIGH, "Ruleset: {}\n", rs_long_name[(int)game.ruleset]);
}

int gt_teamplay = 0;
int gt_ctf = 0;
int gt_g_gametype = 0;
bool gt_teams_on = false;
gametype_t gt_check = GT_NONE;
void GT_Changes() {
	if (!deathmatch->integer)
		return;

	// do these checks only once level has initialised
	if (!level.init)
		return;

	bool changed = false, team_reset = false;
	gametype_t gt = gametype_t::GT_NONE;

	if (gt_g_gametype != g_gametype->modified_count) {
		gt = (gametype_t)clamp(g_gametype->integer, (int)GT_FIRST, (int)GT_LAST);

		if (gt != gt_check) {
			switch (gt) {
			case gametype_t::GT_TDM:
				if (!teamplay->integer)
					gi.cvar_forceset("teamplay", "1");
				break;
			case gametype_t::GT_CTF:
				if (!ctf->integer)
					gi.cvar_forceset("ctf", "1");
				break;
			default:
				if (teamplay->integer)
					gi.cvar_forceset("teamplay", "0");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
				break;
			}
			gt_teamplay = teamplay->modified_count;
			gt_ctf = ctf->modified_count;
			changed = true;
		}
	}

	if (!changed) {
		if (gt_teamplay != teamplay->modified_count) {
			if (teamplay->integer) {
				gt = gametype_t::GT_TDM;
				if (!teamplay->integer)
					gi.cvar_forceset("teamplay", "1");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
			} else {
				gt = gametype_t::GT_FFA;
				if (teamplay->integer)
					gi.cvar_forceset("teamplay", "0");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
			}
			changed = true;
			gt_teamplay = teamplay->modified_count;
			gt_ctf = ctf->modified_count;
		}
		if (gt_ctf != ctf->modified_count) {
			if (ctf->integer) {
				gt = gametype_t::GT_CTF;
				if (teamplay->integer)
					gi.cvar_forceset("teamplay", "0");
				if (!ctf->integer)
					gi.cvar_forceset("ctf", "1");
			} else {
				gt = gametype_t::GT_TDM;
				if (!teamplay->integer)
					gi.cvar_forceset("teamplay", "1");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
			}
			changed = true;
			gt_teamplay = teamplay->modified_count;
			gt_ctf = ctf->modified_count;
		}
	}

	if (!changed || gt == gametype_t::GT_NONE)
		return;

	//gi.Com_PrintFmt("GAMETYPE = {}\n", (int)gt);
	
	if (gt_teams_on != Teams()) {
		team_reset = true;
		gt_teams_on = Teams();
	}

	if (team_reset) {
		// move all to spectator first
		for (auto ec : active_clients()) {
			//SetIntermissionPoint();
			FindIntermissionPoint();

			ec->s.origin = level.intermissionOrigin;
			ec->client->ps.pmove.origin = level.intermissionOrigin;
			ec->client->ps.viewangles = level.intermissionAngle;

			ec->client->awaiting_respawn = true;
			ec->client->ps.pmove.pm_type = PM_FREEZE;
			ec->client->ps.rdflags = RDF_NONE;
			ec->deadFlag = false;
			ec->solid = SOLID_NOT;
			ec->moveType = MOVETYPE_FREECAM;
			ec->s.modelindex = 0;
			ec->svFlags |= SVF_NOCLIENT;
			gi.linkentity(ec);
		}

		// set to team and reset match
		for (auto ec : active_clients()) {
			if (!ClientIsPlaying(ec->client))
				continue;
			SetTeam(ec, PickTeam(-1), false, false, true);
		}
	}

	if ((int)gt != gt_check) {
		gi.cvar_forceset("g_gametype", G_Fmt("{}", (int)gt).data());
		gt_g_gametype = g_gametype->modified_count;
		gt_check = (gametype_t)g_gametype->integer;
	} else return;

	//TODO: save ent string so we can simply reload it and Match_Reset
	//gi.AddCommandString("map_restart");

	gi.AddCommandString(G_Fmt("gamemap {}\n", level.mapname).data());

	GT_PrecacheAssets();
	GT_SetLongName();
	gi.LocBroadcast_Print(PRINT_CENTER, "{}", level.gametype_name);
}

/*
============
PreInitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
extern void GT_Init();
static void PreInitGame() {
	maxclients = gi.cvar("maxclients", G_Fmt("{}", MAX_SPLIT_PLAYERS).data(), CVAR_SERVERINFO | CVAR_LATCH);
	minplayers = gi.cvar("minplayers", "2", CVAR_NOFLAGS);
	maxplayers = gi.cvar("maxplayers", "16", CVAR_NOFLAGS);

	GT_Init();
}

/*
============
InitGame

Called after PreInitGame when the game has set up cvars.
============
*/
extern void Horde_Init();

static void InitGame() {
	gi.Com_Print("==== InitGame ====\n");

	G_InitSave();

	// seed RNG
	mt_rand.seed((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());

	hostname = gi.cvar("hostname", "Welcome to WOR!", CVAR_NOFLAGS);

	gun_x = gi.cvar("gun_x", "0", CVAR_NOFLAGS);
	gun_y = gi.cvar("gun_y", "0", CVAR_NOFLAGS);
	gun_z = gi.cvar("gun_z", "0", CVAR_NOFLAGS);

	g_rollspeed = gi.cvar("g_rollspeed", "200", CVAR_NOFLAGS);
	g_rollangle = gi.cvar("g_rollangle", "2", CVAR_NOFLAGS);
	g_maxvelocity = gi.cvar("g_maxvelocity", "2000", CVAR_NOFLAGS);
	g_gravity = gi.cvar("g_gravity", "800", CVAR_NOFLAGS);

	g_skip_view_modifiers = gi.cvar("g_skip_view_modifiers", "0", CVAR_NOSET);

	g_stopspeed = gi.cvar("g_stopspeed", "100", CVAR_NOFLAGS);

	g_horde_starting_wave = gi.cvar("g_horde_starting_wave", "1", CVAR_SERVERINFO | CVAR_LATCH);

	g_huntercam = gi.cvar("g_huntercam", "1", CVAR_SERVERINFO | CVAR_LATCH);
	g_dm_strong_mines = gi.cvar("g_dm_strong_mines", "0", CVAR_NOFLAGS);
	g_dm_random_items = gi.cvar("g_dm_random_items", "0", CVAR_NOFLAGS);

	// freeze tag
	g_frozen_time = gi.cvar("g_frozen_time", "180", CVAR_NOFLAGS);

	// [Paril-KEX]
	g_coop_player_collision = gi.cvar("g_coop_player_collision", "0", CVAR_LATCH);
	g_coop_squad_respawn = gi.cvar("g_coop_squad_respawn", "1", CVAR_LATCH);
	g_coop_enable_lives = gi.cvar("g_coop_enable_lives", "0", CVAR_LATCH);
	g_coop_num_lives = gi.cvar("g_coop_num_lives", "2", CVAR_LATCH);
	g_coop_instanced_items = gi.cvar("g_coop_instanced_items", "1", CVAR_LATCH);
	g_allow_grapple = gi.cvar("g_allow_grapple", "auto", CVAR_NOFLAGS);
	g_allow_kill = gi.cvar("g_allow_kill", "1", CVAR_NOFLAGS);
	g_grapple_offhand = gi.cvar("g_grapple_offhand", "0", CVAR_NOFLAGS);
	g_grapple_fly_speed = gi.cvar("g_grapple_fly_speed", G_Fmt("{}", DEFAULT_GRAPPLE_SPEED).data(), CVAR_NOFLAGS);
	g_grapple_pull_speed = gi.cvar("g_grapple_pull_speed", G_Fmt("{}", DEFAULT_GRAPPLE_PULL_SPEED).data(), CVAR_NOFLAGS);
	g_grapple_damage = gi.cvar("g_grapple_damage", "10", CVAR_NOFLAGS);

	g_frag_messages = gi.cvar("g_frag_messages", "1", CVAR_NOFLAGS);

	g_debug_monster_paths = gi.cvar("g_debug_monster_paths", "0", CVAR_NOFLAGS);
	g_debug_monster_kills = gi.cvar("g_debug_monster_kills", "0", CVAR_LATCH);

	bot_debug_follow_actor = gi.cvar("bot_debug_follow_actor", "0", CVAR_NOFLAGS);
	bot_debug_move_to_point = gi.cvar("bot_debug_move_to_point", "0", CVAR_NOFLAGS);

	// noset vars
	g_dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);

	// latched vars
	g_cheats = gi.cvar("cheats",
#if defined(_DEBUG)
		"1"
#else
		"0"
#endif
		, CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar("gamename", GAMEVERSION.c_str(), CVAR_SERVERINFO | CVAR_LATCH);

	skill = gi.cvar("skill", "3", CVAR_LATCH);
	maxentities = gi.cvar("maxentities", G_Fmt("{}", MAX_ENTITIES).data(), CVAR_LATCH);

	// change anytime vars
	fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
	roundlimit = gi.cvar("roundlimit", "8", CVAR_SERVERINFO);
	roundtimelimit = gi.cvar("roundtimelimit", "2", CVAR_SERVERINFO);
	capturelimit = gi.cvar("capturelimit", "8", CVAR_SERVERINFO);
	mercylimit = gi.cvar("mercylimit", "0", CVAR_NOFLAGS);
	noplayerstime = gi.cvar("noplayerstime", "10", CVAR_NOFLAGS);

	g_ruleset = gi.cvar("g_ruleset", G_Fmt("{}", (int)RS_Q2).data(), CVAR_SERVERINFO);

	password = gi.cvar("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
	admin_password = gi.cvar("admin_password", "", CVAR_NOFLAGS);
	needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar("filterban", "1", CVAR_NOFLAGS);

	run_pitch = gi.cvar("run_pitch", "0.002", CVAR_NOFLAGS);
	run_roll = gi.cvar("run_roll", "0.005", CVAR_NOFLAGS);
	bob_up = gi.cvar("bob_up", "0.005", CVAR_NOFLAGS);
	bob_pitch = gi.cvar("bob_pitch", "0.002", CVAR_NOFLAGS);
	bob_roll = gi.cvar("bob_roll", "0.002", CVAR_NOFLAGS);

	flood_msgs = gi.cvar("flood_msgs", "4", CVAR_NOFLAGS);
	flood_persecond = gi.cvar("flood_persecond", "4", CVAR_NOFLAGS);
	flood_waitdelay = gi.cvar("flood_waitdelay", "10", CVAR_NOFLAGS);

	ai_allow_dm_spawn = gi.cvar("ai_allow_dm_spawn", "0", CVAR_NOFLAGS);
	ai_damage_scale = gi.cvar("ai_damage_scale", "1", CVAR_NOFLAGS);
	ai_model_scale = gi.cvar("ai_model_scale", "0", CVAR_NOFLAGS);
	ai_movement_disabled = gi.cvar("ai_movement_disabled", "0", CVAR_NOFLAGS);

	g_airaccelerate = gi.cvar("g_airaccelerate", "0", CVAR_NOFLAGS);
	g_allow_admin = gi.cvar("g_allow_admin", "1", CVAR_NOFLAGS);
	g_allow_custom_skins = gi.cvar("g_allow_custom_skins", "1", CVAR_NOFLAGS);
	g_allow_forfeit = gi.cvar("g_allow_forfeit", "1", CVAR_NOFLAGS);
	g_allow_mymap = gi.cvar("g_allow_mymap", "1", CVAR_NOFLAGS);
	g_allow_spec_vote = gi.cvar("g_allow_spec_vote", "0", CVAR_NOFLAGS);
	g_allow_techs = gi.cvar("g_allow_techs", "auto", CVAR_NOFLAGS);
	g_allow_vote_midgame = gi.cvar("g_allow_vote_midgame", "0", CVAR_NOFLAGS);
	g_allow_voting = gi.cvar("g_allow_voting", "1", CVAR_NOFLAGS);
	g_arena_dmg_armor = gi.cvar("g_arena_dmg_armor", "0", CVAR_NOFLAGS);
	g_arena_start_armor = gi.cvar("g_arena_start_armor", "200", CVAR_NOFLAGS);
	g_arena_start_health = gi.cvar("g_arena_start_health", "200", CVAR_NOFLAGS);
	g_coop_health_scaling = gi.cvar("g_coop_health_scaling", "0", CVAR_LATCH);
	g_damage_scale = gi.cvar("g_damage_scale", "1", CVAR_NOFLAGS);
	g_disable_player_collision = gi.cvar("g_disable_player_collision", "0", CVAR_NOFLAGS);
	g_dm_allow_no_humans = gi.cvar("g_dm_allow_no_humans", "1", CVAR_NOFLAGS);
	g_dm_auto_join = gi.cvar("g_dm_auto_join", "1", CVAR_NOFLAGS);
	g_dm_crosshair_id = gi.cvar("g_dm_crosshair_id", "1", CVAR_NOFLAGS);
	g_dm_do_readyup = gi.cvar("g_dm_do_readyup", "0", CVAR_NOFLAGS);
	g_dm_do_warmup = gi.cvar("g_dm_do_warmup", "1", CVAR_NOFLAGS);
	g_dm_exec_level_cfg = gi.cvar("g_dm_exec_level_cfg", "0", CVAR_NOFLAGS);
	g_dm_force_join = gi.cvar("g_dm_force_join", "0", CVAR_NOFLAGS);
	g_dm_force_respawn = gi.cvar("g_dm_force_respawn", "1", CVAR_NOFLAGS);
	g_dm_force_respawn_time = gi.cvar("g_dm_force_respawn_time", "3", CVAR_NOFLAGS);
	g_dm_holdable_adrenaline = gi.cvar("g_dm_holdable_adrenaline", "1", CVAR_NOFLAGS);
	g_dm_instant_items = gi.cvar("g_dm_instant_items", "1", CVAR_NOFLAGS);
	g_dm_intermission_shots = gi.cvar("g_dm_intermission_shots", "0", CVAR_NOFLAGS);
	g_dm_item_respawn_rate = gi.cvar("g_dm_item_respawn_rate", "1.0", CVAR_NOFLAGS);
	g_dm_no_fall_damage = gi.cvar("g_dm_no_fall_damage", "0", CVAR_NOFLAGS);
	g_dm_no_quad_drop = gi.cvar("g_dm_no_quad_drop", "0", CVAR_NOFLAGS);
	g_dm_no_self_damage = gi.cvar("g_dm_no_self_damage", "0", CVAR_NOFLAGS);
	g_dm_overtime = gi.cvar("g_dm_overtime", "120", CVAR_NOFLAGS);
	g_dm_powerup_drop = gi.cvar("g_dm_powerup_drop", "1", CVAR_NOFLAGS);
	g_dm_powerups_minplayers = gi.cvar("g_dm_powerups_minplayers", "0", CVAR_NOFLAGS);
	g_dm_respawn_delay_min = gi.cvar("g_dm_respawn_delay_min", "1", CVAR_NOFLAGS);
	g_dm_respawn_point_min_dist = gi.cvar("g_dm_respawn_point_min_dist", "256", CVAR_NOFLAGS);
	g_dm_respawn_point_min_dist_debug = gi.cvar("g_dm_respawn_point_min_dist_debug", "0", CVAR_NOFLAGS);
	g_dm_same_level = gi.cvar("g_dm_same_level", "0", CVAR_NOFLAGS);
	g_dm_spawnpads = gi.cvar("g_dm_spawnpads", "1", CVAR_NOFLAGS);
	g_dm_telepads = gi.cvar("g_dm_telepads", "1", CVAR_NOFLAGS);
	g_dm_timeout_length = gi.cvar("g_dm_timeout_length", "120", CVAR_NOFLAGS);
	g_dm_weapons_stay = gi.cvar("g_dm_weapons_stay", "0", CVAR_NOFLAGS);
	g_drop_cmds = gi.cvar("g_drop_cmds", "7", CVAR_NOFLAGS);
	g_entity_override_dir = gi.cvar("g_entity_override_dir", "maps", CVAR_NOFLAGS);
	g_entity_override_load = gi.cvar("g_entity_override_load", "1", CVAR_NOFLAGS);
	g_entity_override_save = gi.cvar("g_entity_override_save", "0", CVAR_NOFLAGS);
	g_eyecam = gi.cvar("g_eyecam", "1", CVAR_NOFLAGS);
	g_fast_doors = gi.cvar("g_fast_doors", "1", CVAR_NOFLAGS);
	g_frames_per_frame = gi.cvar("g_frames_per_frame", "1", CVAR_NOFLAGS);
	g_friendly_fire = gi.cvar("g_friendly_fire", "0", CVAR_NOFLAGS);
	g_inactivity = gi.cvar("g_inactivity", "120", CVAR_NOFLAGS);
	g_infinite_ammo = gi.cvar("g_infinite_ammo", "0", CVAR_LATCH);
	g_instant_weapon_switch = gi.cvar("g_instant_weapon_switch", "0", CVAR_LATCH);
	g_item_bobbing = gi.cvar("g_item_bobbing", "1", CVAR_NOFLAGS);
	g_knockback_scale = gi.cvar("g_knockback_scale", "1.0", CVAR_NOFLAGS);
	g_ladder_steps = gi.cvar("g_ladder_steps", "1", CVAR_NOFLAGS);
	g_lag_compensation = gi.cvar("g_lag_compensation", "1", CVAR_NOFLAGS);
	g_level_ruleset = gi.cvar("g_level_ruleset", "0", CVAR_NOFLAGS);
	g_map_list = gi.cvar("g_map_list", "", CVAR_NOFLAGS);
	g_map_list_shuffle = gi.cvar("g_map_list_shuffle", "1", CVAR_NOFLAGS);
	g_map_pool = gi.cvar("g_map_pool", "", CVAR_NOFLAGS);
	g_match_lock = gi.cvar("g_match_lock", "0", CVAR_SERVERINFO);
	g_matchstats = gi.cvar("g_matchstats", "0", CVAR_NOFLAGS);
	g_motd_filename = gi.cvar("g_motd_filename", "motd.txt", CVAR_NOFLAGS);
	g_mover_debug = gi.cvar("g_mover_debug", "0", CVAR_NOFLAGS);
	g_mover_speed_scale = gi.cvar("g_mover_speed_scale", "1.0f", CVAR_NOFLAGS);
	g_no_armor = gi.cvar("g_no_armor", "0", CVAR_NOFLAGS);
	g_no_bfg = gi.cvar("g_no_bfg", "0", CVAR_NOFLAGS);
	g_no_health = gi.cvar("g_no_health", "0", CVAR_NOFLAGS);
	g_no_items = gi.cvar("g_no_items", "0", CVAR_NOFLAGS);
	g_no_mines = gi.cvar("g_no_mines", "0", CVAR_NOFLAGS);
	g_no_nukes = gi.cvar("g_no_nukes", "0", CVAR_NOFLAGS);
	g_no_powerups = gi.cvar("g_no_powerups", "0", CVAR_NOFLAGS);
	g_no_spheres = gi.cvar("g_no_spheres", "0", CVAR_NOFLAGS);
	g_quick_weapon_switch = gi.cvar("g_quick_weapon_switch", "1", CVAR_LATCH);
	g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);
	g_showhelp = gi.cvar("g_showhelp", "1", CVAR_NOFLAGS);
	g_showmotd = gi.cvar("g_showmotd", "1", CVAR_NOFLAGS);
	g_start_items = gi.cvar("g_start_items", "", CVAR_NOFLAGS);
	g_starting_health = gi.cvar("g_starting_health", "100", CVAR_NOFLAGS);
	g_starting_health_bonus = gi.cvar("g_starting_health_bonus", "25", CVAR_NOFLAGS);
	g_starting_armor = gi.cvar("g_starting_armor", "0", CVAR_NOFLAGS);
	g_strict_saves = gi.cvar("g_strict_saves", "1", CVAR_NOFLAGS);
	g_teamplay_allow_team_pick = gi.cvar("g_teamplay_allow_team_pick", "0", CVAR_NOFLAGS);
	g_teamplay_armor_protect = gi.cvar("g_teamplay_armor_protect", "0", CVAR_NOFLAGS);
	g_teamplay_auto_balance = gi.cvar("g_teamplay_auto_balance", "1", CVAR_NOFLAGS);
	g_teamplay_force_balance = gi.cvar("g_teamplay_force_balance", "0", CVAR_NOFLAGS);
	g_teamplay_item_drop_notice = gi.cvar("g_teamplay_item_drop_notice", "1", CVAR_NOFLAGS);
	g_verbose = gi.cvar("g_verbose", "0", CVAR_NOFLAGS);
	g_vote_flags = gi.cvar("g_vote_flags", "0", CVAR_NOFLAGS);
	g_vote_limit = gi.cvar("g_vote_limit", "3", CVAR_NOFLAGS);
	g_warmup_countdown = gi.cvar("g_warmup_countdown", "10", CVAR_NOFLAGS);
	g_warmup_ready_percentage = gi.cvar("g_warmup_ready_percentage", "0.51f", CVAR_NOFLAGS);
	g_weapon_projection = gi.cvar("g_weapon_projection", "0", CVAR_NOFLAGS);
	g_weapon_respawn_time = gi.cvar("g_weapon_respawn_time", "30", CVAR_NOFLAGS);

	g_maps_pool_file = gi.cvar("g_maps_pool_file", "mapdb.json", CVAR_NOFLAGS);
	g_maps_cycle_file = gi.cvar("g_maps_cycle_file", "mapcycle.txt", CVAR_NOFLAGS);
	g_maps_selector = gi.cvar("g_maps_selector", "1", CVAR_NOFLAGS);
	g_maps_mymap = gi.cvar("g_maps_mymap", "1", CVAR_NOFLAGS);

	bot_name_prefix = gi.cvar("bot_name_prefix", "B|", CVAR_NOFLAGS);

	// items
	InitItems();

	// ruleset
	CheckRuleset();

	game = {};

	// initialize all entities for this game
	game.maxentities = maxentities->integer;
	g_entities = (gentity_t *)gi.TagMalloc(game.maxentities * sizeof(g_entities[0]), TAG_GAME);
	globals.gentities = g_entities;
	globals.max_entities = game.maxentities;
	
	// initialize all clients for this game
	game.maxclients = maxclients->integer > MAX_CLIENTS_KEX ? MAX_CLIENTS_KEX : maxclients->integer;
	game.clients = (gclient_t *)gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_entities = game.maxclients + 1;

	// how far back we should support lag origins for
	game.max_lag_origins = 20 * (0.1f / gi.frame_time_s);
	game.lag_origins = (vec3_t *)gi.TagMalloc(game.maxclients * sizeof(vec3_t) * game.max_lag_origins, TAG_GAME);

	game.spawnArmor = !g_no_armor->integer;
	game.spawnAmmo = true;
	game.spawnPowerArmor = true;	// !g_no_armor->integer;
	game.spawnPowerups = !g_no_powerups->integer;
	game.spawnHealth = !g_no_health->integer;
	game.spawnBFG = !g_no_bfg->integer;
	game.fallingDamage = !g_dm_no_fall_damage->integer;
	game.selfDamage = !g_dm_no_self_damage->integer;

	level.levelStartTime = level.time;

	level.readyToExit = false;

	level.match_state = MatchState::MATCH_WARMUP_DELAYED;
	level.match_state_timer = 0_sec;
	level.matchStartTime = 0_sec;
	level.warmup_notice_time = level.time;

	memset(level.locked, false, sizeof(level.locked));

	*level.weapon_count = { 0 };

	level.vote = nullptr;
	level.vote_arg = '\n';

	level.match.totalDeaths = 0;

	gt_teamplay = teamplay->modified_count;
	gt_ctf = ctf->modified_count;
	gt_g_gametype = g_gametype->modified_count;
	gt_teams_on = Teams();

	Horde_Init();

	LoadMotd();

	if (g_dm_exec_level_cfg->integer)
		gi.AddCommandString(G_Fmt("exec {}\n", level.mapname).data());

	if (g_gametype_cfg->integer && deathmatch->integer) {
		//gi.Com_PrintFmt("exec gt-{}.cfg\n", gt_short_name_upper[g_gametype->integer].c_str());
		gi.AddCommandString(G_Fmt("exec gt-{}.cfg\n", GametypeIndexToString((gametype_t)g_gametype->integer).c_str()).data());
	}
}

//===================================================================

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint(void) {
	gentity_t *ent, *target;
	vec3_t	dir;
	bool	is_landmark = false;

	if (level.intermissionSpot) // search only once
		return;

	//gi.Com_Print("FindIntermissionPoint\n");

	// find the intermission spot
	ent = level.spawnSpots[SPAWN_SPOT_INTERMISSION];

	if (!ent) { // the map creator forgot to put in an intermission point...
		SelectSpawnPoint(NULL, level.intermissionOrigin, level.intermissionAngle, false, is_landmark);
	} else {
		level.intermissionOrigin = ent->s.origin;

		// ugly hax!
		if (!Q_strncasecmp(level.mapname, "campgrounds", 11)) {
			gvec3_t v = { -320, -96, 503 };
			if (ent->s.origin == v)
				level.intermissionAngle[PITCH] = -30;
		} else if (!Q_strncasecmp(level.mapname, "rdm10", 5)) {
			gvec3_t v = { -1256, -1672, -136 };
			if (ent->s.origin == v)
				level.intermissionAngle = { 15, 135, 0 };
		} else {
			level.intermissionAngle = ent->s.angles;
		}

		// if it has a target, look towards it
		if (ent->target) {
			//gi.Com_Print("FindIntermissionPoint target\n");
			target = PickTarget(ent->target);
			if (target) {
				//gi.Com_Print("FindIntermissionPoint target 2\n");
				dir = (target->s.origin - ent->s.origin).normalized();
				AngleVectors(dir);
				level.intermissionAngle = dir * 360;
				gi.Com_PrintFmt("FindIntermissionPoint angles: {}\n", level.intermissionAngle);
			}
		}
	}

	level.intermissionSpot = true;
}

/*
==================
SetIntermissionPoint
==================
*/
void SetIntermissionPoint(void) {
	if (level.levelIntermissionSet)
		return;

	//FindIntermissionPoint();
	//gi.Com_Print("SetIntermissionPoint\n");

	gentity_t *ent;
	// find an intermission spot
	ent = G_FindByString<&gentity_t::className>(nullptr, "info_player_intermission");
	if (!ent) { // the map creator forgot to put in an intermission point...
		ent = G_FindByString<&gentity_t::className>(nullptr, "info_player_start");
		if (!ent)
			ent = G_FindByString<&gentity_t::className>(nullptr, "info_player_deathmatch");
	} else { // choose one of four spots
		int32_t i = irandom(4);
		while (i--) {
			ent = G_FindByString<&gentity_t::className>(ent, "info_player_intermission");
			if (!ent) // wrap around the list
				ent = G_FindByString<&gentity_t::className>(ent, "info_player_intermission");
		}
	}

	if (ent) {
		level.intermissionOrigin = ent->s.origin;
		level.spawnSpots[SPAWN_SPOT_INTERMISSION] = ent;
	}
	
	// ugly hax!
	if (!Q_strncasecmp(level.mapname, "campgrounds", 11)) {
		gvec3_t v = { -320, -96, 503 };
		if (ent->s.origin == v)
			level.intermissionAngle[PITCH] = -30;
	} else if (!Q_strncasecmp(level.mapname, "rdm10", 5)) {
		gvec3_t v = { -1256, -1672, -136 };
		if (ent->s.origin == v)
			level.intermissionAngle = { 15, 135, 0 };
	} else {
		// if it has a target, look towards it
		if (ent && ent->target) {
			gentity_t *target = PickTarget(ent->target);

			if (target) {
				//gi.Com_Print("HAS TARGET\n");
				vec3_t	dir = (target->s.origin - level.intermissionOrigin).normalized();
				AngleVectors(dir);
				level.intermissionAngle = dir;
			}
		}
		if (ent && !level.intermissionAngle)
			level.intermissionAngle = ent->s.angles;
	}
	
	//gi.Com_PrintFmt("{}: origin={} angles={}\n", __FUNCTION__, level.intermissionOrigin, level.intermissionAngle);
}

//===================================================================
// MAP QUEUE SYSTEM
//===================================================================

static void MQ_Clear() {
	if (!deathmatch)
		return;

	game.mapqueue.clear();
}

static bool MQ_Update() {
	if (!deathmatch)
		return false;

	if (!g_allow_mymap->integer)
		return false;

	if (!g_map_list->string[0] && game.mapqueue.size()) {
		MQ_Clear();
		gi.Broadcast_Print(PRINT_HIGH, "Map queue has been cleared.\n");
		return false;
	}

	//TODO: remove empty elements?

	return true;
}

bool MQ_Add(gentity_t *ent, const char *mapname) {
	if (!deathmatch)
		return false;

	if (!g_allow_mymap->integer)
		return false;

	if (!mapname[0]) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid map name.\n");
		return false;
	}

	if (!g_map_list->string[0])
		return false;

	if (!strstr(g_map_list->string, mapname)) {
		gi.Client_Print(ent, PRINT_HIGH, "Selected map is either invalid or not in cycle.\n");
		return false;
	}

	if (!MQ_Update())
		return false;
	
	// ensure map isn't already in the queue
	if (std::find(game.mapqueue.begin(), game.mapqueue.end(), mapname) != game.mapqueue.end()) {
		gi.Client_Print(ent, PRINT_HIGH, "Selected map is already in queue.\n");
		return false;
	}

	// add it!
	game.mapqueue.push_back(mapname);

	return true;
}

static void MQ_Remove_Index(gentity_t *ent, int num) {
	if (!deathmatch)
		return;

	if (!MQ_Update())
		return;

	game.mapqueue[num].clear();
}

static const char *MQ_Go_Next() {
	if (!deathmatch)
		return nullptr;

	if (!MQ_Update())
		return nullptr;

	for (size_t i = 0; i < game.mapqueue.size(); i++) {
		if (game.mapqueue[i].empty())
			continue;
		const char *s = G_Fmt("{}", game.mapqueue[i]).data();
		game.mapqueue.erase(game.mapqueue.begin() + i);
		return s;
	}
	return nullptr;
}

int MQ_Count() {
	if (!deathmatch)
		return 0;

	if (!g_allow_mymap->integer)
		return 0;

	if (!MQ_Update())
		return 0;
	//gi.Com_PrintFmt("AAAAAAAAAAAAAAA size={}\n", .size());
	return game.mapqueue.size();
}

//===================================================================

static void ShutdownGame() {
	gi.Com_Print("==== ShutdownGame ====\n");

	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);
}

static void *G_GetExtension(const char *name) {
	return nullptr;
}

const shadow_light_data_t *GetShadowLightData(int32_t entity_number);

gtime_t FRAME_TIME_S;
gtime_t FRAME_TIME_MS;

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
Q2GAME_API game_export_t * GetGameAPI(game_import_t * import) {
	gi = *import;

	FRAME_TIME_S = FRAME_TIME_MS = gtime_t::from_ms(gi.frame_time_ms);

	globals.apiversion = GAME_API_VERSION;
	globals.PreInit = PreInitGame;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGameJson = WriteGameJson;
	globals.ReadGameJson = ReadGameJson;
	globals.WriteLevelJson = WriteLevelJson;
	globals.ReadLevelJson = ReadLevelJson;
	globals.CanSave = CanSave;

	globals.Pmove = Pmove;

	globals.GetExtension = G_GetExtension;

	globals.ClientChooseSlot = ClientChooseSlot;
	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;
	globals.PrepFrame = G_PrepFrame;

	globals.ServerCommand = ServerCommand;
	globals.Bot_SetWeapon = Bot_SetWeapon;
	globals.Bot_TriggerEntity = Bot_TriggerEntity;
	globals.Bot_GetItemID = Bot_GetItemID;
	globals.Bot_UseItem = Bot_UseItem;
	globals.Entity_ForceLookAtPoint = Entity_ForceLookAtPoint;
	globals.Bot_PickedUpItem = Bot_PickedUpItem;

	globals.Entity_IsVisibleToPlayer = Entity_IsVisibleToPlayer;
	globals.GetShadowLightData = GetShadowLightData;

	globals.gentity_size = sizeof(gentity_t);

	return &globals;
}

//======================================================================

/*
=================
ClientEndServerFrames
=================
*/
static void ClientEndServerFrames() {
	// calc the player views now that all pushing
	// and damage has been added
	for (auto ec : active_clients())
		ClientEndServerFrame(ec);
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
gentity_t *CreateTargetChangeLevel(const char *map) {
	gentity_t *ent;

	ent = Spawn();
	ent->className = "target_changelevel";
	Q_strlcpy(level.nextmap, map, sizeof(level.nextmap));
	ent->map = level.nextmap;
	return ent;
}

inline std::vector<std::string> str_split(const std::string_view &str, char by) {
	std::vector<std::string> out;
	size_t start, end = 0;

	while ((start = str.find_first_not_of(by, end)) != std::string_view::npos) {
		end = str.find(by, start);
		out.push_back(std::string{ str.substr(start, end - start) });
	}

	return out;
}

// In g_main.cpp, near where you transition to intermission (e.g. in G_RunFrame or Entities_Reset)
#include <cmath>
#include <vector>
#include <algorithm>

constexpr float SKILL_K = 32.0f;

// helper to get all playing clients
static std::vector<gentity_t *> GetPlayers() {
	std::vector<gentity_t *> out;
	for (auto ent : active_clients()) {
		if (ClientIsPlaying(ent->client))
			out.push_back(ent);
	}
	return out;
}

// calculate Elo expectation
static float EloExpected(float ra, float rb) {
	return 1.0f / (1.0f + std::pow(10.0f, (rb - ra) / 400.0f));
}

static void G_AdjustSkillRatings() {
	auto players = GetPlayers();
	if (players.empty()) return;

	// Special case: Duel (1v1)
	if (GT(GT_DUEL) && players.size() == 2) {
		auto *a = players[0], *b = players[1];
		float Ra = a->client->sess.skillRating;
		float Rb = b->client->sess.skillRating;
		// assume client->resp.score holds who got the frag first / higher
		bool aWon = a->client->resp.score > b->client->resp.score;
		float Ea = EloExpected(Ra, Rb);
		float Eb = 1.0f - Ea;
		a->client->sess.skillRating = Ra + SKILL_K * ((aWon ? 1.0f : 0.0f) - Ea);
		b->client->sess.skillRating = Rb + SKILL_K * ((aWon ? 0.0f : 1.0f) - Eb);
		return;
	}

	// Team game: TDM or CTF (two teams)
	if ((GT(GT_TDM) || GT(GT_CTF)) && players.size() >= 2) {
		std::vector<gentity_t *> red, blue;
		for (auto *ent : players) {
			if (ent->client->sess.team == TEAM_RED) red.push_back(ent);
			else if (ent->client->sess.team == TEAM_BLUE) blue.push_back(ent);
		}
		if (red.empty() || blue.empty()) return;
		// compute average team ratings
		auto avg = [](const std::vector<gentity_t *> &v) {
			float sum = 0;
			for (auto *e : v) sum += e->client->sess.skillRating;
			return sum / v.size();
			};
		float Rr = avg(red), Rb = avg(blue);
		float Er = EloExpected(Rr, Rb), Eb = 1.0f - Er;
		// team scores: sum of resp.score (frags or caps)
		int Sr = 0, Sb = 0;
		for (auto *e : red)   Sr += e->client->resp.score;
		for (auto *e : blue)  Sb += e->client->resp.score;
		bool redWin = Sr > Sb;
		// update each player
		for (auto *e : red) {
			float S = redWin ? 1.0f : 0.0f;
			e->client->sess.skillRating += SKILL_K * (S - Er);
		}
		for (auto *e : blue) {
			float S = redWin ? 0.0f : 1.0f;
			e->client->sess.skillRating += SKILL_K * (S - Eb);
		}
		return;
	}

	// FFA / Deathmatch: n‑player free‑for‑all
	{
		int n = players.size();
		// sort by score descending
		std::sort(players.begin(), players.end(),
			[](gentity_t *a, gentity_t *b) {
				return a->client->resp.score > b->client->resp.score;
			});
		// extract ratings and scores
		std::vector<float> R(n), S(n);
		for (int i = 0; i < n; i++) {
			R[i] = players[i]->client->sess.skillRating;
			// actual score: normalized rank: 1.0 for 1st, 0.0 for last
			S[i] = 1.0f - float(i) / float(n - 1);
		}
		// compute expected performance for each
		std::vector<float> E(n, 0.0f);
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				if (i == j) continue;
				E[i] += EloExpected(R[i], R[j]);
			}
			E[i] /= float(n - 1);
		}
		// apply updates
		for (int i = 0; i < n; i++) {
			float delta = SKILL_K * (S[i] - E[i]);
			players[i]->client->sess.skillRating += delta;
		}
	}
}


/*
=================
Match_End

An end of match condition has been reached
=================
*/
extern void MatchStats_End();
void Match_End() {
	gentity_t *ent;

	G_AdjustSkillRatings();
	MatchStats_End();

	level.match_state = MatchState::MATCH_ENDED;
	level.match_state_timer = 0_sec;

	// see if there is a queued map to go to
	if (MQ_Count()) {
		BeginIntermission(CreateTargetChangeLevel(MQ_Go_Next()));
		return;
	}
	
	// stay on same level flag
	if (g_dm_same_level->integer) {
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	if (*level.forceMap) {
		BeginIntermission(CreateTargetChangeLevel(level.forceMap));
		return;
	}

	// see if it's in the map list
	if (*g_map_list->string) {
		const char *str = g_map_list->string;
		char first_map[MAX_QPATH]{ 0 };
		char *map;

		while (1) {
			map = COM_ParseEx(&str, " ");

			if (!*map)
				break;

			if (Q_strcasecmp(map, level.mapname) == 0) {
				// it's in the list, go to the next one
				map = COM_ParseEx(&str, " ");
				if (!*map) {
					// end of list, go to first one
					if (!first_map[0]) // there isn't a first one, same level
					{
						BeginIntermission(CreateTargetChangeLevel(level.mapname));
						return;
					} else {
						// [Paril-KEX] re-shuffle if necessary
						if (g_map_list_shuffle->integer) {
							auto values = str_split(g_map_list->string, ' ');

							if (values.size() == 1) {
								// meh
								BeginIntermission(CreateTargetChangeLevel(level.mapname));
								return;
							}

							std::shuffle(values.begin(), values.end(), mt_rand);

							// if the current map is the map at the front, push it to the end
							if (values[0] == level.mapname)
								std::swap(values[0], values[values.size() - 1]);

							gi.cvar_forceset("g_map_list", fmt::format("{}", join_strings(values, " ")).data());

							BeginIntermission(CreateTargetChangeLevel(values[0].c_str()));
							return;
						}

						BeginIntermission(CreateTargetChangeLevel(first_map));
						return;
					}
				} else {
					BeginIntermission(CreateTargetChangeLevel(map));
					return;
				}
			}
			if (!first_map[0])
				Q_strlcpy(first_map, map, sizeof(first_map));
		}
	}

	if (level.nextmap[0]) // go to a specific map
	{
		BeginIntermission(CreateTargetChangeLevel(level.nextmap));
		return;
	}

	// search for a changelevel
	ent = G_FindByString<&gentity_t::className>(nullptr, "target_changelevel");

	if (!ent) { // the map designer didn't include a changelevel,
		// so create a fake ent that goes back to the same level
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	BeginIntermission(ent);
}

/*
=================
CheckNeedPass
=================
*/
static void CheckNeedPass() {
	int need;
	static int32_t password_modified, spectator_password_modified;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (Cvar_WasModified(password, password_modified) || Cvar_WasModified(spectator_password, spectator_password_modified)) {
		need = 0;

		if (*password->string && Q_strcasecmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_strcasecmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", G_Fmt("{}", need).data());
	}
}

void QueueIntermission(const char *msg, bool boo, bool reset) {
	if (level.intermissionQueued || level.match_state < MatchState::MATCH_IN_PROGRESS)
		return;

	Q_strlcpy(level.intermission_victor_msg, msg, sizeof(level.intermission_victor_msg));

	//gi.LocBroadcast_Print(PRINT_CHAT, "MATCH END: {}\n", level.intermission_victor_msg[0] ? level.intermission_victor_msg : "Unknown Reason");
	gi.Com_PrintFmt("MATCH END: {}\n", level.intermission_victor_msg[0] ? level.intermission_victor_msg : "Unknown Reason");
	gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex(boo ? "insane/insane4.wav" : "world/xian1.wav"), 1, ATTN_NONE, 0);

	if (reset) {
		Match_Reset();
	} else {
		level.match_state = MatchState::MATCH_ENDED;
		level.match_state_timer = 0_sec;
		level.matchEndTime = level.time;
		level.intermissionQueued = level.time;

		gi.configstring(CS_CDTRACK, "0");
	}
}

/*
============
Teams_CalcRankings

End game rankings
============
*/
void Teams_CalcRankings(std::array<uint32_t, MAX_CLIENTS> &player_ranks) {
	if (!Teams())
		return;

	// we're all winners.. or losers. whatever
	if (level.team_scores[TEAM_RED] == level.team_scores[TEAM_BLUE]) {
		player_ranks.fill(1);
		return;
	}

	team_t winning_team = (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE]) ? TEAM_RED : TEAM_BLUE;

	for (auto player : active_clients())
		if (player->client->pers.spawned && ClientIsPlaying(player->client))
			player_ranks[player->s.number - 1] = player->client->sess.team == winning_team ? 1 : 2;
}

/*
=============
BeginIntermission
=============
*/
extern void Gauntlet_MatchEnd_AdjustScores();
void BeginIntermission(gentity_t *targ) {
	if (level.intermissionTime)
		return; // already activated

	// if in a duel, change the wins / losses
	Gauntlet_MatchEnd_AdjustScores();

	game.autosaved = false;

	level.intermissionTime = level.time;

	// respawn any dead clients
	for (auto ec : active_clients()) {
		if (ec->health <= 0 || ec->client->eliminated) {
			ec->health = 1;
			// give us our max health back since it will reset
			// to pers.health; in instanced items we'd lose the items
			// we touched so we always want to respawn with our max.
			if (P_UseCoopInstancedItems())
				ec->client->pers.health = ec->client->pers.max_health = ec->max_health;

			ClientRespawn(ec);
		}
	}

	level.intermission_server_frame = gi.ServerFrame();
	level.changeMap = targ->map;
	level.intermissionClear = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY);
	level.intermissionEOU = false;
	level.intermissionFade = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_FADE_OUT);

	// destroy all player trails
	PlayerTrail_Destroy(nullptr);

	// [Paril-KEX] update game level entry
	G_UpdateLevelEntry();

	if (strstr(level.changeMap, "*")) {
		if (coop->integer) {
			for (auto ec : active_clients()) {
				// strip players of all keys between units
				for (uint8_t n = 0; n < IT_TOTAL; n++)
					if (itemList[n].flags & IF_KEY)
						ec->client->pers.inventory[n] = 0;
			}
		}

		if (level.achievement && level.achievement[0]) {
			gi.WriteByte(svc_achievement);
			gi.WriteString(level.achievement);
			gi.multicast(vec3_origin, MULTICAST_ALL, true);
		}

		level.intermissionEOU = true;

		// "no end of unit" maps handle intermission differently
		if (!targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT))
			G_EndOfUnitMessage();
		else if (targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE) && !deathmatch->integer) {
			// Need to call this now
			G_ReportMatchDetails(true);
			level.intermissionPreExit = true; // go immediately to the next level
			return;
		}
	} else {
		if (!deathmatch->integer) {
			level.intermissionPreExit = true; // go immediately to the next level
			return;
		}
	}

	// Call while intermission is running
	G_ReportMatchDetails(true);

	level.intermissionPreExit = false;

	//SetIntermissionPoint();

	// move all clients to the intermission point
	for (auto ec : active_clients()) {
		MoveClientToIntermission(ec);
		if (Teams())
			AnnouncerSound(ec, level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE] ? "red_wins" : "blue_wins");
		else {
			if (ClientIsPlaying(ec->client))
				AnnouncerSound(ec, ec->client->resp.rank == 0 ? "you_win" : "you_lose");
		}
	}
}

/*
=============
ExitLevel
=============
*/
extern void Gauntlet_RemoveLoser();

void ExitLevel() {
	if (deathmatch->integer && g_dm_intermission_shots->integer && level.num_playing_human_clients > 0) {
		struct tm *ltime;
		time_t gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		time(&gmtime);
		ltime = localtime(&gmtime);

		const char *s = "";

		if (GT(GT_DUEL)) {
			gentity_t *e1 = &g_entities[level.sorted_clients[0] + 1];
			gentity_t *e2 = &g_entities[level.sorted_clients[1] + 1];
			const char *n1 = e1 ? e1->client->sess.netName : "";
			const char *n2 = e2 ? e2->client->sess.netName : "";

			s = G_Fmt("screenshot {}-vs-{}-{}-{}_{:02}_{:02}-{:02}_{:02}_{:02}\n",
				n1, n2, level.mapname, 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec).data();
			gi.Com_Print(s);
		} else {
			gentity_t *ent = &g_entities[1];
			const char *name = ent->client->followTarget ? ent->client->followTarget->client->sess.netName : ent->client->sess.netName;

			s = G_Fmt("screenshot {}-{}-{}-{}_{:02}_{:02}-{:02}_{:02}_{:02}\n", GametypeIndexToString((gametype_t)g_gametype->integer).c_str(),
				name, level.mapname, 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec).data();
		}
		gi.AddCommandString(s);
	}

	// [Paril-KEX] N64 fade
	if (level.intermissionFade) {
		level.intermissionFadeTime = level.time + 1.3_sec;
		level.intermissionFading = true;
		return;
	}

	ClientEndServerFrames();

	level.intermissionTime = 0_ms;

	// if we are running a gauntlet, kick the loser to queue,
	// which will automatically grab the next queued player and restart
	if (deathmatch->integer) {
		if (GT(GT_GAUNTLET))
			Gauntlet_RemoveLoser();
	} else {
		// [Paril-KEX] support for intermission completely wiping players
		// back to default stuff
		if (level.intermissionClear) {
			level.intermissionClear = false;

			for (auto ec : active_clients()) {
				// [Kex] Maintain user info to keep the player skin. 
				char userInfo[MAX_INFO_STRING];
				memcpy(userInfo, ec->client->pers.userInfo, sizeof(userInfo));

				ec->client->pers = ec->client->resp.coop_respawn = {};
				ec->health = 0; // this should trip the power armor, etc to reset as well

				memcpy(ec->client->pers.userInfo, userInfo, sizeof(userInfo));
				memcpy(ec->client->resp.coop_respawn.userInfo, userInfo, sizeof(userInfo));
			}
		}

		// [Paril-KEX] end of unit, so clear level trackers
		if (level.intermissionEOU) {
			game.level_entries = {};

			// give all players their lives back
			if (g_coop_enable_lives->integer)
				for (auto player : active_clients())
					player->client->pers.lives = g_coop_num_lives->integer + 1;
		}
	}

	if (level.changeMap == nullptr) {
		gi.Com_Error("Got null changeMap when trying to exit level. Was a trigger_changelevel configured correctly?");
		return;
	}

	// for N64 mainly, but if we're directly changing to "victorXXX.pcx" then
	// end game
	size_t start_offset = (level.changeMap[0] == '*' ? 1 : 0);

	if (deathmatch->integer && GT(GT_RR) && level.num_playing_clients > 1 && (!level.num_playing_red || !level.num_playing_blue))
		TeamShuffle();

	if (!deathmatch->integer && strlen(level.changeMap) > (6 + start_offset) &&
		!Q_strncasecmp(level.changeMap + start_offset, "victor", 6) &&
		!Q_strncasecmp(level.changeMap + strlen(level.changeMap) - 4, ".pcx", 4))
		gi.AddCommandString(G_Fmt("endgame \"{}\"\n", level.changeMap + start_offset).data());
	else
		gi.AddCommandString(G_Fmt("gamemap \"{}\"\n", level.changeMap).data());

	level.changeMap = nullptr;
}

/*
=============
FinalizeMapVote
=============
*/
void FinalizeMapVote() {
	int tally[3] = { 0 };

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		int v = level.mapVote.votes[i];
		if (v >= 0 && v < level.mapVote.candidates.size())
			tally[v]++;
	}

	int best = 0;
	for (int i = 1; i < level.mapVote.candidates.size(); ++i) {
		if (tally[i] > tally[best])
			best = i;
	}

	level.changeMap = level.mapVote.candidates[best]->filename.c_str();
}

/*
=============
PreExitLevel

Additional end of match goodness before actually changing level
=============
*/
void PreExitLevel() {
	if (!level.preExitTime) {
		level.preExitTime = level.time;
		return;
	}

	if (g_maps_selector->integer && game.mapSystem.playQueue.empty()) {
		if (level.mapVote.voteStartTime == 0_sec) {
			auto candidates = SelectVoteCandidates();
			if (!candidates.empty()) {
				memset(level.mapVoteCandidates, 0, sizeof(level.mapVoteCandidates));
				memset(level.mapVoteCounts, 0, sizeof(level.mapVoteCounts));
				std::fill_n(level.mapVoteByClient, MAX_CLIENTS, -1);

				for (size_t i = 0; i < candidates.size() && i < 3; ++i)
					level.mapVoteCandidates[i] = candidates[i];

				level.mapVoteStartTime = level.time;

				for (auto ec : active_players()) {
					G_Menu_ArenaSelector_Open(ec);
				}
			}
		}
	}

	if (level.time < level.preExitTime + 5_sec)
		return;

	FinalizeMapVote();

	ExitLevel();
}

/*
=============
CheckPowerups
=============
*/
static int powerup_minplayers_mod_count = -1;
static int numplayers_check = -1;

static void CheckPowerups() {
	bool docheck = false;

	if (powerup_minplayers_mod_count != g_dm_powerups_minplayers->integer) {
		powerup_minplayers_mod_count = g_dm_powerups_minplayers->integer;
		docheck = true;
	}

	if (numplayers_check != level.num_playing_clients) {
		numplayers_check = level.num_playing_clients;
		docheck = true;
	}

	if (!docheck)
		return;

	bool	disable = g_dm_powerups_minplayers->integer > 0 && (level.num_playing_clients < g_dm_powerups_minplayers->integer);
	gentity_t	*ent = nullptr;
	size_t	i;
	for (ent = g_entities + 1, i = 1; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse || !ent->item)
			continue;

		if (!(ent->item->flags & IF_POWERUP))
			continue;
		/*
		if (!(ent->svFlags & SVF_NOCLIENT))
			continue;
		*/
		if (g_quadhog->integer && ent->item->id == IT_POWERUP_QUAD)
			return;

		if (disable) {
			ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
			ent->s.effects |= EF_COLOR_SHELL;
		} else {
			ent->s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
			ent->s.effects &= ~EF_COLOR_SHELL;
		}
	}
}

/*
=============
CheckMinMaxPlayers
=============
*/
static int minplayers_mod_count = -1;
static int maxplayers_mod_count = -1;

static void CheckMinMaxPlayers() {

	if (!deathmatch->integer)
		return;

	if (minplayers_mod_count == minplayers->modified_count &&
			maxplayers_mod_count == maxplayers->modified_count)
		return;

	// set min/maxplayer limits
	if (minplayers->integer < 2) gi.cvar_set("minplayers", "2");
	else if (minplayers->integer > maxclients->integer) gi.cvar_set("minplayers", maxclients->string);
	if (maxplayers->integer < 0) gi.cvar_set("maxplayers", maxclients->string);
	if (maxplayers->integer > maxclients->integer) gi.cvar_set("maxplayers", maxclients->string);
	else if (maxplayers->integer < minplayers->integer) gi.cvar_set("maxplayers", minplayers->string);

	minplayers_mod_count = minplayers->modified_count;
	maxplayers_mod_count = maxplayers->modified_count;
}

static void CheckCvars() {
	if (Cvar_WasModified(g_airaccelerate, game.airacceleration_modified)) {
		// [Paril-KEX] air accel handled by game DLL now, and allow
		// it to be changed in sp/coop
		gi.configstring(CS_AIRACCEL, G_Fmt("{}", g_airaccelerate->integer).data());
		pm_config.airaccel = g_airaccelerate->integer;
	}

	if (Cvar_WasModified(g_gravity, game.gravity_modified))
		level.gravity = g_gravity->value;

	CheckMinMaxPlayers();
}

static bool G_AnyDeadPlayersWithoutLives() {
	for (auto player : active_clients())
		if (player->health <= 0 && (!player->client->pers.lives || player->client->eliminated))
			return true;

	return false;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
extern void AnnounceCountdown(int t, gtime_t &checkRef);
extern void CheckVote(void);
extern void CheckDMEndFrame();

static inline void G_RunFrame_(bool main_loop) {
	level.inFrame = true;

	if (level.timeoutActive > 0_ms && level.timeoutOwner) {
		int tick = level.timeoutActive.seconds<int>() + 1;
		AnnounceCountdown(tick, level.countdown_check);

		level.timeoutActive -= FRAME_TIME_MS;
		if (level.timeoutActive <= 0_ms) {
			TimeoutEnd();
		}

		ClientEndServerFrames();
		return;
	} else {
		// track gametype changes and update accordingly
		GT_Changes();

		// cancel vote if timed out
		CheckVote();

		// for tracking changes
		CheckCvars();

		CheckPowerups();

		CheckRuleset();

		Bot_UpdateDebug();

		level.time += FRAME_TIME_MS;

		if (!deathmatch->integer && level.intermissionFading) {
			if (level.intermissionFadeTime > level.time) {
				float alpha = clamp(1.0f - (level.intermissionFadeTime - level.time - 300_ms).seconds(), 0.f, 1.f);

				for (auto player : active_clients())
					player->client->ps.screen_blend = { 0, 0, 0, alpha };
			} else {
				level.intermissionFade = level.intermissionFading = false;
				ExitLevel();
			}

			level.inFrame = false;

			return;
		}

		// exit intermissions

		if (level.intermissionPreExit) {
			PreExitLevel();
			level.inFrame = false;
			return;
		}
	}

	if (!deathmatch->integer) {
		// reload the map start save if restart time is set (all players are dead)
		if (level.coopLevelRestartTime > 0_ms && level.time > level.coopLevelRestartTime) {
			ClientEndServerFrames();
			gi.AddCommandString("restart_level\n");
		}

		// clear client coop respawn states; this is done
		// early since it may be set multiple times for different
		// players
		if (CooperativeModeOn() && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
			for (auto player : active_clients()) {
				if (player->client->respawn_time >= level.time)
					player->client->coop_respawn_state = COOP_RESPAWN_WAITING;
				else if (g_coop_enable_lives->integer && player->health <= 0 && player->client->pers.lives == 0)
					player->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
				else if (g_coop_enable_lives->integer && G_AnyDeadPlayersWithoutLives())
					player->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
				else
					player->client->coop_respawn_state = COOP_RESPAWN_NONE;
			}
		}
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	gentity_t *ent = &g_entities[0];
	for (size_t i = 0; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse) {
			// defer removing client info so that disconnected, etc works
			if (i > 0 && i <= game.maxclients) {
				if (ent->timeStamp && level.time < ent->timeStamp) {
					int32_t playernum = ent - g_entities - 1;
					gi.configstring(CS_PLAYERSKINS + playernum, "");
					ent->timeStamp = 0_ms;
				}
			}
			continue;
		}

		level.currentEntity = ent;

		// Paril: RF_BEAM entities update their old_origin by hand.
		if (!(ent->s.renderfx & RF_BEAM))
			ent->s.old_origin = ent->s.origin;

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundEntity) && (ent->groundEntity->linkcount != ent->groundEntity_linkCount)) {
			contents_t mask = G_GetClipMask(ent);

			if (!(ent->flags & (FL_SWIM | FL_FLY)) && (ent->svFlags & SVF_MONSTER)) {
				ent->groundEntity = nullptr;
				M_CheckGround(ent, mask);
			} else {
				// if it's still 1 point below us, we're good
				trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin + ent->gravityVector, ent,
					mask);

				if (tr.startsolid || tr.allsolid || tr.ent != ent->groundEntity)
					ent->groundEntity = nullptr;
				else
					ent->groundEntity_linkCount = ent->groundEntity->linkcount;
			}
		}

		Entity_UpdateState(ent);

		if (i > 0 && i <= game.maxclients) {
			ClientBeginServerFrame(ent);
			continue;
		}

		G_RunEntity(ent);
	}

	CheckDMEndFrame();

	// see if needpass needs updated
	CheckNeedPass();

	if (CooperativeModeOn() && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
		// rarely, we can see a flash of text if all players respawned
		// on some other player, so if everybody is now alive we'll reset
		// back to empty
		bool reset_coop_respawn = true;

		for (auto player : active_clients()) {
			if (player->health > 0) {	//muff: changed from >= to >
				reset_coop_respawn = false;
				break;
			}
		}

		if (reset_coop_respawn) {
			for (auto player : active_clients())
				player->client->coop_respawn_state = COOP_RESPAWN_NONE;
		}
	}

	// build the playerstate_t structures for all players
	ClientEndServerFrames();

	// [Paril-KEX] if not in intermission and player 1 is loaded in
	// the game as an entity, increase timer on current entry
	if (level.entry && !level.intermissionTime && g_entities[1].inUse && g_entities[1].client->pers.connected)
		level.entry->time += FRAME_TIME_S;

	// [Paril-KEX] run monster pains now
	for (size_t i = 0; i < globals.num_entities + 1 + game.maxclients + BODY_QUEUE_SIZE; i++) {
		gentity_t *e = &g_entities[i];

		if (!e->inUse || !(e->svFlags & SVF_MONSTER))
			continue;

		M_ProcessPain(e);
	}

	level.inFrame = false;
}

static inline bool G_AnyClientsSpawned() {
	for (auto player : active_clients())
		if (player->client && player->client->pers.spawned)
			return true;

	return false;
}

void G_RunFrame(bool main_loop) {
	if (main_loop && !G_AnyClientsSpawned())
		return;

	for (size_t i = 0; i < g_frames_per_frame->integer; i++)
		G_RunFrame_(main_loop);

	// match details.. only bother if there's at least 1 player in-game
	// and not already end of game
	if (G_AnyClientsSpawned() && !level.intermissionTime) {
		constexpr gtime_t report_time = 45_sec;

		if (level.time - level.next_match_report > report_time) {
			level.next_match_report = level.time + report_time;
			G_ReportMatchDetails(false);
		}
	}
}

/*
================
G_PrepFrame

This has to be done before the world logic, because
player processing happens outside RunFrame
================
*/
void G_PrepFrame() {
	for (size_t i = 0; i < globals.num_entities; i++)
		g_entities[i].s.event = EV_NONE;

	for (auto player : active_clients())
		player->client->ps.stats[STAT_HIT_MARKER] = 0;

	globals.server_flags &= ~SERVER_FLAG_INTERMISSION;

	if (level.intermissionTime) {
		globals.server_flags |= SERVER_FLAG_INTERMISSION;
	}
}
