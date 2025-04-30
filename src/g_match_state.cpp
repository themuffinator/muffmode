
#include "g_local.h"
#include "monsters/m_player.h"	// match starts for death anim

constexpr struct gt_rules_t {
	int						flags = GTF_NONE;
	uint8_t					weaponRespawnDelay = 8;	// in seconds. if 0, weapon stay is on
	bool					holdables = true;			// can hold items such as adrenaline and personal teleporter
	bool					powerupsEnabled = true;	// yes to powerups?
	uint8_t					scoreLimit = 40;
	uint8_t					timeLimit = 10;
	bool					startingHealthBonus = true;
	float					readyUpPercentile = 0.51f;
} gt_rules_t[GT_NUM_GAMETYPES] = {
	/* GT_FFA */ {GTF_FRAGS},
	/* GT_DUEL */ {GTF_FRAGS, 30, false, false, 0},
	/* GT_TDM */ {GTF_TEAMS | GTF_FRAGS, 30, true, true, 100, 20},
	/* GT_CTF */ {GTF_TEAMS | GTF_CTF, 30},
	/* GT_CA */ { },
	/* GT_FREEZE */ { },
	/* GT_STRIKE */ { },
	/* GT_RR */ { },
	/* GT_LMS */ { },
	/* GT_HORDE */ { },
	/* GT_RACE */ { },
	/* GT_BALL */ { }
};

static void Monsters_KillAll() {
	for (size_t i = 0; i < globals.max_entities; i++) {
		if (!g_entities[i].inUse)
			continue;
		if (!(g_entities[i].svFlags & SVF_MONSTER))
			continue;
		FreeEntity(&g_entities[i]);
	}
	level.totalMonsters = 0;
	level.killedMonsters = 0;
}

static void Entities_ItemTeams_Reset() {
	gentity_t	*ent;
	size_t		i;

	gentity_t	*master;
	int			count, choice;

	for (ent = g_entities + 1, i = 1; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse)
			continue;

		if (!ent->item)
			continue;

		if (!ent->team)
			continue;

		if (!ent->teamMaster)
			continue;

		master = ent->teamMaster;

		ent->svFlags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		gi.linkentity(ent);

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

		choice = irandom(count);
		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}
}

/*
============
Entities_Reset

Reset clients and items
============
*/
static void Entities_Reset(bool reset_players, bool reset_ghost, bool reset_score) {
	gentity_t *ent;
	size_t	i;

	// reset the players
	if (reset_players) {
		for (auto ec : active_clients()) {
			ec->client->resp.ctf_state = 0;
			if (reset_score)
				ec->client->resp.score = 0;
			if (reset_ghost)
				ec->client->resp.ghost = nullptr;

			if (ClientIsPlaying(ec->client)) {
				if (reset_ghost) {
					// make up a ghost code
					Match_Ghost_Assign(ec);
				}
				//if (!ec->client->eliminated) {
				Weapon_Grapple_DoReset(ec->client);
				/*
				ec->client->ps.pmove.pm_type = PM_DEAD;

				*/
				//ec->client->animPriority = ANIM_DEATH;
				//ec->s.frame = FRAME_death308 - 1;
				//ec->client->animEnd = FRAME_death308;
				ec->client->eliminated = false;
				ec->moveType = MOVETYPE_NOCLIP;
				ec->deadFlag = true;

				ec->client->ps.gunindex = 0;
				ec->client->ps.gunskin = 0;
				ec->svFlags = SVF_NOCLIENT;
				ec->flags &= ~FL_GODMODE;

				ec->client->sess.playStartTime = level.time;
				ec->client->respawn_time = level.time;
				ec->client->pers.last_spawn_time = level.time;
				//ec->client->pers.health = -999;

				memset(&ec->client->sess.match, 0, sizeof(ec->client->sess.match));

				gi.linkentity(ec);
				//}
			}
		}

		CalculateRanks();
	}

	// reset the level items
	Tech_Reset();
	CTF_ResetFlags();

	Monsters_KillAll();

	Entities_ItemTeams_Reset();

	// reset item spawns and gibs/corpses, remove dropped items and projectiles
	for (ent = g_entities + 1, i = 1; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse)
			continue;

		if (Q_strcasecmp(ent->className, "bodyque") == 0 || Q_strcasecmp(ent->className, "gib") == 0) {
			ent->svFlags = SVF_NOCLIENT;
			ent->takeDamage = false;
			ent->solid = SOLID_NOT;
			gi.unlinkentity(ent);
			FreeEntity(ent);
		} else if ((ent->svFlags & SVF_PROJECTILE) || (ent->clipMask & CONTENTS_PROJECTILECLIP)) {
			FreeEntity(ent);
		} else if (ent->item) {
			// already processed in CTF_ResetFlags()
			if (ent->item->id == IT_FLAG_RED || ent->item->id == IT_FLAG_BLUE)
				continue;

			if (ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)) {
				//FreeEntity(ent);
				ent->nextThink = level.time;
			} else {
				// powerups don't spawn in for a while
				if (ent->item->flags & IF_POWERUP) {
					if (g_quadhog->integer && ent->item->id == IT_POWERUP_QUAD) {
						FreeEntity(ent);
						QuadHog_SetupSpawn(5_sec);
					} else {
						ent->svFlags |= SVF_NOCLIENT;
						ent->solid = SOLID_NOT;

						ent->nextThink = level.time + gtime_t::from_sec(irandom(30, 60));
						//if (!ent->think)
						ent->think = RespawnItem;
					}
					continue;
				} else {
					if (ent->svFlags & (SVF_NOCLIENT | SVF_RESPAWNING) || ent->solid == SOLID_NOT) {
						gtime_t t = 0_sec;
						if (ent->random) {
							t += gtime_t::from_ms((crandom() * ent->random) * 1000);
							if (t < FRAME_TIME_MS) {
								t = FRAME_TIME_MS;
							}
						}
						//if (ent->item->id == IT_HEALTH_MEGA)
						ent->think = RespawnItem;
						ent->nextThink = level.time + t;
					}
				}
			}
		}
	}
}

// =================================================

static gentity_t *FindClosestPlayerToPoint(vec3_t point) {
	float	bestplayerdistance;
	vec3_t	v;
	float	playerdistance;
	gentity_t *closest = nullptr;

	bestplayerdistance = 9999999;

	for (auto ec : active_clients()) {
		if (ec->health <= 0 || ec->client->eliminated)
			continue;

		v = point - ec->s.origin;
		playerdistance = v.length();

		if (playerdistance < bestplayerdistance) {
			bestplayerdistance = playerdistance;
			closest = ec;
		}
	}

	return closest;
}

struct weighted_item_t;

using weight_adjust_func_t = void(*)(const weighted_item_t &item, float &weight);

void adjust_weight_health(const weighted_item_t &item, float &weight);
void adjust_weight_weapon(const weighted_item_t &item, float &weight);
void adjust_weight_ammo(const weighted_item_t &item, float &weight);
void adjust_weight_armor(const weighted_item_t &item, float &weight);

