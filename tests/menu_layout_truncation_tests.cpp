#include "../src/game.h"
#include "../src/p_menu.h"
#include "../src/q_std.h"
#include <cassert>
#include <cstring>

/*
=============
main

Validate that long menu entries are safely truncated when constructing status bar layouts.
=============
*/
int main()
{
	menu_t entries[6] = {};

	for (auto &entry : entries) {
		memset(entry.text, 'x', sizeof(entry.text) - 1);
		entry.text[sizeof(entry.text) - 1] = '\0';
	}

	menu_hnd_t handle{};
	handle.entries = entries;
	handle.num = sizeof(entries) / sizeof(entries[0]);
	handle.cur = 0;

	char small_buffer[64];
	bool truncated_small = P_Menu_BuildLayoutString(&handle, small_buffer, sizeof(small_buffer));

	assert(truncated_small);
	assert(small_buffer[sizeof(small_buffer) - 1] == '\0');
	assert(strlen(small_buffer) < sizeof(small_buffer));

	char large_buffer[MAX_STRING_CHARS];
	memset(large_buffer, 0, sizeof(large_buffer));
	bool truncated_large = P_Menu_BuildLayoutString(&handle, large_buffer, sizeof(large_buffer));

	assert(truncated_large);
	assert(large_buffer[sizeof(large_buffer) - 1] == '\0');
	assert(strlen(large_buffer) < sizeof(large_buffer));

	menu_t short_entry{};
	Q_strlcpy(short_entry.text, "Short entry", sizeof(short_entry.text));

	menu_hnd_t short_handle{};
	short_handle.entries = &short_entry;
	short_handle.num = 1;
	short_handle.cur = 0;

	char comfy_buffer[128] = {};
	bool truncated_short = P_Menu_BuildLayoutString(&short_handle, comfy_buffer, sizeof(comfy_buffer));

	assert(!truncated_short);
	assert(strlen(comfy_buffer) < sizeof(comfy_buffer));

	return 0;
}
