// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "../g_local.h"
#include "../m_player.h"

#include <assert.h>

constexpr const char *BREAKER = "\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37";


/*--------------------------------------------------------------------------*/

#ifndef KEX_Q2_GAME
/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
static edict_t *loc_findradius(edict_t *from, const vec3_t &org, float rad) {
	vec3_t eorg;
	int	   j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for (; from < &g_edicts[globals.num_edicts]; from++) {
		if (!from->inuse)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5f);
		if (eorg.length() > rad)
			continue;
		return from;
	}

	return nullptr;
}
#endif


/*--------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/*
GT_CTF_ScoreBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumaltive.  You get one, they are in importance
order.
*/
void GT_CTF_ScoreBonuses(edict_t *targ, edict_t *inflictor, edict_t *attacker) {
	edict_t		*ent;
	item_id_t	flag_item, enemy_flag_item;
	int			otherteam;
	edict_t		*flag, *carrier = nullptr;
	const char	*c;
	vec3_t		v1, v2;

	if (!ctf->integer)
		return;

	if (targ->client && attacker->client) {
		if (attacker->client->resp.ghost)
			if (attacker != targ)
				attacker->client->resp.ghost->kills++;
		if (targ->client->resp.ghost)
			targ->client->resp.ghost->deaths++;
	}

	// no bonus for fragging yourself
	if (!targ->client || !attacker->client || targ == attacker)
		return;

	otherteam = Teams_OtherTeamNum(targ->client->resp.team);
	if (otherteam < 0)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (targ->client->resp.team == TEAM_RED) {
		flag_item = IT_FLAG_RED;
		enemy_flag_item = IT_FLAG_BLUE;
	} else {
		flag_item = IT_FLAG_BLUE;
		enemy_flag_item = IT_FLAG_RED;
	}

	// did the attacker frag the flag carrier?
	if (targ->client->pers.inventory[enemy_flag_item]) {
		attacker->client->resp.ctf_lastfraggedcarrier = level.time;
		G_AdjustPlayerScore(attacker->client, CTF_FRAG_CARRIER_BONUS, false, 0);
		gi.LocClient_Print(attacker, PRINT_MEDIUM, "$g_bonus_enemy_carrier",
			CTF_FRAG_CARRIER_BONUS);

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (uint32_t i = 1; i <= game.maxclients; i++) {
			ent = g_edicts + i;
			if (ent->inuse && ent->client->resp.team == otherteam)
				ent->client->resp.ctf_lasthurtcarrier = 0_ms;
		}
		return;
	}

	if (targ->client->resp.ctf_lasthurtcarrier &&
		level.time - targ->client->resp.ctf_lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->pers.inventory[flag_item]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		G_AdjustPlayerScore(attacker->client, CTF_CARRIER_DANGER_PROTECT_BONUS, false, 0);
		gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_flag_defense",
			attacker->client->pers.netname,
			Teams_TeamName(attacker->client->resp.team));
		if (attacker->client->resp.ghost)
			attacker->client->resp.ghost->carrierdef++;
		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch (attacker->client->resp.team) {
	case TEAM_RED:
		c = ITEM_CTF_FLAG_RED;
		break;
	case TEAM_BLUE:
		c = ITEM_CTF_FLAG_BLUE;
		break;
	default:
		return;
	}

	flag = nullptr;
	while ((flag = G_FindByString<&edict_t::classname>(flag, c)) != nullptr) {
		if (!(flag->spawnflags & SPAWNFLAG_ITEM_DROPPED))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	// find attacker's team's flag carrier
	for (uint32_t i = 1; i <= game.maxclients; i++) {
		carrier = g_edicts + i;
		if (carrier->inuse &&
			carrier->client->pers.inventory[flag_item])
			break;
		carrier = nullptr;
	}

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	v1 = targ->s.origin - flag->s.origin;
	v2 = attacker->s.origin - flag->s.origin;

	if ((v1.length() < CTF_TARGET_PROTECT_RADIUS ||
		v2.length() < CTF_TARGET_PROTECT_RADIUS ||
		loc_CanSee(flag, targ) || loc_CanSee(flag, attacker)) &&
		attacker->client->resp.team != targ->client->resp.team) {
		// we defended the base flag
		G_AdjustPlayerScore(attacker->client, CTF_FLAG_DEFENSE_BONUS, false, 0);
		if (flag->solid == SOLID_NOT)
			gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_defend_base",
				attacker->client->pers.netname,
				Teams_TeamName(attacker->client->resp.team));
		else
			gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_defend_flag",
				attacker->client->pers.netname,
				Teams_TeamName(attacker->client->resp.team));
		if (attacker->client->resp.ghost)
			attacker->client->resp.ghost->basedef++;
		return;
	}

	if (carrier && carrier != attacker) {
		v1 = targ->s.origin - carrier->s.origin;
		v2 = attacker->s.origin - carrier->s.origin;

		if (v1.length() < CTF_ATTACKER_PROTECT_RADIUS ||
			v2.length() < CTF_ATTACKER_PROTECT_RADIUS ||
			loc_CanSee(carrier, targ) || loc_CanSee(carrier, attacker)) {
			G_AdjustPlayerScore(attacker->client, CTF_CARRIER_PROTECT_BONUS, false, 0);
			gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_defend_carrier",
				attacker->client->pers.netname,
				Teams_TeamName(attacker->client->resp.team));
			if (attacker->client->resp.ghost)
				attacker->client->resp.ghost->carrierdef++;
			return;
		}
	}
}

void GT_CTF_CheckHurtCarrier(edict_t *targ, edict_t *attacker) {
	item_id_t flag_item;

	if (!ctf->integer)
		return;

	if (!targ->client || !attacker->client)
		return;

	if (targ->client->resp.team == TEAM_RED)
		flag_item = IT_FLAG_BLUE;
	else
		flag_item = IT_FLAG_RED;

	if (targ->client->pers.inventory[flag_item] &&
		targ->client->resp.team != attacker->client->resp.team)
		attacker->client->resp.ctf_lasthurtcarrier = level.time;
}

/*------------------------------------------------------------------------*/

void GT_CTF_ResetFlag(team_t team) {
	const char *c;
	edict_t *ent;

	if (!ctf->integer)
		return;

	switch (team) {
	case TEAM_RED:
		c = ITEM_CTF_FLAG_RED;
		break;
	case TEAM_BLUE:
		c = ITEM_CTF_FLAG_BLUE;
		break;
	default:
		return;
	}

	ent = nullptr;
	while ((ent = G_FindByString<&edict_t::classname>(ent, c)) != nullptr) {
		if (ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED))
			G_FreeEdict(ent);
		else {
			ent->svflags &= ~SVF_NOCLIENT;
			ent->solid = SOLID_TRIGGER;
			gi.linkentity(ent);
			ent->s.event = EV_ITEM_RESPAWN;
		}
	}
}

void GT_CTF_ResetFlags() {

	if (!ctf->integer)
		return;

	GT_CTF_ResetFlag(TEAM_RED);
	GT_CTF_ResetFlag(TEAM_BLUE);
}

