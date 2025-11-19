/*
=============
FriendlyMessageHasText

Determines if the provided message pointer is non-null and non-empty.
=============
*/
#pragma once

inline bool FriendlyMessageHasText(const char *msg)
{
	return msg && *msg;
}
