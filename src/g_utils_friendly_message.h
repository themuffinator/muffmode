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

/*
=============
FriendlyMessageShouldPrefixTeam

Determines if a friendly message should include a team prefix for the
recipient based on game mode, target team, and how the recipient is viewing
the game.
=============
*/
inline bool FriendlyMessageShouldPrefixTeam(bool teams_active, bool target_is_spectator, bool recipient_is_playing, bool recipient_on_team, bool recipient_following_team)
{
	if (!teams_active || target_is_spectator)
		return false;

	if (recipient_is_playing)
		return recipient_on_team;

	return recipient_following_team;
}
