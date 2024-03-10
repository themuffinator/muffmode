// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "g_statusbar.h"

/*
======================================================================

INTERMISSION

======================================================================
*/

void DeathmatchScoreboard(edict_t *ent);

void MoveClientToIntermission(edict_t *ent) {
	// [Paril-KEX]
	if (ent->client->ps.pmove.pm_type != PM_FREEZE)
		ent->s.event = EV_OTHER_TELEPORT;
	if (deathmatch->integer)
		ent->client->showscores = true;
	ent->s.origin = level.intermission_origin;
	ent->client->ps.pmove.origin = level.intermission_origin;
	ent->client->ps.viewangles = level.intermission_angle;
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunskin = 0;
	ent->client->ps.damage_blend[3] = ent->client->ps.screen_blend[3] = 0;
	ent->client->ps.rdflags = RDF_NONE;

	// clean up powerup info
	ent->client->pu_time_quad = 0_ms;
	ent->client->pu_time_protection = 0_ms;
	ent->client->pu_time_rebreather = 0_ms;
	ent->client->pu_time_enviro = 0_ms;
	ent->client->pu_time_invisibility = 0_ms;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0_ms;

	ent->client->showhelp = false;
	ent->client->showscores = false;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// RAFAEL
	ent->client->pu_time_duelfire = 0_ms;
	// RAFAEL
	// ROGUE
	ent->client->ir_time = 0_ms;
	ent->client->nuke_time = 0_ms;
	ent->client->pu_time_double = 0_ms;
	ent->client->tracker_pain_time = 0_ms;
	// ROGUE

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = EF_NONE;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;
	ent->movetype = MOVETYPE_NOCLIP;

	gi.linkentity(ent);

	// add the layout

	if (deathmatch->integer) {
		DeathmatchScoreboard(ent);
		ent->client->showscores = true;
	}
}

// [Paril-KEX] update the level entry for end-of-unit screen
void G_UpdateLevelEntry() {
	if (!level.entry)
		return;

	level.entry->found_secrets = level.found_secrets;
	level.entry->total_secrets = level.total_secrets;
	level.entry->killed_monsters = level.killed_monsters;
	level.entry->total_monsters = level.total_monsters;
}

inline void G_EndOfUnitEntry(std::stringstream &layout, const int &y, const level_entry_t &entry) {
	layout << G_Fmt("yv {} ", y);

	// we didn't visit this level, so print it as an unknown entry
	if (!*entry.pretty_name) {
		layout << "table_row 1 ??? ";
		return;
	}

	layout << G_Fmt("table_row 4 \"{}\" ", entry.pretty_name) <<
		G_Fmt("{}/{} ", entry.killed_monsters, entry.total_monsters) <<
		G_Fmt("{}/{} ", entry.found_secrets, entry.total_secrets);

	int32_t minutes = entry.time.milliseconds() / 60000;
	int32_t seconds = (entry.time.milliseconds() / 1000) % 60;
	int32_t milliseconds = entry.time.milliseconds() % 1000;

	layout << G_Fmt("{:02}:{:02}:{:03} ", minutes, seconds, milliseconds);
}

void G_EndOfUnitMessage() {
	// [Paril-KEX] update game level entry
	G_UpdateLevelEntry();

	std::stringstream layout;

	// sort entries
	std::sort(game.level_entries.begin(), game.level_entries.end(), [](const level_entry_t &a, const level_entry_t &b) {
		int32_t a_order = a.visit_order ? a.visit_order : (*a.pretty_name ? (MAX_LEVELS_PER_UNIT + 1) : (MAX_LEVELS_PER_UNIT + 2));
		int32_t b_order = b.visit_order ? b.visit_order : (*b.pretty_name ? (MAX_LEVELS_PER_UNIT + 1) : (MAX_LEVELS_PER_UNIT + 2));

		return a_order < b_order;
		});

	layout << "start_table 4 $m_eou_level $m_eou_kills $m_eou_secrets $m_eou_time ";

	int y = 16;
	level_entry_t totals{};
	int32_t num_rows = 0;

	for (auto &entry : game.level_entries) {
		if (!*entry.map_name)
			break;

		G_EndOfUnitEntry(layout, y, entry);

		y += 8;

		totals.found_secrets += entry.found_secrets;
		totals.killed_monsters += entry.killed_monsters;
		totals.time += entry.time;
		totals.total_monsters += entry.total_monsters;
		totals.total_secrets += entry.total_secrets;

		if (entry.visit_order)
			num_rows++;
	}

	y += 8;

	// make this a space so it prints totals
	if (num_rows > 1) {
		layout << "table_row 0 "; // empty row to separate totals
		totals.pretty_name[0] = ' ';
		G_EndOfUnitEntry(layout, y, totals);
	}

	layout << "xv 160 yt 0 draw_table ";

	layout << "ifgef " << (level.intermission_server_frame + (5_sec).frames()) << " yb -48 xv 0 loc_cstring2 0 \"$m_eou_press_button\" endif ";

	gi.WriteByte(svc_layout);
	gi.WriteString(layout.str().c_str());
	gi.multicast(vec3_origin, MULTICAST_ALL, true);

	for (auto player : active_players())
		player->client->showeou = true;
}

// data is binary now.
// u8 num_teams
// u8 num_players
// [ repeat num_teams:
//   string team_name
// ]
// [ repeat num_players:
//   u8 client_index
//   s32 score
//   u8 ranking
//   (if num_teams > 0)
//     u8 team
// ]
void G_ReportMatchDetails(bool is_end) {
	static std::array<uint32_t, MAX_CLIENTS> player_ranks;

	player_ranks = {};

	// teamplay is simple
	if (IsTeamplay()) {
		Teams_CalcRankings(player_ranks);

		gi.WriteByte(2);
		gi.WriteString("RED TEAM"); // team 0
		gi.WriteString("BLUE TEAM"); // team 1
	} else {
		// sort players by score, then match everybody to
		// the current highest score downwards until we run out of players.
		static std::array<edict_t *, MAX_CLIENTS> sorted_players;
		size_t num_active_players = 0;

		for (auto player : active_players())
			sorted_players[num_active_players++] = player;

		std::sort(sorted_players.begin(), sorted_players.begin() + num_active_players, [](const edict_t *a, const edict_t *b) { return b->client->resp.score < a->client->resp.score; });

		int32_t current_score = INT_MIN;
		int32_t current_rank = 0;

		for (size_t i = 0; i < num_active_players; i++) {
			if (!current_rank || sorted_players[i]->client->resp.score != current_score) {
				current_rank++;
				current_score = sorted_players[i]->client->resp.score;
			}

			player_ranks[sorted_players[i]->s.number - 1] = current_rank;
		}

		gi.WriteByte(0);
	}

	uint8_t num_players = 0;

	for (auto player : active_players()) {
		// leave spectators out of this data, they don't need to be seen.
		if (player->client->pers.spawned && !ClientIsSpectating(player->client)) {
			// just in case...
			if (IsTeamplay() && ClientIsSpectating(player->client))
				continue;

			num_players++;
		}
	}

	gi.WriteByte(num_players);

	for (auto player : active_players()) {
		// leave spectators out of this data, they don't need to be seen.
		if (player->client->pers.spawned && !ClientIsSpectating(player->client)) {
			// just in case...
			if (IsTeamplay() && ClientIsSpectating(player->client))
				continue;

			gi.WriteByte(player->s.number - 1);
			gi.WriteLong(player->client->resp.score);
			gi.WriteByte(player_ranks[player->s.number - 1]);

			if (IsTeamplay())
				gi.WriteByte(player->client->resp.team == TEAM_RED ? 0 : 1);
		}
	}

	gi.ReportMatchDetails_Multicast(is_end);
}

