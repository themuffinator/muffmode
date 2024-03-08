// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "m_player.h"
/*freeze*/
#if 0
#include "freeze.h"
#endif
/*freeze*/

static void Cmd_Print_State(edict_t *ent, bool on_state) {
	const char *s = gi.argv(0);
	if (s)
		gi.LocClient_Print(ent, PRINT_HIGH, "{} {}\n", s, on_state ? "ON" : "OFF");
}


static inline bool CheatsOk(edict_t *ent) {
	if (!deathmatch->integer && !coop->integer) {
		return true;
	}
	
	if (!sv_cheats->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Cheats must be enabled to use this command.\n");
		return false;
	}

	return true;
}

static inline bool AliveOk(edict_t *ent) {
	if (ent->health <= 0) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You must be alive to use this command.\n");
		return false;
	}

	return true;
}

static inline bool SpectatorOk(edict_t *ent) {
	if (ClientIsSpectating(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Spectators cannot use this command.\n");
		return false;
	}

	return true;
}

static inline bool MatchOk(edict_t *ent) {
	if (!deathmatch->integer || level.match == MATCH_NONE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Command can only be used in match mode.\n");
		return false;
	}

	return true;
}

static inline bool AdminOk(edict_t *ent) {
	if (level.match == MATCH_NONE || !allow_admin->integer || !ent->client->resp.admin) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Only admins can use this command.\n");
		return false;
	}

	return true;
}

//=================================================================================

