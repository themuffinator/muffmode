#include "../src/g_activation.h"
#include <cstdio>

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
main

Regression coverage for activation message planning.
=============
*/
int main()
{
	activation_message_plan_t no_activator_broadcast = BuildActivationMessagePlan(true, false, false, true, true, 5);
	Expect(no_activator_broadcast.broadcast_global, "no_activator_broadcast.broadcast_global should be true");
	Expect(!no_activator_broadcast.center_on_activator, "no_activator_broadcast.center_on_activator should be false");
	Expect(!no_activator_broadcast.play_sound, "no_activator_broadcast.play_sound should be false");

	activation_message_plan_t no_activator_silent = BuildActivationMessagePlan(true, false, false, false, true, 2);
	Expect(!no_activator_silent.broadcast_global, "no_activator_silent.broadcast_global should be false");
	Expect(!no_activator_silent.center_on_activator, "no_activator_silent.center_on_activator should be false");
	Expect(!no_activator_silent.play_sound, "no_activator_silent.play_sound should be false");

	activation_message_plan_t player_plan = BuildActivationMessagePlan(true, true, false, false, true, 0);
	Expect(!player_plan.broadcast_global, "player_plan.broadcast_global should be false");
	Expect(player_plan.center_on_activator, "player_plan.center_on_activator should be true");
	Expect(player_plan.play_sound, "player_plan.play_sound should be true");
	Expect(player_plan.sound_index == 0, "player_plan.sound_index should be 0");

	return g_failures == 0 ? 0 : 1;
}