constexpr struct weighted_item_t {
	const char *className;
	int32_t					min_level = -1, max_level = -1;
	float					weight = 1.0f;
	float					lvl_w_adjust = 0;
	int						flags;
	item_id_t				item[4];
	weight_adjust_func_t	adjust_weight = nullptr;
} items[] = {
	{ "item_health_small" },

	{ "item_health",				-1,	-1, 1.0f,	0,		0,		{ IT_NULL },					adjust_weight_health },
	{ "item_health_large",			-1,	-1, 0.85f,	0,		0,		{ IT_NULL },					adjust_weight_health },

	{ "item_armor_shard" },
	{ "item_armor_jacket",			-1,	4,	0.65f,	0,		0,		{ IT_NULL },					adjust_weight_armor },
	{ "item_armor_combat",			2,	-1, 0.62f,	0,		0,		{ IT_NULL },					adjust_weight_armor },
	{ "item_armor_body",			4,	-1, 0.35f,	0,		0,		{ IT_NULL },					adjust_weight_armor },

	{ "weapon_shotgun",				-1,	-1, 0.98f,	0,		0,		{ IT_NULL },					adjust_weight_weapon },
	{ "weapon_supershotgun",		2,	-1, 1.02f,	0,		0,		{ IT_NULL },					adjust_weight_weapon },
	{ "weapon_machinegun",			-1,	-1, 1.05f,	0,		0,		{ IT_NULL },					adjust_weight_weapon },
	{ "weapon_chaingun",			3,	-1, 1.01f,	0,		0,		{ IT_NULL },					adjust_weight_weapon },
	{ "weapon_grenadelauncher",		4,	-1, 0.75f,	0,		0,		{ IT_NULL },					adjust_weight_weapon },

	{ "ammo_shells",				-1,	-1,	1.25f,	0,		0,		{ IT_NULL },					adjust_weight_ammo },
	{ "ammo_bullets",				-1,	-1,	1.25f,	0,		0,		{ IT_NULL },					adjust_weight_ammo },
	{ "ammo_grenades",				2,	-1,	1.25f,	0,		0,		{ IT_NULL },					adjust_weight_ammo },
};

void adjust_weight_health(const weighted_item_t &item, float &weight) {}

void adjust_weight_weapon(const weighted_item_t &item, float &weight) {}

void adjust_weight_ammo(const weighted_item_t &item, float &weight) {}

void adjust_weight_armor(const weighted_item_t &item, float &weight) {}

//	className,						min_level, max_level, weight, lvl_w_adjust, flags, items
constexpr weighted_item_t monsters[] = {
	{ "monster_soldier_light",		-1,	7,	1.50f,	-0.45f,		MF_GROUND,				{ IT_HEALTH_SMALL } },
	{ "monster_soldier",			-1,	7,	0.85f,	-0.25f,		MF_GROUND,				{ IT_AMMO_BULLETS_SMALL, IT_HEALTH_SMALL } },
	{ "monster_soldier_ss",			2,	7,	1.01f,	-0.125f,	MF_GROUND,				{ IT_AMMO_SHELLS_SMALL, IT_HEALTH_SMALL } },
	{ "monster_soldier_hypergun",	2,	9,	1.2f,	0.15f,		MF_GROUND,				{ IT_AMMO_CELLS_SMALL, IT_HEALTH_SMALL } },
	{ "monster_soldier_lasergun",	3,	9,	1.15f,	0.2f,		MF_GROUND,				{ IT_AMMO_CELLS_SMALL, IT_HEALTH_SMALL } },
	{ "monster_soldier_ripper",		3,	9,	1.25f,	0.25f,		MF_GROUND,				{ IT_AMMO_CELLS_SMALL, IT_HEALTH_SMALL } },
	{ "monster_infantry",			3,	16,	1.05f,	0.125f,		MF_GROUND,				{ IT_AMMO_BULLETS_SMALL, IT_AMMO_BULLETS } },
	{ "monster_gunner",				4,	16,	1.08f,	0.5f,		MF_GROUND,				{ IT_AMMO_GRENADES, IT_AMMO_BULLETS_SMALL } },
	{ "monster_berserk",			4,	16,	1.05f,	0.1f,		MF_GROUND,				{ IT_ARMOR_SHARD } },
	//{ "monster_flipper",			4,	8,	0.85f,	-0.15f,		MF_WATER,				{ IT_NULL } },
	{ "monster_parasite",			5,	16,	1.04f,	-0.08f,		MF_GROUND,				{ IT_NULL } },
	{ "monster_gladiator",			5,	16,	1.07f,	0.3f,		MF_GROUND,				{ IT_AMMO_SLUGS } },
	{ "monster_gekk",				6,	16,	0.99f,	-0.15f,		MF_GROUND | MF_WATER,	{ IT_NULL } },
	{ "monster_brain",				6,	16,	0.95f,	0,			MF_GROUND,				{ IT_AMMO_CELLS_SMALL } },
	{ "monster_flyer",				6,	16,	0.92f,	0.15f,		MF_GROUND | MF_AIR,		{ IT_AMMO_CELLS_SMALL } },
	{ "monster_floater",			7,	16,	0.9f,	0,			MF_GROUND | MF_AIR,		{ IT_NULL } },
	{ "monster_mutant",				7,	16,	0.85f,	0,			MF_GROUND,				{ IT_NULL } },
	{ "monster_hover",				8,	16,	0.8f,	0,			MF_GROUND | MF_AIR,		{ IT_NULL } },
	{ "monster_guncmdr",			8,	-1,	0,		0.125f,		MF_GROUND | MF_MEDIUM,	{ IT_AMMO_GRENADES, IT_AMMO_BULLETS_SMALL, IT_AMMO_BULLETS, IT_AMMO_CELLS_SMALL } },
	{ "monster_chick",				9,	20,	1.01f,	-0.05f,		MF_GROUND,				{ IT_AMMO_ROCKETS_SMALL, IT_AMMO_ROCKETS } },
	{ "monster_daedalus",			9,	-1,	0.99f,	0.05f,		MF_GROUND | MF_AIR,		{ IT_AMMO_CELLS_SMALL } },
	{ "monster_medic",				10,	16,	0.95f,	-0.05f,		MF_GROUND,				{ IT_HEALTH_SMALL, IT_HEALTH_MEDIUM } },
	{ "monster_tank",				11,	-1,	0.85f,	0,			MF_GROUND | MF_MEDIUM,	{ IT_AMMO_ROCKETS } },
	{ "monster_chick_heat",			12,	-1,	0.87f,	0.065f,		MF_GROUND,				{ IT_AMMO_CELLS_SMALL, IT_AMMO_CELLS } },
	{ "monster_tank_commander",		12,	-1,	0.45f,	0.16f,		MF_GROUND | MF_MEDIUM,	{ IT_AMMO_ROCKETS_SMALL, IT_AMMO_BULLETS_SMALL, IT_AMMO_ROCKETS, IT_AMMO_BULLETS } },
	{ "monster_medic_commander",	13,	-1,	0.4f,	0.15f,		MF_GROUND | MF_MEDIUM,	{ IT_AMMO_CELLS_SMALL, IT_HEALTH_MEDIUM, IT_HEALTH_LARGE } },
	{ "monster_kamikaze",			13,	-1,	0.85f,	0.04f,		MF_GROUND | MF_AIR,		{ IT_NULL } },
	/*
	{ "monster_boss2",				0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA, IT_AMMO_BULLETS_LARGE, IT_AMMO_ROCKETS } },	// hornet
	{ "monster_jorg",				0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA, IT_AMMO_CELLS_LARGE } },
	{ "monster_makron",				0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA, IT_AMMO_CELLS_LARGE } },
	{ "monster_guardian",			0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA } },
	{ "monster_arachnid",			0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA } },
	{ "monster_boss5",				0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA, IT_AMMO_BULLETS_LARGE, IT_AMMO_CELLS, IT_AMMO_GRENADES } },	// super tank
	{ "monster_carrier",			0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_AMMO_BULLETS_LARGE, IT_AMMO_SLUGS, IT_AMMO_GRENADES, IT_POWERUPS_QUAD } },
	{ "monster_widow",				0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA, IT_AMMO_ROUNDS, IT_POWERUPS_QUAD } },
	{ "monster_widow2",				0,	0,	0,		0,			MF_GROUND | MF_BOSS,	{ IT_HEALTH_MEGA, IT_AMMO_ROUNDS, IT_POWERUPS_QUAD } },
	*/
};