static void SelectNextItem(edict_t *ent, item_flags_t itflags, bool menu = true) {
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t *it;

	cl = ent->client;

	// ZOID
	if (menu && cl->menu) {
		PMenu_Next(ent);
		return;
	} else if (menu && cl->chase_target) {
		ChaseNext(ent);
		return;
	}
	// ZOID

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		index = static_cast<item_id_t>((cl->pers.selected_item + i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
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

static void Cmd_InvNextP_f(edict_t *ent) {
	SelectNextItem(ent, IF_POWERUP | IF_SUPER_POWERUP | IF_SPHERE);
}

static void Cmd_InvNextW_f(edict_t *ent) {
	SelectNextItem(ent, IF_WEAPON);
}

static void Cmd_InvNext_f(edict_t *ent) {
	SelectNextItem(ent, IF_ANY);
}

static void SelectPrevItem(edict_t *ent, item_flags_t itflags) {
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t *it;

	cl = ent->client;

	// ZOID
	if (cl->menu) {
		PMenu_Prev(ent);
		return;
	} else if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}
	// ZOID

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		index = static_cast<item_id_t>((cl->pers.selected_item + IT_TOTAL - i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
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

static void Cmd_InvPrevP_f(edict_t *ent) {
	SelectPrevItem(ent, IF_POWERUP | IF_SUPER_POWERUP | IF_SPHERE);
}

static void Cmd_InvPrevW_f(edict_t *ent) {
	SelectPrevItem(ent, IF_WEAPON);
}

static void Cmd_InvPrev_f(edict_t *ent) {
	SelectPrevItem(ent, IF_ANY);
}

void ValidateSelectedItem(edict_t *ent) {
	gclient_t *cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return; // valid

	SelectNextItem(ent, IF_ANY, false);
}

//=================================================================================

static void SpawnAndGiveItem(edict_t *ent, item_id_t id) {
	gitem_t *it = GetItemByIndex(id);

	if (!it)
		return;

	edict_t *it_ent = G_Spawn();
	it_ent->classname = it->classname;
	SpawnItem(it_ent, it);

	if (it_ent->inuse) {
		Touch_Item(it_ent, ent, null_trace, true);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void Cmd_Give_f(edict_t *ent) {
	const char *name;
	gitem_t *it;
	item_id_t index;
	int		  i;
	bool	  give_all;
	edict_t *it_ent;

	name = gi.args();

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
			it = itemlist + i;
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
			SpawnAndGiveItem(ent, IT_ITEM_PACK);

		for (i = 0; i < IT_TOTAL; i++) {
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_AMMO))
				continue;
			Add_Ammo(ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "armor") == 0) {
		ent->client->pers.inventory[IT_ARMOR_JACKET] = 0;
		ent->client->pers.inventory[IT_ARMOR_COMBAT] = 0;
		ent->client->pers.inventory[IT_ARMOR_BODY] = GetItemByIndex(IT_ARMOR_BODY)->armor_info->max_count;

		if (!give_all)
			return;
	}

	if (give_all) {
		SpawnAndGiveItem(ent, IT_ITEM_POWER_SHIELD);

		if (!give_all)
			return;
	}

	if (give_all) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemlist + i;
			if (!it->pickup)
				continue;
			// ROGUE
			if (it->flags & (IF_ARMOR | IF_POWER_ARMOR | IF_WEAPON | IF_AMMO | IF_NOT_GIVEABLE | IF_TECH))
				continue;
			else if (it->pickup == GT_CTF_PickupFlag)
				continue;
			else if ((it->flags & IF_HEALTH) && !it->use)
				continue;
			// ROGUE
			ent->client->pers.inventory[i] = (it->flags & IF_KEY) ? 8 : 1;
		}

		G_CheckPowerArmor(ent);
		ent->client->pers.power_cubes = 0xFF;
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

	// ROGUE
	if (it->flags & IF_NOT_GIVEABLE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_giveable");
		return;
	}
	// ROGUE

	index = it->id;

	if (!it->pickup) {
		ent->client->pers.inventory[index] = 1;
		return;
	}

	if (it->flags & IF_AMMO) {
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	} else {
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem(it_ent, it);
		// PMM - since some items don't actually spawn when you say to ..
		if (!it_ent->inuse)
			return;
		// pmm
		Touch_Item(it_ent, ent, null_trace, true);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}

static void Cmd_SetPOI_f(edict_t *self) {
	level.current_poi = self->s.origin;
	level.valid_poi = true;
}

static void Cmd_CheckPOI_f(edict_t *self) {
	if (!level.valid_poi)
		return;

	char visible_pvs = gi.inPVS(self->s.origin, level.current_poi, false) ? 'y' : 'n';
	char visible_pvs_portals = gi.inPVS(self->s.origin, level.current_poi, true) ? 'y' : 'n';
	char visible_phs = gi.inPHS(self->s.origin, level.current_poi, false) ? 'y' : 'n';
	char visible_phs_portals = gi.inPHS(self->s.origin, level.current_poi, true) ? 'y' : 'n';

	gi.Com_PrintFmt("pvs {} + portals {}, phs {} + portals {}\n", visible_pvs, visible_pvs_portals, visible_phs, visible_phs_portals);
}

// [Paril-KEX]
static void Cmd_Target_f(edict_t *ent) {
	ent->target = gi.argv(1);
	G_UseTargets(ent, ent);
	ent->target = nullptr;
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f(edict_t *ent) {
	ent->flags ^= FL_GODMODE;
	Cmd_Print_State(ent, ent->flags & FL_GODMODE);
}
void ED_ParseField(const char *key, const char *value, edict_t *ent);

/*
==================
Cmd_Immortal_f

Sets client to immortal - take damage but never go below 1 hp

argv(0) immortal
==================
*/
static void Cmd_Immortal_f(edict_t *ent) {
	ent->flags ^= FL_IMMORTAL;
	Cmd_Print_State(ent, ent->flags & FL_IMMORTAL);
}

/*
=================
Cmd_Spawn_f

Spawn class name

argv(0) spawn
argv(1) <classname>
argv(2+n) "key"...
argv(3+n) "value"...
=================
*/
static void Cmd_Spawn_f(edict_t *ent) {
	solid_t backup = ent->solid;
	ent->solid = SOLID_NOT;
	gi.linkentity(ent);

	edict_t *other = G_Spawn();
	other->classname = gi.argv(1);

	other->s.origin = ent->s.origin + (AngleVectors(ent->s.angles).forward * 24.f);
	other->s.angles[1] = ent->s.angles[1];

	st = {};

	if (gi.argc() > 3) {
		for (int i = 2; i < gi.argc(); i += 2)
			ED_ParseField(gi.argv(i), gi.argv(i + 1), other);
	}

	ED_CallSpawn(other);

	if (other->inuse) {
		vec3_t forward, end;
		AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
		end = ent->s.origin;
		end[2] += ent->viewheight;
		end += (forward * 8192);

		trace_t tr = gi.traceline(ent->s.origin + vec3_t{ 0.f, 0.f, (float)ent->viewheight }, end, other, MASK_SHOT | CONTENTS_MONSTERCLIP);
		other->s.origin = tr.endpos;

		for (int32_t i = 0; i < 3; i++) {
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
				G_FreeEdict(other);
				break;
			}
		}

		if (other->inuse)
			gi.linkentity(other);

		if ((other->svflags & SVF_MONSTER) && other->think)
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
=================
*/
static void Cmd_Teleport_f(edict_t *ent) {
	if (gi.argc() < 4) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} <x> <y> <z> <pitch> <yaw> <roll>\n", gi.argv(0));
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
		ent->client->v_angle = {};
	}

	gi.linkentity(ent);
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_Notarget_f(edict_t *ent) {
	ent->flags ^= FL_NOTARGET;
	Cmd_Print_State(ent, ent->flags & FL_NOTARGET);
}

/*
==================
Cmd_Novisible_f

Sets client to "super notarget"

argv(0) notarget
==================
*/
static void Cmd_Novisible_f(edict_t *ent) {
	ent->flags ^= FL_NOVISIBLE;
	Cmd_Print_State(ent, ent->flags & FL_NOVISIBLE);
}

static void Cmd_AlertAll_f(edict_t *ent) {
	for (size_t i = 0; i < globals.num_edicts; i++) {
		edict_t *t = &g_edicts[i];

		if (!t->inuse || t->health <= 0 || !(t->svflags & SVF_MONSTER))
			continue;

		t->enemy = ent;
		FoundTarget(t);
	}
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
static void Cmd_Noclip_f(edict_t *ent) {
	ent->movetype = ent->movetype == MOVETYPE_NOCLIP ? MOVETYPE_WALK : MOVETYPE_NOCLIP;
	Cmd_Print_State(ent, ent->movetype == MOVETYPE_NOCLIP);
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
static void Cmd_Use_f(edict_t *ent) {
	item_id_t index;
	gitem_t *it;
	const char *s;

	if (ent->health <= 0 || ent->deadflag)
		return;

	s = gi.args();

	const char *cmd = gi.argv(0);
	if (!Q_strcasecmp(cmd, "use_index") || !Q_strcasecmp(cmd, "use_index_only")) {
		it = GetItemByIndex((item_id_t)atoi(s));
	} else {
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
static void Cmd_Drop_f(edict_t *ent) {
	item_id_t index;
	gitem_t *it;
	const char *s;

	if (ent->health <= 0 || ent->deadflag)
		return;

	if (!Q_strcasecmp(gi.args(), "tech")) {
		it = Tech_WhatPlayerHas(ent);

		if (it) {
			it->drop(ent, it);
			ValidateSelectedItem(ent);
		}

		return;
	}

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
	index = it->id;
	if (!ent->client->pers.inventory[index]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickup_name);
		return;
	}

	it->drop(ent, it);

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_Inven_f
=================
*/
static void Cmd_Inven_f(edict_t *ent) {
	int		   i;
	gclient_t *cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// ZOID
	if (deathmatch->integer && ent->client->menu) {
		PMenu_Close(ent);
		ent->client->update_chase = true;
		return;
	}
	// ZOID

	if (cl->showinventory) {
		cl->showinventory = false;
		return;
	}

	if (deathmatch->integer) {
		Menu_Open_Join(ent);
		return;
	}
	globals.server_flags |= SERVER_FLAG_SLOW_TIME;

	cl->showinventory = true;

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
static void Cmd_InvUse_f(edict_t *ent) {
	gitem_t *it;

	// ZOID
	if (deathmatch->integer && ent->client->menu) {
		PMenu_Select(ent);
		return;
	}
	// ZOID

	if (ent->health <= 0 || ent->deadflag)
		return;

	ValidateSelectedItem(ent);

	if (ent->client->pers.selected_item == IT_NULL) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_use");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
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
static void Cmd_WeapPrev_f(edict_t *ent) {
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t *it;
	item_id_t  selected_weapon;

	cl = ent->client;

	if (ent->health <= 0 || ent->deadflag)
		return;
	if (!cl->pers.weapon)
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
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IF_WEAPON))
			continue;
		it->use(ent, it);
		// ROGUE
		if (cl->newweapon == it)
			return; // successful
		// ROGUE
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
static void Cmd_WeapNext_f(edict_t *ent) {
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t *it;
	item_id_t  selected_weapon;

	cl = ent->client;

	if (ent->health <= 0 || ent->deadflag)
		return;
	if (!cl->pers.weapon)
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
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IF_WEAPON))
			continue;
		it->use(ent, it);
		// PMM - prevent scrolling through ALL weapons

		// ROGUE
		if (cl->newweapon == it)
			return;
		// ROGUE
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f(edict_t *ent) {
	gclient_t *cl;
	int		   index;
	gitem_t *it;

	cl = ent->client;

	if (ent->health <= 0 || ent->deadflag)
		return;
	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	// don't allow weapon chains for weaplast
	cl->no_weapon_chains = true;

	index = cl->pers.lastweapon->id;
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
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
static void Cmd_InvDrop_f(edict_t *ent) {
	gitem_t *it;

	if (ent->health <= 0 || ent->deadflag)
		return;

	ValidateSelectedItem(ent);

	if (ent->client->pers.selected_item == IT_NULL) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_drop");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
		return;
	}
	it->drop(ent, it);

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(edict_t *ent) {
	if ((level.time - ent->client->respawn_time) < 5_sec)
		return;

	ent->flags &= ~FL_GODMODE;
	ent->health = 0;

	// ROGUE
	//  make sure no trackers are still hurting us.
	if (ent->client->tracker_pain_time)
		RemoveAttackingPainDaemons(ent);

	if (ent->client->owned_sphere) {
		G_FreeEdict(ent->client->owned_sphere);
		ent->client->owned_sphere = nullptr;
	}
	// ROGUE

	// [Paril-KEX] don't allow kill to take points away in TDM
	player_die(ent, ent, ent, 100000, vec3_origin, { MOD_SUICIDE, !!teamplay->integer });
}

/*
=================
Cmd_Kill_AI_f
=================
*/
static void Cmd_Kill_AI_f(edict_t *ent) {

	// except the one we're looking at...
	edict_t *looked_at = nullptr;

	vec3_t start = ent->s.origin + vec3_t{ 0.f, 0.f, (float)ent->viewheight };
	vec3_t end = start + ent->client->v_forward * 1024.f;

	looked_at = gi.traceline(start, end, ent, MASK_SHOT).ent;

	const int numEdicts = globals.num_edicts;
	for (int edictIdx = 1; edictIdx < numEdicts; ++edictIdx) {
		edict_t *edict = &g_edicts[edictIdx];
		if (!edict->inuse || edict == looked_at) {
			continue;
		}

		if ((edict->svflags & SVF_MONSTER) == 0) {
			continue;
		}

		G_FreeEdict(edict);
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "Kill_AI: All AI Are Dead...\n");
}

/*
=================
Cmd_Where_f
=================
*/
static void Cmd_Where_f(edict_t *ent) {
	if (ent == nullptr || ent->client == nullptr) {
		return;
	}

	const vec3_t &origin = ent->s.origin;

	std::string location;
	fmt::format_to(std::back_inserter(location), FMT_STRING("{:.1f} {:.1f} {:.1f} {:.1f} {:.1f} {:.1f}\n"), origin[0], origin[1], origin[2], ent->client->ps.viewangles[0], ent->client->ps.viewangles[1], ent->client->ps.viewangles[2]);
	gi.LocClient_Print(ent, PRINT_HIGH, "Location: {}\n", location.c_str());
	gi.SendToClipBoard(location.c_str());
}

/*
=================
Cmd_Clear_AI_Enemy_f
=================
*/
static void Cmd_Clear_AI_Enemy_f(edict_t *ent) {
	const int numEdicts = globals.num_edicts;
	for (int edictIdx = 1; edictIdx < numEdicts; ++edictIdx) {
		edict_t *edict = &g_edicts[edictIdx];
		if (!edict->inuse) {
			continue;
		}

		if ((edict->svflags & SVF_MONSTER) == 0) {
			continue;
		}

		edict->monsterinfo.aiflags |= AI_FORGET_ENEMY;
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "Cmd_Clear_AI_Enemy: Clear All AI Enemies...\n");
}

/*
=================
Cmd_PutAway_f
=================
*/
static void Cmd_PutAway_f(edict_t *ent) {
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// ZOID
	if (deathmatch->integer && ent->client->menu)
		PMenu_Close(ent);
	ent->client->update_chase = true;
	// ZOID
}

int PlayerSort(const void *a, const void *b) {
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
Cmd_Players_f
=================
*/
static void Cmd_Players_f(edict_t *ent) {
	size_t	i;
	size_t	count;
	static std::string	small, large;
	int		index[MAX_CLIENTS];

	small.clear();
	large.clear();

	count = 0;
	for (i = 0; i < game.maxclients; i++)
		if (game.clients[i].pers.connected) {
			index[count] = i;
			count++;
		}

	// print information
	large[0] = 0;

	if (count) {
		for (i = 0; i < count; i++) {
			gclient_t *cl = &game.clients[index[i]];

			char value[MAX_INFO_VALUE] = { 0 };
			gi.Info_ValueForKey(cl->pers.userinfo, "name", value, sizeof(value));

			fmt::format_to(std::back_inserter(small), FMT_STRING("{:9} {:32} {:32} {:02}:{:02} {:4} {:5} {}{}\n"), index[i], cl->pers.social_id, value, (level.time - cl->resp.entertime).milliseconds() / 60000,
				((level.time - cl->resp.entertime).milliseconds() % 60000) / 1000, cl->ping,
				cl->resp.score, Teams_TeamName(cl->resp.team), cl->resp.admin ? " (admin)" : cl->resp.inactive ? " (inactive)" : "");

			if (small.length() + large.length() > MAX_IDEAL_PACKET_SIZE - 50) { // can't print all of them in one packet
				large += "...\n";
				break;
			}

			large += small;
			small.clear();
		}

		// remove the last newline
		large.pop_back();
	}

	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "\nclientnum id                               name                             time  ping score team\n");
	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "--------------------------------------------------------------------------------------------------------------\n");
	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, large.c_str());
	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "\n--------------------------------------------------------------------------------------------------------------\n");
	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "total players: {}\n", count);
	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "\n");
}

bool CheckFlood(edict_t *ent) {
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
static void Cmd_Wave_f(edict_t *ent) {
	int i;

	i = atoi(gi.argv(1));

	// no dead or noclip waving
	if (ent->deadflag || ent->movetype == MOVETYPE_NOCLIP)
		return;

	// can't wave when ducked
	bool do_animate = ent->client->anim_priority <= ANIM_WAVE && !(ent->client->ps.pmove.pm_flags & PMF_DUCKED);

	if (do_animate)
		ent->client->anim_priority = ANIM_WAVE;

	const char *other_notify_msg = nullptr, *other_notify_none_msg = nullptr;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 0, 0, 0 }, start, dir);

	// see who we're aiming at
	edict_t *aiming_at = nullptr;
	float best_dist = -9999;

	for (auto player : active_players()) {
		if (player == ent)
			continue;

		vec3_t cdir = player->s.origin - start;
		float dist = cdir.normalize();

		float dot = ent->client->v_forward.dot(cdir);

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
			ent->client->anim_end = FRAME_flip12;
		}
		break;
	case GESTURE_SALUTE:
		other_notify_msg = "$g_salute_other";
		other_notify_none_msg = "$g_salute_none";
		if (do_animate) {
			ent->s.frame = FRAME_salute01 - 1;
			ent->client->anim_end = FRAME_salute11;
		}
		break;
	case GESTURE_TAUNT:
		other_notify_msg = "$g_taunt_other";
		other_notify_none_msg = "$g_taunt_none";
		if (do_animate) {
			ent->s.frame = FRAME_taunt01 - 1;
			ent->client->anim_end = FRAME_taunt17;
		}
		break;
	case GESTURE_WAVE:
		other_notify_msg = "$g_wave_other";
		other_notify_none_msg = "$g_wave_none";
		if (do_animate) {
			ent->s.frame = FRAME_wave01 - 1;
			ent->client->anim_end = FRAME_wave11;
		}
		break;
	case GESTURE_POINT:
	default:
		other_notify_msg = "$g_point_other";
		other_notify_none_msg = "$g_point_none";
		if (do_animate) {
			ent->s.frame = FRAME_point01 - 1;
			ent->client->anim_end = FRAME_point12;
		}
		break;
	}

	bool has_a_target = false;

	if (i == GESTURE_POINT) {
		for (auto player : active_players()) {
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

		trace_t tr = gi.traceline(start, start + (ent->client->v_forward * 2048), ent, MASK_SHOT & ~CONTENTS_WINDOW);
		other_notify_msg = "$g_point_other_ping";

		uint32_t key = GetUnicastKey();

		if (tr.fraction != 1.0f) {
			// send to all teammates
			for (auto player : active_players()) {
				if (player != ent && !OnSameTeam(ent, player))
					continue;

				gi.WriteByte(svc_poi);
				gi.WriteShort(POI_PING + (ent->s.number - 1));
				gi.WriteShort(5000);
				gi.WritePosition(tr.endpos);
				gi.WriteShort(level.pic_ping);
				gi.WriteByte(208);
				gi.WriteByte(POI_FLAG_NONE);
				gi.unicast(player, false);

				gi.local_sound(player, CHAN_AUTO, gi.soundindex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
				gi.LocClient_Print(player, PRINT_HIGH, other_notify_msg, ent->client->pers.netname);
			}
		}
	} else {
		if (CheckFlood(ent))
			return;

		edict_t *targ = nullptr;
		while ((targ = findradius(targ, ent->s.origin, 1024)) != nullptr) {
			if (ent == targ) continue;
			if (!targ->client) continue;
			if (!gi.inPVS(ent->s.origin, targ->s.origin, false)) continue;

			if (aiming_at && other_notify_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_msg, ent->client->pers.netname, aiming_at->client->pers.netname);
			else if (other_notify_none_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_none_msg, ent->client->pers.netname);
		}

		if (aiming_at && other_notify_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_msg, ent->client->pers.netname, aiming_at->client->pers.netname);
		else if (other_notify_none_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_none_msg, ent->client->pers.netname);
	}

	ent->client->anim_time = 0_ms;
}

#ifndef KEX_Q2_GAME
/*
==================
Cmd_Say_f

NB: only used for non-Playfab stuff
==================
*/
static void Cmd_Say_f(edict_t *ent, bool arg0) {
	edict_t *other;
	const char *p_in;
	static std::string text;

	if (gi.argc() < 2 && !arg0)
		return;
	else if (CheckFlood(ent))
		return;

	text.clear();
	fmt::format_to(std::back_inserter(text), FMT_STRING("{}: "), ent->client->pers.netname);

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

	if (sv_dedicated->integer)
		gi.Client_Print(nullptr, PRINT_CHAT, text.c_str());

	for (uint32_t j = 1; j <= game.maxclients; j++) {
		other = &g_edicts[j];
		if (!other->inuse)
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
static void Cmd_Say_Team_f(edict_t *who, const char *msg_in) {
	edict_t *cl_ent;
	char outmsg[256];

	if (CheckFlood(who))
		return;

	Q_strlcpy(outmsg, msg_in, sizeof(outmsg));

	char *msg = outmsg;

	if (*msg == '\"') {
		msg[strlen(msg) - 1] = 0;
		msg++;
	}

	for (uint32_t i = 0; i < game.maxclients; i++) {
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (cl_ent->client->resp.team == who->client->resp.team)
			gi.LocClient_Print(cl_ent, PRINT_CHAT, "({}): {}\n",
				who->client->pers.netname, msg);
	}
}
#endif

/*
=================
Cmd_Switchteam_f
=================
*/
void Cmd_Observer_f(edict_t *ent);
static void Cmd_Switchteam_f(edict_t *ent) {
	if (!IsTeamplay())
		return;

	// [Paril-KEX] in force-join, just do a regular team join.
	if (g_dm_force_join->integer) {
		// check if we should even switch teams
		edict_t *player;
		uint32_t team_red_count = 0, team_blue_count = 0;
		team_t best_team;

		for (uint32_t i = 1; i <= game.maxclients; i++) {
			player = &g_edicts[i];

			// NB: we are counting ourselves in this one, unlike
			// the other assign team func
			if (!player->inuse)
				continue;

			switch (player->client->resp.team) {
			case TEAM_RED:
				team_red_count++;
				break;
			case TEAM_BLUE:
				team_blue_count++;
				break;
			default:
				break;
			}
		}

		if (team_red_count < team_blue_count)
			best_team = TEAM_RED;
		else
			best_team = TEAM_BLUE;

		if (ent->client->resp.team != best_team) {
			////
			ent->svflags = SVF_NONE;
			ent->flags &= ~FL_GODMODE;
			ent->client->resp.team = best_team;
			ent->client->resp.ctf_state = 0;
			char value[MAX_INFO_VALUE] = { 0 };
			gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", value, sizeof(value));
			G_AssignPlayerSkin(ent, value);

			// if anybody has a menu open, update it immediately
			Menu_Dirty();

			if (ent->solid == SOLID_NOT) {
				// spectator
				ClientSpawn(ent);

				G_PostRespawn(ent);

				gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_team",
					ent->client->pers.netname, Teams_TeamName(best_team));
				return;
			}

			ent->health = 0;
			player_die(ent, ent, ent, 100000, vec3_origin, { MOD_CHANGE_TEAM, true });

			// don't even bother waiting for death frames
			ent->deadflag = true;
			respawn(ent);

			G_SetPlayerScore(ent->client, 0);

			gi.LocBroadcast_Print(PRINT_HIGH, "$g_changed_team",
				ent->client->pers.netname, Teams_TeamName(best_team));
		}

		return;
	}

	if (!ClientIsSpectating(ent->client))
		Cmd_Observer_f(ent);
}

static void Cmd_ListMonsters_f(edict_t *ent) {
	if (!g_debug_monster_kills->integer)
		return;

	for (size_t i = 0; i < level.total_monsters; i++) {
		edict_t *e = level.monsters_registered[i];

		if (!e || !e->inuse)
			continue;
		else if (!(e->svflags & SVF_MONSTER) || (e->monsterinfo.aiflags & AI_DO_NOT_COUNT))
			continue;
		else if (e->deadflag)
			continue;

		gi.Com_PrintFmt("{}\n", *e);
	}
}

// =========================================
// TEAMPLAY - MOSTLY PORTED FROM QUAKE III
// =========================================

/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount(int ignoreClientNum, team_t team) {
	int		count = 0;

	for (size_t i = 0; i < game.maxclients; i++) {
		if (i == ignoreClientNum) {
			continue;
		}
		if (!game.clients[i].pers.connected) {
			continue;
		}
		if (game.clients[i].resp.team == team) {
			count++;
		}
	}

	return count;
}


/*
================
PickTeam
================
*/
team_t PickTeam(int ignoreClientNum) {
	int		counts[TEAM_NUM_TEAMS];

	if (!IsTeamplay())
		return TEAM_FREE;

	counts[TEAM_BLUE] = TeamCount(ignoreClientNum, TEAM_BLUE);
	counts[TEAM_RED] = TeamCount(ignoreClientNum, TEAM_RED);

	if (counts[TEAM_BLUE] > counts[TEAM_RED]) {
		return TEAM_RED;
	}
	if (counts[TEAM_RED] > counts[TEAM_BLUE]) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED]) {
		return TEAM_RED;
	} else if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE]) {
		return TEAM_BLUE;
	}
	return brandom() ? TEAM_RED : TEAM_BLUE;
}

/*
=================
BroadcastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange(edict_t *ent, int old_team, bool inactive) {
	const char *s = nullptr, *t = nullptr;
	char name[MAX_INFO_VALUE] = { 0 };
	int32_t client_num;

	if (!deathmatch->integer)
		return;

	if (!ent->client)
		return;

	if (ent->client->resp.team == old_team)
		return;

	client_num = ent - g_edicts - 1;
	gi.Info_ValueForKey(ent->client->pers.userinfo, "name", name, sizeof(name));

	switch (ent->client->resp.team) {
	case TEAM_FREE:
		s = G_Fmt("{} joined the battle.\n", name).data();
		//t = "%bind:inven:Toggles Menu%You have joined the game.";
		t = "You have joined the game.";
		break;
	case TEAM_SPECTATOR:
		if (inactive) {
			s = G_Fmt("{} is inactive,\nmoved to spectators.\n", name).data();
			t = "You are inactive and have been\nmoved to spectators.";
		} else {
			s = G_Fmt("{} joined the spectators.\n", name).data();
			t = "You are now spectating.";
		}
		break;
	case TEAM_RED:
	case TEAM_BLUE:
		s = G_Fmt("{} joined the {} Team.\n", name, Teams_TeamName(ent->client->resp.team)).data();
		t = G_Fmt("You have joined the {} Team.\n", Teams_TeamName(ent->client->resp.team)).data();
		break;
	}

	if (s) {
		for (size_t i = 0; i < game.maxclients; i++) {
			if (&g_edicts[i] == ent)
				continue;
			if (!g_edicts[i].client)
				continue;
			if (!g_edicts[i].client->pers.connected)
				continue;
			if (g_edicts[i].svflags & SVF_BOT)
				continue;
			gi.LocClient_Print(&g_edicts[i], PRINT_CENTER, s);
		}
	}

	if (t) {
		gi.LocClient_Print(ent, PRINT_CENTER, G_Fmt("%bind:inven:Toggles Menu%{}", t).data() );
	}
}


/*
=================
AllowTeamSwitch
=================
*/
bool AllowTeamSwitch(int client_num, int new_team) {
	if (g_teamplay_force_balance->integer) {
		int		counts[TEAM_NUM_TEAMS];

		counts[TEAM_BLUE] = TeamCount(client_num, TEAM_BLUE);
		counts[TEAM_RED] = TeamCount(client_num, TEAM_RED);

		// We allow a spread of two
		if (g_teamplay_force_balance->integer && ((new_team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1) ||
			(new_team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1))) {
			gi.LocClient_Print(&g_edicts[client_num], PRINT_HIGH, "{} has too many players.\n", Teams_TeamName(new_team));
			return false; // ignore the request
		}

		// It's ok, the team we are switching to has less or same number of players
	}

	return true;
}

bool AllowClientTeamSwitch(edict_t *ent) {
	if (!deathmatch->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You may not switch teams during campaigns.\n");
		return false;
	}

	if (g_dm_force_join->integer || !g_teamplay_allow_team_pick->integer) {
		if (!(ent->svflags & SVF_BOT)) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Team choosing is disabled.");
			return false;
		}
	}
	
	if (ent->client->resp.switch_team_time > level.time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
		return false;
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
void StopFollowing(edict_t *ent, bool release) {
	gclient_t *client;

	if (ent->svflags & SVF_BOT || !ent->inuse)
		return;

	client = ent->client;

	client->resp.team = TEAM_SPECTATOR;
	client->resp.spectator_state = SPECTATOR_FREE;
	if (release) {
		client->ps.stats[STAT_HEALTH] = ent->health = 1;
	}
	//SetClientViewAngle(ent, client->ps.viewangles);

	//client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->svflags &= SVF_BOT;

	//client->ps.clientnum = ent - g_edicts;


	//-------------

	ent->client->ps.kick_angles = {};
	ent->client->ps.gunangles = {};
	ent->client->ps.gunoffset = {};
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunskin = 0;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunrate = 0;
	ent->client->ps.screen_blend = {};
	ent->client->ps.damage_blend = {};
	ent->client->ps.rdflags = RDF_NONE;
}

/*
=================
SetTeam
=================
*/
bool SetTeam(edict_t *ent, const char *s, bool inactive) {
	team_t				new_team, old_team;
	gclient_t *client;
	int					client_num;
	spectator_state_t	spec_state;
	int					spec_client;

	//
	// see what change is requested
	//

	client_num = ent - g_edicts;
	client = game.clients + client_num;

#if 0
	// early team override
	if (!client->pers.connected && IsTeamplay()) {
		if (!Q_strcasecmp(s, "red") || !Q_strcasecmp(s, "r")) {
			new_team = TEAM_RED;
		} else if (!Q_strcasecmp(s, "blue") || !Q_strcasecmp(s, "b")) {
			new_team = TEAM_BLUE;
		} else {
			new_team = TEAM_FREE;
		}
		if (new_team != TEAM_FREE && AllowTeamSwitch(client_num, new_team)) {
			client->resp.team = new_team;
			client->pers.team_state.state = TEAM_BEGIN;
			//G_WriteClientSessionData(client);
			// count current clients and rank for scoreboard
			CalculateRanks();
		}

		return true; // bypass flood protection
	}
#endif

	if (!Q_strcasecmp(s, "follow1")) {
		new_team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FOLLOW;
		spec_client = -1;
	} else if (!Q_strcasecmp(s, "follow2")) {
		new_team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FOLLOW;
		spec_client = -2;
	} else if (!Q_strcasecmp(s, "spectator") || !Q_strcasecmp(s, "s")) {
		new_team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FREE;
	} else if (IsTeamplay()) {
		// if running a team game, assign player to one of the teams
		spec_state = SPECTATOR_NOT;
		if (!Q_strcasecmp(s, "red") || !Q_strcasecmp(s, "r")) {
			new_team = TEAM_RED;
			spec_state = SPECTATOR_NOT;
		} else if (!Q_strcasecmp(s, "blue") || !Q_strcasecmp(s, "b") || s[0] == 'b') {
			new_team = TEAM_BLUE;
			spec_state = SPECTATOR_NOT;
		} else if (!Q_strcasecmp(s, "auto") || !Q_strcasecmp(s, "a")) {
			// pick the team with the least number of players
			new_team = PickTeam(client_num);
			spec_state = SPECTATOR_NOT;
		} else {
			return false;
		}

		if (!AllowTeamSwitch(client_num, new_team)) {
			return false;
		}

	} else {
		if (!Q_strcasecmp(s, "free") || !Q_strcasecmp(s, "f") ||
			!Q_strcasecmp(s, "auto") || !Q_strcasecmp(s, "a")) {
			new_team = TEAM_FREE;
			spec_state = SPECTATOR_NOT;
		} else {
			return false;
		}
	}

	spec_client = client_num;

	// override decision if limiting the players
	if (duel->integer && level.num_nonspectator_clients >= 2) {
		//todo: if team free, add to queue
		new_team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FREE;
	} else if (!deathmatch->integer && level.num_nonspectator_clients >= 4) {	// this should never happen
		new_team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FREE;
	} else if (maxplayers->integer > 0 && level.num_nonspectator_clients >= maxplayers->integer) {
		new_team = TEAM_SPECTATOR;
		spec_state = SPECTATOR_FREE;
	}

	//
	// decide if we will allow the change
	//
	old_team = client->resp.team;
	if (new_team == old_team) {
		if (old_team != TEAM_SPECTATOR)
			return false;

		// do soft release if possible
		if (client->ps.stats[STAT_CHASE]) {
			StopFollowing(ent, true);
			return false;
		}

		// second spectator team request will move player to intermission point
		if (old_team == TEAM_SPECTATOR && !client->ps.stats[STAT_CHASE]) {	//pers.team??
			ent->s.origin = level.intermission_origin;
			ent->client->ps.pmove.origin = level.intermission_origin;
			ent->client->ps.viewangles = level.intermission_angle;
			return false;
		}
	} else {
		if (!AllowClientTeamSwitch(ent))
			return false;
	}

	//
	// execute the team change
	//
	PMenu_Close(ent);

	// if the player was dead leave the body
	if (ent->health <= 0) {
		CopyToBodyQue(ent);
	}

	// if they are playing a duel, count as a loss
	if (duel->integer && old_team == TEAM_FREE) {
		ent->client->resp.losses++;
	}

	// he starts at 'base'
	client->pers.team_state.state = TEAM_BEGIN;
	ent->client->resp.inactive = false;

	if (old_team != TEAM_SPECTATOR) {
#if 0
		// revert any casted votes
		if (old_team != new_team)
			G_RevertVote(ent->client);
#endif
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die(ent, ent, ent, 100000, vec3_origin, MOD_CHANGE_TEAM);
	}

	// they go to the end of the line for duels
	if (duel->integer && new_team == TEAM_SPECTATOR) {
		client->resp.spectator_time = 0;
		//todo: add to queue
	}

	BroadcastTeamChange(ent, old_team, inactive);

	client->resp.team = new_team;
	client->resp.spectator_state = spec_state;
	client->resp.spectator_client = spec_client;

	//G_WriteClientSessionData(client);

	// get and distribute relevent parameters
	ClientUserinfoChanged(ent, ent->client->pers.userinfo);

	ent->client->resp.switch_team_time = level.time + 5_sec;

	//q2
	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.ctf_state = 0;

	if (IsTeamplay() && client->resp.team != TEAM_SPECTATOR) {
		char value[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", value, sizeof(value));
		G_AssignPlayerSkin(ent, value);
	}

	// assign a ghost if we are in match mode
	if (level.match == MATCH_GAME) {
		if (ent->client->resp.ghost)
			ent->client->resp.ghost->code = 0;
		ent->client->resp.ghost = nullptr;
		P_Match_AssignGhost(ent);
	}

	ClientSpawn(ent);

	G_PostRespawn(ent);

	if (level.match == MATCH_SETUP) {
		gi.LocCenter_Print(ent, "Type \"ready\" in console to ready up.\n");
	}

	// if anybody has a menu open, update it immediately
	Menu_Dirty();
	//-q2

	//ClientBegin(ent);

	return true;
}

/*
=================
Cmd_Team_f
=================
*/
static void Cmd_Team_f(edict_t *ent) {

	if (gi.argc() != 2) {
		switch (ent->client->resp.team) {
		case TEAM_BLUE:
		case TEAM_RED:
		case TEAM_FREE:
		case TEAM_SPECTATOR:
			gi.LocClient_Print(ent, PRINT_HIGH, "Your team: {}.\n", Teams_TeamName(ent->client->resp.team));
			break;
		default:
			break;
		}
		return;
	}

	SetTeam(ent, gi.argv(1), false);
}


void Team_Join(edict_t *ent, team_t desired_team, bool inactive) {
	team_t old_team = ent->client->resp.team;

	if (desired_team != TEAM_SPECTATOR && desired_team == ent->client->resp.team) {
		PMenu_Close(ent);
		return;
	}

	if (!AllowTeamSwitch(ent - g_edicts, desired_team))
		return;

	if (!inactive && ent->client->resp.switch_team_time > level.time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
		return;
	}

	// allow the change...

	PMenu_Close(ent);

	// start as spectator
	if (ent->movetype == MOVETYPE_NOCLIP)
		Weapon_Grapple_DoReset(ent->client);

	GT_CTF_DeadDropFlag(ent);
	Tech_DeadDrop(ent);

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.team = desired_team;
	ent->client->resp.ctf_state = 0;
	ent->client->resp.inactive = false;
	ent->client->resp.inactivity_time = 0_ms;
	ent->client->resp.switch_team_time = level.time + 5_sec;
	ent->client->resp.spectator_state = desired_team == TEAM_SPECTATOR ? SPECTATOR_FREE : SPECTATOR_NOT;
	ent->client->resp.spectator_client = 0;
	ent->client->resp.spectator_time = 0;

	if (IsTeamplay() && desired_team != TEAM_SPECTATOR) {
		char value[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", value, sizeof(value));
		G_AssignPlayerSkin(ent, value);
	}

	// assign a ghost if we are in match mode
	if (level.match == MATCH_GAME) {
		if (ent->client->resp.ghost)
			ent->client->resp.ghost->code = 0;
		ent->client->resp.ghost = nullptr;
		P_Match_AssignGhost(ent);
	}

	ClientSpawn(ent);

	G_PostRespawn(ent);

	BroadcastTeamChange(ent, old_team, inactive);
#if 0
	if (desired_team != TEAM_SPECTATOR) {
		if (desired_team == TEAM_FREE) {
			gi.LocBroadcast_Print(PRINT_CENTER, "{} joined the battle.\n",
				ent->client->pers.netname);
		} else {
			gi.LocBroadcast_Print(PRINT_CENTER, "$g_joined_team",
				ent->client->pers.netname, Teams_TeamName(desired_team));
		}
	} else {
		if (desired_team != ent->client->resp.team)
			gi.LocBroadcast_Print(PRINT_CENTER, inactive ? "{} is inactive, moved to spectators." : "$g_observing", ent->client->pers.netname);
	}
#endif
	if (level.match == MATCH_SETUP) {
		gi.LocCenter_Print(ent, "Type \"ready\" in console to ready up.\n");
	}

	// if anybody has a menu open, update it immediately
	Menu_Dirty();
}

/*
=================
Cmd_CrosshairID_f
=================
*/
static void Cmd_CrosshairID_f(edict_t *ent) {
	if (ent->client->resp.id_state) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Disabling player identication display.\n");
		ent->client->resp.id_state = false;
	} else {
		gi.LocClient_Print(ent, PRINT_HIGH, "Activating player identication display.\n");
		ent->client->resp.id_state = true;
	}
}


/*
=================
Cmd_Ghost_f
=================
*/
static void Cmd_Ghost_f(edict_t *ent) {
	int i;
	int n;

	if (gi.argc() < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} <code>\n", gi.argv(0));
		return;
	}

	if (!ClientIsSpectating(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You are already in the game.\n");
		return;
	}
	if (level.match != MATCH_GAME) {
		gi.LocClient_Print(ent, PRINT_HIGH, "No match is in progress.\n");
		return;
	}

	n = atoi(gi.argv(1));

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (level.ghosts[i].code && level.ghosts[i].code == n) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Ghost code accepted, your position has been reinstated.\n");
			level.ghosts[i].ent->client->resp.ghost = nullptr;
			ent->client->resp.team = level.ghosts[i].team;
			ent->client->resp.ghost = level.ghosts + i;
			ent->client->resp.score = level.ghosts[i].score;
			ent->client->resp.ctf_state = 0;
			level.ghosts[i].ent = ent;
			ent->svflags = SVF_NONE;
			ent->flags &= ~FL_GODMODE;
			ClientSpawn(ent);
			gi.LocBroadcast_Print(PRINT_HIGH, "{} has been reinstated to {} team.\n",
				ent->client->pers.netname, Teams_TeamName(ent->client->resp.team));
			return;
		}
	}
	gi.LocClient_Print(ent, PRINT_HIGH, "Invalid ghost code.\n");
}


static void Cmd_Stats_f(edict_t *ent) {
	if (!IsTeamplay())
		return;

	ghost_t *g;
	static std::string text;
	edict_t *e2;

	text.clear();

	if (level.match == MATCH_SETUP) {
		for (uint32_t i = 1; i <= game.maxclients; i++) {
			e2 = g_edicts + i;
			if (!e2->inuse)
				continue;
			if (!e2->client->resp.ready && !ClientIsSpectating(e2->client)) {
				std::string_view str = G_Fmt("{} is not ready.\n", e2->client->pers.netname);

				if (text.length() + str.length() < MAX_STRING_CHARS - 50)
					text += str;
			}
		}
	}

	uint32_t i;
	for (i = 0, g = level.ghosts; i < MAX_CLIENTS; i++, g++)
		if (g->ent)
			break;

	if (i == MAX_CLIENTS) {
		if (!text.length())
			text = "No statistics available.\n";

		gi.Client_Print(ent, PRINT_HIGH, text.c_str());
		return;
	}

	text += "  #|Name            |Score|Kills|Death|BasDf|CarDf|Effcy|\n";

	for (i = 0, g = level.ghosts; i < MAX_CLIENTS; i++, g++) {
		if (!*g->netname)
			continue;

		int32_t e;

		if (g->deaths + g->kills == 0)
			e = 50;
		else
			e = g->kills * 100 / (g->kills + g->deaths);
		std::string_view str = G_Fmt("{:3}|{:<16.16}|{:5}|{:5}|{:5}|{:5}|{:5}|{:4}%|\n",
			g->number,
			g->netname,
			g->score,
			g->kills,
			g->deaths,
			g->basedef,
			g->carrierdef,
			e);

		if (text.length() + str.length() > MAX_STRING_CHARS - 50) {
			text += "And more...\n";
			break;
		}

		text += str;
	}

	gi.Client_Print(ent, PRINT_HIGH, text.c_str());
}


void Cmd_Observer_f(edict_t *ent) {
	if (g_dm_force_join->integer) return;

	// start as spectator
	if (ent->movetype == MOVETYPE_NOCLIP)
		Weapon_Grapple_DoReset(ent->client);

	GT_CTF_DeadDropFlag(ent);
	Tech_DeadDrop(ent);

	ent->deadflag = false;
	ent->movetype = MOVETYPE_NOCLIP;
	ent->solid = SOLID_NOT;
	ent->svflags |= SVF_NOCLIENT;
	ent->client->resp.team = TEAM_SPECTATOR;
	ent->client->resp.spectator_state = SPECTATOR_FREE;
	ent->client->resp.spectator_client = 0;
	ent->client->resp.spectator_time = 0;
	ent->client->resp.inactive = false;
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunskin = 0;
	G_SetPlayerScore(ent->client, 0);
	ClientSpawn(ent);
}

static void Cmd_Boot_f(edict_t *ent) {
	edict_t *targ;

	if (!ent->client->resp.admin) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}

	if (gi.argc() < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Who do you want to kick?\n");
		return;
	}

	if (*gi.argv(1) < '0' && *gi.argv(1) > '9') {
		gi.LocClient_Print(ent, PRINT_HIGH, "Specify the player number to kick.\n");
		return;
	}

	uint32_t i = strtoul(gi.argv(1), nullptr, 10);
	if (i < 1 || i > game.maxclients) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid player number.\n");
		return;
	}

	targ = g_edicts + i;
	if (!targ->inuse) {
		gi.LocClient_Print(ent, PRINT_HIGH, "That player number is not connected.\n");
		return;
	}

	gi.AddCommandString(G_Fmt("kick {}\n", i - 1).data());
}

/*-----------------------------------------------------------------------*/

/* VOTING */

void Voting_Warp_f(edict_t *ent) {
	char *token;

	if (gi.argc() < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Where do you want to warp to?\n");
		gi.LocClient_Print(ent, PRINT_HIGH, "Available levels are: {}\n", g_map_list->string);
		return;
	}

	const char *mlist = g_map_list->string;

	while (*(token = COM_Parse(&mlist))) {
		if (Q_strcasecmp(token, gi.argv(1)) == 0)
			break;
	}

	if (!*token) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Unknown level.\n");
		gi.LocClient_Print(ent, PRINT_HIGH, "Available levels are: {}\n", g_map_list->string);
		return;
	}

	if (ent->client->resp.admin) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} is warping to level {}.\n",
			ent->client->pers.netname, gi.argv(1));
		Q_strlcpy(level.forcemap, gi.argv(1), sizeof(level.forcemap));
		EndDMLevel();
		return;
	}

	if (Voting_Begin(ent, VOTING_MAP, G_Fmt("{} has requested warping to level {}.\n",
		ent->client->pers.netname, gi.argv(1)).data()))
		Q_strlcpy(level.voting_map, gi.argv(1), sizeof(level.voting_map));
}

