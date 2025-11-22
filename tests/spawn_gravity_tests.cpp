#include "../src/g_local.h"
#include <cassert>

// Limit g_monster_spawn.cpp surface area for unit tests
#define MONSTER_SPAWN_TESTS
#include "../src/g_monster_spawn.cpp"

// Stub globals expected by the included code
local_game_import_t gi{};
gentity_t *world = reinterpret_cast<gentity_t *>(0x1);

static int g_trace_calls = 0;
static vec3_t g_last_trace_start;
static vec3_t g_last_trace_end;
static vec3_t g_last_bottom_fast;
static vec3_t g_last_bottom_slow;
static bool g_create_monster_should_fail = false;

/*
=============
TestPointContents

Treats all points as non-water for spawn validation.
=============
*/
static contents_t TestPointContents(const vec3_t &)
{
	return 0;
}

/*
=============
CreateMonster

Returns nullptr when configured to simulate spawn failure.
=============
*/
gentity_t *CreateMonster(const vec3_t &origin, const vec3_t &angles, const char *classname)
{
	if (g_create_monster_should_fail)
		return nullptr;

	static gentity_t dummy_ent{};

	dummy_ent.s.origin = origin;
	dummy_ent.s.angles = angles;
	dummy_ent.classname = classname;

	return &dummy_ent;
}

/*
=============
TestTrace

Provides deterministic trace behavior for spawn validation tests.
=============
*/
static trace_t TestTrace(const vec3_t &start, const vec3_t &, const vec3_t &, const vec3_t &end, gentity_t *, contents_t)
{
	trace_t tr{};

	g_last_trace_start = start;
	g_last_trace_end = end;
	g_trace_calls++;

	tr.ent = world;
	tr.endpos = end;
	tr.fraction = 0.5f;
	tr.startsolid = false;
	tr.allsolid = false;

	return tr;
}