bool GT_CTF_PickupFlag(edict_t *ent, edict_t *other) {
	int			team;
	edict_t *player;
	item_id_t	flag_item, enemy_flag_item;

	if (!ctf->integer)
		return false;

	// figure out what team this flag is
	if (ent->item->id == IT_FLAG_RED)
		team = TEAM_RED;
	else if (ent->item->id == IT_FLAG_BLUE)
		team = TEAM_BLUE;
	else {
		gi.LocClient_Print(ent, PRINT_HIGH, "Don't know what team the flag is on.\n");
		return false;
	}

	// same team, if the flag at base, check to he has the enemy flag
	if (team == TEAM_RED) {
		flag_item = IT_FLAG_RED;
		enemy_flag_item = IT_FLAG_BLUE;
	} else {
		flag_item = IT_FLAG_BLUE;
		enemy_flag_item = IT_FLAG_RED;
	}

	if (team == other->client->resp.team) {

		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED)) {
			// the flag is at home base.  if the player has the enemy
			// flag, he's just scored a capture!

			if (other->client->pers.inventory[enemy_flag_item]) {
				if (other->client->pers.team_state.flag_pickup_time) {
					gi.LocBroadcast_Print(PRINT_HIGH, "{} TEAM CAPTURED the flag! ({} captured in {})\n",
						Teams_TeamName(team), other->client->pers.netname, G_TimeStringMs((level.time - other->client->pers.team_state.flag_pickup_time).milliseconds()));
				} else {
					gi.LocBroadcast_Print(PRINT_HIGH, "{} TEAM CAPTURED the flag! (captured by {})\n",
						Teams_TeamName(team), other->client->pers.netname);
				}
				other->client->pers.inventory[enemy_flag_item] = 0;

				level.last_flag_capture = level.time;
				level.last_capture_team = team;
				G_AdjustTeamScore(team, 1);

				gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);

				// other gets capture bonus
				G_AdjustPlayerScore(other->client, CTF_CAPTURE_BONUS, false, 0);
				if (other->client->resp.ghost)
					other->client->resp.ghost->caps++;

				// Ok, let's do the player loop, hand out the bonuses
				for (uint32_t i = 1; i <= game.maxclients; i++) {
					player = &g_edicts[i];
					if (!player->inuse)
						continue;

					if (player->client->resp.team != other->client->resp.team)
						player->client->resp.ctf_lasthurtcarrier = -5_sec;
					else if (player->client->resp.team == other->client->resp.team) {
						if (player != other)
							G_AdjustPlayerScore(player->client, CTF_TEAM_BONUS, false, 0);
						// award extra points for capture assists
						if (player->client->resp.ctf_lastreturnedflag && player->client->resp.ctf_lastreturnedflag + CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
							gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_return", player->client->pers.netname);
							G_AdjustPlayerScore(player->client, CTF_RETURN_FLAG_ASSIST_BONUS, false, 0);
						}
						if (player->client->resp.ctf_lastfraggedcarrier && player->client->resp.ctf_lastfraggedcarrier + CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
							gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_frag_carrier", player->client->pers.netname);
							G_AdjustPlayerScore(player->client, CTF_FRAG_CARRIER_ASSIST_BONUS, false, 0);
						}
					}
				}

				GT_CTF_ResetFlags();
				return false;
			}
			return false; // its at home base already
		}
		// hey, its not home.  return it by teleporting it back
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_returned_flag",
			other->client->pers.netname, Teams_TeamName(team));
		G_AdjustPlayerScore(other->client, CTF_RECOVERY_BONUS, false, 0);
		other->client->resp.ctf_lastreturnedflag = level.time;
		gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
		// GT_CTF_ResetFlag will remove this entity!  We must return false
		GT_CTF_ResetFlag((team_t)team);
		return false;
	}

	// hey, its not our flag, pick it up
	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED)) {
		other->client->pers.team_state.flag_pickup_time = level.time;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "$g_got_flag",
		other->client->pers.netname, Teams_TeamName(team));
	G_AdjustPlayerScore(other->client, CTF_FLAG_BONUS, false, 0);

	other->client->pers.inventory[flag_item] = 1;
	other->client->resp.ctf_flagsince = level.time;

	// pick up the flag
	// if it's not a dropped flag, we just make is disappear
	// if it's dropped, it will be removed by the pickup caller
	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED)) {
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	return true;
}

TOUCH(GT_CTF_DropFlagTouch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void {

	if (!ctf->integer)
		return;

	// owner (who dropped us) can't touch for two secs
	if (other == ent->owner &&
		ent->nextthink - level.time > CTF_AUTO_FLAG_RETURN_TIMEOUT - 2_sec)
		return;

	Touch_Item(ent, other, tr, other_touching_self);
}

THINK(GT_CTF_DropFlagThink) (edict_t *ent) -> void {

	if (!ctf->integer)
		return;

	// auto return the flag
	// reset flag will remove ourselves
	if (ent->item->id == IT_FLAG_RED) {
		GT_CTF_ResetFlag(TEAM_RED);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
			Teams_TeamName(TEAM_RED));
	} else if (ent->item->id == IT_FLAG_BLUE) {
		GT_CTF_ResetFlag(TEAM_BLUE);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
			Teams_TeamName(TEAM_BLUE));
	}

	gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
}

// Called from PlayerDie, to drop the flag from a dying player
void GT_CTF_DeadDropFlag(edict_t *self) {
	edict_t *dropped = nullptr;

	if (!ctf->integer)
		return;

	if (self->client->pers.inventory[IT_FLAG_RED]) {
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG_RED));
		self->client->pers.inventory[IT_FLAG_RED] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
			self->client->pers.netname, Teams_TeamName(TEAM_RED));
	} else if (self->client->pers.inventory[IT_FLAG_BLUE]) {
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG_BLUE));
		self->client->pers.inventory[IT_FLAG_BLUE] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
			self->client->pers.netname, Teams_TeamName(TEAM_BLUE));
	}
	self->client->pers.team_state.flag_pickup_time = 0_ms;
	if (dropped) {
		dropped->think = GT_CTF_DropFlagThink;
		dropped->nextthink = level.time + CTF_AUTO_FLAG_RETURN_TIMEOUT;
		dropped->touch = GT_CTF_DropFlagTouch;
	}
}

void GT_CTF_DropFlag(edict_t *ent, gitem_t *item) {

	if (!ctf->integer)
		return;

	ent->client->pers.team_state.flag_pickup_time = 0_ms;

	if (brandom())
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_lusers_drop_flags");
	else
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_winners_drop_flags");
}

THINK(GT_CTF_FlagThink) (edict_t *ent) -> void {

	if (!ctf->integer)
		return;

	if (ent->solid != SOLID_NOT)
		ent->s.frame = 173 + (((ent->s.frame - 173) + 1) % 16);
	ent->nextthink = level.time + 10_hz;
}

