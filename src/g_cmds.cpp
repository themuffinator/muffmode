// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "monsters/m_player.h"
/*freeze*/
#if 0
#include "freeze.h"
#endif
/*freeze*/

enum cmd_flags_t : uint32_t {
	CF_NONE = 0,
	CF_ALLOW_DEAD = bit_v<0>,
	CF_ALLOW_INT = bit_v<1>,
	CF_ALLOW_SPEC = bit_v<2>,
	CF_MATCH_ONLY = bit_v<3>,
	CF_ADMIN_ONLY = bit_v<4>,
	CF_CHEAT_PROTECT = bit_v<5>,
};

struct cmds_t {
	const		char *name;
	void		(*func)(edict_t *ent);
	uint32_t	flags;
};

static void Cmd_Print_State(edict_t *ent, bool on_state) {
	const char *s = gi.argv(0);
	if (s)
		gi.LocClient_Print(ent, PRINT_HIGH, "{} {}\n", s, on_state ? "ON" : "OFF");
}


static inline bool CheatsOk(edict_t *ent) {
	if (!deathmatch->integer && !coop->integer) {
		return true;
	}
	
	if (!g_cheats->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Cheats must be enabled to use this command.\n");
		return false;
	}

	return true;
}

static inline bool AliveOk(edict_t *ent) {
	if (ent->health <= 0) {
		//gi.LocClient_Print(ent, PRINT_HIGH, "You must be alive to use this command.\n");
		return false;
	}

	return true;
}

static inline bool SpectatorOk(edict_t *ent) {
	if (!ClientIsPlaying(ent->client)) {
		//gi.LocClient_Print(ent, PRINT_HIGH, "Spectators cannot use this command.\n");
		return false;
	}

	return true;
}

static inline bool AdminOk(edict_t *ent) {
	if (!g_allow_admin->integer || !ent->client->resp.admin) {
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

	if (menu && cl->menu) {
		P_Menu_Next(ent);
		return;
	} else if (menu && cl->chase_target) {
		ChaseNext(ent);
		return;
	}

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
	SelectNextItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE);
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

	if (cl->menu) {
		P_Menu_Prev(ent);
		return;
	} else if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

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
	SelectPrevItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE);
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
			SpawnAndGiveItem(ent, IT_PACK);

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
		SpawnAndGiveItem(ent, IT_POWER_SHIELD);

		if (!give_all)
			return;
	}

	if (give_all) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemlist + i;
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

	if (it->flags & IF_NOT_GIVEABLE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_giveable");
		return;
	}

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

