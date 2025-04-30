// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"
#include "../monsters/m_player.h"
#include "bot_utils.h"

constexpr int Team_Coop_Monster = 0;

/*
================
Player_UpdateState
================
*/
static void Player_UpdateState(gentity_t *player) {
	const client_persistant_t &persistant = player->client->pers;

	player->sv.ent_flags = SVFL_NONE;
	if (player->groundEntity != nullptr || (player->flags & FL_PARTIALGROUND) != 0) {
		player->sv.ent_flags |= SVFL_ONGROUND;
	} else {
		if (player->client->ps.pmove.pm_flags & PMF_JUMP_HELD) {
			player->sv.ent_flags |= SVFL_IS_JUMPING;
		}
	}

	if (player->client->ps.pmove.pm_flags & PMF_ON_LADDER) {
		player->sv.ent_flags |= SVFL_ON_LADDER;
	}

	if ((player->client->ps.pmove.pm_flags & PMF_DUCKED) != 0) {
		player->sv.ent_flags |= SVFL_IS_CROUCHING;
	}

	if (player->client->pu_time_quad > level.time) {
		player->sv.ent_flags |= SVFL_HAS_DMG_BOOST;
	} else if (player->client->pu_time_haste > level.time) {
		player->sv.ent_flags |= SVFL_HAS_DMG_BOOST;
	} else if (player->client->pu_time_double > level.time) {
		player->sv.ent_flags |= SVFL_HAS_DMG_BOOST;
	}

	if (player->client->pu_time_battlesuit > level.time) {
		player->sv.ent_flags |= SVFL_HAS_PROTECTION;
	}

	if (player->client->pu_time_invisibility > level.time) {
		player->sv.ent_flags |= SVFL_HAS_INVISIBILITY;
	}

	if ((player->client->ps.pmove.pm_flags & PMF_TIME_KNOCKBACK) != 0) {
		player->sv.ent_flags |= SVFL_HAS_TELEPORTED;
	}

	if (player->takeDamage) {
		player->sv.ent_flags |= SVFL_TAKES_DAMAGE;
	}

	if (player->solid == SOLID_NOT) {
		player->sv.ent_flags |= SVFL_IS_HIDDEN;
	}

	if ((player->flags & FL_INWATER) != 0) {
		if (player->waterlevel >= WATER_WAIST) {
			player->sv.ent_flags |= SVFL_IN_WATER;
		}
	}

	if ((player->flags & FL_NOTARGET) != 0) {
		player->sv.ent_flags |= SVFL_NO_TARGET;
	}

	if ((player->flags & FL_GODMODE) != 0) {
		player->sv.ent_flags |= SVFL_GOD_MODE;
	}

	if (player->moveType == MOVETYPE_NOCLIP) {
		player->sv.ent_flags |= SVFL_IS_NOCLIP;
	}

	if (player->client->animEnd == FRAME_flip12) {
		player->sv.ent_flags |= SVFL_IS_FLIPPING_OFF;
	}

	if (player->client->animEnd == FRAME_salute11) {
		player->sv.ent_flags |= SVFL_IS_SALUTING;
	}

	if (player->client->animEnd == FRAME_taunt17) {
		player->sv.ent_flags |= SVFL_IS_TAUNTING;
	}

	if (player->client->animEnd == FRAME_wave11) {
		player->sv.ent_flags |= SVFL_IS_WAVING;
	}

	if (player->client->animEnd == FRAME_point12) {
		player->sv.ent_flags |= SVFL_IS_POINTING;
	}

	if ((player->client->ps.pmove.pm_flags & PMF_DUCKED) == 0 && player->client->animPriority <= ANIM_WAVE) {
		player->sv.ent_flags |= SVFL_CAN_GESTURE;
	}

	if (player->lastMOD.id == MOD_TELEFRAG || player->lastMOD.id == MOD_TELEFRAG_SPAWN) {
		player->sv.ent_flags |= SVFL_WAS_TELEFRAGGED;
	}

	if (!ClientIsPlaying(player->client) || player->client->eliminated) {
		player->sv.ent_flags |= SVFL_IS_SPECTATOR;
	}

	player_skinnum_t pl_skinnum;
	pl_skinnum.skinnum = player->s.skinnum;
	player->sv.team = pl_skinnum.team_index;

	player->sv.buttons = player->client->buttons;

	const item_id_t armorType = ArmorIndex(player);
	player->sv.armor_type = armorType;
	player->sv.armor_value = persistant.inventory[armorType];

	player->sv.health = (player->deadFlag != true) ? player->health : -1;
	player->sv.weapon = (persistant.weapon != nullptr) ? persistant.weapon->id : IT_NULL;

	player->sv.last_attackertime = static_cast<int32_t>(player->client->last_attacker_time.milliseconds());
	player->sv.respawntime = static_cast<int32_t>(player->client->respawn_time.milliseconds());
	player->sv.waterlevel = player->waterlevel;
	player->sv.viewHeight = player->viewHeight;

	player->sv.viewangles = player->client->vAngle;
	player->sv.viewforward = player->client->vForward;
	player->sv.velocity = player->velocity;

	player->sv.ground_entity = player->groundEntity;
	player->sv.enemy = player->enemy;

	static_assert(sizeof(persistant.inventory) <= sizeof(player->sv.inventory));
	memcpy(&player->sv.inventory, &persistant.inventory, sizeof(persistant.inventory));

	if (!player->sv.init) {
		player->sv.init = true;
		player->sv.className = player->className;
		player->sv.targetname = player->targetname;
		player->sv.lobby_usernum = P_GetLobbyUserNum(player);
		player->sv.starting_health = player->health;
		player->sv.max_health = player->max_health;

		// NOTE: entries are assumed to be ranked with the first armor assumed
		// NOTE: to be the "best", and last the "worst". You don't need to add
		// NOTE: entries for things like armor shards, only actual armors.
		// NOTE: Check "Max_Armor_Types" to raise/lower the armor count.
		armorInfo_t *armorInfo = player->sv.armor_info;
		armorInfo[0].item_id = IT_ARMOR_BODY;
		armorInfo[0].max_count = armor_stats[game.ruleset][ARMOR_BODY].max_count;
		armorInfo[1].item_id = IT_ARMOR_COMBAT;
		armorInfo[1].max_count = armor_stats[game.ruleset][ARMOR_COMBAT].max_count;
		armorInfo[2].item_id = IT_ARMOR_JACKET;
		armorInfo[2].max_count = armor_stats[game.ruleset][ARMOR_JACKET].max_count;

		gi.Info_ValueForKey(player->client->pers.userInfo, "name", player->sv.netname, sizeof(player->sv.netname));

		gi.Bot_RegisterEntity(player);
	}
}

