#include "../src/g_local.h"
#include <cstdio>
#include <vector>

static int g_failures = 0;

/*
=============
Expect

Reports a failed expectation and increments the failure counter.
=============
*/
static void Expect(bool condition, const char *message)
{
	if (!condition)
	{
		std::fprintf(stderr, "Expectation failed: %s\n", message);
		++g_failures;
	}
}

/*
=============
TestAlivePlayerPreventsElimination

Verifies that a living player keeps the wave active regardless of lives.
=============
*/
static void TestAlivePlayerPreventsElimination()
{
	std::vector<player_life_state_t> states = {
		{ true, false, 100, 0 },
		{ true, false, -10, 0 }
	};

	Expect(!Horde_NoLivesRemain(states), "Living players should block elimination");
}

/*
=============
TestRemainingLivesPreventFailure

Ensures remaining lives keep players eligible to respawn.
=============
*/
static void TestRemainingLivesPreventFailure()
{
	std::vector<player_life_state_t> states = {
		{ true, false, -20, 2 },
		{ true, false, -5, 0 }
	};

	Expect(!Horde_NoLivesRemain(states), "Remaining lives should prevent elimination");
}

/*
=============
TestNoLivesTriggersElimination

Confirms that exhausted lives across the roster force elimination.
=============
*/
static void TestNoLivesTriggersElimination()
{
	std::vector<player_life_state_t> states = {
		{ true, false, -10, 0 },
		{ true, true, -15, 0 }
	};

	Expect(Horde_NoLivesRemain(states), "No lives should trigger elimination");
}

/*
=============
TestEmptyRosterTriggersElimination

Ensures waves complete when no playing clients remain.
=============
*/
static void TestEmptyRosterTriggersElimination()
{
	std::vector<player_life_state_t> states;

	Expect(Horde_NoLivesRemain(states), "No playing clients should end the wave");
}

/*
=============
main

Runs Horde life exhaustion regression checks.
=============
*/
int main()
{
	TestAlivePlayerPreventsElimination();
	TestRemainingLivesPreventFailure();
	TestNoLivesTriggersElimination();
	TestEmptyRosterTriggersElimination();

	return g_failures == 0 ? 0 : 1;
}
