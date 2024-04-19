// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

enum {
	MENU_ALIGN_LEFT,
	MENU_ALIGN_CENTER,
	MENU_ALIGN_RIGHT
};

struct menu_t;

using UpdateFunc_t = void (*)(edict_t *ent);

struct menu_hnd_t {
	menu_t	*entries;
	int		cur;
	int		num;
	void	*arg;
	UpdateFunc_t UpdateFunc;
};

using SelectFunc_t = void (*)(edict_t *ent, menu_hnd_t *hnd);

struct menu_t {
	char		 text[26];	// [64];
	int			 align;
	SelectFunc_t SelectFunc;
	char         text_arg1[64];
};

void		P_Menu_Dirty();
menu_hnd_t	*P_Menu_Open(edict_t *ent, const menu_t *entries, int cur, int num, void *arg, UpdateFunc_t UpdateFunc);
void		P_Menu_Close(edict_t *ent);
void		P_Menu_UpdateEntry(menu_t *entry, const char *text, int align, SelectFunc_t SelectFunc);
void		P_Menu_Do_Update(edict_t *ent);
void		P_Menu_Update(edict_t *ent);
void		P_Menu_Next(edict_t *ent);
void		P_Menu_Prev(edict_t *ent);
void		P_Menu_Select(edict_t *ent);
