// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "monsters/m_player.h"

#include <assert.h>

constexpr const char *BREAKER = "\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37";

bool Vote_Menu_Active(gentity_t *ent) {
	if (level.vote_time <= 0_sec)
		return false;

	if (!level.vote_client)
		return false;

	if (ent->client->pers.voted)
		return false;

	return true;
}

static void G_Menu_SetHostName(menu_t *p) {
	Q_strlcpy(p->text, hostname->string, sizeof(p->text));
}

static void G_Menu_SetGamemodName(menu_t *p) {
	Q_strlcpy(p->text, level.gamemod_name, sizeof(p->text));
}

static void G_Menu_SetGametypeName(menu_t *p) {
	Q_strlcpy(p->text, level.gametype_name, sizeof(p->text));
}

static void G_Menu_SetLevelName(menu_t *p) {
	static char levelname[33];

	levelname[0] = '*';
	if (g_entities[0].message)
		Q_strlcpy(levelname + 1, g_entities[0].message, sizeof(levelname) - 1);
	else
		Q_strlcpy(levelname + 1, level.mapname, sizeof(levelname) - 1);
	levelname[sizeof(levelname) - 1] = 0;
	Q_strlcpy(p->text, levelname, sizeof(p->text));
}

/*----------------------------------------------------------------------------------*/
/* ADMIN */

void G_Menu_ReturnToMain(gentity_t *ent, menu_hnd_t *p);

struct admin_settings_t {
	int	 timelimit;
	bool weaponsstay;
	bool instantitems;
	bool pu_drop;
	bool instantweap;
	bool match_lock;
};

void G_Menu_Admin_UpdateSettings(gentity_t *ent, menu_hnd_t *setmenu);
void G_Menu_Admin(gentity_t *ent, menu_hnd_t *p);

static void G_Menu_Admin_SettingsApply(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	if (settings->timelimit != timelimit->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} changed the timelimit to {} minutes.\n",
			ent->client->resp.netname, settings->timelimit);

		gi.cvar_set("timelimit", G_Fmt("{}", settings->timelimit).data());
	}

	if (settings->weaponsstay != !!g_dm_weapons_stay->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} weapons stay.\n",
			ent->client->resp.netname, settings->weaponsstay ? "on" : "off");
		gi.cvar_set("g_dm_weapons_stay", settings->weaponsstay ? "1" : "0");
	}

	if (settings->instantitems != !!g_dm_instant_items->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} instant items.\n",
			ent->client->resp.netname, settings->instantitems ? "on" : "off");
		gi.cvar_set("g_dm_instant_items", settings->instantitems ? "1" : "0");
	}

	if (settings->pu_drop != (bool)g_dm_powerup_drop->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} powerup dropping.\n",
			ent->client->resp.netname, settings->pu_drop ? "on" : "off");
		gi.cvar_set("g_dm_powerup_drop", settings->pu_drop ? "1" : "0");
	}

	if (settings->instantweap != !!(g_instant_weapon_switch->integer || g_frenzy->integer)) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} instant weapon switch.\n",
			ent->client->resp.netname, settings->instantweap ? "on" : "off");
		gi.cvar_set("g_instant_weapon_switch", settings->instantweap ? "1" : "0");
	}

	if (settings->match_lock != !!g_match_lock->integer) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} match lock.\n",
			ent->client->resp.netname, settings->match_lock ? "on" : "off");
		gi.cvar_set("g_match_lock", settings->match_lock ? "1" : "0");
	}

	P_Menu_Close(ent);
	G_Menu_Admin(ent, p);
}

static void G_Menu_Admin_SettingsCancel(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	G_Menu_Admin(ent, p);
}

static void G_Menu_Admin_ChangeMatchLen(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->timelimit = (settings->timelimit % 60) + 5;
	if (settings->timelimit < 5)
		settings->timelimit = 5;

	G_Menu_Admin_UpdateSettings(ent, p);
}

static void G_Menu_Admin_ChangeMatchSetupLen(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	G_Menu_Admin_UpdateSettings(ent, p);
}

static void G_Menu_Admin_ChangeMatchStartLen(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	G_Menu_Admin_UpdateSettings(ent, p);
}

static void G_Menu_Admin_ChangeWeapStay(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->weaponsstay = !settings->weaponsstay;
	G_Menu_Admin_UpdateSettings(ent, p);
}

static void G_Menu_Admin_ChangeInstantItems(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->instantitems = !settings->instantitems;
	G_Menu_Admin_UpdateSettings(ent, p);
}

static void G_Menu_Admin_ChangePowerupDrop(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->pu_drop = !settings->pu_drop;
	G_Menu_Admin_UpdateSettings(ent, p);
}

static void G_Menu_Admin_ChangeInstantWeap(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->instantweap = !settings->instantweap;
	G_Menu_Admin_UpdateSettings(ent, p);
}

static void G_Menu_Admin_ChangeMatchLock(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings = (admin_settings_t *)p->arg;

	settings->match_lock = !settings->match_lock;
	G_Menu_Admin_UpdateSettings(ent, p);
}