struct picked_item_t {
	const weighted_item_t *item;
	float weight;
};

static gitem_t *Horde_PickItem() {
	// collect valid items
	static std::array<picked_item_t, q_countof(items)> picked_items;
	static size_t num_picked_items;

	num_picked_items = 0;

	float total_weight = 0;

	for (auto &item : items) {
		if (item.min_level != -1 && level.round_number < item.min_level)
			continue;
		if (item.max_level != -1 && level.round_number > item.max_level)
			continue;

		float weight = item.weight + ((level.round_number - item.min_level) * item.lvl_w_adjust);

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
			return FindItemByClassname(picked_items[i].item->className);

	return nullptr;
}

static const char *Horde_PickMonster() {
	// collect valid monsters
	static std::array<picked_item_t, q_countof(items)> picked_monsters;
	static size_t num_picked_monsters;

	num_picked_monsters = 0;

	float total_weight = 0;

	for (auto &monster : monsters) {
		if (monster.min_level != -1 && level.round_number < monster.min_level)
			continue;
		if (monster.max_level != -1 && level.round_number > monster.max_level)
			continue;

		float weight = monster.weight + ((level.round_number - monster.min_level) * monster.lvl_w_adjust);

		if (monster.adjust_weight)
			monster.adjust_weight(monster, weight);

		if (weight <= 0)
			continue;

		total_weight += weight;
		picked_monsters[num_picked_monsters++] = { &monster, total_weight };
	}

	if (!total_weight)
		return nullptr;

	float r = frandom() * total_weight;

	for (size_t i = 0; i < num_picked_monsters; i++)
		if (r < picked_monsters[i].weight)
			return picked_monsters[i].item->className;

	return nullptr;
}

static void Horde_RunSpawning() {
	if (notGT(GT_HORDE))
		return;

	bool warmup = level.match_state == MatchState::MATCH_WARMUP_DEFAULT || level.match_state == MatchState::MATCH_WARMUP_READYUP;

	if (!warmup && level.round_state != RoundState::ROUND_IN_PROGRESS)
		return;

	if (warmup && (level.totalMonsters - level.killedMonsters >= 30))
		return;

	if (level.horde_all_spawned)
		return;

	if (level.horde_monster_spawn_time <= level.time) {
		gentity_t *e = Spawn();
		e->className = Horde_PickMonster();
		select_spawn_result_t result = SelectDeathmatchSpawnPoint(nullptr, vec3_origin, false, true, false, false);

		if (result.any_valid) {
			e->s.origin = result.spot->s.origin;
			e->s.angles = result.spot->s.angles;

			e->item = Horde_PickItem();
			ED_CallSpawn(e);
			level.horde_monster_spawn_time = warmup ? level.time + 5_sec : level.time + random_time(0.3_sec, 0.5_sec);

			e->enemy = FindClosestPlayerToPoint(e->s.origin);
			if (e->enemy)
				FoundTarget(e);

			if (!warmup) {
				level.horde_num_monsters_to_spawn--;

				if (!level.horde_num_monsters_to_spawn) {
					//gi.LocBroadcast_Print(PRINT_CENTER, "All monsters spawned.\nClean up time!");
					level.horde_all_spawned = true;
				}
			}
		} else
			level.horde_monster_spawn_time = warmup ? level.time + 5_sec : level.time + 1_sec;
	}
}

void Horde_Init() {
	// this crashes the game
/*
	if (notGT(GT_HORDE))
		return;

	// precache all items
	for (auto &item : itemList)
		PrecacheItem(&item);

	// all monsters too
	for (auto &monster : monsters) {
		gentity_t *e = Spawn();
		e->className = monster.className;
		ED_CallSpawn(e);
		FreeEntity(e);
	}
	*/
}

static bool Horde_AllMonstersDead() {
	for (size_t i = 0; i < globals.max_entities; i++) {
		if (!g_entities[i].inUse)
			continue;
		else if (g_entities[i].svFlags & SVF_MONSTER) {
			if (!g_entities[i].deadFlag && g_entities[i].health > 0)
				return false;
		}
	}

	return true;
}

static void RoundAnnounceWin(team_t team, const char *reason) {
	G_AdjustTeamScore(team, 1);
	gi.LocBroadcast_Print(PRINT_CENTER, "{} wins the round!\n({})\n", Teams_TeamName(team), reason);
	AnnouncerSound(world, team == TEAM_RED ? "red_wins_round" : "blue_wins_round");
}

static void RoundAnnounceDraw() {
	gi.LocBroadcast_Print(PRINT_CENTER, "Round draw!\n");
}

static void CheckRoundEliminationCA() {
	int redAlive = 0, blueAlive = 0;
	for (auto ec : active_players()) {
		if (ec->health <= 0)
			continue;
		switch (ec->client->sess.team) {
		case TEAM_RED: redAlive++; break;
		case TEAM_BLUE: blueAlive++; break;
		}
	}

	if (redAlive && !blueAlive) {
		RoundAnnounceWin(TEAM_RED, "eliminated blue team");
		Round_End();
	} else if (blueAlive && !redAlive) {
		RoundAnnounceWin(TEAM_BLUE, "eliminated red team");
		Round_End();
	}
}

static void CheckRoundTimeLimitCA() {
	if (level.num_living_red > level.num_living_blue) {
		RoundAnnounceWin(TEAM_RED, "players remaining");
	} else if (level.num_living_blue > level.num_living_red) {
		RoundAnnounceWin(TEAM_BLUE, "players remaining");
	} else {
		int healthRed = 0, healthBlue = 0;
		for (auto ec : active_players()) {
			if (ec->health <= 0) continue;
			switch (ec->client->sess.team) {
			case TEAM_RED: healthRed += ec->health; break;
			case TEAM_BLUE: healthBlue += ec->health; break;
			}
		}
		if (healthRed > healthBlue) {
			RoundAnnounceWin(TEAM_RED, "total health");
		} else if (healthBlue > healthRed) {
			RoundAnnounceWin(TEAM_BLUE, "total health");
		} else {
			RoundAnnounceDraw();
		}
	}
	Round_End();
}

static void CheckRoundHorde() {
	Horde_RunSpawning();
	if (level.horde_all_spawned && !(level.totalMonsters - level.killedMonsters)) {
		gi.LocBroadcast_Print(PRINT_CENTER, "Monsters eliminated!\n");
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);
		Round_End();
	}
}

