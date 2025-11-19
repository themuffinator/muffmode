#include "../src/spawn_gravity.h"
#include <cassert>

/*
=============
main

Verifies gravity projection helpers respond correctly to varied gravity vectors.
=============
*/
int main()
{
	vec3_t defaultDir = SpawnGravityDirection({ 0, 0, 0 });
	assert(defaultDir.equals({ 0, 0, -1 }));

	vec3_t ceilingDir = SpawnGravityDirection({ 0, 0, 5 });
	assert(ceilingDir.equals({ 0, 0, 1 }));

	vec3_t wallDir = SpawnGravityDirection({ -4, 0, 0 });
	assert(wallDir.equals({ -1, 0, 0 }));

	vec3_t downEndpoint = SpawnDropEndpoint({ 0, 0, 0 }, { 0, 0, -2 }, 128.f);
	assert(downEndpoint.equals({ 0, 0, -128 }));

	vec3_t ceilingEndpoint = SpawnDropEndpoint({ 4, 8, 12 }, { 0, 0, 3 }, 64.f);
	assert(ceilingEndpoint.equals({ 4, 8, 76 }));

	vec3_t wallEndpoint = SpawnDropEndpoint({ 10, 0, 0 }, { -1, 0, 0 }, 32.f);
	assert(wallEndpoint.equals({ -22, 0, 0 }));

	vec3_t clearance = SpawnClearanceOrigin({ 100, 50, 25 }, { 0, 0, 2 }, 8.f);
	assert(clearance.equals({ 100, 50, 17 }));

	vec3_t wallClearance = SpawnClearanceOrigin({ 16, -4, 0 }, { -8, 0, 0 }, 4.f);
	assert(wallClearance.equals({ 20, -4, 0 }));

	return 0;
}
