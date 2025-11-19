#include "../src/g_activation.h"
#include <cassert>

/*
=============
main

Regression coverage for activation message planning.
=============
*/
int main()
{
	activation_message_plan_t no_activator_broadcast = BuildActivationMessagePlan(true, false, false, true, true, 5);
	assert(no_activator_broadcast.broadcast_global);
	assert(!no_activator_broadcast.center_on_activator);
	assert(!no_activator_broadcast.play_sound);

	activation_message_plan_t no_activator_silent = BuildActivationMessagePlan(true, false, false, false, true, 2);
	assert(!no_activator_silent.broadcast_global);
	assert(!no_activator_silent.center_on_activator);
	assert(!no_activator_silent.play_sound);

	activation_message_plan_t player_plan = BuildActivationMessagePlan(true, true, false, false, true, 0);
	assert(!player_plan.broadcast_global);
	assert(player_plan.center_on_activator);
	assert(player_plan.play_sound);
	assert(player_plan.sound_index == 0);

	return 0;
}
