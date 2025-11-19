/*
=============
BuildActivationMessagePlan

Defines the information needed to safely emit activation messaging and sounds.
=============
*/
#pragma once

struct activation_message_plan_t
{
	bool broadcast_global = false;
	bool center_on_activator = false;
	bool play_sound = false;
	int sound_index = -1;
};

/*
=============
BuildActivationMessagePlan

Creates an activation message plan for the provided context.
=============
*/
activation_message_plan_t BuildActivationMessagePlan(bool has_message, bool has_activator, bool activator_is_monster, bool coop_global, bool coop_enabled, int noise_index);
