#include <cassert>
#include "g_broadcast_utils.h"

/*
=============
main
=============
*/
int main() {
	assert(ResolveFriendlyMessage(nullptr) == nullptr);
	assert(ResolveFriendlyMessage("") == nullptr);
	const char *message = "hello";
	assert(ResolveFriendlyMessage(message) == message);
	return 0;
}
