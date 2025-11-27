// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include <cassert>

/*
============
P_Menu_Dirty
============
*/
void P_Menu_Dirty() {
	for (auto player : active_clients())
		if (player->client->menu) {
			player->client->menudirty = true;
			player->client->menutime = level.time;
		}
}

/*
=============
P_Menu_Open

Open a menu for a client and duplicate entry data for safe modification. The
owns_arg flag controls whether the menu should free arg on close.
=============
*/
menu_hnd_t *P_Menu_Open(gentity_t *ent, const menu_t *entries, int cur, int num, void *arg, bool owns_arg, UpdateFunc_t UpdateFunc) {
	menu_hnd_t		*hnd;
	const menu_t	*p;
	size_t			i;

	if (!ent->client)
		return nullptr;

	if (ent->client->menu) {
		gi.Com_Print("Warning: client already has a menu.\n");
		if (!Vote_Menu_Active(ent))
			P_Menu_Close(ent);
	}

	hnd = (menu_hnd_t *)gi.TagMalloc(sizeof(*hnd), TAG_LEVEL);
	hnd->UpdateFunc = UpdateFunc;

	hnd->arg = arg;
	hnd->owns_arg = owns_arg;
	hnd->entries = (menu_t *)gi.TagMalloc(sizeof(menu_t) * num, TAG_LEVEL);
	memcpy(hnd->entries, entries, sizeof(menu_t) * num);
	// duplicate the strings since they may be from static memory
	for (i = 0; i < num; i++) {
		assert(Q_strlcpy(hnd->entries[i].text.data(), entries[i].text.data(), hnd->entries[i].text.size()) < hnd->entries[i].text.size());
		assert(Q_strlcpy(hnd->entries[i].text_arg1.data(), entries[i].text_arg1.data(), hnd->entries[i].text_arg1.size()) < hnd->entries[i].text_arg1.size());
	}

	hnd->num = num;

	if (cur < 0 || !entries[cur].SelectFunc) {
		for (i = 0, p = entries; i < num; i++, p++)
			if (p->SelectFunc)
				break;
	} else
		i = cur;

	if (i >= num)
		hnd->cur = -1;
	else
		hnd->cur = i;

	ent->client->showscores = true;
	ent->client->inmenu = true;
	ent->client->menu = hnd;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;

	if (UpdateFunc)
		UpdateFunc(ent);

	P_Menu_Do_Update(ent);
	gi.unicast(ent, true);

	return hnd;
}

/*
=============
P_Menu_Close

Closes the active menu for the entity and frees associated resources.
=============
*/
void P_Menu_Close(gentity_t *ent) {
	menu_hnd_t *hnd;

	if (!ent->client)
		return;

	if (!ent->client->menu)
		return;

	hnd = ent->client->menu;
	gi.TagFree(hnd->entries);
	if (hnd->owns_arg && hnd->arg)
		gi.TagFree(hnd->arg);
	gi.TagFree(hnd);
	ent->client->menu = nullptr;
	ent->client->showscores = false;

	gentity_t *e = ent->client->follow_target ? ent->client->follow_target : ent;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) ? 0 : 1;
}


/*
=============
P_Menu_UpdateEntry

Replaces the text and callbacks for a menu entry created by P_Menu_Open.
=============
*/
void P_Menu_UpdateEntry(menu_t *entry, const char *text, int align, SelectFunc_t SelectFunc) {
	Q_strlcpy(entry->text.data(), text, entry->text.size());
	entry->align = align;
	entry->SelectFunc = SelectFunc;
}

/*
=============
P_Menu_Do_Update

Regenerates the menu status bar layout for the client using bounded string
operations.
=============
*/
void P_Menu_Do_Update(gentity_t *ent) {
	menu_hnd_t		*hnd;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->UpdateFunc)
		hnd->UpdateFunc(ent);

	char layout[MAX_STRING_CHARS] = {};

	P_Menu_BuildStatusBar(hnd, layout, sizeof(layout));

	gi.WriteByte(svc_layout);
	gi.WriteString(layout);
}