static void CheckRoundRR() {
	if (!level.num_playing_red || !level.num_playing_blue) {
		gi.Broadcast_Print(PRINT_CENTER, "Round Ends!\n");
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);
		if (level.round_number + 1 >= roundlimit->integer) {
			QueueIntermission("MATCH ENDED", false, false);
		} else {
			Round_End();
		}
	}
}

static void CheckRoundStrikeTimeLimit() {
	if (level.strike_flag_touch) {
		RoundAnnounceWin(level.strike_red_attacks ? TEAM_RED : TEAM_BLUE, "scored a point");
	} else {
		gi.LocBroadcast_Print(PRINT_CENTER, "Turn has ended.\n{} successfully defended!", Teams_TeamName(!level.strike_red_attacks ? TEAM_RED : TEAM_BLUE));
	}
	Round_End();
}

static void CheckRoundStrikeStartTurn() {
	if (!level.strike_turn_red && level.strike_red_attacks) {
		level.strike_turn_red = true;
	} else if (!level.strike_turn_blue && !level.strike_red_attacks) {
		level.strike_turn_blue = true;
	} else {
		level.strike_turn_red = level.strike_red_attacks;
		level.strike_turn_blue = !level.strike_red_attacks;
	}
}

static gclient_t *GetNextQueuedPlayer() {
	gclient_t *next = nullptr;
	for (auto ec : active_clients()) {
		if (ec->client->sess.versusQueued && !ClientIsPlaying(ec->client)) {
			if (!next || ec->client->sess.teamJoinTime < next->sess.teamJoinTime)
				next = ec->client;
		}
	}
	return next;
}

static bool Versus_AddPlayer() {
	if (GTF(GTF_1V1) && level.num_playing_clients >= 2)
		return false;
	if (level.match_state > MatchState::MATCH_WARMUP_DEFAULT || level.intermissionTime || level.intermissionQueued)
		return false;

	gclient_t *next = GetNextQueuedPlayer();
	if (!next)
		return false;

	SetTeam(&g_entities[next - game.clients + 1], TEAM_FREE, false, true, false);

	return true;
}

void Gauntlet_RemoveLoser() {
	if (notGT(GT_GAUNTLET) || level.num_playing_clients != 2)
		return;

	gentity_t *loser = &g_entities[level.sorted_clients[1] + 1];
	if (!loser || !loser->client || !loser->client->pers.connected)
		return;
	if (loser->client->sess.team != TEAM_FREE)
		return;

	if (g_verbose->integer)
		gi.Com_PrintFmt("Gauntlet: Moving the loser, {} to end of queue.\n", loser->client->sess.netName);

	SetTeam(loser, TEAM_NONE, false, true, false);
}

void Gauntlet_MatchEnd_AdjustScores() {
	if (notGT(GT_GAUNTLET))
		return;
	if (level.num_playing_clients < 2)
		return;

	int winnerNum = level.sorted_clients[0];
	if (game.clients[winnerNum].pers.connected) {
		game.clients[winnerNum].sess.wins++;
	}
}

static void EnforceDuelRules() {
	if (notGT(GT_DUEL))
		return;

	if (level.num_playing_clients > 2) {
		// Kick or move spectators if too many players
		for (auto ec : active_clients()) {
			if (ClientIsPlaying(ec->client)) {
				// Allow the first two
				continue;
			}
			if (ec->client->sess.team != TEAM_SPECTATOR) {
				SetTeam(ec, TEAM_SPECTATOR, false, true, false);
				gi.LocClient_Print(ec, PRINT_HIGH, "This is a Duel match (1v1 only).\nYou have been moved to spectator.");
			}
		}
	}
}

/*
=============
Round_StartNew
=============
*/
static bool Round_StartNew() {
	if (notGTF(GTF_ROUNDS)) {
		level.round_state = RoundState::ROUND_NONE;
		level.round_state_timer = 0_sec;
		return false;
	}

	bool horde = GT(GT_HORDE);

	level.round_state = RoundState::ROUND_COUNTDOWN;
	level.round_state_timer = level.time + 10_sec;
	level.countdown_check = 0_sec;

	if (!horde)
		Entities_Reset(!horde, false, false);

	if (GT(GT_STRIKE)) {
		level.strike_red_attacks ^= true;
		level.strike_flag_touch = false;

		int round_num;
		if (level.round_number && (!level.strike_turn_red && level.strike_turn_blue ||
			level.strike_turn_red && !level.strike_turn_blue))
			round_num = level.round_number;
		else {
			round_num = level.round_number + 1;
		}
		BroadcastTeamMessage(TEAM_RED, PRINT_CENTER, G_Fmt("Your team is on {}!\nRound {} - Begins in...", level.strike_red_attacks ? "OFFENSE" : "DEFENSE", round_num).data());
		BroadcastTeamMessage(TEAM_BLUE, PRINT_CENTER, G_Fmt("Your team is on {}!\nRound {} - Begins in...", !level.strike_red_attacks ? "OFFENSE" : "DEFENSE", round_num).data());
	} else {
		int round_num;

		if (horde && !level.round_number && g_horde_starting_wave->integer > 0)
			round_num = g_horde_starting_wave->integer;
		else
			round_num = level.round_number + 1;

		if (GT(GT_RR) && roundlimit->integer) {
			gi.LocBroadcast_Print(PRINT_CENTER, "{} {} of {}\nBegins in...", horde ? "Wave" : "Round", round_num, roundlimit->integer);
		} else
			gi.LocBroadcast_Print(PRINT_CENTER, "{} {}\nBegins in...", horde ? "Wave" : "Round", round_num);
	}

	AnnouncerSound(world, "round_begins_in");

	return true;
}

/*
=============
Round_End
=============
*/
void Round_End() {
	// reset if not round based
	if (notGTF(GTF_ROUNDS)) {
		level.round_state = RoundState::ROUND_NONE;
		level.round_state_timer = 0_sec;
		return;
	}

	// there must be a round to end
	if (level.round_state != RoundState::ROUND_IN_PROGRESS)
		return;

	level.round_state = RoundState::ROUND_ENDED;
	level.round_state_timer = level.time + 3_sec;
	level.horde_all_spawned = false;
}

/*
============
Match_Start

Starts a match
============
*/

extern void MatchStats_Init();
void Match_Start() {
	if (!deathmatch->integer)
		return;

	level.matchStartTime = level.time;
	level.overtime = 0_sec;

	const char *s = TimeString(timelimit->value ? timelimit->value * 1000 : 0, false, true);
	gi.configstring(CONFIG_MATCH_STATE, s);

	level.match_state = MatchState::MATCH_IN_PROGRESS;
	level.match_state_timer = level.time;
	level.warmup_requisite = WarmupState::WARMUP_REQ_NONE;
	level.warmup_notice_time = 0_sec;

	level.team_scores[TEAM_RED] = level.team_scores[TEAM_BLUE] = 0;

	level.match.totalDeaths = 0;

	memset(level.ghosts, 0, sizeof(level.ghosts));

	Entities_Reset(true, true, true);
	UnReadyAll();

	MatchStats_Init();

	if (GT(GT_STRIKE))
		level.strike_red_attacks = brandom();

	if (Round_StartNew())
		return;

	gi.LocBroadcast_Print(PRINT_CENTER, GT(GT_RACE) ? "GO!" : "FIGHT!");
	//gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);
	AnnouncerSound(world, GT(GT_RACE) ? "go" : "fight");
}

