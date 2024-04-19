// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "bots/bot_includes.h"
#include "monsters/m_player.h"	// match starts

CHECK_GCLIENT_INTEGRITY;
CHECK_EDICT_INTEGRITY;

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

edict_t *g_edicts;

cvar_t *hostname;

cvar_t *deathmatch;
cvar_t *duel;
cvar_t *ctf;
cvar_t *teamplay;
cvar_t *freeze;
cvar_t *clanarena;
cvar_t *coop;
cvar_t *horde;

cvar_t *skill;
cvar_t *fraglimit;
cvar_t *capturelimit;
cvar_t *timelimit;
cvar_t *roundlimit;
cvar_t *roundtimelimit;
cvar_t *g_quick_weapon_switch;
cvar_t *g_instant_weapon_switch;
cvar_t *password;
cvar_t *spectator_password;
cvar_t *needpass;
static cvar_t *maxclients;
cvar_t *maxspectators;
static cvar_t *maxentities;
cvar_t *g_select_empty;
cvar_t *g_dedicated;

cvar_t *filterban;

cvar_t *g_maxvelocity;
cvar_t *g_gravity;

cvar_t *g_dm_force_join;
cvar_t *g_teamplay_allow_team_pick;
cvar_t *g_teamplay_force_balance;
cvar_t *g_voting_percentage;

cvar_t *g_skip_view_modifiers;

cvar_t *g_rollspeed;
cvar_t *g_rollangle;
cvar_t *gun_x;
cvar_t *gun_y;
cvar_t *gun_z;

cvar_t *run_pitch;
cvar_t *run_roll;
cvar_t *bob_up;
cvar_t *bob_pitch;
cvar_t *bob_roll;

cvar_t *g_cheats;

cvar_t *g_debug_monster_paths;
cvar_t *g_debug_monster_kills;

cvar_t *bot_debug_follow_actor;
cvar_t *bot_debug_move_to_point;

cvar_t *flood_msgs;
cvar_t *flood_persecond;
cvar_t *flood_waitdelay;

cvar_t *g_stopspeed; // PGM	 (this was a define in g_phys.c)

cvar_t *g_strict_saves;

cvar_t *g_huntercam;
cvar_t *g_dm_strong_mines;
cvar_t *g_dm_random_items;

cvar_t *g_instagib;
cvar_t *g_instagib_splash;
cvar_t *g_quadhog;
cvar_t *g_nadefest;
cvar_t *g_frenzy;
cvar_t *g_coop_player_collision;
cvar_t *g_coop_squad_respawn;
cvar_t *g_coop_enable_lives;
cvar_t *g_coop_num_lives;
cvar_t *g_coop_instanced_items;
cvar_t *g_allow_grapple;
cvar_t *g_grapple_offhand;
cvar_t *g_grapple_fly_speed;
cvar_t *g_grapple_pull_speed;
cvar_t *g_grapple_damage;
cvar_t *g_coop_health_scaling;
cvar_t *g_weapon_respawn_time;

cvar_t *g_frag_messages;

// dm"flags"
cvar_t *g_no_health;
cvar_t *g_no_items;
cvar_t *g_no_powerups;
cvar_t *g_dm_weapons_stay;
cvar_t *g_dm_no_fall_damage;
cvar_t *g_dm_no_self_damage;
cvar_t *g_dm_instant_items;
cvar_t *g_dm_same_level;
cvar_t *g_friendly_fire;
cvar_t *g_dm_force_respawn;
cvar_t *g_dm_force_respawn_time;
cvar_t *g_dm_respawn_delay_min;
cvar_t *g_dm_spawn_farthest;
cvar_t *g_no_armor;
cvar_t *g_dm_allow_exit;
cvar_t *g_infinite_ammo;
cvar_t *g_dm_no_quad_drop;
cvar_t *g_dm_no_quadfire_drop;
cvar_t *g_dm_powerup_drop;
cvar_t *g_no_mines;
cvar_t *g_dm_no_stack_double;
cvar_t *g_no_nukes;
cvar_t *g_no_spheres;
cvar_t *g_teamplay_armor_protect;
cvar_t *g_allow_techs;
cvar_t *g_dm_powerups_style;
cvar_t *g_corpse_sink_time;
cvar_t *g_start_items;
cvar_t *g_map_list;
cvar_t *g_map_list_shuffle;
cvar_t *g_lag_compensation;
cvar_t *g_dm_respawn_point_min_dist;
cvar_t *g_dm_respawn_point_min_dist_debug;

cvar_t *g_motd;

cvar_t *g_inactivity;
cvar_t *g_entity_override_load;
cvar_t *g_entity_override_save;

cvar_t *g_airaccelerate;
cvar_t *g_knockback_scale;
cvar_t *g_frozen_time;
cvar_t *g_damage_scale;
cvar_t *g_vampiric_damage;
cvar_t *g_vampiric_health_max;
cvar_t *g_disable_player_collision;
cvar_t *ai_damage_scale;
cvar_t *ai_model_scale;
cvar_t *ai_allow_dm_spawn;
cvar_t *ai_movement_disabled;

cvar_t *minplayers;
cvar_t *maxplayers;

cvar_t *g_mover_speed_scale;
cvar_t *g_mover_debug;

cvar_t *g_warmup_countdown;
cvar_t *g_warmup_ready_percentage;

//ctf
cvar_t *competition;
cvar_t *g_match_lock;
cvar_t *matchsetuptime;
cvar_t *matchstarttime;
cvar_t *admin_password;
cvar_t *g_allow_admin;
cvar_t *warn_unbalanced;
//-ctf
cvar_t *g_eyecam;
cvar_t *g_teleporter_nofreeze;

cvar_t *g_showhelp;

cvar_t *g_matchstats;

cvar_t *g_dm_spawnpads;

cvar_t *g_expert;

cvar_t *g_item_bobbing;
cvar_t *g_weapon_force_central_projection;

cvar_t *g_allow_custom_skins;

cvar_t *g_allow_voting;
cvar_t *g_allow_vote_midgame;
cvar_t *g_allow_spec_vote;
cvar_t *g_vote_limit;
cvar_t *g_vote_flags;

cvar_t *g_dm_auto_join;

cvar_t *g_dm_overtime;

cvar_t *g_dm_do_warmup;
cvar_t *g_dm_do_readyup;

cvar_t *g_item_chain_random;

cvar_t *g_allow_forfeit;

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

void SpawnEntities(const char *mapname, const char *entities, const char *spawnpoint);
void ClientThink(edict_t *ent, usercmd_t *cmd);
edict_t *ClientChooseSlot(const char *userinfo, const char *social_id, bool is_bot, edict_t **ignore, size_t num_ignore, bool cinematic);
bool ClientConnect(edict_t *ent, char *userinfo, const char *social_id, bool is_bot);
char *WriteGameJson(bool autosave, size_t *out_size);
void ReadGameJson(const char *jsonString);
char *WriteLevelJson(bool transition, size_t *out_size);
void ReadLevelJson(const char *jsonString);
bool G_CanSave();
void ClientDisconnect(edict_t *ent);
void ClientBegin(edict_t *ent);
void ClientCommand(edict_t *ent);
void G_RunFrame(bool main_loop);
void G_PrepFrame();
void InitSave();

#include <chrono>

static void Horde_MonsterSpawn(const char *classname) {
	edict_t *spawn;
	edict_t *point;

	point = SelectDeathmatchSpawnPoint(vec3_origin, true, true, true, false).spot;

	spawn = G_Spawn();
	spawn->classname = "monster_berserk";

	spawn->s.origin = point->s.origin;
	//spawn->s.origin[2] += 32;
	spawn->s.angles[YAW] = point->s.angles[YAW];

	st = {};

	ED_CallSpawn(spawn);
	gi.linkentity(spawn);

	KillBox(spawn, false);

	spawn->s.renderfx |= RF_IR_VISIBLE;
}

void Horde_CreateWave() {
	/*
	const char *list[8] = {
		"monster_berserk",
		"monster_gladiator",
		"monster_gunner",
		"monster_infantry",
		"monster_soldier_light",
		"monster_soldier",
		"monster_soldier_ss",
		"monster_tank",
	};
	for (size_t i = 0; i < 8; i++) {
		Horde_MonsterSpawn(list[i]);
	}
	*/
	if (!horde->integer)
		return;

	edict_t *ent;
	select_spawn_result_t result = SelectDeathmatchSpawnPoint(vec3_origin, true, true, true, false);

	gi.Com_Print("CREATEWAVE\n");
	if (!result.any_valid) {
		gi.Com_Print("NO VALID SPAWN POINT FOUND\n");
		return;
	}

	ent = G_Spawn();
	ent->classname = "monster_soldier";

	ent->s.origin = result.spot->s.origin + vec3_t{ 0, 0, 9 };
	ent->s.angles[YAW] = result.spot->s.angles[YAW];

	st = {};

	ED_CallSpawn(ent);
	gi.linkentity(ent);

	KillBox(ent, false);

	ent->s.renderfx |= RF_IR_VISIBLE;
}

static void Horde_InitFirstWave() {
	gi.Com_Print("Horde_InitFirstWave 0\n");
	if (!horde->integer)
		return;
	gi.Com_Print("Horde_InitFirstWave 1\n");

	Horde_CreateWave();
}

