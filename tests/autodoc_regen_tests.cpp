#include "../src/g_items_limits.h"
#include <cassert>

/*
=============
main

Verifies vampiric regen cap rounds up for odd maximums.
=============
*/
int main()
{
	assert(G_GetTechRegenMax(101, true, false) == 51);
	assert(G_GetTechRegenMax(100, true, false) == 50);

	return 0;
}
