// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

#include "q_std.h"
#include "q_vec3.h"

/*
=============
SpawnGravityDirection

Returns a normalized gravity direction, defaulting to standard down if the
input is zero.
=============
*/
inline vec3_t SpawnGravityDirection(const vec3_t &gravityVector)
{
	vec3_t gravityDir = gravityVector;

	if (!gravityDir.normalize())
		gravityDir = { 0, 0, -1 };

	return gravityDir;
}

/*
=============
SpawnDropEndpoint

Projects an origin along gravity by a specified distance.
=============
*/
inline vec3_t SpawnDropEndpoint(const vec3_t &origin, const vec3_t &gravityVector, float distance)
{
	return origin + (SpawnGravityDirection(gravityVector) * distance);
}

/*
=============
SpawnClearanceOrigin

Moves an origin against gravity to find headroom clearance.
=============
*/
inline vec3_t SpawnClearanceOrigin(const vec3_t &origin, const vec3_t &gravityVector, float distance)
{
	return origin - (SpawnGravityDirection(gravityVector) * distance);
}
