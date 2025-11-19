#ifndef G_BROADCAST_UTILS_H
#define G_BROADCAST_UTILS_H

/*
=============
ResolveFriendlyMessage

Returns a safe message pointer for broadcasting, skipping null or empty inputs.
=============
*/
inline const char *ResolveFriendlyMessage(const char *msg) {
	if (!msg || !*msg)
		return nullptr;

	return msg;
}

#endif // G_BROADCAST_UTILS_H