void BeginIntermission(edict_t *targ) {
	edict_t *ent, *client;

	if (level.intermission_time)
		return; // already activated

	// ZOID
	if (ctf->integer)
		Teams_CalcScores();
	// ZOID
#if 0
	// if in a duel, change the wins / losses
	if (duel->integer) {
		GT_Duel_MatchEnd_AdjustScores();
#endif
	game.autosaved = false;

	level.intermission_time = level.time;

	// respawn any dead clients
	for (uint32_t i = 0; i < game.maxclients; i++) {
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0) {
			// give us our max health back since it will reset
			// to pers.health; in instanced items we'd lose the items
			// we touched so we always want to respawn with our max.
			if (P_UseCoopInstancedItems())
				client->client->pers.health = client->client->pers.max_health = client->max_health;

			respawn(client);
		}
	}

	level.intermission_server_frame = gi.ServerFrame();
	level.changemap = targ->map;
	level.intermission_clear = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY);
	level.intermission_eou = false;
	level.intermission_fade = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_FADE_OUT);

	// destroy all player trails
	PlayerTrail_Destroy(nullptr);

	// [Paril-KEX] update game level entry
	G_UpdateLevelEntry();

	if (strstr(level.changemap, "*")) {
		if (coop->integer) {
			for (uint32_t i = 0; i < game.maxclients; i++) {
				client = g_edicts + 1 + i;
				if (!client->inuse)
					continue;
				// strip players of all keys between units
				for (uint32_t n = 0; n < IT_TOTAL; n++)
					if (itemlist[n].flags & IF_KEY)
						client->client->pers.inventory[n] = 0;
			}
		}

		if (level.achievement && level.achievement[0]) {
			gi.WriteByte(svc_achievement);
			gi.WriteString(level.achievement);
			gi.multicast(vec3_origin, MULTICAST_ALL, true);
		}

		level.intermission_eou = true;

		// "no end of unit" maps handle intermission differently
		if (!targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT))
			G_EndOfUnitMessage();
		else if (targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE) && !deathmatch->integer) {
			// Need to call this now
			G_ReportMatchDetails(true);
			level.intermission_exit = 1; // go immediately to the next level
			return;
		}
	} else {
		if (!deathmatch->integer) {
			level.intermission_exit = 1; // go immediately to the next level
			return;
		}
	}

	// Call while intermission is running
	G_ReportMatchDetails(true);

	level.intermission_exit = 0;

	if (!level.level_intermission_set) {
		// find an intermission spot
		ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_intermission");
		if (!ent) { // the map creator forgot to put in an intermission point...
			ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_start");
			if (!ent)
				ent = G_FindByString<&edict_t::classname>(nullptr, "info_player_deathmatch");
		} else { // choose one of four spots
			int32_t i = irandom(4);
			while (i--) {
				ent = G_FindByString<&edict_t::classname>(ent, "info_player_intermission");
				if (!ent) // wrap around the list
					ent = G_FindByString<&edict_t::classname>(ent, "info_player_intermission");
			}
		}

		level.intermission_origin = ent->s.origin;
		level.intermission_angle = ent->s.angles;
	}

	// move all clients to the intermission point
	for (uint32_t i = 0; i < game.maxclients; i++) {
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		MoveClientToIntermission(client);
	}
}

