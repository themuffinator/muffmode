// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "monsters/m_player.h"
/*freeze*/
#if 0
#include "freeze.h"
#endif
/*freeze*/
#include <sstream>

enum cmd_flags_t : uint32_t {
	CF_NONE				= 0,
	CF_ALLOW_DEAD		= bit_v<0>,
	CF_ALLOW_INT		= bit_v<1>,
	CF_ALLOW_SPEC		= bit_v<2>,
	CF_MATCH_ONLY		= bit_v<3>,
	CF_ADMIN_ONLY		= bit_v<4>,
	CF_CHEAT_PROTECT	= bit_v<5>,
};

struct cmds_t {
	const		char *name;
	void		(*func)(gentity_t *ent);
	uint32_t	flags;
	bool		floodExempt;
};

#include <sstream>			// for ostringstream
#include <iomanip>			// for setw

/*
=========================
FormatUsage
=========================
*/
static std::string FormatUsage(const std::string &command,
	const std::vector<std::string> &required = {},
	const std::vector<std::string> &optional = {},
	const std::string &help = {}) {

	std::ostringstream oss;
	oss << "Usage: " << command;

	for (const auto &arg : required)
		oss << " <" << arg << ">";

	for (const auto &arg : optional)
		oss << " [" << arg << "]";

	if (!help.empty())
		oss << '\n' << help << '\n';

	return oss.str();
}

/*
=========================
PrintMapList
=========================
*/
static void PrintMapList(gentity_t *ent) {
	const char *mapList = g_map_list->string;
	std::vector<std::string> maps;
	std::stringstream ss(mapList);
	std::string map;

	size_t maxLen = 0;
	while (ss >> map) {
		maps.push_back(map);
		if (map.length() > maxLen) maxLen = map.length();
	}

	const size_t colWidth = maxLen + 1;
	const size_t colsPerRow = std::max<size_t>(1, 120 / colWidth);

	std::string fullOutput = "Valid maps:\n";
	std::string currentLine;
	for (size_t i = 0; i < maps.size(); ++i) {
		currentLine += maps[i];
		currentLine.append(colWidth - maps[i].length(), ' ');

		if ((i + 1) % colsPerRow == 0 || i + 1 == maps.size()) {
			fullOutput += currentLine + '\n';
			currentLine.clear();
		}
	}

	const size_t maxPerPrint = 1024;
	size_t start = 0;
	while (start < fullOutput.size()) {
		size_t end = std::min(fullOutput.size(), start + maxPerPrint);
		if (end < fullOutput.size()) {
			end = fullOutput.rfind('\n', end);
			if (end == std::string::npos || end <= start)
				end = start + maxPerPrint;
		}
		std::string chunk = fullOutput.substr(start, end - start);
		gi.LocClient_Print(ent, PRINT_HIGH, "{}", chunk);
		start = end;
	}
}

static void Cmd_Print_State(gentity_t *ent, bool on_state) {
	const char *s = gi.argv(0);
	if (s)
		gi.LocClient_Print(ent, PRINT_HIGH, "{} {}\n", s, on_state ? "ON" : "OFF");
}

static inline bool CheatsOk(gentity_t *ent) {
	if (!deathmatch->integer && !coop->integer)
		return true;
	
	if (!g_cheats->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Cheats must be enabled to use this command.\n");
		return false;
	}

	return true;
}

static inline bool AliveOk(gentity_t *ent) {
	if (ent->health <= 0 || ent->deadFlag) {
		//gi.LocClient_Print(ent, PRINT_HIGH, "You must be alive to use this command.\n");
		return false;
	}

	return true;
}

static inline bool SpectatorOk(gentity_t *ent) {
	if (!ClientIsPlaying(ent->client)) {
		//gi.LocClient_Print(ent, PRINT_HIGH, "Spectators cannot use this command.\n");
		return false;
	}

	return true;
}

static inline bool AdminOk(gentity_t *ent) {
	if (!g_allow_admin->integer || !ent->client->sess.admin) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Only admins can use this command.\n");
		return false;
	}

	return true;
}

//=================================================================================

static void SelectNextItem(gentity_t *ent, item_flags_t itflags, bool menu = true) {
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t *it;

	cl = ent->client;

	if (menu && cl->menu) {
		P_Menu_Next(ent);
		return;
	} else if (menu && cl->followTarget) {
		FollowNext(ent);
		return;
	}

	// scan for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		index = static_cast<item_id_t>((cl->pers.selected_item + i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemList[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
		cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
		return;
	}

	cl->pers.selected_item = IT_NULL;
}

static void Cmd_InvNextP_f(gentity_t *ent) {
	SelectNextItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE);
}

static void Cmd_InvNextW_f(gentity_t *ent) {
	SelectNextItem(ent, IF_WEAPON);
}

static void Cmd_InvNext_f(gentity_t *ent) {
	SelectNextItem(ent, IF_ANY);
}

static void SelectPrevItem(gentity_t *ent, item_flags_t itflags) {
	gclient_t *cl = ent->client;
	item_id_t  i, index;
	gitem_t *it;

	if (cl->menu) {
		P_Menu_Prev(ent);
		return;
	} else if (cl->followTarget) {
		FollowPrev(ent);
		return;
	}

	// scan for the previous valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		index = static_cast<item_id_t>((cl->pers.selected_item + IT_TOTAL - i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemList[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
		cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
		return;
	}

	cl->pers.selected_item = IT_NULL;
}

static void Cmd_InvPrevP_f(gentity_t *ent) {
	SelectPrevItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE);
}

static void Cmd_InvPrevW_f(gentity_t *ent) {
	SelectPrevItem(ent, IF_WEAPON);
}

static void Cmd_InvPrev_f(gentity_t *ent) {
	SelectPrevItem(ent, IF_ANY);
}

void ValidateSelectedItem(gentity_t *ent) {
	gclient_t *cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return; // valid

	SelectNextItem(ent, IF_ANY, false);
}

//=================================================================================

static void SpawnAndGiveItem(gentity_t *ent, item_id_t id) {
	gitem_t *it = GetItemByIndex(id);

	if (!it)
		return;

	gentity_t *it_ent = Spawn();
	it_ent->className = it->className;
	SpawnItem(it_ent, it);

	if (it_ent->inUse) {
		Touch_Item(it_ent, ent, null_trace, true);
		if (it_ent->inUse)
			FreeEntity(it_ent);
	}
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void Cmd_Give_f(gentity_t *ent) {
	const char	*name = gi.args();
	gitem_t		*it;
	size_t		i;
	bool		give_all;
	gentity_t		*it_ent;

	if (Q_strcasecmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_strcasecmp(gi.argv(1), "health") == 0) {
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "weapons") == 0) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "ammo") == 0) {
		if (give_all)
			SpawnAndGiveItem(ent, IT_PACK);

		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_AMMO))
				continue;
			Add_Ammo(ent, it, AMMO_INFINITE);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "armor") == 0) {
		ent->client->pers.inventory[IT_ARMOR_JACKET] = 0;
		ent->client->pers.inventory[IT_ARMOR_COMBAT] = 0;
		ent->client->pers.inventory[IT_ARMOR_BODY] = armor_stats[game.ruleset][ARMOR_BODY].max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "keys") == 0) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_KEY))
				continue;
			ent->client->pers.inventory[i]++;
		}
		ent->client->pers.powerCubes = 0xFF;

		if (!give_all)
			return;
	}

	if (give_all) {
		SpawnAndGiveItem(ent, IT_POWER_SHIELD);

		if (!give_all)
			return;
	}

	if (give_all) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IF_ARMOR | IF_POWER_ARMOR | IF_WEAPON | IF_AMMO | IF_NOT_GIVEABLE | IF_TECH))
				continue;
			else if (it->pickup == CTF_PickupFlag)
				continue;
			else if ((it->flags & IF_HEALTH) && !it->use)
				continue;
			ent->client->pers.inventory[i] = (it->flags & IF_KEY) ? 8 : 1;
		}

		CheckPowerArmorState(ent);
		ent->client->pers.powerCubes = 0xFF;
		return;
	}

	it = FindItem(name);
	if (!it) {
		name = gi.argv(1);
		it = FindItem(name);
	}
	if (!it)
		it = FindItemByClassname(name);

	if (!it) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_item");
		return;
	}

	if (it->flags & IF_NOT_GIVEABLE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_giveable");
		return;
	}

	if (!it->pickup) {
		ent->client->pers.inventory[it->id] = 1;
		return;
	}

	it_ent = Spawn();
	it_ent->className = it->className;
	SpawnItem(it_ent, it);
	if (it->flags & IF_AMMO && gi.argc() == 3)
		it_ent->count = atoi(gi.argv(2));

	// since some items don't actually spawn when you say to ..
	if (!it_ent->inUse)
		return;

	Touch_Item(it_ent, ent, null_trace, true);
	if (it_ent->inUse)
		FreeEntity(it_ent);
}

static void Cmd_SetPOI_f(gentity_t *self) {
	level.currentPOI = self->s.origin;
	level.validPOI = true;
}

static void Cmd_CheckPOI_f(gentity_t *self) {
	if (!level.validPOI)
		return;

	char visible_pvs = gi.inPVS(self->s.origin, level.currentPOI, false) ? 'y' : 'n';
	char visible_pvs_portals = gi.inPVS(self->s.origin, level.currentPOI, true) ? 'y' : 'n';
	char visible_phs = gi.inPHS(self->s.origin, level.currentPOI, false) ? 'y' : 'n';
	char visible_phs_portals = gi.inPHS(self->s.origin, level.currentPOI, true) ? 'y' : 'n';

	gi.Com_PrintFmt("pvs {} + portals {}, phs {} + portals {}\n", visible_pvs, visible_pvs_portals, visible_phs, visible_phs_portals);
}

// [Paril-KEX]
static void Cmd_Target_f(gentity_t *ent) {
	ent->target = gi.argv(1);
	UseTargets(ent, ent);
	ent->target = nullptr;
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f(gentity_t *ent) {
	ent->flags ^= FL_GODMODE;
	Cmd_Print_State(ent, ent->flags & FL_GODMODE);
}

/*
==================
Cmd_Immortal_f

Sets client to immortal - take damage but never go below 1 hp

argv(0) immortal
==================
*/
static void Cmd_Immortal_f(gentity_t *ent) {
	ent->flags ^= FL_IMMORTAL;
	Cmd_Print_State(ent, ent->flags & FL_IMMORTAL);
}

void ED_ParseField(const char *key, const char *value, gentity_t *ent);
/*
=================
Cmd_Spawn_f

Spawn class name

argv(0) spawn
argv(1) <className>
argv(2+n) "key"...
argv(3+n) "value"...
=================
*/
static void Cmd_Spawn_f(gentity_t *ent) {
	solid_t backup = ent->solid;
	ent->solid = SOLID_NOT;
	gi.linkentity(ent);

	gentity_t *other = Spawn();
	other->className = gi.argv(1);

	other->s.origin = ent->s.origin + (AngleVectors(ent->s.angles).forward * 24.f);
	other->s.angles[YAW] = ent->s.angles[YAW];

	st = {};

	if (gi.argc() > 3) {
		for (int i = 2; i < gi.argc(); i += 2)
			ED_ParseField(gi.argv(i), gi.argv(i + 1), other);
	}

	ED_CallSpawn(other);

	if (other->inUse) {
		vec3_t forward, end;
		AngleVectors(ent->client->vAngle, forward, nullptr, nullptr);
		end = ent->s.origin;
		end[2] += ent->viewHeight;
		end += (forward * 8192);

		trace_t tr = gi.traceline(ent->s.origin + vec3_t{ 0.f, 0.f, (float)ent->viewHeight }, end, other, MASK_SHOT | CONTENTS_MONSTERCLIP);
		other->s.origin = tr.endpos;

		for (size_t i = 0; i < 3; i++) {
			if (tr.plane.normal[i] > 0)
				other->s.origin[i] -= other->mins[i] * tr.plane.normal[i];
			else
				other->s.origin[i] += other->maxs[i] * -tr.plane.normal[i];
		}

		while (gi.trace(other->s.origin, other->mins, other->maxs, other->s.origin, other,
			MASK_SHOT | CONTENTS_MONSTERCLIP).startsolid) {
			float dx = other->mins[0] - other->maxs[0];
			float dy = other->mins[1] - other->maxs[1];
			other->s.origin += forward * -sqrtf(dx * dx + dy * dy);

			if ((other->s.origin - ent->s.origin).dot(forward) < 0) {
				gi.Client_Print(ent, PRINT_HIGH, "Couldn't find a suitable spawn location.\n");
				FreeEntity(other);
				break;
			}
		}

		if (other->inUse)
			gi.linkentity(other);

		if ((other->svFlags & SVF_MONSTER) && other->think)
			other->think(other);
	}

	ent->solid = backup;
	gi.linkentity(ent);
}

/*
=================
Cmd_Teleport_f

argv(0) teleport
argv(1) x
argv(2) y
argv(3) z
argv(4) pitch
argv(5) yaw
argv(6) roll
=================
*/
static void Cmd_Teleport_f(gentity_t *ent) {
	if (gi.argc() < 4 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "x", "y", "z" }, { "pitch", "yaw", "roll" }, "Instantly moves the player to the specified coordinates within the world, optionally with the specified angles");
		return;
	}

	ent->s.origin[0] = (float)atof(gi.argv(1));
	ent->s.origin[1] = (float)atof(gi.argv(2));
	ent->s.origin[2] = (float)atof(gi.argv(3));

	if (gi.argc() >= 4) {
		float pitch = (float)atof(gi.argv(4));
		float yaw = (float)atof(gi.argv(5));
		float roll = (float)atof(gi.argv(6));
		vec3_t ang{ pitch, yaw, roll };

		ent->client->ps.pmove.delta_angles = (ang - ent->client->resp.cmd_angles);
		ent->client->ps.viewangles = {};
		ent->client->vAngle = {};
	}

	gi.linkentity(ent);
}

/*
==================
TimeoutEnd
==================
*/
void TimeoutEnd() {
	level.timeoutActive = 0_ms;
	level.timeoutOwner = nullptr;
	gi.Broadcast_Print(PRINT_CENTER, "Timeout has ended.\n");
	gi.positioned_sound(world->s.origin, world, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);

	G_LogEvent("MATCH TIMEOUT ENDED");
}