/*
================
Monster_UpdateState
================
*/
static void Monster_UpdateState(gentity_t *monster) {
	monster->sv.ent_flags = SVFL_NONE;
	if (monster->groundEntity != nullptr) {
		monster->sv.ent_flags |= SVFL_ONGROUND;
	}

	if (monster->takeDamage) {
		monster->sv.ent_flags |= SVFL_TAKES_DAMAGE;
	}

	if (monster->solid == SOLID_NOT || monster->moveType == MOVETYPE_NONE) {
		monster->sv.ent_flags |= SVFL_IS_HIDDEN;
	}

	if ((monster->flags & FL_INWATER) != 0) {
		monster->sv.ent_flags |= SVFL_IN_WATER;
	}

	if (CooperativeModeOn()) {
		monster->sv.team = Team_Coop_Monster;
	} else {
		monster->sv.team = Team_None; // TODO: CTF/TDM/etc...
	}

	monster->sv.health = (monster->deadFlag != true) ? monster->health : -1;
	monster->sv.waterlevel = monster->waterlevel;
	monster->sv.enemy = monster->enemy;
	monster->sv.ground_entity = monster->groundEntity;

	int32_t viewHeight = monster->viewHeight;
	if ((monster->monsterInfo.aiflags & AI_DUCKED) != 0) {
		viewHeight = int32_t(monster->maxs[2] - 4.0f);
	}
	monster->sv.viewHeight = viewHeight;

	monster->sv.viewangles = monster->s.angles;

	AngleVectors(monster->s.angles, monster->sv.viewforward, nullptr, nullptr);

	monster->sv.velocity = monster->velocity;

	if (!monster->sv.init) {
		monster->sv.init = true;
		monster->sv.className = monster->className;
		monster->sv.targetname = monster->targetname;
		monster->sv.starting_health = monster->health;
		monster->sv.max_health = monster->max_health;

		gi.Bot_RegisterEntity(monster);
	}
}