/*
==================
TeamsScoreboardMessage
==================
*/
void TeamsScoreboardMessage(edict_t * ent, edict_t * killer) {
	uint32_t	i, j, k, n;
	uint32_t	sorted[2][MAX_CLIENTS];
	int32_t		sortedscores[2][MAX_CLIENTS];
	int			score;
	uint32_t	total[2];
	int			totalscore[2];
	uint32_t	last[2];
	gclient_t *cl;
	edict_t *cl_ent;
	int			team;

	// sort the clients by team and score
	total[0] = total[1] = 0;
	last[0] = last[1] = 0;
	totalscore[0] = totalscore[1] = 0;
	for (i = 0; i < game.maxclients; i++) {
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (game.clients[i].resp.team == TEAM_RED)
			team = 0;
		else if (game.clients[i].resp.team == TEAM_BLUE)
			team = 1;
		else
			continue; // unknown team?

		score = game.clients[i].resp.score;
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
		totalscore[team] += score;
		total[team]++;
	}

	// print level name and exit rules
	// add the clients in sorted order
	static std::string string;
	string.clear();

	//fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yv -30 cstring2 \"{}\" "), G_Fmt("{} - {}", level.gamemod_name, level.gametype_name));
	//fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yv -20 cstring2 \"{}\" "), G_Fmt("[{}] {}", level.mapname, level.level_name));

	// [Paril-KEX] time & frags
	if (teamplay->integer) {
		if (fraglimit->integer) {
			fmt::format_to(std::back_inserter(string), FMT_STRING("xv -20 yv -10 loc_string2 1 $g_score_frags \"{}\" "), fraglimit->integer);
		}
	} else {
		if (capturelimit->integer) {
			fmt::format_to(std::back_inserter(string), FMT_STRING("xv -20 yv -10 loc_string2 1 $g_score_captures \"{}\" "), capturelimit->integer);
		}
	}
	if (timelimit->value) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 time_limit {} "), gi.ServerFrame() + ((gtime_t::from_min(timelimit->value) - level.time)).milliseconds() / gi.frame_time_ms);
		//fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 loc_string2 1 {} "), gi.ServerFrame() + level.time.milliseconds() / gi.frame_time_ms);
	}

	// team one
	if (teamplay->integer) {
		fmt::format_to(std::back_inserter(string),
			FMT_STRING("if 25 xv -32 yv 8 pic 25 endif "
				"xv -123 yv 28 cstring \"{}\" "
				"xv 41 yv 12 num 3 19 "
				"if 26 xv 208 yv 8 pic 26 endif "
				"xv 117 yv 28 cstring \"{}\" "
				"xv 280 yv 12 num 3 21 "),
			total[0],
			total[1]);
	} else {
		fmt::format_to(std::back_inserter(string),
			FMT_STRING("if 25 xv -32 yv 8 pic 25 endif "
				"xv 0 yv 28 string \"{:4}/{:<3}\" "
				"xv 58 yv 12 num 2 19 "
				"if 26 xv 208 yv 8 pic 26 endif "
				"xv 240 yv 28 string \"{:4}/{:<3}\" "
				"xv 296 yv 12 num 2 21 "),
			totalscore[0], total[0],
			totalscore[1], total[1]);
	}

	for (i = 0; i < 16; i++) {
		if (i >= total[0] && i >= total[1])
			break; // we're done

		// left side
		if (i < total[0]) {
			cl = &game.clients[sorted[0][i]];
			cl_ent = g_edicts + 1 + sorted[0][i];

			std::string_view entry = G_Fmt("ctf -40 {} {} {} {} {} ",
				42 + i * 8,
				sorted[0][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping,
				cl_ent->client->pers.inventory[IT_FLAG_BLUE] ? "sbfctf2" : "\"\"");

			if (string.size() + entry.size() < MAX_STRING_CHARS) {
				string += entry;
				last[0] = i;
			}
		}

		// right side
		if (i < total[1]) {
			cl = &game.clients[sorted[1][i]];
			cl_ent = g_edicts + 1 + sorted[1][i];

			std::string_view entry = G_Fmt("ctf 200 {} {} {} {} {} ",
				42 + i * 8,
				sorted[1][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping,
				cl_ent->client->pers.inventory[IT_FLAG_RED] ? "sbfctf1" : "\"\"");

			if (string.size() + entry.size() < MAX_STRING_CHARS) {
				string += entry;
				last[1] = i;
			}
		}
	}

	// put in spectators if we have enough room
	if (last[0] > last[1])
		j = last[0];
	else
		j = last[1];
	j = (j + 2) * 8 + 42;

	k = n = 0;
	if (string.size() < MAX_STRING_CHARS - 50) {
		for (i = 0; i < game.maxclients; i++) {
			cl_ent = g_edicts + 1 + i;
			cl = &game.clients[i];
			if (!cl_ent->inuse ||
				cl_ent->solid != SOLID_NOT ||
				!ClientIsSpectating(cl_ent->client))
				continue;

			if (!k) {
				k = 1;
				fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yv {} loc_string2 0 \"$g_pc_spectators\" "), j);
				j += 8;
			}

			std::string_view entry = G_Fmt("ctf {} {} {} {} {} \"\" ",
				(n & 1) ? 200 : -40,	// x
				j,						// y
				i,						// playernum
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (string.size() + entry.size() < MAX_STRING_CHARS)
				string += entry;

			if (n & 1)
				j += 8;
			n++;
		}
	}

	if (total[0] - last[0] > 1) // couldn't fit everyone
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv -32 yv {} loc_string 1 $g_ctf_and_more {} "),
			42 + (last[0] + 1) * 8, total[0] - last[0] - 1);
	if (total[1] - last[1] > 1) // couldn't fit everyone
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 208 yv {} loc_string 1 $g_ctf_and_more {} "),
			42 + (last[1] + 1) * 8, total[1] - last[1] - 1);

	if (level.intermission_time) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("ifgef {} yb -48 xv 0 loc_cstring2 0 \"$m_eou_press_button\" endif "), (level.intermission_server_frame + (5_sec).frames()));
	} else
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yb -48 cstring2 \"{}\" "), "Show inventory to toggle menu.");

	gi.WriteByte(svc_layout);
	gi.WriteString(string.c_str());
}

