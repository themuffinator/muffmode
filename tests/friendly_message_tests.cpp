#include "../src/g_utils_friendly_message.h"
#include <cassert>

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

	return 0;
}