void Voting_Passed() {
	gi.LocBroadcast_Print(PRINT_HIGH, "Vote passed.\n");

	switch (level.voting_type) {
	case VOTING_MATCH:
		// reset into match mode
		if (competition->integer < 3)
			gi.cvar_set("competition", "2");
		level.match = MATCH_SETUP;
		Match_ResetAllPlayers();
		break;

	case VOTING_ADMIN:
		level.voting_target->client->resp.admin = true;
		gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", level.voting_target->client->pers.netname);
		gi.LocClient_Print(level.voting_target, PRINT_HIGH, "Type 'admin' to access the adminstration menu.\n");
		break;

	case VOTING_MAP:
		gi.LocBroadcast_Print(PRINT_HIGH, "{} is warping to level {}.\n",
			level.voting_target->client->pers.netname, level.voting_map);
		Q_strlcpy(level.forcemap, level.voting_map, sizeof(level.forcemap));
		EndDMLevel();
		break;

	default:
		break;
	}
	level.voting_type = VOTING_NONE;
}

void Cmd_CallVote_f(edict_t *ent) {
	const char *arg = NULL;

	if (gi.argc() < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} [command] [...]\n", gi.argv(0));
	}

	arg = gi.argv(1);


}

void Cmd_Vote_f(edict_t *ent) {
	const char *arg = NULL;

	if (level.voting_type == VOTING_NONE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "No vote in progress.\n");
		return;
	}
	if (gi.argc() < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} [yes/no]\n", gi.argv(0));
	}
	if (ent->client->resp.voted) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Vote already cast.\n");
		return;
	}
	if (ClientIsSpectating(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Not allowed to vote as spectator.\n");
		return;
	}
	if (level.voting_target == ent) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You can't vote for yourself.\n");
		return;
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "Vote cast.\n");
	ent->client->resp.voted = true;

	arg = gi.argv(1);

	if (arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1') {
		level.voting_count_yes++;
		if (level.voting_count_yes == level.voting_votes_needed) {
			// the election has been won
			Voting_Passed();
			return;
		}
	} else {
		level.voting_count_no++;	//VOTE(%i):%s yes:%i no:%i
		gi.LocBroadcast_Print(PRINT_HIGH, "{}\n", level.voting_name);
		gi.LocBroadcast_Print(PRINT_CHAT, "VOTE({}): yes:{} no:{} needed:{}\n", (level.voting_time - level.time).seconds<int>(), level.voting_count_yes, level.voting_count_no, level.voting_votes_needed);
	}
}