/*
==================
DuelScoreboardMessage
==================
*/
static void DuelScoreboardMessage(edict_t * ent, edict_t * killer) {
	uint32_t	i, j, k, n;
	gclient_t	*cl;
	edict_t		*cl_ent;

	static std::string string;
	string.clear();

	char value[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(g_edicts[1].client->pers.userinfo, "name", value, sizeof(value));

	if (fraglimit->integer) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv -20 yv -10 loc_string2 1 $g_score_frags \"{}\" "), fraglimit->integer);
	}
	if (timelimit->value) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 time_limit {} "), gi.ServerFrame() + ((gtime_t::from_min(timelimit->value) - level.time)).milliseconds() / gi.frame_time_ms);
		//fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 loc_string2 1 {} "), gi.ServerFrame() + level.time.milliseconds() / gi.frame_time_ms);
	}
	/*
	fmt::format_to(std::back_inserter(string),
		FMT_STRING(
			"if {} xv -32 yv 8 pic {} endif "
			"xv -123 yv 28 cstring \"{}\" "
			"xv 41 yv 12 num 3 19 "
			"if {} xv 208 yv 8 pic {} endif "
			"xv 117 yv 28 cstring \"{}\" "
			"xv 280 yv 12 num 3 21 "
		),
		STAT_DUEL_HEADER, STAT_DUEL_HEADER,
		value,
		STAT_DUEL_HEADER, STAT_DUEL_HEADER,
		value
	);
	*/
	j = 58;

	k = n = 0;
	if (string.size() < MAX_STRING_CHARS - 50) {
		for (i = 0; i < game.maxclients; i++) {
			cl_ent = g_edicts + 1 + i;
			cl = &game.clients[i];
			if (!cl_ent->inuse ||
				cl_ent->solid != SOLID_NOT ||
				!ClientIsSpectating(cl_ent->client))
				continue;

			if (!k) {
				k = 1;
				fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yv {} loc_string2 0 \"$g_pc_spectators\" "), j);
				j += 8;
			}

			std::string_view entry = G_Fmt("ctf {} {} {} {} {} \"\" ",
				(n & 1) ? 200 : -40,	// x
				j,						// y
				i,						// playernum
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (string.size() + entry.size() < MAX_STRING_CHARS)
				string += entry;

			if (n & 1)
				j += 8;
			n++;
		}
	}

	if (level.intermission_time) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("ifgef {} yb -48 xv 0 loc_cstring2 0 \"$m_eou_press_button\" endif "), (level.intermission_server_frame + (5_sec).frames()));
	} else
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yb -48 cstring2 \"{}\" "), "Show inventory to toggle menu.");

	gi.WriteByte(svc_layout);
	gi.WriteString(string.c_str());
}

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage(edict_t * ent, edict_t * killer) {
	static std::string entry, string;
	size_t		j;
	int			sorted[MAX_CLIENTS];
	int			sortedscores[MAX_CLIENTS];
	int			score;
	int			x, y;
	gclient_t *cl;
	edict_t *cl_ent;

	if (IsTeamplay()) {
		TeamsScoreboardMessage(ent, ent->enemy);
		return;
	}
	if (duel->integer) {
		DuelScoreboardMessage(ent, ent->enemy);
		return;
	}

	entry.clear();
	string.clear();

	//  sort the clients by score
	uint32_t total = 0;
	for (uint32_t i = 0; i < game.maxclients; i++) {
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || ClientIsSpectating(&game.clients[i]))
			continue;
		score = game.clients[i].resp.score;
		for (j = 0; j < total; j++) {
			if (score > sortedscores[j])
				break;
		}
		for (uint32_t k = total; k > j; k--) {
			sorted[k] = sorted[k - 1];
			sortedscores[k] = sortedscores[k - 1];
		}
		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}

	// add the clients in sorted order
	if (total > 16)
		total = 16;

	for (uint32_t i = 0; i < total; i++) {
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];

		x = (i >= 8) ? 130 : -72;
		y = 0 + 32 * (i % 8);

		if (cl_ent == ent)
			fmt::format_to(std::back_inserter(entry), FMT_STRING("xv {} yv {} picn {} "), x, y, "/tags/default");
		else if (cl_ent == killer)
			fmt::format_to(std::back_inserter(entry), FMT_STRING("xv {} yv {} picn {} "), x, y, "/tags/bloody");

		char vs[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(cl->pers.userinfo, "skin", vs, sizeof(vs));

		if (vs[0]) {
			const char *s = G_Fmt("/players/{}_i", vs).data();
			int32_t		img_index = cl->pers.skin_icon_index;

			if (img_index)
				fmt::format_to(std::back_inserter(entry), FMT_STRING("xv {} yv {} picn {} "), x, y, s);
		}

		if (string.length() + entry.length() > MAX_STRING_CHARS)
			break;

		string += entry;

		entry.clear();

		fmt::format_to(std::back_inserter(entry),
			FMT_STRING("client {} {} {} {} {} {} "),
			x, y, sorted[i], cl->resp.score, cl->ping, (int32_t)(level.time - cl->resp.entertime).minutes());

		if (string.length() + entry.length() > MAX_STRING_CHARS)
			break;

		string += entry;

		entry.clear();
	}

	//fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yv -30 cstring2 \"{}\" "), G_Fmt("{} - {}", level.gamemod_name, level.gametype_name));
	if (ent->client && ent->client->resp.team != TEAM_SPECTATOR && ent->client->resp.score && level.num_playing_clients > 1) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yv -20 cstring2 \"{}\" "), G_Fmt("{}{} place with a score of {}",
			level.intermission_time ? "You finished " : "", G_PlaceString(ent->client->resp.rank + 1), ent->client->resp.score));
	}
	if (fraglimit->integer) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv -20 yv -10 loc_string2 1 $g_score_frags \"{}\" "), fraglimit->integer);
	}
	if (timelimit->value && !level.intermission_time) {
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 time_limit {} "), gi.ServerFrame() + ((gtime_t::from_min(timelimit->value) - level.time)).milliseconds() / gi.frame_time_ms);
#if 0
		//fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 loc_string2 1 {} "), gi.ServerFrame() + level.time.milliseconds() / gi.frame_time_ms);
		int32_t val = gi.ServerFrame() + ((gtime_t::from_min(timelimit->value) - level.time)).milliseconds() / gi.frame_time_ms;
		const char *s;
		int32_t remaining_ms = gtime_t::from_ms(level.time);	// (val - gi.ServerFrame()) *gi.frame_time_ms;

		s = G_Fmt("{:02}:{:02}", (remaining_ms / 1000) / 60, (remaining_ms / 1000) % 60).data();

		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 loc_string2 1 \"{}\" "), s);
#endif
	}

	if (level.intermission_time)
		fmt::format_to(std::back_inserter(string), FMT_STRING("ifgef {} yb -48 xv 0 loc_cstring2 0 \"$m_eou_press_button\" endif "), (level.intermission_server_frame + (5_sec).frames()));
	else
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yb -48 cstring2 \"{}\" "), "Show inventory to toggle menu.");

	gi.WriteByte(svc_layout);
	gi.WriteString(string.c_str());
}

/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard(edict_t * ent) {

	DeathmatchScoreboardMessage(ent, ent->enemy);
	gi.unicast(ent, true);
	ent->client->menutime = level.time + 3_sec;
}

/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f(edict_t * ent) {
	if (level.intermission_time)
		return;

	ent->client->showinventory = false;
	ent->client->showhelp = false;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// ZOID
	if (ent->client->menu)
		PMenu_Close(ent);
	// ZOID

	if (!deathmatch->integer && !coop->integer)
		return;

	if (ent->client->showscores) {
		ent->client->showscores = false;
		ent->client->update_chase = true;
		return;
	}

	//globals.server_flags |= SERVER_FLAG_SLOW_TIME;
	ent->client->showscores = true;
	DeathmatchScoreboard(ent);
}

/*
==================
HelpComputer

Draw help computer.
==================
*/
static void HelpComputer(edict_t * ent) {
	const char *sk;

	if (skill->integer == 0)
		sk = "$m_easy";
	else if (skill->integer == 1)
		sk = "$m_medium";
	else if (skill->integer == 2)
		sk = "$m_hard";
	else
		sk = "$m_nightmare";

	// send the layout

	std::string helpString = "";
	helpString += G_Fmt(
		"xv 32 yv 8 picn help "		   // background
		"xv 0 yv 25 cstring2 \"{}\" ",  // level name
		level.level_name);

	if (level.is_n64) {
		helpString += G_Fmt("xv 0 yv 54 loc_cstring 1 \"{{}}\" \"{}\" ",  // help 1
			game.helpmessage1);
	} else {
		int y = 54;
		if (strlen(game.helpmessage1)) {
			helpString += G_Fmt("xv 0 yv {} loc_cstring2 0 \"$g_pc_primary_objective\" "  // title
				"xv 0 yv {} loc_cstring 0 \"{}\" ",
				y,
				y + 11,
				game.helpmessage1);

			y += 58;
		}

		if (strlen(game.helpmessage2)) {
			helpString += G_Fmt("xv 0 yv {} loc_cstring2 0 \"$g_pc_secondary_objective\" "  // title
				"xv 0 yv {} loc_cstring 0 \"{}\" ",
				y,
				y + 11,
				game.helpmessage2);
		}

	}

	helpString += G_Fmt("xv 55 yv 164 loc_string2 0 \"{}\" "
		"xv 265 yv 164 loc_rstring2 1 \"{{}}: {}/{}\" \"$g_pc_goals\" "
		"xv 55 yv 172 loc_string2 1 \"{{}}: {}/{}\" \"$g_pc_kills\" "
		"xv 265 yv 172 loc_rstring2 1 \"{{}}: {}/{}\" \"$g_pc_secrets\" ",
		sk,
		level.found_goals, level.total_goals,
		level.killed_monsters, level.total_monsters,
		level.found_secrets, level.total_secrets);

	gi.WriteByte(svc_layout);
	gi.WriteString(helpString.c_str());
	gi.unicast(ent, true);
}