void GT_Change(int gt) {
	if (gt < 0)
		return;

	if (gt >= GT_NUM_GAMETYPES)
		return;

	if (coop->integer)
		gi.cvar_forceset("coop", "0");

	if (!deathmatch->integer)
		gi.cvar_forceset("deathmatch", "1");
	
	// action the cvar changes
	for (size_t i = 0; i < GT_NUM_GAMETYPES; i++) {
		gi.cvar_forceset(gt_cvar[gt], i == gt ? "1" : "0");
	}

	// set pre-adjustments
	switch (gt) {
	case GT_HORDE:
		break;
	case GT_FFA:
		break;
	case GT_DUEL:
		break;
	case GT_TDM:
		break;
	case GT_CTF:
		break;
	case GT_CA:
		break;
	case GT_FREEZE:
		break;
	default:
		break;
	}

	// restart the match


	// announce it
	gi.LocBroadcast_Print(PRINT_CENTER, "{}", gt_long_name[gt]);
}

static void G_InitGametype() {
	constexpr const char *COOP = "coop";

	bool force_dm = false;

	if (freeze->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
		// force ctf off
		if (ctf->integer)
			gi.cvar_set(gt_cvar[GT_CTF], "0");
		// force ca off
		if (clanarena->integer)
			gi.cvar_set(gt_cvar[GT_CA], "0");
		// force tdm off
		if (teamplay->integer)
			gi.cvar_set(gt_cvar[GT_TDM], "0");
		// force duel off
		if (duel->integer)
			gi.cvar_set(gt_cvar[GT_DUEL], "0");
		// force horde off
		if (horde->integer)
			gi.cvar_set(gt_cvar[GT_HORDE], "0");
	}
	if (clanarena->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
		// force ctf off
		if (ctf->integer)
			gi.cvar_set(gt_cvar[GT_CTF], "0");
		// force tdm off
		if (teamplay->integer)
			gi.cvar_set(gt_cvar[GT_TDM], "0");
		// force duel off
		if (duel->integer)
			gi.cvar_set(gt_cvar[GT_DUEL], "0");
		// force horde off
		if (horde->integer)
			gi.cvar_set(gt_cvar[GT_HORDE], "0");
	}
	if (ctf->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
		// force tdm off
		if (teamplay->integer)
			gi.cvar_set(gt_cvar[GT_TDM], "0");
		// force duel off
		if (duel->integer)
			gi.cvar_set(gt_cvar[GT_DUEL], "0");
		// force horde off
		if (horde->integer)
			gi.cvar_set(gt_cvar[GT_HORDE], "0");
	}
	if (teamplay->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
		// force duel off
		if (duel->integer)
			gi.cvar_set(gt_cvar[GT_DUEL], "0");
		// force horde off
		if (horde->integer)
			gi.cvar_set(gt_cvar[GT_HORDE], "0");
	}
	if (duel->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
		// force horde off
		if (horde->integer)
			gi.cvar_set(gt_cvar[GT_HORDE], "0");
	}
	if (horde->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
	}

	if (force_dm) {
		gi.Com_PrintFmt("Forcing {}.\n", gt_cvar[GT_FFA]);
		gi.cvar_set(gt_cvar[GT_FFA], "1");
	} else if (deathmatch->integer) {
	}

	// force even maxplayers value during teamplay
	if (Teams()) {
		int pmax = maxplayers->integer;

		if (pmax != floor(pmax / 2))
			gi.cvar_set("maxplayers", G_Fmt("{}", floor(pmax / 2) * 2).data());
	}
}

