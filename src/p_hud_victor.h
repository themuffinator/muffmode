// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

#include <cstddef>

struct intermission_victor_context_t {
	bool intermission_active{};
	const char *existing_message{};
	bool teams{};
	int red_score{};
	int blue_score{};
	const char *red_name{};
	const char *blue_name{};
	const char *ffa_winner_name{};
	int ffa_winner_score{};
	bool ffa_runner_up_present{};
	int ffa_runner_up_score{};
};

/*
=============
BuildIntermissionVictorString

Populates the provided buffer with the appropriate victor string for the current match context.
Returns either the buffer pointer or nullptr if no message is generated.
=============
*/
const char *BuildIntermissionVictorString(const intermission_victor_context_t &context, char *buffer, size_t buffer_size);
