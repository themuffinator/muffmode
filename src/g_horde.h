// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

extern cvar_t *g_horde;

void Horde_PreInit();
void Horde_Init();
void Horde_RunFrame();
gitem_t *G_HordePickItem();
bool G_IsDeathmatch();
bool G_IsCooperative();
