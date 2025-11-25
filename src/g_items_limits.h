#pragma once

#include <cmath>

/*
=============
G_GetHoldableMax

Returns the effective max for a holdable, preferring cvar overrides when set.
=============
*/
constexpr int G_GetHoldableMax(int override_max, int item_max, int fallback)
{
	int max_value = override_max > 0 ? override_max : item_max;
	return max_value > 0 ? max_value : fallback;
}

/*
=============
G_GetTechRegenMax

Returns the regeneration cap applied when the Autodoc tech is active.
=============
*/
inline int G_GetTechRegenMax(int vampiric_health_max, bool vampiric_damage_enabled, bool mod_enabled)
{
	if (vampiric_damage_enabled)
		return (int)ceil(static_cast<float>(vampiric_health_max) / 2.0f);

	return mod_enabled ? 100 : 150;
}

static_assert(G_GetHoldableMax(3, 1, 1) == 3, "Override max should win when positive.");
static_assert(G_GetHoldableMax(0, 2, 1) == 2, "Item max should win when override is unset.");
static_assert(G_GetHoldableMax(0, 0, 1) == 1, "Fallback should apply when no max is defined.");
