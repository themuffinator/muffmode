// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include <optional>

cvar_t *g_horde;

enum class horde_state_t {
	warmup,
	spawning,
	cleanup,
	rest
};

static struct {
	gtime_t			warm_time = 10_sec;
	horde_state_t	state = horde_state_t::warmup;

	gtime_t			monster_spawn_time;
	int32_t			num_to_spawn;
	int32_t			level;
} g_horde_local;

static void Horde_InitLevel(int32_t lvl) {
	g_horde_local.level = lvl;
	g_horde_local.num_to_spawn = 10 + (lvl * 2);
	g_horde_local.monster_spawn_time = level.time + random_time(1_sec, 3_sec);
}

bool G_IsDeathmatch() {
	return deathmatch->integer && !g_horde->integer;
}

bool G_IsCooperative() {
	return coop->integer || g_horde->integer;
}

struct weighted_item_t;

using weight_adjust_func_t = void(*)(const weighted_item_t &item, float &weight);

void adjust_weight_health(const weighted_item_t &item, float &weight);
void adjust_weight_weapon(const weighted_item_t &item, float &weight);
void adjust_weight_ammo(const weighted_item_t &item, float &weight);
void adjust_weight_armor(const weighted_item_t &item, float &weight);

constexpr struct weighted_item_t {
	const char *classname;
	int32_t					min_level = -1, max_level = -1;
	float					weight = 1.0f;
	weight_adjust_func_t	adjust_weight = nullptr;
} items[] = {
	{ "item_health_small" },

	{ "item_health", -1, -1, 1.0f, adjust_weight_health },
	{ "item_health_large", -1, -1, 0.85f, adjust_weight_health },

	{ "item_armor_shard" },
	{ "item_armor_jacket", -1, 4, 0.65f, adjust_weight_armor },
	{ "item_armor_combat", 2, -1, 0.62f, adjust_weight_armor },
	{ "item_armor_body", 4, -1, 0.35f, adjust_weight_armor },

	{ "weapon_shotgun", -1, -1, 0.98f, adjust_weight_weapon },
	{ "weapon_supershotgun", 2, -1, 1.02f, adjust_weight_weapon },
	{ "weapon_machinegun", -1, -1, 1.05f, adjust_weight_weapon },
	{ "weapon_chaingun", 3, -1, 1.01f, adjust_weight_weapon },
	{ "weapon_grenadelauncher", 4, -1, 0.75f, adjust_weight_weapon },

	{ "ammo_shells", -1, -1, 1.25f, adjust_weight_ammo },
	{ "ammo_bullets", -1, -1, 1.25f, adjust_weight_ammo },
	{ "ammo_grenades", 2, -1, 1.25f, adjust_weight_ammo },
};

void adjust_weight_health(const weighted_item_t &item, float &weight) {}

void adjust_weight_weapon(const weighted_item_t &item, float &weight) {}

void adjust_weight_ammo(const weighted_item_t &item, float &weight) {}

void adjust_weight_armor(const weighted_item_t &item, float &weight) {}

constexpr weighted_item_t monsters[] = {
	{ "monster_soldier_light", -1, 2, 1.50f },
	{ "monster_soldier", -1, 6, 0.85f },
	{ "monster_soldier_ss", 2, 6, 1.01f },
	{ "monster_infantry", 2, 6, 1.15f },
	{ "monster_gunner", 2, -1, 1.15f },
	{ "monster_chick", 3, 8, 1.01f },
	{ "monster_gladiator", 4, -1, 1.2f },
	{ "monster_tank", 5, -1, 0.85f },
};

struct picked_item_t {
	const weighted_item_t *item;
	float weight;
};

static gitem_t *G_HordePickItem() {
	// collect valid items
	static std::array<picked_item_t, q_countof(items)> picked_items;
	static size_t num_picked_items;

	num_picked_items = 0;

	float total_weight = 0;

	for (auto &item : items) {
		if (item.min_level != -1 && g_horde_local.level < item.min_level)
			continue;
		if (item.max_level != -1 && g_horde_local.level > item.max_level)
			continue;

		float weight = item.weight;

		if (item.adjust_weight)
			item.adjust_weight(item, weight);

		if (weight <= 0)
			continue;

		total_weight += weight;
		picked_items[num_picked_items++] = { &item, total_weight };
	}

	if (!total_weight)
		return nullptr;

	float r = frandom() * total_weight;

	for (size_t i = 0; i < num_picked_items; i++)
		if (r < picked_items[i].weight)
			return FindItemByClassname(picked_items[i].item->classname);

	return nullptr;
}