/*
================
Item_UpdateState
================
*/
static void Item_UpdateState(gentity_t *item) {
	item->sv.ent_flags = SVFL_IS_ITEM;
	item->sv.respawntime = 0;

	if (item->team != nullptr) {
		item->sv.ent_flags |= SVFL_IN_TEAM;
	} // some DM maps have items chained together in teams...

	if (item->solid == SOLID_NOT) {
		item->sv.ent_flags |= SVFL_IS_HIDDEN;

		if (item->nextThink.milliseconds() > 0) {
			if ((item->svFlags & SVF_RESPAWNING) != 0) {
				const gtime_t pendingRespawnTime = (item->nextThink - level.time);
				item->sv.respawntime = static_cast<int32_t>(pendingRespawnTime.milliseconds());
			} else {
				// item will respawn at some unknown time in the future...
				item->sv.respawntime = Item_UnknownRespawnTime;
			}
		}
	}

	const item_id_t itemID = item->item->id;
	if (itemID == IT_FLAG_RED || itemID == IT_FLAG_BLUE) {
		item->sv.ent_flags |= SVFL_IS_OBJECTIVE;
		// TODO: figure out if the objective is dropped/carried/home...
	}

	// always need to update these for items, since random item spawning
	// could change them at any time...
	item->sv.className = item->className;
	item->sv.item_id = item->item->id;

	if (!item->sv.init) {
		item->sv.init = true;
		item->sv.targetname = item->targetname;

		gi.Bot_RegisterEntity(item);
	}
}

/*
================
Trap_UpdateState
================
*/
static void Trap_UpdateState(gentity_t *danger) {
	danger->sv.ent_flags = SVFL_TRAP_DANGER;
	danger->sv.velocity = danger->velocity;

	if (danger->owner != nullptr && danger->owner->client != nullptr) {
		player_skinnum_t pl_skinnum;
		pl_skinnum.skinnum = danger->owner->s.skinnum;
		danger->sv.team = pl_skinnum.team_index;
	}

	if (danger->groundEntity != nullptr) {
		danger->sv.ent_flags |= SVFL_ONGROUND;
	}

	if ((danger->flags & FL_TRAP_LASER_FIELD) == 0) {
		danger->sv.ent_flags |= SVFL_ACTIVE; // non-lasers are always active
	} else {
		danger->sv.start_origin = danger->s.origin;
		danger->sv.end_origin = danger->s.old_origin;
		if ((danger->svFlags & SVF_NOCLIENT) == 0) {
			if ((danger->s.renderfx & RF_BEAM)) {
				danger->sv.ent_flags |= SVFL_ACTIVE; // lasers are active!!
			}
		}
	}

	if (!danger->sv.init) {
		danger->sv.init = true;
		danger->sv.className = danger->className;

		gi.Bot_RegisterEntity(danger);
	}
}

/*
================
Mover_UpdateState
================
*/
static void Mover_UpdateState(gentity_t *entity) {
	entity->sv.ent_flags = SVFL_NONE;
	entity->sv.health = entity->health;

	if (entity->takeDamage) {
		entity->sv.ent_flags |= SVFL_TAKES_DAMAGE;
	}

	// plats, movers, and doors use this to determine move state.
	const bool isDoor = ((entity->svFlags & SVF_DOOR) != 0);
	const bool isReversedDoor = (isDoor && entity->spawnflags.has(SPAWNFLAG_DOOR_REVERSE));

	// doors have their top/bottom states reversed from plats
	// ( unless "reverse" spawnflag is set! )
	if (isDoor && !isReversedDoor) {
		if (entity->moveinfo.state == STATE_TOP) {
			entity->sv.ent_flags |= SVFL_MOVESTATE_BOTTOM;
		} else if (entity->moveinfo.state == STATE_BOTTOM) {
			entity->sv.ent_flags |= SVFL_MOVESTATE_TOP;
		}
	} else {
		if (entity->moveinfo.state == STATE_TOP) {
			entity->sv.ent_flags |= SVFL_MOVESTATE_TOP;
		} else if (entity->moveinfo.state == STATE_BOTTOM) {
			entity->sv.ent_flags |= SVFL_MOVESTATE_BOTTOM;
		}
	}

	if (entity->moveinfo.state == STATE_UP || entity->moveinfo.state == STATE_DOWN) {
		entity->sv.ent_flags |= SVFL_MOVESTATE_MOVING;
	}

	entity->sv.start_origin = entity->moveinfo.start_origin;
	entity->sv.end_origin = entity->moveinfo.end_origin;

	if (entity->svFlags & SVF_DOOR) {
		if (entity->flags & FL_LOCKED) {
			entity->sv.ent_flags |= SVFL_IS_LOCKED_DOOR;
		}
	}

	if (!entity->sv.init) {
		entity->sv.init = true;
		entity->sv.className = entity->className;
		entity->sv.targetname = entity->targetname;
		entity->sv.spawnflags = entity->spawnflags.value;
	}
}

