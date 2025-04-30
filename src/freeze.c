#include "g_local.h"
#include "m_player.h"
//#include "stdlog.h"
//#include "gslog.h"

#define	nteam	5
#define	game_loop	for (i = 0; i < maxclients->value; i++)
#define	team_loop	for (i = red; i < none; i++)
#define	_team_loop	for (i = red; i <= none; i++)
#define	map_loop	for (i = 0; i < 64; i++)
#define	far_off	100000000

#define	stat_identify	18
#define	stat_red	19
#define	stat_red_arrow	23

#define	_shotgun	0x00000001 // 1
#define	_supershotgun	0x00000002 // 2
#define	_machinegun	0x00000004 // 4
#define	_chaingun	0x00000008 // 8
#define	_grenadelauncher	0x00000010 // 16
#define	_rocketlauncher	0x00000020 // 32
#define	_hyperblaster	0x00000040 // 64
#define	_railgun	0x00000080 // 128

#define	ready_help	0x00000001
#define	thaw_help	0x00000002
#define	frozen_help	0x00000004
#define	chase_help	0x00000008

#define	is_motd	0x00000001
#define	end_vote	0x00000002
#define	mapnohook	0x00000004
#define	everyone_ready 0x00000008

cvar_t *item_respawn_time;
cvar_t *hook_max_len;
cvar_t *hook_rpf;
cvar_t *hook_min_len;
cvar_t *hook_speed;
cvar_t *point_limit;
cvar_t *new_team_count;
cvar_t *frozen_time;
cvar_t *start_weapon;
cvar_t *start_armor;
cvar_t *random_map;
cvar_t *vote_percent;
cvar_t *use_ready;
cvar_t *grapple_wall;
static int	gib_queue;
static int	team_max_count;
static int	moan[8];
static int	lame_hack;
static float	ready_time;

qboolean playerDamage(edict_t *targ, edict_t *attacker, int damage) {
	if (!targ->client)
		return false;
	if (meansOfDeath == MOD_TELEFRAG)
		return false;
	if (!attacker->client)
		return false;
	if (targ->client->hookstate && random() < 0.2)
		targ->client->hookstate = 0;
	if (targ->health > 0) {
		if (!(lame_hack & everyone_ready)) {
			if (!(attacker->client->resp.help & ready_help)) {
				attacker->client->showscores = false;
				attacker->client->resp.help |= ready_help;
				gi.centerprintf(attacker, "Waiting for everyone to be ready.");
				gi.sound(attacker, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
			}
			return true;
		}
		if (targ == attacker)
			return false;
		if (targ->client->resp.team != attacker->client->resp.team && targ->client->respawn_time + 3 < level.time)
			return false;
	} else {
		if (targ->client->frozen) {
			if (random() < 0.1)
				ThrowGib(targ, "models/objects/debris2/tris.md2", damage, GIB_ORGANIC);
			return true;
		} else
			return false;
	}
	if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
		return true;
	meansOfDeath |= MOD_FRIENDLY_FIRE;
	return false;
}

qboolean freezeCheck(edict_t *ent) {
	if (ent->deadflag)
		return false;
	if (meansOfDeath & MOD_FRIENDLY_FIRE)
		return false;
	switch (meansOfDeath) {
	case MOD_FALLING:
	case MOD_SLIME:
	case MOD_LAVA:
		if (random() < 0.08)
			break;
	case MOD_SUICIDE:
	case MOD_CRUSH:
	case MOD_WATER:
	case MOD_EXIT:
	case MOD_TRIGGER_HURT:
	case MOD_BFG_LASER:
	case MOD_BFG_EFFECT:
	case MOD_TELEFRAG:
		return false;
	}
	return true;
}

void freezeAnim(edict_t *ent) {
	ent->client->anim_priority = ANIM_DEATH;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		if (rand() & 1) {
			ent->s.frame = FRAME_crpain1 - 1;
			ent->client->anim_end = FRAME_crpain1 + rand() % 4;
		} else {
			ent->s.frame = FRAME_crdeath1 - 1;
			ent->client->anim_end = FRAME_crdeath1 + rand() % 5;
		}
	} else {
		switch (rand() % 8) {
		case 0:
			ent->s.frame = FRAME_run1 - 1;
			ent->client->anim_end = FRAME_run1 + rand() % 6;
			break;
		case 1:
			ent->s.frame = FRAME_pain101 - 1;
			ent->client->anim_end = FRAME_pain101 + rand() % 4;
			break;
		case 2:
			ent->s.frame = FRAME_pain201 - 1;
			ent->client->anim_end = FRAME_pain201 + rand() % 4;
			break;
		case 3:
			ent->s.frame = FRAME_pain301 - 1;
			ent->client->anim_end = FRAME_pain301 + rand() % 4;
			break;
		case 4:
			ent->s.frame = FRAME_jump1 - 1;
			ent->client->anim_end = FRAME_jump1 + rand() % 6;
			break;
		case 5:
			ent->s.frame = FRAME_death101 - 1;
			ent->client->anim_end = FRAME_death101 + rand() % 6;
			break;
		case 6:
			ent->s.frame = FRAME_death201 - 1;
			ent->client->anim_end = FRAME_death201 + rand() % 6;
			break;
		case 7:
			ent->s.frame = FRAME_death301 - 1;
			ent->client->anim_end = FRAME_death301 + rand() % 6;
			break;
		}
	}

	if (random() < 0.2 && !IsFemale(ent))
		gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("boss3/d_hit.wav"), 1, ATTN_NORM, 0);
	ent->client->frozen = true;
	ent->client->frozen_time = level.time + frozen_time->value;
	ent->client->resp.thawer = NULL;
	ent->client->thaw_time = far_off;
	if (random() > 0.3)
		ent->client->hookstate -= ent->client->hookstate & (grow_on | shrink_on);
	ent->deadflag = DEAD_DEAD;
	gi.linkentity(ent);
}