/*
=============
NoHitTrace

Returns a full-length trace with no impact to simulate missing ground.
=============
*/
static trace_t NoHitTrace(const vec3_t &start, const vec3_t &, const vec3_t &, const vec3_t &end, gentity_t *, contents_t)
{
	trace_t tr{};

	g_last_trace_start = start;
	g_last_trace_end = end;
	g_trace_calls++;

	tr.ent = world;
	tr.endpos = end;
	tr.fraction = 1.0f;
	tr.startsolid = false;
	tr.allsolid = false;

	return tr;
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
bool M_CheckBottom_Fast_Generic(const vec3_t &, const vec3_t &, const vec3_t &gravityVector)
{
	g_last_bottom_fast = gravityVector;
	return false;
}

/*
=============
M_CheckBottom_Slow_Generic

Records the provided gravity vector for verification.
=============
*/
bool M_CheckBottom_Slow_Generic(const vec3_t &, const vec3_t &, const vec3_t &, gentity_t *, contents_t, const vec3_t &gravityVector, bool)
{
	g_last_bottom_slow = gravityVector;
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
TestCeilingDropUsesGravity

Verifies FindSpawnPoint drops along positive gravity vectors.
=============
*/
static void TestCeilingDropUsesGravity()
{
	gi.trace = TestTrace;
	g_trace_calls = 0;

	vec3_t start{ 0.0f, 0.0f, 0.0f };
	vec3_t mins{ -1.0f, -1.0f, -1.0f };
	vec3_t maxs{ 1.0f, 1.0f, 1.0f };
	vec3_t spawn{};

	bool found = FindSpawnPoint(start, mins, maxs, spawn, 32.0f, true, { 0.0f, 0.0f, 1.0f });

	assert(found);
	assert(g_trace_calls >= 2);
	assert(spawn[0] == 0.0f && spawn[1] == 0.0f && spawn[2] == 256.0f);
	assert(g_last_trace_end[2] == 256.0f);
}

/*
=============
TestWallGravityProjectsSpawnVolume

Ensures CheckSpawnPoint traces along the supplied gravity vector.
=============
*/
static void TestWallGravityProjectsSpawnVolume()
{
	gi.trace = TestTrace;
	g_trace_calls = 0;
	
	vec3_t origin{ 5.0f, 5.0f, 5.0f };
	vec3_t mins{ -2.0f, -2.0f, -2.0f };
	vec3_t maxs{ 2.0f, 2.0f, 2.0f };
	vec3_t gravity{ 1.0f, 0.0f, 0.0f };
	
	bool clear = CheckSpawnPoint(origin, mins, maxs, gravity);
	
	assert(clear);
	assert(g_trace_calls >= 2);
	
	vec3_t gravity_dir = gravity.normalized();
	
	if (!gravity_dir)
		gravity_dir = { 0.0f, 0.0f, -1.0f };
	
	vec3_t abs_gravity_dir = gravity_dir.abs();
	const float negative_extent = Q_fabs(DotProduct(mins, abs_gravity_dir));
	vec3_t expected_end = origin + (gravity_dir * negative_extent);
	
	assert(g_last_trace_start == origin);
	assert(g_last_trace_end == expected_end);
}

/*
=============
TestNegativeGravityProjectsSpawnVolume

Ensures CheckSpawnPoint projects extents correctly when gravity points into negative axes.
=============
*/
static void TestNegativeGravityProjectsSpawnVolume()
{
	gi.trace = TestTrace;
	g_trace_calls = 0;
	
	vec3_t origin{ -12.0f, 4.0f, 8.0f };
	vec3_t mins{ -3.0f, -1.0f, -2.0f };
	vec3_t maxs{ 5.0f, 2.0f, 3.0f };
	vec3_t gravity{ -1.0f, -0.5f, 0.0f };
	
	bool clear = CheckSpawnPoint(origin, mins, maxs, gravity);
	
	assert(clear);
	assert(g_trace_calls >= 2);
	
	vec3_t gravity_dir = gravity.normalized();
	
	if (!gravity_dir)
		gravity_dir = { 0.0f, 0.0f, -1.0f };
	
	vec3_t abs_gravity_dir = gravity_dir.abs();
	const float negative_extent = Q_fabs(DotProduct(mins, abs_gravity_dir));
	vec3_t expected_end = origin + (gravity_dir * negative_extent);
	
	assert(g_last_trace_start == origin);
	assert(g_last_trace_end == expected_end);
}

/*
=============
TestGroundChecksUseGravityVector

Confirms bottom checks receive the supplied gravity vector.
=============
*/
static void TestGroundChecksUseGravityVector()
{
	gi.trace = TestTrace;
	g_last_bottom_fast = { 0.0f, 0.0f, 0.0f };
	g_last_bottom_slow = { 0.0f, 0.0f, 0.0f };

	vec3_t origin{ 0.0f, 0.0f, 0.0f };
	vec3_t mins{ -8.0f, -8.0f, -8.0f };
	vec3_t maxs{ 8.0f, 8.0f, 8.0f };
	vec3_t gravity{ 0.0f, 1.0f, 0.0f };

	bool grounded = CheckGroundSpawnPoint(origin, mins, maxs, 128.0f, gravity);

	assert(grounded);
	assert(g_last_bottom_fast == gravity);
	assert(g_last_bottom_slow == gravity);
}

/*
=============
TestGroundSpawnHonorsHeight

Fails ground spawn validation when the surface is beyond the allowed height.
=============
*/
static void TestGroundSpawnHonorsHeight()
{
	gi.trace = NoHitTrace;
	g_trace_calls = 0;
	g_last_trace_start = { 0.0f, 0.0f, 0.0f };
	g_last_trace_end = { 0.0f, 0.0f, 0.0f };

	vec3_t origin{ 0.0f, 0.0f, 0.0f };
	vec3_t mins{ -8.0f, -8.0f, -8.0f };
	vec3_t maxs{ 8.0f, 8.0f, 8.0f };
	vec3_t gravity{ 0.0f, 0.0f, -1.0f };
	float height = 64.0f;

	bool grounded = CheckGroundSpawnPoint(origin, mins, maxs, height, gravity);

	assert(!grounded);
	assert(g_trace_calls >= 3);

	vec3_t gravity_dir = gravity.normalized();

	if (!gravity_dir)
		gravity_dir = { 0.0f, 0.0f, -1.0f };

	vec3_t expected_end = origin + (gravity_dir * height);

	assert(g_last_trace_start == origin);
	assert(g_last_trace_end == expected_end);
}

/*
=============
TestCreateFlyMonsterReturnsNullOnFailedSpawn

Ensures CreateFlyMonster returns nullptr when CreateMonster fails.
=============
*/
static void TestCreateFlyMonsterReturnsNullOnFailedSpawn()
{
	gi.trace = TestTrace;
	gi.pointcontents = TestPointContents;
	g_create_monster_should_fail = true;

	vec3_t origin{ 12.0f, -4.0f, 20.0f };
	vec3_t angles{ 0.0f, 45.0f, 0.0f };
	vec3_t mins{ -4.0f, -4.0f, -4.0f };
	vec3_t maxs{ 4.0f, 4.0f, 4.0f };

	gentity_t *spawned = CreateFlyMonster(origin, angles, mins, maxs, "monster_flyer");

	assert(spawned == nullptr);

	g_create_monster_should_fail = false;
}

/*
=============
TestCreateGroundMonsterReturnsNullOnFailedSpawn

Ensures CreateGroundMonster returns nullptr when CreateMonster fails.
=============
*/
static void TestCreateGroundMonsterReturnsNullOnFailedSpawn()
{
	gi.trace = TestTrace;
	gi.pointcontents = TestPointContents;
	g_create_monster_should_fail = true;

	vec3_t origin{ 16.0f, 8.0f, 4.0f };
	vec3_t angles{ 0.0f, 90.0f, 0.0f };
	vec3_t mins{ -8.0f, -8.0f, -8.0f };
	vec3_t maxs{ 8.0f, 8.0f, 8.0f };

	gentity_t *spawned = CreateGroundMonster(origin, angles, mins, maxs, "monster_infantry", 64.0f);

	assert(spawned == nullptr);
}

/*
=============
main
=============
*/
int main()
{
	gi.pointcontents = TestPointContents;
	TestCeilingDropUsesGravity();
	TestWallGravityProjectsSpawnVolume();
	TestNegativeGravityProjectsSpawnVolume();
	TestGroundChecksUseGravityVector();
	TestGroundSpawnHonorsHeight();
	TestCreateFlyMonsterReturnsNullOnFailedSpawn();
	TestCreateGroundMonsterReturnsNullOnFailedSpawn();
	return 0;
}