/*
================
Entity_UpdateState
================
*/
void Entity_UpdateState(gentity_t *ent) {
	if (ent->svFlags & SVF_MONSTER) {
		Monster_UpdateState(ent);
	} else if (ent->flags & FL_TRAP || ent->flags & FL_TRAP_LASER_FIELD) {
		Trap_UpdateState(ent);
	} else if (ent->item != nullptr) {
		Item_UpdateState(ent);
	} else if (ent->client != nullptr) {
		Player_UpdateState(ent);
	} else {
		Mover_UpdateState(ent);
	}
}

static USE(info_nav_lock_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	gentity_t *n = nullptr;

	while ((n = G_FindByString<&gentity_t::targetname>(n, self->target))) {
		if (!(n->svFlags & SVF_DOOR)) {
			gi.Com_PrintFmt("{} tried targeting {}, a non-SVF_DOOR\n", *self, *n);
			continue;
		}

		n->flags ^= FL_LOCKED;
	}
}

/*QUAKED info_nav_lock (1.0 1.0 0.0) (-16 -16 0) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Toggles locked state on linked entity.
*/
void SP_info_nav_lock(gentity_t *self) {
	if (!self->targetname) {
		gi.Com_PrintFmt("{} missing targetname\n", *self);
		FreeEntity(self);
		return;
	}

	if (!self->target) {
		gi.Com_PrintFmt("{} missing target\n", *self);
		FreeEntity(self);
		return;
	}

	self->svFlags |= SVF_NOCLIENT;
	self->use = info_nav_lock_use;
}

/*
================
FindLocalPlayer
================
*/
const gentity_t *FindLocalPlayer() {
	const gentity_t *localPlayer = nullptr;

	const gentity_t *ent = &g_entities[0];
	for (size_t i = 0; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse || !(ent->svFlags & SVF_PLAYER)) {
			continue;
		}

		if (ent->health <= 0) {
			continue;
		}

		localPlayer = ent;
		break;
	}

	return localPlayer;
}

/*
================
FindFirstBot
================
*/
const gentity_t *FindFirstBot() {
	const gentity_t *firstBot = nullptr;

	const gentity_t *ent = &g_entities[0];
	for (size_t i = 0; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse || !(ent->svFlags & SVF_PLAYER)) {
			continue;
		}

		if (ent->health <= 0) {
			continue;
		}

		if (!(ent->svFlags & SVF_BOT)) {
			continue;
		}

		firstBot = ent;
		break;
	}

	return firstBot;
}

/*
================
FindFirstMonster
================
*/
const gentity_t *FindFirstMonster() {
	const gentity_t *firstMonster = nullptr;

	const gentity_t *ent = &g_entities[0];
	for (size_t i = 0; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse || !(ent->svFlags & SVF_MONSTER)) {
			continue;
		}

		if (ent->health <= 0) {
			continue;
		}

		firstMonster = ent;
		break;
	}

	return firstMonster;
}

/*
================
FindFirstMonster

"Actors" are either players or monsters - i.e. something alive and thinking.
================
*/
const gentity_t *FindActorUnderCrosshair(const gentity_t *player) {
	if (player == nullptr || !player->inUse) {
		return nullptr;
	}

	vec3_t forward, right, up;
	AngleVectors(player->client->vAngle, forward, right, up);

	const vec3_t eye_position = (player->s.origin + vec3_t{ 0.0f, 0.0f, (float)player->viewHeight });
	const vec3_t end = (eye_position + (forward * 8192.0f));
	const contents_t mask = (MASK_PROJECTILE & ~CONTENTS_DEADMONSTER);

	trace_t tr = gi.traceline(eye_position, end, player, mask);

	const gentity_t *traceEnt = tr.ent;
	if (traceEnt == nullptr || !tr.ent->inUse) {
		return nullptr;
	}

	if (!(traceEnt->svFlags & SVF_PLAYER) && !(traceEnt->svFlags & SVF_MONSTER)) {
		return nullptr;
	}

	if (traceEnt->health <= 0) {
		return nullptr;
	}

	return traceEnt;
}