qboolean gibCheck() {
	if (gib_queue > 35)
		return true;
	else {
		gib_queue++;
		return false;
	}
}

void gibThink(edict_t *ent) {
	gib_queue--;
	G_FreeEdict(ent);
}

static void playerView(edict_t *ent) {
	int	i;
	edict_t *other;
	vec3_t	ent_origin;
	vec3_t	forward;
	vec3_t	other_origin;
	vec3_t	dist;
	trace_t	trace;
	float	dot;
	float	other_dot;
	edict_t *best_other;

	if (level.framenum & 7)
		return;

	other_dot = 0.3;
	best_other = NULL;
	VectorCopy(ent->s.origin, ent_origin);
	ent_origin[2] += ent->viewheight;
	AngleVectors(ent->s.angles, forward, NULL, NULL);

	game_loop
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (other->light_level < 10)
			continue;
		if (other->health <= 0 && !other->client->frozen)
			continue;
		VectorCopy(other->s.origin, other_origin);
		other_origin[2] += other->viewheight;
		VectorSubtract(other_origin, ent_origin, dist);
		if (VectorLength(dist) > 800)
			continue;
		trace = gi.trace(ent_origin, vec3_origin, vec3_origin, other_origin, ent, MASK_OPAQUE);
		if (trace.fraction != 1)
			continue;
		VectorNormalize(dist);
		dot = DotProduct(dist, forward);
		if (dot > other_dot) {
			other_dot = dot;
			best_other = other;
		}
	}
		if (best_other)
			ent->client->viewed = best_other;
		else
			ent->client->viewed = NULL;
}