void Match_Start();
void Match_End();
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
		t = (level.matchtime - level.time).seconds<int>();

		// no team warnings in match mode
		level.warnactive = 0;

		if (t <= 0) { // time ended on something
			switch (level.match) {
			case MATCH_SETUP:
				// go back to normal mode
				if (competition->integer < 3) {
					level.match = MATCH_NONE;
					gi.cvar_set("competition", "1");
					Match_ResetAllPlayers();
				} else {
					// reset the time
					level.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
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
				if (!ClientIsSpectating(ent->client) &&
					!ent->client->resp.ready)
					j++;
			}

			if (competition->integer < 3)
				G_FmtTo(text, "{:02}:{:02} SETUP: {} not ready", t / 60, t % 60, j);
			else
				G_FmtTo(text, "SETUP: {} not ready", j);

			gi.configstring(CONFIG_CTF_MATCH, text);
			break;

		case MATCH_PREGAME:
			G_FmtTo(text, "{:02}:{:02} UNTIL START", t / 60, t % 60);
			gi.configstring(CONFIG_CTF_MATCH, text);

			if (t <= 10 && !level.countdown) {
				level.countdown = true;
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
			break;

		case MATCH_GAME:
			G_FmtTo(text, "{:02}:{:02} MATCH", t / 60, t % 60);
			gi.configstring(CONFIG_CTF_MATCH, text);
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

	if (capturelimit->integer &&
		(level.team_scores[TEAM_RED] >= capturelimit->integer ||
			level.team_scores[TEAM_BLUE] >= capturelimit->integer)) {
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_capturelimit_hit");
		return true;
	}
	return false;
}

bool Voting_Begin(edict_t *ent, voting_t type, const char *msg) {
	int		 count;
	edict_t *e;

	if (g_voting_percentage->value <= 0) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Voting is disabled, only an admin can process this action.\n");
		return false;
	}

	if (level.voting_type != VOTING_NONE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Voting is already in progress.\n");
		return false;
	}

	// clear votes
	count = 0;
	for (uint32_t i = 1; i <= game.maxclients; i++) {
		e = g_edicts + i;
		e->client->resp.voted = false;
		if (e->inuse && !ClientIsSpectating(e->client))
			count++;
	}

	if (count < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Not enough players to vote.\n");
		return false;
	}

	level.voting_target = ent;
	level.voting_type = type;
	level.voting_count_yes = 0;
	level.voting_votes_needed = (int)((count * g_voting_percentage->value) / 100);
	level.voting_time = level.time + 20_sec; // twenty seconds for election
	Q_strlcpy(level.voting_name, msg, sizeof(level.voting_name));

	// tell everyone
	gi.Broadcast_Print(PRINT_CHAT, level.voting_name);
	gi.LocBroadcast_Print(PRINT_HIGH, "Type VOTE YES or VOTE NO to vote on this request.\n");
	gi.LocBroadcast_Print(PRINT_HIGH, "VOTE({}): yes:{} no:{} needed:{}\n", (level.voting_time - level.time).seconds<int>(), level.voting_count_yes, level.voting_count_no, level.voting_votes_needed);

	return true;
}

/*----------------------------------------------------------------*/

static void Cmd_Admin_f(edict_t *ent) {
	if (!allow_admin->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Administration is disabled\n");
		return;
	}

	if (gi.argc() > 1 && admin_password->string && *admin_password->string &&
		!ent->client->resp.admin && strcmp(admin_password->string, gi.argv(1)) == 0) {
		ent->client->resp.admin = true;
		gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", ent->client->pers.netname);
		gi.LocClient_Print(ent, PRINT_HIGH, "Type 'admin' to access the adminstration menu.\n");
	}

	if (!ent->client->resp.admin) {
		Voting_Begin(ent, VOTING_ADMIN, G_Fmt("{} has requested admin rights.\n",
			ent->client->pers.netname).data());
		return;
	}

	if (ent->client->menu)
		PMenu_Close(ent);

	//Menu_Admin(ent);
}

/*----------------------------------------------------------------*/

static void Cmd_Ready_f(edict_t *ent) {
	uint32_t i;
	edict_t *e;
	uint32_t count_total, count_ready;

	if (ClientIsSpectating(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Pick a team first (hit <TAB> for menu)\n");
		return;
	}

	if (level.match != MATCH_SETUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "A match is not being setup.\n");
		return;
	}

	if (ent->client->resp.ready) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You have already committed.\n");
		return;
	}

	ent->client->resp.ready = true;

	count_total = count_ready = 0;
	for (count_ready = 0, i = 1; i <= game.maxclients; i++) {
		e = g_edicts + i;
		if (!e->inuse)
			continue;

		if (!ClientIsSpectating(e->client)) {
			count_total++;

			if (e->client->resp.ready || e->svflags & SVF_BOT)
				count_ready++;
		}
	}

	if ((float)count_ready / (float)count_total >= g_warmup_ready_percentage->value) {
		// enough players have readied
		gi.LocBroadcast_Print(PRINT_CENTER, "Match starting...\n");
		level.match = MATCH_PREGAME;
		level.matchtime = level.time + gtime_t::from_sec(g_warmup_countdown->value);
		level.countdown = false;
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/talk1.wav"), 1, ATTN_NONE, 0);
	} else {
		gi.LocBroadcast_Print(PRINT_CENTER, "{} is ready.\n", ent->client->pers.netname);
	}
}