/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f(edict_t * ent) {
	// this is for backwards compatability
	if (deathmatch->integer) {
		Cmd_Score_f(ent);
		return;
	}

	if (level.intermission_time)
		return;

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp &&
		(ent->client->pers.game_help1changed == game.help1changed ||
			ent->client->pers.game_help2changed == game.help2changed)) {
		ent->client->showhelp = false;
		globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	globals.server_flags |= SERVER_FLAG_SLOW_TIME;
	HelpComputer(ent);
}

//=======================================================================

// [Paril-KEX] for stats we want to always be set in coop
// even if we're spectating
void G_SetCoopStats(edict_t * ent) {
	if (coop->integer && g_coop_enable_lives->integer)
		ent->client->ps.stats[STAT_LIVES] = ent->client->pers.lives + 1;
	else
		ent->client->ps.stats[STAT_LIVES] = 0;

	// stat for text on what we're doing for respawn
	if (ent->client->coop_respawn_state)
		ent->client->ps.stats[STAT_COOP_RESPAWN] = CONFIG_COOP_RESPAWN_STRING + (ent->client->coop_respawn_state - COOP_RESPAWN_IN_COMBAT);
	else
		ent->client->ps.stats[STAT_COOP_RESPAWN] = 0;
}

struct powerup_info_t {
	item_id_t item;
	gtime_t gclient_t:: *time_ptr = nullptr;
	int32_t gclient_t:: *count_ptr = nullptr;
} powerup_table[] = {
	{ IT_POWERUP_QUAD, &gclient_t::pu_time_quad },
	{ IT_POWERUP_DUELFIRE, &gclient_t::pu_time_duelfire },
	{ IT_POWERUP_DOUBLE, &gclient_t::pu_time_double },
	{ IT_POWERUP_PROTECTION, &gclient_t::pu_time_protection },
	{ IT_POWERUP_INVISIBILITY, &gclient_t::pu_time_invisibility },
	{ IT_POWERUP_ENVIROSUIT, &gclient_t::pu_time_enviro },
	{ IT_POWERUP_REBREATHER, &gclient_t::pu_time_rebreather },
	{ IT_ITEM_IR_GOGGLES, &gclient_t::ir_time },
	{ IT_POWERUP_SILENCER, nullptr, &gclient_t::silencer_shots }
};


static void SetCrosshairIDView(edict_t * ent) {
	vec3_t	 forward, dir;
	trace_t	 tr;
	edict_t *who, *best;
	float	 bd = 0, d;

	// only check every few frames
	if (level.time - ent->client->resp.lastidtime < 250_ms)
		return;
	ent->client->resp.lastidtime = level.time;

	ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = 0;
	ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = 0;

	AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
	forward *= 1024;
	forward = ent->s.origin + forward;
	tr = gi.traceline(ent->s.origin, forward, ent, MASK_SOLID);
	if (tr.fraction < 1 && tr.ent && tr.ent->client && tr.ent->health > 0) {
		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = (tr.ent - g_edicts);
		if (tr.ent->client->resp.team == TEAM_RED)
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_logo_red;
		else if (tr.ent->client->resp.team == TEAM_BLUE)
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_logo_blue;
		return;
	}

	AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
	best = nullptr;
	for (uint32_t i = 1; i <= game.maxclients; i++) {
		who = g_edicts + i;
		if (!who->inuse || who->solid == SOLID_NOT)
			continue;
		dir = who->s.origin - ent->s.origin;
		dir.normalize();
		d = forward.dot(dir);

		// we have teammate indicators that are better for this
		if (IsTeamplay() && ent->client->resp.team == who->client->resp.team)
			continue;

		if (d > bd && loc_CanSee(ent, who)) {
			bd = d;
			best = who;
		}
	}
	if (bd > 0.90f) {
		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = (best - g_edicts);
		if (best->client->resp.team == TEAM_RED)
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_logo_red;
		else if (best->client->resp.team == TEAM_BLUE)
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_logo_blue;
	}
}


static void SetCTFStats(edict_t *ent, bool blink) {
	uint32_t i;
	int		 p1, p2;
	edict_t *e;

	if (!ctf->integer) return;

	// figure out what icon to display for team logos
	// three states:
	//   flag at base
	//   flag taken
	//   flag dropped
	p1 = ii_ctf_red_default;
	e = G_FindByString<&edict_t::classname>(nullptr, ITEM_CTF_FLAG_RED);
	if (e != nullptr) {
		if (e->solid == SOLID_NOT) {
			// not at base
			// check if on player
			p1 = ii_ctf_red_dropped; // default to dropped
			for (i = 1; i <= game.maxclients; i++)
				if (g_edicts[i].inuse &&
					g_edicts[i].client->pers.inventory[IT_FLAG_RED]) {
					// enemy has it
					p1 = ii_ctf_red_taken;
					break;
				}

			// [Paril-KEX] make sure there is a dropped version on the map somewhere
			if (p1 == ii_ctf_red_dropped) {
				e = G_FindByString<&edict_t::classname>(e, ITEM_CTF_FLAG_RED);

				if (e == nullptr) {
					GT_CTF_ResetFlag(TEAM_RED);
					gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
						Teams_TeamName(TEAM_RED));
					gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
				}
			}
		} else if (e->spawnflags.has(SPAWNFLAG_ITEM_DROPPED))
			p1 = ii_ctf_red_dropped; // must be dropped
	}
	p2 = ii_ctf_blue_default;
	e = G_FindByString<&edict_t::classname>(nullptr, ITEM_CTF_FLAG_BLUE);
	if (e != nullptr) {
		if (e->solid == SOLID_NOT) {
			// not at base
			// check if on player
			p2 = ii_ctf_blue_dropped; // default to dropped
			for (i = 1; i <= game.maxclients; i++)
				if (g_edicts[i].inuse &&
					g_edicts[i].client->pers.inventory[IT_FLAG_BLUE]) {
					// enemy has it
					p2 = ii_ctf_blue_taken;
					break;
				}

			// [Paril-KEX] make sure there is a dropped version on the map somewhere
			if (p2 == ii_ctf_blue_dropped) {
				e = G_FindByString<&edict_t::classname>(e, ITEM_CTF_FLAG_BLUE);

				if (e == nullptr) {
					GT_CTF_ResetFlag(TEAM_BLUE);
					gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
						Teams_TeamName(TEAM_BLUE));
					gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
				}
			}
		} else if (e->spawnflags.has(SPAWNFLAG_ITEM_DROPPED))
			p2 = ii_ctf_blue_dropped; // must be dropped
	}

	ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = p1;
	ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = p2;

	if (level.last_flag_capture && level.time - level.last_flag_capture < 5_sec) {
		if (level.last_capture_team == TEAM_RED)
			if (blink)
				ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = p1;
			else
				ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = 0;
		else if (blink)
			ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = p2;
		else
			ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = 0;
	}

	ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = level.team_scores[TEAM_RED];
	ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = level.team_scores[TEAM_BLUE];

	ent->client->ps.stats[STAT_CTF_FLAG_PIC] = 0;
	if (ent->client->resp.team == TEAM_RED &&
		ent->client->pers.inventory[IT_FLAG_BLUE] &&
		(blink))
		ent->client->ps.stats[STAT_CTF_FLAG_PIC] = ii_ctf_blue_default;

	else if (ent->client->resp.team == TEAM_BLUE &&
		ent->client->pers.inventory[IT_FLAG_RED] &&
		(blink))
		ent->client->ps.stats[STAT_CTF_FLAG_PIC] = ii_ctf_red_default;
}