/*
==================
Cmd_TimeIn_f

Ends a timeout session.
==================
*/
static void Cmd_TimeIn_f(gentity_t *ent) {
	if (!level.timeoutActive) {
		gi.Client_Print(ent, PRINT_HIGH, "A timeout is not currently in effect.\n");
		return;
	}
	if (!ent->client->sess.admin && level.timeoutOwner != ent) {
		gi.Client_Print(ent, PRINT_HIGH, "The timeout can only be ended by the timeout caller or an admin.\n");
		return;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "{} is resuming the match.\n", ent->client->sess.netName);
	level.timeoutActive = 3_sec;
}

/*
==================
Cmd_TimeOut_f

Calls a timeout session.
==================
*/
static void Cmd_TimeOut_f(gentity_t *ent) {
	if (g_dm_timeout_length->integer <= 0) {
		gi.Client_Print(ent, PRINT_HIGH, "Server has disabled timeouts.\n");
		return;
	}
	if (level.match_state != MatchState::MATCH_IN_PROGRESS) {
		gi.Client_Print(ent, PRINT_HIGH, "Timeouts can only be issued during a match.\n");
		return;
	}
	if (ent->client->pers.timeout_used && !ent->client->sess.admin) {
		gi.Client_Print(ent, PRINT_HIGH, "You have already used your timeout.\n");
		return;
	}
	if (level.timeoutActive > 0_ms) {
		gi.Client_Print(ent, PRINT_HIGH, "A timeout is already in progress.\n");
		return;
	}

	level.timeoutOwner = ent;
	level.timeoutActive = gtime_t::from_sec(g_dm_timeout_length->integer);
	gi.LocBroadcast_Print(PRINT_CENTER, "{} called a timeout!\n{} has been granted.", ent->client->sess.netName, TimeString(g_dm_timeout_length->integer * 1000, false, false));
	gi.positioned_sound(world->s.origin, world, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("world/klaxon2.wav"), 1, ATTN_NONE, 0);
	ent->client->pers.timeout_used = true;

	G_LogEvent("MATCH TIMEOUT STARTED");
}

/*
==================
Cmd_NoTarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_NoTarget_f(gentity_t *ent) {
	ent->flags ^= FL_NOTARGET;
	Cmd_Print_State(ent, ent->flags & FL_NOTARGET);
}

/*
==================
Cmd_NoVisible_f

Sets client to "super notarget"

argv(0) novisible
==================
*/
static void Cmd_NoVisible_f(gentity_t *ent) {
	ent->flags ^= FL_NOVISIBLE;
	Cmd_Print_State(ent, ent->flags & FL_NOVISIBLE);
}

/*
==================
Cmd_AlertAll_f

argv(0) alertall
==================
*/
static void Cmd_AlertAll_f(gentity_t *ent) {
	for (size_t i = 0; i < globals.num_entities; i++) {
		gentity_t *t = &g_entities[i];

		if (!t->inUse || t->health <= 0 || !(t->svFlags & SVF_MONSTER))
			continue;

		t->enemy = ent;
		FoundTarget(t);
	}
}

/*
==================
Cmd_NoClip_f

argv(0) noclip
==================
*/
static void Cmd_NoClip_f(gentity_t *ent) {
	ent->moveType = ent->moveType == MOVETYPE_NOCLIP ? MOVETYPE_WALK : MOVETYPE_NOCLIP;
	Cmd_Print_State(ent, ent->moveType == MOVETYPE_NOCLIP);
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
static void Cmd_Use_f(gentity_t *ent) {
	item_id_t	index;
	gitem_t		*it = nullptr;
	const char	*s = gi.args();
	const char	*cmd = gi.argv(0);

	if (!Q_strcasecmp(cmd, "use_index") || !Q_strcasecmp(cmd, "use_index_only")) {
		it = GetItemByIndex((item_id_t)atoi(s));
	} else {
		if (!Q_strcasecmp(s, "holdable")) {
			if (ent->client->pers.inventory[IT_AMMO_NUKE])
				it = GetItemByIndex(IT_AMMO_NUKE);
			else if (ent->client->pers.inventory[IT_DOPPELGANGER])
				it = GetItemByIndex(IT_DOPPELGANGER);
			else if (ent->client->pers.inventory[IT_TELEPORTER])
				it = GetItemByIndex(IT_TELEPORTER);
			else if (ent->client->pers.inventory[IT_ADRENALINE])
				it = GetItemByIndex(IT_ADRENALINE);
			else if (ent->client->pers.inventory[IT_COMPASS])
				it = GetItemByIndex(IT_COMPASS);
			else return;
		}

		if (!it)
			it = FindItem(s);
	}

	if (!it) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_item_name", s);
		return;
	}
	if (!it->use) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
		return;
	}
	index = it->id;

	if (CombatIsDisabled() && !(it->flags & IF_WEAPON))
		return;

	// Paril: Use_Weapon handles weapon availability
	if (!(it->flags & IF_WEAPON) && !ent->client->pers.inventory[index]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickup_name);
		return;
	}

	// allow weapon chains for use
	ent->client->no_weapon_chains = !!strcmp(gi.argv(0), "use") && !!strcmp(gi.argv(0), "use_index");

	it->use(ent, it);

	ValidateSelectedItem(ent);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