/*
============
PreInitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
static void PreInitGame() {
	maxclients = gi.cvar("maxclients", G_Fmt("{}", MAX_SPLIT_PLAYERS).data(), CVAR_SERVERINFO | CVAR_LATCH);
	minplayers = gi.cvar("minplayers", "2", CVAR_NOFLAGS);
	maxplayers = gi.cvar("maxplayers", "16", CVAR_NOFLAGS);

	deathmatch = gi.cvar("deathmatch", "0", CVAR_LATCH);
	duel = gi.cvar("duel", "0", CVAR_LATCH);
	ctf = gi.cvar("ctf", "0", CVAR_SERVERINFO | CVAR_LATCH);
	teamplay = gi.cvar("teamplay", "0", CVAR_LATCH);
	freeze = gi.cvar("freeze", "0", CVAR_SERVERINFO | CVAR_LATCH);
	clanarena = gi.cvar("clanarena", "0", CVAR_SERVERINFO | CVAR_LATCH);
	coop = gi.cvar("coop", "0", CVAR_LATCH);
	horde = gi.cvar("horde", "0", CVAR_LATCH);

	//gi.AddCommandString("cl_notifytime -1\n");

	G_InitGametype();
}

/*
============
InitGame

Called after PreInitGame when the game has set up cvars.
============
*/
static void InitGame() {
	gi.Com_Print("==== InitGame ====\n");

	InitSave();

	// seed RNG
	mt_rand.seed((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());

	hostname = gi.cvar("hostname", "Welcome to Muff Mode!", CVAR_NOFLAGS);

	gun_x = gi.cvar("gun_x", "0", CVAR_NOFLAGS);
	gun_y = gi.cvar("gun_y", "0", CVAR_NOFLAGS);
	gun_z = gi.cvar("gun_z", "0", CVAR_NOFLAGS);

	g_rollspeed = gi.cvar("g_rollspeed", "200", CVAR_NOFLAGS);
	g_rollangle = gi.cvar("g_rollangle", "2", CVAR_NOFLAGS);
	g_maxvelocity = gi.cvar("g_maxvelocity", "2000", CVAR_NOFLAGS);
	g_gravity = gi.cvar("g_gravity", "800", CVAR_NOFLAGS);

	g_skip_view_modifiers = gi.cvar("g_skip_view_modifiers", "0", CVAR_NOSET);

	g_stopspeed = gi.cvar("g_stopspeed", "100", CVAR_NOFLAGS);

	g_huntercam = gi.cvar("g_huntercam", "1", CVAR_SERVERINFO | CVAR_LATCH);
	g_dm_strong_mines = gi.cvar("g_dm_strong_mines", "0", CVAR_NOFLAGS);
	g_dm_random_items = gi.cvar("g_dm_random_items", "0", CVAR_NOFLAGS);

	// game modifications
	g_instagib = gi.cvar("g_instagib", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_instagib_splash = gi.cvar("g_instagib_splash", "0", CVAR_NOFLAGS);
	g_quadhog = gi.cvar("g_quadhog", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_nadefest = gi.cvar("g_nadefest", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_frenzy = gi.cvar("g_frenzy", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_vampiric_damage = gi.cvar("g_vampiric_damage", "0", CVAR_NOFLAGS);
	g_vampiric_health_max = gi.cvar("g_vampiric_health_max", "500", CVAR_NOFLAGS);

	// freeze tag
	g_frozen_time = gi.cvar("g_frozen_time", "180", CVAR_NOFLAGS);

	// [Paril-KEX]
	g_coop_player_collision = gi.cvar("g_coop_player_collision", "0", CVAR_LATCH);
	g_coop_squad_respawn = gi.cvar("g_coop_squad_respawn", "1", CVAR_LATCH);
	g_coop_enable_lives = gi.cvar("g_coop_enable_lives", "0", CVAR_LATCH);
	g_coop_num_lives = gi.cvar("g_coop_num_lives", "2", CVAR_LATCH);
	g_coop_instanced_items = gi.cvar("g_coop_instanced_items", "1", CVAR_LATCH);
	g_allow_grapple = gi.cvar("g_allow_grapple", "auto", CVAR_NOFLAGS);
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
	gi.cvar("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);

	maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
	skill = gi.cvar("skill", "1", CVAR_LATCH);
	maxentities = gi.cvar("maxentities", G_Fmt("{}", MAX_EDICTS).data(), CVAR_LATCH);

	// change anytime vars
	fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
	roundlimit = gi.cvar("roundlimit", "8", CVAR_SERVERINFO);
	roundtimelimit = gi.cvar("roundtimelimit", "2", CVAR_SERVERINFO);
	capturelimit = gi.cvar("capturelimit", "0", CVAR_SERVERINFO);
	g_quick_weapon_switch = gi.cvar("g_quick_weapon_switch", "1", CVAR_LATCH);
	g_instant_weapon_switch = gi.cvar("g_instant_weapon_switch", "0", CVAR_LATCH);
	password = gi.cvar("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
	needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar("filterban", "1", CVAR_NOFLAGS);

	g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);

	run_pitch = gi.cvar("run_pitch", "0.002", CVAR_NOFLAGS);
	run_roll = gi.cvar("run_roll", "0.005", CVAR_NOFLAGS);
	bob_up = gi.cvar("bob_up", "0.005", CVAR_NOFLAGS);
	bob_pitch = gi.cvar("bob_pitch", "0.002", CVAR_NOFLAGS);
	bob_roll = gi.cvar("bob_roll", "0.002", CVAR_NOFLAGS);

	// flood control
	flood_msgs = gi.cvar("flood_msgs", "4", CVAR_NOFLAGS);
	flood_persecond = gi.cvar("flood_persecond", "4", CVAR_NOFLAGS);
	flood_waitdelay = gi.cvar("flood_waitdelay", "10", CVAR_NOFLAGS);

	g_strict_saves = gi.cvar("g_strict_saves", "1", CVAR_NOFLAGS);

	g_airaccelerate = gi.cvar("g_airaccelerate", "0", CVAR_NOFLAGS);

	g_knockback_scale = gi.cvar("g_knockback_scale", "1.0", CVAR_NOFLAGS);
	g_damage_scale = gi.cvar("g_damage_scale", "1", CVAR_NOFLAGS);
	g_disable_player_collision = gi.cvar("g_disable_player_collision", "0", CVAR_NOFLAGS);
	ai_damage_scale = gi.cvar("ai_damage_scale", "1", CVAR_NOFLAGS);
	ai_model_scale = gi.cvar("ai_model_scale", "0", CVAR_NOFLAGS);
	ai_allow_dm_spawn = gi.cvar("ai_allow_dm_spawn", "0", CVAR_NOFLAGS);
	ai_movement_disabled = gi.cvar("ai_movement_disabled", "0", CVAR_NOFLAGS);

	g_mover_speed_scale = gi.cvar("g_mover_speed_scale", "1.0f", CVAR_NOFLAGS);
	g_mover_debug = gi.cvar("g_mover_debug", "0", CVAR_NOFLAGS);

	g_warmup_countdown = gi.cvar("g_warmup_countdown", "10", CVAR_NOFLAGS);
	g_warmup_ready_percentage = gi.cvar("g_warmup_ready_percentage", "0.51f", CVAR_NOFLAGS);

	g_frames_per_frame = gi.cvar("g_frames_per_frame", "1", CVAR_NOFLAGS);

	g_coop_health_scaling = gi.cvar("g_coop_health_scaling", "0", CVAR_LATCH);
	g_weapon_respawn_time = gi.cvar("g_weapon_respawn_time", "30", CVAR_NOFLAGS);

	// dm "flags"
	g_no_health = gi.cvar("g_no_health", "0", CVAR_NOFLAGS);
	g_no_items = gi.cvar("g_no_items", "0", CVAR_NOFLAGS);
	g_no_powerups = gi.cvar("g_no_powerups", "0", CVAR_NOFLAGS);
	g_dm_weapons_stay = gi.cvar("g_dm_weapons_stay", "0", CVAR_NOFLAGS);
	g_dm_no_fall_damage = gi.cvar("g_dm_no_fall_damage", "0", CVAR_NOFLAGS);
	g_dm_no_self_damage = gi.cvar("g_dm_no_self_damage", "0", CVAR_NOFLAGS);
	g_dm_instant_items = gi.cvar("g_dm_instant_items", "1", CVAR_NOFLAGS);
	g_dm_same_level = gi.cvar("g_dm_same_level", "0", CVAR_NOFLAGS);
	g_friendly_fire = gi.cvar("g_friendly_fire", "0", CVAR_NOFLAGS);
	g_dm_force_respawn = gi.cvar("g_dm_force_respawn", "1", CVAR_NOFLAGS);
	g_dm_force_respawn_time = gi.cvar("g_dm_force_respawn_time", "3", CVAR_NOFLAGS);
	g_dm_respawn_delay_min = gi.cvar("g_dm_respawn_delay_min", "1", CVAR_NOFLAGS);
	g_dm_spawn_farthest = gi.cvar("g_dm_spawn_farthest", "1", CVAR_NOFLAGS);
	g_no_armor = gi.cvar("g_no_armor", "0", CVAR_NOFLAGS);
	g_dm_allow_exit = gi.cvar("g_dm_allow_exit", "0", CVAR_NOFLAGS);
	g_infinite_ammo = gi.cvar("g_infinite_ammo", "0", CVAR_LATCH);
	g_dm_no_quad_drop = gi.cvar("g_dm_no_quad_drop", "0", CVAR_NOFLAGS);
	g_dm_no_quadfire_drop = gi.cvar("g_dm_no_quadfire_drop", "0", CVAR_NOFLAGS);
	g_dm_powerup_drop = gi.cvar("g_dm_powerup_drop", "1", CVAR_NOFLAGS);
	g_no_mines = gi.cvar("g_no_mines", "0", CVAR_NOFLAGS);
	g_dm_no_stack_double = gi.cvar("g_dm_no_stack_double", "0", CVAR_NOFLAGS);
	g_no_nukes = gi.cvar("g_no_nukes", "0", CVAR_NOFLAGS);
	g_no_spheres = gi.cvar("g_no_spheres", "0", CVAR_NOFLAGS);
	g_dm_force_join = gi.cvar("g_dm_force_join", "0", CVAR_NOFLAGS);
	g_teamplay_allow_team_pick = gi.cvar("g_teamplay_allow_team_pick", "0", CVAR_NOFLAGS);
	g_teamplay_force_balance = gi.cvar("g_teamplay_force_balance", "0", CVAR_NOFLAGS);
	g_teamplay_armor_protect = gi.cvar("g_teamplay_armor_protect", "0", CVAR_NOFLAGS);
	g_allow_techs = gi.cvar("g_allow_techs", "auto", CVAR_NOFLAGS);
	g_dm_powerups_style = gi.cvar("g_dm_powerups_style", "1", CVAR_NOFLAGS);

	g_corpse_sink_time = gi.cvar("g_corpse_sink_time", "15", CVAR_NOFLAGS);

	g_motd = gi.cvar("g_motd", "", CVAR_NOFLAGS);

	g_dm_respawn_point_min_dist = gi.cvar("g_dm_respawn_point_min_dist", "256", CVAR_NOFLAGS);
	g_dm_respawn_point_min_dist_debug = gi.cvar("g_dm_respawn_point_min_dist_debug", "0", CVAR_NOFLAGS);

	g_start_items = gi.cvar("g_start_items", "", CVAR_LATCH);
	g_map_list = gi.cvar("g_map_list", "", CVAR_NOFLAGS);
	g_map_list_shuffle = gi.cvar("g_map_list_shuffle", "0", CVAR_NOFLAGS);
	g_lag_compensation = gi.cvar("g_lag_compensation", "1", CVAR_NOFLAGS);

	g_inactivity = gi.cvar("g_inactivity", "120", CVAR_NOFLAGS);
	g_entity_override_load = gi.cvar("g_entity_override_load", "1", CVAR_NOFLAGS);
	g_entity_override_save = gi.cvar("g_entity_override_save", "0", CVAR_NOFLAGS);

	//ctf
	competition = gi.cvar("competition", "0", CVAR_SERVERINFO);
	g_match_lock = gi.cvar("g_match_lock", "0", CVAR_SERVERINFO);
	matchsetuptime = gi.cvar("matchsetuptime", "10", CVAR_NOFLAGS);
	matchstarttime = gi.cvar("matchstarttime", "20", CVAR_NOFLAGS);
	admin_password = gi.cvar("admin_password", "", CVAR_NOFLAGS);
	g_allow_admin = gi.cvar("g_allow_admin", "1", CVAR_NOFLAGS);
	warn_unbalanced = gi.cvar("warn_unbalanced", "0", CVAR_NOFLAGS);
	//-ctf
	g_eyecam = gi.cvar("g_eyecam", "1", CVAR_NOFLAGS);
	g_teleporter_nofreeze = gi.cvar("g_teleporter_nofreeze", "0", CVAR_NOFLAGS);

	g_showhelp = gi.cvar("g_showhelp", "1", CVAR_NOFLAGS);

	g_matchstats = gi.cvar("g_matchstats", "0", CVAR_NOFLAGS);

	g_dm_spawnpads = gi.cvar("g_dm_spawnpads", "1", CVAR_NOFLAGS);

	g_expert = gi.cvar("g_expert", "0", CVAR_NOFLAGS);

	g_item_bobbing = gi.cvar("g_item_bobbing", "1", CVAR_NOFLAGS);
	g_weapon_force_central_projection = gi.cvar("g_weapon_force_central_projection", "0", CVAR_NOFLAGS);

	g_allow_custom_skins = gi.cvar("g_allow_custom_skins", "1", CVAR_NOFLAGS);

	g_allow_voting = gi.cvar("g_allow_voting", "0", CVAR_NOFLAGS);
	g_allow_vote_midgame = gi.cvar("g_allow_vote_midgame", "0", CVAR_NOFLAGS);
	g_allow_spec_vote = gi.cvar("g_allow_spec_vote", "1", CVAR_NOFLAGS);
	g_vote_limit = gi.cvar("g_vote_limit", "3", CVAR_NOFLAGS);
	g_vote_flags = gi.cvar("g_vote_flags", "0", CVAR_NOFLAGS);

	g_dm_auto_join = gi.cvar("g_dm_auto_join", "0", CVAR_NOFLAGS);

	g_dm_overtime = gi.cvar("g_dm_overtime", "120", CVAR_NOFLAGS);

	g_dm_do_warmup = gi.cvar("g_dm_do_warmup", "1", CVAR_NOFLAGS);
	g_dm_do_readyup = gi.cvar("g_dm_do_readyup", "0", CVAR_NOFLAGS);

	g_item_chain_random = gi.cvar("g_item_chain_random", "0", CVAR_NOFLAGS);

	g_allow_forfeit = gi.cvar("g_allow_forfeit", "1", CVAR_NOFLAGS);

	// items
	InitItems();

	game = {};

	// initialize all entities for this game
	game.maxentities = maxentities->integer;
	g_edicts = (edict_t *)gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = maxclients->integer;
	game.clients = (gclient_t *)gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients + 1;

	// how far back we should support lag origins for
	game.max_lag_origins = 20 * (0.1f / gi.frame_time_s);
	game.lag_origins = (vec3_t *)gi.TagMalloc(game.maxclients * sizeof(vec3_t) * game.max_lag_origins, TAG_GAME);

	level.start_time = level.time;

	level.ready_to_exit = false;

	level.match_state = MS_WARMUP_DELAYED;
	level.match_state_timer = 0_sec;
	level.match_time = level.time;

	level.locked[TEAM_SPECTATOR] = false;
	level.locked[TEAM_FREE] = false;
	level.locked[TEAM_RED] = false;
	level.locked[TEAM_BLUE] = false;

	//Horde_InitFirstWave();
	Horde_CreateWave();
}

//===================================================================

#if 0
bool Match_CheckRules() {
	int		 t;
	uint32_t i, j;
	char	 text[64];
	edict_t *ent;

	if (level.voting_type != VOTING_NONE && level.voting_time <= level.time) {
		gi.LocBroadcast_Print(PRINT_CHAT, "Voting timed out and has been cancelled.\n");
		level.voting_type = VOTING_NONE;
	}

	if (level.match != MATCH_NONE) {
		t = (level.match_time - level.time).seconds<int>();

		// no team warnings in match mode
		level.warnactive = 0;

		if (t <= 0) { // time ended on something
			switch (level.match) {
			case MATCH_SETUP:
				// go back to normal mode
				if (competition->integer < 3) {
					level.match = MATCH_NONE;
					gi.cvar_set("competition", "1");
					Match_Reset();
				} else {
					// reset the time
					level.match_time = level.time + gtime_t::from_min(matchsetuptime->value);
				}
				return false;

			case MATCH_PREGAME:
				// match started!
				Match_Start();
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);
				return false;

			case MATCH_GAME:
				// match ended!
				Match_End();
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/bigtele.wav"), 1, ATTN_NONE, 0);
				return false;

			default:
				break;
			}
		}

		if (t == level.lasttime)
			return false;

		level.lasttime = t;

		switch (level.match) {
		case MATCH_SETUP:
			for (j = 0, i = 1; i <= game.maxclients; i++) {
				ent = g_edicts + i;
				if (!ent->inuse)
					continue;
				if (ClientIsPlaying(ent->client) &&
					!ent->client->resp.ready)
					j++;
			}

			if (competition->integer < 3)
				G_FmtTo(text, "{:02}:{:02} SETUP: {} not ready", t / 60, t % 60, j);
			else
				G_FmtTo(text, "SETUP: {} not ready", j);

			gi.configstring(CONFIG_MATCH_STATE, text);
			break;

		case MATCH_PREGAME:
			G_FmtTo(text, "{:02}:{:02} UNTIL START", t / 60, t % 60);
			gi.configstring(CONFIG_MATCH_STATE, text);

			if (t <= 10 && !level.countdown) {
				level.countdown = true;
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
			break;

		case MATCH_GAME:
			G_FmtTo(text, "{:02}:{:02} MATCH", t / 60, t % 60);
			gi.configstring(CONFIG_MATCH_STATE, text);
			if (t <= 10 && !level.countdown) {
				level.countdown = true;
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
			break;

		default:
			break;
		}
		return false;
	} else {
		if (Teams()) {
			int tally_red_score = 0, tally_blue_score = 0;

			if (level.time == gtime_t::from_sec(level.lasttime))
				return false;
			level.lasttime = level.time.seconds<int>();
			// this is only done in non-match (public) mode

			if (warn_unbalanced->integer) {
				// count up the team totals
				for (i = 1; i <= game.maxclients; i++) {
					ent = g_edicts + i;
					if (!ent->inuse)
						continue;
					if (ent->client->resp.team == TEAM_RED)
						tally_red_score++;
					else if (ent->client->resp.team == TEAM_BLUE)
						tally_blue_score++;
				}

				if (tally_red_score - tally_blue_score >= 2 && tally_blue_score >= 2) {
					if (level.warnactive != TEAM_RED) {
						level.warnactive = TEAM_RED;
						gi.configstring(CONFIG_CTF_TEAMINFO, "WARNING: Red has too many players");
					}
				} else if (tally_blue_score - tally_red_score >= 2 && tally_red_score >= 2) {
					if (level.warnactive != TEAM_BLUE) {
						level.warnactive = TEAM_BLUE;
						gi.configstring(CONFIG_CTF_TEAMINFO, "WARNING: Blue has too many players");
					}
				} else
					level.warnactive = 0;
			} else
				level.warnactive = 0;
		}
	}

	if (capturelimit->integer &&
		(level.team_scores[TEAM_RED] >= capturelimit->integer ||
			level.team_scores[TEAM_BLUE] >= capturelimit->integer)) {
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_capturelimit_hit");
		return true;
	}
	return false;
}
#endif

#if 0
static void ClearBodyQue(void) {
	int	i;
	edict_t *ent;

	for (i = 0; i < BODY_QUEUE_SIZE; i++) {
		ent = level.bodyQue[i];
		if (ent->linked) {
			gi.unlinkentity(ent);
		}
	}
}
#endif
/*
static void G_WarmupEnd(void) {
	gclient_t *client;
	edict_t *ent;
	int i, t;

	// remove corpses
	//ClearBodyQue();

	// return flags
	CTF_ResetFlags();

	memset(level.team_scores, 0, sizeof(level.team_scores));

	level.match_state_timer = 0_ms;
	level.match_state = MS_NONE;
	level.start_time = level.time;

	Match_Reset();
	
	// respawn items, remove projectiles, etc.
	ent = g_edicts + MAX_CLIENTS;
	for (i = MAX_CLIENTS; i < MAX_ITEMS; i++, ent++) {

		if (!ent->inuse)
			continue;

		if (ent->tag == TAG_DONTSPAWN) {
			ent->nextthink = 0;
			continue;
		}

		if (ent->s.eType == ET_ITEM && ent->item) {

			// already processed in Team_ResetFlags()
			if (ent->item->id == IT_FLAG_RED || ent->item->id == IT_FLAG_BLUE)
				continue;

			// remove dropped items
			if (ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED)) {
				ent->nextthink = level.time;
				continue;
			}

			// respawn picked up items
			t = SpawnTime(ent, true);
			if (t != 0) {
				ent->svflags &= ~SVF_NOCLIENT;
				ent->solid = SOLID_TRIGGER;
				gi.linkentity(ent);
				ent->s.event = EV_ITEM_RESPAWN;
			} else {
				t = FRAMETIME;
				if (ent->activator) {
					ent->activator = NULL;
					ent->think = SpawnItem;
				}
			}
			if (ent->random) {
				t += (crandom() * ent->random) * 1000;
				if (t < FRAMETIME) {
					t = FRAMETIME;
				}
			}
			ent->nextthink = level.time + t;

		} else if (ent->svflags == SVF_PROJECTILE) {
			// remove all launched missiles
			G_FreeEdict(ent);
		}
	}
}
*/

/*
============
Entities_Reset

Reset clients and items
============
*/
static void Entities_Reset() {
	edict_t *ent;
	size_t	i;

	// reset the players
	for (i = 1; i <= game.maxclients; i++) {
		ent = g_edicts + i;

		if (!ent->inuse)
			continue;

		if (!ent->client)
			continue;

		if (!ent->client->pers.connected)
			continue;

		//G_SetPlayerScore(ent->client, 0);
		ent->client->resp.ctf_state = 0;
		ent->client->resp.ghost = nullptr;
		ent->client->resp.score = 0;

		//gi.LocCenter_Print(ent, "MATCH HAS STARTED!");

		if (ClientIsPlaying(ent->client)) {
			// make up a ghost code
			Match_Ghost_Assign(ent);
			Weapon_Grapple_DoReset(ent->client);
			ent->svflags = SVF_NOCLIENT;
			ent->flags &= ~FL_GODMODE;

			ent->client->respawn_time = level.time;	// +random_time(1_sec, 4_sec);
			ent->client->ps.pmove.pm_type = PM_DEAD;
			ent->client->anim_priority = ANIM_DEATH;
			ent->s.frame = FRAME_death308 - 1;
			ent->client->anim_end = FRAME_death308;
			ent->deadflag = true;
			ent->movetype = MOVETYPE_NOCLIP;
			ent->client->ps.gunindex = 0;
			ent->client->ps.gunskin = 0;
			gi.linkentity(ent);
		}
	}

	CalculateRanks();

	// reset the level items
	Tech_Reset();
	CTF_ResetFlags();

	// set up weapon chains
	for (i = IT_NULL; i < IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		if (!itemlist[i].chain)
			continue;

		gitem_t *item = &itemlist[i];

		// already initialized
		if (item->chain_next)
			continue;

		gitem_t *chain_item = &itemlist[item->chain];

		if (!chain_item)
			gi.Com_ErrorFmt("Invalid item chain {} for {}", (int32_t)item->chain, item->pickup_name);

		// set up initial chain
		if (!chain_item->chain_next)
			chain_item->chain_next = chain_item;

		// if we're not the first in chain, add us now
		if (chain_item != item) {
			gitem_t *c;

			// end of chain is one whose chain_next points to chain_item
			for (c = chain_item; c->chain_next != chain_item; c = c->chain_next)
				continue;

			// splice us in
			item->chain_next = chain_item;
			c->chain_next = item;
		}
	}

	for (ent = g_edicts + 1, i = 1; i < globals.num_edicts; i++, ent++) {
		if (ent->inuse && ent->item) {	//!ent->client) {
			if (ent->solid == SOLID_NOT && /*ent->think == RespawnItem && */ent->nextthink >= level.time) {
				// powerups don't spawn in for a while
				if (g_dm_powerups_style->integer && (ent->item->flags & IF_POWERUP)) {
					ent->svflags |= SVF_NOCLIENT;
					ent->solid = SOLID_NOT;

					ent->nextthink = level.time + gtime_t::from_sec(irandom(30, 60));
					ent->think = RespawnItem;
					continue;
				} else {
					ent->nextthink = 0_ms;
					RespawnItem(ent);
				}
			}
		}
	}
}

/*
============
Match_Start

Starts a match
============
*/
void Match_Start() {
	if (!deathmatch->integer)
		return;

	level.match_time = level.time;	// +gtime_t::from_min(timelimit->value);
	level.overtime = 0_sec;

	const char *s = G_TimeString(timelimit->value ? timelimit->value : 0 * 1000);
	gi.configstring(CONFIG_MATCH_STATE, s);

	level.match_state = MS_MATCH_IN_PROGRESS;
	level.match_state_timer = level.time;

	level.team_scores[TEAM_RED] = level.team_scores[TEAM_BLUE] = 0;

	memset(level.ghosts, 0, sizeof(level.ghosts));

	Entities_Reset();

	gi.LocBroadcast_Print(PRINT_TTS, "The match has started!\n");
	gi.LocBroadcast_Print(PRINT_CENTER, "FIGHT!");
	gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);
}

/*
============
Match_Reset
============
*/
void Match_Reset() {

	if (!g_dm_do_warmup->integer) {
		Match_Start();
		return;
	}

	Entities_Reset();

	level.match_time = level.time;
	level.match_state = MS_WARMUP_DEFAULT;
	level.match_state_timer = 0_sec;

	CalculateRanks();

	gi.LocBroadcast_Print(PRINT_TTS, "The match has been reset.\n");
}

/*
=============
Duel_AddPlayer

If there are less than two players in the arena, place the
next queued player in the game and restart
=============
*/
static void Duel_AddPlayer(void) {
	gclient_t	*client, *next_in_line;

	if (level.num_playing_clients >= 2)
		return;

	// never change during intermission
	if (level.match_state == MS_MATCH_ENDED || level.intermission_time || level.intermission_queued)
		return;

	next_in_line = nullptr;

	for (size_t i = 0; i < game.maxclients; i++) {
		client = &game.clients[i];
		if (!client->pers.connected)
			continue;

		if (client->resp.team != TEAM_SPECTATOR)
			continue;

		if (!client->resp.duel_queued)
			continue;

		if (client->resp.spectator_client < 0)
			continue;

		if (!next_in_line || client->resp.spectator_time > next_in_line->resp.spectator_time)
			next_in_line = client;
	}

	if (!next_in_line)
		return;

	level.match_state_timer = 0_sec;
	level.match_state = MS_WARMUP_DEFAULT;

	// set them to free-for-all team
	SetTeam(&g_edicts[next_in_line - game.clients], TEAM_FREE, false, false);
}

/*
=======================
Duel_RemoveLoser

Make the loser a queued player at the back of the line
=======================
*/
static void Duel_RemoveLoser(void) {
	int client_num;

	if (level.num_playing_clients != 2)
		return;

	client_num = level.sorted_clients[1];

	if (!game.clients[client_num].pers.connected)
		return;

	gi.Com_PrintFmt( "Duel: Moving the loser, {}, to spectator.\n", game.clients[client_num].resp.netname);

	// make them a queued player
	SetTeam(&g_edicts[client_num+1], TEAM_NONE, false, true);
}

/*
=======================
Duel_MatchEnd_AdjustScores
=======================
*/
static void Duel_MatchEnd_AdjustScores(void) {
	if (!duel->integer)
		return;

	int client_num;

	client_num = level.sorted_clients[0];
	if (game.clients[client_num].pers.connected) {
		game.clients[client_num].resp.wins++;
		//ClientUserinfoChanged(&g_edicts[client_num], g_edicts[client_num].client->pers.userinfo);
	}

	client_num = level.sorted_clients[1];
	if (game.clients[client_num].pers.connected) {
		game.clients[client_num].resp.losses++;
		//ClientUserinfoChanged(&g_edicts[client_num], g_edicts[client_num].client->pers.userinfo);
	}
}

/*
=============
ReadyAll
=============
*/
void ReadyAll() {
	uint8_t i;
	edict_t *e;

	for (i = 1; i <= game.maxclients; i++) {
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (!e->client)
			continue;

		e->client->resp.ready = true;
	}
}

/*
=============
UnReadyAll
=============
*/
void UnReadyAll() {
	uint8_t i;
	edict_t *e;

	for (i = 1; i <= game.maxclients; i++) {
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (!e->client)
			continue;

		e->client->resp.ready = false;
	}
}

/*
=============
CheckReady
=============
*/
static bool CheckReady() {
	uint8_t i, count_total, count_ready;
	edict_t *e;

	if (!g_dm_do_readyup->integer)
		return true;

	count_total = count_ready = 0;
	for (i = 1; i <= game.maxclients; i++) {
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (!e->client)
			continue;
		if (!ClientIsPlaying(e->client))
			continue;
		if (e->svflags & SVF_BOT)
			continue;

		count_total++;

		if (e->client->resp.ready)
			count_ready++;
	}

	if (!count_ready || !count_total)
		return false;

	if (duel->integer && count_ready == 2 && count_total == 2)
		return true;

	if (((float)count_ready / (float)count_total) * 100 >= g_warmup_ready_percentage->value * 100)
		return true;
	return false;
}

/*
=============
Round_End
=============
*/
static void Round_End() {
	level.round_state = MS_ROUND_NONE;
	level.round_state_timer = level.time + 1_sec;
}

/*
=============
CheckRoundState
=============
*/
static void CheckRoundState(void) {
	if (!clanarena->integer)
		return;

	if (level.match_state != MS_MATCH_IN_PROGRESS)
		return;

	// initiate round
	if (level.round_state == MS_ROUND_NONE) {
		if (level.round_state_timer > level.time)
			return;
		level.round_state = MS_ROUND_COUNTDOWN;
		level.round_state_timer = level.time + 10_sec;
		return;
	}

	// start round
	if (level.round_state == MS_ROUND_COUNTDOWN) {
		if (level.match_state_timer > level.round_state_timer) {
			level.round_state = MS_ROUND_IN_PROGRESS;
			level.round_state_timer = 0_sec;
			level.round_number++;
		} else {
			return;
		}
	}

	// end round
	if (level.round_state == MS_ROUND_IN_PROGRESS) {
		if (!level.num_living_red && level.num_eliminated_red && level.num_living_blue) {
			G_AdjustTeamScore(TEAM_BLUE, 1);
			gi.LocBroadcast_Print(PRINT_CENTER, "BLUE TEAM wins the round!\n");
			Round_End();
			return;
		}
		if (!level.num_living_blue && level.num_eliminated_blue && level.num_living_red) {
			G_AdjustTeamScore(TEAM_RED, 1);
			gi.LocBroadcast_Print(PRINT_CENTER, "RED TEAM wins the round!\n");
			Round_End();
			return;
		}
	}
}

/*
=============
CheckCountdown
=============
*/
static void CheckCountdown(void) {
	if (level.match_state != MS_MATCH_COUNTDOWN) {
		if (level.countdown_check) {
			level.countdown_check = 0_sec;
		}
		return;
	}

	int t = (level.match_state_timer - level.time).seconds<int>() + 1;

	if (!level.countdown_check || level.countdown_check.seconds<int>() > t) {
		if (t <= 10) {
			gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex(G_Fmt("world/{}.wav", t).data()), 1, ATTN_NONE, 0);
		}
		level.countdown_check = gtime_t::from_sec(t);
	}
}

/*
=============
CheckWarmupState

Once a frame, check for changes in match state during warmup
=============
*/
static void CheckWarmupState(void) {

	if (!deathmatch->integer)
		return;

	if (!g_dm_do_warmup->integer) {
		Match_Start();
		return;
	}

	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if (!level.num_playing_clients) {
		if (level.match_state != MS_WARMUP_DELAYED) {
			level.match_state = MS_WARMUP_DELAYED;
			level.match_state_timer = level.time + 5_sec;
		}
		return;
	}

	if (level.match_state == MS_WARMUP_DELAYED && level.match_state_timer > level.time)
		return;

	if (duel->integer) {

		// pull in a queued spectator if needed
		if (level.num_playing_clients < 2)
			Duel_AddPlayer();

		// if we don't have two players, go back to "waiting for players"
		if (level.num_playing_clients != 2) {
			if (level.match_state <= MS_MATCH_COUNTDOWN) {
				if (level.match_state == MS_WARMUP_READYUP)
					UnReadyAll();
				else if (level.match_state == MS_MATCH_COUNTDOWN)
					gi.LocBroadcast_Print(PRINT_CENTER, "Countdown cancelled:\nnot enough players.\n");

				if (level.match_state != MS_WARMUP_DEFAULT) {
					level.match_state_timer = 0_sec;
					level.match_state = MS_WARMUP_DEFAULT;
				}
				//G_LogPrintf("Warmup:\n");
			}
			return;
		}

		if (level.match_state > MS_MATCH_COUNTDOWN)
			return;

		// if the warmup is changed at the console, restart it
		if (g_warmup_countdown->modified_count != level.warmup_modification_count) {
			level.warmup_modification_count = g_warmup_countdown->modified_count;
			level.match_state_timer = 0_sec;
			level.match_state = MS_WARMUP_DEFAULT;
		}

		// if all players have arrived, let them ready up
		if (level.match_state == MS_WARMUP_DEFAULT) {
			level.match_state = MS_WARMUP_READYUP;
			level.match_state_timer = 0_ms;
			return;
		}

		// if both players are ready, let's start
		if (level.match_state == MS_WARMUP_READYUP) {
			if (CheckReady()) {
				if (g_warmup_countdown->integer > 0) {
					level.match_state_timer = level.time + gtime_t::from_sec(g_warmup_countdown->integer);
					level.match_state = MS_MATCH_COUNTDOWN;
					//gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);

					// announce it
					if (&game.clients[level.sorted_clients[0]] && &game.clients[level.sorted_clients[1]])
						gi.LocBroadcast_Print(PRINT_CENTER, "{} vs {}\nBegins in...", game.clients[level.sorted_clients[0]].resp.netname, game.clients[level.sorted_clients[1]].resp.netname);
				} else {
					level.match_state_timer = 0_ms;
					level.match_state = MS_MATCH_IN_PROGRESS;
				}
			}
			return;
		}

		// if the warmup time has counted down, start the match
		if (level.match_state == MS_MATCH_COUNTDOWN && level.time.seconds() > level.match_state_timer.seconds()) {
			Match_Start();
			return;
		}
	} else {
		bool not_enough = false;

		if (Teams()) {
			int counts[TEAM_NUM_TEAMS];
			counts[TEAM_BLUE] = TeamConnectedCount(-1, TEAM_BLUE);
			counts[TEAM_RED] = TeamConnectedCount(-1, TEAM_RED);

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1 || level.num_playing_clients < minplayers->integer) {
				not_enough = true;
			}
		} else if (level.num_playing_clients < minplayers->integer) {
			not_enough = true;
		}

		if (not_enough) {
			if (level.match_state <= MS_MATCH_COUNTDOWN) {
				if (level.match_state == MS_WARMUP_READYUP)
					UnReadyAll();
				else if (level.match_state == MS_MATCH_COUNTDOWN)
					gi.LocBroadcast_Print(PRINT_CENTER, "Countdown cancelled:\nnot enough players.\n");

				if (level.match_state != MS_WARMUP_DEFAULT) {
					level.match_state = MS_WARMUP_DEFAULT;
					level.match_state_timer = 0_sec;
					//G_LogPrintf("Warmup:\n");
				}
			}
			return; // still waiting for players
		}

		if (level.match_state == MS_WARMUP_DEFAULT) {
			level.match_state = MS_WARMUP_READYUP;
			level.match_state_timer = 0_sec;
			//G_LogPrintf("Warmup:\n");
			return;
		}

		if (level.match_state > MS_MATCH_COUNTDOWN)
			return;

		// if the warmup is changed at the console, restart it
		if (g_warmup_countdown->modified_count != level.warmup_modification_count) {
			level.warmup_modification_count = g_warmup_countdown->modified_count;
			level.match_state_timer = 0_sec;
			level.match_state = MS_WARMUP_DEFAULT;
		}

		// if enough players have arrived, let them ready up
		if (level.match_state == MS_WARMUP_DEFAULT) {
			level.match_state = MS_WARMUP_READYUP;
			level.match_state_timer = 0_ms;
			return;
		}

		// if enough players are ready, start countdown
		if (level.match_state == MS_WARMUP_READYUP) {
			if (CheckReady()) {
				if (g_warmup_countdown->integer > 0) {
					level.match_state_timer = level.time + gtime_t::from_sec(g_warmup_countdown->integer);
					level.match_state = MS_MATCH_COUNTDOWN;
					//gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);

					// announce it
					gi.LocBroadcast_Print(PRINT_CENTER, "{}\nBegins in...", level.gametype_name);
				} else {
					level.match_state_timer = 0_ms;
					level.match_state = MS_MATCH_COUNTDOWN;
				}
			}
			return;
		}

		// if the warmup time has counted down, start the match
		if (level.match_state == MS_MATCH_COUNTDOWN && level.time.seconds() > level.match_state_timer.seconds()) {
			Match_Start();
			return;
		}
	}
}

/*
==================
CheckVote
==================
*/
static void CheckVote(void) {

	// vote has passed, execute
	if (level.vote_execute_time) {
		if (level.time > level.vote_execute_time) {
			Cmd_Vote_Passed();
			//gi.AddCommandString(G_Fmt("{} {}\n", level.vote->name, level.vote_arg).data());
		}
		return;
	}

	if (!level.vote_time)
		return;

	if (level.time - level.vote_time >= 30_sec) {
		gi.LocBroadcast_Print(PRINT_HIGH, "Vote failed.\n");
	} else {
		if (level.vote_yes > level.num_voting_clients / 2) {
			// execute the command, then remove the vote
			gi.LocBroadcast_Print(PRINT_HIGH, "Vote passed.\n");
			level.vote_execute_time = level.time + 3_sec;
		} else if (level.vote_no >= level.num_voting_clients / 2) {
			// same behavior as a timeout
			gi.LocBroadcast_Print(PRINT_HIGH, "Vote failed.\n");
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.vote_time = 0_sec;
	//trap_SetConfigstring( CS_VOTE_TIME, "" );
}

// ----------------

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint(void) {
	edict_t *ent, *target;
	vec3_t	dir;
	bool	is_landmark = false;

	if (level.intermission_spot) // search only once
		return;

	// find the intermission spot
	ent = level.spawn_spots[SPAWN_SPOT_INTERMISSION];

	if (!ent) { // the map creator forgot to put in an intermission point...
		SelectSpawnPoint(NULL, level.intermission_origin, level.intermission_angle, false, is_landmark);
	} else {
		level.intermission_origin = ent->s.origin;
		level.intermission_angle = ent->s.angles;
		// if it has a target, look towards it
		if (ent->target) {
			target = G_PickTarget(ent->target);
			if (target) {
				dir = (target->s.origin - level.intermission_origin).normalized();
				AngleVectors(dir);
			}
		}
	}

	level.intermission_spot = true;
}

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.

Adapted from Quake III
=================
*/

static void CheckIntermissionExit(void) {
	int			ready, not_ready;
	gclient_t	*cl;
	edict_t		*ent;

	// see which players are ready
	ready = not_ready = 0;
	cl = game.clients;
	ent = g_edicts + 1;
	for (size_t i = 0; i < game.maxclients; i++, cl++, ent++) {
		if (!ent->inuse)
			continue;
		if (!cl)
			continue;
		if (!cl->pers.connected)
			continue;
		if (!ClientIsPlaying(cl))
			continue;

		if (ent->client->resp.is_a_bot) {
			cl->ready_to_exit = true;
			//gi.Client_Print(&g_edicts[1], PRINT_HIGH, "THERE BE A BOT\n");
			//continue;
		}

		if (cl->ready_to_exit) {
			ready++;
			//gi.Client_Print(&g_edicts[1], PRINT_HIGH, "READY\n");
			//continue;
		} else {
			//gi.Client_Print(&g_edicts[1], PRINT_HIGH, "NOT READY\n");
			not_ready++;
		}
	}

	// vote in progress
	if (level.vote_time || level.vote_execute_time) {
		ready = 0;
		not_ready = 1;
	}

	// never exit in less than five seconds
	if (level.time < level.intermission_time + 5_sec && !level.exit_time)
		return;

	// if nobody wants to go, clear timer
	// skip this if no players present
	if (!ready && not_ready) {
		level.ready_to_exit = false;
		return;
	}

	// if everyone wants to go, go now
	if (!not_ready) {
		//gi.Client_Print(&g_edicts[1], PRINT_HIGH, "EXIT THE DAMNED LEVEL\n");
		//Match_End();
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if (ready && !level.ready_to_exit) {
		level.ready_to_exit = true;
		level.exit_time = level.time + 10_sec;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if (level.time < level.exit_time)
		return;

	//Match_End();
	ExitLevel();
}

/*
=============
ScoreIsTied

Adapted from Quake III
=============
*/
static bool ScoreIsTied(void) {
	if (level.num_playing_clients < 2) {
		return false;
	}

	if (Teams())
		return level.team_scores[TEAM_RED] == level.team_scores[TEAM_BLUE];

	return game.clients[level.sorted_clients[0]].resp.score == game.clients[level.sorted_clients[1]].resp.score;
}

/*
=============
SortRanks

Adapted from Quake III
=============
*/
static int SortRanks(const void *a, const void *b) {
	gclient_t *ca, *cb;

	ca = &game.clients[*(int *)a];
	cb = &game.clients[*(int *)b];

	// sort special clients last
	if (ca->resp.spectator_client < 0)
		return 1;
	if (cb->resp.spectator_client < 0)
		return -1;

	// then connecting clients
	if (!ca->pers.connected)
		return 1;
	if (!cb->pers.connected)
		return -1;

	// then spectators
	if (ca->resp.team == TEAM_SPECTATOR && cb->resp.team == TEAM_SPECTATOR) {
		if (ca->resp.duel_queued && cb->resp.duel_queued) {
			if (ca->resp.spectator_time > cb->resp.spectator_time)
				return -1;
			if (ca->resp.spectator_time < cb->resp.spectator_time)
				return 1;
		}
		if (ca->resp.duel_queued) {
			return -1;
		}
		if (cb->resp.duel_queued) {
			return 1;
		}
		if (ca->resp.spectator_time > cb->resp.spectator_time)
			return -1;
		if (ca->resp.spectator_time < cb->resp.spectator_time)
			return 1;
		return 0;
	}
	if (ca->resp.team == TEAM_SPECTATOR)
		return 1;
	if (cb->resp.team == TEAM_SPECTATOR)
		return -1;

	// then sort by score
	if (ca->resp.score > cb->resp.score)
		return -1;
	if (ca->resp.score < cb->resp.score)
		return 1;

	// then sort by time
	if (ca->resp.entertime < cb->resp.entertime)
		return -1;
	if (ca->resp.entertime > cb->resp.entertime)
		return 1;

	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.

Adapted from Quake III
============
*/
void CalculateRanks() {
	gclient_t *cl;
	size_t		i;

	if (level.restarted)
		return;

	level.num_connected_clients = 0;
	level.num_nonspectator_clients = 0;
	level.num_playing_clients = 0;
	level.num_human_clients = 0;
	level.num_eliminated_red = 0;
	level.num_eliminated_blue = 0;
	level.num_living_red = 0;
	level.num_living_blue = 0;
	for (i = 0; i < game.maxclients; i++) {
		cl = &game.clients[i];
		if (cl->pers.connected) {
			level.sorted_clients[level.num_connected_clients] = i;
			level.num_connected_clients++;

			if (ClientIsPlaying(cl)) {
				level.num_nonspectator_clients++;

				// decide if this should be auto-followed
				if (cl->pers.connected) {
					level.num_playing_clients++;
					if (!(g_edicts[i].svflags & SVF_BOT)) {
						level.num_human_clients++;
					}
					if (level.follow1 == -1) {
						level.follow1 = i;
					} else if (level.follow2 == -1) {
						level.follow2 = i;
					}

					if (Teams()) {
						if (cl->resp.team == TEAM_RED) {
							if (cl->pers.health > 0)
								level.num_living_red++;
							else
								level.num_eliminated_red++;
						} else {
							if (cl->pers.health > 0)
								level.num_living_blue++;
							else
								level.num_eliminated_blue++;
						}
					}
				}
			}
		}
	}

	qsort(level.sorted_clients, level.num_connected_clients, sizeof(level.sorted_clients[0]), SortRanks);

	// set the rank value for all clients that are connected and not spectators
	if (Teams()) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for (i = 0; i < level.num_connected_clients; i++) {
			cl = &game.clients[level.sorted_clients[i]];
			if (level.team_scores[TEAM_RED] == level.team_scores[TEAM_BLUE]) {
				cl->resp.rank = 2;
			} else if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE]) {
				cl->resp.rank = 0;
			} else {
				cl->resp.rank = 1;
			}
		}
	} else {
		int score = 0, new_score, rank;

		for (i = 0; i < level.num_playing_clients; i++) {
			if (game.clients[i].pers.connected) {
				cl = &game.clients[level.sorted_clients[i]];
				new_score = cl->resp.score;
				if (i == 0 || new_score != score) {
					rank = i;
					// assume we aren't tied until the next client is checked
					game.clients[level.sorted_clients[i]].resp.rank = rank;
				} else {
					// we are tied with the previous client
					game.clients[level.sorted_clients[i - 1]].resp.rank = rank | RANK_TIED_FLAG;
					game.clients[level.sorted_clients[i]].resp.rank = rank | RANK_TIED_FLAG;
				}
				score = new_score;
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();
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
	globals.CanSave = G_CanSave;

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
	globals.Bot_TriggerEdict = Bot_TriggerEdict;
	globals.Bot_GetItemID = Bot_GetItemID;
	globals.Bot_UseItem = Bot_UseItem;
	globals.Edict_ForceLookAtPoint = Edict_ForceLookAtPoint;
	globals.Bot_PickedUpItem = Bot_PickedUpItem;

	globals.Entity_IsVisibleToPlayer = Entity_IsVisibleToPlayer;
	globals.GetShadowLightData = GetShadowLightData;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

//======================================================================

/*
=================
ClientEndServerFrames
=================
*/
static void ClientEndServerFrames() {
	edict_t *ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (size_t i = 0; i < game.maxclients; i++) {
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame(ent);
	}
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
static edict_t *CreateTargetChangeLevel(const char *map) {
	edict_t *ent;

	ent = G_Spawn();
	ent->classname = "target_changelevel";
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

/*
=================
Match_End

The time limit or score limit has been exceeded
=================
*/
void Match_End() {
	edict_t *ent;
	
	// stay on same level flag
	if (g_dm_same_level->integer) {
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	if (*level.forcemap) {
		BeginIntermission(CreateTargetChangeLevel(level.forcemap));
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
	ent = G_FindByString<&edict_t::classname>(nullptr, "target_changelevel");

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

void QueueIntermission(const char *msg) {
	if (level.intermission_queued)
		return;

	level.match_state = MS_MATCH_ENDED;
	level.match_state_timer = 0_sec;
	level.match_time = level.time;

	gi.LocBroadcast_Print(PRINT_CHAT, "MATCH END: {}\n", msg[0] ? msg : "Unknown Reason");
	level.intermission_queued = level.time;
	gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/xian1.wav"), 1, ATTN_NONE, 0);	//klaxon1.wav
	gi.configstring(CS_CDTRACK, "0");
}

int GT_ScoreLimit() {
	if (ctf->integer)
		return capturelimit->integer;
	if (clanarena->integer || freeze->integer)
		return roundlimit->integer;
	return fraglimit->integer;
}

const char *GT_ScoreLimitString() {
	if (ctf->integer)
		return "capture";
	if (clanarena->integer || freeze->integer)
		return "round";
	return "frag";
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag/capture.
=================
*/
void CheckExitRules() {
	gclient_t	*cl;
	int			scorelimit = 0;

	if (!deathmatch->integer)
		return;

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (level.intermission_time) {
		CheckIntermissionExit();
		return;
	}

	if (level.intermission_queued) {
		if (level.time - level.intermission_queued >= 1_sec) {
			level.intermission_queued = 0_ms;
			Match_End();
		}
		return;
	}

	if (level.match_state < MS_MATCH_IN_PROGRESS)
		return;
	
	if (level.time - level.match_time <= FRAME_TIME_MS)
		return;
		
	if (timelimit->value) {
		//if (level.time >= gtime_t::from_min(timelimit->value) + level.overtime) {
		if (level.time >= level.match_time + gtime_t::from_min(timelimit->value) + level.overtime) {
			// check for overtime
			if (ScoreIsTied()) {
				if (duel->integer) {
					if (g_dm_overtime->integer > 0) {
						level.overtime += gtime_t::from_sec(g_dm_overtime->integer);
						gi.LocBroadcast_Print(PRINT_CENTER, "Overtime!\n{} added", G_TimeString(g_dm_overtime->integer * 1000));
						gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/klaxon2.wav"), 1, ATTN_NONE, 0);
						return;
					}
				} else {
					if (!level.suddendeath) {
						gi.LocBroadcast_Print(PRINT_CENTER, "Sudden Death!");
						level.suddendeath = true;
						gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/klaxon2.wav"), 1, ATTN_NONE, 0);
						return;
					}
				}
				return;
			}

			// find the winner and broadcast it
			if (Teams()) {
				if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE]) {
					QueueIntermission(G_Fmt("Red Team WINS with a final score of {} to {}.\n", level.team_scores[TEAM_RED], level.team_scores[TEAM_BLUE]).data());
					return;
				}
				if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED]) {
					QueueIntermission(G_Fmt("Blue Team WINS with a final score of {} to {}.\n", level.team_scores[TEAM_BLUE], level.team_scores[TEAM_RED]).data());
					return;
				}
			} else {
				QueueIntermission(G_Fmt("{} WINS with a final score of {}.", game.clients[level.sorted_clients[0]].resp.netname, game.clients[level.sorted_clients[0]].resp.score).data());
				return;
			}

			QueueIntermission("Timelimit hit.");
			return;
		}
	}

	if (minplayers->integer && level.num_playing_clients < minplayers->integer) {
		QueueIntermission("Not enough players remaining.");
		return;
	}

	if (Teams() && g_teamplay_force_balance->integer) {
		if (abs(TeamCount(-1, TEAM_RED) - TeamCount(-1, TEAM_BLUE)) > 1) {
			QueueIntermission("Teams are imbalanced.");
			return;
		}
	}

	// check for sudden death
	if (ScoreIsTied()) {
		// always wait for sudden death
		return;
	}

	scorelimit = GT_ScoreLimit();
	if (!scorelimit) return;

	if (Teams()) {
		if (level.team_scores[TEAM_RED] >= scorelimit) {
			QueueIntermission(G_Fmt("Red Team WINS! (hit the {} limit)", GT_ScoreLimitString()).data());
			return;
		}

		if (level.team_scores[TEAM_BLUE] >= scorelimit) {
			QueueIntermission(G_Fmt("Blue Team WINS! (hit the {} limit)", GT_ScoreLimitString()).data());
			return;
		}
	} else {
		for (size_t i = 0; i < game.maxclients; i++) {
			cl = game.clients + i;
			if (!cl->pers.connected)
				continue;
			if (cl->resp.team != TEAM_FREE)
				continue;

			if (cl->resp.score >= scorelimit) {
				QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", cl->resp.netname, GT_ScoreLimitString()).data());
				return;
			}
		}
	}
}

static bool Match_NextMap() {
	if (level.match_state == MS_MATCH_ENDED) {
		level.match_state = MS_WARMUP_DELAYED;
		Match_Reset();
		return true;
	}
	return false;
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

	for (auto player : active_players())
		if (player->client->pers.spawned && ClientIsPlaying(player->client))
			player_ranks[player->s.number - 1] = player->client->resp.team == winning_team ? 1 : 2;
}

/*
=============
BeginIntermission
=============
*/
void BeginIntermission(edict_t *targ) {
	edict_t *cl_ent;

	if (level.intermission_time)
		return; // already activated

	// if in a duel, change the wins / losses
	Duel_MatchEnd_AdjustScores();

	game.autosaved = false;

	level.intermission_time = level.time;

	// respawn any dead clients
	for (size_t i = 0; i < game.maxclients; i++) {
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (cl_ent->health <= 0) {
			// give us our max health back since it will reset
			// to pers.health; in instanced items we'd lose the items
			// we touched so we always want to respawn with our max.
			if (P_UseCoopInstancedItems())
				cl_ent->client->pers.health = cl_ent->client->pers.max_health = cl_ent->max_health;

			ClientRespawn(cl_ent);
		}

		//MoveClientToIntermission(client);
	}

	level.intermission_server_frame = gi.ServerFrame();
	level.changemap = targ->map;
	level.intermission_clear = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY);
	level.intermission_eou = false;
	level.intermission_fade = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_FADE_OUT);

	// destroy all player trails
	PlayerTrail_Destroy(nullptr);

	// [Paril-KEX] update game level entry
	G_UpdateLevelEntry();

	if (strstr(level.changemap, "*")) {
		if (coop->integer) {
			for (size_t i = 0; i < game.maxclients; i++) {
				cl_ent = g_edicts + 1 + i;
				if (!cl_ent->inuse)
					continue;
				// strip players of all keys between units
				for (uint32_t n = 0; n < IT_TOTAL; n++)
					if (itemlist[n].flags & IF_KEY)
						cl_ent->client->pers.inventory[n] = 0;
			}
		}

		if (level.achievement && level.achievement[0]) {
			gi.WriteByte(svc_achievement);
			gi.WriteString(level.achievement);
			gi.multicast(vec3_origin, MULTICAST_ALL, true);
		}

		level.intermission_eou = true;

		// "no end of unit" maps handle intermission differently
		if (!targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT))
			G_EndOfUnitMessage();
		else if (targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE) && !deathmatch->integer) {
			// Need to call this now
			G_ReportMatchDetails(true);
			level.intermission_exit = true; // go immediately to the next level
			return;
		}
	} else {
		if (!deathmatch->integer) {
			level.intermission_exit = true; // go immediately to the next level
			return;
		}
	}

	// Call while intermission is running
	G_ReportMatchDetails(true);

	level.intermission_exit = false;

	if (!level.level_intermission_set) {
		FindIntermissionPoint();
		/*
		edict_t *ent;
		// find an intermission spot
		ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_intermission");
		if (!ent) { // the map creator forgot to put in an intermission point...
			ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_start");
			if (!ent)
				ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_deathmatch");
		} else { // choose one of four spots
			int32_t i = irandom(4);
			while (i--) {
				ent = G_FindByString<&edict_t::classname>(ent, "info_player_intermission");
				if (!ent) // wrap around the list
					ent = G_FindByString<&edict_t::classname>(ent, "info_player_intermission");
			}
		}

		level.intermission_origin = ent->s.origin;
		level.intermission_angle = ent->s.angles;
		*/
	}

	// move all clients to the intermission point
	for (size_t i = 0; i < game.maxclients; i++) {
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		MoveClientToIntermission(cl_ent);
	}
}

/*
=============
ExitLevel
=============
*/
void ExitLevel() {
	// [Paril-KEX] N64 fade
	if (level.intermission_fade) {
		level.intermission_fade_time = level.time + 1.3_sec;
		level.intermission_fading = true;
		return;
	}

	ClientEndServerFrames();

	// if we are running a duel, kick the loser to queue,
	// which will automatically grab the next queued player and restart
	if (duel->integer) {
		if (!level.restarted) {
			Duel_RemoveLoser();
			//trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
			//Match_Reset();
			level.restarted = true;
			//level.intermission_time = 0_sec;
		}
		//return;
	}

	level.intermission_time = 0_ms;

	// [Paril-KEX] support for intermission completely wiping players
	// back to default stuff
	if (level.intermission_clear) {
		level.intermission_clear = false;

		for (size_t i = 0; i < game.maxclients; i++) {
			// [Kex] Maintain user info to keep the player skin. 
			char userinfo[MAX_INFO_STRING];
			memcpy(userinfo, game.clients[i].pers.userinfo, sizeof(userinfo));

			game.clients[i].pers = game.clients[i].resp.coop_respawn = {};
			g_edicts[i + 1].health = 0; // this should trip the power armor, etc to reset as well

			memcpy(game.clients[i].pers.userinfo, userinfo, sizeof(userinfo));
			memcpy(game.clients[i].resp.coop_respawn.userinfo, userinfo, sizeof(userinfo));
		}
	}

	// [Paril-KEX] end of unit, so clear level trackers
	if (level.intermission_eou) {
		game.level_entries = {};

		// give all players their lives back
		if (g_coop_enable_lives->integer)
			for (auto player : active_players())
				player->client->pers.lives = g_coop_num_lives->integer + 1;
	}
	/*
	if (Match_NextMap())
		return;
		*/
	if (level.changemap == nullptr) {
		gi.Com_Error("Got null changemap when trying to exit level. Was a trigger_changelevel configured correctly?");
		return;
	}

	// for N64 mainly, but if we're directly changing to "victorXXX.pcx" then
	// end game
	size_t start_offset = (level.changemap[0] == '*' ? 1 : 0);

	if (strlen(level.changemap) > (6 + start_offset) &&
		!Q_strncasecmp(level.changemap + start_offset, "victor", 6) &&
		!Q_strncasecmp(level.changemap + strlen(level.changemap) - 4, ".pcx", 4))
		gi.AddCommandString(G_Fmt("endgame \"{}\"\n", level.changemap + start_offset).data());
	else
		gi.AddCommandString(G_Fmt("gamemap \"{}\"\n", level.changemap).data());

	level.changemap = nullptr;
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
	for (auto player : active_players())
		if (player->health <= 0 && !player->client->pers.lives)
			return true;

	return false;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
static inline void G_RunFrame_(bool main_loop) {
	level.in_frame = true;

	// cancel vote if timed out
	CheckVote();

	// for tracking changes
	CheckCvars();

	Bot_UpdateDebug();

	level.time += FRAME_TIME_MS;

	if (level.intermission_fading) {
		if (level.intermission_fade_time > level.time) {
			float alpha = clamp(1.0f - (level.intermission_fade_time - level.time - 300_ms).seconds(), 0.f, 1.f);

			for (auto player : active_players())
				player->client->ps.screen_blend = { 0, 0, 0, alpha };
		} else {
			level.intermission_fade = level.intermission_fading = false;
			ExitLevel();
		}

		level.in_frame = false;

		return;
	}

	edict_t *ent;

	// exit intermissions

	if (level.intermission_exit) {
		ExitLevel();
		level.in_frame = false;
		return;
	}

	// reload the map start save if restart time is set (all players are dead)
	if (level.coop_level_restart_time > 0_ms && level.time > level.coop_level_restart_time) {
		ClientEndServerFrames();
		gi.AddCommandString("restart_level\n");
	}

	// clear client coop respawn states; this is done
	// early since it may be set multiple times for different
	// players
	if (coop->integer && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
		for (auto player : active_players()) {
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

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (size_t i = 0; i < globals.num_edicts; i++, ent++) {
		if (!ent->inuse) {
			// defer removing client info so that disconnected, etc works
			if (i > 0 && i <= game.maxclients) {
				if (ent->timestamp && level.time < ent->timestamp) {
					int32_t playernum = ent - g_edicts - 1;
					gi.configstring(CS_PLAYERSKINS + playernum, "");
					ent->timestamp = 0_ms;
				}
			}
			continue;
		}

		level.current_entity = ent;

		// Paril: RF_BEAM entities update their old_origin by hand.
		if (!(ent->s.renderfx & RF_BEAM))
			ent->s.old_origin = ent->s.origin;

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount)) {
			contents_t mask = G_GetClipMask(ent);

			if (!(ent->flags & (FL_SWIM | FL_FLY)) && (ent->svflags & SVF_MONSTER)) {
				ent->groundentity = nullptr;
				M_CheckGround(ent, mask);
			} else {
				// if it's still 1 point below us, we're good
				trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin + ent->gravityVector, ent,
					mask);

				if (tr.startsolid || tr.allsolid || tr.ent != ent->groundentity)
					ent->groundentity = nullptr;
				else
					ent->groundentity_linkcount = ent->groundentity->linkcount;
			}
		}

		Entity_UpdateState(ent);

		if (i > 0 && i <= game.maxclients) {
			ClientBeginServerFrame(ent);
			continue;
		}

		G_RunEntity(ent);
	}

	// see if it is time to do a match restart
	CheckWarmupState();
	CheckCountdown();

	CheckRoundState();

	// see if it is time to end a deathmatch
	CheckExitRules();
	//gi.Com_PrintFmt("intermission ---> {} {}\n", level.intermission_time ? "time" : "", level.intermission_queued ? "queued" : "");

	// see if needpass needs updated
	CheckNeedPass();

	if (coop->integer && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
		// rarely, we can see a flash of text if all players respawned
		// on some other player, so if everybody is now alive we'll reset
		// back to empty
		bool reset_coop_respawn = true;

		for (auto player : active_players()) {
			if (player->health >= 0) {
				reset_coop_respawn = false;
				break;
			}
		}

		if (reset_coop_respawn) {
			for (auto player : active_players())
				player->client->coop_respawn_state = COOP_RESPAWN_NONE;
		}
	}

	// build the playerstate_t structures for all players
	ClientEndServerFrames();

	// [Paril-KEX] if not in intermission and player 1 is loaded in
	// the game as an entity, increase timer on current entry
	if (level.entry && !level.intermission_time && g_edicts[1].inuse && g_edicts[1].client->pers.connected)
		level.entry->time += FRAME_TIME_S;

	// [Paril-KEX] run monster pains now
	for (size_t i = 0; i < globals.num_edicts + 1 + game.maxclients + BODY_QUEUE_SIZE; i++) {
		edict_t *e = &g_edicts[i];

		if (!e->inuse || !(e->svflags & SVF_MONSTER))
			continue;

		M_ProcessPain(e);
	}

	level.in_frame = false;
}

static inline bool G_AnyPlayerSpawned() {
	for (auto player : active_players())
		if (player->client && player->client->pers.spawned)
			return true;

	return false;
}

void G_RunFrame(bool main_loop) {
	if (main_loop && !G_AnyPlayerSpawned())
		return;

	for (size_t i = 0; i < g_frames_per_frame->integer; i++)
		G_RunFrame_(main_loop);

	// match details.. only bother if there's at least 1 player in-game
	// and not already end of game
	if (G_AnyPlayerSpawned() && !level.intermission_time) {
		constexpr gtime_t MATCH_REPORT_TIME = 45_sec;

		if (level.time - level.next_match_report > MATCH_REPORT_TIME) {
			level.next_match_report = level.time + MATCH_REPORT_TIME;
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
	for (size_t i = 0; i < globals.num_edicts; i++)
		g_edicts[i].s.event = EV_NONE;

	for (auto player : active_players())
		player->client->ps.stats[STAT_HIT_MARKER] = 0;

	globals.server_flags &= ~SERVER_FLAG_INTERMISSION;

	if (level.intermission_time) {
		globals.server_flags |= SERVER_FLAG_INTERMISSION;
	}
}