static void Cmd_NotReady_f(edict_t *ent) {
	if (ClientIsSpectating(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Pick a team first (hit <TAB> for menu)\n");
		return;
	}

	if (level.match != MATCH_SETUP && level.match != MATCH_PREGAME) {
		gi.LocClient_Print(ent, PRINT_HIGH, "A match is not being setup.\n");
		return;
	}

	if (!ent->client->resp.ready) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You haven't committed.\n");
		return;
	}

	ent->client->resp.ready = false;

	if (level.match == MATCH_PREGAME) {
		gi.LocBroadcast_Print(PRINT_CENTER, "Match halted.\n");
		level.match = MATCH_SETUP;
		level.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
	} else {
		gi.LocBroadcast_Print(PRINT_CENTER, "{} is not ready.\n", ent->client->pers.netname);
	}
}

static void Cmd_Hook_f(edict_t *ent) {
	if (!ent->client)
		return;

	if (!g_allow_grapple->integer || !g_grapple_offhand->integer)
		return;

	// add hook time (1-2 frames), hook unfired once level.time exceeds this
	//ent->client->hook_cmdtime = level.time + 500_ms;
	Weapon_Hook(ent);
}

static void Cmd_UnHook_f(edict_t *ent) {
	if (!ent->client)
		return;
	Weapon_Grapple_DoReset(ent->client);
}