/*
============
Match_Reset
============
*/
void Match_Reset() {
	//if (!g_dm_do_warmup->integer) {
	//	Match_Start();
	//	return;
	//}

	Entities_Reset(true, true, true);
	UnReadyAll();

	level.matchStartTime = 0_sec;
	level.match_state = MatchState::MATCH_WARMUP_DEFAULT;
	level.warmup_requisite = WarmupState::WARMUP_REQ_NONE;
	level.warmup_notice_time = 0_sec;
	level.match_state_timer = 0_sec;
	level.intermissionQueued = 0_sec;
	level.intermissionPreExit = false;
	level.intermissionTime = 0_sec;
	memset(&level.match, 0, sizeof(level.match));

	CalculateRanks();

	gi.LocBroadcast_Print(PRINT_TTS, "The match has been reset.\n");
}

/*
============
CheckDMRoundState
============
*/
static void CheckDMRoundState() {
	if (notGTF(GTF_ROUNDS) || level.match_state != MatchState::MATCH_IN_PROGRESS)
		return;

	if (level.round_state == RoundState::ROUND_NONE || level.round_state == RoundState::ROUND_ENDED) {
		if (level.round_state_timer > level.time)
			return;
		if (GT(GT_RR) && level.round_state == RoundState::ROUND_ENDED)
			TeamShuffle();
		Round_StartNew();
		return;
	}

	if (level.round_state == RoundState::ROUND_COUNTDOWN && level.time >= level.round_state_timer) {
		for (auto ec : active_clients())
			ec->client->latchedButtons = BUTTON_NONE;
		level.round_state = RoundState::ROUND_IN_PROGRESS;
		level.round_state_timer = level.time + gtime_t::from_min(roundtimelimit->value);
		level.round_number++;
		gi.LocBroadcast_Print(PRINT_CENTER, "FIGHT!\n");
		AnnouncerSound(world, "fight");

		if (GT(GT_STRIKE)) {
			CheckRoundStrikeStartTurn();
		}
		return;
	}

	if (level.round_state == RoundState::ROUND_IN_PROGRESS) {
		switch (g_gametype->integer) {
		case GT_CA:     CheckRoundEliminationCA(); break;
		case GT_HORDE:  CheckRoundHorde(); break;
		case GT_RR:     CheckRoundRR(); break;
		}

		if (level.time >= level.round_state_timer) {
			switch (g_gametype->integer) {
			case GT_CA:     CheckRoundTimeLimitCA(); break;
			case GT_STRIKE: CheckRoundStrikeTimeLimit(); break;
				// Additional GTs can be added here
			}
		}
	}
}

/*
=============
ReadyAll
=============
*/
void ReadyAll() {
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		ec->client->resp.ready = true;
	}
}

/*
=============
UnReadyAll
=============
*/
void UnReadyAll() {
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		ec->client->resp.ready = false;
	}
}

/*
=============
CheckReady
=============
*/
static bool CheckReady() {
	if (!g_dm_do_readyup->integer)
		return true;

	uint8_t count_ready, count_humans, count_bots;

	count_ready = count_humans = count_bots = 0;
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		if (ec->svFlags & SVF_BOT || ec->client->sess.is_a_bot) {
			count_bots++;
			continue;
		}

		if (ec->client->resp.ready)
			count_ready++;
		count_humans++;
	}

	// wait if no players at all
	if (!count_humans && !count_bots)
		return true;

	// wait if below minimum players
	if (minplayers->integer > 0 && (count_humans + count_bots) < minplayers->integer)
		return false;

	// start if only bots
	if (!count_humans && count_bots && g_dm_allow_no_humans->integer)
		return true;

	// wait if no ready humans
	if (!count_ready)
		return false;

	// start if over min ready percentile
	if (((float)count_ready / (float)count_humans) * 100.0f >= g_warmup_ready_percentage->value * 100.0f)
		return true;

	return false;
}

/*
=============
AnnounceCountdown
=============
*/
void AnnounceCountdown(int t, gtime_t &checkRef) {
	const gtime_t nextCheck = gtime_t::from_sec(t);
	if (!checkRef || checkRef > nextCheck) {
		static constexpr std::array<std::string_view, 3> labels = {
			"one", "two", "three"
		};
		if (t >= 1 && t <= static_cast<int>(labels.size())) {
			AnnouncerSound(world, labels[t - 1].data());
		}
		checkRef = nextCheck;
	}
}

/*
=============
CheckDMCountdown
=============
*/
static void CheckDMCountdown() {
	// bail out if we're not in a true countdown
	if ((level.match_state != MatchState::MATCH_COUNTDOWN
		&& level.round_state != RoundState::ROUND_COUNTDOWN)
		|| level.intermissionTime) {
		level.countdown_check = 0_sec;
		return;
	}

	// choose the correct base timer
	gtime_t base = (level.round_state == RoundState::ROUND_COUNTDOWN)
		? level.round_state_timer
		: level.match_state_timer;

	int t = (base + 1_sec - level.time).seconds<int>();

	// DEBUG: print current countdown info
	if (g_verbose->integer) {
		gi.Com_PrintFmt("[Countdown] match_state={}, round_state={}, base={}, now={}, countdown={}\n",
			(int)level.match_state,
			(int)level.round_state,
			base.milliseconds(),
			level.time.milliseconds(),
			t
		);
	}

	AnnounceCountdown(t, level.countdown_check);
}

/*
=============
CheckDMMatchEndWarning
=============
*/
static void CheckDMMatchEndWarning(void) {
	if (GTF(GTF_ROUNDS))
		return;

	if (level.match_state != MatchState::MATCH_IN_PROGRESS || !timelimit->value) {
		if (level.matchendwarn_check)
			level.matchendwarn_check = 0_sec;
		return;
	}

	int t = (level.matchStartTime + gtime_t::from_min(timelimit->value) - level.time).seconds<int>();	// +1;

	if (!level.matchendwarn_check || level.matchendwarn_check.seconds<int>() > t) {
		if (t && (t == 30 || t == 20 || t <= 10)) {
			//AnnouncerSound(world, nullptr, G_Fmt("world/{}{}.wav", t, t >= 20 ? "sec" : "").data(), false);
			//gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex(G_Fmt("world/{}{}.wav", t, t >= 20 ? "sec" : "").data()), 1, ATTN_NONE, 0);
			if (t >= 10)
				gi.LocBroadcast_Print(PRINT_HIGH, "{} second warning!\n", t);
		} else if (t == 300 || t == 60) {
			AnnouncerSound(world, G_Fmt("{}_minute", t == 300 ? 5 : 1).data());
		}
		level.matchendwarn_check = gtime_t::from_sec(t);
	}
}