static const char *G_HordePickMonster() {
	// collect valid monsters
	static std::array<picked_item_t, q_countof(items)> picked_monsters;
	static size_t num_picked_monsters;

	num_picked_monsters = 0;

	float total_weight = 0;

	for (auto &item : monsters) {
		if (item.min_level != -1 && g_horde_local.level < item.min_level)
			continue;
		if (item.max_level != -1 && g_horde_local.level > item.max_level)
			continue;

		float weight = item.weight;

		if (item.adjust_weight)
			item.adjust_weight(item, weight);

		if (weight <= 0)
			continue;

		total_weight += weight;
		picked_monsters[num_picked_monsters++] = { &item, total_weight };
	}

	if (!total_weight)
		return nullptr;

	float r = frandom() * total_weight;

	for (size_t i = 0; i < num_picked_monsters; i++)
		if (r < picked_monsters[i].weight)
			return picked_monsters[i].item->classname;

	return nullptr;
}

void Horde_PreInit() {
	g_horde = gi.cvar("horde", "0", CVAR_LATCH);

	if (!g_horde->integer)
		return;

	if (!deathmatch->integer || ctf->integer || teamplay->integer || coop->integer) {
		gi.Com_Print("Horde mode must be DM.\n");
		gi.cvar_set("deathmatch", "1");
		gi.cvar_set("ctf", "0");
		gi.cvar_set("teamplay", "0");
		gi.cvar_set("coop", "0");
	}
}

void Horde_Init() {
	// precache all items
	for (auto &item : itemlist)
		PrecacheItem(&item);

	// all monsters too
	for (auto &monster : monsters) {
		edict_t *e = G_Spawn();
		e->classname = monster.classname;
		ED_CallSpawn(e);
		G_FreeEdict(e);
	}

	g_horde_local.warm_time = level.time + 10_sec;
}

static bool Horde_AllMonstersDead() {
	for (size_t i = 0; i < globals.max_edicts; i++) {
		if (!g_edicts[i].inuse)
			continue;
		else if (g_edicts[i].svflags & SVF_MONSTER) {
			if (!g_edicts[i].deadflag && g_edicts[i].health > 0)
				return false;
		}
	}

	return true;
}

static void Horde_CleanBodies() {
	for (size_t i = 0; i < globals.max_edicts; i++) {
		if (!g_edicts[i].inuse)
			continue;
		else if (g_edicts[i].svflags & SVF_MONSTER) {
			if (g_edicts[i].health <= 0 || g_edicts[i].deadflag || (g_edicts[i].svflags & SVF_DEADMONSTER))
				G_FreeEdict(&g_edicts[i]);
		}
	}
}

void Horde_RunFrame() {
	switch (g_horde_local.state) {
	case horde_state_t::warmup:
		if (g_horde_local.warm_time < level.time) {
			gi.LocBroadcast_Print(PRINT_CENTER, "Warmup ended.\nSpawning monsters.");
			g_horde_local.state = horde_state_t::spawning;
			Horde_InitLevel(1);
		}
		break;

	case horde_state_t::spawning:
		if (g_horde_local.monster_spawn_time <= level.time) {
			edict_t *e = G_Spawn();
			e->classname = G_HordePickMonster();
			select_spawn_result_t result = SelectDeathmatchSpawnPoint(false, true, false);

			if (result.any_valid) {
				e->s.origin = result.spot->s.origin;
				e->s.angles = result.spot->s.angles;
				e->item = G_HordePickItem();
				ED_CallSpawn(e);
				g_horde_local.monster_spawn_time = level.time + random_time(0.2_sec, 1.5_sec);

				e->enemy = &g_edicts[1];
				FoundTarget(e);

				--g_horde_local.num_to_spawn;

				if (!g_horde_local.num_to_spawn) {
					gi.LocBroadcast_Print(PRINT_CENTER, "All monsters spawned.\nClean up time!");
					g_horde_local.state = horde_state_t::cleanup;
					g_horde_local.monster_spawn_time = level.time + 3_sec;
				}
			} else
				g_horde_local.monster_spawn_time = level.time + 1.5_sec;
		}
		break;

	case horde_state_t::cleanup:
		if (g_horde_local.monster_spawn_time < level.time) {
			if (Horde_AllMonstersDead()) {
				gi.LocBroadcast_Print(PRINT_CENTER, "All monsters dead.\nShort rest time.");
				g_horde_local.warm_time = level.time + 10_sec;
				g_horde_local.state = horde_state_t::rest;
			} else
				g_horde_local.monster_spawn_time = level.time + 3_sec;
		}

		break;

	case horde_state_t::rest:
		if (g_horde_local.warm_time < level.time) {
			gi.LocBroadcast_Print(PRINT_CENTER, "Starting next level.");
			g_horde_local.state = horde_state_t::spawning;
			Horde_InitLevel(g_horde_local.level + 1);
			Horde_CleanBodies();
		}
		break;
	}
}
