// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "p_menu.h"
#include "q_std.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef UNIT_TESTS
/*
=============
Q_strlcpy

Test-safe implementation of Q_strlcpy used when running unit tests without the
full game import table.
=============
*/
size_t Q_strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	if (n != 0 && --n != 0)
		do {
			if ((*d++ = *s++) == '\0')
				break;
		} while (--n != 0);

	if (n == 0) {
		if (siz != 0)
			*d = '\0';
		while (*s++)
			;
		}

	return static_cast<size_t>(s - src - 1);
		}
/*
=============
Q_strlcat

Test-safe implementation of Q_strlcat used when running unit tests without the
full game import table.
=============
*/
size_t Q_strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	while (n-- != 0 && *d != '\0')
		d++;
	dlen = static_cast<size_t>(d - dst);
	n = siz - dlen;

	if (n == 0)
		return dlen + std::strlen(s);

	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}		s++;
}

	*d = '\0';

	return dlen + static_cast<size_t>(s - src);
		}#endif

/*
=============
			P_Menu_Appendf

Appends formatted text to a layout buffer while ensuring the destination is
not overrun.
=============
*/
static bool P_Menu_Appendf(char *layout, size_t layout_size, const char *fmt, ...)
{
	if (layout_size == 0)
		return false;

	std::vector<char> chunk(layout_size, '\0');

	va_list args;
	va_start(args, fmt);
	vsnprintf(chunk.data(), chunk.size(), fmt, args);
	va_end(args);

	return Q_strlcat(layout, chunk.data(), layout_size) < layout_size;
		}
/*
=============
P_Menu_BuildStatusBar

Constructs the status bar layout string for the active menu using a bounded
buffer to avoid overflow.
=============
*/
size_t P_Menu_BuildStatusBar(const menu_hnd_t *hnd, char *layout, size_t layout_size)
{
	if (!hnd || !layout || layout_size == 0)
		return 0;

	layout[0] = '\0';

	if (!hnd->entries)
		return 0;

	P_Menu_Appendf(layout, layout_size, "xv %d yv %d picn %s ", 32, 8, "inventory");

	bool alt = false;

	for (int i = 0; i < hnd->num; i++) {
		const menu_t *p = hnd->entries + i;

		if (!*(p->text))
			continue;

		const char *t = p->text;

		if (*t == '*') {
			alt = true;
			t++;
		}
		const int y = 32 + i * 8;
		int x = 64;
		int caret_x = 56;
		const char *loc_func = "loc_string";

		if (p->align == MENU_ALIGN_CENTER) {
			x = 0;
			caret_x = 152;
			loc_func = "loc_cstring";
		} else if (p->align == MENU_ALIGN_RIGHT) {
			x = 260;
			caret_x = 252;
			loc_func = "loc_rstring";
		}
		P_Menu_Appendf(layout, layout_size, "yv %d ", y);
		P_Menu_Appendf(layout, layout_size, "xv %d %s%s 1 \"%s\" \"%s\" ", x, loc_func, (hnd->cur == i || alt) ? "2" : "", t, p->text_arg1);

		if (hnd->cur == i)
			P_Menu_Appendf(layout, layout_size, "xv %d string2 \"%s\" ", caret_x, ">\"");

		alt = false;
	}

	return std::strlen(layout);
}
