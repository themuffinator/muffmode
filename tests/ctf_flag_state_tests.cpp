#include "../src/g_local.h"
#include "../src/bots/bot_utils.h"
#include <cassert>
#include <cstring>
#include <type_traits>

local_game_import_t gi{};
level_locals_t level{};
game_export_t globals{};

static constexpr size_t kTestEntityCount = 8;
static std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)> g_entity_storage[kTestEntityCount];
gentity_t *g_entities = reinterpret_cast<gentity_t *>(g_entity_storage);
static size_t g_next_entity = 0;

/*
=============
AllocTestEntity

Provides zeroed entity storage for simulation tests.
=============
*/
static gentity_t *AllocTestEntity()
{
	assert(g_next_entity < kTestEntityCount);
	gentity_t *ent = reinterpret_cast<gentity_t *>(&g_entity_storage[g_next_entity++]);
	std::memset(ent, 0, sizeof(gentity_t));
	return ent;
}

/*
=============
StubInfoValueForKey

Provides a predictable value for name lookups during testing.
=============
*/
static size_t StubInfoValueForKey(const char *, const char *, char *buffer, size_t buffer_len)
{
	if (buffer_len > 0)
		buffer[0] = '\0';

	return 0;
}

/*
=============
StubBotRegisterEntity

Tracks entity registration attempts without engine side effects.
=============
*/
static void StubBotRegisterEntity(const gentity_t *)
{
}

/*
=============
StubTrace

Returns an empty trace result for planner lookups.
=============
*/
static trace_t StubTrace(gvec3_cref_t, gvec3_cptr_t, gvec3_cptr_t, gvec3_cref_t, const gentity_t *, contents_t)
{
	trace_t tr{};
	return tr;
}

/*
=============
StubComPrint

Ignores formatted bot utility logging during tests.
=============
*/
static void StubComPrint(const char *)
{
}

/*
=============
StubAngleVectors

Zeroes output vectors for tests that depend on angle calculations.
=============
*/
void AngleVectors(const vec3_t &, vec3_t &forward, vec3_t *right, vec3_t *up)
{
	forward = { 0.0f, 0.0f, 0.0f };
	if (right)
		*right = { 0.0f, 0.0f, 0.0f };
	if (up)
		*up = { 0.0f, 0.0f, 0.0f };
}

/*
=============
ClientIsPlaying
=============
*/
bool ClientIsPlaying(gclient_t *)
{
	return true;
}

/*
=============
ArmorIndex
=============
*/
item_id_t ArmorIndex(gentity_t *)
{
	return IT_ARMOR_BODY;
}

/*
=============
P_GetLobbyUserNum
=============
*/
unsigned int P_GetLobbyUserNum(const gentity_t *)
{
	return 0;
}

/*
=============
G_FreeEntity
=============
*/
void G_FreeEntity(gentity_t *)
{
}

/*
=============
MakeFlagItem

Creates a minimal flag item descriptor for tests.
=============
*/
static gitem_t MakeFlagItem(item_id_t id, const char *classname)
{
	gitem_t item{};
	item.id = id;
	item.classname = classname;
	return item;
}

/*
=============
MakeFlagEntity

Constructs a minimal flag entity for simulation tests.
=============
*/
static gentity_t *MakeFlagEntity(gitem_t *item, const vec3_t &origin)
{
	gentity_t *flag = AllocTestEntity();
	flag->item = item;
	flag->classname = item->classname;
	flag->s.origin = origin;
	flag->solid = SOLID_TRIGGER;
	flag->svflags = SVF_NONE;
	return flag;
}

/*
=============
TestFlagAtBaseState

Verifies that a visible flag at its spawn point is reported as home.
=============
*/
static void TestFlagAtBaseState()
{
	level.time = 0_ms;

	gitem_t red_flag_item = MakeFlagItem(IT_FLAG_RED, ITEM_CTF_FLAG_RED);
	gentity_t *flag = MakeFlagEntity(&red_flag_item, { 16.0f, -8.0f, 4.0f });

	Entity_UpdateState(flag);

	assert(flag->sv.team == TEAM_RED);
	assert(flag->sv.ent_flags & SVFL_IS_OBJECTIVE);
	assert(flag->sv.ent_flags & SVFL_OBJECTIVE_AT_BASE);
	assert(flag->sv.objective_state == objective_state_t::AtBase);
	assert(flag->sv.start_origin == flag->sv.end_origin);
}

/*
=============
TestFlagCarriedState

Ensures a hidden respawning flag is marked as carried.
=============
*/
static void TestFlagCarriedState()
{
	level.time = 5_sec;

	gitem_t blue_flag_item = MakeFlagItem(IT_FLAG_BLUE, ITEM_CTF_FLAG_BLUE);
	gentity_t *flag = MakeFlagEntity(&blue_flag_item, { -24.0f, 12.0f, 0.0f });
	flag->solid = SOLID_NOT;
	flag->svflags = SVF_NOCLIENT;
	flag->flags = FL_RESPAWN;

	Entity_UpdateState(flag);

	assert(flag->sv.team == TEAM_BLUE);
	assert(flag->sv.ent_flags & SVFL_IS_HIDDEN);
	assert(flag->sv.ent_flags & SVFL_OBJECTIVE_CARRIED);
	assert(flag->sv.objective_state == objective_state_t::Carried);
	assert(flag->sv.respawntime == Item_UnknownRespawnTime);
}

/*
=============
TestFlagDroppedState

Confirms a dropped flag reports its dropped state and return timer.
=============
*/
static void TestFlagDroppedState()
{
	level.time = 12_sec;

	gitem_t red_flag_item = MakeFlagItem(IT_FLAG_RED, ITEM_CTF_FLAG_RED);
	gentity_t *carrier = AllocTestEntity();
	gentity_t *flag = MakeFlagEntity(&red_flag_item, { 4.0f, 4.0f, 32.0f });
	flag->spawnflags = SPAWNFLAG_ITEM_DROPPED;
	flag->owner = carrier;
	flag->nextthink = level.time + 30_sec;

	Entity_UpdateState(flag);

	assert(flag->sv.ent_flags & SVFL_OBJECTIVE_DROPPED);
	assert(flag->sv.objective_state == objective_state_t::Dropped);
	assert(flag->sv.respawntime == 30000);
}

/*
=============
SetupTestEnvironment

Resets shared test state and installs required engine stubs.
=============
*/
static void SetupTestEnvironment()
{
	g_next_entity = 0;
	std::memset(g_entity_storage, 0, sizeof(g_entity_storage));
	std::memset(&level, 0, sizeof(level));
	std::memset(&globals, 0, sizeof(globals));
	gi = {};

	gi.Info_ValueForKey = StubInfoValueForKey;
	gi.Bot_RegisterEntity = StubBotRegisterEntity;
	gi.game_import_t::trace = StubTrace;
	gi.Com_Print = StubComPrint;
	globals.num_entities = kTestEntityCount;
}

/*
=============
main
=============
*/
int main()
{
	SetupTestEnvironment();

	TestFlagAtBaseState();
	TestFlagCarriedState();
	TestFlagDroppedState();

	return 0;
}
