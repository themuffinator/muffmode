// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

#include "g_local.h"

/*
=============
G_RunThinkImpl

Shared implementation for running entity think functions
=============
*/
template<typename Entity, typename Logger, typename Fallback>
bool G_RunThinkImpl(Entity *ent, const gtime_t &current_time, Logger &&log_warning, Fallback &&fallback_action) {
	gtime_t thinktime = ent->nextthink;
	if (thinktime <= 0_ms)
		return true;
	if (thinktime > current_time)
		return true;

	ent->nextthink = 0_ms;
	if (!ent->think) {
		log_warning(ent);
		fallback_action(ent);
		return false;
	}

	ent->think(ent);

	return false;
}
