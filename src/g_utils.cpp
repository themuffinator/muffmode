// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_utils.c -- misc utility functions for game module

#include "g_local.h"

/*
=============
FindEntity

Searches all active entities for the next one that validates the given callback.

Searches beginning at the entity after from, or the beginning if nullptr
nullptr will be returned if the end of the list is reached.
=============
*/
gentity_t *FindEntity(gentity_t *from, std::function<bool(gentity_t *e)> matcher) {
	if (!from)
		from = g_entities;
	else
		from++;

	for (; from < &g_entities[globals.num_entities]; from++) {
		if (!from->inUse)
			continue;
		if (matcher(from))
			return from;
	}

	return nullptr;
}

/*
=================
FindRadius

Returns entities that have origins within a spherical area

FindRadius (origin, radius)
=================
*/
gentity_t *FindRadius(gentity_t *from, const vec3_t &org, float rad) {
	vec3_t eorg;
	int	   j;

	if (!from)
		from = g_entities;
	else
		from++;
	for (; from < &g_entities[globals.num_entities]; from++) {
		if (!from->inUse)
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
PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs in the structure.

Searches beginning at the entity after from, or the beginning if nullptr
nullptr will be returned if the end of the list is reached.

=============
*/
constexpr size_t MAXCHOICES = 8;

gentity_t *PickTarget(const char *targetname) {
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
	UseTargets(ent, ent->activator);
	FreeEntity(ent);
}

void PrintActivationMessage(gentity_t *ent, gentity_t *activator, bool coop_global) {
	//
	// print the message
	//
	if ((ent->message) && !(activator->svFlags & SVF_MONSTER)) {
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
UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void UseTargets(gentity_t *ent, gentity_t *activator) {
	gentity_t *t;

	if (!ent)
		return;

	if (CombatIsDisabled())
		return;

	//
	// check for a delay
	//
	if (ent->delay) {
		// create a temp object to fire at a later time
		t = Spawn();
		t->className = "DelayedUse";
		t->nextThink = level.time + gtime_t::from_sec(ent->delay);
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
	PrintActivationMessage(ent, activator, true);

	//
	// kill killtargets
	//
	if (ent->killtarget) {
		t = nullptr;
		while ((t = G_FindByString<&gentity_t::targetname>(t, ent->killtarget))) {
			if (t->teamMaster) {
				// if this entity is part of a chain, cleanly remove it
				if (t->flags & FL_TEAMSLAVE) {
					for (gentity_t *master = t->teamMaster; master; master = master->teamChain) {
						if (master->teamChain == t) {
							master->teamChain = t->teamChain;
							break;
						}
					}
				}
				// [Paril-KEX] remove teamMaster too
				else if (t->flags & FL_TEAMMASTER) {
					t->teamMaster->flags &= ~FL_TEAMMASTER;

					gentity_t *new_master = t->teamMaster->teamChain;

					if (new_master) {
						new_master->flags |= FL_TEAMMASTER;
						new_master->flags &= ~FL_TEAMSLAVE;

						for (gentity_t *m = new_master; m; m = m->teamChain)
							m->teamMaster = new_master;
					}
				}
			}

			// [Paril-KEX] if we killtarget a monster, clean up properly
			if (t->svFlags & SVF_MONSTER) {
				if (!t->deadFlag && !(t->monsterInfo.aiflags & AI_DO_NOT_COUNT) && !(t->spawnflags & SPAWNFLAG_MONSTER_DEAD))
					G_MonsterKilled(t);
			}

			FreeEntity(t);

			if (!ent->inUse) {
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
			if (!Q_strcasecmp(t->className, "func_areaportal") &&
				(!Q_strcasecmp(ent->className, "func_door") || !Q_strcasecmp(ent->className, "func_door_rotating")
					|| !Q_strcasecmp(ent->className, "func_door_secret") || !Q_strcasecmp(ent->className, "func_water")))
				continue;

			if (t == ent) {
				gi.Com_PrintFmt("{}: WARNING: Entity used itself.\n", __FUNCTION__);
			} else {
				if (t->use)
					t->use(t, ent, activator);
			}
			if (!ent->inUse) {
				gi.Com_PrintFmt("{}: Entity was removed while using targets.\n", __FUNCTION__);
				return;
			}
		}
	}
}

/*
===============
SetMoveDir
===============
*/
void SetMoveDir(vec3_t &angles, vec3_t &moveDir) {
	static vec3_t VEC_UP		= { 0, -1, 0 };
	static vec3_t MOVEDIR_UP	= { 0, 0, 1 };
	static vec3_t VEC_DOWN		= { 0, -2, 0 };
	static vec3_t MOVEDIR_DOWN	= { 0, 0, -1 };

	if (angles == VEC_UP) {
		moveDir = MOVEDIR_UP;
	} else if (angles == VEC_DOWN) {
		moveDir = MOVEDIR_DOWN;
	} else {
		AngleVectors(angles, moveDir, nullptr, nullptr);
	}

	angles = {};
}

/*
===============
CopyString
===============
*/
char *CopyString(const char *in, int32_t tag) {
	if (!in)
		return nullptr;
	const size_t amt = strlen(in) + 1;
	char *const out = static_cast<char *>(gi.TagMalloc(amt, tag));
	Q_strlcpy(out, in, amt);
	return out;
}

void InitGEntity(gentity_t *e) {
	// FIXME -
	//   this fixes a bug somewhere that is setting "nextThink" for an entity that has
	//   already been released. nextThink is being set to FRAME_TIME_S after level.time,
	//   since freeTime = nextThink - FRAME_TIME_S
	if (e->nextThink)
		e->nextThink = 0_ms;

	e->inUse = true;
	e->sv.init = false;
	e->className = "noClass";
	e->gravity = 1.0;
	e->s.number = e - g_entities;

	// do this before calling the spawn function so it can be overridden.
	e->gravityVector = { 0.0, 0.0, -1.0 };
}

/*
=================
Spawn

Either finds a free entity, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *Spawn() {
	gentity_t *e = &g_entities[game.maxclients + 1];
	size_t i;

	for (i = game.maxclients + 1; i < globals.num_entities; i++, e++) {
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inUse && (e->freeTime < 2_sec || level.time - e->freeTime > 500_ms)) {
			InitGEntity(e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.Com_ErrorFmt("{}: no free entities.", __FUNCTION__);

	globals.num_entities++;
	InitGEntity(e);
	//gi.Com_PrintFmt("{}: total:{}\n", __FUNCTION__, i);
	return e;
}

/*
=================
FreeEntity

Marks the entity as free
=================
*/
THINK(FreeEntity) (gentity_t *ed) -> void {
	// already freed
	if (!ed->inUse)
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
	ed->className = "freed";
	ed->freeTime = level.time;
	ed->inUse = false;
	ed->spawn_count = id;
	ed->sv.init = false;
}

/*
============
TouchTriggers
============
*/

static BoxEntitiesResult_t TouchTriggers_BoxFilter(gentity_t *hit, void *) {
	if (!hit->touch)
		return BoxEntitiesResult_t::Skip;

	return BoxEntitiesResult_t::Keep;
}

void TouchTriggers(gentity_t *ent) {
	int				num;
	static gentity_t	*touch[MAX_ENTITIES];
	gentity_t			*hit;

	if (ent->client && ent->client->eliminated);
	else
		// dead things don't activate triggers!
		if ((ent->client || (ent->svFlags & SVF_MONSTER)) && (ent->health <= 0))
			return;

	num = gi.BoxEntities(ent->absMin, ent->absMax, touch, MAX_ENTITIES, AREA_TRIGGERS, TouchTriggers_BoxFilter, nullptr);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (size_t i = 0; i < num; i++) {
		hit = touch[i];
		if (!hit->inUse)
			continue;
		if (!hit->touch)
			continue;
		if (ent->moveType == MOVETYPE_FREECAM)
			if (!strstr(hit->className, "teleport"))
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
		trace_t tr = gi.trace(previous_origin, ent->mins, ent->maxs, ent->s.origin, ent, ent->clipMask | CONTENTS_PROJECTILE);

		if (tr.fraction == 1.0f)
			break;
		else if (!(tr.ent->svFlags & SVF_PROJECTILE))
			break;

		// always skip this projectile since certain conditions may cause the projectile
		// to not disappear immediately
		tr.ent->svFlags &= ~SVF_PROJECTILE;
		skipped.push_back({ tr.ent, tr.ent->spawn_count });

		// if we're both players and it's coop, allow the projectile to "pass" through
		if (ent->client && tr.ent->owner && tr.ent->owner->client && !G_ShouldPlayersCollide(true))
			continue;

		G_Impact(ent, tr);
	}

	for (auto &skip : skipped)
		if (skip.projectile->inUse && skip.projectile->spawn_count == skip.spawn_count)
			skip.projectile->svFlags |= SVF_PROJECTILE;

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
	if (!hit->solid || !hit->takeDamage || hit->solid == SOLID_TRIGGER)
		return BoxEntitiesResult_t::Skip;

	return BoxEntitiesResult_t::Keep;
}

bool KillBox(gentity_t *ent, bool from_spawning, mod_id_t mod, bool bsp_clipping) {
	// don't telefrag as spectator or noclip player...
	if (ent->moveType == MOVETYPE_NOCLIP || ent->moveType == MOVETYPE_FREECAM)
		return true;

	contents_t mask = CONTENTS_MONSTER | CONTENTS_PLAYER;

	// [Paril-KEX] don't gib other players in coop if we're not colliding
	if (from_spawning && ent->client && CooperativeModeOn() && !G_ShouldPlayersCollide(false))
		mask &= ~CONTENTS_PLAYER;

	int		 i, num;
	static gentity_t *touch[MAX_ENTITIES];
	gentity_t *hit;

	num = gi.BoxEntities(ent->absMin, ent->absMax, touch, MAX_ENTITIES, AREA_SOLID, KillBox_BoxFilter, nullptr);

	for (i = 0; i < num; i++) {
		hit = touch[i];

		if (hit == ent)
			continue;
		else if (!hit->inUse || !hit->takeDamage || !hit->solid || hit->solid == SOLID_TRIGGER || hit->solid == SOLID_BSP)
			continue;
		else if (hit->client && !(mask & CONTENTS_PLAYER))
			continue;

		if ((ent->solid == SOLID_BSP || (ent->svFlags & SVF_HULL)) && bsp_clipping) {
			trace_t clip = gi.clip(ent, hit->s.origin, hit->mins, hit->maxs, hit->s.origin, G_GetClipMask(hit));

			if (clip.fraction == 1.0f)
				continue;
		}

		// [Paril-KEX] don't allow telefragging of friends in coop.
		// the player that is about to be telefragged will have collision
		// disabled until another time.
		if (ent->client && hit->client && CooperativeModeOn()) {
			hit->clipMask &= ~CONTENTS_PLAYER;
			ent->clipMask &= ~CONTENTS_PLAYER;
			continue;
		}

		Damage(hit, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, mod);
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

/*
=================
CleanSkinName
=================
*/
static std::string CleanSkinName(const std::string &in) {
	std::string out;
	for (char c : in) {
		if (isalnum(c) || c == '_' || c == '-') {
			out += c;
		}
	}
	return out.empty() ? "male" : out;
}

/*
=================
AssignPlayerSkin
=================
*/

constexpr const char *TEAM_RED_SKIN = "ctf_r";
constexpr const char *TEAM_BLUE_SKIN = "ctf_b";

void AssignPlayerSkin(gentity_t *ent, const std::string &skin) {
	if (!ent || !ent->client)
		return;

	int playernum = ent - g_entities - 1;

	// Sanitize the input skin
	std::string cleanSkin = CleanSkinName(skin);

	std::string_view baseSkin(cleanSkin);
	size_t slashPos = baseSkin.find_first_of('/');

	std::string modelPath;
	if (slashPos != std::string_view::npos) {
		modelPath = std::string(baseSkin.substr(0, slashPos + 1));
	} else {
		modelPath = "male/";
	}

	const char *teamColor = nullptr;
	switch (ent->client->sess.team) {
	case TEAM_RED:
		teamColor = "red";
		break;
	case TEAM_BLUE:
		teamColor = "blue";
		break;
	default:
		teamColor = nullptr;
		break;
	}

	std::string finalSkin;
	if (teamColor) {
		finalSkin = G_Fmt("{}\\{}{}\\default", ent->client->sess.netName, modelPath, teamColor);
	} else {
		finalSkin = G_Fmt("{}\\{}\\default", ent->client->sess.netName, cleanSkin);
	}

	gi.configstring(CS_PLAYERSKINS + playernum, finalSkin.c_str());
}

/*
===================
G_AdjustPlayerScore
===================
*/
void G_AdjustPlayerScore(gclient_t *cl, int32_t offset, bool adjust_team, int32_t team_offset) {
	if (!cl) return;

	if (ScoringIsDisabled())
		return;

	if (level.intermissionQueued)
		return;

	if (offset || team_offset) {
		cl->resp.score += offset;
		//gi.Com_PrintFmt("G_AdjustPlayerScore: player: {}\n", offset);

		if (adjust_team && team_offset) {
			//G_AdjustTeamScore(cl->sess.team, team_offset);

			if (Teams() && notGT(GT_RR)) {

				if (cl->sess.team == TEAM_RED)
					level.team_scores[TEAM_RED] += offset;
				else if (cl->sess.team == TEAM_BLUE)
					level.team_scores[TEAM_BLUE] += offset;

				//gi.Com_PrintFmt("G_AdjustPlayerScore: team: {}\n", offset);
			}
		}

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

	if (ScoringIsDisabled())
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

	if (ScoringIsDisabled())
		return;

	if (level.intermissionQueued)
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
	if (ScoringIsDisabled())
		return;

	if (level.intermissionQueued)
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
	if (ScoringIsDisabled())
		return;

	if (level.intermissionQueued)
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
PlaceString
===================
*/
const char *PlaceString(int rank) {
	static char str[64];

	const char *prefix = "";
	if (rank & RANK_TIED_FLAG) {
		rank &= ~RANK_TIED_FLAG;
		prefix = "Tied for ";
	}

	static constexpr const char *const suffixTable[10] = {
		"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"
	};

	const char *suffix = "th";
	int mod100 = rank % 100;

	if (mod100 < 11 || mod100 > 13) {
		suffix = suffixTable[rank % 10];
	}

	snprintf(str, sizeof(str), "%s%d%s", prefix, rank, suffix);
	str[sizeof(str) - 1] = '\0'; // Force null-termination

	return str;
}

/*
=================
ItemSpawnsEnabled
=================
*/
bool ItemSpawnsEnabled() {
	if (g_no_items->integer)
		return false;
	if (g_instagib->integer || g_nadefest->integer)
		return false;
	if (GTF(GTF_ARENA))
		return false;
	return true;
}

/*
=================
LocBuildBoxPoints
=================
*/
static void LocBuildBoxPoints(vec3_t(&p)[8], const vec3_t &org, const vec3_t &mins, const vec3_t &maxs) {
	// Bottom
	p[0] = org + mins;
	p[1] = p[0]; p[1][0] += (maxs[0] - mins[0]);
	p[2] = p[0]; p[2][1] += (maxs[1] - mins[1]);
	p[3] = p[0]; p[3][0] += (maxs[0] - mins[0]); p[3][1] += (maxs[1] - mins[1]);
	// Top
	p[4] = org + maxs;
	p[5] = p[4]; p[5][0] -= (maxs[0] - mins[0]);
	p[6] = p[4]; p[6][1] -= (maxs[1] - mins[1]);
	p[7] = p[4]; p[7][0] -= (maxs[0] - mins[0]); p[7][1] -= (maxs[1] - mins[1]);
}

/*
=================
LocCanSee
=================
*/
bool LocCanSee(gentity_t *targetEnt, gentity_t *sourceEnt) {
	if (!targetEnt || !sourceEnt)
		return false;

	if (targetEnt->moveType == MOVETYPE_PUSH)
		return false; // bmodels not supported

	vec3_t targpoints[8];
	LocBuildBoxPoints(targpoints, targetEnt->s.origin, targetEnt->mins, targetEnt->maxs);

	vec3_t viewpoint = sourceEnt->s.origin;
	viewpoint[2] += sourceEnt->viewHeight;

	for (int i = 0; i < 8; i++) {
		trace_t trace = gi.traceline(viewpoint, targpoints[i], sourceEnt, CONTENTS_MIST | MASK_WATER | MASK_SOLID);
		if (trace.fraction == 1.0f)
			return true; // Early exit if any point is visible
	}

	return false;
}

/*
=================
Teams
=================
*/
bool Teams() {
	return GTF(GTF_TEAMS);
	//return GT(GT_CTF) || GT(GT_TDM) || GT(GT_FREEZE) || GT(GT_CA) || GT(GT_STRIKE) || GT(GT_RR);
}

/*
=================
TimeString
=================
*/
const char *TimeString(int msec, bool showMilliseconds, bool state) {
	static char timeString[32];

	if (state) {
		if (level.match_state < MatchState::MATCH_COUNTDOWN)
			return "WARMUP";
		if (level.intermissionQueued || level.intermissionTime)
			return "MATCH END";
	}

	int absMs = (msec >= 0) ? msec : -msec;
	int totalSeconds = absMs / 1000;
	int milliseconds = absMs % 1000;

	int hours = totalSeconds / 3600;
	int mins = (totalSeconds % 3600) / 60;
	int seconds = totalSeconds % 60;

	if (showMilliseconds) {
		if (hours > 0) {
			snprintf(timeString, sizeof(timeString), "%s%d:%02d:%02d.%03d",
				(msec < 0 ? "-" : ""), hours, mins, seconds, milliseconds);
		} else {
			snprintf(timeString, sizeof(timeString), "%s%02d:%02d.%03d",
				(msec < 0 ? "-" : ""), mins, seconds, milliseconds);
		}
	} else {
		if (hours > 0) {
			snprintf(timeString, sizeof(timeString), "%s%d:%02d:%02d",
				(msec < 0 ? "-" : ""), hours, mins, seconds);
		} else {
			snprintf(timeString, sizeof(timeString), "%s%02d:%02d",
				(msec < 0 ? "-" : ""), mins, seconds);
		}
	}

	timeString[sizeof(timeString) - 1] = '\0'; // Force null-termination
	return timeString;
}

/*
=================
StringToTeamNum
=================
*/
team_t StringToTeamNum(const char *in) {
	struct {
		const char *name;
		team_t team;
	} mappings[] = {
		{ "spectator", team_t::TEAM_SPECTATOR },
		{ "s",         team_t::TEAM_SPECTATOR },
		{ "auto",      team_t::TEAM_NONE }, // special case handled separately
		{ "a",         team_t::TEAM_NONE }, // special case handled separately
		{ "blue",      team_t::TEAM_BLUE },
		{ "b",         team_t::TEAM_BLUE },
		{ "red",       team_t::TEAM_RED },
		{ "r",         team_t::TEAM_RED },
		{ "free",      team_t::TEAM_FREE },
		{ "f",         team_t::TEAM_FREE },
	};

	if (!in || !*in)
		return team_t::TEAM_NONE;

	for (const auto &map : mappings) {
		if (!Q_strcasecmp(in, map.name)) {
			if (map.team == team_t::TEAM_NONE) {
				return PickTeam(-1); // 'auto' special case
			}
			if (!Teams()) {
				// Only allow free-for-all team if not in team mode
				if (map.team == team_t::TEAM_FREE)
					return team_t::TEAM_FREE;
				// Ignore team picks if no teams
				return team_t::TEAM_NONE;
			}
			// Normal team return
			return map.team;
		}
	}

	return team_t::TEAM_NONE;
}

/*
=================
InAMatch
=================
*/
bool InAMatch() {
	if (!deathmatch->integer)
		return false;
	if (level.intermissionQueued)
		return false;
	if (level.match_state == MatchState::MATCH_IN_PROGRESS)
		return true;

	return false;
}

/*
=================
CombatIsDisabled
=================
*/
bool CombatIsDisabled() {
	if (!deathmatch->integer)
		return false;
	if (level.intermissionQueued)
		return true;
	if (level.intermissionTime)
		return true;
	if (level.match_state == MatchState::MATCH_COUNTDOWN)
		return true;
	if (GTF(GTF_ROUNDS) && level.match_state == MatchState::MATCH_IN_PROGRESS) {
		// added round ended to allow gibbing etc. at end of rounds
		// scoring to be explicitly disabled during this time
		if (level.round_state == RoundState::ROUND_COUNTDOWN && (notGT(GT_HORDE)))
			return true;
	}
	return false;
}

/*
=================
ItemPickupsAreDisabled
=================
*/
bool ItemPickupsAreDisabled() {
	if (!deathmatch->integer)
		return false;
	if (level.intermissionQueued)
		return true;
	if (level.intermissionTime)
		return true;
	if (level.match_state == MatchState::MATCH_COUNTDOWN)
		return true;
	return false;
}

/*
=================
ScoringIsDisabled
=================
*/
bool ScoringIsDisabled() {
	if (level.match_state != MatchState::MATCH_IN_PROGRESS)
		return true;
	if (CombatIsDisabled())
		return true;
	if (GTF(GTF_ROUNDS) && level.round_state != RoundState::ROUND_IN_PROGRESS)
		return true;
	return false;
}

/*
=================
GametypeStringToIndex
=================
*/
gametype_t GametypeStringToIndex(const std::string &input) {
	auto to_lower = [](const std::string &str) -> std::string {
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
			return std::tolower(c);
			});
		return result;
		};

	std::string lowered_input = to_lower(input);

	for (size_t i = 0; i < static_cast<size_t>(gametype_t::GT_NUM_GAMETYPES); i++) {
		if (lowered_input == to_lower(gt_short_name_upper[i]))
			return static_cast<gametype_t>(i);
	}

	for (size_t i = 0; i < static_cast<size_t>(gametype_t::GT_NUM_GAMETYPES); i++) {
		if (lowered_input == to_lower(gt_long_name[i]))
			return static_cast<gametype_t>(i);
	}

	return gametype_t::GT_NONE;
}

/*
=================
GametypeIndexToString
=================
*/
std::string GametypeIndexToString(gametype_t gametype) {
	if (gametype != gametype_t::GT_NONE &&
			static_cast<size_t>(gametype) < static_cast<size_t>(gametype_t::GT_NUM_GAMETYPES)) {
		return gt_short_name_upper[static_cast<size_t>(gametype)];
	}
	return "GT_NONE";
}

/*
=================
GametypeOptionList
=================
*/
std::string GametypeOptionList() {
	std::string result = "<";
	for (size_t i = 0; i < gt_short_name.size(); ++i) {
		result += gt_short_name[i];
		if (i != gt_short_name.size() - 1)
			result += "|";
	}
	result += ">";
	return result;
}

/*
=================
BroadcastReadyReminderMessage
=================
*/
void BroadcastReadyReminderMessage() {
	for (auto ec : active_players()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		if (ec->client->sess.is_a_bot)
			continue;
		if (ec->client->resp.ready)
			continue;
		//gi.LocCenter_Print(ec, "%bind:+wheel2:Use Compass to toggle your ready status.%MATCH IS IN WARMUP\nYou are NOT ready.");
		gi.LocCenter_Print(ec, "%bind:+wheel2:$map_item_wheel%Use Compass to Ready.\nMATCH IS IN WARMUP\nYou are NOT ready.");
	}
}

/*
=================
TeleporterVelocity
=================
*/
void TeleporterVelocity(gentity_t *ent, gvec3_t angles) {
	float len = ent->velocity.length();
	ent->velocity[2] = 0;
	AngleVectors(angles, ent->velocity, NULL, NULL);
	ent->velocity *= len;

	ent->client->ps.pmove.pm_time = 160; // hold time
	ent->client->ps.pmove.pm_flags |= PMF_TIME_KNOCKBACK;
}

/*
=================
TeleportPlayer
=================
*/
void TeleportPlayer(gentity_t *player, vec3_t origin, vec3_t angles) {
	Weapon_Grapple_DoReset(player->client);

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(player);

	player->s.origin = origin;
	player->s.old_origin = origin;
	player->s.origin[2] += 10;

	TeleporterVelocity(player, angles);

	// set angles
	player->client->ps.pmove.delta_angles = angles - player->client->resp.cmd_angles;

	player->s.angles = {};
	player->client->ps.viewangles = {};
	player->client->vAngle = {};
	AngleVectors(player->client->vAngle, player->client->vForward, nullptr, nullptr);

	gi.linkentity(player);

	// kill anything at the destination
	KillBox(player, !!player->client);

	// [Paril-KEX] move sphere, if we own it
	if (player->client->ownedSphere) {
		gentity_t *sphere = player->client->ownedSphere;
		sphere->s.origin = player->s.origin;
		sphere->s.origin[2] = player->absMax[2];
		sphere->s.angles[YAW] = player->s.angles[YAW];
		gi.linkentity(sphere);
	}
}

/*
=================
TeleportPlayerToRandomSpawnPoint
=================
*/
void TeleportPlayerToRandomSpawnPoint(gentity_t *ent, bool fx) {
	bool	valid_spawn = false;
	vec3_t	spawn_origin, spawn_angles;
	bool	is_landmark = false;

	valid_spawn = SelectSpawnPoint(ent, spawn_origin, spawn_angles, true, is_landmark);

	if (!valid_spawn)
		return;

	TeleportPlayer(ent, spawn_origin, spawn_angles);

	ent->s.event = fx ? EV_PLAYER_TELEPORT : EV_OTHER_TELEPORT;
}

/*
=================
CooperativeModeOn
=================
*/
bool CooperativeModeOn() {
	return coop->integer || GT(GT_HORDE) || GT(GT_RACE);
}

/*
=================
ClientEntFromString
=================
*/
gentity_t *ClientEntFromString(const char *in) {
	// check by nick first
	for (auto ec : active_clients())
		if (!strcmp(in, ec->client->sess.netName))
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

/*
===============
AnnouncerSound
===============
*/
void AnnouncerSound(gentity_t *announcer, std::string_view soundKey) {
	if (soundKey.empty())
		return;

	// Build the path once and grab its index
	const std::string path = G_Fmt("vo/{}.wav", soundKey).data();
	const int         idx = gi.soundindex(path.c_str());

	// Flag and parameter constants
	constexpr soundchan_t SOUND_FLAGS = static_cast<soundchan_t>(CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX);
	constexpr float       VOLUME = 1.0f;
	constexpr float       ATTENUATION = ATTN_NONE;
	constexpr float       TIME_OFFSET = 0.0f;
	constexpr uint32_t    DUPE_KEY = 0;    // no duplicate‐filtering

	// World‐wide announcer
	if (announcer == world) {
		gi.positioned_sound(
			world->s.origin,
			world,
			SOUND_FLAGS,
			idx,
			VOLUME,
			ATTENUATION,
			TIME_OFFSET
		);
		return;
	}

	// Send to each active client who should hear it
	for (auto *targetEnt : active_clients()) {
		auto *cl = targetEnt->client;
		bool  hear = false;

		if (!ClientIsPlaying(cl)) {
			// spectator only if following this announcer
			hear = (cl->followTarget == announcer);
		} else {
			// player only if it's their own award, and skip bots
			hear = (targetEnt == announcer && !cl->sess.is_a_bot);
		}

		if (!hear) {
			continue;
		}

		gi.local_sound(
			targetEnt,
			SOUND_FLAGS,
			idx,
			VOLUME,
			ATTENUATION,
			TIME_OFFSET,
			DUPE_KEY
		);
	}
}

/*
===============
CreateSpawnPad
===============
*/
void CreateSpawnPad(gentity_t *ent) {
	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 0;
	ent->solid = SOLID_BBOX;
	ent->clipMask |= MASK_SOLID;

	ent->mins = { -32, -32, -24 };
	ent->maxs = { 32, 32, -16 };
	gi.linkentity(ent);
}

/*
===============
LogAccuracyHit
===============
*/
bool LogAccuracyHit(gentity_t *target, gentity_t *attacker) {
	if (!target->takeDamage)
		return false;

	if (target == attacker)
		return false;

	if (!attacker->client)
		return false;
	
	if (deathmatch->integer && !target->client)
		return false;

	if (target->health <= 0)
		return false;

	if (OnSameTeam(target, attacker))
		return false;

	return true;
}

/*
==================
G_LogEvent
==================
*/
void G_LogEvent(std::string str) {
	match_event_t ev;

	ev.time = level.time - level.matchStartTime;
	ev.eventStr = str;
	level.match.eventLog.push_back(std::move(ev));
}

/*
==================
GT_SetLongName
==================
*/
void GT_SetLongName(void) {
	struct {
		cvar_t *cvar;
		const char *prefix;
	} suffixModes[] = {
		{ g_instagib,        "Insta" },
		{ g_vampiric_damage, "Vampiric" },
		{ g_frenzy,          "Frenzy" },
		{ g_nadefest,        "NadeFest" },
		{ g_quadhog,         "Quad Hog" },
	};

	const char *prefix = nullptr;
	const char *base = nullptr;
	bool useShort = false; // flag: use short name if a modifier is active

	if (!deathmatch->integer) {
		base = coop->integer ? "Co-op" : "Single Player";
	} else {
		// Find a game modifier active (e.g., Instagib)
		for (const auto &mod : suffixModes) {
			if (mod.cvar && mod.cvar->integer) {
				prefix = mod.prefix;
				useShort = true;
				break;
			}
		}

		// Base name depends whether a modifier is active
		if (useShort) {
			// Find short base name
			if (g_gametype->integer >= 0 && g_gametype->integer < GT_NUM_GAMETYPES)
				base = gt_short_name_upper[g_gametype->integer];
			else
				base = "Unknown";
		} else {
			// Find full long name
			if (g_gametype->integer >= 0 && g_gametype->integer < GT_NUM_GAMETYPES)
				base = gt_long_name[g_gametype->integer];
			else
				base = "Unknown";
		}
	}

	char longName[MAX_QPATH] = "";

	if (prefix && base) {
		// Exception: InstaGib (not Insta-FFA)
		if (strcmp(base, "FFA") == 0 && strcmp(prefix, "Insta") == 0) {
			Q_strlcpy(longName, "InstaGib", sizeof(longName));
		} else {
			Q_strlcpy(longName, prefix, sizeof(longName));
			Q_strlcat(longName, "-", sizeof(longName));
			Q_strlcat(longName, base, sizeof(longName));
		}
	} else if (base) {
		Q_strlcpy(longName, base, sizeof(longName));
	} else {
		Q_strlcpy(longName, "Unknown Gametype", sizeof(longName));
	}

	if (longName[0])
		Q_strlcpy(level.gametype_name, longName, sizeof(level.gametype_name));
}

/*
=================
HandleLeadChanges

Detect changes in individual player rank
=================
*/
static void HandleLeadChanges() {
	for (auto ec : active_players()) {
		gclient_t *cl = ec->client;
		int newRank = cl->resp.rank;
		int oldRank = cl->resp.oldRank;

		bool newTied = (newRank & RANK_TIED_FLAG);
		bool oldTied = (oldRank & RANK_TIED_FLAG);

		newRank &= ~RANK_TIED_FLAG;
		oldRank &= ~RANK_TIED_FLAG;

		if (newRank == oldRank)
			continue;

		if (newRank == 0) {
			// Now in first place
			if (oldTied != newTied)
				AnnouncerSound(ec, newTied ? "lead_tied" : "lead_taken");

			// Update followers
			for (auto spec : active_clients()) {
				if (!ClientIsPlaying(spec->client) &&
					spec->client->sess.pc.follow_leader &&
					spec->client->followTarget != ec) {
					spec->client->follow_queued_target = ec;
					spec->client->follow_queued_time = level.time;
				}
			}
		} else if (oldRank == 0) {
			// Lost lead
			AnnouncerSound(ec, "lead_lost");
		}
	}
}

/*
=================
HandleTeamLeadChanges

Detect changes in team lead state
=================
*/
static void HandleTeamLeadChanges() {
	int oldRank = 2; // 2 = tied, 0 = red leads, 1 = blue leads
	int newRank = 2;

	if (level.team_old_scores[TEAM_RED] > level.team_old_scores[TEAM_BLUE])
		oldRank = 0;
	else if (level.team_old_scores[TEAM_BLUE] > level.team_old_scores[TEAM_RED])
		oldRank = 1;

	if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE])
		newRank = 0;
	else if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED])
		newRank = 1;

	if (oldRank != newRank) {
		if (oldRank == 2 && newRank != 2) {
			// A team just took the lead
			AnnouncerSound(world, newRank == 0 ? "red_leads" : "blue_leads");
		} else if (oldRank != 2 && newRank == 2) {
			// Now tied
			AnnouncerSound(world, "teams_tied");
		}
	}

	// Update old scores for next comparison
	level.team_old_scores[TEAM_RED] = level.team_scores[TEAM_RED];
	level.team_old_scores[TEAM_BLUE] = level.team_scores[TEAM_BLUE];
}

/*
=============
SortRanks
=============
*/
static int SortRanks(const void *a, const void *b) {
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

	// then sort by score
	if (GT(GT_RACE)) {
		if (ca->resp.score > 0 && (ca->resp.score < cb->resp.score))
			return -1;
		if (cb->resp.score > 0 && (ca->resp.score > cb->resp.score))
			return 1;
	} else {
		if (ca->resp.score > cb->resp.score)
			return -1;
		if (ca->resp.score < cb->resp.score)
			return 1;
	}

	// then sort by time
	if (ca->sess.teamJoinTime < cb->sess.teamJoinTime)
		return -1;
	if (ca->sess.teamJoinTime > cb->sess.teamJoinTime)
		return 1;

	return 0;
}

/*
=============
CalculateRanks
=============
*/
void CalculateRanks() {
	if (level.restarted)
		return;

	bool teams = Teams();

	// Reset counters
	level.num_connected_clients = 0;
	level.num_console_clients = 0;
	level.num_nonspectator_clients = 0;
	level.num_playing_clients = 0;
	level.num_playing_human_clients = 0;
	level.num_eliminated_red = 0;
	level.num_eliminated_blue = 0;
	level.num_living_red = 0;
	level.num_living_blue = 0;
	level.num_playing_red = 0;
	level.num_playing_blue = 0;
	level.num_voting_clients = 0;
	level.follow1 = level.follow2 = -1;

	for (size_t i = 0; i < MAX_CLIENTS; i++)
		level.sorted_clients[i] = -1;

	// Phase 1: Gather active clients
	for (auto ec : active_clients()) {
		gclient_t *cl = ec->client;
		int clientNum = cl - game.clients;

		level.sorted_clients[level.num_connected_clients++] = clientNum;
		if (cl->sess.console)
			level.num_console_clients++;

		if (!ClientIsPlaying(cl)) {
			if (g_allow_spec_vote->integer)
				level.num_voting_clients++;
			continue;
		}

		level.num_nonspectator_clients++;
		level.num_playing_clients++;

		if (!cl->sess.is_a_bot) {
			level.num_playing_human_clients++;
			level.num_voting_clients++;
		}

		// Assign first two follow targets
		if (level.follow1 == -1)
			level.follow1 = clientNum;
		else if (level.follow2 == -1)
			level.follow2 = clientNum;

		if (teams) {
			if (cl->sess.team == TEAM_RED) {
				level.num_playing_red++;
				if (cl->pers.health > 0)
					level.num_living_red++;
				else if (cl->eliminated)
					level.num_eliminated_red++;
			} else {
				level.num_playing_blue++;
				if (cl->pers.health > 0)
					level.num_living_blue++;
				else if (cl->eliminated)
					level.num_eliminated_blue++;
			}
		}
	}

	// Phase 2: Sort
	qsort(level.sorted_clients, level.num_connected_clients, sizeof(level.sorted_clients[0]), SortRanks);

	// Phase 3: Assign ranks
	if (teams && notGT(GT_RR)) {
		// Team-based ranking
		for (size_t i = 0; i < level.num_connected_clients; i++) {
			gclient_t *cl = &game.clients[level.sorted_clients[i]];

			if (level.team_scores[TEAM_RED] == level.team_scores[TEAM_BLUE])
				cl->resp.rank = 2; // tied
			else if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE])
				cl->resp.rank = 0; // red leads
			else
				cl->resp.rank = 1; // blue leads
		}
	} else {
		// Individual ranking
		int last_score = -99999;
		int current_rank = 0;
		for (size_t i = 0; i < level.num_playing_clients; i++) {
			gclient_t *cl = &game.clients[level.sorted_clients[i]];
			cl->resp.oldRank = cl->resp.rank;

			if (cl->resp.score != last_score) {
				current_rank = i;
				cl->resp.rank = current_rank;
			} else {
				cl->resp.rank = current_rank | RANK_TIED_FLAG;
				game.clients[level.sorted_clients[i - 1]].resp.rank = current_rank | RANK_TIED_FLAG;
			}

			last_score = cl->resp.score;
		}
	}

	// Phase 4: Handle "no players" time
	if (!level.num_playing_clients && !level.no_players_time)
		level.no_players_time = level.time;
	else if (level.num_playing_clients)
		level.no_players_time = 0_sec;

	level.warmup_notice_time = level.time;

	// Phase 5: Frag limit warnings (if applicable)
	if (level.match_state == MatchState::MATCH_IN_PROGRESS && GTF(GTF_FRAGS)) {
		if (fraglimit->integer > 3) {
			int lead_score = game.clients[level.sorted_clients[0]].resp.score;
			int score_diff = fraglimit->integer - lead_score;

			if (score_diff <= 3 && !level.frag_warning[score_diff - 1]) {
				AnnouncerSound(world, G_Fmt("{}_frag{}", score_diff, score_diff > 1 ? "s" : "").data());
				level.frag_warning[score_diff - 1] = true;
				CheckDMExitRules();
				return;
			}
		}
	}

	// Phase 6: Lead/tied/lost sounds
	if (level.match_state == MatchState::MATCH_IN_PROGRESS) {
		if (!teams && game.clients[level.sorted_clients[0]].resp.score > 0) {
			HandleLeadChanges();
		} else if (teams && GTF(GTF_FRAGS)) {
			HandleTeamLeadChanges();
		}
	}

	CheckDMExitRules();
}

/*
=============
TimeStamp
=============
*/
std::string TimeStamp() {
	struct tm *ltime;
	time_t gmtime;

	time(&gmtime);
	ltime = localtime(&gmtime);
	time(&gmtime);
	ltime = localtime(&gmtime);

	return G_Fmt("{}-{:02}-{:02} {:02}:{:02}:{:02}", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec).data();
}

/*
=============
FileTimeStamp
=============
*/
std::string FileTimeStamp() {
	struct tm *ltime;
	time_t gmtime;

	time(&gmtime);
	ltime = localtime(&gmtime);
	time(&gmtime);
	ltime = localtime(&gmtime);

	return G_Fmt("{}-{:02}-{:02}_{:02}-{:02}-{:02}", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec).data();
}

/*
=============
DateStamp
=============
*/
std::string DateStamp() {
	struct tm *ltime;
	time_t gmtime;


	time(&gmtime);
	ltime = localtime(&gmtime);
	time(&gmtime);
	ltime = localtime(&gmtime);

	return G_Fmt("{}-{:02}-{:02}", 1900 + ltime->tm_year, ltime->tm_mon + 1, ltime->tm_mday).data();
}

/*
=============
FormatSeconds
=============
*/
std::string FormatSeconds(int seconds) {
	int minutes = seconds / 60;
	seconds = seconds % 60;
	if (minutes > 0)
		return fmt::format("{}m {}s", minutes, seconds);
	else
		return fmt::format("{}s", seconds);
}