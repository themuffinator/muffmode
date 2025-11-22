#include "../src/g_local.h"
#include <cassert>

#define MONSTER_SPAWN_TESTS
#include "../src/g_monster_spawn.cpp"

// Stub globals expected by the included code
local_game_import_t gi{};
gentity_t *world = reinterpret_cast<gentity_t *>(0x1);

static int g_create_monster_calls = 0;

/*
=============
ClearTrace

Provides a non-solid trace result for spawn validation.
=============
*/
static trace_t ClearTrace(const vec3_t &start, const vec3_t &, const vec3_t &, const vec3_t &end, gentity_t *, contents_t)
{
	trace_t tr{};

	tr.ent = world;
	tr.start = start;
	tr.endpos = end;
	tr.fraction = 1.0f;
	tr.startsolid = false;
	tr.allsolid = false;

	return tr;
}

/*
=============
ZeroContents

Indicates empty contents for spawn point checks.
=============
*/
static contents_t ZeroContents(const vec3_t &)
{
	return static_cast<contents_t>(0);
}

/*
=============
G_FixStuckObject_Generic

Trivial stub to satisfy FindSpawnPoint dependency.
=============
*/
stuck_result_t G_FixStuckObject_Generic(vec3_t &origin, const vec3_t &, const vec3_t &, std::function<stuck_object_trace_fn_t>)
{
	(void)origin;
	return stuck_result_t::GOOD_POSITION;
}

/*
=============
M_CheckBottom_Fast_Generic

Records the provided gravity vector for verification.
=============
*/
bool M_CheckBottom_Fast_Generic(const vec3_t &, const vec3_t &, const vec3_t &)
{
	return false;
}

/*
=============
M_CheckBottom_Slow_Generic

Records the provided gravity vector for verification.
=============
*/
bool M_CheckBottom_Slow_Generic(const vec3_t &, const vec3_t &, const vec3_t &, gentity_t *, contents_t, const vec3_t &, bool)
{
	return true;
}

/*
=============
M_droptofloor_generic

Drops an origin along the provided gravity vector until contact is made or a blocking volume is found.
=============
*/
bool M_droptofloor_generic(vec3_t &origin, const vec3_t &mins, const vec3_t &maxs, const vec3_t &gravityVector, gentity_t *ignore, contents_t mask, bool allow_partial)
{
	vec3_t gravity_dir = gravityVector.normalized();

	if (!gravity_dir)
		gravity_dir = { 0.0f, 0.0f, -1.0f };

	trace_t trace = gi.trace(origin, mins, maxs, origin, ignore, mask);

	if (trace.startsolid)
		origin -= gravity_dir;

	vec3_t end = origin + (gravity_dir * 256.0f);

	trace = gi.trace(origin, mins, maxs, end, ignore, mask);

	if (trace.fraction == 1 || trace.allsolid || (!allow_partial && trace.startsolid))
		return false;

	origin = trace.endpos;

	return true;
}

/*
=============
CreateMonster

Stubbed CreateMonster to simulate allocation failure.
=============
*/
gentity_t *CreateMonster(const vec3_t &, const vec3_t &, const char *)
{
	g_create_monster_calls++;
	return nullptr;
}

/*
=============
main

Verifies CreateFlyMonster returns nullptr when the monster creation fails.
=============
*/
int main()
{
	gi.trace = ClearTrace;
	gi.pointcontents = ZeroContents;

	vec3_t origin{ 0.0f, 0.0f, 0.0f };
	vec3_t angles{ 0.0f, 90.0f, 0.0f };
	vec3_t mins{ -16.0f, -16.0f, -16.0f };
	vec3_t maxs{ 16.0f, 16.0f, 16.0f };

	gentity_t *result = CreateFlyMonster(origin, angles, mins, maxs, "monster_test");

	assert(g_create_monster_calls == 1);
	assert(result == nullptr);

	return 0;
}
