// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

void FreeFollower(edict_t *ent) {
	if (!ent)
		return;

	if (!ent->client->follow_target)
		return;

	ent->client->follow_target = nullptr;
	ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);

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

void FreeClientFollowers(edict_t *ent) {
	if (!ent)
		return;

	for (auto ec : active_clients()) {
		if (!ec->client->follow_target)
			continue;
		if (ec->client->follow_target == ent)
			FreeFollower(ec);
	}
}

void UpdateChaseCam(edict_t *ent) {
	vec3_t	o, ownerv, goal;
	edict_t	*targ = ent->client->follow_target;
	vec3_t	forward, right;
	trace_t	trace;
	vec3_t	oldgoal;
	vec3_t	angles;
	
	// is our follow target gone?
	if (!targ || !targ->inuse || !targ->client || !ClientIsPlaying(targ->client) || targ->client->eliminated) {
		//SetTeam(ent, TEAM_SPECTATOR, false, false, false);
		FreeClientFollowers(targ);
		return;
	}

	ownerv = targ->s.origin;
	oldgoal = ent->s.origin;

	// Q2Eaks eyecam handling
	if (g_eyecam->integer) {
		// mark the chased player as instanced so we can disable their model's visibility
		targ->svflags |= SVF_INSTANCED;

		// copy everything from ps but pmove, pov, stats, and team_id
		ent->client->ps.viewangles = targ->client->ps.viewangles;
		ent->client->ps.viewoffset = targ->client->ps.viewoffset;
		ent->client->ps.kick_angles = targ->client->ps.kick_angles;
		ent->client->ps.gunangles = targ->client->ps.gunangles;
		ent->client->ps.gunoffset = targ->client->ps.gunoffset;
		ent->client->ps.gunindex = targ->client->ps.gunindex;
		ent->client->ps.gunskin = targ->client->ps.gunskin;
		ent->client->ps.gunframe = targ->client->ps.gunframe;
		ent->client->ps.gunrate = targ->client->ps.gunrate;
		ent->client->ps.screen_blend = targ->client->ps.screen_blend;
		ent->client->ps.damage_blend = targ->client->ps.damage_blend;
		ent->client->ps.rdflags = targ->client->ps.rdflags;

		// do pmove stuff so view looks right, but not pm_flags
		ent->client->ps.pmove.origin = targ->client->ps.pmove.origin;
		ent->client->ps.pmove.velocity = targ->client->ps.pmove.velocity;
		ent->client->ps.pmove.pm_time = targ->client->ps.pmove.pm_time;
		ent->client->ps.pmove.gravity = targ->client->ps.pmove.gravity;
		ent->client->ps.pmove.delta_angles = targ->client->ps.pmove.delta_angles;
		ent->client->ps.pmove.viewheight = targ->client->ps.pmove.viewheight;
		
		ent->client->pers.hand = targ->client->pers.hand;
		ent->client->pers.weapon = targ->client->pers.weapon;
		
		//FIXME: color shells and damage blends not working

		// unadjusted view and origin handling
		angles = targ->client->v_angle;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		o = ownerv;
		trace = gi.traceline(ownerv, o, targ, MASK_SOLID);
		goal = trace.endpos;
	}
	// vanilla chasecam code
	else {
		ownerv[2] += targ->viewheight;

		angles = targ->client->v_angle;
		if (angles[PITCH] > 56)
			angles[PITCH] = 56;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		o = ownerv + (forward * -30);

		if (o[2] < targ->s.origin[2] + 20)
			o[2] = targ->s.origin[2] + 20;

		// jump animation lifts
		if (!targ->groundentity)
			o[2] += 16;

		trace = gi.traceline(ownerv, o, targ, MASK_SOLID);

		goal = trace.endpos;

		goal += (forward * 2);

		// pad for floors and ceilings
		o = goal;
		o[2] += 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] -= 6;
		}

		o = goal;
		o[2] -= 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] += 6;
		}
	}

	if (targ->deadflag)
		ent->client->ps.pmove.pm_type = PM_DEAD;
	else
		ent->client->ps.pmove.pm_type = PM_FREEZE;

	ent->s.origin = goal;
	ent->client->ps.pmove.delta_angles = targ->client->v_angle - ent->client->resp.cmd_angles;

	if (targ->deadflag) {
		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = targ->client->killer_yaw;
	} else {
		ent->client->ps.viewangles = targ->client->v_angle;
		ent->client->v_angle = targ->client->v_angle;
		AngleVectors(ent->client->v_angle, ent->client->v_forward, nullptr, nullptr);
	}

	edict_t *e = targ ? targ : ent;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = e->client->resp.team == TEAM_SPECTATOR || e->client->eliminated ? 0 : 1;

	ent->viewheight = 0;
	if (!g_eyecam->integer)
		ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;
	gi.linkentity(ent);
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
static void SanitizeString(const char *in, char *out) {
	while (*in) {
		if (*in < ' ') {
			in++;
			continue;
		}
		*out = tolower(*in);
		out++;
		in++;
	}

	*out = '\0';
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int ClientNumberFromString(edict_t *to, char *s) {
	gclient_t	*cl;
	uint32_t	idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	
	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if ((unsigned)idnum >= (unsigned)game.maxclients) {
			gi.LocClient_Print(to, PRINT_HIGH, "Bad client slot: {}\n\"", idnum);
			return -1;
		}

		cl = &game.clients[idnum];
		if (!cl->pers.connected) {
			gi.LocClient_Print(to, PRINT_HIGH, "Client {} is not active.\n\"", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString(s, s2);
	for (idnum = 0, cl = game.clients; idnum < game.maxclients; idnum++, cl++) {
		if (!cl->pers.connected)
			continue;
		SanitizeString(cl->resp.netname, n2);
		if (!strcmp(n2, s2)) {
			return idnum;
		}
	}

	gi.LocClient_Print(to, PRINT_HIGH, "User {} is not on the server.\n\"", s);
	return -1;
}

void FollowNext(edict_t *ent) {
	ptrdiff_t i;
	edict_t *e;

	if (!ent->client->follow_target)
		return;

	i = ent->client->follow_target - g_edicts;
	do {
		i++;
		if (i > game.maxclients)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (ent->client->eliminated && ent->client->resp.team != e->client->resp.team)
			continue;
		if (ClientIsPlaying(e->client) && !e->client->eliminated)
			break;
	} while (e != ent->client->follow_target);

	ent->client->follow_target = e;
	ent->client->follow_update = true;
}

void FollowPrev(edict_t *ent) {
	int		 i;
	edict_t *e;

	if (!ent->client->follow_target)
		return;

	i = ent->client->follow_target - g_edicts;
	do {
		i--;
		if (i < 1)
			i = game.maxclients;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (ent->client->eliminated && ent->client->resp.team != e->client->resp.team)
			continue;
		if (ClientIsPlaying(e->client) && !e->client->eliminated)
			break;
	} while (e != ent->client->follow_target);

	ent->client->follow_target = e;
	ent->client->follow_update = true;
}

void FollowCycle(edict_t *ent, int dir) {
	int			clientnum;
	int			original;
	gclient_t	*cl = ent->client;
	edict_t		*follow_ent = nullptr;

	// if they are playing a duel game, count as a loss
	if (duel->integer && ent->client->resp.team == TEAM_FREE)
		ent->client->resp.losses++;

	// first set them to spectator
	if (cl->resp.spectator_state == SPECTATOR_NOT && !cl->eliminated)
		SetTeam(ent, TEAM_SPECTATOR, false, false, false);

	clientnum = cl->resp.spectator_client;
	original = clientnum;
	do {
		clientnum = (clientnum + dir) % game.maxclients;
		follow_ent = &g_edicts[clientnum + 1];

		// can only follow connected clients
		if (!follow_ent->client->pers.connected)
			continue;
		
		// can't follow another spectator
		if (!ClientIsPlaying(follow_ent->client))
			continue;

		if (follow_ent->client->eliminated)
			continue;

		if (ent->client->eliminated && ent->client->resp.team != follow_ent->client->resp.team)
			continue;

		// this is good, we can use it
		//q3
		cl->resp.spectator_client = clientnum;
		cl->resp.spectator_state = SPECTATOR_FOLLOW;

		//q2
		ent->client->follow_target = follow_ent;
		ent->client->follow_update = true;

		return;
	} while (clientnum != original);

	// leave it where it was
}

void GetFollowTarget(edict_t *ent) {
	for (auto ec : active_clients()) {
		if (ec->inuse && ClientIsPlaying(ec->client) && !ec->client->eliminated) {
			if (ent->client->eliminated && ent->client->resp.team != ec->client->resp.team)
				continue;
			ent->client->follow_target = ec;
			ent->client->follow_update = true;
			UpdateChaseCam(ent);
			return;
		}
	}
	/*
	if (ent->client->chase_msg_time <= level.time) {
		if (ent->client->resp.initialised) {
			gi.LocCenter_Print(ent, "$g_no_players_chase");
			ent->client->chase_msg_time = level.time + 5_sec;
		} else {
			G_Menu_Join_Open(ent);
		}
	}
	*/
}
