// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "p_hud_victor.h"

#include <cassert>
#include <cstring>

/*
=============
TestTeamVictor

Verifies that team victories report the correct winning name and score spread.
=============
*/
static void TestTeamVictor() {
	intermission_victor_context_t context{};
	context.intermission_active = true;
	context.teams = true;
	context.red_name = "Red";
	context.blue_name = "Blue";
	context.red_score = 25;
	context.blue_score = 10;

	char buffer[64];
	const char *result = BuildIntermissionVictorString(context, buffer, sizeof(buffer));
	assert(result);
	assert(std::strcmp(result, "Red WINS with a final score of 25 to 10.") == 0);
}

/*
=============
TestFFAVictor

Ensures FFA results prefer the top scorer when there is no tie.
=============
*/
static void TestFFAVictor() {
	intermission_victor_context_t context{};
	context.intermission_active = true;
	context.ffa_winner_name = "PlayerOne";
	context.ffa_winner_score = 15;
	context.ffa_runner_up_present = true;
	context.ffa_runner_up_score = 10;

	char buffer[64];
	const char *result = BuildIntermissionVictorString(context, buffer, sizeof(buffer));
	assert(result);
	assert(std::strcmp(result, "PlayerOne WINS with a final score of 15.") == 0);
}

/*
=============
TestFFATie

Confirms that ties in FFA emit a tie-specific victor string.
=============
*/
static void TestFFATie() {
	intermission_victor_context_t context{};
	context.intermission_active = true;
	context.ffa_winner_name = "PlayerOne";
	context.ffa_winner_score = 12;
	context.ffa_runner_up_present = true;
	context.ffa_runner_up_score = 12;

	char buffer[64];
	const char *result = BuildIntermissionVictorString(context, buffer, sizeof(buffer));
	assert(result);
	assert(std::strcmp(result, "Match ended in a tie at 12.") == 0);
}

/*
=============
main
=============
*/
int main() {
	TestTeamVictor();
	TestFFAVictor();
	TestFFATie();
	return 0;
}