void ED_ParseField(const char *key, const char *value, edict_t *ent);
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
	other->s.angles[YAW] = ent->s.angles[YAW];

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
		it = Tech_Held(ent);

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

	if (deathmatch->integer && ent->client->menu) {
		P_Menu_Close(ent);
		ent->client->update_chase = true;
		if (!ent->client->initial_menu_closure) {
			gi.LocClient_Print(ent, PRINT_CENTER, "%bind:inven:Toggles Menu%{}", " ");
			ent->client->initial_menu_closure = true;
		}
		return;
	}

	if (cl->showinventory) {
		cl->showinventory = false;
		return;
	}

	if (deathmatch->integer) {
		G_Menu_Join_Open(ent);
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

	if (deathmatch->integer && ent->client->menu) {
		P_Menu_Select(ent);
		return;
	}

	if (ent->client->resp.team == TEAM_SPECTATOR)
		return;

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
		if (cl->newweapon == it)
			return; // successful
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

		if (cl->newweapon == it)
			return;
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f(edict_t *ent) {
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

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
Cmd_Forfeit_f
=================
*/
static void Cmd_Forfeit_f(edict_t *ent) {
	if (!duel->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Forfeit is only available in a duel.\n");
		return;
	}
	if (level.match_state < MS_MATCH_IN_PROGRESS) {
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

	QueueIntermission(G_Fmt("{} forfeits the match.", ent->client->resp.netname).data(), true, false);
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

	//  make sure no trackers are still hurting us.
	if (ent->client->tracker_pain_time)
		RemoveAttackingPainDaemons(ent);

	if (ent->client->owned_sphere) {
		G_FreeEdict(ent->client->owned_sphere);
		ent->client->owned_sphere = nullptr;
	}

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

	if (deathmatch->integer && ent->client->menu)
		P_Menu_Close(ent);
	ent->client->update_chase = true;
}

static int PlayerSort(const void *a, const void *b) {
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
PlayersList
=================
*/
static void PlayersList(edict_t *ent, bool ranked) {
	size_t	i, count;
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

	// sort by score
	if (ranked)
		qsort(index, count, sizeof(index[0]), PlayerSort);

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

/*
=================
Cmd_Players_f
=================
*/
static void Cmd_Players_f(edict_t *ent) {
	PlayersList(ent, false);
}

/*
=================
Cmd_PlayersRanked_f
=================
*/
static void Cmd_PlayersRanked_f(edict_t *ent) {
	PlayersList(ent, true);
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
	int i = atoi(gi.argv(1));

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
				gi.LocClient_Print(player, PRINT_HIGH, other_notify_msg, ent->client->resp.netname);
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
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_msg, ent->client->resp.netname, aiming_at->client->resp.netname);
			else if (other_notify_none_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_none_msg, ent->client->resp.netname);
		}

		if (aiming_at && other_notify_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_msg, ent->client->resp.netname, aiming_at->client->resp.netname);
		else if (other_notify_none_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_none_msg, ent->client->resp.netname);
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
	fmt::format_to(std::back_inserter(text), FMT_STRING("{}: "), ent->client->resp.netname);

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

	for (size_t i = 0; i < game.maxclients; i++) {
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (cl_ent->client->resp.team == who->client->resp.team)
			gi.LocClient_Print(cl_ent, PRINT_CHAT, "({}): {}\n",
				who->client->resp.netname, msg);
	}
}
#endif

/*
=================
Cmd_Switchteam_f
=================
*/
static void Cmd_Switchteam_f(edict_t *ent) {
	if (!Teams())
		return;

	// [Paril-KEX] in force-join, just do a regular team join.
	if (g_dm_force_join->integer) {
		// check if we should even switch teams
		team_t best_team = PickTeam(-1);

		if (ent->client->resp.team != best_team) {

			ent->svflags = SVF_NONE;
			ent->flags &= ~FL_GODMODE;
			ent->client->resp.team = best_team;
			ent->client->resp.ctf_state = 0;

			G_AssignPlayerSkin(ent, ent->client->pers.skin);

			// if anybody has a menu open, update it immediately
			P_Menu_Dirty();

			if (ent->solid == SOLID_NOT) {
				// spectator
				ClientSpawn(ent);

				G_PostRespawn(ent);

				gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_team",
					ent->client->resp.netname, Teams_TeamName(best_team));
				return;
			}

			ent->health = 0;
			player_die(ent, ent, ent, 100000, vec3_origin, { MOD_CHANGE_TEAM, true });

			// don't even bother waiting for death frames
			ent->deadflag = true;
			ClientRespawn(ent);

			G_SetPlayerScore(ent->client, 0);

			gi.LocBroadcast_Print(PRINT_HIGH, "$g_changed_team",
				ent->client->resp.netname, Teams_TeamName(best_team));
		}

		return;
	}

	if (ClientIsPlaying(ent->client))
		SetTeam(ent, TEAM_SPECTATOR, false, false);
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
int8_t TeamCount(int8_t ignore_client_num, team_t team) {
	uint8_t	count = 0;

	for (size_t i = 0; i < game.maxclients; i++) {
		if (i == ignore_client_num)
			continue;
		if (!game.clients[i].pers.connected)
			continue;
		if (game.clients[i].resp.team != team)
			continue;

		count++;
	}

	return count;
}


/*
================
PickTeam
================
*/
team_t PickTeam(int ignore_client_num) {
	uint8_t counts[TEAM_NUM_TEAMS] = {};

	if (!Teams())
		return TEAM_FREE;

	counts[TEAM_BLUE] = TeamCount(ignore_client_num, TEAM_BLUE);
	counts[TEAM_RED] = TeamCount(ignore_client_num, TEAM_RED);

	if (counts[TEAM_BLUE] > counts[TEAM_RED])
		return TEAM_RED;

	if (counts[TEAM_RED] > counts[TEAM_BLUE])
		return TEAM_BLUE;

	// equal team count, so join the team with the lowest score
	if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED])
		return TEAM_RED;
	if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE])
		return TEAM_BLUE;

	// equal team scores, so join team with lowest total individual scores
	// skip in tdm as it's redundant
	if (!teamplay->integer) {
		int iscore_red = 0, iscore_blue = 0;

		for (size_t i = 0; i < game.maxclients; i++) {
			if (i == ignore_client_num)
				continue;
			if (!game.clients[i].pers.connected)
				continue;

			if (game.clients[i].resp.team == TEAM_RED) {
				iscore_red += game.clients[i].resp.score;
				continue;
			}
			if (game.clients[i].resp.team == TEAM_BLUE) {
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
void BroadcastTeamChange(edict_t *ent, int old_team, bool inactive) {
	const char *s = nullptr, *t = nullptr;
	char		name[MAX_INFO_VALUE] = { 0 };
	int32_t		client_num;

	if (!deathmatch->integer)
		return;

	if (!ent->client)
		return;

	if (!duel->integer && ent->client->resp.team == old_team)
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
			if (duel->integer && ent->client->resp.duel_queued) {
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
		s = G_Fmt("{} joined the {} Team.\n", name, Teams_TeamName(ent->client->resp.team)).data();
		t = G_Fmt("You have joined the {} Team.\n", Teams_TeamName(ent->client->resp.team)).data();
		break;
	}

	if (s) {
		edict_t *e = g_edicts;
		for (size_t i = 1; i <= game.maxclients; i++, e++) {
			if (e == ent)
				continue;
			if (!e->client)
				continue;
			if (!e->client->pers.connected)
				continue;
			if (e->svflags & SVF_BOT)
				continue;
			gi.LocClient_Print(e, PRINT_CENTER, s);
		}
		gi.Com_Print(s);
	}
	
	if (g_motd->string[0]) {
		gi.LocCenter_Print(ent, g_motd->string);
		return;
	}

	if (level.match_state == MS_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_CENTER, G_Fmt("%bind:inven:Toggles Menu%{}\nType \"ready\" in console to ready up.", t).data());
	} else if (t) {
		gi.LocClient_Print(ent, PRINT_CENTER, G_Fmt("%bind:inven:Toggles Menu%{}", t).data() );
	}
}


/*
=================
AllowTeamSwitch
=================
*/
bool AllowTeamSwitch(int client_num, int new_team) {
	if (maxplayers->integer && level.num_playing_clients >= maxplayers->integer) {
		gi.LocClient_Print(&g_edicts[client_num], PRINT_HIGH, "Maximum player count has been reached.\n");
		return false; // ignore the request
	}

	if (level.locked[new_team]) {
		gi.LocClient_Print(&g_edicts[client_num], PRINT_HIGH, "{} is locked.\n", Teams_TeamName(new_team));
		return false; // ignore the request
	}

	if (Teams()) {
		if (g_teamplay_force_balance->integer) {
			uint8_t counts[TEAM_NUM_TEAMS] = {};

			counts[TEAM_BLUE] = TeamCount(client_num, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(client_num, TEAM_RED);

			// We allow a spread of two
			if ((new_team == TEAM_RED && abs(counts[TEAM_RED] - counts[TEAM_BLUE]) > 1) ||
				(new_team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1)) {
				gi.LocClient_Print(&g_edicts[client_num], PRINT_HIGH, "{} has too many players.\n", Teams_TeamName(new_team));
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
bool AllowClientTeamSwitch(edict_t *ent) {
	if (!deathmatch->integer)
		return false;

	if (g_dm_force_join->integer || !g_teamplay_allow_team_pick->integer) {
		if (!(ent->svflags & SVF_BOT)) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Team picks are disabled.");
			return false;
		}
	}
	
	if (ent->client->resp.team_delay_time > level.time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
		return false;
	}

	return true;
}

/*
=============
SortRandom
=============
*/
static int SortRandom(const void *a, const void *b) {
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

	// then randomly
	return brandom() ? 1 : -1;
}
#if 0
int RandomIntArray(int array[]) {
	uint32_t count = ARRAY_LEN(array);

	for (int i = 0; i < count; i++) {
		int j = rand() % count;
		std::swap(array[i], array[j]);
	}
}
#endif
/*
================
TeamShuffle

Randomly shuffles all players in teamplay
================
*/
static bool TeamShuffle() {
	if (!Teams())
		return false;

	if (level.num_playing_clients < 3)
		return false;

	bool join_red = true;
	gclient_t *cl;
	edict_t *ent;
#if 0
	// ---
	int array[MAX_CLIENTS];
	uint32_t count = MAX_CLIENTS;

	std::vector<int> myvector(count);
	std::copy(level.sorted_clients, level.sorted_clients + count, myvector.begin());

	for (std::vector<int>::iterator it = myvector.begin(); it != myvector.end(); ++it) {

	}

	for (int i = 0; i < count; i++) {
		int j = rand() % count;
		std::swap(array[i], array[j]);
	}
	// ---
#endif
	// randomise player list
	//qsort(level.random_clients, level.num_connected_clients, sizeof(level.random_clients[0]), SortRandom);
#if 0
	int count = 0;
	edict_t *e2 = nullptr;
	int ar[MAX_CLIENTS];
	bool red = !!irandom(2);

	std::vector<int> v{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(v.begin(), v.end(), g);

	std::shuffle(level.sorted_clients, level.sorted_clients + MAX_CLIENTS - 1, ar);

	for (size_t i = 0; i < game.maxclients; i++) {
		cl = &game.clients[level.sorted_clients[i]];
		ent = &g_edicts[cl - game.clients];

		if (!ent)
			continue;
		if (!ent->inuse)
			continue;
		if (!cl)
			continue;
		if (!cl->pers.connected)
			continue;
		if (cl->resp.team == TEAM_SPECTATOR)
			continue;

		SetTeam(ent, red ? TEAM_RED : TEAM_BLUE, false, true);
		red ^= true;
	}
#endif
#if 0
	// randomize list
	for (size_t i = 0; i < game.maxclients, count < level.num_playing_clients; i++) {
		cl = &game.clients[level.sorted_clients[i]];
		ent = &g_edicts[cl - game.clients];

		if (!cl->pers.connected)
			continue;
		if (cl->resp.team == TEAM_SPECTATOR)
			continue;

		count++;
	}

	for (size_t i = 0; i < game.maxclients; i++) {
		cl = &game.clients[level.random_clients[i]];
		ent = &g_edicts[cl - game.clients];

		if (!cl->pers.connected)
			continue;
		if (cl->resp.team == TEAM_SPECTATOR)
			continue;

		// alternate between red and blue
		cl->resp.team = join_red ? TEAM_RED : TEAM_BLUE;
		join_red ^= true;

		G_AssignPlayerSkin(ent, cl->pers.skin);

		// assign a ghost code
		Match_Ghost_DoAssign(ent);

		ClientSpawn(ent);

		G_PostRespawn(ent);

		if (level.match_state == MS_WARMUP_READYUP) {
			gi.LocCenter_Print(ent, "Type \"ready\" in console to ready up.\n");
		}

		// if anybody has a menu open, update it immediately
		P_Menu_Dirty();
	}
#endif
	return true;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
static void StopFollowing(edict_t *ent, bool release) {
	gclient_t *client;

	if (ent->svflags & SVF_BOT || !ent->inuse)
		return;

	client = ent->client;

	client->resp.team = TEAM_SPECTATOR;
	client->resp.spectator_state = SPECTATOR_FREE;
	if (release) {
		client->ps.stats[STAT_HEALTH] = ent->health = 1;
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;
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
bool SetTeam(edict_t *ent, team_t desired_team, bool inactive, bool force) {
	team_t old_team = ent->client->resp.team;
	bool queue = false;

	if (!force) {
		if (level.match_state == MS_MATCH_IN_PROGRESS && g_match_lock->integer) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Match is locked, no joining permitted.\n");
			P_Menu_Close(ent);
			return false;
		}

		if (desired_team != TEAM_SPECTATOR && desired_team == ent->client->resp.team) {
			P_Menu_Close(ent);
			return false;
		}

		if (duel->integer) {
			if (desired_team != TEAM_SPECTATOR && level.num_playing_clients >= 2) {
				desired_team = TEAM_SPECTATOR;
				queue = true;
				P_Menu_Close(ent);
			}
		}

		if (!AllowTeamSwitch(ent - g_edicts, desired_team))
			return false;

		if (!inactive && ent->client->resp.team_delay_time > level.time) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
			P_Menu_Close(ent);
			return false;
		}
	} else {
		if (duel->integer) {
			if (desired_team == TEAM_NONE) {
				desired_team = TEAM_SPECTATOR;
				queue = true;
			}
		}
	}

	// allow the change...

	P_Menu_Close(ent);

	// start as spectator
	if (ent->movetype == MOVETYPE_NOCLIP)
		Weapon_Grapple_DoReset(ent->client);

	CTF_DeadDropFlag(ent);
	Tech_DeadDrop(ent);

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.score = 0;
	ent->client->resp.team = desired_team;
	ent->client->resp.ctf_state = 0;
	ent->client->resp.inactive = inactive;
	ent->client->resp.inactivity_time = 0_ms;
	ent->client->resp.team_join_time = level.time;
	ent->client->resp.team_delay_time = force ? level.time : level.time + 5_sec;
	ent->client->resp.spectator_state = desired_team == TEAM_SPECTATOR ? SPECTATOR_FREE : SPECTATOR_NOT;
	ent->client->resp.spectator_client = 0;
	ent->client->resp.spectator_time = 0;
	ent->client->resp.duel_queued = queue;

	ent->client->chase_target = nullptr;

	if (desired_team != TEAM_SPECTATOR) {
		if (Teams())
			G_AssignPlayerSkin(ent, ent->client->pers.skin);

		G_RevertVote(ent->client);

		// assign a ghost code
		Match_Ghost_DoAssign(ent);
	}
	if (!force) {
		ent->client->resp.initialised = true;
	}

	// if they are playing a duel, count as a loss
	if (duel->integer && old_team == TEAM_FREE)
		ent->client->resp.losses++;

	ClientSpawn(ent);

	G_PostRespawn(ent);

	BroadcastTeamChange(ent, old_team, inactive);

	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = desired_team == TEAM_SPECTATOR ? 0 : 1;

	// if anybody has a menu open, update it immediately
	P_Menu_Dirty();

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
			gi.LocClient_Print(ent, PRINT_HIGH, "Your team: {}.\n", Teams_TeamName(ent->client->resp.team));
			break;
		case TEAM_FREE:
			gi.LocClient_Print(ent, PRINT_HIGH, "You are in the match.\n");
			break;
		case TEAM_SPECTATOR:
			gi.LocClient_Print(ent, PRINT_HIGH, "You are spectating.\n");
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

	SetTeam(ent, team, false, false);
}

/*
=================
Cmd_CrosshairID_f
=================
*/
static void Cmd_CrosshairID_f(edict_t *ent) {
	ent->client->resp.id_state ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "{} player identication display.\n", ent->client->resp.id_state ? "Activating" : "Disabling");
}

/*
=================
Cmd_Timer_f
=================
*/
static void Cmd_Timer_f(edict_t *ent) {
	ent->client->resp.timer_state ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "{} match timer display.\n", ent->client->resp.timer_state ? "Activating" : "Disabling");
}

/*
=================
Cmd_FragMessages_f
=================
*/
static void Cmd_FragMessages_f(edict_t *ent) {
	ent->client->resp.fragmessage_state ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "{} frag messages.\n", ent->client->resp.fragmessage_state ? "Activating" : "Disabling");
}

/*
=================
Cmd_KillBeep_f
=================
*/
static void Cmd_KillBeep_f(edict_t *ent) {
	int num = 0;
	if (gi.argc() > 1) {
		num = atoi(gi.argv(1));
		if (num < 0)
			num = 0;
		else if (num > 4)
			num = 4;
	} else {
		num = (ent->client->resp.killbeep_num + 1) % 5;
	}
	const char *sb[5] = { "off", "clang", "beep-boop", "insane", "tang-tang" };
	ent->client->resp.killbeep_num = num;
	gi.LocClient_Print(ent, PRINT_HIGH, "Kill beep changed to: {}\n", sb[num]);
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

	if (ClientIsPlaying(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You are already in the game.\n");
		return;
	}
	if (level.match_state != MS_MATCH_IN_PROGRESS) {
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
				ent->client->resp.netname, Teams_TeamName(ent->client->resp.team));
			return;
		}
	}
	gi.LocClient_Print(ent, PRINT_HIGH, "Invalid ghost code.\n");
}


static void Cmd_Stats_f(edict_t *ent) {
	if (!ctf->integer)
		return;

	ghost_t *g;
	static std::string text;
	edict_t *e2;

	text.clear();

	if (level.match_state == MS_WARMUP_READYUP) {
		for (uint32_t i = 1; i <= game.maxclients; i++) {
			e2 = g_edicts + i;
			if (!e2->inuse)
				continue;
			if (!e2->client->resp.ready && ClientIsPlaying(e2->client)) {
				std::string_view str = G_Fmt("{} is not ready.\n", e2->client->resp.netname);

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
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid client number.\n");
		return;
	}

	targ = g_edicts + i;
	if (!targ->inuse) {
		gi.LocClient_Print(ent, PRINT_HIGH, "That client number is not connected.\n");
		return;
	}

	gi.AddCommandString(G_Fmt("kick {}\n", i - 1).data());
}

/*----------------------------------------------------------------*/

// NEW VOTING CODE

static bool Vote_Val_None(edict_t *ent) {
	return true;
}

static void Vote_Pass_Map() {
	Q_strlcpy(level.forcemap, level.vote_arg, sizeof(level.forcemap));
	Match_End();
	//BeginIntermission(CreateTargetChangeLevel(level.forcemap));
	level.intermission_time -= 5_sec;
	ExitLevel();
}

static bool Vote_Val_Map(edict_t *ent) {
	char *token;

	if (gi.argc() < 3) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Valid maps are: {}\n", g_map_list->string);
		return false;
	}

	const char *mlist = g_map_list->string;

	while (*(token = COM_Parse(&mlist))) {
		if (!Q_strcasecmp(token, gi.argv(2)))
			break;
	}

	if (!*token) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Unknown map.\n");
		gi.LocClient_Print(ent, PRINT_HIGH, "Valid maps are: {}\n", g_map_list->string);
		return false;
	}

	return true;
}

static void Vote_Pass_RestartMatch() {
	Q_strlcpy(level.forcemap, level.mapname, sizeof(level.forcemap));
	Match_End();
}

static void Vote_Pass_Gametype() {
	const char *arg = level.vote_arg;

	for (size_t i = 0; i < GT_NUM_GAMETYPES; i++) {
		if (!Q_strcasecmp(arg, gt_short_name[i])) {
			GT_Change(i);
			break;
		}
	}
}

static bool Vote_Val_Gametype(edict_t *ent) {
	const char *arg = gi.argv(2);

	for (size_t i = 0; i < GT_NUM_GAMETYPES; i++) {
		if (!Q_strcasecmp(arg, gt_short_name[i]))
			return true;
	}

	return false;
}

static void Vote_Pass_NextMap() {
	Match_End();
}

static void Vote_Pass_ShuffleTeams() {
	TeamShuffle();
}

static bool Vote_Val_ShuffleTeams(edict_t *ent) {
	if (!Teams())
		return false;

	return true;
}

static void Vote_Pass_Unlagged() {
	int argi = atoi(level.vote_arg);

	gi.LocBroadcast_Print(PRINT_HIGH, "Lag compensation has been {}.\n", argi ? "ENABLED" : "DISABLED");

	gi.cvar_forceset("g_lag_compensation", argi ? "1" : "0");
}

static bool Vote_Val_Unlagged(edict_t *ent) {
	int arg = atoi(gi.argv(2));


	return true;
}

static bool Vote_Val_Random(edict_t *ent) {
	int arg = atoi(gi.argv(2));

	if (arg > 100 || arg < 2)
		return false;

	return true;
}

static void Vote_Pass_Cointoss() {
	gi.LocBroadcast_Print(PRINT_HIGH, "The coin is: {}\n", brandom() ? "HEADS" : "TAILS");
}

static void Vote_Pass_Random() {
	gi.LocBroadcast_Print(PRINT_HIGH, "The random number is: {}\n", irandom(2, atoi(level.vote_arg)));
}

static void Vote_Pass_Timelimit() {
	const char *s = level.vote_arg;
	int argi = atoi(s);

	if (!argi)
		gi.LocBroadcast_Print(PRINT_HIGH, "Time limit has been DISABLED.\n");
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "Time limit has been set to {}.\n", G_TimeString(argi * 1000));

	gi.cvar_forceset("timelimit", level.vote_arg);
}

static bool Vote_Val_Timelimit(edict_t *ent) {
	int argi = atoi(gi.argv(2));

	if (argi < 0) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid time limit value.\n");
		return false;
	}
	
	if (argi == timelimit->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Time limit is already set to {}.\n", G_TimeString(argi * 1000));
		return false;
	}
	return true;
}

static void Vote_Pass_Scorelimit() {
	int argi = atoi(level.vote_arg);

	//if (argi)
		gi.LocBroadcast_Print(PRINT_HIGH, "Score limit has been set to {}.\n", argi);
	//else
	//	gi.LocBroadcast_Print(PRINT_HIGH, "Score limit has been DISABLED.\n");

	gi.cvar_forceset(G_Fmt("{}limit", GT_ScoreLimitString()).data(), level.vote_arg);
}

static bool Vote_Val_Scorelimit(edict_t *ent) {
	int argi = atoi(gi.argv(2));

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

vcmds_t vote_cmds[] = {
	{"map",					Vote_Val_Map,			Vote_Pass_Map,			1,		2,	"[mapname]",						"changes to the specified map"},
	{"nextmap",				Vote_Val_None,			Vote_Pass_NextMap,		2,		1,	"",									"move to the next map in the rotation"},
	{"restart",				Vote_Val_None,			Vote_Pass_RestartMatch,	4,		1,	"",									"restarts the current match"},
	{"gametype",			Vote_Val_Gametype,		Vote_Pass_Gametype,		8,		2,	"<ffa|duel|tdm|ctf|ca|ft|horde>",	"changes the current gametype"},
	{"timelimit",			Vote_Val_Timelimit,		Vote_Pass_Timelimit,	16,		2,	"<0..$>",							"alters the match time limit, 0 for no time limit"},
	{"scorelimit",			Vote_Val_Scorelimit,	Vote_Pass_Scorelimit,	32,		2,	"<0..$>",							"alters the match score limit, 0 for no score limit"},
	{"shuffle",				Vote_Val_ShuffleTeams,	Vote_Pass_ShuffleTeams,	64,		2,	"",									"shuffles teams"},
	{"unlagged",			Vote_Val_Unlagged,		Vote_Pass_Unlagged,		128,	2,	"<0/1>",							"enables or disables lag compensation"},
	{"cointoss",			Vote_Val_None,			Vote_Pass_Cointoss,		256,	1,	"",									"invokes a HEADS or TAILS cointoss"},
	{"random",				Vote_Val_Random,		Vote_Pass_Random,		512,	1,	"<2-100>",							"randomly selects a number from 2 to specified value"},
};

/*
===============
FindVoteCmdByName

===============
*/
static vcmds_t *FindVoteCmdByName(const char *name) {
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
Cmd_Vote_Passed
==================
*/
void Cmd_Vote_Passed() {
	level.vote->func();

	level.vote = nullptr;
	//Q_strlcpy(level.vote_arg, nullptr, sizeof(level.vote_arg));
	//level.vote_arg = nullptr;
	level.vote_execute_time = 0_sec;
}

/*
=================
ValidVoteCommand
=================
*/
static bool ValidVoteCommand(edict_t *ent) {
	if (!ent->client)
		return false; // not fully in game yet

	level.vote = nullptr;

	vcmds_t *cc = FindVoteCmdByName(gi.argv(1));

	if (!cc) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid vote command: {}\n", gi.argv(1));
		return false;
	}

	if (cc->args && gi.argc() < (2 + cc->args ? 1 : 0)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{}: {}\nUsage: {} {}\n", cc->name, cc->help, cc->name, cc->args);
		return false;
	}

	if (!cc->val_func(ent))
		return false;

	level.vote = cc;
	//Q_strlcpy(level.vote_arg, gi.argv(2), sizeof(level.vote_arg));
	level.vote_arg = gi.argv(2);
	gi.Com_PrintFmt("vote_arg={}\n", level.vote_arg);
	return true;
}

/*
==================
Cmd_CallVote_f
==================
*/
static void Cmd_CallVote_f(edict_t *ent) {
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
		gi.LocClient_Print(ent, PRINT_HIGH, "Voting not allowed here.\n\"");
		return;
	}

	if (!g_allow_vote_midgame->integer && level.match_state >= MS_MATCH_COUNTDOWN) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Voting is only allowed during the warm up period.\n\"");
		return;
	}

	if (level.vote_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "A vote is already in progress.\n\"");
		return;
	}

	// if there is still a vote to be executed
	if (level.vote_execute_time || level.restarted) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Previous vote command is still awaiting execution.\n\"");
		return;
	}

	if (!g_allow_spec_vote->integer && !ClientIsPlaying(ent->client)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You are not allowed to call a vote as a spectator.\n\"");
		return;
	}

	if (g_vote_limit->integer && ent->client->pers.vote_count >= g_vote_limit->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You have called the maximum number of votes ({}).\n\"", g_vote_limit->integer);
		return;
	}

	if (gi.argc() < 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} <command> <params>\nValid Voting Commands:{}\n", gi.argv(0), vstr);
		return;
	}

	// make sure it is a valid command to vote on
	if (!ValidVoteCommand(ent))
		return;

	gi.LocBroadcast_Print(PRINT_CENTER, "{} called a vote:\n{}{}\n", ent->client->resp.netname, gi.argv(1), (gi.argc() > 2 && strlen(level.vote->args)) ? G_Fmt(" {}", gi.argv(2)).data() : "");

	// start the voting, the caller automatically votes yes
	level.vote_time = level.time;
	level.vote_yes = 1;
	level.vote_no = 0;

	for (size_t i = 0; i < game.maxclients; i++)
		game.clients[i].pers.voted = 0;

	ent->client->pers.voted = 1;

	ent->client->pers.vote_count++;

	//trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.vote_time));
	//trap_SetConfigstring(CS_VOTE_STRING, level.vote_display_string);
	//trap_SetConfigstring(CS_VOTE_YES, va("%i", level.vote_yes));
	//trap_SetConfigstring(CS_VOTE_NO, va("%i", level.vote_no));
}

