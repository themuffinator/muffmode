// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_utils.c -- misc utility functions for game module

#include "g_local.h"

/*
=============
G_Find

Searches all active entities for the next one that validates the given callback.

Searches beginning at the edict after from, or the beginning if nullptr
nullptr will be returned if the end of the list is reached.
=============
*/
edict_t *G_Find(edict_t *from, std::function<bool(edict_t *e)> matcher) {
	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.num_edicts]; from++) {
		if (!from->inuse)
			continue;
		if (matcher(from))
			return from;
	}

	return nullptr;
}

/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t *findradius(edict_t *from, const vec3_t &org, float rad) {
	vec3_t eorg;
	int	   j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for (; from < &g_edicts[globals.num_edicts]; from++) {
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5f);
		if (eorg.length() > rad)
			continue;
		return from;
	}

	return nullptr;
}


/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs in the structure.

Searches beginning at the edict after from, or the beginning if nullptr
nullptr will be returned if the end of the list is reached.

=============
*/
constexpr size_t MAXCHOICES = 8;

edict_t *G_PickTarget(const char *targetname) {
	edict_t *ent = nullptr;
	int		 num_choices = 0;
	edict_t *choice[MAXCHOICES];

	if (!targetname) {
		gi.Com_Print("G_PickTarget called with nullptr targetname\n");
		return nullptr;
	}

	while (1) {
		ent = G_FindByString<&edict_t::targetname>(ent, targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices) {
		gi.Com_PrintFmt("G_PickTarget: target {} not found\n", targetname);
		return nullptr;
	}

	return choice[irandom(num_choices)];
}

static THINK(Think_Delay) (edict_t *ent) -> void {
	G_UseTargets(ent, ent->activator);
	G_FreeEdict(ent);
}

void G_PrintActivationMessage(edict_t *ent, edict_t *activator, bool coop_global) {
	//
	// print the message
	//
	if ((ent->message) && !(activator->svflags & SVF_MONSTER)) {
		if (coop_global && coop->integer)
			gi.LocBroadcast_Print(PRINT_CENTER, "{}", ent->message);
		else
			gi.LocCenter_Print(activator, "{}", ent->message);

		// [Paril-KEX] allow non-noisy centerprints
		if (ent->noise_index >= 0) {
			if (ent->noise_index)
				gi.sound(activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
			else
				gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		}
	}
}

void BroadcastSpectatorMessage(const char *msg) {
	edict_t *ent;

	for (size_t i = 0; i < game.maxclients; i++) {
		ent = g_edicts + i;

		if (!ent->inuse)
			continue;

		if (!ent->client)
			continue;

		if (!ent->client->pers.connected)
			continue;

		if (ent->client->resp.team != TEAM_SPECTATOR)
			continue;

		gi.LocClient_Print(ent, PRINT_HIGH, msg);
	}
}

void G_MonsterKilled(edict_t *self);

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets(edict_t *ent, edict_t *activator) {
	edict_t *t;

	//
	// check for a delay
	//
	if (ent->delay) {
		// create a temp object to fire at a later time
		t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + gtime_t::from_sec(ent->delay);
		t->think = Think_Delay;
		t->activator = activator;
		if (!activator)
			gi.Com_Print("Think_Delay with no activator\n");
		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		return;
	}

	//
	// print the message
	//
	G_PrintActivationMessage(ent, activator, true);

	//
	// kill killtargets
	//
	if (ent->killtarget) {
		t = nullptr;
		while ((t = G_FindByString<&edict_t::targetname>(t, ent->killtarget))) {
			if (t->teammaster) {
				// if this entity is part of a chain, cleanly remove it
				if (t->flags & FL_TEAMSLAVE) {
					for (edict_t *master = t->teammaster; master; master = master->teamchain) {
						if (master->teamchain == t) {
							master->teamchain = t->teamchain;
							break;
						}
					}
				}
				// [Paril-KEX] remove teammaster too
				else if (t->flags & FL_TEAMMASTER) {
					t->teammaster->flags &= ~FL_TEAMMASTER;

					edict_t *new_master = t->teammaster->teamchain;

					if (new_master) {
						new_master->flags |= FL_TEAMMASTER;
						new_master->flags &= ~FL_TEAMSLAVE;

						for (edict_t *m = new_master; m; m = m->teamchain)
							m->teammaster = new_master;
					}
				}
			}

			// [Paril-KEX] if we killtarget a monster, clean up properly
			if (t->svflags & SVF_MONSTER) {
				if (!t->deadflag && !(t->monsterinfo.aiflags & AI_DO_NOT_COUNT) && !(t->spawnflags & SPAWNFLAG_MONSTER_DEAD))
					G_MonsterKilled(t);
			}

			G_FreeEdict(t);

			if (!ent->inuse) {
				gi.Com_Print("entity was removed while using killtargets\n");
				return;
			}
		}
	}

	//
	// fire targets
	//
	if (ent->target) {
		t = nullptr;
		while ((t = G_FindByString<&edict_t::targetname>(t, ent->target))) {
			// doors fire area portals in a specific way
			if (!Q_strcasecmp(t->classname, "func_areaportal") &&
				(!Q_strcasecmp(ent->classname, "func_door") || !Q_strcasecmp(ent->classname, "func_door_rotating")
					|| !Q_strcasecmp(ent->classname, "func_door_secret") || !Q_strcasecmp(ent->classname, "func_water")))
				continue;

			if (t == ent) {
				gi.Com_Print("WARNING: Entity used itself.\n");
			} else {
				if (t->use)
					t->use(t, ent, activator);
			}
			if (!ent->inuse) {
				gi.Com_Print("entity was removed while using targets\n");
				return;
			}
		}
	}
}

constexpr vec3_t VEC_UP = { 0, -1, 0 };
constexpr vec3_t MOVEDIR_UP = { 0, 0, 1 };
constexpr vec3_t VEC_DOWN = { 0, -2, 0 };
constexpr vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

void G_SetMovedir(vec3_t &angles, vec3_t &movedir) {
	if (angles == VEC_UP) {
		movedir = MOVEDIR_UP;
	} else if (angles == VEC_DOWN) {
		movedir = MOVEDIR_DOWN;
	} else {
		AngleVectors(angles, movedir, nullptr, nullptr);
	}

	angles = {};
}

char *G_CopyString(const char *in, int32_t tag) {
	if (!in)
		return nullptr;
	const size_t amt = strlen(in) + 1;
	char *const out = static_cast<char *>(gi.TagMalloc(amt, tag));
	Q_strlcpy(out, in, amt);
	return out;
}

void G_InitEdict(edict_t *e) {
	// FIXME -
	//   this fixes a bug somewhere that is setting "nextthink" for an entity that has
	//   already been released.  nextthink is being set to FRAME_TIME_S after level.time,
	//   since freetime = nextthink - FRAME_TIME_S
	if (e->nextthink)
		e->nextthink = 0_ms;

	e->inuse = true;
	e->sv.init = false;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;

	// do this before calling the spawn function so it can be overridden.
	e->gravityVector[0] = 0.0;
	e->gravityVector[1] = 0.0;
	e->gravityVector[2] = -1.0;
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn() {
	uint32_t i;
	edict_t *e;

	e = &g_edicts[game.maxclients + 1];
	for (i = game.maxclients + 1; i < globals.num_edicts; i++, e++) {
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && (e->freetime < 2_sec || level.time - e->freetime > 500_ms)) {
			G_InitEdict(e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.Com_Error("ED_Alloc: no free edicts");

	globals.num_edicts++;
	G_InitEdict(e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
THINK(G_FreeEdict) (edict_t *ed) -> void {
	// already freed
	if (!ed->inuse)
		return;

	gi.unlinkentity(ed); // unlink from world

	if ((ed - g_edicts) <= (ptrdiff_t)(game.maxclients + BODY_QUEUE_SIZE)) {
#ifdef _DEBUG
		gi.Com_Print("tried to free special edict\n");
#endif
		return;
	}

	gi.Bot_UnRegisterEdict(ed);

	int32_t id = ed->spawn_count + 1;
	memset(ed, 0, sizeof(*ed));
	ed->s.number = ed - g_edicts;
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
	ed->spawn_count = id;
	ed->sv.init = false;
}

BoxEdictsResult_t G_TouchTriggers_BoxFilter(edict_t *hit, void *) {
	if (!hit->touch)
		return BoxEdictsResult_t::Skip;

	return BoxEdictsResult_t::Keep;
}

/*
============
G_TouchTriggers

============
*/
void G_TouchTriggers(edict_t *ent) {
	int		 i, num;
	static edict_t *touch[MAX_EDICTS];
	edict_t *hit;

/*freeze*/
	if (freeze->integer && ent->client && ent->client->frozen);
	else
/*freeze*/
			// dead things don't activate triggers!
		if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
			return;

	num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_TRIGGERS, G_TouchTriggers_BoxFilter, nullptr);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i = 0; i < num; i++) {
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;
		hit->touch(hit, ent, null_trace, true);
	}
}

// [Paril-KEX] scan for projectiles between our movement positions
// to see if we need to collide against them
void G_TouchProjectiles(edict_t *ent, vec3_t previous_origin) {
	struct skipped_projectile {
		edict_t *projectile;
		int32_t		spawn_count;
	};
	// a bit ugly, but we'll store projectiles we are ignoring here.
	static std::vector<skipped_projectile> skipped;

	while (true) {
		trace_t tr = gi.trace(previous_origin, ent->mins, ent->maxs, ent->s.origin, ent, ent->clipmask | CONTENTS_PROJECTILE);

		if (tr.fraction == 1.0f)
			break;
		else if (!(tr.ent->svflags & SVF_PROJECTILE))
			break;

		// always skip this projectile since certain conditions may cause the projectile
		// to not disappear immediately
		tr.ent->svflags &= ~SVF_PROJECTILE;
		skipped.push_back({ tr.ent, tr.ent->spawn_count });

		// if we're both players and it's coop, allow the projectile to "pass" through
		if (ent->client && tr.ent->owner && tr.ent->owner->client && !G_ShouldPlayersCollide(true))
			continue;

		G_Impact(ent, tr);
	}

	for (auto &skip : skipped)
		if (skip.projectile->inuse && skip.projectile->spawn_count == skip.spawn_count)
			skip.projectile->svflags |= SVF_PROJECTILE;

	skipped.clear();
}

/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.
=================
*/

BoxEdictsResult_t KillBox_BoxFilter(edict_t *hit, void *) {
	if (!hit->solid || !hit->takedamage || hit->solid == SOLID_TRIGGER)
		return BoxEdictsResult_t::Skip;

	return BoxEdictsResult_t::Keep;
}

bool KillBox(edict_t *ent, bool from_spawning, mod_id_t mod, bool bsp_clipping) {
	// don't telefrag as spectator...
	if (ent->movetype == MOVETYPE_NOCLIP)
		return true;

	contents_t mask = CONTENTS_MONSTER | CONTENTS_PLAYER;

	// [Paril-KEX] don't gib other players in coop if we're not colliding
	if (from_spawning && ent->client && coop->integer && !G_ShouldPlayersCollide(false))
		mask &= ~CONTENTS_PLAYER;

	int		 i, num;
	static edict_t *touch[MAX_EDICTS];
	edict_t *hit;

	num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_SOLID, KillBox_BoxFilter, nullptr);

	for (i = 0; i < num; i++) {
		hit = touch[i];

		if (hit == ent)
			continue;
		else if (!hit->inuse || !hit->takedamage || !hit->solid || hit->solid == SOLID_TRIGGER || hit->solid == SOLID_BSP)
			continue;
		else if (hit->client && !(mask & CONTENTS_PLAYER))
			continue;

		if ((ent->solid == SOLID_BSP || (ent->svflags & SVF_HULL)) && bsp_clipping) {
			trace_t clip = gi.clip(ent, hit->s.origin, hit->mins, hit->maxs, hit->s.origin, G_GetClipMask(hit));

			if (clip.fraction == 1.0f)
				continue;
		}

		// [Paril-KEX] don't allow telefragging of friends in coop.
		// the player that is about to be telefragged will have collision
		// disabled until another time.
		if (ent->client && hit->client && coop->integer) {
			hit->clipmask &= ~CONTENTS_PLAYER;
			ent->clipmask &= ~CONTENTS_PLAYER;
			continue;
		}

		T_Damage(hit, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, mod);
	}

	return true; // all clear
}

/*--------------------------------------------------------------------------*/

const char *Teams_TeamName(int team) {
	switch (team) {
	case TEAM_RED:
		return "RED";
	case TEAM_BLUE:
		return "BLUE";
	case TEAM_SPECTATOR:
		return "SPECTATOR";
	case TEAM_FREE:
		return "FREE";
	}
	return "NONE";
}

const char *Teams_OtherTeamName(int team) {
	switch (team) {
	case TEAM_RED:
		return "BLUE";
	case TEAM_BLUE:
		return "RED";
	}
	return "UNKNOWN";
}

int Teams_OtherTeamNum(team_t team) {
	switch (team) {
	case TEAM_RED:
		return TEAM_BLUE;
	case TEAM_BLUE:
		return TEAM_RED;
	}
	return -1; // invalid value
}

constexpr const char *TEAM_RED_SKIN = "ctf_r";
constexpr const char *TEAM_BLUE_SKIN = "ctf_b";

/*
=================
G_AssignPlayerSkin
=================
*/
void G_AssignPlayerSkin(edict_t *ent, const char *s) {
	int	  playernum = ent - g_edicts - 1;
	std::string_view t(s);

	if (size_t i = t.find_first_of('/'); i != std::string_view::npos)
		t = t.substr(0, i + 1);
	else
		t = "male/";

	switch (ent->client->resp.team) {
	case TEAM_RED:
		t = G_Fmt("{}\\{}{}\\default", ent->client->resp.netname, t, TEAM_RED_SKIN);
		break;
	case TEAM_BLUE:
		t = G_Fmt("{}\\{}{}\\default", ent->client->resp.netname, t, TEAM_BLUE_SKIN);
		break;
	default:
		t = G_Fmt("{}\\{}\\default", ent->client->resp.netname, s);
		break;
	}

	gi.configstring(CS_PLAYERSKINS + playernum, t.data());

	//	gi.LocClient_Print(ent, PRINT_HIGH, "$g_assigned_team", ent->client->resp.netname);
}

/*
===================
G_AdjustPlayerScore
===================
*/
void G_AdjustPlayerScore(gclient_t *cl, int32_t offset, bool adjust_team, int32_t team_offset) {
	if (!cl) return;

	if (level.match_state != MS_MATCH_IN_PROGRESS)
		return;

	if (level.intermission_queued)
		return;

	if (offset || team_offset) {
		cl->resp.score += offset;
		CalculateRanks();
	}

	if (adjust_team && team_offset && Teams()) {
		G_AdjustTeamScore(cl->resp.team, team_offset);
	}
}

/*
===================
Horde_AdjustPlayerScore
===================
*/
void Horde_AdjustPlayerScore(gclient_t *cl, int32_t offset) {
	if (!horde->integer) return;
	if (!cl) return;

	if (level.match_state != MS_MATCH_IN_PROGRESS)
		return;

	gi.Com_PrintFmt("monster kill score = {}\n", offset);
	G_AdjustPlayerScore(cl, offset, false, 0);
}

/*
===================
G_SetPlayerScore
===================
*/
void G_SetPlayerScore(gclient_t *cl, int32_t value) {
	if (!cl) return;

	if (level.match_state != MS_MATCH_IN_PROGRESS)
		return;

	if (level.intermission_queued)
		return;

	cl->resp.score = value;
	CalculateRanks();
}


/*
===================
G_AdjustTeamScore
===================
*/
void G_AdjustTeamScore(int team, int32_t offset) {

	if (level.match_state != MS_MATCH_IN_PROGRESS)
		return;

	if (level.intermission_queued)
		return;

	if (team == TEAM_RED)
		level.team_scores[TEAM_RED] += offset;
	else if (team == TEAM_BLUE)
		level.team_scores[TEAM_BLUE] += offset;
	CalculateRanks();
}


/*
===================
G_SetTeamScore
===================
*/
void G_SetTeamScore(int team, int32_t value) {

	if (level.match_state != MS_MATCH_IN_PROGRESS)
		return;

	if (level.intermission_queued)
		return;

	if (team == TEAM_RED)
		level.team_scores[TEAM_RED] = value;
	else if (team == TEAM_BLUE)
		level.team_scores[TEAM_BLUE] = value;
	CalculateRanks();
}


/*
===================
G_PlaceString

Adapted from Quake III
===================
*/
const char *G_PlaceString(int rank) {
	static char	str[64];
	const char *s, *t;

	if (rank & RANK_TIED_FLAG) {
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	} else {
		t = "";
	}

	if (rank == 1) {
		s = "1st";
	} else if (rank == 2) {
		s = "2nd";
	} else if (rank == 3) {
		s = "3rd";
	} else if (rank == 11) {
		s = "11th";
	} else if (rank == 12) {
		s = "12th";
	} else if (rank == 13) {
		s = "13th";
	} else if (rank % 10 == 1) {
		s = G_Fmt("{}st", rank).data();
	} else if (rank % 10 == 2) {
		s = G_Fmt("{}nd", rank).data();
	} else if (rank % 10 == 3) {
		s = G_Fmt("{}rd", rank).data();
	} else {
		s = G_Fmt("{}th", rank).data();
	}
	Q_strlcpy(str, G_Fmt("{}{}", t, s).data(), sizeof(str));
	return str;
}

bool ItemSpawnsEnabled() {
	if (g_no_items->integer)
		return false;
	if (g_instagib->integer || g_nadefest->integer)
		return false;
	if (clanarena->integer)
		return false;
	return true;
}


static void loc_buildboxpoints(vec3_t(&p)[8], const vec3_t &org, const vec3_t &mins, const vec3_t &maxs) {
	p[0] = org + mins;
	p[1] = p[0];
	p[1][0] -= mins[0];
	p[2] = p[0];
	p[2][1] -= mins[1];
	p[3] = p[0];
	p[3][0] -= mins[0];
	p[3][1] -= mins[1];
	p[4] = org + maxs;
	p[5] = p[4];
	p[5][0] -= maxs[0];
	p[6] = p[0];
	p[6][1] -= maxs[1];
	p[7] = p[0];
	p[7][0] -= maxs[0];
	p[7][1] -= maxs[1];
}

bool loc_CanSee(edict_t *targ, edict_t *inflictor) {
	trace_t trace;
	vec3_t	targpoints[8];
	int		i;
	vec3_t	viewpoint;

	// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
		return false; // bmodels not supported

	loc_buildboxpoints(targpoints, targ->s.origin, targ->mins, targ->maxs);

	viewpoint = inflictor->s.origin;
	viewpoint[2] += inflictor->viewheight;

	for (i = 0; i < 8; i++) {
		trace = gi.traceline(viewpoint, targpoints[i], inflictor, CONTENTS_MIST|MASK_WATER|MASK_SOLID);
		if (trace.fraction == 1.0f)
			return true;
	}

	return false;
}

bool Teams() {
	return ctf->integer || teamplay->integer || freeze->integer || clanarena->integer;
}

/*
=================
G_TimeString
=================
*/
const char *G_TimeString(const int64_t msec) {
	int ms = abs(msec);
	int hours, mins, seconds;

	seconds = ms / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	hours = mins / 60;
	mins -= hours * 60;

	if (hours > 0) {
		return G_Fmt("{}{}:{:02}:{:02}", msec < 1000 ? "-" : "", hours, mins, seconds).data();
	} else {
		return G_Fmt("{}{:02}:{:02}", msec < 1000 ? "-" : "", mins, seconds).data();
	}
}
/*
=================
G_TimeStringMs
=================
*/
const char *G_TimeStringMs(const int64_t msec) {
	int hours, mins, seconds, ms = msec;

	seconds = ms / 1000;
	ms -= seconds * 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	hours = mins / 60;
	mins -= hours * 60;

	if (hours > 0) {
		return G_Fmt("{}:{:02}:{:02}.{}", hours, mins, seconds, ms).data();
	} else {
		return G_Fmt("{:02}:{:02}.{}", mins, seconds, ms).data();
	}
}

team_t StringToTeamNum(const char *in) {
	if (!Q_strcasecmp(in, "spectator") || !Q_strcasecmp(in, "s")) {
		return TEAM_SPECTATOR;
	} else if (!Q_strcasecmp(in, "auto") || !Q_strcasecmp(in, "a")) {
		return PickTeam(-1);
	} else if (Teams()) {
		if (!Q_strcasecmp(in, "blue") || !Q_strcasecmp(in, "b"))
			return TEAM_BLUE;
		else if (!Q_strcasecmp(in, "red") || !Q_strcasecmp(in, "r"))
			return TEAM_RED;
	} else {
		if (!Q_strcasecmp(in, "free") || !Q_strcasecmp(in, "f"))
			return TEAM_FREE;
	}
	return TEAM_NONE;
}