THINK(GT_CTF_FlagSetup) (edict_t *ent) -> void {
	trace_t tr;
	vec3_t	dest;

	if (!ctf->integer)
		return;

	ent->mins = { -15, -15, -15 };
	ent->maxs = { 15, 15, 15 };

	if (ent->model)
		gi.setmodel(ent, ent->model);
	else
		gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;
	ent->s.frame = 173;

	dest = ent->s.origin + vec3_t{ 0, 0, -128 };

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid) {
		gi.Com_PrintFmt("GT_CTF_FlagSetup: {} startsolid at {}\n", ent->classname, ent->s.origin);
		G_FreeEdict(ent);
		return;
	}

	ent->s.origin = tr.endpos;

	gi.linkentity(ent);

	ent->nextthink = level.time + 10_hz;
	ent->think = GT_CTF_FlagThink;
}

void GT_CTF_Effects(edict_t *player) {

	if (!ctf->integer)
		return;

	player->s.effects &= ~(EF_FLAG_RED | EF_FLAG_BLUE);
	if (player->health > 0) {
		if (player->client->pers.inventory[IT_FLAG_RED])
			player->s.effects |= EF_FLAG_RED;
		if (player->client->pers.inventory[IT_FLAG_BLUE])
			player->s.effects |= EF_FLAG_BLUE;
	}

	if (player->client->pers.inventory[IT_FLAG_RED])
		player->s.modelindex3 = modelindex_flag1;
	else if (player->client->pers.inventory[IT_FLAG_BLUE])
		player->s.modelindex3 = modelindex_flag2;
	else
		player->s.modelindex3 = 0;
}

// called when we enter the intermission
void Teams_CalcScores() {
	if (!IsTeamplay())
		return;

	level.team_scores[TEAM_RED] = level.team_scores[TEAM_BLUE] = 0;
	for (uint32_t i = 0; i < game.maxclients; i++) {
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.team == TEAM_RED)
			level.team_scores[TEAM_RED] += game.clients[i].resp.score;
		else if (game.clients[i].resp.team == TEAM_BLUE)
			level.team_scores[TEAM_BLUE] += game.clients[i].resp.score;
	}
}

// [Paril-KEX] end game rankings
void Teams_CalcRankings(std::array<uint32_t, MAX_CLIENTS> &player_ranks) {
	if (!IsTeamplay())
		return;

	// we're all winners.. or losers. whatever
	if (level.team_scores[TEAM_RED] == level.team_scores[TEAM_BLUE]) {
		player_ranks.fill(1);
		return;
	}

	team_t winning_team = (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE]) ? TEAM_RED : TEAM_BLUE;

	for (auto player : active_players())
		if (player->client->pers.spawned && !ClientIsSpectating(player->client))
			player_ranks[player->s.number - 1] = player->client->resp.team == winning_team ? 1 : 2;
}

void CheckEndTDMLevel() {
	if (level.team_scores[TEAM_RED] >= fraglimit->integer || level.team_scores[TEAM_BLUE] >= fraglimit->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_fraglimit_hit");
		EndDMLevel();
	}
}


void Menu_Dirty() {
	for (auto player : active_players())
		if (player->client->menu) {
			player->client->menudirty = true;
			player->client->menutime = level.time;
		}
}

#if 0
void CTFTeam_f(edict_t *ent) {
	const char *t;
	team_t	  desired_team;
	bool teams = IsTeamplay();

	t = gi.args();
	if (!*t) {
		gi.LocClient_Print(ent, PRINT_CENTER, "$g_you_are_on_team",
			Teams_TeamName(ent->client->resp.team));
		return;
	}

	if (level.match > MATCH_SETUP) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_change_teams");
		return;
	}

	// [Paril-KEX] with force-join, don't allow us to switch
	// using this command.
	if (g_dm_force_join->integer) {
		if (!(ent->svflags & SVF_BOT)) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_change_teams");
			return;
		}
	}

	if (teams && (!Q_strcasecmp(t, "red") || !Q_strcasecmp(t, "r")))
		desired_team = TEAM_RED;
	else if (teams && (!Q_strcasecmp(t, "blue") || !Q_strcasecmp(t, "b")))
		desired_team = TEAM_BLUE;
	else if (!teams && (!Q_strcasecmp(t, "free") || !Q_strcasecmp(t, "f")))
		desired_team = TEAM_FREE;
	else if (!Q_strcasecmp(t, "auto") || !Q_strcasecmp(t, "a"))
		if (teams) {
			desired_team = irandom(2) ? TEAM_BLUE : TEAM_RED;
		} else {
			desired_team = TEAM_FREE;
		} else if (!Q_strcasecmp(t, "spectator") || !Q_strcasecmp(t, "spectate") || !Q_strcasecmp(t, "spec") || !Q_strcasecmp(t, "s")) {
			//desired_team = TEAM_SPECTATOR;
			Cmd_Observer_f(ent);
			return;
		} else {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_team", t);
			return;
		}

		if (ent->client->resp.team == desired_team) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_already_on_team",
				Teams_TeamName(ent->client->resp.team));
			return;
		}

		////
		ent->svflags = SVF_NONE;
		ent->flags &= ~FL_GODMODE;
		ent->client->resp.team = desired_team;
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
				ent->client->pers.netname, Teams_TeamName(desired_team));

			if (desired_team == TEAM_SPECTATOR) {
				gi.WriteString("spectator 1\n");
			} else {
				gi.WriteString("spectator 0\n");
			}
			return;
		}

		ent->health = 0;
		player_die(ent, ent, ent, 100000, vec3_origin, { MOD_CHANGE_TEAM, true });

		// don't even bother waiting for death frames
		ent->deadflag = true;
		respawn(ent);

		G_SetPlayerScore(ent->client, 0);

		gi.LocBroadcast_Print(PRINT_HIGH, "$g_changed_team",
			ent->client->pers.netname, Teams_TeamName(desired_team));
}
#endif
/*-----------------------------------------------------------------------*/

static void Menu_SetHostName(pmenu_t *p) {
	Q_strlcpy(p->text, hostname->string, sizeof(p->text));
}

static void Menu_SetGamemodName(pmenu_t *p) {
	Q_strlcpy(p->text, level.gamemod_name, sizeof(p->text));
}

static void Menu_SetGametypeName(pmenu_t *p) {
	Q_strlcpy(p->text, level.gametype_name, sizeof(p->text));
}

static void Menu_SetLevelName(pmenu_t *p) {
	static char levelname[33];

	levelname[0] = '*';
	if (g_edicts[0].message)
		Q_strlcpy(levelname + 1, g_edicts[0].message, sizeof(levelname) - 1);
	else
		Q_strlcpy(levelname + 1, level.mapname, sizeof(levelname) - 1);
	levelname[sizeof(levelname) - 1] = 0;
	Q_strlcpy(p->text, levelname, sizeof(p->text));
}