/*
=============
CheckDMWarmupState
=============
*/
static void CheckDMWarmupState() {
	const bool duel = GTF(GTF_1V1);
	const int min_players = duel ? 2 : minplayers->integer;

	// Handle no players
	if (!level.num_playing_clients) {
		if (level.match_state != MatchState::MATCH_NONE) {
			level.match_state = MatchState::MATCH_NONE;
			level.match_state_timer = 0_sec;
			level.warmup_requisite = WarmupState::WARMUP_REQ_NONE;
			level.warmup_notice_time = 0_sec;
		}

		// Pull in idle bots
		for (auto ec : active_clients())
			if (!ClientIsPlaying(ec->client) && (ec->client->sess.is_a_bot || ec->svFlags & SVF_BOT))
				SetTeam(ec, PickTeam(-1), false, false, false);
		return;
	}

	// Pull queued players (if needed) during 1v1
	if (GTF(GTF_1V1) && Versus_AddPlayer())
		return;

	// If warmup disabled and enough players, start match
	if (level.match_state < MatchState::MATCH_COUNTDOWN &&
		!g_dm_do_warmup->integer &&
		level.num_playing_clients >= min_players) {
		Match_Start();
		return;
	}

	// Trigger initial delayed warmup on fresh map
	if (level.match_state == MatchState::MATCH_NONE) {
		level.match_state = MatchState::MATCH_WARMUP_DELAYED;
		level.match_state_timer = level.time + 5_sec;
		level.warmup_requisite = WarmupState::WARMUP_REQ_NONE;
		level.warmup_notice_time = level.time;
		return;
	}

	// Wait for delayed warmup to trigger
	if (level.match_state == MatchState::MATCH_WARMUP_DELAYED &&
		level.match_state_timer > level.time)
		return;

	// Run spawning logic during warmup (e.g., Horde)
	if (level.match_state == MatchState::MATCH_WARMUP_DEFAULT ||
		level.match_state == MatchState::MATCH_WARMUP_READYUP) {
		Horde_RunSpawning();
	}

	// Check for imbalance or missing players
	const bool forceBalance = Teams() && g_teamplay_force_balance->integer;
	const bool teamsImbalanced = forceBalance &&
		std::abs(level.num_playing_red - level.num_playing_blue) > 1;
	const bool notEnoughPlayers =
		(Teams() && (level.num_playing_red < 1 || level.num_playing_blue < 1)) ||
		(duel && level.num_playing_clients != 2) ||
		(!Teams() && !duel && level.num_playing_clients < min_players) ||
		(!g_dm_allow_no_humans->integer && !level.num_playing_human_clients);

	if (teamsImbalanced || notEnoughPlayers) {
		if (level.match_state <= MatchState::MATCH_COUNTDOWN) {
			if (level.match_state == MatchState::MATCH_WARMUP_READYUP)
				UnReadyAll();

			if (level.match_state == MatchState::MATCH_COUNTDOWN) {
				const char *reason = teamsImbalanced ? "teams are imbalanced" : "not enough players";
				gi.LocBroadcast_Print(PRINT_CENTER, "Countdown cancelled: {}\n", reason);
			}

			if (level.match_state != MatchState::MATCH_WARMUP_DEFAULT) {
				level.match_state = MatchState::MATCH_WARMUP_DEFAULT;
				level.match_state_timer = 0_sec;
				level.warmup_requisite = teamsImbalanced ? WarmupState::WARMUP_REQ_BALANCE : WarmupState::WARMUP_REQ_MORE_PLAYERS;
				level.warmup_notice_time = level.time;
			}
		}
		return;
	}

	// If we're in default warmup and ready-up is required
	if (level.match_state == MatchState::MATCH_WARMUP_DEFAULT) {
		if (!g_dm_do_readyup->integer) {
			// Skip to countdown
			level.match_state = MatchState::MATCH_COUNTDOWN;
		} else {
			// Transition to ready-up
			level.match_state = MatchState::MATCH_WARMUP_READYUP;
			level.match_state_timer = 0_sec;
			level.warmup_requisite = WarmupState::WARMUP_REQ_READYUP;
			level.warmup_notice_time = level.time;

			if (!duel) {
				// Pull in bots
				for (auto ec : active_clients())
					if (!ClientIsPlaying(ec->client) && ec->client->sess.is_a_bot)
						SetTeam(ec, PickTeam(-1), false, false, false);
			}

			BroadcastReadyReminderMessage();
			return;
		}
	}

	// Cancel countdown if warmup settings changed
	if (level.match_state <= MatchState::MATCH_COUNTDOWN &&
		g_warmup_countdown->modified_count != level.warmup_modification_count) {
		level.warmup_modification_count = g_warmup_countdown->modified_count;
		level.match_state = MatchState::MATCH_WARMUP_DEFAULT;
		level.match_state_timer = 0_sec;
		level.warmup_requisite = WarmupState::WARMUP_REQ_NONE;
		level.warmup_notice_time = 0_sec;
		level.prepare_to_fight = false;
		return;
	}

	// Ready-up check
	if (level.match_state == MatchState::MATCH_WARMUP_READYUP) {
		if (!CheckReady())
			return;

		level.match_state = MatchState::MATCH_COUNTDOWN;
		level.warmup_requisite = WarmupState::WARMUP_REQ_NONE;
		level.warmup_notice_time = 0_sec;
		Monsters_KillAll();

		if (g_warmup_countdown->integer > 0) {
			level.match_state_timer = level.time + gtime_t::from_sec(g_warmup_countdown->integer);

			if ((duel || (level.num_playing_clients == 2 && g_match_lock->integer)) &&
				game.clients[level.sorted_clients[0]].pers.connected &&
				game.clients[level.sorted_clients[1]].pers.connected) {
				gi.LocBroadcast_Print(PRINT_CENTER, "{} vs {}\nBegins in...",
					game.clients[level.sorted_clients[0]].sess.netName,
					game.clients[level.sorted_clients[1]].sess.netName);
			} else {
				gi.LocBroadcast_Print(PRINT_CENTER, "{}\nBegins in...", level.gametype_name);
			}

			if (!level.prepare_to_fight) {
				const char *sound = (Teams() && level.num_playing_clients >= 4) ? "prepare_your_team" : "prepare_to_fight";
				AnnouncerSound(world, sound);
				level.prepare_to_fight = true;
			}
			return;
		}

		// No delay, start immediately
		level.match_state_timer = 0_sec;
		Match_Start();
		return;
	}

	// Final check: countdown timer expired?
	if (level.match_state == MatchState::MATCH_COUNTDOWN &&
		level.time.seconds() >= level.match_state_timer.seconds()) {
		Match_Start();
	}
}