static void Cmd_Drop_f(gentity_t *ent) {
	item_id_t	index;
	gitem_t		*it;
	const char	*s;

	// don't drop anything when combat is disabled
	if (CombatIsDisabled())
		return;

	s = gi.args();

	const char *cmd = gi.argv(0);

	if (!Q_strcasecmp(cmd, "drop_index")) {
		it = GetItemByIndex((item_id_t)atoi(s));
	} else {
		it = FindItem(s);
	}

	if (!it) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Unknown item : {}\n", s);
		return;
	}
	if (!it->drop) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
		return;
	}

	const char *t = nullptr;
	if (it->id == IT_FLAG_RED || it->id == IT_FLAG_BLUE) {
		if (!(g_drop_cmds->integer & 1))
			t = "Flag";
	} else if (it->flags & IF_POWERUP) {
		if (!(g_drop_cmds->integer & 2))
			t = "Powerup";
	} else if (it->flags & IF_WEAPON || it->flags & IF_AMMO) {
		if (!(g_drop_cmds->integer & 4))
			t = "Weapon and ammo";
		else if (!ItemSpawnsEnabled()) {
			gi.Client_Print(ent, PRINT_HIGH, "Weapon and ammo dropping is not available in this mode.\n");
			return;
		}
	} else if (it->flags & IF_WEAPON && deathmatch->integer && g_dm_weapons_stay->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "Weapon dropping is not available during weapons stay mode.\n");
	}

	if (t != nullptr) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} dropping has been disabled on this server.\n", t);
		return;
	}

	index = it->id;
	if (!ent->client->pers.inventory[index]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickup_name);
		return;
	}

	if (!Q_strcasecmp(gi.args(), "tech")) {
		it = Tech_Held(ent);

		if (it) {
			it->drop(ent, it);
			ValidateSelectedItem(ent);
		}

		return;
	}

	if (!Q_strcasecmp(gi.args(), "weapon")) {
		it = ent->client->pers.weapon;

		if (it) {
			it->drop(ent, it);
			ValidateSelectedItem(ent);
		}

		return;
	}

	it->drop(ent, it);

	if (Teams() && g_teamplay_item_drop_notice->integer) {
		// add drop notice to all team mates
		//BroadcastTeamMessage(ent->client->sess.team, PRINT_CHAT, G_Fmt("[TEAM]: {} drops {}\n", ent->client->sess.netName, it->use_name).data());

		uint32_t key = GetUnicastKey();

		for (auto ec : active_clients()) {
			if (ent == ec)
				continue;
			if (ClientIsPlaying(ec->client) && !OnSameTeam(ent, ec))
				continue;
			if (!ClientIsPlaying(ec->client) && !ec->client->followTarget)
				continue;
			if (!ClientIsPlaying(ec->client) && ec->client->followTarget && !OnSameTeam(ent, ec->client->followTarget))
				continue;
			if (!ClientIsPlaying(ec->client) && ec->client->followTarget && ent == ec->client->followTarget)
				continue;
			
			gi.WriteByte(svc_poi);
			gi.WriteShort(POI_PING + (ent->s.number - 1));
			gi.WriteShort(5000);
			gi.WritePosition(ent->s.origin);
			gi.WriteShort(gi.imageindex(it->icon));
			gi.WriteByte(215);
			gi.WriteByte(POI_FLAG_NONE);
			gi.unicast(ec, false);
			gi.local_sound(ec, CHAN_AUTO, gi.soundindex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
			
			gi.LocClient_Print(ec, PRINT_TTS, G_Fmt("[TEAM]: {} drops {}\n", ent->client->sess.netName, it->use_name).data(), ent->client->sess.netName);
		}
	}

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_Inven_f
=================
*/
static void Cmd_Inven_f(gentity_t *ent) {
	size_t		i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showScores = false;
	cl->showHelp = false;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	if (deathmatch->integer && ent->client->menu) {
		if (Vote_Menu_Active(ent))
			return;
		//gi.Client_Print(ent, PRINT_HIGH, "ARGH!\n");
		P_Menu_Close(ent);
		ent->client->follow_update = true;
		if (!ent->client->initial_menu_closure) {
			gi.LocClient_Print(ent, PRINT_CENTER, "%bind:inven:Toggles Menu%{}", " ");
			ent->client->initial_menu_closure = true;
		}
		return;
	}

	if (cl->showInventory) {
		cl->showInventory = false;
		return;
	}

	if (deathmatch->integer) {
		if (Vote_Menu_Active(ent))
			return;

		G_Menu_Join_Open(ent);
		return;
	}
	globals.server_flags |= SERVER_FLAG_SLOW_TIME;

	cl->showInventory = true;

	gi.WriteByte(svc_inventory);
	for (i = 0; i < IT_TOTAL; i++)
		gi.WriteShort(cl->pers.inventory[i]);
	for (; i < MAX_ITEMS; i++)
		gi.WriteShort(0);
	gi.unicast(ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
static void Cmd_InvUse_f(gentity_t *ent) {
	gitem_t *it;

	if (deathmatch->integer && ent->client->menu) {
		P_Menu_Select(ent);
		return;
	}

	if (!ClientIsPlaying(ent->client))
		return;

	if (ent->health <= 0 || ent->deadFlag)
		return;

	ValidateSelectedItem(ent);

	if (ent->client->pers.selected_item == IT_NULL) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_use");
		return;
	}

	it = &itemList[ent->client->pers.selected_item];
	if (!it->use) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
		return;
	}

	// don't allow weapon chains for invuse
	ent->client->no_weapon_chains = true;
	it->use(ent, it);

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
static void Cmd_WeapPrev_f(gentity_t *ent) {
	gclient_t	*cl = ent->client;
	item_id_t	i, index;
	gitem_t		*it;
	item_id_t	selected_weapon;

	if (cl->menu) {
		P_Menu_Prev(ent);
		return;
	} else if (!cl->pers.weapon)
		return;

	// don't allow weapon chains for weapprev
	cl->no_weapon_chains = true;

	selected_weapon = cl->pers.weapon->id;

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		// PMM - prevent scrolling through ALL weapons
		index = static_cast<item_id_t>((selected_weapon + IT_TOTAL - i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;

		it = &itemList[index];
		if (!it->use)
			continue;

		if (!(it->flags & IF_WEAPON))
			continue;

		it->use(ent, it);
		if (cl->newWeapon == it)
			return; // successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
static void Cmd_WeapNext_f(gentity_t *ent) {
	gclient_t	*cl = ent->client;
	item_id_t	i, index;
	gitem_t		*it;
	item_id_t	selected_weapon;

	if (cl->menu) {
		P_Menu_Next(ent);
		return;
	} else if (!cl->pers.weapon)
		return;

	// don't allow weapon chains for weapnext
	cl->no_weapon_chains = true;

	selected_weapon = cl->pers.weapon->id;

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		// PMM - prevent scrolling through ALL weapons
		index = static_cast<item_id_t>((selected_weapon + i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;

		it = &itemList[index];
		if (!it->use)
			continue;

		if (!(it->flags & IF_WEAPON))
			continue;

		it->use(ent, it);
		// PMM - prevent scrolling through ALL weapons

		if (cl->newWeapon == it)
			return;
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f(gentity_t *ent) {
	gclient_t	*cl = ent->client;
	int			index;
	gitem_t		*it;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	// don't allow weapon chains for weaplast
	cl->no_weapon_chains = true;

	index = cl->pers.lastweapon->id;
	if (!cl->pers.inventory[index])
		return;

	it = &itemList[index];
	if (!it->use)
		return;

	if (!(it->flags & IF_WEAPON))
		return;

	it->use(ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
static void Cmd_InvDrop_f(gentity_t *ent) {
	gitem_t *it;

	ValidateSelectedItem(ent);

	if (ent->client->pers.selected_item == IT_NULL) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_drop");
		return;
	}

	it = &itemList[ent->client->pers.selected_item];
	if (!it->drop) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
		return;
	}
	it->drop(ent, it);

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_Forfeit_f
=================
*/
static void Cmd_Forfeit_f(gentity_t *ent) {
	if (notGTF(GTF_1V1)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Forfeit is only available during Duel or Gauntlet.\n");
		return;
	}
	if (level.match_state < MatchState::MATCH_IN_PROGRESS) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Forfeit is not available during warmup.\n");
		return;
	}
	if (ent->client != &game.clients[level.sorted_clients[1]]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Forfeit is only available to the losing player.\n");
		return;
	}
	if (!g_allow_forfeit->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Forfeits are not enabled on this server.\n");
		return;
	}

	QueueIntermission(G_Fmt("{} forfeits the match.", ent->client->sess.netName).data(), true, false);
}

/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(gentity_t *ent) {
	if (deathmatch->integer && notGT(GT_RACE) && (level.time - ent->client->respawn_time) < 5_sec)
		return;

	if (CombatIsDisabled())
		return;

	if (GT(GT_RACE)) {
		ClientSpawn(ent);
		G_PostRespawn(ent);
		return;
	}

	ent->flags &= ~FL_GODMODE;
	ent->health = 0;

	//  make sure no trackers are still hurting us.
	if (ent->client->trackerPainTime)
		RemoveAttackingPainDaemons(ent);

	if (ent->client->ownedSphere) {
		FreeEntity(ent->client->ownedSphere);
		ent->client->ownedSphere = nullptr;
	}

	// [Paril-KEX] don't allow kill to take points away in TDM
	player_die(ent, ent, ent, 100000, vec3_origin, { MOD_SUICIDE, GT(GT_TDM) });
}

/*
=================
Cmd_Kill_AI_f
=================
*/
static void Cmd_Kill_AI_f(gentity_t *ent) {
	// except the one we're looking at...
	gentity_t *looked_at = nullptr;

	vec3_t start = ent->s.origin + vec3_t{ 0.f, 0.f, (float)ent->viewHeight };
	vec3_t end = start + ent->client->vForward * 1024.f;

	looked_at = gi.traceline(start, end, ent, MASK_SHOT).ent;

	const int numEntities = globals.num_entities;
	for (int entnum = 1; entnum < numEntities; ++entnum) {
		gentity_t *entity = &g_entities[entnum];
		if (!entity->inUse || entity == looked_at) {
			continue;
		}

		if ((entity->svFlags & SVF_MONSTER) == 0) {
			continue;
		}

		FreeEntity(entity);
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "{}: All AI Are Dead...\n", __FUNCTION__);
}

/*
=================
Cmd_Where_f
=================
*/
static void Cmd_Where_f(gentity_t *ent) {
	if (ent == nullptr || ent->client == nullptr)
		return;

	const vec3_t &origin = ent->s.origin;
	
	std::string location;
	fmt::format_to(std::back_inserter(location), FMT_STRING("{:.1f} {:.1f} {:.1f} {:.1f} {:.1f} {:.1f}\n"), origin[0], origin[1], origin[2], ent->client->ps.viewangles[PITCH], ent->client->ps.viewangles[YAW], ent->client->ps.viewangles[ROLL]);
	gi.LocClient_Print(ent, PRINT_HIGH, "Location: {}\n", location.c_str());
	gi.SendToClipBoard(location.c_str());
}

/*
=================
Cmd_Clear_AI_Enemy_f
=================
*/
static void Cmd_Clear_AI_Enemy_f(gentity_t *ent) {
	for (size_t i = 1; i < globals.num_entities; i++) {
		gentity_t *entity = &g_entities[i];
		if (!entity->inUse)
			continue;
		if ((entity->svFlags & SVF_MONSTER) == 0)
			continue;

		entity->monsterInfo.aiflags |= AI_FORGET_ENEMY;
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "{}: Clear All AI Enemies...\n", __FUNCTION__);
}

/*
=================
Cmd_PutAway_f
=================
*/
static void Cmd_PutAway_f(gentity_t *ent) {
	ent->client->showScores = false;
	ent->client->showHelp = false;
	ent->client->showInventory = false;

	gentity_t *e = ent->client->followTarget ? ent->client->followTarget : ent;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) || e->client->eliminated ? 0 : 1;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	ent->client->follow_update = true;

	if (deathmatch->integer && ent->client->menu) {
		if (Vote_Menu_Active(ent))
			return;
		//gi.Client_Print(ent, PRINT_HIGH, "ARGH! 2\n");
		P_Menu_Close(ent);
	}
}

/*
=================
ClientListSortByScore
=================
*/
static int ClientListSortByScore(const void *a, const void *b) {
	int anum, bnum;

	anum = *(const int *)a;
	bnum = *(const int *)b;

	anum = game.clients[anum].resp.score;
	bnum = game.clients[bnum].resp.score;

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
ClientListSortByJoinTime
=================
*/
static int ClientListSortByJoinTime(const void *a, const void *b) {
	int anum, bnum;

	anum = *(const int *)a;
	bnum = *(const int *)b;

	anum = game.clients[anum].sess.teamJoinTime.milliseconds();
	bnum = game.clients[bnum].sess.teamJoinTime.milliseconds();

	if (anum > bnum)
		return -1;
	if (anum < bnum)
		return 1;
	return 0;
}

const enum class ClientListSort {
	CLIENTSORT_NONE,
	CLIENTSORT_SCORE,
	CLIENTSORT_TIME,
};

/*
=================
ClientList
=================
*/
static void ClientList(gentity_t *ent, ClientListSort sort) {
	size_t i, count_total = 0, count_bots = 0, skill = 0;
	int index[MAX_CLIENTS] = { 0 };
	std::string row, chunk;
	std::vector<std::string> messageChunks;

	for (auto ec : active_clients()) {
		index[count_total] = ec - g_entities - 1;
		count_total++;
		if (ec->client->sess.skillRating > 0)
			skill += ec->client->sess.skillRating;
		if (ec->client->sess.is_a_bot)
			count_bots++;
	}

	switch (sort) {
	case ClientListSort::CLIENTSORT_SCORE:
		qsort(index, count_total, sizeof(index[0]), ClientListSortByScore);
		break;
	case ClientListSort::CLIENTSORT_TIME:
		qsort(index, count_total, sizeof(index[0]), ClientListSortByJoinTime);
		break;
	}

	constexpr const char *header = "\nclientnum name                             id                                  sr   time ping score team\n";
	constexpr const char *divider = "--------------------------------------------------------------------------------------------------------------------\n";
	std::string fullOutput;
	fullOutput += header;
	fullOutput += divider;

	for (i = 0; i < count_total; i++) {
		gclient_t *cl = &game.clients[index[i]];

		fmt::format_to(std::back_inserter(row), FMT_STRING("{:9} {:32} {:32} {:5} {:03}:{:02} {:4} {:5} {}{}{}\n"),
			index[i],
			cl->sess.netName,
			cl->sess.socialID,
			cl->sess.skillRating,
			(level.time - cl->resp.enter_time).milliseconds() / 60000,
			((level.time - cl->resp.enter_time).milliseconds() % 60000) / 1000,
			cl->ping,
			cl->resp.score,
			cl->sess.versusQueued ? "QUEUE" : Teams_TeamName(cl->sess.team),
			(index[i] == 0) ? " (host)" : cl->sess.admin ? " (admin)" : "",
			cl->sess.inactive ? " (inactive)" : ""
		);

		if (fullOutput.length() + row.length() > 950) {
			messageChunks.push_back(fullOutput);
			fullOutput = header;
			fullOutput += divider;
		}

		fullOutput += row;
		row.clear();
	}

	if (!fullOutput.empty()) {
		messageChunks.push_back(fullOutput);
	}

	for (const auto &msg : messageChunks) {
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "{}", msg);
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, divider);
	}

	if ((count_total - count_bots) > 0)
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "total human players: {}\n", count_total - count_bots);
	if (count_bots > 0)
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "total bot players: {}\n", count_bots);
	if (skill > 0 && (count_total - count_bots) > 0)
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "average skill rating: {}\n", skill / (count_total - count_bots));

	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "\n");
}

/*
=================
Cmd_ClientList_f
=================
*/
static void Cmd_ClientList_f(gentity_t *ent) {
	ClientListSort sortMode = ClientListSort::CLIENTSORT_NONE;

	if (gi.argc() > 1) {
		std::string arg = gi.argv(1);
		std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

		if (arg == "score") {
			sortMode = ClientListSort::CLIENTSORT_SCORE;
		} else if (arg == "time") {
			sortMode = ClientListSort::CLIENTSORT_TIME;
		}
	}

	ClientList(ent, sortMode);
}

bool CheckFlood(gentity_t *ent) {
	int		   i;
	gclient_t *cl;

	if (flood_msgs->integer) {
		cl = ent->client;

		if (level.time < cl->flood_locktill) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_flood_cant_talk",
				(cl->flood_locktill - level.time).seconds<int32_t>());
			return true;
		}
		i = cl->flood_whenhead - flood_msgs->integer + 1;
		if (i < 0)
			i = (sizeof(cl->flood_when) / sizeof(cl->flood_when[0])) + i;
		if (i >= q_countof(cl->flood_when))
			i = 0;
		if (cl->flood_when[i] && level.time - cl->flood_when[i] < gtime_t::from_sec(flood_persecond->value)) {
			cl->flood_locktill = level.time + gtime_t::from_sec(flood_waitdelay->value);
			gi.LocClient_Print(ent, PRINT_CHAT, "$g_flood_cant_talk",
				flood_waitdelay->integer);
			return true;
		}
		cl->flood_whenhead = (cl->flood_whenhead + 1) % (sizeof(cl->flood_when) / sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}
	return false;
}

/*
=================
Cmd_Wave_f
=================
*/
static void Cmd_Wave_f(gentity_t *ent) {
	int i = atoi(gi.argv(1));

	// no dead or noclip waving
	if (ent->deadFlag || ent->moveType == MOVETYPE_NOCLIP)
		return;

	// can't wave when ducked
	bool do_animate = ent->client->animPriority <= ANIM_WAVE && !(ent->client->ps.pmove.pm_flags & PMF_DUCKED);

	if (do_animate)
		ent->client->animPriority = ANIM_WAVE;

	const char *other_notify_msg = nullptr, *other_notify_none_msg = nullptr;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, 0 }, start, dir);

	// see who we're aiming at
	gentity_t *aiming_at = nullptr;
	float best_dist = -9999;

	for (auto player : active_clients()) {
		if (player == ent)
			continue;

		vec3_t cdir = player->s.origin - start;
		float dist = cdir.normalize();

		float dot = ent->client->vForward.dot(cdir);

		if (dot < 0.97)
			continue;
		else if (dist < best_dist)
			continue;

		best_dist = dist;
		aiming_at = player;
	}

	switch (i) {
	case GESTURE_FLIP_OFF:
		other_notify_msg = "$g_flipoff_other";
		other_notify_none_msg = "$g_flipoff_none";
		if (do_animate) {
			ent->s.frame = FRAME_flip01 - 1;
			ent->client->animEnd = FRAME_flip12;
		}
		break;
	case GESTURE_SALUTE:
		other_notify_msg = "$g_salute_other";
		other_notify_none_msg = "$g_salute_none";
		if (do_animate) {
			ent->s.frame = FRAME_salute01 - 1;
			ent->client->animEnd = FRAME_salute11;
		}
		break;
	case GESTURE_TAUNT:
		other_notify_msg = "$g_taunt_other";
		other_notify_none_msg = "$g_taunt_none";
		if (do_animate) {
			ent->s.frame = FRAME_taunt01 - 1;
			ent->client->animEnd = FRAME_taunt17;
		}
		break;
	case GESTURE_WAVE:
		other_notify_msg = "$g_wave_other";
		other_notify_none_msg = "$g_wave_none";
		if (do_animate) {
			ent->s.frame = FRAME_wave01 - 1;
			ent->client->animEnd = FRAME_wave11;
		}
		break;
	case GESTURE_POINT:
	default:
		other_notify_msg = "$g_point_other";
		other_notify_none_msg = "$g_point_none";
		if (do_animate) {
			ent->s.frame = FRAME_point01 - 1;
			ent->client->animEnd = FRAME_point12;
		}
		break;
	}

	bool has_a_target = false;

	if (i == GESTURE_POINT) {
		for (auto player : active_clients()) {
			if (player == ent)
				continue;
			else if (!OnSameTeam(ent, player))
				continue;

			has_a_target = true;
			break;
		}
	}

	if (i == GESTURE_POINT && has_a_target) {
		// don't do this stuff if we're flooding
		if (CheckFlood(ent))
			return;

		trace_t tr = gi.traceline(start, start + (ent->client->vForward * 2048), ent, MASK_SHOT & ~CONTENTS_WINDOW);
		other_notify_msg = "$g_point_other_ping";

		uint32_t key = GetUnicastKey();

		if (tr.fraction != 1.0f) {
			// send to all teammates
			for (auto player : active_clients()) {
				if (player != ent && !OnSameTeam(ent, player))
					continue;

				gi.WriteByte(svc_poi);
				gi.WriteShort(POI_PING + (ent->s.number - 1));
				gi.WriteShort(5000);
				gi.WritePosition(tr.endpos);
				gi.WriteShort(level.picPing);
				gi.WriteByte(208);
				gi.WriteByte(POI_FLAG_NONE);
				gi.unicast(player, false);

				gi.local_sound(player, CHAN_AUTO, gi.soundindex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
				gi.LocClient_Print(player, PRINT_HIGH, other_notify_msg, ent->client->sess.netName);
			}
		}
	} else {
		if (CheckFlood(ent))
			return;

		gentity_t *targ = nullptr;
		while ((targ = FindRadius(targ, ent->s.origin, 1024)) != nullptr) {
			if (ent == targ) continue;
			if (!targ->client) continue;
			if (!gi.inPVS(ent->s.origin, targ->s.origin, false)) continue;

			if (aiming_at && other_notify_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_msg, ent->client->sess.netName, aiming_at->client->sess.netName);
			else if (other_notify_none_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_none_msg, ent->client->sess.netName);
		}

		if (aiming_at && other_notify_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_msg, ent->client->sess.netName, aiming_at->client->sess.netName);
		else if (other_notify_none_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_none_msg, ent->client->sess.netName);
	}

	ent->client->animTime = 0_ms;
}

#ifndef KEX_Q2_GAME
/*
==================
Cmd_Say_f

NB: only used for non-Playfab stuff
==================
*/
static void Cmd_Say_f(gentity_t *ent, bool arg0) {
	gentity_t *other;
	const char *p_in;
	static std::string text;

	if (gi.argc() < 2 && !arg0)
		return;
	else if (CheckFlood(ent))
		return;

	text.clear();
	fmt::format_to(std::back_inserter(text), FMT_STRING("{}: "), ent->client->sess.netName);

	if (arg0) {
		text += gi.argv(0);
		text += " ";
		text += gi.args();
	} else {
		p_in = gi.args();
		size_t in_len = strlen(p_in);

		if (p_in[0] == '\"' && p_in[in_len - 1] == '\"')
			text += std::string_view(p_in + 1, in_len - 2);
		else
			text += p_in;
	}

	// don't let text be too long for malicious reasons
	if (text.length() > 150)
		text.resize(150);

	if (text.back() != '\n')
		text.push_back('\n');

	if (g_dedicated->integer)
		gi.Client_Print(nullptr, PRINT_CHAT, text.c_str());

	for (uint32_t j = 1; j <= game.maxclients; j++) {
		other = &g_entities[j];
		if (!other->inUse)
			continue;
		if (!other->client)
			continue;
		gi.Client_Print(other, PRINT_CHAT, text.c_str());
	}
}