void G_Menu_Admin_UpdateSettings(gentity_t *ent, menu_hnd_t *setmenu) {
	int				  i = 2;
	admin_settings_t *settings = (admin_settings_t *)setmenu->arg;

	P_Menu_UpdateEntry(setmenu->entries + i, G_Fmt("time limit: {:2} mins", settings->timelimit).data(), MENU_ALIGN_LEFT, G_Menu_Admin_ChangeMatchLen);
	i++;

	P_Menu_UpdateEntry(setmenu->entries + i, G_Fmt("weapons stay: {}", settings->weaponsstay ? "Yes" : "No").data(), MENU_ALIGN_LEFT, G_Menu_Admin_ChangeWeapStay);
	i++;

	P_Menu_UpdateEntry(setmenu->entries + i, G_Fmt("instant items: {}", settings->instantitems ? "Yes" : "No").data(), MENU_ALIGN_LEFT, G_Menu_Admin_ChangeInstantItems);
	i++;

	P_Menu_UpdateEntry(setmenu->entries + i, G_Fmt("powerup drops: {}", settings->pu_drop ? "Yes" : "No").data(), MENU_ALIGN_LEFT, G_Menu_Admin_ChangePowerupDrop);
	i++;

	P_Menu_UpdateEntry(setmenu->entries + i, G_Fmt("instant weapon switch: {}", settings->instantweap ? "Yes" : "No").data(), MENU_ALIGN_LEFT, G_Menu_Admin_ChangeInstantWeap);
	i++;

	P_Menu_UpdateEntry(setmenu->entries + i, G_Fmt("match lock: {}", settings->match_lock ? "Yes" : "No").data(), MENU_ALIGN_LEFT, G_Menu_Admin_ChangeMatchLock);
	i++;

	P_Menu_Update(ent);
}

const menu_t def_setmenu[] = {
	{ "*Settings Menu", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr }, // int timelimit;
	{ "", MENU_ALIGN_LEFT, nullptr }, // bool weaponsstay;
	{ "", MENU_ALIGN_LEFT, nullptr }, // bool instantitems;
	{ "", MENU_ALIGN_LEFT, nullptr }, // bool pu_drop;
	{ "", MENU_ALIGN_LEFT, nullptr }, // bool instantweap;
	{ "", MENU_ALIGN_LEFT, nullptr }, // bool g_match_lock;
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

static void G_Menu_Admin_Settings(gentity_t *ent, menu_hnd_t *p) {
	admin_settings_t *settings;
	menu_hnd_t *menu;

	P_Menu_Close(ent);

	settings = (admin_settings_t *)gi.TagMalloc(sizeof(*settings), TAG_LEVEL);

	settings->timelimit = timelimit->integer;
	settings->weaponsstay = g_dm_weapons_stay->integer;
	settings->instantitems = g_dm_instant_items->integer;
	settings->pu_drop = !!g_dm_powerup_drop->integer;
	settings->instantweap = g_instant_weapon_switch->integer != 0;
	settings->match_lock = g_match_lock->integer != 0;

	menu = P_Menu_Open(ent, def_setmenu, -1, sizeof(def_setmenu) / sizeof(menu_t), settings, true, nullptr);
	G_Menu_Admin_UpdateSettings(ent, menu);
}

static void G_Menu_Admin_MatchSet(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);

	if (level.match_state <= matchst_t::MATCH_COUNTDOWN) {
		gi.LocBroadcast_Print(PRINT_CHAT, "Match has been forced to start.\n");
		Match_Start();
	} else if (level.match_state == matchst_t::MATCH_IN_PROGRESS) {
		gi.LocBroadcast_Print(PRINT_CHAT, "Match has been forced to terminate.\n");
		Match_Reset();
	}
}

static void G_Menu_Admin_Cancel(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
}

menu_t adminmenu[] = {
	{ "*Administration Menu", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "Settings", MENU_ALIGN_LEFT, G_Menu_Admin_Settings },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

void G_Menu_Admin(gentity_t *ent, menu_hnd_t *p) {
	adminmenu[3].text[0] = '\0';
	adminmenu[3].SelectFunc = nullptr;
	adminmenu[4].text[0] = '\0';
	adminmenu[4].SelectFunc = nullptr;

	if (level.match_state <= matchst_t::MATCH_COUNTDOWN) {
		Q_strlcpy(adminmenu[3].text, "Force start match", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = G_Menu_Admin_MatchSet;

	} else if (level.match_state == matchst_t::MATCH_IN_PROGRESS) {
		Q_strlcpy(adminmenu[3].text, "Reset match", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = G_Menu_Admin_MatchSet;
	}

	P_Menu_Close(ent);
	P_Menu_Open(ent, adminmenu, -1, sizeof(adminmenu) / sizeof(menu_t), nullptr, false, nullptr);
}

/*-----------------------------------------------------------------------*/

const menu_t pmstatsmenu[] = {
	{ "Player Match Stats", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

static void G_Menu_PMStats_Update(gentity_t *ent) {

	if (!g_matchstats->integer)
		return;

	menu_t *entries = ent->client->menu->entries;
	client_match_stats_t *st = &ent->client->mstats;
	int i = 0;
	char value[MAX_INFO_VALUE] = { 0 };
	if (game.maxclients > 0 && g_entities[1].client) {
		gi.Info_ValueForKey(g_entities[1].client->pers.userinfo, "name", value, sizeof(value));
	}

	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, "Player Stats for Match", sizeof(entries[i].text));
	i++;

	if (value[0]) {
		if (i < ent->client->menu->num)
			Q_strlcpy(entries[i].text, G_Fmt("{}", value).data(), sizeof(entries[i].text));
		i++;
	}

	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, BREAKER, sizeof(entries[i].text));
	i++;

	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, G_Fmt("kills: {}", st->total_kills).data(), sizeof(entries[i].text));
	i++;
	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, G_Fmt("deaths: {}", st->total_deaths).data(), sizeof(entries[i].text));
	i++;
	if (st->total_kills) {
		if (i < ent->client->menu->num) {
			if (st->total_deaths > 0) {
				float val = (float)st->total_kills / (float)st->total_deaths;
				Q_strlcpy(entries[i].text, G_Fmt("k/d ratio: {:2}", val).data(), sizeof(entries[i].text));
			} else {
				Q_strlcpy(entries[i].text, "k/d ratio: N/A", sizeof(entries[i].text));
			}
		}
		i++;
	}
	if (i < ent->client->menu->num)
		entries[i].text[0] = '\0';
	i++;
	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, G_Fmt("dmg dealt: {}", st->total_dmg_dealt).data(), sizeof(entries[i].text));
	i++;
	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, G_Fmt("dmg received: {}", st->total_dmg_received).data(), sizeof(entries[i].text));
	i++;
	if (st->total_dmg_dealt) {
		if (i < ent->client->menu->num) {
			if (st->total_dmg_received > 0) {
				float val = (float)st->total_dmg_dealt / (float)st->total_dmg_received;
				Q_strlcpy(entries[i].text, G_Fmt("dmg ratio: {:02}", val).data(), sizeof(entries[i].text));
			} else {
				Q_strlcpy(entries[i].text, "dmg ratio: N/A", sizeof(entries[i].text));
			}
		}
		i++;
	}
	if (i < ent->client->menu->num)
		entries[i].text[0] = '\0';
	i++;
	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, G_Fmt("shots fired: {}", st->total_shots).data(), sizeof(entries[i].text));
	i++;
	if (i < ent->client->menu->num)
		Q_strlcpy(entries[i].text, G_Fmt("shots on target: {}", st->total_hits).data(), sizeof(entries[i].text));
	i++;
	if (st->total_hits) {
		if (i < ent->client->menu->num) {
			if (st->total_shots > 0) {
				int val = (int)(((float)st->total_hits / (float)st->total_shots) * 100.f);
				Q_strlcpy(entries[i].text, G_Fmt("total accuracy: {}%", val).data(), sizeof(entries[i].text));
			} else {
				Q_strlcpy(entries[i].text, "total accuracy: N/A", sizeof(entries[i].text));
			}
		}
		i++;
	}
}