void Match_ResetAllPlayers() {
	uint32_t i;
	edict_t *ent;

	for (i = 1; i <= game.maxclients; i++) {
		ent = g_edicts + i;
		if (!ent->inuse)
			continue;

		if (ent->client->menu)
			PMenu_Close(ent);

		Weapon_Grapple_DoReset(ent->client);
		GT_CTF_DeadDropFlag(ent);
		Tech_DeadDrop(ent);

		ent->client->resp.team = TEAM_SPECTATOR;
		ent->client->resp.ready = false;
		ent->client->resp.inactive = false;
		ent->client->resp.inactivity_time = 0_ms;

		ent->svflags = SVF_NONE;
		ent->flags &= ~FL_GODMODE;
		ClientSpawn(ent);
	}

	// reset the level
	Tech_Reset();
	GT_CTF_ResetFlags();

	for (ent = g_edicts + 1, i = 1; i < globals.num_edicts; i++, ent++) {
		if (ent->inuse && !ent->client) {
			if (ent->solid == SOLID_NOT && ent->think == DoRespawn &&
				ent->nextthink >= level.time) {
				ent->nextthink = 0_ms;
				DoRespawn(ent);
			}
		}
	}
	if (level.match == MATCH_SETUP)
		level.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
}

// start a match
void Match_Start() {
	edict_t *ent;

	level.match = MATCH_GAME;
	level.matchtime = level.time + gtime_t::from_min(timelimit->integer);
	level.countdown = false;

	level.team_scores[TEAM_RED] = level.team_scores[TEAM_BLUE] = 0;

	memset(level.ghosts, 0, sizeof(level.ghosts));

	for (uint32_t i = 1; i <= game.maxclients; i++) {
		ent = g_edicts + i;
		if (!ent->inuse)
			continue;

		G_SetPlayerScore(ent->client, 0);
		ent->client->resp.ctf_state = 0;
		ent->client->resp.ghost = nullptr;

		gi.LocCenter_Print(ent, "******************\n\nMATCH HAS STARTED!\n\n******************");

		if (!ClientIsSpectating(ent->client)) {
			// make up a ghost code
			P_Match_AssignGhost(ent);
			Weapon_Grapple_DoReset(ent->client);
			ent->svflags = SVF_NOCLIENT;
			ent->flags &= ~FL_GODMODE;

			ent->client->respawn_time = level.time + random_time(1_sec, 4_sec);
			ent->client->ps.pmove.pm_type = PM_DEAD;
			ent->client->anim_priority = ANIM_DEATH;
			ent->s.frame = FRAME_death308 - 1;
			ent->client->anim_end = FRAME_death308;
			ent->deadflag = true;
			ent->movetype = MOVETYPE_NOCLIP;
			ent->client->ps.gunindex = 0;
			ent->client->ps.gunskin = 0;
			gi.linkentity(ent);
		}
	}
}

void Match_End() {
	level.match = MATCH_POST;
	gi.LocBroadcast_Print(PRINT_CHAT, "MATCH COMPLETED!\n");

	Teams_CalcScores();

	gi.LocBroadcast_Print(PRINT_HIGH, "RED TEAM:  {} points\n", level.team_scores[TEAM_RED]);
	gi.LocBroadcast_Print(PRINT_HIGH, "BLUE TEAM:  {} points\n", level.team_scores[TEAM_BLUE]);

	if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE])
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_red_wins_points",
			level.team_scores[TEAM_RED] - level.team_scores[TEAM_BLUE]);
	else if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED])
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_blue_wins_points",
			level.team_scores[TEAM_BLUE] - level.team_scores[TEAM_RED]);
	else
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_tie_game");

	EndDMLevel();
}

/*-----------------------------------------------------------------------*/



/*----------------------------------------------------------------------------------*/
/* ADMIN */

void Menu_ReturnToMain(edict_t *ent, pmenuhnd_t *p);

struct admin_settings_t {
	int	 matchlen;
	int	 matchsetuplen;
	int	 matchstartlen;
	bool weaponsstay;
	bool instantitems;
	bool pu_drop;
	bool instantweap;
	bool matchlock;
};

void Admin_UpdateSettings(edict_t *ent, pmenuhnd_t *setmenu);
void Menu_Admin(edict_t *ent, pmenuhnd_t *p);

static void Admin_SettingsApply(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	if (settings->matchlen != timelimit->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} changed the match length to {} minutes.\n",
			ent->client->pers.netname, settings->matchlen);
		if (level.match == MATCH_GAME) {
			// in the middle of a match, change it on the fly
			level.matchtime = (level.matchtime - gtime_t::from_min(timelimit->integer)) + gtime_t::from_min(settings->matchlen);
		}
		;
		gi.cvar_set("timelimit", G_Fmt("{}", settings->matchlen).data());
	}

	if (settings->matchsetuplen != matchsetuptime->value) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} changed the match setup time to {} minutes.\n",
			ent->client->pers.netname, settings->matchsetuplen);
		if (level.match == MATCH_SETUP) {
			// in the middle of a match, change it on the fly
			level.matchtime = (level.matchtime - gtime_t::from_min(matchsetuptime->value)) + gtime_t::from_min(settings->matchsetuplen);
		}
		;
		gi.cvar_set("matchsetuptime", G_Fmt("{}", settings->matchsetuplen).data());
	}

	if (settings->matchstartlen != matchstarttime->value) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} changed the match start time to {} seconds.\n",
			ent->client->pers.netname, settings->matchstartlen);
		if (level.match == MATCH_PREGAME) {
			// in the middle of a match, change it on the fly
			level.matchtime = (level.matchtime - gtime_t::from_sec(matchstarttime->value)) + gtime_t::from_sec(settings->matchstartlen);
		}
		gi.cvar_set("matchstarttime", G_Fmt("{}", settings->matchstartlen).data());
	}

	if (settings->weaponsstay != !!g_dm_weapons_stay->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} weapons stay.\n",
			ent->client->pers.netname, settings->weaponsstay ? "on" : "off");
		gi.cvar_set("g_dm_weapons_stay", settings->weaponsstay ? "1" : "0");
	}

	if (settings->instantitems != !!g_dm_instant_items->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} instant items.\n",
			ent->client->pers.netname, settings->instantitems ? "on" : "off");
		gi.cvar_set("g_dm_instant_items", settings->instantitems ? "1" : "0");
	}

	if (settings->pu_drop != (bool)g_dm_powerup_drop->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} powerup dropping.\n",
			ent->client->pers.netname, settings->pu_drop ? "on" : "off");
		gi.cvar_set("g_dm_powerup_drop", settings->pu_drop ? "1" : "0");
	}

	if (settings->instantweap != !!(g_instant_weapon_switch->integer || g_frenzy->integer)) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} instant weapon switch.\n",
			ent->client->pers.netname, settings->instantweap ? "on" : "off");
		gi.cvar_set("g_instant_weapon_switch", settings->instantweap ? "1" : "0");
	}

	if (settings->matchlock != !!matchlock->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} match lock.\n",
			ent->client->pers.netname, settings->matchlock ? "on" : "off");
		gi.cvar_set("matchlock", settings->matchlock ? "1" : "0");
	}

	PMenu_Close(ent);
	Menu_Admin(ent, p);
}

static void Admin_SettingsCancel(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);
	Menu_Admin(ent, p);
}

static void Admin_ChangeMatchLen(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->matchlen = (settings->matchlen % 60) + 5;
	if (settings->matchlen < 5)
		settings->matchlen = 5;

	Admin_UpdateSettings(ent, p);
}