/*
=================
Cmd_Say_Team_f

NB: only used for non-Playfab stuff
=================
*/
static void Cmd_Say_Team_f(gentity_t *who, const char *msg_in) {
	gentity_t *cl_ent;
	char outmsg[256];

	if (CheckFlood(who))
		return;

	Q_strlcpy(outmsg, msg_in, sizeof(outmsg));

	char *msg = outmsg;

	if (*msg == '\"') {
		msg[strlen(msg) - 1] = 0;
		msg++;
	}

	for (size_t i = 0; i < game.maxclients; i++) {
		cl_ent = g_entities + 1 + i;
		if (!cl_ent->inUse)
			continue;
		if (cl_ent->client->sess.team == who->client->sess.team)
			gi.LocClient_Print(cl_ent, PRINT_CHAT, "({}): {}\n",
				who->client->sess.netName, msg);
	}
}
#endif

/*
=================
Cmd_ListEntities_f
=================
*/
static void Cmd_ListEntities_f(gentity_t *ent) {
	int count = 0;

	for (size_t i = 1; i < game.maxentities; i++) {
		gentity_t *e = &g_entities[i];

		if (!e || !e->inUse)
			continue;
		
		if (gi.argc() > 1) {
			if (!strstr(e->className, gi.argv(1)))
				continue;
		}
		if (gi.argc() > 2) {
			float num = atof(gi.argv(3));
			if (e->s.origin[0] != num)
				continue;
		}
		if (gi.argc() > 3) {
			float num = atof(gi.argv(4));
			if (e->s.origin[1] != num)
				continue;
		}
		if (gi.argc() > 4) {
			float num = atof(gi.argv(5));
			if (e->s.origin[2] != num)
				continue;
		}

		gi.Com_PrintFmt("{}: {}", i, *e);
//#if 0
		if (e->target)
			gi.Com_PrintFmt(", target={}", e->target);
		if (e->targetname)
			gi.Com_PrintFmt(", targetname={}", e->targetname);
//#endif
		gi.Com_Print("\n");

		count++;
	}
	gi.Com_PrintFmt("\ntotal valid entities={}\n", count);
}

/*
=================
Cmd_ListMonsters_f
=================
*/
static void Cmd_ListMonsters_f(gentity_t *ent) {
	if (!g_debug_monster_kills->integer)
		return;

	for (size_t i = 0; i < level.totalMonsters; i++) {
		gentity_t *e = level.monstersRegistered[i];

		if (!e || !e->inUse)
			continue;
		else if (!(e->svFlags & SVF_MONSTER) || (e->monsterInfo.aiflags & AI_DO_NOT_COUNT))
			continue;
		else if (e->deadFlag)
			continue;

		gi.Com_PrintFmt("{}\n", *e);
	}
}

// =========================================
// TEAMPLAY - MOSTLY PORTED FROM QUAKE III
// =========================================

/*
================
PickTeam
================
*/
team_t PickTeam(int ignore_client_num) {
	if (!Teams())
		return TEAM_FREE;

	if (level.num_playing_blue > level.num_playing_red)
		return TEAM_RED;

	if (level.num_playing_red > level.num_playing_blue)
		return TEAM_BLUE;

	// equal team count, so join the team with the lowest score
	if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED])
		return TEAM_RED;
	if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE])
		return TEAM_BLUE;

	// equal team scores, so join team with lowest total individual scores
	// skip in tdm as it's redundant
	if (notGT(GT_TDM)) {
		int iscore_red = 0, iscore_blue = 0;

		for (size_t i = 0; i < game.maxclients; i++) {
			if (i == ignore_client_num)
				continue;
			if (!game.clients[i].pers.connected)
				continue;

			if (game.clients[i].sess.team == TEAM_RED) {
				iscore_red += game.clients[i].resp.score;
				continue;
			}
			if (game.clients[i].sess.team == TEAM_BLUE) {
				iscore_blue += game.clients[i].resp.score;
				continue;
			}
		}

		if (iscore_blue > iscore_red)
			return TEAM_RED;
		if (iscore_red > iscore_blue)
			return TEAM_BLUE;
	}

	// otherwise just randomly select a team
	return brandom() ? TEAM_RED : TEAM_BLUE;
}

/*
=================
BroadcastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange(gentity_t *ent, int old_team, bool inactive, bool silent) {

	if (!deathmatch->integer)
		return;

	if (!ent->client)
		return;

	if (notGTF(GTF_1V1) && ent->client->sess.team == old_team)
		return;

	if (silent)
		return;

	const char *s = nullptr, *t = nullptr;
	char		name[MAX_INFO_VALUE] = { 0 };
	int32_t		client_num;

	client_num = ent - g_entities - 1;
	gi.Info_ValueForKey(ent->client->pers.userInfo, "name", name, sizeof(name));

	switch (ent->client->sess.team) {
	case TEAM_FREE:
		s = G_Fmt("{} joined the battle.\n", name).data();
		//t = "%bind:inven:Toggles Menu%You have joined the game."; 
		if (ent->client->sess.skillRating > 0) {
			t = G_Fmt("You have joined the game.\nYour Skill Rating: {}", ent->client->sess.skillRating).data();
		} else {
			t = "You have joined the game.";
		}
		break;
	case TEAM_SPECTATOR:
		if (inactive) {
			s = G_Fmt("{} is inactive,\nmoved to spectators.\n", name).data();
			t = "You are inactive and have been\nmoved to spectators.";
		} else {
			if (GTF(GTF_1V1) && ent->client->sess.versusQueued) {
				s = G_Fmt("{} is in the queue to play.\n", name).data();
				t = "You are in the queue to play.";
			} else {
				s = G_Fmt("{} joined the spectators.\n", name).data();
				t = "You are now spectating.";
			}
		}
		break;
	case TEAM_RED:
	case TEAM_BLUE:
		s = G_Fmt("{} joined the {} Team.\n", name, Teams_TeamName(ent->client->sess.team)).data();

		if (ent->client->sess.skillRating > 0) {
			t = G_Fmt("You have joined the {} Team.\nYour Skill Rating: {}", Teams_TeamName(ent->client->sess.team), ent->client->sess.skillRating).data();
		} else {
			t = G_Fmt("You have joined the {} Team.\n", Teams_TeamName(ent->client->sess.team)).data();
		}
		break;
	}

	if (s) {
		for (auto ec : active_clients()) {
			if (ec == ent)
				continue;
			if (ec->svFlags & SVF_BOT)
				continue;
			gi.LocClient_Print(ec, PRINT_CENTER, s);
		}
		//gi.Com_Print(s);
	}

	if (g_dm_do_readyup->integer && level.match_state == MatchState::MATCH_WARMUP_READYUP) {
		BroadcastReadyReminderMessage();
	} else if (t) {
		gi.LocClient_Print(ent, PRINT_CENTER, G_Fmt("%bind:inven:Toggles Menu%{}", t).data() );
	}
}

/*
=================
AllowTeamSwitch
=================
*/
static bool AllowTeamSwitch(gentity_t *ent, team_t desired_team) {
	/*
	if (desired_team != ent->client->sess.team && GT(GT_RR) && level.match_state == MatchState::MATCH_IN_PROGRESS) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot change teams during a Red Rover match.\n");
		return false;
	}
	*/
	if (desired_team != TEAM_SPECTATOR && maxplayers->integer && level.num_playing_human_clients >= maxplayers->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Maximum player count has been reached.\n");
		return false; // ignore the request
	}

	if (level.locked[desired_team]) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} is locked.\n", Teams_TeamName(desired_team));
		return false; // ignore the request
	}

	if (Teams()) {
		if (g_teamplay_force_balance->integer) {
			// We allow a spread of two
			if ((desired_team == TEAM_RED && (level.num_playing_red - level.num_playing_blue > 1)) ||
				(desired_team == TEAM_BLUE && (level.num_playing_blue - level.num_playing_red > 1))) {
				gi.LocClient_Print(ent, PRINT_HIGH, "{} has too many players.\n", Teams_TeamName(desired_team));
				return false; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}
	}

	return true;
}

/*
=================
AllowClientTeamSwitch
=================
*/
bool AllowClientTeamSwitch(gentity_t *ent) {
	if (!deathmatch->integer)
		return false;

	if (g_dm_force_join->integer || !g_teamplay_allow_team_pick->integer) {
		if (!(ent->svFlags & SVF_BOT)) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Team picks are disabled.");
			return false;
		}
	}
	
	if (ent->client->resp.teamDelayTime > level.time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
		return false;
	}

	return true;
}

/*
================
TeamBalance

Balance the teams without shuffling.
Switch last joined player(s) from stacked team.
================
*/
int TeamBalance(bool force) {
	if (!Teams())
		return 0;

	if (GT(GT_RR))
		return 0;

	int delta = abs(level.num_playing_red - level.num_playing_blue);

	if (delta < 2)
		return level.num_playing_red - level.num_playing_blue;

	team_t stack_team = level.num_playing_red > level.num_playing_blue ? TEAM_RED : TEAM_BLUE;

	size_t	count = 0;
	int		index[MAX_CLIENTS_KEX/2];
	memset(index, 0, sizeof(index));

	// assemble list of client nums of everyone on stacked team
	for (auto ec : active_clients()) {
		if (ec->client->sess.team != stack_team)
			continue;
		index[count] = ec - g_entities;
		count++;
	}

	// sort client num list by join time
	qsort(index, count, sizeof(index[0]), ClientListSortByJoinTime);

	//run through sort list, switching from stack_team until teams are even
	if (count) {
		size_t	i;
		int switched = 0;
		gclient_t *cl = nullptr;
		for (i = 0; i < count, delta > 1; i++) {
			cl = &game.clients[index[i]];

			if (!cl)
				continue;

			if (!cl->pers.connected)
				continue;

			if (cl->sess.team != stack_team)
				continue;

			cl->sess.team = stack_team == TEAM_RED ? TEAM_BLUE : TEAM_RED;

			//TODO: queue this change in round-based games
			ClientRespawn(&g_entities[cl - game.clients + 1]);
			gi.LocClient_Print(&g_entities[cl - game.clients + 1], PRINT_CENTER, "You have changed teams to rebalance the game.\n");

			delta--;
			switched++;
		}

		if (switched) {
			gi.LocBroadcast_Print(PRINT_HIGH, "Teams have been balanced.\n");
			return switched;
		}
	}
	return 0;
}

/*
=============
SortPlayersBySkillRating

=============
*/
static int SortPlayersBySkillRating(const void *a, const void *b) {
	gclient_t *ca, *cb;

	ca = &game.clients[*(int *)a];
	cb = &game.clients[*(int *)b];

	// sort special clients last
	if (ca->sess.spectator_client < 0)
		return 1;
	if (cb->sess.spectator_client < 0)
		return -1;

	// then connecting clients
	if (!ca->pers.connected)
		return 1;
	if (!cb->pers.connected)
		return -1;

	// then spectators
	if (!ClientIsPlaying(ca) && !ClientIsPlaying(cb)) {
		if (ca->sess.versusQueued && cb->sess.versusQueued) {
			if (ca->sess.teamJoinTime > cb->sess.teamJoinTime)
				return -1;
			if (ca->sess.teamJoinTime < cb->sess.teamJoinTime)
				return 1;
		}
		if (ca->sess.versusQueued)
			return -1;
		if (cb->sess.versusQueued)
			return 1;
		if (ca->sess.teamJoinTime > cb->sess.teamJoinTime)
			return -1;
		if (ca->sess.teamJoinTime < cb->sess.teamJoinTime)
			return 1;
		return 0;
	}
	if (!ClientIsPlaying(ca))
		return 1;
	if (!ClientIsPlaying(cb))
		return -1;

	if (ca->sess.skillRating > cb->sess.skillRating)
		return -1;
	if (ca->sess.skillRating < cb->sess.skillRating)
		return 1;

	return 0;
}

/*
================
TeamSkillShuffle

Randomly shuffles all players in teamplay, tries to balance the skill
================
*/
bool TeamSkillShuffle() {
	int totalSkill = 0, numPlayers = 0;
	int oldRedSkill = 0, oldBlueSkill = 0;
	int averageSkill;

	if (!Teams())
		return false;

	// count total skill rating
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		totalSkill += ec->client->sess.skillRating;
		numPlayers++;

		if (ec->client->sess.team == TEAM_RED)
			oldRedSkill += ec->client->sess.skillRating;
		else if (ec->client->sess.team == TEAM_BLUE)
			oldBlueSkill += ec->client->sess.skillRating;
	}

	if (numPlayers < 2)
		return false;

	averageSkill = totalSkill / numPlayers;

	// sort players by skill
	qsort(level.skill_sorted_clients, level.num_connected_clients, sizeof(level.skill_sorted_clients[0]), SortPlayersBySkillRating);

	// divide players into pairs, decending down the skill ranking, randomly assign teams between the pairs
	for (int i = 0; i < numPlayers / 2; i++) {
		gclient_t *cl1 = &game.clients[level.skill_sorted_clients[i]];
		gclient_t *cl2 = &game.clients[level.skill_sorted_clients[i+1]];
		bool join_red = brandom();

		cl1->sess.team = join_red ? TEAM_RED : TEAM_BLUE;

		if (cl2)
			cl2->sess.team = cl1->sess.team == TEAM_RED ? TEAM_BLUE : TEAM_RED;
	}

	Match_Reset();

	int newRedSkill = 0, newBlueSkill = 0;

	// count total skill rating
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		if (ec->client->sess.team == TEAM_RED)
			newRedSkill += ec->client->sess.skillRating;
		else if (ec->client->sess.team == TEAM_BLUE)
			newBlueSkill += ec->client->sess.skillRating;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "Team shuffle result: RedSkill={}->{} BlueSkill={}->{}\n", oldRedSkill, newRedSkill, oldBlueSkill, newBlueSkill);

	return true;
}