/*
================
CheckDMEndFrame
================
*/
void CheckDMEndFrame() {
	if (!deathmatch->integer)
		return;

	// see if it is time to do a match restart
	CheckDMWarmupState();     // Manages warmup -> countdown -> match start
	CheckDMCountdown();       // Handles audible/visual countdown
	CheckDMRoundState();      // Handles per-round progression
	CheckDMMatchEndWarning(); // Optional: match-ending warnings

	// see if it is time to end a deathmatch
	CheckDMExitRules();       // Handles intermission and map end

	if (g_verbose->integer) {
		static constexpr const char *MatchStateNames[] = {
			"MATCH_NONE",
			"MATCH_WARMUP_DELAYED",
			"MATCH_WARMUP_DEFAULT",
			"MATCH_WARMUP_READYUP",
			"MATCH_COUNTDOWN",
			"MATCH_IN_PROGRESS",
			"MATCH_ENDED"
		};

		const char *stateName = (static_cast<size_t>(level.match_state) < std::size(MatchStateNames))
			? MatchStateNames[static_cast<size_t>(level.match_state)]
			: "UNKNOWN";

		gi.Com_PrintFmt("MatchState: {}, NumPlayers: {}\n", stateName, level.num_playing_clients);
	}

}

/*
==================
CheckVote
==================
*/
void CheckVote(void) {
	if (!deathmatch->integer)
		return;

	// vote has passed, execute
	if (level.vote_execute_time) {
		if (level.time > level.vote_execute_time)
			Vote_Passed();
		return;
	}

	if (!level.vote_time)
		return;

	if (!level.vote_client)
		return;

	// give it a minimum duration
	if (level.time - level.vote_time < 1_sec)
		return;

	if (level.time - level.vote_time >= 30_sec) {
		gi.LocBroadcast_Print(PRINT_HIGH, "Vote timed out.\n");
		AnnouncerSound(world, "vote_failed");
	} else {
		int halfpoint = level.num_voting_clients / 2;
		if (level.vote_yes > halfpoint) {
			// execute the command, then remove the vote
			gi.LocBroadcast_Print(PRINT_HIGH, "Vote passed.\n");
			level.vote_execute_time = level.time + 3_sec;
			AnnouncerSound(world, "vote_passed");
		} else if (level.vote_no >= halfpoint) {
			// same behavior as a timeout
			gi.LocBroadcast_Print(PRINT_HIGH, "Vote failed.\n");
			AnnouncerSound(world, "vote_failed");
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.vote_time = 0_sec;
}


/*
=================
CheckDMIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.

Adapted from Quake III
=================
*/

static void CheckDMIntermissionExit(void) {
	int ready, not_ready;

	// see which players are ready
	ready = not_ready = 0;
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;

		if (ec->client->sess.is_a_bot)
			ec->client->readyToExit = true;

		if (ec->client->readyToExit)
			ready++;
		else
			not_ready++;
	}

	// vote in progress
	if (level.vote_time || level.vote_execute_time) {
		ready = 0;
		not_ready = 1;
	}

	// never exit in less than five seconds
	if (level.time < level.intermissionTime + 5_sec && !level.exitTime)
		return;

	// if nobody wants to go, clear timer
	// skip this if no players present
	if (!ready && not_ready) {
		level.readyToExit = false;
		return;
	}

	// if everyone wants to go, go now
	if (!not_ready) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if (ready && !level.readyToExit) {
		level.readyToExit = true;
		level.exitTime = level.time + 10_sec;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if (level.time < level.exitTime)
		return;

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
static bool ScoreIsTied(void) {
	if (level.num_playing_clients < 2)
		return false;

	if (Teams() && notGT(GT_RR))
		return level.team_scores[TEAM_RED] == level.team_scores[TEAM_BLUE];

	return game.clients[level.sorted_clients[0]].resp.score == game.clients[level.sorted_clients[1]].resp.score;
}


int GT_ScoreLimit() {
	if (GTF(GTF_ROUNDS))
		return roundlimit->integer;
	if (GT(GT_CTF))
		return capturelimit->integer;
	return fraglimit->integer;
}

const char *GT_ScoreLimitString() {
	if (GT(GT_CTF))
		return "capture";
	if (GTF(GTF_ROUNDS))
		return "round";
	return "frag";
}

/*
=================
CheckDMExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag/capture.
=================
*/
void CheckDMExitRules() {

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (level.intermissionTime) {
		CheckDMIntermissionExit();
		return;
	}

	if (!level.num_playing_clients && noplayerstime->integer && level.time > level.no_players_time + gtime_t::from_min(noplayerstime->integer)) {
		Match_End();
		return;
	}

	if (level.intermissionQueued) {
		if (level.time - level.intermissionQueued >= 1_sec) {
			level.intermissionQueued = 0_ms;
			Match_End();
		}
		return;
	}

	if (level.match_state < MatchState::MATCH_IN_PROGRESS)
		return;

	if (level.time - level.matchStartTime <= FRAME_TIME_MS)
		return;

	if (GT(GT_HORDE)) {
		if ((level.totalMonsters - level.killedMonsters) >= 100) {
			gi.Broadcast_Print(PRINT_CENTER, "DEFEATED!");
			QueueIntermission("OVERRUN BY MONSTERS!", true, false);
			return;
		}
	}

	if (GTF(GTF_ROUNDS) && level.round_state != RoundState::ROUND_ENDED)
		return;

	if (GT(GT_HORDE)) {
		if (roundlimit->integer > 0 && level.round_number >= roundlimit->integer) {
			QueueIntermission(G_Fmt("{} WINS with a final score of {}.", game.clients[level.sorted_clients[0]].sess.netName, game.clients[level.sorted_clients[0]].resp.score).data(), false, false);
			return;
		}
	}

	if (!g_dm_allow_no_humans->integer && !level.num_playing_human_clients) {
		if (!level.endmatch_grace) {
			level.endmatch_grace = level.time;
			return;
		}
		if (level.time > level.endmatch_grace + 200_ms)
			QueueIntermission("No human players remaining.", true, false);
		return;
	}

	if (minplayers->integer > 0 && level.num_playing_clients < minplayers->integer) {
		if (!level.endmatch_grace) {
			level.endmatch_grace = level.time;
			return;
		}
		if (level.time > level.endmatch_grace + 200_ms)
			QueueIntermission("Not enough players remaining.", true, false);
		return;
	}

	bool teams = Teams() && notGT(GT_RR);

	if (teams && g_teamplay_force_balance->integer) {
		if (abs(level.num_playing_red - level.num_playing_blue) > 1) {
			if (g_teamplay_auto_balance->integer) {
				TeamBalance(true);
			} else {
				if (!level.endmatch_grace) {
					level.endmatch_grace = level.time;
					return;
				}
				if (level.time > level.endmatch_grace + 200_ms)
					QueueIntermission("Teams are imbalanced.", true, true);
			}
			return;
		}
	}

	if (timelimit->value) {
		if (notGTF(GTF_ROUNDS) || level.round_state == RoundState::ROUND_ENDED) {
			if (level.time >= level.matchStartTime + gtime_t::from_min(timelimit->value) + level.overtime) {
				// check for overtime
				if (ScoreIsTied()) {
					bool play = false;

					if (GTF(GTF_1V1) && g_dm_overtime->integer > 0) {
						level.overtime += gtime_t::from_sec(g_dm_overtime->integer);
						gi.LocBroadcast_Print(PRINT_CENTER, "Overtime!\n{} added", TimeString(g_dm_overtime->integer * 1000, false, false));
						AnnouncerSound(world, "overtime");
						play = true;
					} else if (!level.suddendeath) {
						gi.LocBroadcast_Print(PRINT_CENTER, "Sudden Death!");
						AnnouncerSound(world, "sudden_death");
						level.suddendeath = true;
						play = true;
					}

					//if (play)
						//gi.positioned_sound(world->s.origin, world, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("world/klaxon2.wav"), 1, ATTN_NONE, 0);
					return;
				}

				// find the winner and broadcast it
				if (teams) {
					if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE]) {
						QueueIntermission(G_Fmt("{} Team WINS with a final score of {} to {}.\n", Teams_TeamName(TEAM_RED), level.team_scores[TEAM_RED], level.team_scores[TEAM_BLUE]).data(), false, false);
						return;
					}
					if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED]) {
						QueueIntermission(G_Fmt("{} Team WINS with a final score of {} to {}.\n", Teams_TeamName(TEAM_BLUE), level.team_scores[TEAM_BLUE], level.team_scores[TEAM_RED]).data(), false, false);
						return;
					}
				} else {
					QueueIntermission(G_Fmt("{} WINS with a final score of {}.", game.clients[level.sorted_clients[0]].sess.netName, game.clients[level.sorted_clients[0]].resp.score).data(), false, false);
					return;
				}

				QueueIntermission("Timelimit hit.", false, false);
				return;
			}
		}
	}

	if (mercylimit->integer > 0) {
		if (teams) {
			if (level.team_scores[TEAM_RED] >= level.team_scores[TEAM_BLUE] + mercylimit->integer) {
				QueueIntermission(G_Fmt("{} hit the mercylimit ({}).", Teams_TeamName(TEAM_RED), mercylimit->integer).data(), true, false);
				return;
			}
			if (level.team_scores[TEAM_BLUE] >= level.team_scores[TEAM_RED] + mercylimit->integer) {
				QueueIntermission(G_Fmt("{} hit the mercylimit ({}).", Teams_TeamName(TEAM_BLUE), mercylimit->integer).data(), true, false);
				return;
			}
		} else {
			if (notGT(GT_HORDE)) {
				gclient_t *cl1, *cl2;

				cl1 = &game.clients[level.sorted_clients[0]];
				cl2 = &game.clients[level.sorted_clients[1]];
				if (cl1 && cl2) {
					if (cl1->resp.score >= cl2->resp.score + mercylimit->integer) {
						QueueIntermission(G_Fmt("{} hit the mercylimit ({}).", cl1->sess.netName, mercylimit->integer).data(), true, false);
						return;
					}
				}
			}
		}
	}

	// check for sudden death
	if (ScoreIsTied())
		return;

	// no score limit in horde
	if (GT(GT_HORDE))
		return;

	// no score limit in race
	if (GT(GT_RACE))
		return;

	int	scorelimit = GT_ScoreLimit();
	if (!scorelimit) return;

	if (teams) {
		if (level.team_scores[TEAM_RED] >= scorelimit) {
			QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", Teams_TeamName(TEAM_RED), GT_ScoreLimitString()).data(), false, false);
			return;
		}
		if (level.team_scores[TEAM_BLUE] >= scorelimit) {
			QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", Teams_TeamName(TEAM_BLUE), GT_ScoreLimitString()).data(), false, false);
			return;
		}
	} else {
		for (auto ec : active_clients()) {
			if (ec->client->sess.team != TEAM_FREE)
				continue;

			if (ec->client->resp.score >= scorelimit) {
				QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", ec->client->sess.netName, GT_ScoreLimitString()).data(), false, false);
				return;
			}
		}
	}
}

