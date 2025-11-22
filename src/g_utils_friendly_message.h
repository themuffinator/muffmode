/*
=============
FriendlyMessageHasText

Determines if the provided message pointer is non-null, non-empty, and
contains a terminator within a limited scan.
=============
*/
#pragma once
#include <cstring>

inline bool FriendlyMessageHasText(const char *msg)
{
	return msg && *msg && strnlen(msg, 256) < 256;
}