static void playerThaw(edict_t *ent) {
	int	i;
	edict_t *other;
	int	j;
	vec3_t	eorg;

	game_loop
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (other->health <= 0)
			continue;
		if (other->client->resp.team != ent->client->resp.team)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = ent->s.origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5);
		if (VectorLength(eorg) > MELEE_DISTANCE)
			continue;
		if (!(other->client->resp.help & thaw_help)) {
			other->client->showscores = false;
			other->client->resp.help |= thaw_help;
			gi.centerprintf(other, "Wait here a second to free them.");
			gi.sound(other, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
		}
		ent->client->resp.thawer = other;
		if (ent->client->thaw_time == far_off) {
			ent->client->thaw_time = level.time + 3;
			gi.sound(ent, CHAN_BODY, gi.soundindex("world/steam3.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}
	ent->client->resp.thawer = NULL;
	ent->client->thaw_time = far_off;
}

static void playerBreak(edict_t *ent, int force) {
	int	n;

	ent->client->respawn_time = level.time + 1;
	if (ent->waterlevel == 3)
		gi.sound(ent, CHAN_BODY, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("world/brkglas.wav"), 1, ATTN_NORM, 0);
	n = rand() % (gib_queue > 10 ? 5 : 3);
	if (rand() & 1) {
		switch (n) {
		case 0:
			ThrowGib(ent, "models/objects/gibs/arm/tris.md2", force, GIB_ORGANIC);
			break;
		case 1:
			ThrowGib(ent, "models/objects/gibs/bone/tris.md2", force, GIB_ORGANIC);
			break;
		case 2:
			ThrowGib(ent, "models/objects/gibs/bone2/tris.md2", force, GIB_ORGANIC);
			break;
		case 3:
			ThrowGib(ent, "models/objects/gibs/chest/tris.md2", force, GIB_ORGANIC);
			break;
		case 4:
			ThrowGib(ent, "models/objects/gibs/leg/tris.md2", force, GIB_ORGANIC);
			break;
		}
	}
	while (n--)
		ThrowGib(ent, "models/objects/debris1/tris.md2", force, GIB_ORGANIC);
	ent->takedamage = DAMAGE_NO;
	ent->movetype = MOVETYPE_TOSS;
	ThrowClientHead(ent, force);
	ent->client->frozen = false;
	freeze[ent->client->resp.team].update = true;
	ent->client->ps.stats[STAT_CHASE] = 0;
}

static void playerUnfreeze(edict_t *ent) {
	if (level.time > ent->client->frozen_time && level.time > ent->client->respawn_time) {
		playerBreak(ent, 50);
		return;
	}
	if (ent->waterlevel == 3 && !(level.framenum & 3))
		ent->client->frozen_time -= 0.15;
	if (level.time > ent->client->thaw_time) {
		if (!ent->client->resp.thawer || !ent->client->resp.thawer->inuse) {
			ent->client->resp.thawer = NULL;
			ent->client->thaw_time = far_off;
		} else {
			ent->client->resp.thawer->client->resp.score++;
			ent->client->resp.thawer->client->resp.thawed++;
			sl_LogScore(&gi, ent->client->resp.thawer->client->pers.netname, NULL, "Thaw", NULL, 1, level.time, ent->client->resp.thawer->client->ping);
			freeze[ent->client->resp.team].thawed++;
			if (rand() & 1)
				gi.bprintf(PRINT_HIGH, "%s thaws %s like a package of frozen peas.\n", ent->client->resp.thawer->client->pers.netname, ent->client->pers.netname);
			else
				gi.bprintf(PRINT_HIGH, "%s evicts %s from their igloo.\n", ent->client->resp.thawer->client->pers.netname, ent->client->pers.netname);
			playerBreak(ent, 100);
		}
	}
}

static void playerMove(edict_t *ent) {
	int	i;
	edict_t *other;
	vec3_t	forward;
	float	dist;
	int	j;
	vec3_t	eorg;

	if (ent->client->hookstate)
		return;
	AngleVectors(ent->s.angles, forward, NULL, NULL);
	game_loop
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (!other->client->frozen)
			continue;
		if (other->client->resp.team == ent->client->resp.team)
			continue;
		if (other->client->hookstate)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = ent->s.origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5);
		dist = VectorLength(eorg);
		if (dist > MELEE_DISTANCE)
			continue;
		VectorScale(forward, 600, other->velocity);
		other->velocity[2] = 200;
		gi.linkentity(other);
	}
}

void freezeMain(edict_t *ent) {
	if (!ent->inuse)
		return;
	playerView(ent);
	if (ent->client->resp.spectator)
		return;
	if (ent->client->frozen) {
		playerThaw(ent);
		playerUnfreeze(ent);
	} else if (ent->health > 0)
		playerMove(ent);
}

void freezeScore(edict_t *ent, edict_t *killer) {
	int	i, j, k;
	edict_t *other;
	int	team, score;
	int	total[nteam];
	int	sorted[nteam][MAX_CLIENTS];
	int	sortedscores[nteam][MAX_CLIENTS];
	int	count, best_total, best_team;
	int	x, y;
	int	move_over;
	char	string[1400];
	int	stringlength;
	char *tag;
	char	entry[1024];
	gclient_t *cl;

	_team_loop
		total[i] = 0;
	game_loop
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			team = none;
		else
			team = other->client->resp.team;
		score = other->client->resp.score;
		for (j = 0; j < total[team]; j++) {
			if (score > sortedscores[team][j])
				break;
		}
		for (k = total[team]; k > j; k--) {
			sorted[team][k] = sorted[team][k - 1];
			sortedscores[team][k] = sortedscores[team][k - 1];
		}
		sorted[team][j] = i;
		sortedscores[team][j] = score;
		total[team]++;
	}

	for (;;) {
		count = 0;
		team_loop
			count += 2 + total[i];
		if (count <= 48)
			break;
		best_total = 0;
		team_loop
			if (total[i] > best_total) {
				best_total = total[i];
				best_team = i;
			}
		if (best_total)
			total[best_team]--;
	}

	x = 0;
	y = 32;

	count = 4;
	_team_loop
		if (total[i])
			count += 3 + total[i];
	move_over = (int)(count / 2) * 8;

	string[0] = 0;
	stringlength = strlen(string);

	_team_loop
	{
		if (i == red)
			tag = "k_redkey";
		else if (i == blue)
			tag = "k_bluekey";
		else if (i == green)
			tag = "k_security";
		else
			tag = "k_powercube";

		if (i == none)
			Com_sprintf(entry, sizeof(entry), "xv %d yv %d string \"%6.6s\" ", x, y, freeze_team_[i]);
		else
			Com_sprintf(entry, sizeof(entry), "xv %d yv %d if %d picn %s endif string \"%6.6s Sco%3d Tha%3d\" ", x, y, 19 + i, tag, freeze_team_[i], freeze[i].score, freeze[i].thawed);
		k = strlen(entry);
		if (stringlength + k > 1024)
			break;
		if (total[i]) {
			strcpy(string + stringlength, entry);
			stringlength += k;
			y += 16;
		} else
			continue;
		for (j = 0; j < total[i]; j++) {
			if (y >= 224) {
				if (x == 0)
					x = 160;
				else
					break;
				y = 32;
			}
			cl = &game.clients[sorted[i][j]];
			Com_sprintf(entry, sizeof(entry), "ctf %d %d %d %d %d ", x, y, sorted[i][j], cl->resp.score, level.intermissiontime ? cl->resp.thawed : (cl->ping > 999 ? 999 : cl->ping));
			if (cl->frozen)
				sprintf(entry + strlen(entry), "xv %d yv %d string2 \"/\" ", x + 56, y);
			k = strlen(entry);
			if (stringlength + k > 1024)
				break;
			strcpy(string + stringlength, entry);
			stringlength += k;
			y += 8;
		}
		Com_sprintf(entry, sizeof(entry), "xv %d yv %d string \"--------------------\" ", x, y);
		k = strlen(entry);
		if (stringlength + k > 1024)
			break;
		strcpy(string + stringlength, entry);
		stringlength += k;
		if (y >= 208 || (y >= move_over && x == 0)) {
			if (x == 0)
				x = 160;
			else
				break;
			y = 32;
		} else
			y += 8;
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
}

void freezeIntermission(void) {
	int	i, j, k;
	int	team;

	i = j = k = 0;
	team_loop
		if (freeze[i].score > j)
			j = freeze[i].score;

	team_loop
		if (freeze[i].score == j) {
			k++;
			team = i;
		}

	if (k > 1) {
		i = j = k = 0;
		team_loop
			if (freeze[i].thawed > j)
				j = freeze[i].thawed;

		team_loop
			if (freeze[i].thawed == j) {
				k++;
				team = i;
			}
	}
	if (k != 1) {
		gi.bprintf(PRINT_HIGH, "Stalemate!\n");
		return;
	}
	gi.bprintf(PRINT_HIGH, "%s team is the winner!\n", freeze_team[team]);
	team_loop
		freeze[i].win_time = level.time;
	freeze[team].win_time = far_off;
}

char *makeGreen(char *s) {
	static char	string[16];
	int	i;

	if (!*s)
		return "";
	for (i = 0; i < 15 && *s; i++, s++) {
		string[i] = *s;
		string[i] |= 0x80;
	}
	string[i] = 0;
	return string;
}

static void playerHealth(edict_t *ent) {
	int	n;

	for (n = 0; n < game.num_items; n++)
		ent->client->pers.inventory[n] = 0;

	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->flags &= ~FL_POWER_ARMOR;

	ent->health = ent->client->pers.max_health;

	ent->s.sound = 0;
	ent->client->weapon_sound = 0;
}

static void breakTeam(int team) {
	int	i;
	edict_t *ent;
	float	break_time;

	break_time = level.time;
	game_loop
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->frozen) {
			if (ent->client->resp.team != team && team_max_count >= 3)
				continue;
			ent->client->frozen_time = break_time;
			break_time += 0.25;
			continue;
		}
		if (ent->health > 0 && team_max_count < 3) {
			playerHealth(ent);
			playerWeapon(ent);
		}
	}
	freeze[team].break_time = break_time + 1;
	if (rand() & 1)
		gi.bprintf(PRINT_HIGH, "%s team was run circles around by their foe.\n", freeze_team[team]);
	else
		gi.bprintf(PRINT_HIGH, "%s team was less than a match for their foe.\n", freeze_team[team]);
}