/*
================
TeamShuffle

Randomly shuffles all players in teamplay
================
*/
bool TeamShuffle() {
	if (!Teams())
		return false;
	/*
	if (level.num_playing_clients < 3)
		return false;
		*/
	bool join_red = brandom();
	gentity_t *ent;
	int32_t index[MAX_CLIENTS_KEX] = { 0 };

	memset(index, -1, sizeof(index));

	// determine max team size based from active players
	int maxteam = ceil(level.num_playing_clients / 2);
	int count_red = 0, count_blue = 0;
	team_t setteam = join_red ? TEAM_RED : TEAM_BLUE;
	
	// create random array
	for (size_t i = 0; i < MAX_CLIENTS_KEX; i++) {
		if (index[i] >= 0)
			continue;

		int rnd = irandom(0, MAX_CLIENTS_KEX);
		while (index[rnd] >= 0)
			rnd = irandom(0, MAX_CLIENTS_KEX);

		index[i] = rnd;
		index[rnd] = i;
	}
#if 0
	for (size_t i = 0; i < MAX_CLIENTS_KEX; i++) {
		gi.Com_PrintFmt("{}={}\n", i, index[i]);
	}
#endif

	// set teams
	for (size_t i = 1; i <= MAX_CLIENTS_KEX; i++) {
		ent = &g_entities[index[i-1]];
		if (!ent)
			continue;
		if (!ent->inUse)
			continue;
		if (!ent->client)
			continue;
		if (!ent->client->pers.connected)
			continue;
		if (!ClientIsPlaying(ent->client))
			continue;

		if (count_red >= maxteam || count_red > count_blue)
			setteam = TEAM_BLUE;
		else if (count_blue >= maxteam || count_blue > count_red)
			setteam = TEAM_RED;
		
		ent->client->sess.team = setteam;

		if (setteam == TEAM_RED)
			count_red++;
		else count_blue++;

		join_red ^= true;
		setteam = join_red ? TEAM_RED : TEAM_BLUE;
	}

	return true;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
static void StopFollowing(gentity_t *ent, bool release) {
	gclient_t *client;

	if (ent->svFlags & SVF_BOT || !ent->inUse)
		return;

	client = ent->client;

	client->sess.team = TEAM_SPECTATOR;
	client->sess.spectator_state = SPECTATOR_FREE;
	if (release) {
		client->ps.stats[STAT_HEALTH] = ent->health = 1;
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;
	}
	//SetClientViewAngle(ent, client->ps.viewangles);

	//client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->svFlags &= SVF_BOT;

	//client->ps.clientnum = ent - g_entities;


	//-------------

	ent->client->ps.kick_angles = {};
	ent->client->ps.gunangles = {};
	ent->client->ps.gunoffset = {};
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunskin = 0;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunrate = 0;
	ent->client->ps.screen_blend = {};
	ent->client->ps.damageBlend = {};
	ent->client->ps.rdflags = RDF_NONE;
}

/*
=================
SetTeam
=================
*/
bool SetTeam(gentity_t *ent, team_t desired_team, bool inactive, bool force, bool silent) {
	team_t old_team = ent->client->sess.team;
	bool queue = false;
	
	if (!force) {
		if (!ClientIsPlaying(ent->client) && desired_team != TEAM_SPECTATOR) {
			bool revoke = false;
			if (level.match_state >= MatchState::MATCH_COUNTDOWN && g_match_lock->integer) {
				gi.LocClient_Print(ent, PRINT_HIGH, "Match is locked whilst in progress, no joining permitted now.\n");
				revoke = true;
			} else if (level.num_playing_human_clients >= maxplayers->integer) {
				gi.LocClient_Print(ent, PRINT_HIGH, "Maximum player load reached.\n");
				revoke = true;
			}
			if (revoke) {
				P_Menu_Close(ent);
				return false;
			}
		}

		if (desired_team != TEAM_SPECTATOR && desired_team == ent->client->sess.team) {
			P_Menu_Close(ent);
			return false;
		}

		if (GTF(GTF_1V1)) {
			if (desired_team != TEAM_SPECTATOR && level.num_playing_clients >= 2) {
				desired_team = TEAM_SPECTATOR;
				queue = true;
				P_Menu_Close(ent);
			}
		}

		if (!AllowTeamSwitch(ent, desired_team))
			return false;

		if (!inactive && ent->client->resp.teamDelayTime > level.time) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
			P_Menu_Close(ent);
			return false;
		}
	} else {
		if (GTF(GTF_1V1)) {
			if (desired_team == TEAM_NONE) {
				desired_team = TEAM_SPECTATOR;
				queue = true;
			}
		}
	}

	// allow the change...

	P_Menu_Close(ent);

	// start as spectator
	if (ent->moveType == MOVETYPE_NOCLIP)
		Weapon_Grapple_DoReset(ent->client);

	CTF_DeadDropFlag(ent);
	Tech_DeadDrop(ent);

	FreeFollower(ent);

	ent->svFlags &= ~SVF_NOCLIENT;
	ent->client->resp.score = 0;
	ent->client->sess.team = desired_team;
	ent->client->resp.ctf_state = 0;
	ent->client->sess.inactive = inactive;
	ent->client->sess.inactivity_time = level.time + 1_min;
	ent->client->sess.teamJoinTime = desired_team == TEAM_SPECTATOR ? 0_sec : level.time;
	ent->client->sess.playStartTime = level.time;
	ent->client->resp.teamDelayTime = force || !ent->client->sess.initialised ? level.time : level.time + 5_sec;
	ent->client->sess.spectator_state = desired_team == TEAM_SPECTATOR ? SPECTATOR_FREE : SPECTATOR_NOT;
	ent->client->sess.spectator_client = 0;
	ent->client->sess.versusQueued = queue;

	if (desired_team != TEAM_SPECTATOR) {
		if (Teams())
			AssignPlayerSkin(ent, ent->client->sess.skin);

		G_RevertVote(ent->client);

		// assign a ghost code
		Match_Ghost_DoAssign(ent);

		// free any followers
		FreeClientFollowers(ent);
	}

	ent->client->sess.initialised = true;

	// if they are playing gauntlet, count as a loss
	if (GT(GT_GAUNTLET) && old_team == TEAM_FREE)
		ent->client->sess.losses++;

	ClientSpawn(ent);
	G_PostRespawn(ent);

	BroadcastTeamChange(ent, old_team, inactive, silent);

	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = desired_team == TEAM_SPECTATOR || ent->client->eliminated ? 0 : 1;

	// if anybody has a menu open, update it immediately
	P_Menu_Dirty();

	return true;
}

/*
=================
Cmd_Team_f
=================
*/
static void Cmd_Team_f(gentity_t *ent) {
	if (gi.argc() == 1) {
		switch (ent->client->sess.team) {
		case TEAM_SPECTATOR:
			gi.LocClient_Print(ent, PRINT_HIGH, "You are spectating.\n");
			break;
		case TEAM_FREE:
			gi.LocClient_Print(ent, PRINT_HIGH, "You are in the match.\n");
			break;
		case TEAM_RED:
		case TEAM_BLUE:
			gi.LocClient_Print(ent, PRINT_HIGH, "Your team: {}\n", Teams_TeamName(ent->client->sess.team));
			break;
		default:
			break;
		}
		return;
	}

	const char *s = gi.argv(1);
	team_t team = StringToTeamNum(s);
	if (team == TEAM_NONE)
		return;

	SetTeam(ent, team, false, false, false);
}

/*
=================
Cmd_CrosshairID_f
=================
*/
static void Cmd_CrosshairID_f(gentity_t *ent) {
	ent->client->sess.pc.show_id ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Player identication display: {}\n", ent->client->sess.pc.show_id ? "ON" : "OFF");
}

/*
=================
Cmd_Timer_f
=================
*/
static void Cmd_Timer_f(gentity_t *ent) {
	ent->client->sess.pc.show_timer ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Match timer display: {}\n", ent->client->sess.pc.show_timer ? "ON" : "OFF");
}

/*
=================
Cmd_FragMessages_f
=================
*/
static void Cmd_FragMessages_f(gentity_t *ent) {
	ent->client->sess.pc.show_fragmessages ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "{} frag messages.\n", ent->client->sess.pc.show_fragmessages ? "Activating" : "Disabling");
}

/*
=================
Cmd_KillBeep_f
=================
*/
static void Cmd_KillBeep_f(gentity_t *ent) {
	int num = 0;
	if (gi.argc() > 1) {
		num = atoi(gi.argv(1));
		if (num < 0)
			num = 0;
		else if (num > 4)
			num = 4;
	} else {
		num = (ent->client->sess.pc.killbeep_num + 1) % 5;
	}
	const char *sb[5] = { "off", "clang", "beep-boop", "insane", "tang-tang" };
	ent->client->sess.pc.killbeep_num = num;
	gi.LocClient_Print(ent, PRINT_HIGH, "Kill beep changed to: {}\n", sb[num]);
}


/*
=================
Cmd_Ghost_f
=================
*/
static void Cmd_Ghost_f(gentity_t *ent) {
	int i, n;

	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "code" }, {}, "Reinstates a previous match state.");
		return;
	}

	if (ClientIsPlaying(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You are already in the game.\n");
		return;
	}
	if (level.match_state != MatchState::MATCH_IN_PROGRESS) {
		gi.LocClient_Print(ent, PRINT_HIGH, "No match is in progress.\n");
		return;
	}

	n = atoi(gi.argv(1));

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (level.ghosts[i].code && level.ghosts[i].code == n) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Ghost code accepted, your position has been reinstated.\n");
			level.ghosts[i].ent->client->resp.ghost = nullptr;
			ent->client->sess.team = level.ghosts[i].team;
			ent->client->resp.ghost = level.ghosts + i;
			ent->client->resp.score = level.ghosts[i].score;
			ent->client->resp.ctf_state = 0;
			level.ghosts[i].ent = ent;
			ent->svFlags = SVF_NONE;
			ent->flags &= ~FL_GODMODE;
			ClientSpawn(ent);
			gi.LocBroadcast_Print(PRINT_HIGH, "{} has been reinstated to {} team.\n",
				ent->client->sess.netName, Teams_TeamName(ent->client->sess.team));
			return;
		}
	}
	gi.LocClient_Print(ent, PRINT_HIGH, "Invalid ghost code.\n");
}


static void Cmd_Stats_f(gentity_t *ent) {
	if (notGTF(GTF_CTF))
		return;


}

static void Cmd_Boot_f(gentity_t *ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "client name/number" }, {}, "Removes the specified client from the server. Does not work properly in Kex.");
		return;
	}

	if (*gi.argv(1) < '0' && *gi.argv(1) > '9') {
		gi.LocClient_Print(ent, PRINT_HIGH, "Specify the player number to kick.\n");
		return;
	}

	gentity_t *targ = ClientEntFromString(gi.argv(1));

	if (targ == nullptr) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid client number.\n");
		return;
	}

	if (targ == &g_entities[1]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot kick the lobby owner.\n");
		return;
	}
	
	if (targ->client->sess.admin) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot kick an admin.\n");
		return;
	}

	gi.AddCommandString(G_Fmt("kick {}\n", targ - g_entities).data());
}

/*----------------------------------------------------------------*/

// NEW VOTING CODE

static bool Vote_Val_None(gentity_t *ent) {
	return true;
}

void Vote_Pass_Map() {
	const MapEntry *map = game.mapSystem.GetMapEntry(level.vote_arg);
	if (!map) {
		gi.Com_Print("Map not found in pool at vote pass stage.\n");
		return;
	}

	level.changeMap = map->filename.c_str();
	game.overrideEnableFlags = level.vote_flags_enable;
	game.overrideDisableFlags = level.vote_flags_disable;

	ExitLevel();
}

static bool Vote_Val_Map(gentity_t *ent) {
	if (gi.argc() < 3 || !Q_strcasecmp(gi.argv(1), "?")) {
		PrintMapList(ent, false);
		return false;
	}

	const char *mapName = gi.argv(2);
	const MapEntry *map = game.mapSystem.GetMapEntry(mapName);
	if (!map) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' not found in map pool.\n", mapName);
		PrintMapList(ent, false);
		return false;
	}

	if (map->lastPlayed.seconds() > 0 && (level.time - map->lastPlayed) < 1800_sec) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' was played recently.\n", mapName);
		return false;
	}

	// Store map name and flags in level vars
	level.vote_arg = map->filename;

	// Parse override flags
	uint8_t enableFlags = 0, disableFlags = 0;
	std::vector<std::string> flags;
	for (int i = 3; i < gi.argc(); ++i)
		flags.emplace_back(gi.argv(i));

	if (!ParseMyMapFlags(flags, enableFlags, disableFlags)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid flag(s). Use +pu -fd etc.\n");
		return false;
	}

	level.vote_flags_enable = enableFlags;
	level.vote_flags_disable = disableFlags;

	return true;
}

void Vote_Pass_RestartMatch() {
	Match_Reset();
}

void Vote_Pass_Gametype() {
	gametype_t gt = GametypeStringToIndex(level.vote_arg.data());
	if (gt == GT_NONE)
		return;
	
	ChangeGametype(gt);
}

static bool Vote_Val_Gametype(gentity_t *ent) {
	if (GametypeStringToIndex(gi.argv(2)) == gametype_t::GT_NONE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid gametype.\n");
		return false;
	}

	return true;
}

static void Vote_Pass_Ruleset() {
	ruleset_t rs = RS_IndexFromString(level.vote_arg.data());
	if (rs == ruleset_t::RS_NONE)
		return;

	gi.cvar_forceset("g_ruleset", G_Fmt("{}", (int)rs).data());
}

static bool Vote_Val_Ruleset(gentity_t *ent) {
	ruleset_t desired_rs = RS_IndexFromString(gi.argv(2));
	if (desired_rs == ruleset_t::RS_NONE) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid ruleset.\n");
		return false;
	}
	if ((int)desired_rs == game.ruleset) {
		gi.Client_Print(ent, PRINT_HIGH, "Ruleset currently active.\n");
		return false;
	}

	return true;
}

void Vote_Pass_NextMap() {
	// Play queue overrides auto-select
	if (!game.mapSystem.playQueue.empty()) {
		const auto &queued = game.mapSystem.playQueue.front();
		level.nextMap = queued.filename;
		level.overrideEnableFlags = queued.settings.to_ulong();
		level.overrideDisableFlags = 0;
		ExitLevel();
		return;
	}

	auto result = AutoSelectNextMap();
	if (result.has_value()) {
		level.nextMap = result->filename;
		level.overrideEnableFlags = 0;
		level.overrideDisableFlags = 0;
		ExitLevel();
	} else {
		gi.LocBroadcast_Print(PRINT_HIGH, "No eligible maps available.\n");
	}
}

void Vote_Pass_ShuffleTeams() {
	TeamSkillShuffle();
	//Match_Reset();
	gi.LocBroadcast_Print(PRINT_HIGH, "Teams have been shuffled.\n");
}

static bool Vote_Val_ShuffleTeams(gentity_t *ent) {
	if (!Teams())
		return false;

	return true;
}

static void Vote_Pass_Unlagged() {
	int argi = strtoul(level.vote_arg.data(), nullptr, 10);
	
	gi.LocBroadcast_Print(PRINT_HIGH, "Lag compensation has been {}.\n", argi ? "ENABLED" : "DISABLED");

	gi.cvar_forceset("g_lag_compensation", argi ? "1" : "0");
}

