// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "p_hud_victor.h"

#include <cstdio>

/*
=============
BuildIntermissionVictorString

Populates the provided buffer with the appropriate victor string for the current match context.
Returns either the buffer pointer or nullptr if no message is generated.
=============
*/
const char *BuildIntermissionVictorString(const intermission_victor_context_t &context, char *buffer, size_t buffer_size) {
	if (!buffer || !buffer_size)
		return nullptr;

	buffer[0] = '\0';

	if (!context.intermission_active)
		return nullptr;

	if (context.existing_message && context.existing_message[0]) {
		std::snprintf(buffer, buffer_size, "%s", context.existing_message);
		return buffer;
	}

	if (context.teams) {
		if (context.red_score > context.blue_score && context.red_name) {
			std::snprintf(buffer, buffer_size, "%s WINS with a final score of %d to %d.", context.red_name, context.red_score, context.blue_score);
		} else if (context.blue_score > context.red_score && context.blue_name) {
			std::snprintf(buffer, buffer_size, "%s WINS with a final score of %d to %d.", context.blue_name, context.blue_score, context.red_score);
		} else if (context.red_name && context.blue_name) {
			std::snprintf(buffer, buffer_size, "Match is a tie: %d to %d.", context.red_score, context.blue_score);
		}
	} else if (context.ffa_winner_name) {
		if (context.ffa_runner_up_present && context.ffa_runner_up_score == context.ffa_winner_score) {
			std::snprintf(buffer, buffer_size, "Match ended in a tie at %d.", context.ffa_winner_score);
		} else {
			std::snprintf(buffer, buffer_size, "%s WINS with a final score of %d.", context.ffa_winner_name, context.ffa_winner_score);
		}
	}

	return buffer[0] ? buffer : nullptr;
}
