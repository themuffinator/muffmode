#include "../src/g_local.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <strings.h>
#include <string>
#include <vector>

local_game_import_t gi{};
g_fmt_data_t g_fmt_data{};

static int g_failures = 0;
static std::vector<std::string> g_messages;

std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers;
std::array<const char *, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs;

static char game_name[] = "game";
static std::vector<char> game_string_storage;
static cvar_t game_cvar{
	game_name,
	nullptr,
	nullptr,
	static_cast<cvar_flags_t>(0),
	0,
	0.0f,
	nullptr,
	0
};

static char filterban_name[] = "filterban";
static char filterban_value[] = "1";
static cvar_t filterban_stub{
	filterban_name,
	filterban_value,
	nullptr,
	static_cast<cvar_flags_t>(0),
	0,
	0.0f,
	nullptr,
	1
};

cvar_t *filterban = &filterban_stub;

/*
=============
StubArgc

Provides a default argc of zero for command stubs.
=============
*/
static int StubArgc()
{
	return 0;
}

/*
=============
StubArgv

Provides empty argv content for command stubs.
=============
*/
static const char *StubArgv(int)
{
	return "";
}

/*
=============
StubCvar

Returns the configured game cvar or nullptr for unknown names.
=============
*/
static cvar_t *StubCvar(const char *var_name, const char *, cvar_flags_t)
{
	if (std::strcmp(var_name, game_name) == 0)
	{
		return &game_cvar;
	}

	return nullptr;
}

/*
=============
StubLocPrint

Captures localization print requests for verification.
=============
*/
static void StubLocPrint(gentity_t *, print_type_t, const char *base, const char **args, size_t num_args)
{
	std::string message = base;
	for (size_t i = 0; i < num_args; ++i)
	{
		message += args[i];
	}

	g_messages.push_back(message);
}

/*
=============
Match_End

Stubbed to satisfy linkage when including g_svcmds.cpp.
=============
*/
void Match_End()
{
}

#include "../src/g_svcmds.cpp"

/*
=============
Q_strcasecmp

Implements case-insensitive string comparison for test coverage.
=============
*/
int Q_strcasecmp(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2);
}

/*
=============
Expect

Reports a failed expectation and increments the failure counter.
=============
*/
static void Expect(bool condition, const char *message)
{
	if (!condition)
	{
		std::fprintf(stderr, "Expectation failed: %s\n", message);
		++g_failures;
	}
}

/*
=============
ConfigureGamePath

Assigns the game cvar's string storage to the provided path.
=============
*/
static void ConfigureGamePath(const std::filesystem::path &path)
{
	const auto string_data = path.string();
	game_string_storage.assign(string_data.begin(), string_data.end());
	game_string_storage.push_back('\0');
	game_cvar.string = game_string_storage.data();
}

/*
=============
ReadFileContents

Returns the full text of the specified file.
=============
*/
static std::string ReadFileContents(const std::filesystem::path &path)
{
	std::ifstream stream(path);
	return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/*
=============
ValidateWriteIpSerialization

Ensures writeip persists filterban and IP filters to listip.cfg.
=============
*/
static void ValidateWriteIpSerialization()
{
	gi.argc = &StubArgc;
	gi.argv = &StubArgv;
	gi.cvar = &StubCvar;
	gi.Loc_Print = &StubLocPrint;

	numipfilters = 0;
	Expect(StringToFilter("10.0.0.1", &ipfilters[numipfilters++]), "Should parse 10.0.0.1");
	Expect(StringToFilter("192.168.5.0", &ipfilters[numipfilters++]), "Should parse 192.168.5.0");

	std::filesystem::path output_dir = std::filesystem::temp_directory_path() / "muffmode_writeip";
	std::filesystem::remove_all(output_dir);
	std::filesystem::create_directories(output_dir);
	ConfigureGamePath(output_dir);

	SVCmd_WriteIP_f();

	std::filesystem::path output_file = output_dir / "listip.cfg";
	Expect(std::filesystem::exists(output_file), "listip.cfg should be created");

	const std::string file_contents = ReadFileContents(output_file);
	const std::string expected_contents = "set filterban 1\nsv addip 10.0.0.1\nsv addip 192.168.5.0\n";
	Expect(file_contents == expected_contents, "listip.cfg should include filterban and both entries");

	std::filesystem::remove_all(output_dir);
}

/*
=============
main

Executes writeip serialization coverage.
=============
*/
int main()
{
	ValidateWriteIpSerialization();
	return g_failures == 0 ? 0 : 1;
}