static bool Vote_Val_Unlagged(gentity_t *ent) {
	int arg = strtoul(gi.argv(2), nullptr, 10);
	
	if ((g_lag_compensation->integer && arg)
			|| (!g_lag_compensation->integer && !arg)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Lag compensation is already {}.\n", arg ? "ENABLED" : "DISABLED");
		return false;
	}

	return true;
}

static bool Vote_Val_Random(gentity_t *ent) {
	int arg = strtoul(gi.argv(2), nullptr, 10);

	if (arg > 100 || arg < 2)
		return false;

	return true;
}

void Vote_Pass_Cointoss() {
	gi.LocBroadcast_Print(PRINT_HIGH, "The coin is: {}\n", brandom() ? "HEADS" : "TAILS");
}

void Vote_Pass_Random() {
	gi.LocBroadcast_Print(PRINT_HIGH, "The random number is: {}\n", irandom(2, atoi(level.vote_arg.data())));
}

void Vote_Pass_Timelimit() {
	const char *s = level.vote_arg.data();
	int argi = strtoul(s, nullptr, 10);
	
	if (!argi)
		gi.LocBroadcast_Print(PRINT_HIGH, "Time limit has been DISABLED.\n");
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "Time limit has been set to {}.\n", TimeString(argi * 60000, false, false));

	gi.cvar_forceset("timelimit", s);
}

static bool Vote_Val_Timelimit(gentity_t *ent) {
	int argi = strtoul(gi.argv(2), nullptr, 10);
	
	if (argi < 0 || argi > 1440) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid time limit value.\n");
		return false;
	}
	
	if (argi == timelimit->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Time limit is already set to {}.\n", TimeString(argi * 60000, false, false));
		return false;
	}
	return true;
}

void Vote_Pass_Scorelimit() {
	int argi = strtoul(level.vote_arg.data(), nullptr, 10);
	
	if (argi)
		gi.LocBroadcast_Print(PRINT_HIGH, "Score limit has been set to {}.\n", argi);
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "Score limit has been DISABLED.\n");

	gi.cvar_forceset(G_Fmt("{}limit", GT_ScoreLimitString()).data(), level.vote_arg.data());
}

static bool Vote_Val_Scorelimit(gentity_t *ent) {
	int argi = strtoul(gi.argv(2), nullptr, 10);
	
	if (argi < 0) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid score limit value.\n");
		return false;
	}

	if (argi == GT_ScoreLimit()) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Score limit is already set to {}.\n", argi);
		return false;
	}

	return true;
}

static void Vote_Pass_BalanceTeams() {
	TeamBalance(true);
}

static bool Vote_Val_BalanceTeams(gentity_t *ent) {
	if (!Teams())
		return false;

	return true;
}

vcmds_t vote_cmds[] = {
	{"map",					Vote_Val_Map,			Vote_Pass_Map,			1,		2,	"[mapname]",						"changes to the specified map"},
	{"nextmap",				Vote_Val_None,			Vote_Pass_NextMap,		2,		1,	"",									"move to the next map in the rotation"},
	{"restart",				Vote_Val_None,			Vote_Pass_RestartMatch,	4,		1,	"",									"restarts the current match"},
	{"gametype",			Vote_Val_Gametype,		Vote_Pass_Gametype,		8,		2,	"<ffa|duel|tdm|ctf|ca|ft|horde|gauntlet>",	"changes the current gametype"},
	{"timelimit",			Vote_Val_Timelimit,		Vote_Pass_Timelimit,	16,		2,	"<0..$>",							"alters the match time limit, 0 for no time limit"},
	{"scorelimit",			Vote_Val_Scorelimit,	Vote_Pass_Scorelimit,	32,		2,	"<0..$>",							"alters the match score limit, 0 for no score limit"},
	{"shuffle",				Vote_Val_ShuffleTeams,	Vote_Pass_ShuffleTeams,	64,		2,	"",									"shuffles teams"},
	{"unlagged",			Vote_Val_Unlagged,		Vote_Pass_Unlagged,		128,	2,	"<0/1>",							"enables or disables lag compensation"},
	{"cointoss",			Vote_Val_None,			Vote_Pass_Cointoss,		256,	1,	"",									"invokes a HEADS or TAILS cointoss"},
	{"random",				Vote_Val_Random,		Vote_Pass_Random,		512,	1,	"<2-100>",							"randomly selects a number from 2 to specified value"},
	{"balance",				Vote_Val_BalanceTeams,	Vote_Pass_BalanceTeams,	1024,	1,	"",									"balance teams without shuffling"},
	{"ruleset",				Vote_Val_Ruleset,		Vote_Pass_Ruleset,		2048,	2,	"<q1/q2/q3a>",						"changes the current ruleset"},
};

/*
===============
FindVoteCmdByName

===============
*/
vcmds_t *FindVoteCmdByName(const char *name) {
	vcmds_t *cc = vote_cmds;

	for (size_t i = 0; i < ARRAY_LEN(vote_cmds); i++, cc++) {
		if (!cc->name)
			continue;
		if (!Q_strcasecmp(cc->name, name))
			return cc;
	}

	return nullptr;
}

/*
==================
Vote_Passed
==================
*/
void Vote_Passed() {
	level.vote->func();

	level.vote = nullptr;
	level.vote_arg.clear();
	level.vote_execute_time = 0_sec;
}

/*
=================
ValidVoteCommand
=================
*/
static bool ValidVoteCommand(gentity_t *ent) {
	if (!ent->client)
		return false; // not fully in game yet

	level.vote = nullptr;

	vcmds_t *cc = FindVoteCmdByName(gi.argv(1));

	if (!cc) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid vote command: {}\n", gi.argv(1));
		return false;
	}
	
	if (cc->args && gi.argc() < (1 + cc->min_args)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{}: {}\nUsage: {} {}\n", cc->name, cc->help, cc->name, cc->args);
		return false;
	}

	if (!cc->val_func(ent))
		return false;

	level.vote = cc;
	level.vote_arg = std::string(gi.argv(2));
	//gi.Com_PrintFmt("argv={} vote_arg={}\n", gi.argv(2), level.vote_arg);
	return true;
}

/*
=================
VoteCommandStore
=================
*/
void VoteCommandStore(gentity_t *ent) {
	// start the voting, the caller automatically votes yes
	level.vote_client = ent->client;
	level.vote_time = level.time;
	level.vote_yes = 1;
	level.vote_no = 0;
	
	gi.LocBroadcast_Print(PRINT_CENTER, "{} called a vote:\n{}{}\n", level.vote_client->sess.netName, level.vote->name, level.vote_arg[0] ? G_Fmt(" {}", level.vote_arg).data() : "");

	for (auto ec : active_clients())
		ec->client->pers.voted = ec == ent ? 1 : 0;

	ent->client->pers.vote_count++;
	AnnouncerSound(world, "vote_now");

	for (auto ec : active_players()) {
		if (ec->svFlags & SVF_BOT)
			continue;

		//gi.local_sound(ec, CHAN_AUTO, gi.soundindex("misc/pc_up.wav"), 1, ATTN_NONE, 0);

		if (ec->client == level.vote_client)
			continue;

		if (!ClientIsPlaying(ec->client) && !g_allow_spec_vote->integer)
			continue;

		ec->client->showInventory = false;
		ec->client->showHelp = false;
		ec->client->showScores = false;
		gentity_t *e = ec->client->followTarget ? ec->client->followTarget : ec;
		ec->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) ? 0 : 1;
		P_Menu_Close(ec);
		G_Menu_Vote_Open(ec);
	}
}

/*
==================
Cmd_CallVote_f
==================
*/
static void Cmd_CallVote_f(gentity_t *ent) {
	if (!deathmatch->integer)
		return;

	// formulate list of allowed voting commands
	vcmds_t *cc = vote_cmds;

	char vstr[1024] = " ";
	for (size_t i = 0; i < ARRAY_LEN(vote_cmds); i++, cc++) {
		if (!cc->name)
			continue;
		
		if (g_vote_flags->integer & cc->flag)
			continue;
			
		strcat(vstr, G_Fmt("{} ", cc->name).data());
	}

	if (!g_allow_voting->integer || strlen(vstr) <= 1) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Voting not allowed here.\n");
		return;
	}

	if (!g_allow_vote_midgame->integer && level.match_state >= MatchState::MATCH_COUNTDOWN) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Voting is only allowed during the warm up period.\n");
		return;
	}

	if (level.vote_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "A vote is already in progress.\n");
		return;
	}

	// if there is still a vote to be executed
	if (level.vote_execute_time || level.restarted) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Previous vote command is still awaiting execution.\n");
		return;
	}

	if (!g_allow_spec_vote->integer && !ClientIsPlaying(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You are not allowed to call a vote as a spectator.\n");
		return;
	}

	if (g_vote_limit->integer && ent->client->pers.vote_count >= g_vote_limit->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You have called the maximum number of votes ({}).\n", g_vote_limit->integer);
		return;
	}

	if (gi.argc() < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} <command> <params>\nValid Voting Commands:{}\n", gi.argv(0), vstr);
		return;
	}

	// make sure it is a valid command to vote on
	if (!ValidVoteCommand(ent))
		return;

	VoteCommandStore(ent);
}

/*
==================
Cmd_Vote_f
==================
*/
static void Cmd_Vote_f(gentity_t *ent) {
	if (!deathmatch->integer)
		return;

	if (!ClientIsPlaying(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Not allowed to vote as spectator.\n");
		return;
	}
	
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "yes", "no" }, {}, "Casts your vote in current voting session.");
		return;
	}

	if (!level.vote_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "No vote in progress.\n");
		return;
	}

	if (ent->client->pers.voted != 0) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Vote already cast.\n");
		return;
	}

	const char *arg = gi.argv(1);

	if (arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1') {
		level.vote_yes++;
		ent->client->pers.voted = 1;
	} else {
		level.vote_no++;
		ent->client->pers.voted = -1;
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "Vote cast.\n");
	
	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

void G_RevertVote(gclient_t *client) {
	if (!level.vote_time)
		return;

	if (!level.vote_client)
		return;

	if (client->pers.voted == 1) {
		level.vote_yes--;
		client->pers.voted = 0;
		//trap_SetConfigstring(CS_VOTE_YES, va("%i", level.vote_yes));
	} else if (client->pers.voted == -1) {
		level.vote_no--;
		client->pers.voted = 0;
		//trap_SetConfigstring(CS_VOTE_NO, va("%i", level.vote_no));
	}
}

/*
=================
Cmd_Follow_f
=================
*/
static void Cmd_Follow_f(gentity_t *ent) {
	if (ClientIsPlaying(ent->client)) {
		gi.Client_Print(ent, PRINT_HIGH, "You must spectate before you can follow.\n");
		return;
	}
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "client name/number" }, {}, "Follows the specified player.");
		return;
	}

	gentity_t *follow_ent = ClientEntFromString(gi.argv(1));

	if (!follow_ent || !follow_ent->inUse) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid client specified.\n");
		return;
	}

	if (ClientIsPlaying(follow_ent->client)) {
		gi.Client_Print(ent, PRINT_HIGH, "Specified client is not playing.\n");
		return;
	}

	ent->client->followTarget = follow_ent;
	ent->client->follow_update = true;
	UpdateChaseCam(ent);
}

/*
=================
Cmd_FollowKiller_f
=================
*/
static void Cmd_FollowKiller_f(gentity_t *ent) {
	ent->client->sess.pc.follow_killer ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow killer: {}\n", ent->client->sess.pc.follow_killer ? "ON" : "OFF");
}

/*
=================
Cmd_FollowLeader_f
=================
*/
static void Cmd_FollowLeader_f(gentity_t *ent) {
	gentity_t *leader = &g_entities[level.sorted_clients[0] + 1];
	ent->client->sess.pc.follow_leader ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow leader: {}\n", ent->client->sess.pc.follow_leader ? "ON" : "OFF");

	if (!ClientIsPlaying(ent->client) && ent->client->sess.pc.follow_leader && ent->client->followTarget != leader) {
		ent->client->followTarget = leader;
		ent->client->follow_update = true;
		UpdateChaseCam(ent);
	}
}

/*
=================
Cmd_FollowPowerup_f
=================
*/
static void Cmd_FollowPowerup_f(gentity_t *ent) {
	ent->client->sess.pc.follow_powerup ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow powerup pick-ups: {}\n", ent->client->sess.pc.follow_powerup ? "ON" : "OFF");
}

/*----------------------------------------------------------------*/

/*
=================
Cmd_LockTeam_f
=================
*/
static void Cmd_LockTeam_f(gentity_t *ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "team" }, {}, "Locks a team, prevents players from joining.");
		return;
	}

	team_t team = StringToTeamNum(gi.argv(1));

	if (team == TEAM_NONE || team == TEAM_SPECTATOR) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}

	if (level.locked[team]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already locked.\n", Teams_TeamName(team));
		return;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: {} has been locked.\n", Teams_TeamName(team));
	level.locked[team] = true;
}

/*
=================
Cmd_UnlockTeam_f
=================
*/
static void Cmd_UnlockTeam_f(gentity_t *ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "team" }, {}, "Unlocks a locked team, allows players to join the team.");
		return;
	}

	team_t team = StringToTeamNum(gi.argv(1));

	if (team == TEAM_NONE || team == TEAM_SPECTATOR) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}

	if (!level.locked[team]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already unlocked.\n", Teams_TeamName(team));
		return;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: {} has been unlocked.\n", Teams_TeamName(team));
	level.locked[team] = false;
}

/*
=================
Cmd_SetTeam_f
=================
*/
static void Cmd_SetTeam_f(gentity_t *ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "client name/number" }, {}, "Moves the client to the team.");
		return;
	}

	gentity_t *targ = ClientEntFromString(gi.argv(1));

	if (!targ || !targ->inUse || !targ->client) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid client name or number.\n");
		return;
	}

	if (gi.argc() == 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is on {} team.\n", targ->client->sess.netName, gi.argv(0));
		return;
	}

	team_t team = StringToTeamNum(gi.argv(2));
	if (team == TEAM_NONE) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}

	if (targ->client->sess.team == team) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already on {} team.\n", targ->client->sess.netName, Teams_TeamName(team));
		return;
	}

	if ((Teams() && team == TEAM_FREE) || (!Teams() && team != TEAM_SPECTATOR && team != TEAM_FREE)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: Moved {} to {} team.\n", targ->client->sess.netName, Teams_TeamName(team));
	SetTeam(targ, team, false, true, false);
}