static void Admin_ChangeMatchSetupLen(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->matchsetuplen = (settings->matchsetuplen % 60) + 5;
	if (settings->matchsetuplen < 5)
		settings->matchsetuplen = 5;

	Admin_UpdateSettings(ent, p);
}

static void Admin_ChangeMatchStartLen(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->matchstartlen = (settings->matchstartlen % 600) + 10;
	if (settings->matchstartlen < 20)
		settings->matchstartlen = 20;

	Admin_UpdateSettings(ent, p);
}

static void Admin_ChangeWeapStay(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->weaponsstay = !settings->weaponsstay;
	Admin_UpdateSettings(ent, p);
}

static void Admin_ChangeInstantItems(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->instantitems = !settings->instantitems;
	Admin_UpdateSettings(ent, p);
}

static void Admin_ChangePowerupDrop(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->pu_drop = !settings->pu_drop;
	Admin_UpdateSettings(ent, p);
}

static void Admin_ChangeInstantWeap(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->instantweap = !settings->instantweap;
	Admin_UpdateSettings(ent, p);
}

static void Admin_ChangeMatchLock(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->matchlock = !settings->matchlock;
	Admin_UpdateSettings(ent, p);
}

void Admin_UpdateSettings(edict_t *ent, pmenuhnd_t *setmenu) {
	int				  i = 2;
	admin_settings_t *settings = (admin_settings_t *)setmenu->arg;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("time limit: {:2} mins", settings->matchlen).data(), PMENU_ALIGN_LEFT, Admin_ChangeMatchLen);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("match setup len: {:2} mins", settings->matchsetuplen).data(), PMENU_ALIGN_LEFT, Admin_ChangeMatchSetupLen);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("match start len: {:2} secs", settings->matchstartlen).data(), PMENU_ALIGN_LEFT, Admin_ChangeMatchStartLen);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("weapons stay: {}", settings->weaponsstay ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, Admin_ChangeWeapStay);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("instant items: {}", settings->instantitems ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, Admin_ChangeInstantItems);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("powerup drops: {}", settings->pu_drop ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, Admin_ChangePowerupDrop);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("instant weapon switch: {}", settings->instantweap ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, Admin_ChangeInstantWeap);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("match lock: {}", settings->matchlock ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, Admin_ChangeMatchLock);
	i++;

	PMenu_Update(ent);
}
#if 0
const pmenu_t def_setmenu[] = {
	{ "*Settings Menu", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchlen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchsetuplen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchstartlen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool weaponsstay;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool instantitems;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool pu_drop;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool instantweap;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool matchlock;
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Apply", PMENU_ALIGN_LEFT, Admin_SettingsApply },
	{ "Cancel", PMENU_ALIGN_LEFT, Admin_SettingsCancel }
};
#endif
const pmenu_t def_setmenu[] = {
	{ "*Settings Menu", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchlen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchsetuplen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchstartlen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool weaponsstay;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool instantitems;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool pu_drop;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool instantweap;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool matchlock;
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", PMENU_ALIGN_LEFT, Menu_ReturnToMain }
};

void Admin_Settings(edict_t *ent, pmenuhnd_t *p) {
	admin_settings_t *settings;
	pmenuhnd_t *menu;

	PMenu_Close(ent);

	settings = (admin_settings_t *)gi.TagMalloc(sizeof(*settings), TAG_LEVEL);

	settings->matchlen = timelimit->integer;
	settings->matchsetuplen = matchsetuptime->integer;
	settings->matchstartlen = matchstarttime->integer;
	settings->weaponsstay = g_dm_weapons_stay->integer;
	settings->instantitems = g_dm_instant_items->integer;
	settings->pu_drop = g_dm_powerup_drop->integer;
	settings->instantweap = g_instant_weapon_switch->integer != 0;
	settings->matchlock = matchlock->integer != 0;

	menu = PMenu_Open(ent, def_setmenu, -1, sizeof(def_setmenu) / sizeof(pmenu_t), settings, nullptr);
	Admin_UpdateSettings(ent, menu);
}

void Admin_MatchSet(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);

	if (level.match == MATCH_SETUP) {
		gi.LocBroadcast_Print(PRINT_CHAT, "Match has been forced to start.\n");
		level.match = MATCH_PREGAME;
		level.matchtime = level.time + gtime_t::from_sec(matchstarttime->value);
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/talk1.wav"), 1, ATTN_NONE, 0);
		level.countdown = false;
	} else if (level.match == MATCH_GAME) {
		gi.LocBroadcast_Print(PRINT_CHAT, "Match has been forced to terminate.\n");
		level.match = MATCH_SETUP;
		level.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
		Match_ResetAllPlayers();
	}
}

void Admin_MatchMode(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);

	if (level.match != MATCH_SETUP) {
		if (competition->integer < 3)
			gi.cvar_set("competition", "2");
		level.match = MATCH_SETUP;
		Match_ResetAllPlayers();
	}
}

void Admin_Reset(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);

	// go back to normal mode
	gi.LocBroadcast_Print(PRINT_CHAT, "Match mode has been terminated, reseting to normal game.\n");
	level.match = MATCH_NONE;
	gi.cvar_set("competition", "1");
	Match_ResetAllPlayers();
}

void Admin_Cancel(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);
}
#if 0
pmenu_t adminmenu[] = {
	{ "*Administration Menu", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr }, // blank
	{ "Settings", PMENU_ALIGN_LEFT, Admin_Settings },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Cancel", PMENU_ALIGN_LEFT, Admin_Cancel },
	{ "", PMENU_ALIGN_CENTER, nullptr },
};
#endif
pmenu_t adminmenu[] = {
	{ "*Administration Menu", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Settings", PMENU_ALIGN_LEFT, Admin_Settings },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", PMENU_ALIGN_LEFT, Menu_ReturnToMain }
};

