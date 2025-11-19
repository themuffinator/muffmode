#include "g_items_limits.h"
#include <cassert>

/*
=============
main

Ensures doppelganger max selection honors override and item defaults.
=============
*/
int main()
{
	assert(G_GetHoldableMax(2, 1, 1) == 2);
	assert(G_GetHoldableMax(0, 3, 1) == 3);
	assert(G_GetHoldableMax(0, 0, 1) == 1);
	return 0;
}
