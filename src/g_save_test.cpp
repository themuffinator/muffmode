#include <cassert>
#include <cstring>

#include "g_local.h"

// Stub globals required by g_save.cpp
local_game_import_t gi{};
g_fmt_data_t g_fmt_data{};

cvar_t strict_stub{
	const_cast<char *>("g_strict_saves"),
	const_cast<char *>("0"),
	nullptr,
	static_cast<cvar_flags_t>(0),
	0,
	0.0f,
	nullptr,
	0
};

cvar_t deathmatch_stub{
	const_cast<char *>("deathmatch"),
	const_cast<char *>("0"),
	nullptr,
	static_cast<cvar_flags_t>(0),
	0,
	0.0f,
	nullptr,
	0
};

cvar_t *g_strict_saves = &strict_stub;
cvar_t *deathmatch = &deathmatch_stub;

/*
=============
StubComPrint

Captures formatted error output for validation.
=============
*/
static void StubComPrint(const char *message) {
	(void)message;
}

/*
=============
StubComError

Captures fatal errors for validation.
=============
*/
static void StubComError(const char *message) {
	(void)message;
}

#include "g_save.cpp"

/*
=============
ValidateUnknownTagLookup

Ensures save_data_list_t::fetch returns an invalid sentinel for unknown pointers.
=============
*/
static void ValidateUnknownTagLookup() {
	strict_stub.integer = 0;
	deathmatch_stub.integer = 0;
	gi.Com_Print = &StubComPrint;
	gi.Com_Error = &StubComError;

	const void *unknown_ptr = reinterpret_cast<const void *>(0x1234);
	const save_data_list_t *result = save_data_list_t::fetch(unknown_ptr, SAVE_FUNC_THINK);

	assert(result != nullptr);
	assert(!result->valid);
	assert(result->ptr == unknown_ptr);
	assert(result->tag == SAVE_FUNC_THINK);
}

/*
=============
main
=============
*/
int main() {
	ValidateUnknownTagLookup();
	return 0;
}