static void updateTeam(int team) {
	int	i;
	edict_t *ent;
	int	frozen, alive;
	char	small[32];
	int	play_sound = 0;

	frozen = alive = 0;
	game_loop
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->resp.spectator)
			continue;
		if (ent->client->resp.team != team)
			continue;
		if (ent->client->frozen)
			frozen++;
		if (ent->health > 0)
			alive++;
	}
	freeze[team].frozen = frozen;
	freeze[team].alive = alive;

	if (frozen && !alive) {
		team_loop
		{
			if (freeze[i].alive) {
				play_sound++;
				freeze[i].score++;
				freeze[i].win_time = level.time + 5;
				freeze[i].update = true;
			}
		}
		breakTeam(team);

		if (play_sound <= 1)
			gi.positioned_sound(vec3_origin, world, CHAN_VOICE | CHAN_RELIABLE, gi.soundindex("world/xian1.wav"), 1, ATTN_NONE, 0);
	}

	Com_sprintf(small, sizeof(small), " %s%3d/%3d", freeze_team__[team], freeze[team].score, freeze[team].alive);
	//	if (!(freeze[team].alive == 1 && freeze[team].frozen))
	//		makeGreen(small);
	gi.configstring(CS_GENERAL + team, small);
}

qboolean endCheck() {
	int	i;

	if (!(level.framenum & 31)) {
		if (new_team_count->value) {
			int	_new_team_count = new_team_count->value;
			int	total[nteam];

			_team_loop
				total[i] = freeze[i].alive + freeze[i].frozen;

			if (total[yellow])
				team_max_count = 4;
			else if (total[red] >= _new_team_count && total[blue] >= _new_team_count) {
				if (total[green] >= _new_team_count)
					team_max_count = 4;
				else
					team_max_count = 3;
			} else if (total[green])
				team_max_count = 3;
			else
				team_max_count = 0;
		} else
			team_max_count = 0;
	}

	if (use_ready->value && !(lame_hack & everyone_ready)) {
		switch ((int)(ready_time / FRAMETIME) - level.framenum) {
		case 150:
		case 100:
		case 50:
		case 40:
		case 30:
		case 20:
			gi.bprintf(PRINT_HIGH, "Begin in %d seconds!\n", (int)(((ready_time / FRAMETIME) - level.framenum) * FRAMETIME));
		}
		if (level.time > ready_time) {
			edict_t *ent;

			lame_hack |= everyone_ready;
			gi.bprintf(PRINT_HIGH, "Begin!\n");
			game_loop
			{
				ent = g_edicts + 1 + i;
				if (!ent->inuse)
					continue;
				if (ent->client->resp.spectator)
					continue;
				if (ent->health > 0) {
					playerHealth(ent);
					playerWeapon(ent);
				}
			}
		}
	} else
		lame_hack |= everyone_ready;

	team_loop
		if (freeze[i].update && level.time > freeze[i].last_update) {
			updateTeam(i);
			freeze[i].update = false;
			freeze[i].last_update = level.time + 3;
		}

	if (point_limit->value) {
		int	_point_limit;

		_point_limit = point_limit->value;
		if (team_max_count >= 3)
			_point_limit *= 3;
		team_loop
			if (freeze[i].score >= _point_limit)
				return true;
	}
	if (lame_hack & end_vote)
		return true;

	return false;
}