static void SetExtendedStats(edict_t * ent) {
	bool teams = IsTeamplay();
	int16_t	pos1_num = -1, pos2_num = -1;
	int16_t own_num = -1;

	bool blink = (level.time.milliseconds() % 1000) < 500;

	ent->client->ps.stats[STAT_MATCH_STATE] = level.match > MATCH_NONE ? CONFIG_CTF_MATCH : 0;
	ent->client->ps.stats[STAT_TEAMPLAY_INFO] = level.warnactive ? CONFIG_CTF_TEAMINFO : 0;

	// ghosting
	if (ent->client->resp.ghost) {
		ent->client->resp.ghost->score = ent->client->resp.score;
		Q_strlcpy(ent->client->resp.ghost->netname, ent->client->pers.netname, sizeof(ent->client->resp.ghost->netname));
		ent->client->resp.ghost->number = ent->s.number;
	}

	// mini score player indexes
	if (!teams) {
		int16_t	own_rank = -1, other_rank = -1;
		int16_t	other_num = -1;
		int16_t	other_other_num = -1;

		if (ent->client->resp.team == TEAM_FREE || ent->client->chase_target) {
			own_num = ent->client->chase_target ? ent->client->chase_target->client - game.clients : ent->client - game.clients;
			own_rank = game.clients[own_num].resp.rank;
			own_rank &= ~RANK_TIED_FLAG;
		}

		// find opponent positions
		for (size_t i = 0; i < MAX_CLIENTS; i++) {

			if (level.sorted_clients[i] < 0)
				continue;

			if (level.sorted_clients[i] >= MAX_CLIENTS)
				continue;

			if (level.sorted_clients[i] == own_num)
				continue;

			if (!game.clients[level.sorted_clients[i]].pers.connected)
				continue;

			if (game.clients[level.sorted_clients[i]].resp.team == TEAM_SPECTATOR)
				continue;

			if (i && !level.sorted_clients[i - 1] && !level.sorted_clients[i])
				break;

			if (other_num < 0 && other_other_num < 0) {
				other_rank = i;
				other_num = level.sorted_clients[i];

				if (own_rank >= 0)
					break;
				continue;
			}

			if (other_rank >= 0) {
				other_other_num = level.sorted_clients[i];
				break;
			}
		}

		// set miniscore indexes
		if (own_rank >= 0) {
			// client is playing
			if (own_rank == 0) {
				pos1_num = own_num;
				pos2_num = other_num >= 0 ? other_num : other_other_num;
			} else {
				pos1_num = other_num >= 0 ? other_num : other_other_num;
				pos2_num = own_num;
			}
		} else {
			// client is spectating
			pos1_num = other_num;
			pos2_num = other_other_num;
		}

		if (duel->integer) {
			ent->client->ps.stats[STAT_DUEL_HEADER] = ii_duel_header;
		}

		//if (own_num == 0)
		//	gi.Com_PrintFmt("own_num={} own_rank={} pos1_num={} pos2_num={}\n", own_num, own_rank, pos1_num, pos2_num);
	} else {
		// logo headers for the frag display
		ent->client->ps.stats[STAT_TEAM_RED_HEADER] = ii_teams_header_red;
		ent->client->ps.stats[STAT_TEAM_BLUE_HEADER] = ii_teams_header_blue;

		// if during intermission, we must blink the team header of the winning team
		if (level.intermission_time && blink) {
			// blink half second
			// note that level.total[12] is set when we go to intermission
			if (level.team_scores[TEAM_RED] > level.team_scores[TEAM_BLUE])
				ent->client->ps.stats[STAT_TEAM_RED_HEADER] = 0;
			else if (level.team_scores[TEAM_BLUE] > level.team_scores[TEAM_RED])
				ent->client->ps.stats[STAT_TEAM_BLUE_HEADER] = 0;
			else { // tie game!
				ent->client->ps.stats[STAT_TEAM_RED_HEADER] = 0;
				ent->client->ps.stats[STAT_TEAM_BLUE_HEADER] = 0;
			}
		}
	}

	// set miniscores scores and images
	if (ctf->integer) {
		SetCTFStats(ent, blink);
	} else {
		if (teams) {
			ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = level.team_scores[TEAM_RED];
			ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = level.team_scores[TEAM_BLUE];
		} else {
			int16_t pic1 = 0, pic2 = 0;

			ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = -999;
			if (pos1_num >= 0) {
				pic1 = game.clients[pos1_num].pers.skin_icon_index;
				ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = game.clients[pos1_num].resp.score;
			}
			ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = pic1;

			ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = -999;
			if (pos2_num >= 0) {
				pic2 = game.clients[pos2_num].pers.skin_icon_index;
				ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = game.clients[pos2_num].resp.score;
			}
			ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = pic2;

			ent->client->ps.stats[STAT_SCORELIMIT] = fraglimit->integer;
		}
	}

	// highlight minescores position/team
	ent->client->ps.stats[STAT_MINISCORE_FIRST_POS] = 0;
	ent->client->ps.stats[STAT_MINISCORE_SECOND_POS] = 0;
	if (teams) {
		if (ent->client->resp.team == TEAM_RED)
			ent->client->ps.stats[STAT_MINISCORE_FIRST_POS] = ii_highlight;
		else if (ent->client->resp.team == TEAM_BLUE)
			ent->client->ps.stats[STAT_MINISCORE_SECOND_POS] = ii_highlight;
	} else {
		if (own_num >= 0) {
			if (own_num == pos1_num)
				ent->client->ps.stats[STAT_MINISCORE_FIRST_POS] = ii_highlight;
			else if (own_num == pos2_num)
				ent->client->ps.stats[STAT_MINISCORE_SECOND_POS] = ii_highlight;
		}
	}

	// set crosshair ID
	if (ent->client->resp.id_state)
		SetCrosshairIDView(ent);
	else {
		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = 0;
		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = 0;
	}
}