static bool Match_NextMap() {
	if (level.match_state == MatchState::MATCH_ENDED) {
		level.match_state = MatchState::MATCH_WARMUP_DELAYED;
		level.warmup_notice_time = level.time;
		Match_Reset();
		return true;
	}
	return false;
}

void GT_Init() {
	constexpr const char *COOP = "coop";
	bool force_dm = false;

	deathmatch = gi.cvar("deathmatch", "1", CVAR_LATCH);
	teamplay = gi.cvar("teamplay", "0", CVAR_SERVERINFO);
	ctf = gi.cvar("ctf", "0", CVAR_SERVERINFO);
	g_gametype = gi.cvar("g_gametype", G_Fmt("{}", (int)GT_FFA).data(), CVAR_SERVERINFO);
	coop = gi.cvar("coop", "0", CVAR_LATCH);

	// game modifications
	g_instagib = gi.cvar("g_instagib", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_instagib_splash = gi.cvar("g_instagib_splash", "0", CVAR_NOFLAGS);
	g_owner_auto_join = gi.cvar("g_owner_auto_join", "1", CVAR_NOFLAGS);
	g_owner_push_scores = gi.cvar("g_owner_push_scores", "0", CVAR_NOFLAGS);
	g_gametype_cfg = gi.cvar("g_gametype_cfg", "1", CVAR_NOFLAGS);
	g_quadhog = gi.cvar("g_quadhog", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_nadefest = gi.cvar("g_nadefest", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_frenzy = gi.cvar("g_frenzy", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_vampiric_damage = gi.cvar("g_vampiric_damage", "0", CVAR_NOFLAGS);
	g_vampiric_exp_min = gi.cvar("g_vampiric_exp_min", "0", CVAR_NOFLAGS);
	g_vampiric_health_max = gi.cvar("g_vampiric_health_max", "9999", CVAR_NOFLAGS);
	g_vampiric_percentile = gi.cvar("g_vampiric_percentile", "0.67f", CVAR_NOFLAGS);

	if (g_gametype->integer < 0 || g_gametype->integer >= GT_NUM_GAMETYPES)
		gi.cvar_forceset("g_gametype", G_Fmt("{}", clamp(g_gametype->integer, (int)GT_FIRST, (int)GT_LAST)).data());

	if (ctf->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
		// force tdm off
		if (teamplay->integer)
			gi.cvar_set("teamplay", "0");
	}
	if (teamplay->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
	}

	if (force_dm && !deathmatch->integer) {
		gi.Com_Print("Forcing deathmatch.\n");
		gi.cvar_forceset("deathmatch", "1");
	}

	// force even maxplayers value during teamplay
	if (Teams()) {
		int pmax = maxplayers->integer;

		if (pmax != floor(pmax / 2))
			gi.cvar_set("maxplayers", G_Fmt("{}", floor(pmax / 2) * 2).data());
	}

	GT_SetLongName();
}

void ChangeGametype(gametype_t gt) {
	switch (gt) {
	case gametype_t::GT_CTF:
		if (!ctf->integer)
			gi.cvar_forceset("ctf", "1");
		break;
	case gametype_t::GT_TDM:
		if (!teamplay->integer)
			gi.cvar_forceset("teamplay", "1");
		break;
	default:
		if (ctf->integer)
			gi.cvar_forceset("ctf", "0");
		if (teamplay->integer)
			gi.cvar_forceset("teamplay", "0");
		break;
	}

	if (!deathmatch->integer) {
		gi.Com_Print("Forcing deathmatch.\n");
		gi.cvar_forceset("deathmatch", "1");
	}

	if ((int)gt != g_gametype->integer)
		gi.cvar_forceset("g_gametype", G_Fmt("{}", (int)gt).data());
}
