#include "g_activation.h"

/*
=============
BuildActivationMessagePlan

Creates an activation message plan for the provided context.
=============
*/
activation_message_plan_t BuildActivationMessagePlan(bool has_message, bool has_activator, bool activator_is_monster, bool coop_global, bool coop_enabled, int noise_index)
{
	activation_message_plan_t plan{};

	if (!has_message)
		return plan;

	if (coop_global && coop_enabled)
		plan.broadcast_global = true;

	if (!has_activator || activator_is_monster)
		return plan;

	plan.center_on_activator = true;

	if (noise_index >= 0)
	{
		plan.play_sound = true;
		plan.sound_index = noise_index;
	}

	return plan;
}