/*
==================
Cmd_Vote_f
==================
*/
static void Cmd_Vote_f(edict_t *ent) {
	if (!deathmatch->integer)
		return;

	if (!level.vote_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "print \"No vote in progress.\n\"");
		return;
	}

	if (ent->client->pers.voted != 0) {
		gi.LocClient_Print(ent, PRINT_HIGH, "print \"Vote already cast.\n\"");
		return;
	}

	if (ent->client->resp.team == TEAM_SPECTATOR) {
		gi.LocClient_Print(ent, PRINT_HIGH, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "print \"Vote cast.\n\"");

	const char *arg = gi.argv(1);

	if (arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1') {
		level.vote_yes++;
		ent->client->pers.voted = 1;
		//trap_SetConfigstring(CS_VOTE_YES, va("%i", level.vote_yes));
	} else {
		level.vote_no++;
		ent->client->pers.voted = -1;
		//trap_SetConfigstring(CS_VOTE_NO, va("%i", level.vote_no));
	}
	
	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

void G_RevertVote(gclient_t *client) {
	if (!level.vote_time)
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

/*----------------------------------------------------------------*/

/*
=================
Cmd_Admin_AllReady_f
=================
*/
static void Cmd_Admin_AllReady_f(edict_t *ent) {
	if (level.match_state != MS_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Cannot change ready state at this point.\n");
		return;
	}
	ReadyAll();
}

/*
=================
Cmd_Admin_AllUnready_f
=================
*/
static void Cmd_Admin_AllUnready_f(edict_t *ent) {
	if (level.match_state != MS_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Cannot change ready state at this point.\n");
		return;
	}
	UnReadyAll();
}

/*
=================
Cmd_Admin_LockTeam_f
=================
*/
static void Cmd_Admin_LockTeam_f(edict_t *ent) {
	if (gi.argc() < 3) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} {} [team]\n", gi.argv(0), gi.argv(1));
		return;
	}

	team_t team = StringToTeamNum(gi.argv(2));

	if (team == TEAM_NONE || team == TEAM_SPECTATOR) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}

	if (level.locked[team]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already locked.\n", Teams_TeamName(team));
		return;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "{} has been locked\n", Teams_TeamName(team));
	level.locked[team] = true;
}

