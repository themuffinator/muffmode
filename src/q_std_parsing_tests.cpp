#include "q_std.cpp"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

local_game_import_t gi{};
char local_game_import_t::print_buffer[0x10000];
static std::vector<std::string> g_print_buffer;

/*
=============
DummyComPrint

Captures formatted output for verification without relying on the engine.
=============
*/
static void DummyComPrint(const char *msg)
{
	if (msg)
		g_print_buffer.emplace_back(msg);
}

/*
=============
ResetPrintBuffer
=============
*/
static void ResetPrintBuffer()
{
	g_print_buffer.clear();
}

/*
=============
TestZeroLengthBufferGuard

Ensures zero-length buffers are guarded and parsing still returns a token.
=============
*/
static void TestZeroLengthBufferGuard()
{
	const char *data = "token";
	const char *cursor = data;
	char buffer[] = "Z";
	bool overflowed = false;
	ResetPrintBuffer();
	char *token = COM_ParseEx(&cursor, "\r\n	 ", buffer, 0, &overflowed);
	assert(token != nullptr);
	assert(overflowed);
	assert(std::strcmp(token, "token") == 0);
	assert(buffer[0] == 'Z');
}

/*
=============
TestOversizedTokenFlag

Confirms oversized tokens set the overflow flag and truncate instead of clearing.
=============
*/
static void TestOversizedTokenFlag()
{
	const char *data = "oversize";
	const char *cursor = data;
	char buffer[5];
	bool overflowed = false;
	ResetPrintBuffer();
	char *token = COM_ParseEx(&cursor, "\r\n	 ", buffer, sizeof(buffer), &overflowed);
	assert(token != nullptr);
	assert(overflowed);
	assert(std::strcmp(token, "over") == 0);
	assert(!g_print_buffer.empty());
}

/*
=============
TestExactFitToken

Verifies tokens that fit exactly within the buffer size do not trigger overflow handling.
=============
*/
static void TestExactFitToken()
{
	const char *data = "fits";
	const char *cursor = data;
	char buffer[5];
	bool overflowed = false;
	ResetPrintBuffer();
	char *token = COM_ParseEx(&cursor, "\r\n	 ", buffer, sizeof(buffer), &overflowed);
	assert(token != nullptr);
	assert(!overflowed);
	assert(std::strcmp(token, "fits") == 0);
	assert(g_print_buffer.empty());
}

/*
=============
main
=============
*/
int main()
{
	gi.Com_Print = DummyComPrint;
	TestZeroLengthBufferGuard();
	TestOversizedTokenFlag();
	TestExactFitToken();
	return 0;
}
