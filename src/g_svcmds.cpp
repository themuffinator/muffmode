// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cerrno>
#include <system_error>

/*
=============
PackBytesToUint32

Assembles four bytes into a uint32_t using little-endian order to maintain
consistent packet filter behavior across platforms.
=============
*/
static uint32_t PackBytesToUint32(const byte bytes[4]) {
	return (static_cast<uint32_t>(bytes[0])) |
		(static_cast<uint32_t>(bytes[1]) << 8) |
		(static_cast<uint32_t>(bytes[2]) << 16) |
		(static_cast<uint32_t>(bytes[3]) << 24);
}

static void Svcmd_Test_f() {
	gi.LocClient_Print(nullptr, PRINT_HIGH, "Svcmd_Test_f()\n");
}

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire
class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single
host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and
restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the
default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that
only allows players from your local network.


==============================================================================
*/

struct ipfilter_t {
	uint32_t mask;
	uint32_t compare;
};

constexpr size_t MAX_IPFILTERS = 1024;

ipfilter_t ipfilters[MAX_IPFILTERS];
int		   numipfilters;

/*
=================
StringToFilter
=================
*/
static bool StringToFilter(const char *s, ipfilter_t *f) {
	char num[128];
	int      i, j;
	byte b[4];
	byte m[4];

	for (i = 0; i < 4; i++) {
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0; i < 4; i++) {
		if (*s < '0' || *s > '9') {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Bad filter address: {}\n", s);
			return false;
		}

		j = 0;
		while (*s >= '0' && *s <= '9') {
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = strtoul(num, nullptr, 10);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = PackBytesToUint32(m);
	f->compare = PackBytesToUint32(b);

	return true;
}

/*
=================
G_FilterPacket
=================
*/
bool G_FilterPacket(const char *from) {
	int              i;
	uint32_t in;
	byte     m[4];
	const char *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i] * 10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++;
		p++;
	}

	in = PackBytesToUint32(m);

	for (i = 0; i < numipfilters; i++)
		if ((in & ipfilters[i].mask) == ipfilters[i].compare)
			return filterban->integer;

	return !filterban->integer;
}

/*
=================
SVCmd_AddIP_f
=================
*/
static void SVCmd_AddIP_f() {
	int i;

	if (gi.argc() < 3) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv {} <ip-mask>\n", gi.argv(1));
		return;
	}

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break; // free spot
	if (i == numipfilters) {
		if (numipfilters == MAX_IPFILTERS) {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	if (!StringToFilter(gi.argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
G_RemoveIP_f
=================
*/
static void SVCmd_RemoveIP_f() {
	ipfilter_t f;
	int		   i, j;

	if (gi.argc() < 3) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv {} <ip-mask>\n", gi.argv(1));
		return;
	}

	if (!StringToFilter(gi.argv(2), &f))
		return;

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].mask == f.mask && ipfilters[i].compare == f.compare) {
			for (j = i + 1; j < numipfilters; j++)
				ipfilters[j - 1] = ipfilters[j];
			numipfilters--;
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Removed.\n");
			return;
		}
	gi.LocClient_Print(nullptr, PRINT_HIGH, "Didn't find {}.\n", gi.argv(2));
}

/*
=================
G_ListIP_f
=================
*/
static void SVCmd_ListIP_f() {
	int	 i;
	byte b[4];

	gi.LocClient_Print(nullptr, PRINT_HIGH, "Filter list:\n");
	for (i = 0; i < numipfilters; i++) {
		*(unsigned *)b = ipfilters[i].compare;
		gi.LocClient_Print(nullptr, PRINT_HIGH, "{}.{}.{}.{}\n", b[0], b[1], b[2], b[3]);
	}
}

// [Paril-KEX]
static void SVCmd_NextMap_f() {
	gi.LocBroadcast_Print(PRINT_HIGH, "$g_map_ended_by_server");
	Match_End();
}

/*
=================
SVCmd_WriteIP_f

Serializes the current filterban value and IP filters to listip.cfg using
platform-aware file handling.
=================
*/
static void SVCmd_WriteIP_f(void) {
	byte b[4];
	int i;
	cvar_t *game;

	game = gi.cvar("game", "", static_cast<cvar_flags_t>(0));

	std::filesystem::path listip_path = *game->string ? std::filesystem::path(game->string) : std::filesystem::path(GAMEVERSION);
	listip_path /= "listip.cfg";

	gi.LocClient_Print(nullptr, PRINT_HIGH, "Writing {}.\n", listip_path.string().c_str());

	if (!listip_path.parent_path().empty()) {
		std::error_code create_error;
		std::filesystem::create_directories(listip_path.parent_path(), create_error);
		if (create_error) {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Couldn't write {} ({})\n", listip_path.string().c_str(), create_error.message().c_str());
			return;
		}
	}

	std::ofstream file(listip_path, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open()) {
		const std::error_code open_error(errno, std::generic_category());
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Couldn't write {} ({})\n", listip_path.string().c_str(), open_error.message().c_str());
		return;
	}

	file << "set filterban " << filterban->integer << '\n';

	for (i = 0; i < numipfilters; i++) {
		*(unsigned *)b = ipfilters[i].compare;
		file << "sv addip " << static_cast<int>(b[0]) << '.' << static_cast<int>(b[1]) << '.' << static_cast<int>(b[2]) << '.' << static_cast<int>(b[3]) << '\n';
	}

	file.close();
	if (file.fail()) {
		const std::error_code close_error(errno, std::generic_category());
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Couldn't write {} ({})\n", listip_path.string().c_str(), close_error.message().c_str());
	}
}

/*
=================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/
void ServerCommand() {
	const char *cmd = gi.argv(1);

	if (Q_strcasecmp(cmd, "test") == 0)
		Svcmd_Test_f();
	else if (Q_strcasecmp(cmd, "addip") == 0)
		SVCmd_AddIP_f();
	else if (Q_strcasecmp(cmd, "removeip") == 0)
		SVCmd_RemoveIP_f();
	else if (Q_strcasecmp(cmd, "listip") == 0)
		SVCmd_ListIP_f();
	else if (Q_strcasecmp(cmd, "writeip") == 0)
		SVCmd_WriteIP_f();
	else if (Q_strcasecmp(cmd, "nextmap") == 0)
		SVCmd_NextMap_f();
	else
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Unknown server command \"{}\"\n", cmd);
}