void Menu_Admin(edict_t *ent, pmenuhnd_t *p) {
	adminmenu[3].text[0] = '\0';
	adminmenu[3].SelectFunc = nullptr;
	adminmenu[4].text[0] = '\0';
	adminmenu[4].SelectFunc = nullptr;
	if (level.match == MATCH_SETUP) {
		Q_strlcpy(adminmenu[3].text, "Force start match", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = Admin_MatchSet;
		Q_strlcpy(adminmenu[4].text, "Reset to pickup mode", sizeof(adminmenu[4].text));
		adminmenu[4].SelectFunc = Admin_Reset;
	} else if (level.match == MATCH_GAME || level.match == MATCH_PREGAME) {
		Q_strlcpy(adminmenu[3].text, "Cancel match", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = Admin_MatchSet;
	} else if (level.match == MATCH_NONE && competition->integer) {
		Q_strlcpy(adminmenu[3].text, "Switch to match mode", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = Admin_MatchMode;
	}

	PMenu_Close(ent);
	PMenu_Open(ent, adminmenu, -1, sizeof(adminmenu) / sizeof(pmenu_t), nullptr, nullptr);
}

/*-----------------------------------------------------------------------*/

void Team_Join_Free(edict_t *ent, pmenuhnd_t *p);
void Team_Join_Red(edict_t *ent, pmenuhnd_t *p);
void Team_Join_Blue(edict_t *ent, pmenuhnd_t *p);
void Team_Join_Spec(edict_t *ent, pmenuhnd_t *p);

void Menu_ReturnToMain(edict_t *ent, pmenuhnd_t *p);
void Menu_ChaseCam(edict_t *ent, pmenuhnd_t *p);
void Menu_HostInfo(edict_t *ent, pmenuhnd_t *p);
void Menu_ServerInfo(edict_t *ent, pmenuhnd_t *p);

static const int jmenu_hostname = 0;
static const int jmenu_gametype = 1;
static const int jmenu_level = 2;
static const int jmenu_match = 3;

static const int jmenu_teams_join_red = 5;
static const int jmenu_teams_join_blue = 6;
static const int jmenu_teams_spec = 7;
static const int jmenu_teams_chase = 8;
static const int jmenu_teams_reqmatch = 10;
static const int jmenu_teams_hostinfo = 11;
static const int jmenu_teams_svinfo = 12;
static const int jmenu_teams_gamerules = 13;
static const int jmenu_teams_admin = 14;

static const int jmenu_free_join = 5;
static const int jmenu_free_spec = 6;
static const int jmenu_free_chase = 7;
static const int jmenu_free_reqmatch = 9;
static const int jmenu_free_hostinfo = 10;
static const int jmenu_free_svinfo = 11;
static const int jmenu_free_gamerules = 12;
static const int jmenu_free_admin = 13;

static const int jmenu_gamemod = 16;
static const int jmenu_notice = 17;

const pmenu_t teams_join_menu[] = {
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_join_red_team", PMENU_ALIGN_LEFT, Team_Join_Red },
	{ "$g_pc_join_blue_team", PMENU_ALIGN_LEFT, Team_Join_Blue },
	{ "Spectate", PMENU_ALIGN_LEFT, Team_Join_Spec },
	{ "$g_pc_chase_camera", PMENU_ALIGN_LEFT, Menu_ChaseCam },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Host Info", PMENU_ALIGN_LEFT, Menu_HostInfo },
	{ "Match Info", PMENU_ALIGN_LEFT, Menu_ServerInfo },
	//{ "Game Rules", PMENU_ALIGN_LEFT, Menu_GameRules },
	{ "", PMENU_ALIGN_LEFT, nullptr },	//gamerules
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr }
};

const pmenu_t free_join_menu[] = {
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Join Game", PMENU_ALIGN_LEFT, Team_Join_Free },
	{ "Spectate", PMENU_ALIGN_LEFT, Team_Join_Spec },
	{ "$g_pc_chase_camera", PMENU_ALIGN_LEFT, Menu_ChaseCam },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Host Info", PMENU_ALIGN_LEFT, Menu_HostInfo },
	{ "Match Info", PMENU_ALIGN_LEFT, Menu_ServerInfo },
	//{ "Game Rules", PMENU_ALIGN_LEFT, Menu_GameRules },
	{ "", PMENU_ALIGN_LEFT, nullptr },	//gamerules
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr }
};

const pmenu_t nochasemenu[] = {
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_no_chase", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_return", PMENU_ALIGN_LEFT, Menu_ReturnToMain }
};

const pmenu_t hostinfomenu[] = {
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_return", PMENU_ALIGN_LEFT, Menu_ReturnToMain }
};

const pmenu_t svinfomenu[] = {
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", PMENU_ALIGN_LEFT, Menu_ReturnToMain }
};

void Team_Join_Free(edict_t *ent, pmenuhnd_t *p) {
	Team_Join(ent, TEAM_FREE, false);
}

void Team_Join_Red(edict_t *ent, pmenuhnd_t *p) {
	Team_Join(ent, !g_teamplay_allow_team_pick->integer ? PickTeam(-1) : TEAM_RED, false);
}

void Team_Join_Blue(edict_t *ent, pmenuhnd_t *p) {
	if (!g_teamplay_allow_team_pick->integer)
		return;

	Team_Join(ent, TEAM_BLUE, false);
}

void Team_Join_Spec(edict_t *ent, pmenuhnd_t *p) {
	Team_Join(ent, TEAM_SPECTATOR, false);
}

static void Menu_NoChaseCamUpdate(edict_t *ent) {
	pmenu_t *entries = ent->client->menu->entries;

	Menu_SetGamemodName(&entries[jmenu_gamemod]);
	Menu_SetGametypeName(&entries[jmenu_gametype]);
	Menu_SetLevelName(&entries[jmenu_level]);
}

void Menu_ChaseCam(edict_t *ent, pmenuhnd_t *p) {
	edict_t *e;

	Team_Join(ent, TEAM_SPECTATOR, false);

	if (ent->client->chase_target) {
		ent->client->chase_target = nullptr;
		ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
		PMenu_Close(ent);
		return;
	}

	for (uint32_t i = 1; i <= game.maxclients; i++) {
		e = g_edicts + i;
		if (e->inuse && e->solid != SOLID_NOT) {
			ent->client->chase_target = e;
			PMenu_Close(ent);
			ent->client->update_chase = true;
			return;
		}
	}

	PMenu_Close(ent);
	PMenu_Open(ent, nochasemenu, -1, sizeof(nochasemenu) / sizeof(pmenu_t), nullptr, Menu_NoChaseCamUpdate);
}

void Menu_ReturnToMain(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);
	Menu_Open_Join(ent);
}

void Menu_RequestMatch(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);

	Voting_Begin(ent, VOTING_MATCH, G_Fmt("{} has requested to switch to competition mode.\n",
		ent->client->pers.netname).data());
}

static void Menu_HostInfo_Update(edict_t *ent) {
	pmenu_t *entries = ent->client->menu->entries;
	int			i = 0;
	bool		limits = false;

	if (hostname->string[0]) {
		Q_strlcpy(entries[i].text, "Server Name:", sizeof(entries[i].text));
		i++;
		Q_strlcpy(entries[i].text, hostname->string, sizeof(entries[i].text));
		i++;
		i++;
	}

	if (g_edicts[1].client) {
		char value[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(g_edicts[1].client->pers.userinfo, "name", value, sizeof(value));

		if (value[0]) {
			Q_strlcpy(entries[i].text, "Host:", sizeof(entries[i].text));
			i++;
			Q_strlcpy(entries[i].text, value, sizeof(entries[i].text));
			i++;
			i++;
		}
	}

	if (g_motd->string[0]) {
		Q_strlcpy(entries[i].text, "Message of the Day:", sizeof(entries[i].text));
		i++;
		// 26 char line width
		// 9 lines
		// = 234

		Q_strlcpy(entries[i].text, g_motd->string, sizeof(entries[i].text));
	}
}

void Menu_HostInfo(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);
	PMenu_Open(ent, hostinfomenu, -1, sizeof(hostinfomenu) / sizeof(pmenu_t), nullptr, Menu_HostInfo_Update);
}