/*
=================
Cmd_Shuffle_f
=================
*/
static void Cmd_Shuffle_f(gentity_t *ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced team shuffle.\n");
	TeamSkillShuffle();
	//Match_Reset();
}

/*
=================
Cmd_BalanceTeams_f
=================
*/
static void Cmd_BalanceTeams_f(gentity_t *ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced team balancing.\n");
	TeamBalance(true);
}

/*
=================
Cmd_StartMatch_f
=================
*/
static void Cmd_StartMatch_f(gentity_t *ent) {
	if (level.match_state > MatchState::MATCH_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has already started.\n");
		return;
	}

	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced match start.\n");
	Match_Start();
}

/*
=================
Cmd_EndMatch_f
=================
*/
static void Cmd_EndMatch_f(gentity_t *ent) {
	if (level.match_state < MatchState::MATCH_IN_PROGRESS) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has not yet begun.\n");
		return;
	}
	if (level.intermissionTime) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has already ended.\n");
		return;
	}
	QueueIntermission("[ADMIN]: Forced match end.", true, false);
}

/*
=================
Cmd_ResetMatch_f
=================
*/
static void Cmd_ResetMatch_f(gentity_t *ent) {
	if (level.match_state < MatchState::MATCH_IN_PROGRESS) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has not yet begun.\n");
		return;
	}
	if (level.intermissionTime) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has already ended.\n");
		return;
	}
	
	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: Forced match reset.\n");
	Match_Reset();
}

/*
=================
Cmd_ForceVote_f
=================
*/
static void Cmd_ForceVote_f(gentity_t *ent) {
	if (!deathmatch->integer)
		return;

	if (!level.vote_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "No vote in progress.\n");
		return;
	}

	const char *arg = gi.argv(1);

	if (arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1') {
		gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Passed the vote.\n");
		level.vote_execute_time = level.time + 3_sec;
		level.vote_client = nullptr;
	} else {
		gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Failed the vote.\n");
		level.vote_time = 0_sec;
		level.vote_client = nullptr;
	}
}

/*
=================
Cmd_Gametype_f
=================
*/
static void Cmd_Gametype_f(gentity_t *ent) {
	if (!deathmatch->integer)
		return;

	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0),
			{ "gametype name" },
			{},
			G_Fmt("Changes the current gametype. Current gametype is {} ({}).\nValid gametypes: {}\n",
				gt_long_name[g_gametype->integer],
				g_gametype->integer,
				GametypeOptionList()).data()
		);
		return;
	}

	gametype_t gt = GametypeStringToIndex(gi.argv(1));
	if (gt == GT_NONE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid gametype.\n");
		return;
	}

	ChangeGametype(gt);
}

/*
=================
Cmd_Ruleset_f
=================
*/
static void Cmd_Ruleset_f(gentity_t *ent) {
	if (!deathmatch->integer)
		return;

	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0),
			{ "q1/q2/q3a" },
			{},
			G_Fmt("Changes the current ruleset. Current ruleset is {} ({}).\nValid rulesets: <q1|q2|q3a>\n",
				rs_long_name[static_cast<int>(game.ruleset)],
				static_cast<int>(game.ruleset)).data()
		);
		return;
	}

	ruleset_t rs = RS_IndexFromString(gi.argv(1));
	if (rs == RS_NONE) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid ruleset.\n");
		return;
	}

	gi.cvar_forceset("g_ruleset", G_Fmt("{}", (int)rs).data());
}

static void Cmd_SetMap_f(gentity_t *ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0),
			{ "mapname" },
			{},
			"Changes to a map within the map list."
		);
		PrintMapList(ent);
		return;
	}

	if (g_map_list->string[0] && !strstr(g_map_list->string, gi.argv(1))) {
		gi.Client_Print(ent, PRINT_HIGH, "Map name is not valid.\n");
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: Changing map to {}\n", gi.argv(1));
	level.changeMap = gi.argv(1);
	ExitLevel();
}

extern void ClearWorldEntities();
static void Cmd_MapRestart_f(gentity_t *ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Session reset.\n");

	//TODO: reset match variables, clear world entities, reload world entities
	//SpawnEntities(level.mapname, level.entstring.c_str(), nullptr);
	//Match_Reset();
	//ClearWorldEntities();
	gi.AddCommandString(G_Fmt("gamemap {}\n", level.mapname).data());
}

static void Cmd_NextMap_f(gentity_t *ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Changing to next map.\n");

	// If queue has maps, use the first
	if (!game.mapSystem.playQueue.empty()) {
		const auto &queued = game.mapSystem.playQueue.front();
		level.nextMap = queued.filename;
		level.overrideEnableFlags = queued.settings.to_ulong();
		level.overrideDisableFlags = 0;

		game.mapSystem.playQueue.erase(game.mapSystem.playQueue.begin());
		Match_End();
		level.intermissionPreExit = true;
		return;
	}

	// Otherwise auto-select one
	if (auto result = AutoSelectNextMap(); result.has_value()) {
		level.nextMap = result->filename;
		level.overrideEnableFlags = 0;
		level.overrideDisableFlags = 0;

		Match_End();
		level.intermissionPreExit = true;
	} else {
		gi.LocClient_Print(ent, PRINT_HIGH, "No eligible next map found.\n");
	}
}

static void Cmd_Admin_f(gentity_t *ent) {
	if (!g_allow_admin->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Administration is disabled\n");
		return;
	}
	
	if (gi.argc() > 1) {
		if (ent->client->sess.admin) {
			gi.Client_Print(ent, PRINT_HIGH, "You already have administrative rights.\n");
			return;
		}
		if (admin_password->string && *admin_password->string && Q_strcasecmp(admin_password->string, gi.argv(1)) == 0) {
			if (!ent->client->sess.admin) {
				ent->client->sess.admin = true;
				gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", ent->client->sess.netName);
			}
			return;
		}
	}
	
	// run command if valid...

}

/*----------------------------------------------------------------*/

static bool ReadyConditions(gentity_t *ent, bool desired_status, bool admin_cmd) {
	if (level.match_state == MatchState::MATCH_WARMUP_READYUP)
		return true;

	const char *s = nullptr;
	if (admin_cmd) {
		s = "You cannot force ready status until ";
	} else {
		s = "You cannot change your ready status until ";
	}

	switch (level.warmup_requisite) {
	case WarmupState::WARMUP_REQ_MORE_PLAYERS:
	{
		int minp = GTF(GTF_1V1) ? 2 : minplayers->integer;
		int req = minp - level.num_playing_clients;
		gi.LocClient_Print(ent, PRINT_HIGH, "{}{} more player{} present.\n", s, req, req > 1 ? "s are" : " is");
		break;
	}
	case WarmupState::WARMUP_REQ_BALANCE:
		gi.LocClient_Print(ent, PRINT_HIGH, "{}teams are balanced.\n", s);
		break;
	default:
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot {}ready at this stage of the match.\n", desired_status ? "" : "un");
		break;
	}
	return false;
}

static void Cmd_ReadyAll_f(gentity_t *ent) {
	if (!ReadyConditions(ent, true, true))
		return;

	ReadyAll();

	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced all players to ready status\n");
}

static void Cmd_UnReadyAll_f(gentity_t *ent) {
	if (!ReadyConditions(ent, false, true))
		return;

	UnReadyAll();

	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced all players to NOT ready status\n");
}

static void BroadcastReadyStatus(gentity_t *ent) {
	gi.LocBroadcast_Print(PRINT_CENTER, "%bind:+wheel2:Use Compass to toggle your ready status.%MATCH IS IN WARMUP\n{} is {}ready.", ent->client->sess.netName, ent->client->resp.ready ? "" : "NOT ");
}

static void Cmd_Ready_f(gentity_t *ent) {
	if (!ReadyConditions(ent, true, false))
		return;

	if (level.match_state != MatchState::MATCH_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready at this stage of the match.\n");
		return;
	}

	if (ent->client->resp.ready) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You have already committed.\n");
		return;
	}

	ent->client->resp.ready = true;
	BroadcastReadyStatus(ent);
}

static void Cmd_NotReady_f(gentity_t *ent) {
	if (!ReadyConditions(ent, false, false))
		return;

	if (!ent->client->resp.ready) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You haven't committed.\n");
		return;
	}

	ent->client->resp.ready = false;
	BroadcastReadyStatus(ent);
}

void Cmd_ReadyUp_f(gentity_t *ent) {
	if (!ReadyConditions(ent, !ent->client->resp.ready, false))
		return;

	ent->client->resp.ready ^= true;
	BroadcastReadyStatus(ent);
}

static void Cmd_Hook_f(gentity_t *ent) {
	if (!g_allow_grapple->integer || !g_grapple_offhand->integer)
		return;

	Weapon_Hook(ent);
}

static void Cmd_UnHook_f(gentity_t *ent) {
	Weapon_Grapple_DoReset(ent->client);
}

static void Cmd_MapInfo_f(gentity_t *ent) {
	if (level.mapname[0])
		gi.LocClient_Print(ent, PRINT_HIGH, "MAP INFO:\nfilename: {}\n", level.mapname);
	else return;
	if (level.levelName[0])
		gi.LocClient_Print(ent, PRINT_HIGH, "longname: {}\n", level.levelName);
	if (level.author[0])
		gi.LocClient_Print(ent, PRINT_HIGH, "author{}: {}{}{}\n", level.author2[0] ? "s" : "", level.author, level.author2[0] ? ", " : "", level.author2[0] ? level.author2 : "");
}

static void Cmd_LoadMotd_f(gentity_t *ent) {
	LoadMotd();
}

static void Cmd_Motd_f(gentity_t *ent) {
	const char *s = nullptr;

	if (game.motd.size())
		s = G_Fmt("Message of the Day:\n{}\n", game.motd.c_str()).data();
	else
		s = "No Message of the Day set.\n";
	gi.LocClient_Print(ent, PRINT_HIGH, "{}", s);
}

static void Cmd_MySkill_f(gentity_t *ent) {
	int totalSkill = 0, numPlayers = 0;
	int averageSkill = 0;

	// count total skill rating
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		totalSkill += ec->client->sess.skillRating;
		numPlayers++;
	}

	if (totalSkill && numPlayers)
		averageSkill = totalSkill / numPlayers;

	gi.LocClient_Print(ent, PRINT_HIGH, "Your Skill Rating in {}: {} (server avg: {})\n", level.gametype_name, ent->client->sess.skillRating, averageSkill);
}

// ======================================================

void PrintMapList(gentity_t *ent, bool cycleOnly) {
	if (!ent || !ent->client)
		return;

	const int maxMsgLen = 1024;
	const int maxLineLen = 120;
	int longestName = 0;

	// Determine longest map name for formatting
	for (const auto &map : game.mapSystem.mapPool) {
		if (cycleOnly && !map.isCycleable)
			continue;
		if ((int)map.filename.length() > longestName)
			longestName = (int)map.filename.length();
	}

	int colWidth = longestName + 1;
	int cols = maxLineLen / colWidth;
	if (cols == 0) cols = 1;

	std::string message;
	int colCount = 0;

	for (const auto &map : game.mapSystem.mapPool) {
		if (cycleOnly && !map.isCycleable)
			continue;

		message += map.filename;
		message.append(colWidth - map.filename.length(), ' '); // pad spaces

		if (++colCount >= cols) {
			message += '\n';
			colCount = 0;
		}
	}

	// Ensure proper message segmentation
	size_t pos = 0;
	while (pos < message.length()) {
		std::string part = message.substr(pos, maxMsgLen);
		size_t lastNewline = part.find_last_of('\n');
		if (lastNewline != std::string::npos && (pos + lastNewline) < message.length())
			part = message.substr(pos, lastNewline + 1);

		gi.LocClient_Print(ent, PRINT_HIGH, "{}", part);
		pos += part.length();
	}
}

void Cmd_MapPool_f(gentity_t *ent) {
	PrintMapList(ent, false); // Show all maps
}

void Cmd_MapCycle_f(gentity_t *ent) {
	PrintMapList(ent, true); // Show only cycleable maps
}

static void Cmd_LoadMapPool_f(gentity_t *ent) {
	LoadMapPool(ent);
	LoadMapCycle(ent);
}

static void Cmd_LoadMapCycle_f(gentity_t *ent) {
	LoadMapCycle(ent);
}

bool ParseMyMapFlags(const std::vector<std::string> &args, uint8_t &enableFlags, uint8_t &disableFlags) {
	enableFlags = 0;
	disableFlags = 0;

	for (const std::string &arg : args) {
		if (arg.length() < 2 || (arg[0] != '+' && arg[0] != '-'))
			return false;

		bool enable = (arg[0] == '+');
		const char *flag = arg.c_str() + 1;

		uint8_t bit = 0;
		if (_stricmp(flag, "pu") == 0) bit = MAPFLAG_PU;
		else if (_stricmp(flag, "pa") == 0) bit = MAPFLAG_PA;
		else if (_stricmp(flag, "ar") == 0) bit = MAPFLAG_AR;
		else if (_stricmp(flag, "am") == 0) bit = MAPFLAG_AM;
		else if (_stricmp(flag, "ht") == 0) bit = MAPFLAG_HT;
		else if (_stricmp(flag, "bfg") == 0) bit = MAPFLAG_BFG;
		else if (_stricmp(flag, "fd") == 0) bit = MAPFLAG_FD;
		else if (_stricmp(flag, "sd") == 0) bit = MAPFLAG_SD;
		else return false;

		if (enable) enableFlags |= bit;
		else disableFlags |= bit;
	}

	return true;
}

constexpr int MAX_MYMAP_QUEUE = 8;