/*
=============
P_Menu_Update

Performs periodic menu updates when the client is viewing a menu.
=============
*/
void P_Menu_Update(gentity_t *ent) {
	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	if (level.time - ent->client->menutime >= 1_sec) {
		// been a second or more since last update, update now
		P_Menu_Do_Update(ent);
		gi.unicast(ent, true);
		ent->client->menutime = level.time + 1_sec;
		ent->client->menudirty = false;
	}

	ent->client->menutime = level.time;
	ent->client->menudirty = true;
	gi.local_sound(ent, CHAN_AUTO, gi.soundindex("misc/menu2.wav"), 1, ATTN_NONE, 0);
}

/*
=============
P_Menu_Next

Advances the menu cursor to the next selectable entry.
=============
*/
void P_Menu_Next(gentity_t *ent) {
	menu_hnd_t	*hnd;
	int			i;
	menu_t		*p;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do {
		i++;
		p++;
		if (i == hnd->num) {
			i = 0;
			p = hnd->entries;
		}
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;

	P_Menu_Update(ent);
}

/*
=============
P_Menu_Prev

Moves the menu cursor to the previous selectable entry.
=============
*/
void P_Menu_Prev(gentity_t *ent) {
	menu_hnd_t	*hnd;
	int			i;
	menu_t		*p;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do {
		if (i == 0) {
			i = hnd->num - 1;
			p = hnd->entries + i;
		} else {
			i--;
			p--;
		}
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;

	P_Menu_Update(ent);
}

/*
=============
P_Menu_Select

Triggers the select callback for the current menu entry.
=============
*/
void P_Menu_Select(gentity_t *ent) {
	menu_hnd_t	*hnd;
	menu_t		*p;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	// no selecting during intermission
	if (level.intermission_queued || level.intermission_time)
		return;

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	p = hnd->entries + hnd->cur;

	if (p->SelectFunc)
		p->SelectFunc(ent, hnd);
	//gi.local_sound(ent, CHAN_AUTO, gi.soundindex("misc/menu1.wav"), 1, ATTN_NONE, 0);
}

namespace {

constexpr const char *BANNED_MENU_LINES[] = {
	"You are banned from this mod",
	"due to extremely poor behaviour",
	"towards the community."
};

menu_t banned_menu_entries[] = {
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
	{ "", MENU_ALIGN_CENTER, nullptr },
};

/*
=============
P_Menu_Banned_Update

No-op update hook for the banned menu.
=============
*/
void P_Menu_Banned_Update(gentity_t *ent) {
	(void)ent;
}

/*
=============
P_Menu_Banned_InitEntries

Initializes the static banned menu lines.
=============
*/
void P_Menu_Banned_InitEntries() {
	for (size_t i = 0; i < sizeof(banned_menu_entries) / sizeof(banned_menu_entries[0]); ++i)
		Q_strlcpy(banned_menu_entries[i].text.data(), BANNED_MENU_LINES[i], banned_menu_entries[i].text.size());
}

} // namespace

/*
=============
P_Menu_OpenBanned

Opens the banned notification menu for a client.
=============
*/
void P_Menu_OpenBanned(gentity_t *ent) {
	if (!ent->client)
		return;

	P_Menu_Banned_InitEntries();
	P_Menu_Open(ent, banned_menu_entries, -1, sizeof(banned_menu_entries) / sizeof(menu_t), nullptr, false, P_Menu_Banned_Update);
}

/*
=============
P_Menu_IsBannedMenu

Returns true when the supplied menu handle is the banned menu.
=============
*/
bool P_Menu_IsBannedMenu(const menu_hnd_t *hnd) {
	if (!hnd)
		return false;

	return hnd->UpdateFunc == P_Menu_Banned_Update;
}
