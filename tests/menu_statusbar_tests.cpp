#include "../src/p_menu.h"
#include "../src/q_std.h"

#include <cassert>
#include <cstring>

constexpr size_t MAX_STRING_CHARS = 1024;

/*
=============
MakeMenuEntry

Constructs a menu_t with the supplied label and default alignment.
=============
*/
static menu_t MakeMenuEntry(const char *label)
{
	menu_t entry{};
	Q_strlcpy(entry.text, label, sizeof(entry.text));
	entry.align = MENU_ALIGN_LEFT;
	entry.SelectFunc = nullptr;
	entry.text_arg1[0] = '\0';
	return entry;
}

/*
=============
main

Validates that status bar layout construction clamps long menu text safely.
=============
*/
int main()
{
	menu_t entries[4];
	entries[0] = MakeMenuEntry("Short");
	entries[1] = MakeMenuEntry("*Highlighted");
	entries[2] = MakeMenuEntry("Centered");
	entries[2].align = MENU_ALIGN_CENTER;
	entries[3] = MakeMenuEntry("Right");
	entries[3].align = MENU_ALIGN_RIGHT;

	menu_hnd_t hnd{};
	hnd.entries = entries;
	hnd.cur = 1;
	hnd.num = static_cast<int>(sizeof(entries) / sizeof(entries[0]));

	char layout[MAX_STRING_CHARS] = {};
	const size_t short_length = P_Menu_BuildStatusBar(&hnd, layout, sizeof(layout));
	assert(short_length < MAX_STRING_CHARS);
	assert(layout[MAX_STRING_CHARS - 1] == '\0');

	char oversized_text[sizeof(entries[0].text)];
	std::memset(oversized_text, 'Z', sizeof(oversized_text));
	oversized_text[sizeof(oversized_text) - 1] = '\0';

	menu_t long_entries[6];
	for (auto &entry : long_entries)
		entry = MakeMenuEntry(oversized_text);

	menu_hnd_t long_hnd{};
	long_hnd.entries = long_entries;
	long_hnd.cur = 0;
	long_hnd.num = static_cast<int>(sizeof(long_entries) / sizeof(long_entries[0]));

	std::memset(layout, 'X', sizeof(layout));
	layout[sizeof(layout) - 1] = '\0';
	const size_t long_length = P_Menu_BuildStatusBar(&long_hnd, layout, sizeof(layout));

	assert(long_length == MAX_STRING_CHARS - 1);
	assert(layout[sizeof(layout) - 1] == '\0');

	return 0;
}