void freezeRespawn(edict_t *ent, float delay) {
	if (item_respawn_time->value)
		SetRespawn(ent, item_respawn_time->value);
	else
		SetRespawn(ent, delay);
}

void playerShell(edict_t *ent, int team) {
	ent->s.effects |= EF_COLOR_SHELL;
	ent->s.renderfx |= RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE;

}

void freezeEffects(edict_t *ent) {
	if (level.intermissiontime)
		return;
	if (!ent->client->frozen)
		return;
	if (!ent->client->resp.thawer || level.framenum & 8)
		playerShell(ent, ent->client->resp.team);
}

void playerStat(edict_t *ent) {
	int	i;

	if (ent->client->viewed && ent->client->viewed->inuse) {
		int	playernum = ent->client->viewed - g_edicts - 1;

		ent->client->ps.stats[stat_identify] = CS_PLAYERSKINS + playernum;
	} else
		ent->client->ps.stats[stat_identify] = 0;

	team_loop
	{
		if (((i == green && team_max_count < 3) || (i == yellow && team_max_count < 4)) ||
			(freeze[i].win_time > level.time && !(level.framenum & 8))) {
			ent->client->ps.stats[stat_red + i] = 0;
			ent->client->ps.stats[stat_red_arrow + i] = 0;
			continue;
		}

		ent->client->ps.stats[stat_red + i] = CS_GENERAL + i;
		if (ent->client->resp.team == i && !ent->client->resp.spectator)
			ent->client->ps.stats[stat_red_arrow + i] = CS_GENERAL + 5;
		else
			ent->client->ps.stats[stat_red_arrow + i] = 0;
	}
}

void freezeSpawn() {
	int	i;

	loadMessage();
	loadMap();

	memset(freeze, 0, sizeof(freeze));
	team_loop
		freeze[i].update = true;
	lame_hack &= ~everyone_ready;
	ready_time = far_off;
	gib_queue = 0;

	moan[0] = gi.soundindex("insane/insane1.wav");
	moan[1] = gi.soundindex("insane/insane2.wav");
	moan[2] = gi.soundindex("insane/insane3.wav");
	moan[3] = gi.soundindex("insane/insane4.wav");
	moan[4] = gi.soundindex("insane/insane6.wav");
	moan[5] = gi.soundindex("insane/insane8.wav");
	moan[6] = gi.soundindex("insane/insane9.wav");
	moan[7] = gi.soundindex("insane/insane10.wav");

	mapLight();
	gi.configstring(CS_GENERAL + 5, ">");
}