/*
=================
Cmd_Admin_UnlockTeam_f
=================
*/
static void Cmd_Admin_UnlockTeam_f(edict_t *ent) {
	if (gi.argc() < 3) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} {} [team]\n", gi.argv(0), gi.argv(1));
		return;
	}

	team_t team = StringToTeamNum(gi.argv(2));

	if (team == TEAM_NONE || team == TEAM_SPECTATOR) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}

	if (!level.locked[team]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already unlocked.\n", Teams_TeamName(team));
		return;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "{} has been unlocked\n", Teams_TeamName(team));
	level.locked[team] = false;
}

/*
=================
Cmd_Admin_SetTeam_f
=================
*/
static void Cmd_Admin_SetTeam_f(edict_t *ent) {
	// admin forceteam [playername/num] [team]
	if (gi.argc() < 4) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} {} [playername/num] [team]\n", gi.argv(0), gi.argv(1));
		return;
	}

	char name[MAX_NETNAME];
	team_t team = StringToTeamNum(gi.argv(3));
	edict_t *targ = nullptr;

	if (team == TEAM_NONE)
		return;

	Q_strlcpy(name, gi.argv(2), sizeof(name));

	// check by nick first
	for (size_t i = 0; i < game.maxclients; i++) {
		if (!Q_strcasecmp(name, game.clients[i].resp.netname)) {
			targ = &g_edicts[&game.clients[i] - game.clients];
			break;
		}
	}

	// otherwise check client num
	if (!targ) {
		size_t num = atoi(gi.argv(2));
		if (num >= 0 && num < game.maxclients) {
			targ = &g_edicts[&game.clients[num] - game.clients + 1];
		}
	}

	if (!targ || !targ->inuse || !targ->client) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid client name or number.\n");
		return;
	}

	if (targ->client->resp.team == team) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Client is already on selected team.\n");
		return;
	}

	if ((Teams() && team == TEAM_FREE) || (!Teams() && team != TEAM_SPECTATOR && team != TEAM_FREE)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}

	SetTeam(targ, team, false, true);
}