// =========================================

enum cmd_flags_t : uint32_t {
	CF_NONE = 0,
	CF_ALLOW_DEAD = bit_v<0>,
	CF_ALLOW_INT = bit_v<1>,
	CF_ALLOW_SPEC = bit_v<2>,
	CF_MATCH_ONLY = bit_v<3>,
	CF_ADMIN_ONLY = bit_v<4>,
	CF_CHEAT_PROTECT = bit_v<5>,
};

struct client_commands_t {
	const		char *name;
	void		(*func)(edict_t *ent);
	uint32_t	flags;
};

client_commands_t cmds[] = {
	{"-hk",				Cmd_UnHook_f,			CF_NONE},
	{"+hk",				Cmd_Hook_f,				CF_NONE},
	//{"callvote",		Cmd_CallVote_f,			CF_ALLOW_DEAD|CF_ALLOW_SPEC},
	{"admin",			Cmd_Admin_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"alertall",		Cmd_AlertAll_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"boot",			Cmd_Boot_f,				CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"checkpoi",		Cmd_CheckPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"clear_ai_enemy",	Cmd_Clear_AI_Enemy_f,	CF_CHEAT_PROTECT},
	{"drop",			Cmd_Drop_f,				CF_NONE},
	{"drop_index",		Cmd_Drop_f,				CF_NONE},
	{"ghost",			Cmd_Ghost_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"give",			Cmd_Give_f,				CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"god",				Cmd_God_f,				CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"help",			Cmd_Help_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"hook",			Cmd_Hook_f,				CF_NONE},
	{"id",				Cmd_CrosshairID_f,		CF_ALLOW_SPEC},
	{"immortal",		Cmd_Immortal_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"invdrop",			Cmd_InvDrop_f,			CF_NONE},
	{"inven",			Cmd_Inven_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"invnext",			Cmd_InvNext_f,			CF_ALLOW_SPEC},	//spec for menu up/down
	{"invnextp",		Cmd_InvNextP_f,			CF_NONE},
	{"invnextw",		Cmd_InvNextW_f,			CF_NONE},
	{"invprev",			Cmd_InvPrev_f,			CF_ALLOW_SPEC},	//spec for menu up/down
	{"invprevp",		Cmd_InvPrevP_f,			CF_NONE},
	{"invprevw",		Cmd_InvPrevW_f,			CF_NONE},
	{"invuse",			Cmd_InvUse_f,			CF_ALLOW_SPEC},	//spec for menu up/down
	{"kill",			Cmd_Kill_f,				CF_NONE},
	{"kill_ai",			Cmd_Kill_AI_f,			CF_CHEAT_PROTECT},
	{"listmonsters",	Cmd_ListMonsters_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"noclip",			Cmd_Noclip_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"notarget",		Cmd_Notarget_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"notready",		Cmd_NotReady_f,			CF_ALLOW_DEAD},
	{"novisible",		Cmd_Novisible_f,		CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"observer",		Cmd_Observer_f,			CF_ALLOW_SPEC},
	{"players",			Cmd_Players_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"putaway",			Cmd_PutAway_f,			CF_ALLOW_SPEC},	//spec for menu close
	{"ready",			Cmd_Ready_f,			CF_ALLOW_DEAD},
	{"score",			Cmd_Score_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"setpoi",			Cmd_SetPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"spawn",			Cmd_Spawn_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"stats",			Cmd_Stats_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"switchteam",		Cmd_Switchteam_f,		CF_ALLOW_SPEC},
	{"target",			Cmd_Target_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"team",			Cmd_Team_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"teleport",		Cmd_Teleport_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"unhook",			Cmd_UnHook_f,			CF_NONE},
	{"use",				Cmd_Use_f,				CF_NONE},
	{"use_index",		Cmd_Use_f,				CF_NONE},
	{"use_index_only",	Cmd_Use_f,				CF_NONE},
	{"use_only",		Cmd_Use_f,				CF_NONE},
	{"warp",			Voting_Warp_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"wave",			Cmd_Wave_f,				CF_NONE},
	{"weaplast",		Cmd_WeapLast_f,			CF_NONE},
	{"weapnext",		Cmd_WeapNext_f,			CF_NONE},
	{"weapprev",		Cmd_WeapPrev_f,			CF_NONE},
	{"where",			Cmd_Where_f,			CF_ALLOW_SPEC},
#if 0
	{"forceready",		Cmd_Match_Ready_f,		CF_ADMIN_ONLY},
	{"forcenotready",	Cmd_Match_NotReady_f,	CF_ADMIN_ONLY},

	{"setteam",			Cmd_Admin_SetTeam_f,	CF_ADMIN_ONLY},
	{"forcestart",		Cmd_Admin_ForceStart_f,	CF_ADMIN_ONLY},
	{"forceend",		Cmd_Admin_ForceEnd_f,	CF_ADMIN_ONLY},
	{"shuffle",			Cmd_Admin_Shuffle_f,	CF_ADMIN_ONLY}
#endif
};

