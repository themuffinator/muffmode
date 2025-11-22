#include "../src/q_std.h"
#include <cassert>
#include <cstring>

/*
=============
main

Verify Q_strlcpy copies menu-sized buffers without truncation when the destination size is provided.
=============
*/
int main()
{
	char destination[256] = {};
	const char short_source[] = "Short label";
	const size_t copied = Q_strlcpy(destination, short_source, sizeof(destination));

	assert(copied == strlen(short_source));
	assert(strcmp(destination, short_source) == 0);

	char long_source[300];
	memset(long_source, 'a', sizeof(long_source));
	long_source[sizeof(long_source) - 1] = '\0';

	char truncated[16] = {};
	const size_t truncated_length = Q_strlcpy(truncated, long_source, sizeof(truncated));
	assert(truncated_length >= sizeof(truncated));
	assert(truncated[sizeof(truncated) - 1] == '\0');

	return 0;
}