static void G_Menu_PMStats(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	P_Menu_Open(ent, pmstatsmenu, -1, sizeof(pmstatsmenu) / sizeof(menu_t), nullptr, false, G_Menu_PMStats_Update);
}

/*-----------------------------------------------------------------------*/
static const int cvmenu_map = 3;
static const int cvmenu_nextmap = 4;
static const int cvmenu_restart = 5;
static const int cvmenu_gametype = 6;
static const int cvmenu_timelimit = 7;
static const int cvmenu_scorelimit = 8;
static const int cvmenu_shuffle = 9;
static const int cvmenu_balance = 10;
static const int cvmenu_unlagged = 11;
static const int cvmenu_cointoss = 12;
static const int cvmenu_random = 13;

void G_Menu_CallVote_Map(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_NextMap(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_Restart(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_GameType(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_TimeLimit_Update(gentity_t *ent);
void G_Menu_CallVote_TimeLimit(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_ScoreLimit(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_ShuffleTeams(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_BalanceTeams(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_Unlagged(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_Cointoss(gentity_t *ent, menu_hnd_t *p);
void G_Menu_CallVote_Random(gentity_t *ent, menu_hnd_t *p);

void G_Menu_CallVote_Map_Selection(gentity_t *ent, menu_hnd_t *p);

const menu_t pmcallvotemenu[] = {
	{ "Call a Vote", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "change map", MENU_ALIGN_LEFT, G_Menu_CallVote_Map },
	{ "go to next map", MENU_ALIGN_LEFT, G_Menu_CallVote_NextMap },
	{ "restart match", MENU_ALIGN_LEFT, G_Menu_CallVote_Restart },
	{ "change gametype", MENU_ALIGN_LEFT, G_Menu_CallVote_GameType },
	{ "change time limit", MENU_ALIGN_LEFT, G_Menu_CallVote_TimeLimit },
	{ "change score limit", MENU_ALIGN_LEFT, G_Menu_CallVote_ScoreLimit },
	{ "shuffle teams", MENU_ALIGN_LEFT, G_Menu_CallVote_ShuffleTeams },
	{ "balance teams", MENU_ALIGN_LEFT, G_Menu_CallVote_BalanceTeams },
	{ "lag compensation", MENU_ALIGN_LEFT, G_Menu_CallVote_Unlagged },
	{ "generate heads/tails", MENU_ALIGN_LEFT, G_Menu_CallVote_Cointoss },
	{ "generate random number", MENU_ALIGN_LEFT, G_Menu_CallVote_Random },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

const menu_t pmcallvotemenu_map[] = {
	{ "Choose a Map", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection },
	{ "", MENU_ALIGN_LEFT, G_Menu_CallVote_Map_Selection }
};

const menu_t pmcallvotemenu_timelimit[] = {
	{ "Select Time Limit (mins)", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "5", MENU_ALIGN_LEFT, nullptr },
	{ "10", MENU_ALIGN_LEFT, nullptr },
	{ "15", MENU_ALIGN_LEFT, nullptr },
	{ "20", MENU_ALIGN_LEFT, nullptr },
	{ "25", MENU_ALIGN_LEFT, nullptr },
	{ "30", MENU_ALIGN_LEFT, nullptr },
	{ "35", MENU_ALIGN_LEFT, nullptr },
	{ "40", MENU_ALIGN_LEFT, nullptr },
	{ "45", MENU_ALIGN_LEFT, nullptr },
	{ "50", MENU_ALIGN_LEFT, nullptr },
	{ "55", MENU_ALIGN_LEFT, nullptr },
	{ "60", MENU_ALIGN_LEFT, nullptr },
	{ "120", MENU_ALIGN_LEFT, nullptr },
	{ "150", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

void G_Menu_CallVote_Map_Selection(gentity_t *ent, menu_hnd_t *p) {
	vcmds_t *cc = FindVoteCmdByName("map");
	if (!cc) {
		gi.Com_PrintFmt("{}: missing map vote command.\n", __FUNCTION__);
		return;
	}
	if (!p || !p->entries || p->cur < 0 || p->cur >= p->num) {
		gi.Com_PrintFmt("{}: invalid map selection index.\n", __FUNCTION__);
		return;
	}

	const menu_t &selected = p->entries[p->cur];
	if (!selected.text[0]) {
		gi.Com_PrintFmt("{}: no map selected.\n", __FUNCTION__);
		return;
	}

	level.vote = cc;
	level.vote_arg = selected.text;

	VoteCommandStore(ent);
	P_Menu_Close(ent);
}

inline std::vector<std::string> str_split(const std::string_view &str, char by) {
	std::vector<std::string> out;
	size_t start = 0;

	while (true) {
		start = str.find_first_not_of(by, start);
		if (start == std::string_view::npos)
			break;

		size_t end = str.find(by, start);
		if (end == std::string_view::npos) {
			out.emplace_back(str.substr(start));
			break;
		}

		out.emplace_back(str.substr(start, end - start));
		start = end + 1;
	}

	return out;
}

static void G_Menu_CallVote_Map_Update(gentity_t *ent) {

	menu_t *entries = ent->client->menu->entries;
	int i = 2;
	size_t num = 0;

	auto values = str_split(g_map_list->string, ' ');

	if (!values.size())
		return;

	for (i = 2; i < 15; i++) {
		entries[i].SelectFunc = nullptr;
		entries[i].text[0] = '\0';
	}

	for (num = 0, i = 2; num < values.size() && num < 15; num++, i++) {
		Q_strlcpy(entries[i].text, values[num].c_str(), sizeof(entries[i].text));
		entries[i].SelectFunc = G_Menu_CallVote_Map_Selection;
	}
}

void G_Menu_CallVote_Map(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	P_Menu_Open(ent, pmcallvotemenu_map, -1, sizeof(pmcallvotemenu_map) / sizeof(menu_t), nullptr, false, G_Menu_CallVote_Map_Update);
}

void G_Menu_CallVote_NextMap(gentity_t *ent, menu_hnd_t *p) {
	level.vote = FindVoteCmdByName("nextmap");
	level.vote_arg.clear();
	VoteCommandStore(ent);
	P_Menu_Close(ent);
}

void G_Menu_CallVote_Restart(gentity_t *ent, menu_hnd_t *p) {
	level.vote = FindVoteCmdByName("restart");
	level.vote_arg.clear();
	VoteCommandStore(ent);
	P_Menu_Close(ent);
}

void G_Menu_CallVote_GameType(gentity_t *ent, menu_hnd_t *p) {

}

void G_Menu_CallVote_TimeLimit_Update(gentity_t *ent) {

	level.vote_arg.clear();
}

void G_Menu_CallVote_TimeLimit(gentity_t *ent, menu_hnd_t *p) {
	//level.vote = FindVoteCmdByName("timelimit");
	//level.vote_arg.clear();
	//VoteCommandStore(ent);
	P_Menu_Close(ent);
	P_Menu_Open(ent, pmcallvotemenu_timelimit, -1, sizeof(pmcallvotemenu_timelimit) / sizeof(menu_t), nullptr, false, G_Menu_CallVote_TimeLimit_Update);
}

void G_Menu_CallVote_ScoreLimit(gentity_t *ent, menu_hnd_t *p) {

}

void G_Menu_CallVote_ShuffleTeams(gentity_t *ent, menu_hnd_t *p) {
	level.vote = FindVoteCmdByName("shuffle");
	level.vote_arg.clear();
	VoteCommandStore(ent);
	P_Menu_Close(ent);
}

void G_Menu_CallVote_BalanceTeams(gentity_t *ent, menu_hnd_t *p) {
	level.vote = FindVoteCmdByName("balance");
	level.vote_arg.clear();
	VoteCommandStore(ent);
	P_Menu_Close(ent);

}

void G_Menu_CallVote_Unlagged(gentity_t *ent, menu_hnd_t *p) {

}

void G_Menu_CallVote_Cointoss(gentity_t *ent, menu_hnd_t *p) {
	level.vote = FindVoteCmdByName("cointoss");
	level.vote_arg.clear();
	VoteCommandStore(ent);
	P_Menu_Close(ent);
}

void G_Menu_CallVote_Random(gentity_t *ent, menu_hnd_t *p) {

}

static void G_Menu_CallVote_Update(gentity_t *ent) {
	menu_t *entries = ent->client->menu->entries;
	int i = 0;

	Q_strlcpy(entries[i].text, "Call a Vote", sizeof(entries[i].text));
	i++;
	i++;
	/*
	entries[cvmenu_map].SelectFunc = G_Menu_CallVote_Map;
	i++;
	entries[cvmenu_nextmap].SelectFunc = G_Menu_CallVote_NextMap;
	i++;
	entries[cvmenu_restart].SelectFunc = G_Menu_CallVote_Restart;
	i++;
	entries[cvmenu_gametype].SelectFunc = G_Menu_CallVote_GameType;
	i++;
	entries[cvmenu_timelimit].SelectFunc = G_Menu_CallVote_TimeLimit;
	i++;
	entries[cvmenu_scorelimit].SelectFunc = G_Menu_CallVote_ScoreLimit;
	i++;
	entries[cvmenu_shuffle].SelectFunc = G_Menu_CallVote_ShuffleTeams;
	i++;
	entries[cvmenu_balance].SelectFunc = G_Menu_CallVote_BalanceTeams;
	i++;
	entries[cvmenu_unlagged].SelectFunc = G_Menu_CallVote_Unlagged;
	i++;
	entries[cvmenu_cointoss].SelectFunc = G_Menu_CallVote_Cointoss;
	i++;
	entries[cvmenu_random].SelectFunc = G_Menu_CallVote_Random;
	*/
}

static void G_Menu_CallVote(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	P_Menu_Open(ent, pmcallvotemenu, -1, sizeof(pmcallvotemenu) / sizeof(menu_t), nullptr, false, G_Menu_CallVote_Update);
}
/*-----------------------------------------------------------------------*/

static void G_Menu_Vote_Yes(gentity_t *ent, menu_hnd_t *p) {
	level.vote_yes++;
	ent->client->pers.voted = 1;

	gi.LocClient_Print(ent, PRINT_HIGH, "Vote cast.\n");
	P_Menu_Close(ent);
}

static void G_Menu_Vote_No(gentity_t *ent, menu_hnd_t *p) {
	level.vote_no++;
	ent->client->pers.voted = -1;

	gi.LocClient_Print(ent, PRINT_HIGH, "Vote cast.\n");
	P_Menu_Close(ent);
}

const menu_t votemenu[] = {
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "Voting Menu", MENU_ALIGN_CENTER, nullptr },	//x called a vote
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "none", MENU_ALIGN_CENTER, nullptr },				//vote type, eg: map q2dm1
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "[ YES ]", MENU_ALIGN_CENTER, G_Menu_Vote_Yes },	// GET READY TO VOTE...	/ vote yes
	{ "[ NO ]", MENU_ALIGN_CENTER, G_Menu_Vote_No },	// COUNTDOWN... / vote no
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "30", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr }
};

static void G_Menu_Vote_Update(gentity_t *ent) {
	if (!Vote_Menu_Active(ent)) {
		P_Menu_Close(ent);
		return;
	}

	int timeout = 30 - (level.time - level.vote_time).seconds<int>();

	if (timeout <= 0) {
		P_Menu_Close(ent);
		return;
	}

	menu_t *entries = ent->client->menu->entries;
	int i = 2;
	Q_strlcpy(entries[i].text, G_Fmt("{} called a vote:", level.vote_client->resp.netname).data(), sizeof(entries[i].text));
	
	i = 4;
	Q_strlcpy(entries[i].text, G_Fmt("{} {}", level.vote->name, level.vote_arg).data(), sizeof(entries[i].text));

	if (level.vote_time + 3_sec > level.time) {
		i = 7;
		Q_strlcpy(entries[i].text, "GET READY TO VOTE!", sizeof(entries[i].text));
		entries[i].SelectFunc = nullptr;

		i = 8;
		int time = 3 - (level.time - level.vote_time).seconds<int>();
		Q_strlcpy(entries[i].text, G_Fmt("{}...", time).data(), sizeof(entries[i].text));
		entries[i].SelectFunc = nullptr;
		return;
	}

	i = 7;
	Q_strlcpy(entries[i].text, "[ YES ]", sizeof(entries[i].text));
	entries[i].SelectFunc = G_Menu_Vote_Yes;
	i = 8;
	Q_strlcpy(entries[i].text, "[ NO ]", sizeof(entries[i].text));
	entries[i].SelectFunc = G_Menu_Vote_No;

	i = 16;
	Q_strlcpy(entries[i].text, G_Fmt("{}", timeout).data(), sizeof(entries[i].text));
}

void G_Menu_Vote_Open(gentity_t *ent) {
	P_Menu_Open(ent, votemenu, -1, sizeof(votemenu) / sizeof(menu_t), nullptr, false, G_Menu_Vote_Update);
}


/*-----------------------------------------------------------------------*/

static void G_Menu_Join_Team_Free(gentity_t *ent, menu_hnd_t *p) {
	SetTeam(ent, TEAM_FREE, false, false, false);
}

static void G_Menu_Join_Team_Red(gentity_t *ent, menu_hnd_t *p) {
	SetTeam(ent, !g_teamplay_allow_team_pick->integer ? PickTeam(-1) : TEAM_RED, false, false, false);
}

static void G_Menu_Join_Team_Blue(gentity_t *ent, menu_hnd_t *p) {
	if (!g_teamplay_allow_team_pick->integer)
		return;

	SetTeam(ent, TEAM_BLUE, false, false, false);
}

static void G_Menu_Join_Team_Spec(gentity_t *ent, menu_hnd_t *p) {
	SetTeam(ent, TEAM_SPECTATOR, false, false, false);
}

void G_Menu_ReturnToMain(gentity_t *ent, menu_hnd_t *p);
void G_Menu_ChaseCam(gentity_t *ent, menu_hnd_t *p);
void G_Menu_HostInfo(gentity_t *ent, menu_hnd_t *p);
void G_Menu_ServerInfo(gentity_t *ent, menu_hnd_t *p);

static const int jmenu_hostname = 0;
static const int jmenu_gametype = 1;
static const int jmenu_level = 2;
static const int jmenu_match = 3;

static const int jmenu_teams_join_red = 5;
static const int jmenu_teams_join_blue = 6;
static const int jmenu_teams_spec = 7;
static const int jmenu_teams_chase = 8;
static const int jmenu_teams_hostinfo = 10;
static const int jmenu_teams_svinfo = 11;
static const int jmenu_teams_player = 12;
static const int jmenu_teams_callvote = 13;
static const int jmenu_teams_admin = 14;

static const int jmenu_free_join = 5;
static const int jmenu_free_spec = 7;
static const int jmenu_free_chase = 8;
static const int jmenu_free_hostinfo = 10;
static const int jmenu_free_svinfo = 11;
static const int jmenu_free_player = 12;
static const int jmenu_free_callvote = 13;
static const int jmenu_free_admin = 14;

static const int jmenu_gamemod = 16;
static const int jmenu_notice = 17;

const menu_t teams_join_menu[] = {
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_join_red_team", MENU_ALIGN_LEFT, G_Menu_Join_Team_Red },
	{ "$g_pc_join_blue_team", MENU_ALIGN_LEFT, G_Menu_Join_Team_Blue },
	{ "Spectate", MENU_ALIGN_LEFT, G_Menu_Join_Team_Spec },
	{ "$g_pc_chase_camera", MENU_ALIGN_LEFT, G_Menu_ChaseCam },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "Host Info", MENU_ALIGN_LEFT, G_Menu_HostInfo },
	{ "Match Info", MENU_ALIGN_LEFT, G_Menu_ServerInfo },
	//{ "Game Rules", MENU_ALIGN_LEFT, G_Menu_GameRules },
	{ "Player Stats", MENU_ALIGN_LEFT, G_Menu_PMStats },
	//{ "Call a Vote", MENU_ALIGN_LEFT, G_Menu_CallVote },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "Admin", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr }
};

const menu_t free_join_menu[] = {
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "Join Game", MENU_ALIGN_LEFT, G_Menu_Join_Team_Free },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "Spectate", MENU_ALIGN_LEFT, G_Menu_Join_Team_Spec },
	{ "$g_pc_chase_camera", MENU_ALIGN_LEFT, G_Menu_ChaseCam },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "Host Info", MENU_ALIGN_LEFT, G_Menu_HostInfo },
	{ "Match Info", MENU_ALIGN_LEFT, G_Menu_ServerInfo },
	//{ "Game Rules", MENU_ALIGN_LEFT, G_Menu_GameRules },
	{ "Player Stats", MENU_ALIGN_LEFT, G_Menu_PMStats },
	//{ "Call a Vote", MENU_ALIGN_LEFT,  G_Menu_CallVote },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "Admin", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr }
};

const menu_t nochasemenu[] = {
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_no_chase", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

const menu_t hostinfomenu[] = {
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

const menu_t svinfomenu[] = {
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "", MENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_return", MENU_ALIGN_LEFT, G_Menu_ReturnToMain }
};

static void G_Menu_NoChaseCamUpdate(gentity_t *ent) {
	menu_t *entries = ent->client->menu->entries;

	G_Menu_SetGamemodName(&entries[jmenu_gamemod]);
	G_Menu_SetGametypeName(&entries[jmenu_gametype]);
	G_Menu_SetLevelName(&entries[jmenu_level]);
}

void G_Menu_ChaseCam(gentity_t *ent, menu_hnd_t *p) {
	SetTeam(ent, TEAM_SPECTATOR, false, false, false);

	if (ent->client->follow_target) {
		FreeFollower(ent);
		P_Menu_Close(ent);
		return;
	}

	GetFollowTarget(ent);

	P_Menu_Close(ent);
	P_Menu_Open(ent, nochasemenu, -1, sizeof(nochasemenu) / sizeof(menu_t), nullptr, false, G_Menu_NoChaseCamUpdate);
}

void G_Menu_ReturnToMain(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	G_Menu_Join_Open(ent);
	gi.local_sound(ent, CHAN_AUTO, gi.soundindex("misc/menu3.wav"), 1, ATTN_NONE, 0);
}

static void G_Menu_HostInfo_Update(gentity_t *ent) {
	menu_t *entries = ent->client->menu->entries;
	int		i = 0;
	bool	limits = false;

	if (hostname->string[0]) {
		Q_strlcpy(entries[i].text, "Server Name:", sizeof(entries[i].text));
		i++;
		Q_strlcpy(entries[i].text, hostname->string, sizeof(entries[i].text));
		i++;
		i++;
	}

	if (g_entities[1].client) {
		char value[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(g_entities[1].client->pers.userinfo, "name", value, sizeof(value));

		if (value[0]) {
			Q_strlcpy(entries[i].text, "Host:", sizeof(entries[i].text));
			i++;
			Q_strlcpy(entries[i].text, value, sizeof(entries[i].text));
			i++;
			i++;
		}
	}

	if (game.motd.size()) {
		Q_strlcpy(entries[i].text, "Message of the Day:", sizeof(entries[i].text));
		i++;
		// 26 char line width
		// 9 lines
		// = 234

		Q_strlcpy(entries[i].text, G_Fmt("{}", game.motd.c_str()).data(), sizeof(entries[i].text));
	}
}

void G_Menu_HostInfo(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	P_Menu_Open(ent, hostinfomenu, -1, sizeof(hostinfomenu) / sizeof(menu_t), nullptr, false, G_Menu_HostInfo_Update);
}

static void G_Menu_ServerInfo_Update(gentity_t *ent) {
	menu_t *entries = ent->client->menu->entries;
	int		i = 0;
	bool	limits = false;
	bool	infiniteammo = InfiniteAmmoOn(nullptr);
	bool	items = ItemSpawnsEnabled();
	int		scorelimit = GT_ScoreLimit();
	
	Q_strlcpy(entries[i].text, "Match Info", sizeof(entries[i].text));
	i++;

	Q_strlcpy(entries[i].text, BREAKER, sizeof(entries[i].text));
	i++;

	Q_strlcpy(entries[i].text, level.gametype_name, sizeof(entries[i].text));
	i++;
	
	if (level.level_name[0]) {
		Q_strlcpy(entries[i].text, G_Fmt("map: {}", level.level_name).data(), sizeof(entries[i].text));
		i++;
	}
	if (level.mapname[0]) {
		Q_strlcpy(entries[i].text, G_Fmt("mapname: {}", level.mapname).data(), sizeof(entries[i].text));
		i++;
	}
	if (level.author[0]) {
		Q_strlcpy(entries[i].text, G_Fmt("author: {}", level.author).data(), sizeof(entries[i].text));
		i++;
	}
	if (level.author2[0] && level.author[0]) {
		Q_strlcpy(entries[i].text, G_Fmt("      {}", level.author2).data(), sizeof(entries[i].text));
		i++;
	}

	Q_strlcpy(entries[i].text, G_Fmt("ruleset: {}", rs_long_name[(int)game.ruleset]).data(), sizeof(entries[i].text));
	i++;

	if (scorelimit) {
		Q_strlcpy(entries[i].text, G_Fmt("{} limit: {}", GT_ScoreLimitString(), scorelimit).data(), sizeof(entries[i].text));
		i++;
		limits = true;
	}

	if (timelimit->value > 0) {
		Q_strlcpy(entries[i].text, G_Fmt("time limit: {}", G_TimeString(timelimit->value * 60000, false)).data(), sizeof(entries[i].text));
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
	if (Teams() && g_friendly_fire->integer) {
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

	if (g_teleporter_freeze->integer) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "teleporter freeze", sizeof(entries[i].text));
		i++;
	}

	if (Teams() && g_teamplay_force_balance->integer && notGT(GT_RR)) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, "forced team balancing", sizeof(entries[i].text));
		i++;
	}

	if (g_dm_random_items->integer && items) {
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

	if (items) {
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

	if (g_mover_speed_scale->value != 1.0f) {
		if (i >= 16) return;
		Q_strlcpy(entries[i].text, G_Fmt("mover speed scale: {}", g_mover_speed_scale->value).data(), sizeof(entries[i].text));
		i++;
	}

}

void G_Menu_ServerInfo(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	P_Menu_Open(ent, svinfomenu, -1, sizeof(svinfomenu) / sizeof(menu_t), nullptr, false, G_Menu_ServerInfo_Update);
}

static void G_Menu_GameRules_Update(gentity_t *ent) {
	menu_t *entries = ent->client->menu->entries;
	int		i = 0;
	bool	limits = false;

	Q_strlcpy(entries[i].text, "Game Rules", sizeof(entries[i].text)); i++;
	Q_strlcpy(entries[i].text, BREAKER, sizeof(entries[i].text)); i++;
	Q_strlcpy(entries[i].text, G_Fmt("{}", level.gametype_name).data(), sizeof(entries[i].text)); i++;
}

static void G_Menu_GameRules(gentity_t *ent, menu_hnd_t *p) {
	P_Menu_Close(ent);
	P_Menu_Open(ent, svinfomenu, -1, sizeof(svinfomenu) / sizeof(menu_t), nullptr, false, G_Menu_GameRules_Update);
}

static void G_Menu_Join_Update(gentity_t *ent) {
	menu_t *entries = ent->client->menu->entries;
	int		pmax = maxplayers->integer;
	uint8_t	num_red = 0, num_blue = 0, num_free = 0, num_queue = 0;

	for (auto ec : active_clients()) {
		if (GT(GT_DUEL) && ec->client->sess.team == TEAM_SPECTATOR && ec->client->sess.duel_queued) {
			num_queue++;
		} else {
			switch (ec->client->sess.team) {
			case TEAM_FREE:
				num_free++;
				break;
			case TEAM_RED:
				num_red++;
				break;
			case TEAM_BLUE:
				num_blue++;
				break;
			}
		}
	}

	if (pmax < 1) pmax = 1;

	G_Menu_SetGamemodName(entries + jmenu_gamemod);
	G_Menu_SetGametypeName(entries + jmenu_gametype);

	if (Teams()) {
		if (!g_teamplay_allow_team_pick->integer && !level.locked[TEAM_RED] && !level.locked[TEAM_BLUE]) {
			Q_strlcpy(entries[jmenu_teams_join_red].text, G_Fmt("Join a Team ({}/{})", num_red + num_blue, pmax).data(), sizeof(entries[jmenu_teams_join_red].text));
			Q_strlcpy(entries[jmenu_teams_join_blue].text, "", sizeof(entries[jmenu_teams_join_blue].text));

			entries[jmenu_teams_join_red].SelectFunc = G_Menu_Join_Team_Red;
			entries[jmenu_teams_join_blue].SelectFunc = nullptr;
		} else {
			if (level.locked[TEAM_RED] || level.match_state >= matchst_t::MATCH_COUNTDOWN && g_match_lock->integer) {
				Q_strlcpy(entries[jmenu_teams_join_red].text, G_Fmt("{} is LOCKED during play", Teams_TeamName(TEAM_RED)).data(), sizeof(entries[jmenu_teams_join_red].text));
				entries[jmenu_teams_join_red].SelectFunc = nullptr;
			} else {
				Q_strlcpy(entries[jmenu_teams_join_red].text, G_Fmt("Join {} ({}/{})", Teams_TeamName(TEAM_RED), num_red, floor(pmax / 2)).data(), sizeof(entries[jmenu_teams_join_red].text));
				entries[jmenu_teams_join_red].SelectFunc = G_Menu_Join_Team_Red;
			}
			if (level.locked[TEAM_BLUE] || level.match_state >= matchst_t::MATCH_COUNTDOWN && g_match_lock->integer) {
				Q_strlcpy(entries[jmenu_teams_join_blue].text, G_Fmt("{} is LOCKED during play", Teams_TeamName(TEAM_BLUE)).data(), sizeof(entries[jmenu_teams_join_blue].text));
				entries[jmenu_teams_join_blue].SelectFunc = nullptr;
			} else {
				Q_strlcpy(entries[jmenu_teams_join_blue].text, G_Fmt("Join {} ({}/{})", Teams_TeamName(TEAM_BLUE), num_blue, floor(pmax / 2)).data(), sizeof(entries[jmenu_teams_join_blue].text));
				entries[jmenu_teams_join_blue].SelectFunc = G_Menu_Join_Team_Blue;
			}

		}
	} else {
		if (level.locked[TEAM_FREE] || level.match_state >= matchst_t::MATCH_COUNTDOWN && g_match_lock->integer) {
			Q_strlcpy(entries[jmenu_free_join].text, "Match LOCKED during play", sizeof(entries[jmenu_free_join].text));
			entries[jmenu_free_join].SelectFunc = nullptr;
		} else if (GT(GT_DUEL) && level.num_playing_clients == 2) {
			Q_strlcpy(entries[jmenu_free_join].text, G_Fmt("Join Queue to Play ({}/{})", num_queue, pmax - 2).data(), sizeof(entries[jmenu_free_join].text));
			entries[jmenu_free_join].SelectFunc = G_Menu_Join_Team_Free;
		} else {
			Q_strlcpy(entries[jmenu_free_join].text, G_Fmt("Join Match ({}/{})", num_free, GT(GT_DUEL) ? 2 : pmax).data(), sizeof(entries[jmenu_free_join].text));
			entries[jmenu_free_join].SelectFunc = G_Menu_Join_Team_Free;
		}
	}

	if (!g_matchstats->integer) {
		;
		int index = Teams() ? jmenu_teams_player : jmenu_free_player;
		Q_strlcpy(entries[index].text, "", sizeof(entries[index].text));
		entries[index].SelectFunc = nullptr;
	} else {
		int index = Teams() ? jmenu_teams_player : jmenu_free_player;
		Q_strlcpy(entries[index].text, "Player Stats", sizeof(entries[index].text));
		entries[index].SelectFunc = G_Menu_PMStats;
	}

	if (g_dm_force_join->string && *g_dm_force_join->string) {
		if (Teams()) {
			if (Q_strcasecmp(g_dm_force_join->string, "red") == 0) {
				entries[jmenu_teams_join_blue].text[0] = '\0';
				entries[jmenu_teams_join_blue].SelectFunc = nullptr;
			} else if (Q_strcasecmp(g_dm_force_join->string, "blue") == 0) {
				entries[jmenu_teams_join_red].text[0] = '\0';
				entries[jmenu_teams_join_red].SelectFunc = nullptr;
			}
		}
	}

	int index = Teams() ? jmenu_teams_chase : jmenu_free_chase;
	if (ent->client->follow_target)
		Q_strlcpy(entries[index].text, "$g_pc_leave_chase_camera", sizeof(entries[index].text));
	else
		Q_strlcpy(entries[index].text, "$g_pc_chase_camera", sizeof(entries[index].text));

	G_Menu_SetHostName(entries + jmenu_hostname);
	G_Menu_SetGametypeName(entries + jmenu_gametype);
	G_Menu_SetLevelName(entries + jmenu_level);

	G_Menu_SetGamemodName(entries + jmenu_gamemod);

	switch (level.match_state) {
	case matchst_t::MATCH_NONE:
		entries[jmenu_match].text[0] = '\0';
		break;

	case matchst_t::MATCH_WARMUP_DELAYED:
	case matchst_t::MATCH_WARMUP_DEFAULT:
	case matchst_t::MATCH_WARMUP_READYUP:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH WARMUP", sizeof(entries[jmenu_match].text));
		break;

	case matchst_t::MATCH_COUNTDOWN:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH IS STARTING", sizeof(entries[jmenu_match].text));
		break;

	case matchst_t::MATCH_IN_PROGRESS:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH IN PROGRESS", sizeof(entries[jmenu_match].text));
		break;

	default:
		Q_strlcpy(entries[jmenu_match].text, BREAKER, sizeof(entries[jmenu_match].text));
		break;
	}

	int admin_index = Teams() ? jmenu_teams_admin : jmenu_free_admin;
	if (ent->client->sess.admin) {
		Q_strlcpy(entries[admin_index].text, "Admin", sizeof(entries[admin_index].text));
		entries[admin_index].SelectFunc = G_Menu_Admin;
	} else {
		Q_strlcpy(entries[admin_index].text, "", sizeof(entries[admin_index].text));
		entries[admin_index].SelectFunc = nullptr;
	}

	Q_strlcpy(entries[jmenu_notice].text, "github.com/themuffinator", sizeof(entries[jmenu_notice].text));
}

void G_Menu_Join_Open(gentity_t *ent) {
	if (Vote_Menu_Active(ent))
		return;

	if (Teams()) {
		team_t team = TEAM_SPECTATOR;
		uint8_t num_red = 0, num_blue = 0;

		for (auto ec : active_clients()) {
			switch (ec->client->sess.team) {
			case TEAM_RED:
				num_red++;
				break;
			case TEAM_BLUE:
				num_blue++;
				break;
			}
		}

		if (num_red > num_blue)
			team = TEAM_RED;
		else if (num_blue > num_red)
			team = TEAM_BLUE;
		else
			team = brandom() ? TEAM_RED : TEAM_BLUE;

		P_Menu_Open(ent, teams_join_menu, team, sizeof(teams_join_menu) / sizeof(menu_t), nullptr, false, G_Menu_Join_Update);
	} else {
		P_Menu_Open(ent, free_join_menu, TEAM_FREE, sizeof(free_join_menu) / sizeof(menu_t), nullptr, false, G_Menu_Join_Update);
	}
}
