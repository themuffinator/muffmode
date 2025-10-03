// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_utils.c -- misc utility functions for game module

#include "g_local.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <string>

/*
=============
G_Find

Searches all active entities for the next one that validates the given callback.

Searches beginning at the entity after from, or the beginning if nullptr
nullptr will be returned if the end of the list is reached.
=============
*/
gentity_t *G_Find(gentity_t *from, std::function<bool(gentity_t *e)> matcher) {
	if (!from)
		from = g_entities;
	else
		from++;

	for (; from < &g_entities[globals.num_entities]; from++) {
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
gentity_t *findradius(gentity_t *from, const vec3_t &org, float rad) {
	vec3_t eorg;
	int	   j;

	if (!from)
		from = g_entities;
	else
		from++;
	for (; from < &g_entities[globals.num_entities]; from++) {
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

Searches beginning at the entity after from, or the beginning if nullptr
nullptr will be returned if the end of the list is reached.

=============
*/
constexpr size_t MAXCHOICES = 8;

gentity_t *G_PickTarget(const char *targetname) {
	gentity_t	*choice[MAXCHOICES];
	gentity_t	*ent = nullptr;
	int		num_choices = 0;

	if (!targetname) {
		gi.Com_PrintFmt("{}: called with nullptr targetname.\n", __FUNCTION__);
		return nullptr;
	}

	while (1) {
		ent = G_FindByString<&gentity_t::targetname>(ent, targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices) {
		gi.Com_PrintFmt("{}: target {} not found\n", __FUNCTION__, targetname);
		return nullptr;
	}

	return choice[irandom(num_choices)];
}

static THINK(Think_Delay) (gentity_t *ent) -> void {
	G_UseTargets(ent, ent->activator);
	G_FreeEntity(ent);
}

void G_PrintActivationMessage(gentity_t *ent, gentity_t *activator, bool coop_global) {
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

void BroadcastFriendlyMessage(team_t team, const char *msg) {
	for (auto ce : active_clients()) {
		if (!ClientIsPlaying(ce->client) || (Teams() && ce->client->sess.team == team)) {
			gi.LocClient_Print(ce, PRINT_HIGH, G_Fmt("{}{}", ce->client->sess.team != TEAM_SPECTATOR ? "[TEAM]: " : "", msg).data());
		}
	}
}

void BroadcastTeamMessage(team_t team, print_type_t level, const char *msg) {
	for (auto ce : active_clients()) {
		if (ce->client->sess.team != team)
			continue;

		gi.LocClient_Print(ce, level, msg);

	}
}

void G_MonsterKilled(gentity_t *self);

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
void G_UseTargets(gentity_t *ent, gentity_t *activator) {
	gentity_t *t;

	if (!ent)
		return;

	if (IsCombatDisabled())
		return;

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
			gi.Com_PrintFmt("{}: {} with no activator.\n", __FUNCTION__, *t);
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
		while ((t = G_FindByString<&gentity_t::targetname>(t, ent->killtarget))) {
			if (t->teammaster) {
				// if this entity is part of a chain, cleanly remove it
				if (t->flags & FL_TEAMSLAVE) {
					for (gentity_t *master = t->teammaster; master; master = master->teamchain) {
						if (master->teamchain == t) {
							master->teamchain = t->teamchain;
							break;
						}
					}
				}
				// [Paril-KEX] remove teammaster too
				else if (t->flags & FL_TEAMMASTER) {
					t->teammaster->flags &= ~FL_TEAMMASTER;

					gentity_t *new_master = t->teammaster->teamchain;

					if (new_master) {
						new_master->flags |= FL_TEAMMASTER;
						new_master->flags &= ~FL_TEAMSLAVE;

						for (gentity_t *m = new_master; m; m = m->teamchain)
							m->teammaster = new_master;
					}
				}
			}

			// [Paril-KEX] if we killtarget a monster, clean up properly
			if (t->svflags & SVF_MONSTER) {
				if (!t->deadflag && !(t->monsterinfo.aiflags & AI_DO_NOT_COUNT) && !(t->spawnflags & SPAWNFLAG_MONSTER_DEAD))
					G_MonsterKilled(t);
			}

			G_FreeEntity(t);

			if (!ent->inuse) {
				gi.Com_PrintFmt("{}: Entity was removed while using killtargets.\n", __FUNCTION__);
				return;
			}
		}
	}

	//
	// fire targets
	//
	if (ent->target) {
		t = nullptr;
		while ((t = G_FindByString<&gentity_t::targetname>(t, ent->target))) {
			// doors fire area portals in a specific way
			if (!Q_strcasecmp(t->classname, "func_areaportal") &&
				(!Q_strcasecmp(ent->classname, "func_door") || !Q_strcasecmp(ent->classname, "func_door_rotating")
					|| !Q_strcasecmp(ent->classname, "func_door_secret") || !Q_strcasecmp(ent->classname, "func_water")))
				continue;

			if (t == ent) {
				gi.Com_PrintFmt("{}: WARNING: Entity used itself.\n", __FUNCTION__);
			} else {
				if (t->use)
					t->use(t, ent, activator);
			}
			if (!ent->inuse) {
				gi.Com_PrintFmt("{}: Entity was removed while using targets.\n", __FUNCTION__);
				return;
			}
		}
	}
}

/*
===============
G_SetMovedir
===============
*/
void G_SetMovedir(vec3_t &angles, vec3_t &movedir) {
	static vec3_t VEC_UP		= { 0, -1, 0 };
	static vec3_t MOVEDIR_UP	= { 0, 0, 1 };
	static vec3_t VEC_DOWN		= { 0, -2, 0 };
	static vec3_t MOVEDIR_DOWN	= { 0, 0, -1 };

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

void G_InitGentity(gentity_t *e) {
	// FIXME -
	//   this fixes a bug somewhere that is setting "nextthink" for an entity that has
	//   already been released. nextthink is being set to FRAME_TIME_S after level.time,
	//   since freetime = nextthink - FRAME_TIME_S
	if (e->nextthink)
		e->nextthink = 0_ms;

	e->inuse = true;
	e->sv.init = false;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_entities;

	// do this before calling the spawn function so it can be overridden.
	e->gravityVector = { 0.0, 0.0, -1.0 };
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_Spawn() {
	gentity_t *e = &g_entities[game.maxclients + 1];
	size_t i;

	for (i = game.maxclients + 1; i < globals.num_entities; i++, e++) {
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && (e->freetime < 2_sec || level.time - e->freetime > 500_ms)) {
			G_InitGentity(e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.Com_ErrorFmt("{}: no free entities.", __FUNCTION__);

	globals.num_entities++;
	G_InitGentity(e);
	//gi.Com_PrintFmt("{}: total:{}\n", __FUNCTION__, i);
	return e;
}

/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
THINK(G_FreeEntity) (gentity_t *ed) -> void {
	// already freed
	if (!ed->inuse)
		return;

	gi.unlinkentity(ed); // unlink from world

	if ((ed - g_entities) <= (ptrdiff_t)(game.maxclients + BODY_QUEUE_SIZE)) {
#ifdef _DEBUG
		gi.Com_Print("Tried to free special entity.\n");
#endif
		return;
	}
	//gi.Com_PrintFmt("{}: removing {}\n", __FUNCTION__, *ed);

	gi.Bot_UnRegisterEntity(ed);

	int32_t id = ed->spawn_count + 1;
	memset(ed, 0, sizeof(*ed));
	ed->s.number = ed - g_entities;
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
	ed->spawn_count = id;
	ed->sv.init = false;
}

BoxEntitiesResult_t G_TouchTriggers_BoxFilter(gentity_t *hit, void *) {
	if (!hit->touch)
		return BoxEntitiesResult_t::Skip;

	return BoxEntitiesResult_t::Keep;
}

/*
============
G_TouchTriggers

============
*/
void G_TouchTriggers(gentity_t *ent) {
	int				num;
	static gentity_t	*touch[MAX_ENTITIES];
	gentity_t			*hit;

	if (ent->client && ent->client->eliminated);
	else
		// dead things don't activate triggers!
		if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
			return;

	num = gi.BoxEntities(ent->absmin, ent->absmax, touch, MAX_ENTITIES, AREA_TRIGGERS, G_TouchTriggers_BoxFilter, nullptr);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (size_t i = 0; i < num; i++) {
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;
		if (ent->movetype == MOVETYPE_FREECAM)
			if (!strstr(hit->classname, "teleport"))
				continue;

		hit->touch(hit, ent, null_trace, true);
	}
}

// [Paril-KEX] scan for projectiles between our movement positions
// to see if we need to collide against them
void G_TouchProjectiles(gentity_t *ent, vec3_t previous_origin) {
	struct skipped_projectile {
		gentity_t *projectile;
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

BoxEntitiesResult_t KillBox_BoxFilter(gentity_t *hit, void *) {
	if (!hit->solid || !hit->takedamage || hit->solid == SOLID_TRIGGER)
		return BoxEntitiesResult_t::Skip;

	return BoxEntitiesResult_t::Keep;
}

bool KillBox(gentity_t *ent, bool from_spawning, mod_id_t mod, bool bsp_clipping) {
	// don't telefrag as spectator or noclip player...
	if (ent->movetype == MOVETYPE_NOCLIP || ent->movetype == MOVETYPE_FREECAM)
		return true;

	contents_t mask = CONTENTS_MONSTER | CONTENTS_PLAYER;

	// [Paril-KEX] don't gib other players in coop if we're not colliding
	if (from_spawning && ent->client && InCoopStyle() && !G_ShouldPlayersCollide(false))
		mask &= ~CONTENTS_PLAYER;

	int		 i, num;
	static gentity_t *touch[MAX_ENTITIES];
	gentity_t *hit;

	num = gi.BoxEntities(ent->absmin, ent->absmax, touch, MAX_ENTITIES, AREA_SOLID, KillBox_BoxFilter, nullptr);

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
		if (ent->client && hit->client && InCoopStyle()) {
			hit->clipmask &= ~CONTENTS_PLAYER;
			ent->clipmask &= ~CONTENTS_PLAYER;
			continue;
		}

		T_Damage(hit, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, mod);
	}

	return true; // all clear
}

/*--------------------------------------------------------------------------*/

const char *Teams_TeamName(team_t team) {
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

const char *Teams_OtherTeamName(team_t team) {
	switch (team) {
	case TEAM_RED:
		return "BLUE";
	case TEAM_BLUE:
		return "RED";
	}
	return "UNKNOWN";
}

team_t Teams_OtherTeam(team_t team) {
	switch (team) {
	case TEAM_RED:
		return TEAM_BLUE;
	case TEAM_BLUE:
		return TEAM_RED;
	}
	return TEAM_SPECTATOR; // invalid value
}

constexpr const char *TEAM_RED_SKIN = "ctf_r";
constexpr const char *TEAM_BLUE_SKIN = "ctf_b";

/*
=================
G_AssignPlayerSkin
=================
*/
void G_AssignPlayerSkin(gentity_t *ent, const char *s) {
	int	  playernum = ent - g_entities - 1;
	std::string_view t(s);

	if (size_t i = t.find_first_of('/'); i != std::string_view::npos)
		t = t.substr(0, i + 1);
	else
		t = "male/";

	switch (ent->client->sess.team) {
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

	if (IsScoringDisabled())
		return;

	if (level.intermission_queued)
		return;

	if (adjust_team && team_offset)
		G_AdjustTeamScore(cl->sess.team, team_offset);

	if (offset || team_offset) {
		cl->resp.score += offset;
		CalculateRanks();
	}
}

/*
===================
Horde_AdjustPlayerScore
===================
*/
void Horde_AdjustPlayerScore(gclient_t *cl, int32_t offset) {
	if (notGT(GT_HORDE)) return;
	if (!cl || !cl->pers.connected) return;

	if (IsScoringDisabled())
		return;

	G_AdjustPlayerScore(cl, offset, false, 0);
}

/*
===================
G_SetPlayerScore
===================
*/
void G_SetPlayerScore(gclient_t *cl, int32_t value) {
	if (!cl) return;

	if (IsScoringDisabled())
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
void G_AdjustTeamScore(team_t team, int32_t offset) {
	if (IsScoringDisabled())
		return;

	if (!Teams() || GT(GT_RR))
		return;

	if (team == TEAM_RED)
		level.team_scores[TEAM_RED] += offset;
	else if (team == TEAM_BLUE)
		level.team_scores[TEAM_BLUE] += offset;
	else return;
	CalculateRanks();
}

/*
===================
G_SetTeamScore
===================
*/
void G_SetTeamScore(team_t team, int32_t value) {
	if (IsScoringDisabled())
		return;

	if (level.intermission_queued)
		return;

	if (!Teams() || GT(GT_RR))
		return;

	if (team == TEAM_RED)
		level.team_scores[TEAM_RED] = value;
	else if (team == TEAM_BLUE)
		level.team_scores[TEAM_BLUE] = value;
	else return;
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
	if (GTF(GTF_ARENA))
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

bool loc_CanSee(gentity_t *targ, gentity_t *inflictor) {
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
	return GTF(GTF_TEAMS);
	//return GT(GT_CTF) || GT(GT_TDM) || GT(GT_FREEZE) || GT(GT_CA) || GT(GT_STRIKE) || GT(GT_RR);
}

/*
=================
G_TimeString
=================
*/
const char *G_TimeString(const int msec, bool state) {
        if (state) {
                if (Match_IsPreCountdown(level.match_state))
			return "WARMUP";

		if (level.intermission_queued || level.intermission_time)
			return "MATCH END";
	}

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
const char *G_TimeStringMs(const int msec, bool state) {
        if (state) {
                if (Match_IsPreCountdown(level.match_state))
			return "WARMUP";

		if (level.intermission_queued || level.intermission_time)
			return "MATCH END";
	}

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

bool InAMatch() {
	if (!deathmatch->integer)
		return false;
	if (level.intermission_queued)
		return false;
	if (Match_IsOngoing(level.match_state))
		return true;

	return false;
}

bool IsCombatDisabled() {
	if (!deathmatch->integer)
		return false;
	if (level.intermission_queued)
		return true;
	if (level.intermission_time)
		return true;
	if (Match_IsCountdown(level.match_state))
		return true;
	if (GTF(GTF_ROUNDS) && Match_IsOngoing(level.match_state)) {
		// added round ended to allow gibbing etc. at end of rounds
		// scoring to be explicitly disabled during this time
		if (level.round_state == roundst_t::ROUND_COUNTDOWN && (notGT(GT_HORDE)))
			return true;
	}
	return false;
}

bool IsPickupsDisabled() {
	if (!deathmatch->integer)
		return false;
	if (level.intermission_queued)
		return true;
	if (level.intermission_time)
		return true;
	if (Match_IsCountdown(level.match_state))
		return true;
	return false;
}

bool IsScoringDisabled() {
	if (!deathmatch->integer)
		return true;
	if (!Match_IsOngoing(level.match_state))
		return true;
	if (level.intermission_time)
		return true;
	if (GTF(GTF_ROUNDS) && level.round_state != roundst_t::ROUND_IN_PROGRESS)
		return true;
	return false;
}

namespace {

constexpr size_t GT_MAX_COMMAND_ALIASES = 5;

constexpr std::array<std::array<const char *, GT_MAX_COMMAND_ALIASES>, static_cast<size_t>(gametype_t::GT_NUM_GAMETYPES)> gt_command_aliases = {{
        /* GT_NONE */      std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "campaign", "cmp", nullptr, nullptr, nullptr },
        /* GT_FFA */       std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "ffa", "dm", "deathmatch", nullptr, nullptr },
        /* GT_DUEL */      std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "duel", "1v1", "tourney", nullptr, nullptr },
        /* GT_TDM */       std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "tdm", "teamdm", "teamdeathmatch", nullptr, nullptr },
        /* GT_CTF */       std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "ctf", "capturetheflag", nullptr, nullptr, nullptr },
        /* GT_CA */        std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "ca", "clanarena", "arena", nullptr, nullptr },
        /* GT_FREEZE */    std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "freeze", "ft", "freezetag", nullptr, nullptr },
        /* GT_STRIKE */    std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "strike", "capturestrike", nullptr, nullptr, nullptr },
        /* GT_RR */        std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "rr", "redrover", nullptr, nullptr, nullptr },
        /* GT_LMS */       std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "lms", "lastmanstanding", "lastman", nullptr, nullptr },
        /* GT_LTS */       std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "lts", "lastteamstanding", "lastteam", nullptr, nullptr },
        /* GT_HORDE */     std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "horde", nullptr, nullptr, nullptr, nullptr },
        /* GT_BALL */      std::array<const char *, GT_MAX_COMMAND_ALIASES>{ "ball", "proball", nullptr, nullptr, nullptr },
}};

bool GT_StringEquals(const char *lhs, const char *rhs) {
        if (!lhs || !rhs)
                return false;
        return !Q_strcasecmp(lhs, rhs);
}

} // namespace

gametype_t GT_IndexFromString(const char *in) {
        if (!in)
                return gametype_t::GT_NONE;

        while (*in && isspace(*in))
                in++;

        if (!*in)
                return gametype_t::GT_NONE;

        char *end = nullptr;
        long value = strtol(in, &end, 10);

        if (end && *end == '\0') {
                if (value >= static_cast<long>(gametype_t::GT_FIRST) && value <= static_cast<long>(gametype_t::GT_LAST))
                        return static_cast<gametype_t>(value);
                return gametype_t::GT_NONE;
        }

        for (size_t i = 0; i < static_cast<size_t>(gametype_t::GT_NUM_GAMETYPES); i++) {
                if (GT_StringEquals(in, gt_short_name[i]))
                        return static_cast<gametype_t>(i);
                if (GT_StringEquals(in, gt_short_name_upper[i]))
                        return static_cast<gametype_t>(i);
                if (GT_StringEquals(in, gt_long_name[i]))
                        return static_cast<gametype_t>(i);

                for (const char *alias : gt_command_aliases[i]) {
                        if (GT_StringEquals(in, alias))
                                return static_cast<gametype_t>(i);
                }
        }

        return gametype_t::GT_NONE;
}

const char *GT_CommandName(gametype_t gt) {
        size_t index = static_cast<size_t>(gt);
        if (index >= static_cast<size_t>(gametype_t::GT_NUM_GAMETYPES))
                return nullptr;

        for (const char *alias : gt_command_aliases[index]) {
                if (alias && alias[0])
                        return alias;
        }

        return gt_short_name[index];
}

const char *GT_CallvoteList() {
        static std::string buffer;

        if (buffer.empty()) {
                for (int i = static_cast<int>(gametype_t::GT_FIRST); i <= static_cast<int>(gametype_t::GT_LAST); i++) {
                        const char *name = GT_CommandName(static_cast<gametype_t>(i));
                        if (!name || !name[0])
                                continue;

                        if (!buffer.empty())
                                buffer += '|';

                        buffer += name;
                }
        }

        return buffer.c_str();
}

const char *GT_CallvoteArgs() {
        static std::string buffer;

        if (buffer.empty()) {
                buffer = "<";
                buffer += GT_CallvoteList();
                buffer += ">";
        }

        return buffer.c_str();
}

void BroadcastReadyReminderMessage() {
	for (auto ec : active_players()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		if (ec->client->sess.is_a_bot)
			continue;
		if (ec->client->resp.ready)
			continue;
		gi.LocCenter_Print(ec, "%bind:+wheel2:Use Compass to toggle your ready status.%MATCH IS IN WARMUP\nYou are NOT ready.");
	}
}

void TeleportPlayerToRandomSpawnPoint(gentity_t *ent, bool fx) {
	bool	valid_spawn = false;
	vec3_t	spawn_origin, spawn_angles;
	bool	is_landmark = false;

	valid_spawn = SelectSpawnPoint(ent, spawn_origin, spawn_angles, true, is_landmark);

	if (!valid_spawn)
		return;

	TeleportPlayer(ent, spawn_origin, spawn_angles);

	ent->s.event = fx ? EV_PLAYER_TELEPORT : EV_OTHER_TELEPORT;
	//other->s.event = fx ? EV_PLAYER_TELEPORT : EV_OTHER_TELEPORT;
}

bool InCoopStyle() {
        return coop->integer || GT(GT_HORDE);
}

bool G_GametypeUsesLives() {
        if (InCoopStyle() && g_coop_enable_lives->integer)
                return true;

        if (GT(GT_LMS))
                return true;

        if (GT(GT_LTS))
                return true;

        return false;
}

int G_GametypeInitialLives() {
        if (InCoopStyle() && g_coop_enable_lives->integer)
                return std::max(0, g_coop_num_lives->integer + 1);

        if (GT(GT_LMS))
                return std::max(1, g_lms_num_lives->integer);

        if (GT(GT_LTS))
                return std::max(0, g_lts_num_lives->integer + 1);

        return 0;
}

bool G_GametypeUsesSquadRespawn() {
        if (InCoopStyle())
                return g_coop_squad_respawn->integer != 0;

        if (GT(GT_LTS))
                return true;

        return false;
}

bool G_GametypeEliminationHUDActive() {
        if (InCoopStyle() && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer))
                return true;

        if (GT(GT_LMS) || GT(GT_LTS))
                return true;

        return false;
}

/*
=================
ClientEntFromString
=================
*/
gentity_t *ClientEntFromString(const char *in) {
	// check by nick first
	for (auto ec : active_clients())
		if (!strcmp(in, ec->client->resp.netname))
			return ec;

	// otherwise check client num
	uint32_t num = strtoul(in, nullptr, 10);
	if (num >= 0 && num < game.maxclients)
		return &g_entities[&game.clients[num] - game.clients + 1];

	return nullptr;
}

/*
=================
RS_IndexFromString
=================
*/
ruleset_t RS_IndexFromString(const char *in) {
	for (size_t i = 1; i < (int)RS_NUM_RULESETS; i++) {
		if (!strcmp(in, rs_short_name[i]))
			return (ruleset_t)i;
		if (!strcmp(in, rs_long_name[i]))
			return (ruleset_t)i;
	}
	return ruleset_t::RS_NONE;
}

void TeleporterVelocity(gentity_t *ent, gvec3_t angles) {
	if (g_teleporter_freeze->integer) {
		// clear the velocity and hold them in place briefly
		ent->velocity = {};
		ent->client->ps.pmove.pm_time = 160; // hold time
		ent->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;
	} else {
		// preserve velocity and 'spit' them out of destination
		float len = ent->velocity.length();

		ent->velocity[2] = 0;
		AngleVectors(angles, ent->velocity, NULL, NULL);
		ent->velocity *= len;
	}
}

static bool MS_Validation(gclient_t *cl, mstats_t index) {
	if (!cl)
		return false;

	if (index <= MSTAT_NONE || index >= MSTAT_TOTAL) {
		gi.Com_PrintFmt("invalid match stat index: {}\n", static_cast<int>(index));
		return false;
	}

	if (!g_matchstats->integer || !Match_IsOngoing(level.match_state))
		return false;

	if (cl->sess.is_a_bot)
		return false;

	return true;
}

int MS_Value(gclient_t *cl, mstats_t index) {
	if (!MS_Validation(cl, index))
		return 0;

	return cl->resp.mstats[index];
}

void MS_Adjust(gclient_t *cl, mstats_t index, int count) {
	if (!MS_Validation(cl, index))
		return;

	cl->resp.mstats[index] += count;
}

void MS_AdjustDuo(gclient_t *cl, mstats_t index1, mstats_t index2, int count) {
	if (!MS_Validation(cl, index1))
		return;

	cl->resp.mstats[index1] += count;
	cl->resp.mstats[index2] += count;
}

void MS_Set(gclient_t *cl, mstats_t index, int value) {
	if (!MS_Validation(cl, index))
		return;

	cl->resp.mstats[index] = value;
}

const char *stime() {
	struct tm *ltime;
	time_t gmtime;

	time(&gmtime);
	ltime = localtime(&gmtime);

	const char *s;
	s = G_Fmt("{}{:02}{:02}{:02}{:02}{:02}",
		1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec
	).data();

	return s;
}

void AnnouncerSound(gentity_t *ent, const char *announcer_sound, const char *backup_sound, bool use_backup) {
	for (auto ec : active_clients()) {
		if (ent == world || ent == ec || (!ClientIsPlaying(ec->client) && ec->client->follow_target == ent)) {
			if (ec->client->sess.is_a_bot)
				continue;
			if (!ec->client->sess.pc.use_expanded || (announcer_sound == nullptr && use_backup)) {
				if (backup_sound)
					gi.local_sound(ec, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex(backup_sound), 1, ATTN_NONE, 0);
				continue;
			}
			//gi.local_sound(ec, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex(announcer_sound), 1, ATTN_NONE, 0);
			
			if (ec->client->sess.pc.use_expanded && announcer_sound)
				gi.local_sound(ec, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex(G_Fmt("vo_evil/{}.wav", announcer_sound).data()), 1, ATTN_NONE, 0);
		}
	}
	//if (announcer_sound && ent == world)
	//	gi.positioned_sound(world->s.origin, world, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex(G_Fmt("vo_evil/{}.wav", announcer_sound).data()), 1, ATTN_NONE, 0);
}


void QLSound(gentity_t *ent, const char *ql_sound, const char *backup_sound, bool use_backup) {
	for (auto ec : active_clients()) {
		if (ent == world || ent == ec || (!ClientIsPlaying(ec->client) && ec->client->follow_target == ent)) {
			if (ec->client->sess.is_a_bot)
				continue;
			if (!ec->client->sess.pc.use_expanded || (ql_sound == nullptr && use_backup)) {
				if (backup_sound)
					gi.local_sound(ec, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex(backup_sound), 1, ATTN_NONE, 0);
				continue;
			}
			//gi.local_sound(ec, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex(ql_sound), 1, ATTN_NONE, 0);
			
			if (ec->client->sess.pc.use_expanded && ql_sound)
				gi.local_sound(ec, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex(G_Fmt("{}.wav", ql_sound).data()), 1, ATTN_NONE, 0);
		}
	}
}

void G_StuffCmd(gentity_t *e, const char *fmt, ...) {
	va_list		argptr;
	char		text[512];

	if (e && !e->client->pers.connected)
		gi.Com_ErrorFmt("{}: Bad client %d for '%s'", __FUNCTION__, (int)(e - g_entities - 1), fmt);

	va_start(argptr, fmt);
	vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);
	text[sizeof(text) - 1] = 0;

	gi.WriteByte(svc_stufftext);
	gi.WriteString(text);

	if (e)
		gi.unicast(e, true);
	else
		gi.multicast(vec3_origin, MULTICAST_ALL, true);
}