/*
=================
Cmd_Admin_Shuffle_f
=================
*/
static void Cmd_Admin_Shuffle_f(edict_t *ent) {
	TeamShuffle();

	gi.Com_Print("SHUFFLE\n");
}

/*
=================
Cmd_Admin_StartMatch_f
=================
*/
static void Cmd_Admin_StartMatch_f(edict_t *ent) {
	if (level.match_state > MS_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has already started.\n");
		return;
	}

	Match_Start();
}

/*
=================
Cmd_Admin_EndMatch_f
=================
*/
static void Cmd_Admin_EndMatch_f(edict_t *ent) {
	if (level.match_state < MS_MATCH_IN_PROGRESS) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has not yet begun.\n");
		return;
	}
	if (level.intermission_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has already ended.\n");
		return;
	}
	QueueIntermission("Admin has forced the match to end.", true, false);
}

/*
=================
Cmd_Admin_ResetMatch_f
=================
*/
static void Cmd_Admin_ResetMatch_f(edict_t *ent) {
	if (level.match_state < MS_MATCH_IN_PROGRESS) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has not yet begun.\n");
		return;
	}
	if (level.intermission_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Match has already ended.\n");
		return;
	}
	
	gi.LocBroadcast_Print(PRINT_HIGH, "Admin has reset the match.\n");
	Match_Reset();
}