static void Menu_ServerInfo_Update(edict_t *ent) {
	pmenu_t *entries = ent->client->menu->entries;
	int i = 0;
	bool limits = false;
	bool noitems = clanarena->integer || g_instagib->integer || g_nadefest->integer;
	bool infiniteammo = g_instagib->integer || g_nadefest->integer;
	bool items = ItemSpawnsEnabled();

	Q_strlcpy(entries[i].text, "Match Info", sizeof(entries[i].text));
	i++;

	Q_strlcpy(entries[i].text, BREAKER, sizeof(entries[i].text));
	i++;

	Q_strlcpy(entries[i].text, level.gametype_name, sizeof(entries[i].text));
	i++;

	if (level.mapname[0]) {
		Q_strlcpy(entries[i].text, G_Fmt("map: {}", level.mapname).data(), sizeof(entries[i].text));
		i++;
	}

	if (!ctf->integer && fraglimit->integer > 0) {
		Q_strlcpy(entries[i].text, G_Fmt("frag limit: {}", fraglimit->integer).data(), sizeof(entries[i].text));
		i++;
		limits = true;
	} else if (ctf->integer && capturelimit->integer > 0) {
		Q_strlcpy(entries[i].text, G_Fmt("capture limit: {}", capturelimit->integer).data(), sizeof(entries[i].text));
		i++;
		limits = true;
	}

	if (timelimit->integer > 0) {
		Q_strlcpy(entries[i].text, G_Fmt("time limit: {}", timelimit->integer).data(), sizeof(entries[i].text));
		i++;
		limits = true;
	}

	if (limits) {
		Q_strlcpy(entries[i].text, BREAKER, sizeof(entries[i].text));
		i++;
	}

	if (g_instagib->integer) {
		if (g_instagib_splash->integer) {
			Q_strlcpy(entries[i].text, "InstaGib + Rail Splash", sizeof(entries[i].text));
		} else {
			Q_strlcpy(entries[i].text, "InstaGib", sizeof(entries[i].text));
		}
		i++;
	}
	if (g_vampiric_damage->integer) {
		Q_strlcpy(entries[i].text, "Vampiric Damage", sizeof(entries[i].text));
		i++;
	}
	if (g_frenzy->integer) {
		Q_strlcpy(entries[i].text, "Weapons Frenzy", sizeof(entries[i].text));
		i++;
	}
	if (g_nadefest->integer) {
		Q_strlcpy(entries[i].text, "Nade Fest", sizeof(entries[i].text));
		i++;
	}
	if (g_quadhog->integer) {
		Q_strlcpy(entries[i].text, "Quad Hog", sizeof(entries[i].text));
		i++;
	}

	Q_strlcpy(entries[i].text, BREAKER, sizeof(entries[i].text));
	i++;

	if (items) {
		if (g_dm_weapons_stay->integer) {
			Q_strlcpy(entries[i].text, "weapons stay", sizeof(entries[i].text));
			i++;
		} else {
			if (g_weapon_respawn_time->integer != 30) {
				Q_strlcpy(entries[i].text, G_Fmt("weapon respawn delay: {}", g_weapon_respawn_time->integer).data(), sizeof(entries[i].text));
				i++;
			}
		}
	}

	if (g_infinite_ammo->integer && !infiniteammo) {
		Q_strlcpy(entries[i].text, "infinite ammo", sizeof(entries[i].text));
		i++;
	}
	if (IsTeamplay() && g_friendly_fire->integer) {
		Q_strlcpy(entries[i].text, "friendly fire", sizeof(entries[i].text));
		i++;
	}

	if (g_allow_grapple->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, G_Fmt("{}grapple enabled", g_grapple_offhand->integer ? "off-hand " : "").data(), sizeof(entries[i].text));
		i++;
	}

	if (g_inactivity->integer > 0) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, G_Fmt("inactivity timer: {} sec", g_inactivity->integer).data(), sizeof(entries[i].text));
		i++;
	}

	if (g_teleporter_nofreeze->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "no teleporter freeze", sizeof(entries[i].text));
		i++;
	}

	if (IsTeamplay() && g_teamplay_force_balance->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "forced team balancing", sizeof(entries[i].text));
		i++;
	}

	if (g_dm_random_items->integer && !noitems) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "random items", sizeof(entries[i].text));
		i++;
	}

	if (g_dm_force_join->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "forced game joining", sizeof(entries[i].text));
		i++;
	}

	if (!g_dm_powerup_drop->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "no powerup drops", sizeof(entries[i].text));
		i++;
	}

	if (g_knockback_scale->value != 1) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, G_Fmt("knockback scale: {}", g_knockback_scale->value).data(), sizeof(entries[i].text));
		i++;
	}

	if (g_dm_no_self_damage->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "no self-damage", sizeof(entries[i].text));
		i++;
	}

	if (g_dm_no_fall_damage->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "no falling damage", sizeof(entries[i].text));
		i++;
	}

	if (!g_dm_instant_items->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "no instant items", sizeof(entries[i].text));
		i++;
	}

	if (!noitems) {
		if (i >= 16) return;
		if (g_no_items->integer) {
			Q_strlcpy(entries[i].text, "no items", sizeof(entries[i].text));
			i++;
		} else {
			if (i >= 16) return;
			if (g_no_health->integer) {
				Q_strlcpy(entries[i].text, "no health spawns", sizeof(entries[i].text));
				i++;
			}

			if (i >= 16) return;
			if (g_no_armor->integer) {
				Q_strlcpy(entries[i].text, "no armor spawns", sizeof(entries[i].text));
				i++;
			}

			if (i >= 16) return;
			if (g_no_mines->integer) {
				Q_strlcpy(entries[i].text, "no mines", sizeof(entries[i].text));
				i++;
			}
		}
	}

	if (g_dm_allow_exit->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "allow exiting", sizeof(entries[i].text));
		i++;
	}

	if (g_dm_powerups_style->integer && !noitems) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "arena powerups (2 min)", sizeof(entries[i].text));
		i++;
	}

	if (g_mover_speed_scale->value != 1.0f) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, G_Fmt("mover speed scale: {}", g_mover_speed_scale->value).data(), sizeof(entries[i].text));
		i++;
	}

}

void Menu_ServerInfo(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);
	PMenu_Open(ent, svinfomenu, -1, sizeof(svinfomenu) / sizeof(pmenu_t), nullptr, Menu_ServerInfo_Update);
}

static void Menu_GameRules_Update(edict_t *ent) {
	pmenu_t *entries = ent->client->menu->entries;
	int i = 0;
	bool limits = false;

	Q_strlcpy(entries[i].text, "Game Rules", sizeof(entries[i].text));
	i++;

	Q_strlcpy(entries[i].text, BREAKER, sizeof(entries[i].text));
	i++;
	
	Q_strlcpy(entries[i].text, G_Fmt("{}", level.gametype_name).data(), sizeof(entries[i].text));
	i++;
}

void Menu_GameRules(edict_t *ent, pmenuhnd_t *p) {
	PMenu_Close(ent);
	PMenu_Open(ent, svinfomenu, -1, sizeof(svinfomenu) / sizeof(pmenu_t), nullptr, Menu_GameRules_Update);
}