/*
===============
G_SetStats
===============
*/
void G_SetStats(edict_t * ent) {
	gitem_t *item;
	item_id_t		index;
	int				cells = 0;
	item_id_t		power_armor_type;
	unsigned int	invIndex;
	bool			minhud = g_instagib->integer || g_nadefest->integer;
	int				i;	//for techs

	int32_t		img_index;
#if 0
	char vm[MAX_INFO_VALUE] = { 0 };
	char vs[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(ent->client->pers.userinfo, "model", vm, sizeof(vm));
	gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", vs, sizeof(vs));
#endif
	img_index = ent->client->pers.skin_icon_index;	// gi.imageindex(G_Fmt("players{}/{}_i", vm, vs).data());

	//
	// health
	//
	if (ent->s.renderfx & RF_USE_DISGUISE)
		ent->client->ps.stats[STAT_HEALTH_ICON] = level.disguise_icon;
	else
		ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	if (!minhud) {
		//
		// weapons
		//
		uint32_t weaponbits = 0;

		for (invIndex = IT_WEAPON_GRAPPLE; invIndex <= IT_WEAPON_DISRUPTOR; invIndex++) {
			if (ent->client->pers.inventory[invIndex]) {
				weaponbits |= 1 << GetItemByIndex((item_id_t)invIndex)->weapon_wheel_index;
			}
		}

		ent->client->ps.stats[STAT_WEAPONS_OWNED_1] = (weaponbits & 0xFFFF);
		ent->client->ps.stats[STAT_WEAPONS_OWNED_2] = (weaponbits >> 16);

		ent->client->ps.stats[STAT_ACTIVE_WHEEL_WEAPON] = (ent->client->newweapon ? ent->client->newweapon->weapon_wheel_index :
			ent->client->pers.weapon ? ent->client->pers.weapon->weapon_wheel_index :
			-1);
		ent->client->ps.stats[STAT_ACTIVE_WEAPON] = ent->client->pers.weapon ? ent->client->pers.weapon->weapon_wheel_index : -1;

		//
		// ammo
		//
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;

		if (ent->client->pers.weapon && ent->client->pers.weapon->ammo) {
			item = GetItemByIndex(ent->client->pers.weapon->ammo);

			if (!G_CheckInfiniteAmmo(item)) {
				ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(item->icon);
				ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->pers.weapon->ammo];
			}
		}

		memset(&ent->client->ps.stats[STAT_AMMO_INFO_START], 0, sizeof(uint16_t) * NUM_AMMO_STATS);
		for (unsigned int ammoIndex = AMMO_BULLETS; ammoIndex < AMMO_MAX; ++ammoIndex) {
			gitem_t *ammo = GetItemByAmmo((ammo_t)ammoIndex);
			uint16_t val = G_CheckInfiniteAmmo(ammo) ? AMMO_VALUE_INFINITE : clamp(ent->client->pers.inventory[ammo->id], 0, AMMO_VALUE_INFINITE - 1);
			G_SetAmmoStat((uint16_t *)&ent->client->ps.stats[STAT_AMMO_INFO_START], ammo->ammo_wheel_index, val);
		}

		//
		// armor
		//
		power_armor_type = PowerArmorType(ent);
		if (power_armor_type)
			cells = ent->client->pers.inventory[IT_AMMO_CELLS];

		index = ArmorIndex(ent);
		if (power_armor_type && (!index || (level.time.milliseconds() % 3000) < 1500)) { // flash between power armor and other armor icon
			ent->client->ps.stats[STAT_ARMOR_ICON] = power_armor_type == IT_ITEM_POWER_SHIELD ? gi.imageindex("i_powershield") : gi.imageindex("i_powerscreen");
			ent->client->ps.stats[STAT_ARMOR] = cells;
		} else if (index) {
			item = GetItemByIndex(index);
			ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(item->icon);
			ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
		} else {
			ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
			ent->client->ps.stats[STAT_ARMOR] = 0;
		}

		//
		// pickup message
		//
		if (level.time > ent->client->pickup_msg_time) {
			ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
			ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
		}

		// owned powerups
		memset(&ent->client->ps.stats[STAT_POWERUP_INFO_START], 0, sizeof(uint16_t) * NUM_POWERUP_STATS);
		for (unsigned int powerupIndex = POWERUP_SCREEN; powerupIndex < POWERUP_MAX; ++powerupIndex) {
			gitem_t *powerup = GetItemByPowerup((powerup_t)powerupIndex);
			uint16_t val;

			switch (powerup->id) {
			case IT_ITEM_POWER_SCREEN:
			case IT_ITEM_POWER_SHIELD:
				if (!ent->client->pers.inventory[powerup->id])
					val = 0;
				else if (ent->flags & FL_POWER_ARMOR)
					val = 2;
				else
					val = 1;
				break;
			case IT_ITEM_FLASHLIGHT:
				if (!ent->client->pers.inventory[powerup->id])
					val = 0;
				else if (ent->flags & FL_FLASHLIGHT)
					val = 2;
				else
					val = 1;
				break;
			default:
				val = clamp(ent->client->pers.inventory[powerup->id], 0, 3);
				break;
			}

			G_SetPowerupStat((uint16_t *)&ent->client->ps.stats[STAT_POWERUP_INFO_START], powerup->powerup_wheel_index, val);
		}

		ent->client->ps.stats[STAT_TIMER_ICON] = 0;
		ent->client->ps.stats[STAT_TIMER] = 0;

		//
		// timers
		//
		// PGM
		if (ent->client->owned_sphere) {
			if (ent->client->owned_sphere->spawnflags == SPHERE_DEFENDER) // defender
				ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_defender");
			else if (ent->client->owned_sphere->spawnflags == SPHERE_HUNTER) // hunter
				ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_hunter");
			else if (ent->client->owned_sphere->spawnflags == SPHERE_VENGEANCE) // vengeance
				ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_vengeance");
			else // error case
				ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("i_fixme");

			ent->client->ps.stats[STAT_TIMER] = ceil(ent->client->owned_sphere->wait - level.time.seconds());
		} else {
			powerup_info_t *best_powerup = nullptr;

			for (auto &powerup : powerup_table) {
				auto *powerup_time = powerup.time_ptr ? &(ent->client->*powerup.time_ptr) : nullptr;
				auto *powerup_count = powerup.count_ptr ? &(ent->client->*powerup.count_ptr) : nullptr;

				if (powerup_time && *powerup_time <= level.time)
					continue;
				else if (powerup_count && !*powerup_count)
					continue;

				if (!best_powerup) {
					best_powerup = &powerup;
					continue;
				}

				if (powerup_time && *powerup_time < ent->client->*best_powerup->time_ptr) {
					best_powerup = &powerup;
					continue;
				} else if (powerup_count && !best_powerup->time_ptr) {
					best_powerup = &powerup;
					continue;
				}
			}

			if (best_powerup) {
				int16_t value;

				if (best_powerup->count_ptr)
					value = (ent->client->*best_powerup->count_ptr);
				else
					value = ceil((ent->client->*best_powerup->time_ptr - level.time).seconds());

				ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex(GetItemByIndex(best_powerup->item)->icon);
				ent->client->ps.stats[STAT_TIMER] = value;
			}
		}
		// PGM

		//
		// selected item
		//
		ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

		if (ent->client->pers.selected_item == IT_NULL)
			ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
		else {
			ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex(itemlist[ent->client->pers.selected_item].icon);

			if (ent->client->pers.selected_item_time < level.time)
				ent->client->ps.stats[STAT_SELECTED_ITEM_NAME] = 0;
		}
	}

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->integer) {
		if (ent->client->pers.health <= 0 || level.intermission_time || ent->client->showscores)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;
	} else {
		if (ent->client->showscores || ent->client->showhelp || ent->client->showeou)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;

		if (ent->client->showhelp)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HELP;
	}

	if (level.intermission_time || ent->client->awaiting_respawn) {
		if (ent->client->awaiting_respawn || (level.intermission_eou || level.is_n64 || (deathmatch->integer && (ent->client->showscores || level.intermission_time))))
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HIDE_HUD;

		// N64 always merges into one screen on level ends
		if (level.intermission_eou || level.is_n64 || (deathmatch->integer && level.intermission_time))
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INTERMISSION;
	}


	if (deathmatch->integer && ClientIsSpectating(ent->client) && !ent->client->ps.stats[STAT_CHASE])
		ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HIDE_CROSSHAIR;
	else {
		if (level.story_active)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HIDE_CROSSHAIR;
		else
			ent->client->ps.stats[STAT_LAYOUTS] &= ~LAYOUTS_HIDE_CROSSHAIR;
	}

	// [Paril-KEX] key display
	if (!deathmatch->integer) {
		int32_t key_offset = 0;
		player_stat_t stat = STAT_KEY_A;

		ent->client->ps.stats[STAT_KEY_A] =
			ent->client->ps.stats[STAT_KEY_B] =
			ent->client->ps.stats[STAT_KEY_C] = 0;

		// there's probably a way to do this in one pass but
		// I'm lazy
		std::array<item_id_t, IT_TOTAL> keys_held;
		size_t num_keys_held = 0;

		for (auto &item : itemlist) {
			if (!(item.flags & IF_KEY))
				continue;
			else if (!ent->client->pers.inventory[item.id])
				continue;

			keys_held[num_keys_held++] = item.id;
		}

		if (num_keys_held > 3)
			key_offset = (int32_t)(level.time.seconds() / 5);

		for (int32_t i = 0; i < min(num_keys_held, (size_t)3); i++, stat = (player_stat_t)(stat + 1))
			ent->client->ps.stats[stat] = gi.imageindex(GetItemByIndex(keys_held[(i + key_offset) % num_keys_held])->icon);
	}

	//
	// help icon / current weapon if not shown
	//
	if (ent->client->pers.helpchanged >= 1 && ent->client->pers.helpchanged <= 2 && (level.time.milliseconds() % 1000) < 500) // haleyjd: time-limited
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex("i_help");
	else if ((ent->client->pers.hand == CENTER_HANDED) && ent->client->pers.weapon) {
		if (!minhud || (minhud && ent->client->pers.weapon->id == IT_WEAPON_GRAPPLE))
			ent->client->ps.stats[STAT_HELPICON] = gi.imageindex(ent->client->pers.weapon->icon);
	} else
		ent->client->ps.stats[STAT_HELPICON] = 0;

	ent->client->ps.stats[STAT_SPECTATOR] = 0;

	// set & run the health bar stuff
	for (size_t i = 0; i < MAX_HEALTH_BARS; i++) {
		byte *health_byte = reinterpret_cast<byte *>(&ent->client->ps.stats[STAT_HEALTH_BARS]) + i;

		if (!level.health_bar_entities[i])
			*health_byte = 0;
		else if (level.health_bar_entities[i]->timestamp) {
			if (level.health_bar_entities[i]->timestamp < level.time) {
				level.health_bar_entities[i] = nullptr;
				*health_byte = 0;
				continue;
			}

			*health_byte = 0b10000000;
		} else {
			// enemy dead
			if (!level.health_bar_entities[i]->enemy->inuse || level.health_bar_entities[i]->enemy->health <= 0) {
				// hack for Makron
				if (level.health_bar_entities[i]->enemy->monsterinfo.aiflags & AI_DOUBLE_TROUBLE) {
					*health_byte = 0b10000000;
					continue;
				}

				if (level.health_bar_entities[i]->delay) {
					level.health_bar_entities[i]->timestamp = level.time + gtime_t::from_sec(level.health_bar_entities[i]->delay);
					*health_byte = 0b10000000;
				} else {
					level.health_bar_entities[i] = nullptr;
					*health_byte = 0;
				}

				continue;
			} else if (level.health_bar_entities[i]->spawnflags.has(SPAWNFLAG_HEALTHBAR_PVS_ONLY) && !gi.inPVS(ent->s.origin, level.health_bar_entities[i]->enemy->s.origin, true)) {
				*health_byte = 0;
				continue;
			}

			float health_remaining = ((float)level.health_bar_entities[i]->enemy->health) / level.health_bar_entities[i]->enemy->max_health;
			*health_byte = ((byte)(health_remaining * 0b01111111)) | 0b10000000;
		}
	}

	// tech icon
	i = 0;
	ent->client->ps.stats[STAT_TECH] = 0;
	for (; i < q_countof(tech_ids); i++) {
		if (ent->client->pers.inventory[tech_ids[i]]) {
			ent->client->ps.stats[STAT_TECH] = gi.imageindex(GetItemByIndex(tech_ids[i])->icon);
			break;
		}
	}

	SetExtendedStats(ent);

	//
	// match timer
	//
#if 0
	if (g_match_timer->integer) {
		// Don't update any more than once/second
		static int lasttime = 0;
		int	t;
		if (timelimit->value)
			t = (gtime_t::from_min(timelimit->value) - level.time).seconds<int>();
		else
			t = level.time.seconds<int>();
		if (t != ent->client->last_match_timer_update) {
			ent->client->last_match_timer_update = t;
			char game_timer[64];
			G_FmtTo(game_timer, "{:02}:{:02}", t / 60, t % 60);
			ent->client->ps.stats[STAT_MATCH_TIMER] = CONFIG_MATCH_TIMER;
			gi.configstring(STAT_MATCH_TIMER, game_timer);
		}
	}
#endif
}

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats(edict_t * ent) {
	gclient_t *cl;

	for (uint32_t i = 1; i <= game.maxclients; i++) {
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;
		cl->ps.stats = ent->client->ps.stats;
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats(edict_t * ent) {
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		G_SetStats(ent);

	cl->ps.stats[STAT_SPECTATOR] = 1;

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if (cl->pers.health <= 0 || level.intermission_time || cl->showscores)
		cl->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;
	if (cl->showinventory && cl->pers.health > 0)
		cl->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS +
		(cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats[STAT_CHASE] = 0;
}
