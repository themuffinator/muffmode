// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include <cstddef>

struct gentity_t;

enum {
	MENU_ALIGN_LEFT,
	MENU_ALIGN_CENTER,
	MENU_ALIGN_RIGHT
};

struct menu_t;

using UpdateFunc_t = void (*)(gentity_t *ent);

struct menu_hnd_t {
	menu_t	*entries;
	int		cur;
	int		num;
	void	*arg;
	bool	owns_arg;
	UpdateFunc_t UpdateFunc;
};

using SelectFunc_t = void (*)(gentity_t *ent, menu_hnd_t *hnd);

struct menu_t {
	char		 text[256];	// 26];	// [64];
	int			 align;
	SelectFunc_t SelectFunc;
	char         text_arg1[64];
};

void		P_Menu_Dirty();
menu_hnd_t	*P_Menu_Open(gentity_t *ent, const menu_t *entries, int cur, int num, void *arg, bool owns_arg, UpdateFunc_t UpdateFunc);
void		P_Menu_Close(gentity_t *ent);
void		P_Menu_UpdateEntry(menu_t *entry, const char *text, int align, SelectFunc_t SelectFunc);
size_t		P_Menu_BuildStatusBar(const menu_hnd_t *hnd, char *layout, size_t layout_size);
void		P_Menu_Do_Update(gentity_t *ent);
void		P_Menu_Update(gentity_t *ent);
void		P_Menu_Next(gentity_t *ent);
void		P_Menu_Prev(gentity_t *ent);
void		P_Menu_Select(gentity_t *ent);
void            P_Menu_OpenBanned(gentity_t *ent);
bool            P_Menu_IsBannedMenu(const menu_hnd_t *hnd);
