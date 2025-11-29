// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include <cstddef>
#include <cstring>
#include <string>

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
	int			cur;
	int			num;
	void	*arg;
	bool	owns_arg;
	UpdateFunc_t UpdateFunc;
};

using SelectFunc_t = void (*)(gentity_t *ent, menu_hnd_t *hnd);

constexpr size_t MENU_TEXT_MAX = 256;
constexpr size_t MENU_TEXT_ARG_MAX = 64;

/*
=============
P_Menu_InitText

Creates a bounded std::string from the provided source string.
=============
*/
template <size_t N>
inline std::string P_Menu_InitText(const char *src) {
	if (!src)
		return {};

	size_t length = 0;
	while (length < N - 1 && src[length] != '\0')
		length++;
	return std::string(src, length);
}

struct menu_t {
	std::string		text;
	int			align;
	SelectFunc_t	SelectFunc;
	std::string		text_arg1;
};

/*
=============
P_Menu_CreateEntry

Initializes a menu_t instance with bounded text and optional callback data.
=============
*/
inline menu_t P_Menu_CreateEntry(const char *text, int align, SelectFunc_t SelectFunc = nullptr, const char *text_arg1 = "") {
	return { P_Menu_InitText<MENU_TEXT_MAX>(text), align, SelectFunc, P_Menu_InitText<MENU_TEXT_ARG_MAX>(text_arg1) };
}

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
void		P_Menu_OpenBanned(gentity_t *ent);
bool		P_Menu_IsBannedMenu(const menu_hnd_t *hnd);