void Menu_Update_Join(edict_t *ent) {
	pmenu_t *entries = ent->client->menu->entries;
	int pmax = maxplayers->integer;
	uint32_t num_red = 0, num_blue = 0, num_free = 0;

	for (uint32_t i = 0; i < game.maxclients; i++) {
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.team == TEAM_FREE)
			num_free++;
		if (game.clients[i].resp.team == TEAM_RED)
			num_red++;
		else if (game.clients[i].resp.team == TEAM_BLUE)
			num_blue++;
	}

	if (pmax < 1) pmax = 1;

	Menu_SetGamemodName(entries + jmenu_gamemod);
	Menu_SetGametypeName(entries + jmenu_gametype);

	if (level.match >= MATCH_PREGAME && matchlock->integer) {
		if (IsTeamplay()) {
			Q_strlcpy(entries[jmenu_teams_join_red].text, "MATCH IS LOCKED", sizeof(entries[jmenu_teams_join_red].text));
			entries[jmenu_teams_join_red].SelectFunc = nullptr;
			Q_strlcpy(entries[jmenu_teams_join_blue].text, "  (entry is not permitted)", sizeof(entries[jmenu_teams_join_blue].text));
			entries[jmenu_teams_join_blue].SelectFunc = nullptr;
		} else {
			Q_strlcpy(entries[jmenu_free_join].text, "MATCH IS LOCKED", sizeof(entries[jmenu_free_join].text));
			entries[jmenu_free_join].SelectFunc = nullptr;
		}
	} else {
		if (IsTeamplay()) {
			if (!g_teamplay_allow_team_pick->integer) {
				Q_strlcpy(entries[jmenu_teams_join_red].text, G_Fmt("Join a Team ({}/{})", num_red + num_blue, pmax).data(), sizeof(entries[jmenu_teams_join_red].text));
				Q_strlcpy(entries[jmenu_teams_join_blue].text, "", sizeof(entries[jmenu_teams_join_blue].text));

				entries[jmenu_teams_join_red].SelectFunc = Team_Join_Red;
				entries[jmenu_teams_join_blue].SelectFunc = nullptr;
			} else {
				if (level.match >= MATCH_PREGAME) {
					Q_strlcpy(entries[jmenu_teams_join_red].text, "Join Red MATCH Team", sizeof(entries[jmenu_teams_join_red].text));
					Q_strlcpy(entries[jmenu_teams_join_blue].text, "Join Blue MATCH Team", sizeof(entries[jmenu_teams_join_blue].text));
				} else {
					Q_strlcpy(entries[jmenu_teams_join_red].text, G_Fmt("Join Red Team ({}/{})", num_red, floor(pmax / 2)).data(), sizeof(entries[jmenu_teams_join_red].text));
					Q_strlcpy(entries[jmenu_teams_join_blue].text, G_Fmt("Join Blue Team ({}/{})", num_blue, floor(pmax / 2)).data(), sizeof(entries[jmenu_teams_join_blue].text));
				}
				entries[jmenu_teams_join_red].SelectFunc = Team_Join_Red;
				entries[jmenu_teams_join_blue].SelectFunc = Team_Join_Blue;
			}
		} else {
			if (level.match >= MATCH_PREGAME) {
				Q_strlcpy(entries[jmenu_free_join].text, "Join MATCH", sizeof(entries[jmenu_free_join].text));
			} else {
				Q_strlcpy(entries[jmenu_free_join].text, G_Fmt("Join Game ({}/{})", num_free, pmax).data(), sizeof(entries[jmenu_free_join].text));
			}
			entries[jmenu_free_join].SelectFunc = Team_Join_Free;
		}
	}

	// KEX_FIXME: what's this for?
	if (g_dm_force_join->string && *g_dm_force_join->string) {
		if (IsTeamplay()) {
			if (Q_strcasecmp(g_dm_force_join->string, "red") == 0) {
				entries[jmenu_teams_join_blue].text[0] = '\0';
				entries[jmenu_teams_join_blue].SelectFunc = nullptr;
			} else if (Q_strcasecmp(g_dm_force_join->string, "blue") == 0) {
				entries[jmenu_teams_join_red].text[0] = '\0';
				entries[jmenu_teams_join_red].SelectFunc = nullptr;
			}
		}
	}

	int index = IsTeamplay() ? jmenu_teams_chase : jmenu_free_chase;
	if (ent->client->chase_target)
		Q_strlcpy(entries[index].text, "$g_pc_leave_chase_camera", sizeof(entries[index].text));
	else
		Q_strlcpy(entries[index].text, "$g_pc_chase_camera", sizeof(entries[index].text));

	Menu_SetHostName(entries + jmenu_hostname);
	Menu_SetGametypeName(entries + jmenu_gametype);
	Menu_SetLevelName(entries + jmenu_level);

	Menu_SetGamemodName(entries + jmenu_gamemod);

	switch (level.match) {
	case MATCH_NONE:
		entries[jmenu_match].text[0] = '\0';
		break;

	case MATCH_SETUP:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH SETUP IN PROGRESS", sizeof(entries[jmenu_match].text));
		break;

	case MATCH_PREGAME:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH STARTING", sizeof(entries[jmenu_match].text));
		break;

	case MATCH_GAME:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH IN PROGRESS", sizeof(entries[jmenu_match].text));
		break;

	default:
		Q_strlcpy(entries[jmenu_match].text, BREAKER, sizeof(entries[jmenu_match].text));
		break;
	}

	int admin_index = IsTeamplay() ? jmenu_teams_admin : jmenu_free_admin;
	if (ent->client->resp.admin) {
		Q_strlcpy(entries[admin_index].text, "Admin", sizeof(entries[admin_index].text));
		entries[admin_index].SelectFunc = Menu_Admin;
	} else {
		Q_strlcpy(entries[admin_index].text, "", sizeof(entries[admin_index].text));
		entries[admin_index].SelectFunc = nullptr;
	}

	Q_strlcpy(entries[jmenu_notice].text, "github.com/themuffinator", sizeof(entries[jmenu_notice].text));
}

void Menu_Open_Join(edict_t *ent) {
	if (IsTeamplay()) {
		int team = TEAM_SPECTATOR;
		uint32_t num_red = 0, num_blue = 0;

		for (uint32_t i = 0; i < game.maxclients; i++) {
			if (!g_edicts[i + 1].inuse)
				continue;
			if (game.clients[i].resp.team == TEAM_RED)
				num_red++;
			else if (game.clients[i].resp.team == TEAM_BLUE)
				num_blue++;
		}


		if (num_red > num_blue)
			team = TEAM_RED;
		else if (num_blue > num_red)
			team = TEAM_BLUE;
		else
			team = brandom() ? TEAM_RED : TEAM_BLUE;

		PMenu_Open(ent, teams_join_menu, team, sizeof(teams_join_menu) / sizeof(pmenu_t), nullptr, Menu_Update_Join);
	} else {
		PMenu_Open(ent, free_join_menu, TEAM_FREE, sizeof(free_join_menu) / sizeof(pmenu_t), nullptr, Menu_Update_Join);
	}

}