/*
=================
Cmd_Admin_ForceVote_f
=================
*/
static void Cmd_Admin_ForceVote_f(edict_t *ent) {
	if (!deathmatch->integer)
		return;

	if (!level.vote_time) {
		gi.LocClient_Print(ent, PRINT_HIGH, "print \"No vote in progress.\n\"");
		return;
	}

	const char *arg = gi.argv(2);

	if (arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1') {
		gi.LocBroadcast_Print(PRINT_HIGH, "Admin passed the vote.\n");
		level.vote_execute_time = level.time + 3_sec;
	} else {
		gi.LocBroadcast_Print(PRINT_HIGH, "Admin failed the vote.\n");
		level.vote_time = 0_sec;
	}
}

/*
=================
Cmd_Admin_Spawn_f
=================
*/
static void Cmd_Admin_Spawn_f(edict_t *ent) {
	solid_t backup = ent->solid;
	ent->solid = SOLID_NOT;
	gi.linkentity(ent);

	edict_t *other = G_Spawn();
	other->classname = gi.argv(2);

	other->s.origin = ent->s.origin + (AngleVectors(ent->s.angles).forward * 24.f);
	other->s.angles[YAW] = ent->s.angles[YAW];

	st = {};

	if (gi.argc() > 4) {
		for (int i = 3; i < gi.argc(); i += 2)
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

cmds_t admin_cmds[] = {
	{"readyall",		Cmd_Admin_AllReady_f},
	{"unreadyall",		Cmd_Admin_AllUnready_f},
	{"startmatch",		Cmd_Admin_StartMatch_f},
	{"endmatch",		Cmd_Admin_EndMatch_f},
	{"reset",			Cmd_Admin_ResetMatch_f},
	{"lockteam",		Cmd_Admin_LockTeam_f},
	{"unlockteam",		Cmd_Admin_UnlockTeam_f },
	{"setteam",			Cmd_Admin_SetTeam_f},
	{"shuffle",			Cmd_Admin_Shuffle_f},
	{"vote",			Cmd_Admin_ForceVote_f },
	{"spawn",			Cmd_Admin_Spawn_f },
};

/*
===============
FindAdminCmdByName

===============
*/
static cmds_t *FindAdminCmdByName(const char *name) {
	cmds_t *cc = admin_cmds;
	int					i;

	for (i = 0; i < (sizeof(admin_cmds) / sizeof(admin_cmds[0])); i++, cc++) {
		if (!cc->name)
			continue;
		if (!Q_strcasecmp(cc->name, name))
			return cc;
	}

	return nullptr;
}

static void Cmd_Admin_f(edict_t *ent) {
	if (!g_allow_admin->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Administration is disabled\n");
		return;
	}

	if (!ent->client->resp.admin) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You do not have administrative rights.\n");
		return;
	}
	/*
	if (gi.argc() > 1 && admin_password->string && *admin_password->string &&
		!ent->client->resp.admin && strcmp(admin_password->string, gi.argv(1)) == 0) {
		ent->client->resp.admin = true;
		gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", ent->client->resp.netname);
		gi.LocClient_Print(ent, PRINT_HIGH, "Type 'admin' to access the adminstration menu.\n");
	}
	*/
	// run command if valid...

	if (gi.argc() < 2) {
		// formulate list of admin commands
		cmds_t *ac = admin_cmds;

		char vstr[1024] = " ";
		for (size_t i = 0; i < ARRAY_LEN(admin_cmds); i++, ac++) {
			if (!ac->name)
				continue;

			strcat(vstr, G_Fmt("{} ", ac->name).data());
		}

		gi.LocClient_Print(ent, PRINT_HIGH, "Usage: {} <command> <params>\nValid Admin Commands:{}\n", gi.argv(0), vstr);
		return;
	}

	cmds_t *cc = FindAdminCmdByName(gi.argv(1));

	if (!cc)
		return;

	if (!cc->func)
		return;

	cc->func(ent);

	if (ent->client->menu)
		P_Menu_Close(ent);

	//G_Menu_Admin(ent);
}

/*----------------------------------------------------------------*/

static void Cmd_Ready_f(edict_t *ent) {
	if (level.match_state == MS_WARMUP_DEFAULT) {
		if (Teams()) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready until teams are present and balanced.\n");
		} else if (duel->integer) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready until your opponent is present.\n");
		} else {
			gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready until players are present.\n");
		}
		return;
	}

	if (level.match_state != MS_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready at this stage of the match.\n");
		return;
	}

	if (ent->client->resp.ready) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You have already committed.\n");
		return;
	}

	ent->client->resp.ready = true;
	gi.LocBroadcast_Print(PRINT_CENTER, "{} is ready.\n", ent->client->resp.netname);
}

