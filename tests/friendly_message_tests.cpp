#include "../src/g_utils_friendly_message.h"
#include <cassert>
#include <algorithm>

/*
=============
main

Regression coverage for friendly message validation.
=============
*/
int main()
	{
	assert(!FriendlyMessageHasText(nullptr));
	assert(!FriendlyMessageHasText(""));
	assert(FriendlyMessageHasText("hello"));

	char unterminated[256];
	std::fill_n(unterminated, 256, 'a');
	assert(!FriendlyMessageHasText(unterminated));

	char bounded[256];
	std::fill_n(bounded, 255, 'b');
	bounded[255] = '\0';
	assert(FriendlyMessageHasText(bounded));

	assert(FriendlyMessageShouldPrefixTeam(true, false, false, false, true));
	assert(!FriendlyMessageShouldPrefixTeam(true, false, false, false, false));
	assert(FriendlyMessageShouldPrefixTeam(true, false, true, true, false));

	return 0;
}