void Cmd_MyMap_f(gentity_t *ent) {
	if (!ent || !ent->client || !g_maps_mymap->integer)
		return;

	const char *socialID = ent->client->sess.socialID;
	if (!socialID || !*socialID) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You must be logged in to use MyMap.\n");
		return;
	}

	int argc = gi.argc();
	if (argc < 2 || strcmp(gi.argv(1), "?") == 0) {
		gi.LocClient_Print(ent, PRINT_HIGH,
			"MyMap Usage:\n  mymap <mapname> [+flag] [-flag] ...\n"
			"Flags: +pu +pa +ar +am +ht +bfg +fd +sd (prefix with - to disable)\n"
			"Use 'mymap ?' to view this message\n"
			"Use 'mappool' to list maps, or see queue below.\n");

		if (!game.mapSystem.playQueue.empty()) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Current Queue:\n");
			for (const auto &q : game.mapSystem.playQueue) {
				gi.LocClient_Print(ent, PRINT_HIGH, "- {}\n", q.filename);
			}
		}
		return;
	}

	std::string mapName = gi.argv(1);
	const MapEntry *map = game.mapSystem.GetMapEntry(mapName);
	if (!map) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' not found in map pool.\n", mapName);
		return;
	}

	if (game.mapSystem.IsMapInQueue(mapName)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' is already in the play queue.\n", mapName);
		return;
	}

	if (game.mapSystem.IsClientInQueue(socialID)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You already have a map queued.\n");
		return;
	}

	if (map->lastPlayed.seconds() > 0 && (level.time - map->lastPlayed) < 1800_sec) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' was played recently. Try a different one.\n", mapName);
		return;
	}

	if (game.mapSystem.playQueue.size() >= MAX_MYMAP_QUEUE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "The play queue is full.\n");
		return;
	}

	uint8_t enableFlags = 0, disableFlags = 0;
	std::vector<std::string> flagArgs;
	for (int i = 2; i < argc; ++i)
		flagArgs.emplace_back(gi.argv(i));

	if (!ParseMyMapFlags(flagArgs, enableFlags, disableFlags)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid flag(s). Use 'mymap ?' for help.\n");
		return;
	}

	QueuedMap queued;
	queued.filename = map->filename;
	queued.socialID = socialID;
	queued.settings = (enableFlags | disableFlags);

	game.mapSystem.playQueue.push_back(queued);

	MyMapRequest req;
	req.mapName = map->filename;
	req.socialID = socialID;
	req.enableFlags = enableFlags;
	req.disableFlags = disableFlags;
	req.queuedTime = level.time;

	game.mapSystem.myMapQueue.push_back(req);

	gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' added to the queue.\n", map->filename);
}

// =========================================

cmds_t client_cmds[] = {
	{"admin",			Cmd_Admin_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},						// enables admin rights, requires entering in admin password
	{"alertall",		Cmd_AlertAll_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// set all monsters in level to be alerted to presence of player
	{"balance",			Cmd_BalanceTeams_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// balance the teams without shuffling
	{"boot",			Cmd_Boot_f,				CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// remove a client from the server, non-functional
	{"callvote",		Cmd_CallVote_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// call a vote
	{"checkpoi",		Cmd_CheckPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// details all points of interest in campaigns
	{"clear_ai_enemy",	Cmd_Clear_AI_Enemy_f,	CF_CHEAT_PROTECT},									// opposite of alertall, makes all monsters forget the player
	{"clientlist",		Cmd_ClientList_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},		// lists all clients in server, arg1 score or time to sort
	{"cv",				Cmd_CallVote_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// short-hand for callvote
	{"drop",			Cmd_Drop_f,				CF_NONE},											// drop an item from inventory
	{"drop_index",		Cmd_Drop_f,				CF_NONE},											// drop an item from inventory using index
	{"endmatch",		Cmd_EndMatch_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// force an end of a match
	{"fm",				Cmd_FragMessages_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allow client to toggle display of frag messages
	{"follow",			Cmd_Follow_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// follow a player
	{"followkiller",	Cmd_FollowKiller_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// toggles behaviour to auto-follow killers as kills arise
	{"followleader",	Cmd_FollowLeader_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// toggles behaviour to auto-follow score leader as rankings are updated
	{"followpowerup",	Cmd_FollowPowerup_f,	CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// toggles behaviour to auto-follow players when they pick up a powerup
	{"forcevote",		Cmd_ForceVote_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// force a voting result
	{"forfeit",			Cmd_Forfeit_f,			CF_ALLOW_DEAD, true},								// forfeit a match
	{"gametype",		Cmd_Gametype_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// change the gametype, resets the game
	{"ghost",			Cmd_Ghost_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},		// activates ghost reinstatement into match
	{"give",			Cmd_Give_f,				CF_CHEAT_PROTECT, true},							// give an item
	{"god",				Cmd_God_f,				CF_CHEAT_PROTECT, true},							// toggles god mode
	{"help",			Cmd_Help_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC, true},				// shows help (in campaigns) or scoreboard (in deathmatch)
	{"hook",			Cmd_Hook_f,				CF_NONE, true},										// fires off-hand hook (when enabled on server)
	{"id",				Cmd_CrosshairID_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allows clients to toggle crosshair ID drawing
	{"immortal",		Cmd_Immortal_f,			CF_CHEAT_PROTECT},									// cheat to take damage down to a minimum of 1 hp
	{"invdrop",			Cmd_InvDrop_f,			CF_NONE},											// drops current inventory item
	{"inven",			Cmd_Inven_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC, true},				// toggles drawing of inventory list (campaigns) or match menu (deathmatch)
	{"invnext",			Cmd_InvNext_f,			CF_ALLOW_SPEC, true},	//spec for menu up/down		// cycles to next inventory item
	{"invnextp",		Cmd_InvNextP_f,			CF_NONE, true},										// cycles to next powerup in inventory
	{"invnextw",		Cmd_InvNextW_f,			CF_NONE, true},										// cycles to next weapon in inventory
	{"invprev",			Cmd_InvPrev_f,			CF_ALLOW_SPEC, true},	//spec for menu up/down		// cycles to previous inventory item
	{"invprevp",		Cmd_InvPrevP_f,			CF_NONE, true},										// cycles to previous powerup in inventory
	{"invprevw",		Cmd_InvPrevW_f,			CF_NONE, true},										// cycles to previous weapon in inventory
	{"invuse",			Cmd_InvUse_f,			CF_ALLOW_SPEC, true},	//spec for menu up/down		// uses current inventory item
	{"kb",				Cmd_KillBeep_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allows clients to toggle or specify killbeeps
	{"kill",			Cmd_Kill_f,				CF_NONE},											// player commits suicide
	{"kill_ai",			Cmd_Kill_AI_f,			CF_CHEAT_PROTECT},									// removes all monsters from level
	{"listentities",	Cmd_ListEntities_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},	// lists all entities in level
	{"listmonsters",	Cmd_ListMonsters_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},	// lists all monsters in level
	{"loadmotd",		Cmd_LoadMotd_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// load/refresh message of the day from file
	{"loadmappool",		Cmd_LoadMapPool_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC },		// load map pool file
	{"loadmapcycle",	Cmd_LoadMapCycle_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC },		// load map cycle file
	{"lockteam",		Cmd_LockTeam_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// locks a team (prevents joining)
	{"map_restart",		Cmd_MapRestart_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// restarts the current map, does not reset game state
	{"mapinfo",			Cmd_MapInfo_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// prints details about current map
	{"mappool",			Cmd_MapPool_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// prints current map pool
	{"mapcycle",		Cmd_MapCycle_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// prints current map cycle
	{"motd",			Cmd_Motd_f,				CF_ALLOW_SPEC | CF_ALLOW_INT},						// prints message of the day
	{"mymap",			Cmd_MyMap_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// adds a map to the play queue
	{"nextmap",			Cmd_NextMap_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// changes to the next map in the queue/picks from map list
	{"noclip",			Cmd_NoClip_f,			CF_CHEAT_PROTECT, true},							// toggles no clipping
	{"notarget",		Cmd_NoTarget_f,			CF_CHEAT_PROTECT, true},							// toggles no targetting from monsters
	{"notready",		Cmd_NotReady_f,			CF_ALLOW_DEAD},										// sets "not ready" status during warmup readyup stage
	{"novisible",		Cmd_NoVisible_f,		CF_CHEAT_PROTECT},									// toggles players being "invisible" to monsters
	{"putaway",			Cmd_PutAway_f,			CF_ALLOW_SPEC},	//spec for menu close				// hides weapon
	{"ready",			Cmd_Ready_f,			CF_ALLOW_DEAD},										// sets "ready" status during warmup readyup stage
	{"readyall",		Cmd_ReadyAll_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces all players to be ready during warmup readyup stage
	{"readyup",			Cmd_ReadyUp_f,			CF_ALLOW_DEAD},										// toggles ready status during warmup readyup stage
	{"resetmatch",		Cmd_ResetMatch_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces the match back to warmup
	{"ruleset",			Cmd_Ruleset_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// changes the ruleset
	{"score",			Cmd_Score_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC, true},// show scores
	{"setpoi",			Cmd_SetPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// sets the client's origin as a point of interest
	{"setmap",			Cmd_SetMap_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin: change the map, restricted to maps within the map list
	{"setteam",			Cmd_SetTeam_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces a client to a team
	{"shuffle",			Cmd_Shuffle_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// shuffles the teams, uses skill ratings as a reference
	{"spawn",			Cmd_Spawn_f,			CF_ADMIN_ONLY | CF_ALLOW_SPEC},						// spawns an entity
	{"sr",				Cmd_MySkill_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// shows client's skill rating in current server and gametype
	{"startmatch",		Cmd_StartMatch_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces the match to start
	{"stats",			Cmd_Stats_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},						// 
	{"target",			Cmd_Target_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},	// 
	{"team",			Cmd_Team_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// joins a team
	{"teleport",		Cmd_Teleport_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// moves the client to specified coordinates and optionally sets view angles
	{"time-out",		Cmd_TimeOut_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// calls a timeout during a match
	{"time-in",			Cmd_TimeIn_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// cancels a timeout during a match
	{"timer",			Cmd_Timer_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allows client to toggle drawing/hiding of HUD clock
	{"unhook",			Cmd_UnHook_f,			CF_NONE, true},										// releases off-hand hook (when enabled on server)
	{"unlockteam",		Cmd_UnlockTeam_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// unlocks a locked team (allows joining)
	{"unreadyall",		Cmd_UnReadyAll_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces all players to "not ready" status during warmup readyup stage
	{"use",				Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item by name
	{"use_index",		Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item by index
	{"use_index_only",	Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item only by index
	{"use_only",		Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item only by name
	{"vote",			Cmd_Vote_f,				CF_ALLOW_DEAD},										// cast a vote for the current voting session
	{"wave",			Cmd_Wave_f,				CF_NONE},											// issues a player animation and corresponding broadcasted message
	{"weaplast",		Cmd_WeapLast_f,			CF_NONE, true},										// switches to last used weapon
	{"weapnext",		Cmd_WeapNext_f,			CF_NONE, true},										// switches to next weapon in cycle
	{"weapprev",		Cmd_WeapPrev_f,			CF_NONE, true},										// switches to previous weapon in cycle
	{"where",			Cmd_Where_f,			CF_ALLOW_SPEC},										// prints out client's current world coordinates and viee angles, copies to clipboard
};

/*
===============
FindClientCmdByName
===============
*/
static cmds_t *FindClientCmdByName(const char *name) {
	cmds_t	*cc = client_cmds;

	for (size_t i = 0; i < (sizeof(client_cmds) / sizeof(client_cmds[0])); i++, cc++) {
		if (!cc->name)
			continue;
		if (!Q_strcasecmp(cc->name, name))
			return cc;
	}

	return nullptr;
}

/*
=================
HandleDynamicCvarCommand

Allows replace_* and disable_* cvars to be used by server host at all times.
=================
*/
static inline bool HandleDynamicCvarCommand(gentity_t *ent, const char *cmd) {
	if (gi.argc() > 1 && (strstr(cmd, "replace_") || strstr(cmd, "disable_"))) {
		gi.cvar_forceset(cmd, gi.argv(1));
		return true;
	}
	return false;
}

/*
=================
HasCommandPermission
=================
*/
static inline bool HasCommandPermission(gentity_t *ent, const cmds_t *cmd) {
	if ((cmd->flags & CF_ADMIN_ONLY) && !AdminOk(ent)) return false;
	if ((cmd->flags & CF_CHEAT_PROTECT) && !CheatsOk(ent)) return false;
	if (!(cmd->flags & CF_ALLOW_DEAD) && !AliveOk(ent)) return false;
	if (!(cmd->flags & CF_ALLOW_SPEC) && !SpectatorOk(ent)) return false;
	if (!(cmd->flags & CF_ALLOW_INT) && level.intermissionTime) return false;
	return true;
}

constexpr int FLOOD_LIMIT = 6;             // max allowed commands
constexpr int FLOOD_TIME_MS = 4000;        // window in milliseconds
constexpr int FLOOD_SILENCE_MS = 3000;     // lockout if flood exceeded

/*
=================
CmdFloodCheck
=================
*/
static inline bool CmdFloodCheck(gentity_t *ent) {
	if (!ent || !ent->client)
		return false;

	gclient_t *cl = ent->client;

	// If they're still in silence timeout, block them
	if (level.time < cl->sess.command_flood_time)
		return true;

	// Count and compare window
	if ((level.time - cl->sess.command_flood_time).milliseconds() > FLOOD_TIME_MS) {
		cl->sess.command_flood_count = 1;
		cl->sess.command_flood_time = level.time;
	} else {
		cl->sess.command_flood_count++;
		if (cl->sess.command_flood_count > FLOOD_LIMIT) {
			cl->sess.command_flood_time = level.time + gtime_t::from_ms(FLOOD_SILENCE_MS);
			gi.LocClient_Print(ent, PRINT_HIGH, "Command flood detected. Please wait a moment before trying again.\n");
			return true;
		}
	}

	return false;
}

/*
=================
ClientCommand
=================
*/
void ClientCommand(gentity_t *ent) {
	cmds_t		*cc;
	const char	*cmd;

	if (!ent->client)
		return; // not fully in game yet

	cmd = gi.argv(0);
	cc = FindClientCmdByName(cmd);

	// [Paril-KEX] these have to go through the lobby system
#ifndef KEX_Q2_GAME
	if (!Q_strcasecmp(cmd, "say")) {
		Cmd_Say_f(ent, false);
		return;
	}
	if (!Q_strcasecmp(cmd, "say_team") || !Q_strcasecmp(cmd, "steam")) {
		if (Teams())
			Cmd_Say_Team_f(ent, gi.args());
		else
			Cmd_Say_f(ent, false);
		return;
	}
#endif

	if (cc->floodExempt) {
		// exempted
	} else if (CmdFloodCheck(ent)) {
		return;
	}

	if (!HandleDynamicCvarCommand(ent, cmd)) return;
	if (!HasCommandPermission(ent, cc)) return;

	cc->func(ent);
}