static void Cmd_NotReady_f(edict_t *ent) {
	if (level.match_state != MS_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready at this stage of the match.\n");
		return;
	}

	if (!ent->client->resp.ready) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You haven't committed.\n");
		return;
	}

	ent->client->resp.ready = false;
	gi.LocBroadcast_Print(PRINT_CENTER, "{} is NOT ready.\n", ent->client->resp.netname);
}

static void Cmd_ReadyUp_f(edict_t *ent) {
	if (level.match_state == MS_WARMUP_DEFAULT) {
		if (Teams()) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready until teams are present and balanced.\n");
		} else if (duel->integer) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready until your opponent is present.\n");
		} else {
			gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready until players are present.\n");
		}
		return;
	}

	if (level.match_state != MS_WARMUP_READYUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot ready at this stage of the match.\n");
		return;
	}

	ent->client->resp.ready ^= true;
	gi.LocBroadcast_Print(PRINT_CENTER, "{} is {}ready.\n", ent->client->resp.netname, ent->client->resp.ready ? "" : "NOT ");
}

static void Cmd_Hook_f(edict_t *ent) {
	if (!g_allow_grapple->integer || !g_grapple_offhand->integer)
		return;

	Weapon_Hook(ent);
}

static void Cmd_UnHook_f(edict_t *ent) {
	Weapon_Grapple_DoReset(ent->client);
}

static void MQ_PrintList(edict_t *ent) {
	std::string text;
	for (size_t i = 0; i < 32; i++) {
		if (game.map_queue[i])
			text += G_Fmt("{} \n", game.map_queue[i]).data();
	}
	
	gi.LocClient_Print(ent, PRINT_HIGH, G_Fmt("{}\n", text).data());
}

static void Cmd_MapList_f(edict_t *ent) {
	if (g_map_list->string[0]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Current map list:\n");
		gi.LocClient_Print(ent, PRINT_HIGH, G_Fmt("{}\n", g_map_list->string).data());
		if (MQ_Count()) {
			gi.LocClient_Print(ent, PRINT_HIGH, "\nCurrent maps queued:\n");
			MQ_PrintList(ent);
		}
	} else {
		gi.LocClient_Print(ent, PRINT_HIGH, "No Map List set.\n");
	}
}