/*
===============
FindClientCmdByName

===============
*/
static client_commands_t *FindClientCmdByName(const char *name) {
	client_commands_t *cc = cmds;
	int					i;

	for (i = 0; i < (sizeof(cmds) / sizeof(cmds[0])); i++, cc++) {
		if (!cc->name)
			continue;
		if (!Q_strcasecmp(cc->name, name))
			return cc;
	}

	return nullptr;
}

/*
=================
ClientCommand
=================
*/
void ClientCommand(edict_t *ent) {
	client_commands_t *cc;
	const char *cmd;

	if (!ent->client)
		return; // not fully in game yet

	cmd = gi.argv(0);

	// [Paril-KEX] these have to go through the lobby system
#ifndef KEX_Q2_GAME
	if (!Q_strcasecmp(cmd, "say")) {
		Cmd_Say_f(ent, false);
		return;
	}
	if (!Q_strcasecmp(cmd, "say_team") == 0 || !Q_strcasecmp(cmd, "steam")) {
		if (IsTeamplay())
			Cmd_Say_Team_f(ent, gi.args());
		else
			Cmd_Say_f(ent, false);
		return;
	}
#endif

	cc = FindClientCmdByName(cmd);
	if (!cc) {
		if (cmd == "admin") {
			if (!(cc->flags & CF_ADMIN_ONLY))
				if (!AdminOk(ent))
					return;
		} else {
			gi.LocClient_Print(ent, PRINT_HIGH, "Invalid client command: \"{}\"\n", cmd);
			return;
		}
	}

	if (cc->flags & CF_CHEAT_PROTECT)
		if (!CheatsOk(ent))
			return;

	if (!(cc->flags & CF_ALLOW_DEAD))
		if (!AliveOk(ent))
			return;

	if (!(cc->flags & CF_ALLOW_SPEC))
		if (!SpectatorOk(ent))
			return;

	if (cc->flags & CF_MATCH_ONLY)
		if (!MatchOk(ent))
			return;

	if (!(cc->flags & CF_ALLOW_INT))
		if (level.intermission_time)
			return;

	cc->func(ent);
}