static void Cmd_MyMap_f(edict_t *ent) {
	if (gi.argc() < 2) {
		if (!g_map_list->string[0]) {
			gi.LocClient_Print(ent, PRINT_HIGH, "No maps are queued as no map list is present.\n");
			return;
		}

		int count = MQ_Count();
		if (!count) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Add a map to the playlist.\nRecognized maps are:\n");
			gi.LocClient_Print(ent, PRINT_HIGH, "{}\n", g_map_list->string);
			return;
		}

		gi.LocClient_Print(ent, PRINT_HIGH, "mymap queue => ");
		MQ_PrintList(ent);
		return;
	}

	MQ_Add(gi.argv(1));

	std::string text;
	for (size_t i = 0; i < 32; i++) {
		if (game.map_queue[i])
			text += G_Fmt("{} \n", game.map_queue[i]).data();
	}

	gi.LocClient_Print(ent, PRINT_HIGH, G_Fmt("mymap queue => {}\n", text).data());
}

static void Cmd_Motd_f(edict_t *ent) {
	const char *s = g_motd->string;
	if (g_motd->string[0])
		s = G_Fmt("Message of the Day:\n{}\n", g_motd->string).data();
	else
		s = "No Message of the Day set.\n";
	gi.LocClient_Print(ent, PRINT_HIGH, s);
}

// =========================================

cmds_t client_cmds[] = {
	{"a",				Cmd_Admin_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"admin",			Cmd_Admin_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"alertall",		Cmd_AlertAll_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"boot",			Cmd_Boot_f,				CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"callvote",		Cmd_CallVote_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"checkpoi",		Cmd_CheckPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"clear_ai_enemy",	Cmd_Clear_AI_Enemy_f,	CF_CHEAT_PROTECT},
	{"cv",				Cmd_CallVote_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"drop",			Cmd_Drop_f,				CF_NONE},
	{"drop_index",		Cmd_Drop_f,				CF_NONE},
	{"fm",				Cmd_FragMessages_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD},
	{"ghost",			Cmd_Ghost_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"give",			Cmd_Give_f,				CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"god",				Cmd_God_f,				CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"help",			Cmd_Help_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"hook",			Cmd_Hook_f,				CF_NONE},
	{"id",				Cmd_CrosshairID_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD},
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
	{"forfeit",			Cmd_Forfeit_f,			CF_ALLOW_DEAD},
	{"kb",				Cmd_KillBeep_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD},
	{"kill",			Cmd_Kill_f,				CF_NONE},
	{"kill_ai",			Cmd_Kill_AI_f,			CF_CHEAT_PROTECT},
	{"listmonsters",	Cmd_ListMonsters_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"maplist",			Cmd_MapList_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"mymap",			Cmd_MyMap_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"motd",			Cmd_Motd_f,				CF_ALLOW_SPEC | CF_ALLOW_INT},
	{"noclip",			Cmd_Noclip_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"notarget",		Cmd_Notarget_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"notready",		Cmd_NotReady_f,			CF_ALLOW_DEAD},
	{"novisible",		Cmd_Novisible_f,		CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"players",			Cmd_Players_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"playrank",		Cmd_PlayersRanked_f,	CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"putaway",			Cmd_PutAway_f,			CF_ALLOW_SPEC},	//spec for menu close
	{"ready",			Cmd_Ready_f,			CF_ALLOW_DEAD},
	{"readyup",			Cmd_ReadyUp_f,			CF_ALLOW_DEAD},
	{"score",			Cmd_Score_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"setpoi",			Cmd_SetPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"spawn",			Cmd_Spawn_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"stats",			Cmd_Stats_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},
	{"switchteam",		Cmd_Switchteam_f,		CF_ALLOW_SPEC},
	{"target",			Cmd_Target_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"team",			Cmd_Team_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC},
	{"teleport",		Cmd_Teleport_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},
	{"timer",			Cmd_Timer_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD},
	{"unhook",			Cmd_UnHook_f,			CF_NONE},
	{"use",				Cmd_Use_f,				CF_NONE},
	{"use_index",		Cmd_Use_f,				CF_NONE},
	{"use_index_only",	Cmd_Use_f,				CF_NONE},
	{"use_only",		Cmd_Use_f,				CF_NONE},
	{"vote",			Cmd_Vote_f,				CF_ALLOW_DEAD},
	{"wave",			Cmd_Wave_f,				CF_NONE},
	{"weaplast",		Cmd_WeapLast_f,			CF_NONE},
	{"weapnext",		Cmd_WeapNext_f,			CF_NONE},
	{"weapprev",		Cmd_WeapPrev_f,			CF_NONE},
	{"where",			Cmd_Where_f,			CF_ALLOW_SPEC}
};

/*
===============
FindClientCmdByName

===============
*/
static cmds_t *FindClientCmdByName(const char *name) {
	cmds_t *cc = client_cmds;
	int					i;

	for (i = 0; i < (sizeof(client_cmds) / sizeof(client_cmds[0])); i++, cc++) {
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
	cmds_t *cc;
	const char *cmd;

	if (!ent->client)
		return; // not fully in game yet

	cmd = gi.argv(0);
	cc = FindClientCmdByName(cmd);
	/*
	if (ent->client->resp.is_888) {
		if (!Q_strcasecmp(cmd, "say")) {
			gi.AddCommandString("say This is the real 888-LEGEND.COM server, enjoy :)\n");
			return;
		}
	}
	*/
	// [Paril-KEX] these have to go through the lobby system
#ifndef KEX_Q2_GAME
	if (!Q_strcasecmp(cmd, "say")) {
		Cmd_Say_f(ent, false);
		return;
	}
	if (!Q_strcasecmp(cmd, "say_team") == 0 || !Q_strcasecmp(cmd, "steam")) {
		if (Teams())
			Cmd_Say_Team_f(ent, gi.args());
		else
			Cmd_Say_f(ent, false);
		return;
	}
#endif

	if (!cc) {
		// always allow replace_/disable_ item cvars
		if (gi.argc() > 1 && strstr(cmd, "replace_") || strstr(cmd, "disable_")) {
			gi.cvar_forceset(cmd, gi.argv(1));
		} else
			gi.LocClient_Print(ent, PRINT_HIGH, "Invalid client command: \"{}\"\n", cmd);
		return;
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
		return;

	if (!(cc->flags & CF_ALLOW_INT))
		if (level.intermission_time)
			return;

	cc->func(ent);
}
