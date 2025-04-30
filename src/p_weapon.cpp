// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_weapon.c

#include "g_local.h"
#include "monsters/m_player.h"

bool is_quad;
bool is_haste;
player_muzzle_t is_silenced;
byte damage_multiplier;

/*
================
InfiniteAmmoOn
================
*/
bool InfiniteAmmoOn(gitem_t *item) {
	if (item && item->flags & IF_NO_INFINITE_AMMO)
		return false;

	return g_infinite_ammo->integer || (deathmatch->integer && (g_instagib->integer || g_nadefest->integer || g_gametype->integer == GT_RACE));
}

/*
================
Stats_AddShot
================
*/
static void Stats_AddShot(gentity_t *ent) {
}

/*
================
PlayerDamageModifier
================
*/
byte PlayerDamageModifier(gentity_t *ent) {
	is_quad = false;
	damage_multiplier = 0;

	//WOR: make these stack but additive rather than multiplicative

	if (ent->client->pu_time_quad > level.time) {
		damage_multiplier += 4;
		is_quad = true;
	}

	if (ent->client->pu_time_double > level.time) {
		damage_multiplier += 2;
		is_quad = true;
	}

	if (ent->client->pers.inventory[IT_TECH_POWER_AMP])
		damage_multiplier += 2;

	damage_multiplier = damage_multiplier ? damage_multiplier : 1;

	return damage_multiplier;
}

/*
================
P_CurrentKickFactor

[Paril-KEX] kicks in vanilla take place over 2 10hz server
frames; this is to mimic that visual behavior on any tickrate.
================
*/

static inline float P_CurrentKickFactor(gentity_t *ent) {
	if (ent->client->kick.time < level.time)
		return 0.f;

	float f = (ent->client->kick.time - level.time).seconds() / ent->client->kick.total.seconds();
	return f;
}

/*
================
P_CurrentKickAngles
================
*/
vec3_t P_CurrentKickAngles(gentity_t *ent) {
	return ent->client->kick.angles * P_CurrentKickFactor(ent);
}

/*
================
P_CurrentKickOrigin
================
*/
vec3_t P_CurrentKickOrigin(gentity_t *ent) {
	return ent->client->kick.origin * P_CurrentKickFactor(ent);
}

/*
================
P_AddWeaponKick
================
*/
void P_AddWeaponKick(gentity_t *ent, const vec3_t &origin, const vec3_t &angles) {
	ent->client->kick.origin = origin;
	ent->client->kick.angles = angles;
	ent->client->kick.total = 200_ms;
	ent->client->kick.time = level.time + ent->client->kick.total;
}

/*
================
P_ProjectSource
================
*/
void P_ProjectSource(gentity_t *ent, const vec3_t &angles, vec3_t distance, vec3_t &result_start, vec3_t &result_dir) {
	if (g_weapon_projection->integer) {
		distance[1] = 0;
		if (g_weapon_projection->integer > 1)
			distance[2] = 0;
	} else if (ent->client->pers.hand == LEFT_HANDED)
		distance[1] *= -1;
	else if (ent->client->pers.hand == CENTER_HANDED)
		distance[1] = 0;

	vec3_t forward, right, up;
	vec3_t eye_position = (ent->s.origin + vec3_t{ 0, 0, (float)ent->viewHeight });

	AngleVectors(angles, forward, right, up);

	result_start = G_ProjectSource2(eye_position, distance, forward, right, up);

	vec3_t	   end = eye_position + forward * 8192;
	contents_t mask = MASK_PROJECTILE & ~CONTENTS_DEADMONSTER;

	// [Paril-KEX]
	if (!G_ShouldPlayersCollide(true))
		mask &= ~CONTENTS_PLAYER;

	trace_t tr = gi.traceline(eye_position, end, ent, mask);

	// if the point was a monster & close to us, use raw forward
	// so railgun pierces properly
	if (tr.startsolid || ((tr.contents & (CONTENTS_MONSTER | CONTENTS_PLAYER)) && (tr.fraction * 8192.f) < 128.f))
		result_dir = forward;
	else {
		end = tr.endpos;
		result_dir = (end - result_start).normalized();
	}
}

/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(gentity_t *who, const vec3_t &where, player_noise_t type) {
	gentity_t *noise;

	if (type == PNOISE_WEAPON) {
		if (who->client->silencer_shots)
			who->client->invisibility_fade_time = level.time + (INVISIBILITY_TIME / 5);
		else
			who->client->invisibility_fade_time = level.time + INVISIBILITY_TIME;

		if (who->client->silencer_shots) {
			who->client->silencer_shots--;
			return;
		}

		if (who->client->pu_time_spawn_protection > level.time)
			who->client->pu_time_spawn_protection = 0_ms;
	}

	if (deathmatch->integer)
		return;

	if (who->flags & FL_NOTARGET)
		return;

	if (type == PNOISE_SELF &&
		(who->client->landmark_free_fall || who->client->landmark_noise_time >= level.time))
		return;

	if (who->flags & FL_DISGUISED) {
		if (type == PNOISE_WEAPON) {
			level.disguiseViolator = who;
			level.disguiseViolationTime = level.time + 500_ms;
		} else
			return;
	}

	if (!who->mynoise) {
		noise = Spawn();
		noise->className = "player_noise";
		noise->mins = { -8, -8, -8 };
		noise->maxs = { 8, 8, 8 };
		noise->owner = who;
		noise->svFlags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = Spawn();
		noise->className = "player_noise";
		noise->mins = { -8, -8, -8 };
		noise->maxs = { 8, 8, 8 };
		noise->owner = who;
		noise->svFlags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON) {
		noise = who->mynoise;
		who->client->sound_entity = noise;
		who->client->sound_entity_time = level.time;
	} else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		who->client->sound2_entity = noise;
		who->client->sound2_entity_time = level.time;
	}

	noise->s.origin = where;
	noise->absMin = where - noise->maxs;
	noise->absMax = where + noise->maxs;
	noise->teleport_time = level.time;
	gi.linkentity(noise);
}

/*
================
G_WeaponShouldStay
================
*/
static inline bool G_WeaponShouldStay() {
	if (deathmatch->integer)
		return g_dm_weapons_stay->integer;
	else if (coop->integer)
		return !P_UseCoopInstancedItems();

	return false;
}

/*
================
Pickup_Weapon
================
*/
void G_CheckAutoSwitch(gentity_t *ent, gitem_t *item, bool is_new);
bool Pickup_Weapon(gentity_t *ent, gentity_t *other) {
	item_id_t index = ent->item->id;

	if (G_WeaponShouldStay() && other->client->pers.inventory[index]) {
		if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
			return false; // leave the weapon for others to pickup
	}

	gitem_t	*ammo;
	bool	is_new = !other->client->pers.inventory[index];

	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) || ent->count) {
		// give them some ammo with it if appropriate
		if (ent->item->ammo) {
			ammo = GetItemByIndex(ent->item->ammo);
			if (InfiniteAmmoOn(ammo))
				Add_Ammo(other, ammo, AMMO_INFINITE);
			else {
				int count = 0;

				if (RS(RS_Q3A)) {
					if (ent->count)
						count = ent->count;
					else {
						if (ammo->id == IT_AMMO_GRENADES ||
							ammo->id == IT_AMMO_ROCKETS ||
							ammo->id == IT_AMMO_SLUGS)
							count = 10;
						else
							count = ammo->quantity;
					}

					if (other->client->pers.inventory[ammo->id] < count)
						count = count - other->client->pers.inventory[ammo->id];
					else
						count = 1;

				} else {
					if (ammo && InfiniteAmmoOn(ent->item))
						count = AMMO_INFINITE;
					else if (ent->count)
						count = ent->count;
					else {
						if (ammo->id == IT_AMMO_SLUGS) {
							switch (game.ruleset) {
							case RS_Q1:
								count = 1;
								break;
							case RS_Q3A:

								if (other->client->pers.inventory[ammo->id] < count)
									count = count - other->client->pers.inventory[ammo->id];
								else
									count = 1;

								count = 10;
								break;
							default:
								count = 8;
								break;
							}
						} else {
							count = ammo->quantity;
						}
					}
				}
				
				Add_Ammo(other, ammo, count);
			}
		}

		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER)) {
			if (deathmatch->integer) {
				if (g_dm_weapons_stay->integer)
					ent->flags |= FL_RESPAWN;

				SetRespawn(ent, gtime_t::from_sec(g_weapon_respawn_time->integer), !g_dm_weapons_stay->integer);
			}
			if (coop->integer)
				ent->flags |= FL_RESPAWN;
		}
	}

	other->client->pers.inventory[index]++;

	G_CheckAutoSwitch(other, ent->item, is_new);

	return true;
}

/*
================
Weapon_RunThink
================
*/
static void Weapon_RunThink(gentity_t *ent) {
	// call active weapon think routine
	if (!ent->client->pers.weapon->weaponthink)
		return;

	PlayerDamageModifier(ent);

	is_haste = (ent->client->pu_time_haste > level.time);

	if (ent->client->silencer_shots)
		is_silenced = MZ_SILENCED;
	else
		is_silenced = MZ_NONE;
	ent->client->pers.weapon->weaponthink(ent);
}

/*
===============
Change_Weapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void Change_Weapon(gentity_t *ent) {
	// [Paril-KEX]
	if (ent->health > 0 && !g_instant_weapon_switch->integer && !g_frenzy->integer && ((ent->client->latchedButtons | ent->client->buttons) & BUTTON_HOLSTER))
		return;

	if (ent->client->grenade_time) {
		// force a weapon think to drop the held grenade
		ent->client->weapon_sound = 0;
		Weapon_RunThink(ent);
		ent->client->grenade_time = 0_ms;
	}

	if (ent->client->pers.weapon) {
		ent->client->pers.lastweapon = ent->client->pers.weapon;

		if (ent->client->newWeapon && ent->client->newWeapon != ent->client->pers.weapon) {
			//muff: only make the sound if we can switch faster
			if (g_quick_weapon_switch->integer || g_instant_weapon_switch->integer)
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/change.wav"), 1, ATTN_NORM, 0);
		}
	}

	ent->client->pers.weapon = ent->client->newWeapon;
	ent->client->newWeapon = nullptr;

	// set visible model
	if (ent->s.modelindex == MODELINDEX_PLAYER)
		P_AssignClientSkinnum(ent);

	if (!ent->client->pers.weapon) { // dead
		ent->client->ps.gunindex = 0;
		ent->client->ps.gunskin = 0;
		return;
	}

	ent->client->weaponState = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
	ent->client->ps.gunskin = 0;
	ent->client->weapon_sound = 0;

	ent->client->animPriority = ANIM_PAIN;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crpain1;
		ent->client->animEnd = FRAME_crpain4;
	} else {
		ent->s.frame = FRAME_pain301;
		ent->client->animEnd = FRAME_pain304;
	}
	ent->client->animTime = 0_ms;

	// for instantweap, run think immediately
	// to set up correct start frame
	if (g_instant_weapon_switch->integer || g_frenzy->integer)
		Weapon_RunThink(ent);
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(gentity_t *ent, bool sound) {
	if (sound) {
		if (level.time >= ent->client->empty_click_sound) {
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->client->empty_click_sound = level.time + 1_sec;
		}
	}

	constexpr item_id_t no_ammo_order[] = {
		IT_WEAPON_DISRUPTOR,
		IT_WEAPON_BFG,
		IT_WEAPON_RAILGUN,
		IT_WEAPON_PLASMABEAM,
		IT_WEAPON_IONRIPPER,
		IT_WEAPON_HYPERBLASTER,
		IT_WEAPON_ETF_RIFLE,
		IT_WEAPON_CHAINGUN,
		IT_WEAPON_MACHINEGUN,
		IT_WEAPON_SSHOTGUN,
		IT_WEAPON_SHOTGUN,
		IT_WEAPON_PHALANX,
		IT_WEAPON_RLAUNCHER,
		IT_WEAPON_GLAUNCHER,
		IT_WEAPON_PROXLAUNCHER,
		IT_AMMO_GRENADES,
		IT_WEAPON_BLASTER,
		IT_WEAPON_CHAINFIST
	};

	for (size_t i = 0; i < q_countof(no_ammo_order); i++) {
		gitem_t *item = GetItemByIndex(no_ammo_order[i]);

		if (!item)
			gi.Com_ErrorFmt("Invalid no ammo weapon switch weapon {}\n", (int32_t)no_ammo_order[i]);

		if (!ent->client->pers.inventory[item->id])
			continue;

		if (item->ammo && ent->client->pers.inventory[item->ammo] < item->quantity)
			continue;

		ent->client->newWeapon = item;
		return;
	}
}

/*
================
RemoveAmmo
================
*/
static void RemoveAmmo(gentity_t *ent, int32_t quantity) {
	if (InfiniteAmmoOn(ent->client->pers.weapon))
		return;

	bool pre_warning = ent->client->pers.inventory[ent->client->pers.weapon->ammo] <=
		ent->client->pers.weapon->quantity_warn;

	ent->client->pers.inventory[ent->client->pers.weapon->ammo] -= quantity;

	bool post_warning = ent->client->pers.inventory[ent->client->pers.weapon->ammo] <=
		ent->client->pers.weapon->quantity_warn;

	if (!pre_warning && post_warning)
		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("weapons/lowammo.wav"), 1, ATTN_NORM, 0);

	CheckPowerArmorState(ent);
}

/*
================
Weapon_AnimationTime

[Paril-KEX] get time per animation frame
================
*/
static inline gtime_t Weapon_AnimationTime(gentity_t *ent) {
	if ((g_quick_weapon_switch->integer || g_frenzy->integer) && (gi.tick_rate >= 20) &&
		(ent->client->weaponState == WEAPON_ACTIVATING || ent->client->weaponState == WEAPON_DROPPING))
		ent->client->ps.gunrate = 20;
	else
		ent->client->ps.gunrate = 10;

	if (ent->client->ps.gunframe != 0 && (!(ent->client->pers.weapon->flags & IF_NO_HASTE) || ent->client->weaponState != WEAPON_FIRING)) {
		if (is_haste)
			ent->client->ps.gunrate *= 1.5;
		if (Tech_ApplyTimeAccel(ent))
			ent->client->ps.gunrate *= 2;
		if (g_frenzy->integer)
			ent->client->ps.gunrate *= 2;
	}

	// network optimization...
	if (ent->client->ps.gunrate == 10) {
		ent->client->ps.gunrate = 0;
		return 100_ms;
	}

	return gtime_t::from_ms((1.f / ent->client->ps.gunrate) * 1000);
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(gentity_t *ent) {
	if (!ClientIsPlaying(ent->client) || ent->client->eliminated)
		return;

	// if just died, put the weapon away
	if (ent->health < 1) {
		ent->client->newWeapon = nullptr;
		Change_Weapon(ent);
	}

	if (!ent->client->pers.weapon) {
		if (ent->client->newWeapon)
			Change_Weapon(ent);
		return;
	}

	// call active weapon think routine
	Weapon_RunThink(ent);

	// check remainder from time accel; on 100ms/50ms server frames we may have
	// 'run next frame in' times that we can't possibly catch up to,
	// so we have to run them now.
	if (33_ms < FRAME_TIME_MS) {
		gtime_t relative_time = Weapon_AnimationTime(ent);

		if (relative_time < FRAME_TIME_MS) {
			// check how many we can't run before the next server tick
			gtime_t next_frame = level.time + FRAME_TIME_S;
			int64_t remaining_ms = (next_frame - ent->client->weaponThinkTime).milliseconds();

			while (remaining_ms > 0) {
				ent->client->weaponThinkTime -= relative_time;
				ent->client->weaponFireFinished -= relative_time;
				Weapon_RunThink(ent);
				remaining_ms -= relative_time.milliseconds();
			}
		}
	}
}

/*
================
Weapon_AttemptSwitch
================
*/
enum weap_switch_t {
	WEAP_SWITCH_ALREADY_USING,
	WEAP_SWITCH_NO_WEAPON,
	WEAP_SWITCH_NO_AMMO,
	WEAP_SWITCH_NOT_ENOUGH_AMMO,
	WEAP_SWITCH_VALID
};

static weap_switch_t Weapon_AttemptSwitch(gentity_t *ent, gitem_t *item, bool silent) {
	if (!item)
		return WEAP_SWITCH_NO_WEAPON;
	if (ent->client->pers.weapon == item)
		return WEAP_SWITCH_ALREADY_USING;
	else if (!ent->client->pers.inventory[item->id])
		return WEAP_SWITCH_NO_WEAPON;

	if (item->ammo && !g_select_empty->integer && !(item->flags & IF_AMMO)) {
		gitem_t *ammo_item = GetItemByIndex(item->ammo);

		if (!ent->client->pers.inventory[item->ammo]) {
			if (!silent)
				gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_ammo", ammo_item->pickup_name, item->pickup_name_definite);
			return WEAP_SWITCH_NO_AMMO;
		} else if (ent->client->pers.inventory[item->ammo] < item->quantity) {
			if (!silent)
				gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_enough_ammo", ammo_item->pickup_name, item->pickup_name_definite);
			return WEAP_SWITCH_NOT_ENOUGH_AMMO;
		}
	}

	return WEAP_SWITCH_VALID;
}

static inline bool Weapon_IsPartOfChain(gitem_t *item, gitem_t *other) {
	return other && other->chain && item->chain && other->chain == item->chain;
}

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(gentity_t *ent, gitem_t *item) {
	gitem_t			*wanted, *root;
	weap_switch_t	result = WEAP_SWITCH_NO_WEAPON;

	// if we're switching to a weapon in this chain already,
	// start from the weapon after this one in the chain
	if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->newWeapon)) {
		root = ent->client->newWeapon;
		wanted = root->chain_next;
	}
	// if we're already holding a weapon in this chain,
	// start from the weapon after that one
	else if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->pers.weapon)) {
		root = ent->client->pers.weapon;
		wanted = root->chain_next;
	}
	// start from beginning of chain (if any)
	else
		wanted = root = item;

	while (true) {
		// try the weapon currently in the chain
		if ((result = Weapon_AttemptSwitch(ent, wanted, false)) == WEAP_SWITCH_VALID)
			break;

		// no chains
		if (!wanted || !wanted->chain_next || ent->client->no_weapon_chains)
			break;

		wanted = wanted->chain_next;

		// we wrapped back to the root item
		if (wanted == root)
			break;
	}

	if (result == WEAP_SWITCH_VALID)
		ent->client->newWeapon = wanted; // change to this weapon when down
	else if ((result = Weapon_AttemptSwitch(ent, wanted, true)) == WEAP_SWITCH_NO_WEAPON && wanted != ent->client->pers.weapon && wanted != ent->client->newWeapon)
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", wanted->pickup_name);
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(gentity_t *ent, gitem_t *item) {
	// [Paril-KEX]
	if (deathmatch->integer && g_dm_weapons_stay->integer)
		return;

	item_id_t index = item->id;

	if (ent->client->pers.inventory[index] < 1)
		return;

	gentity_t *drop = Drop_Item(ent, item);
	drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
	drop->svFlags &= ~SVF_INSTANCED;

	if (drop->item->id != IT_WEAPON_BLASTER && drop->item->id != IT_WEAPON_GRAPPLE && drop->item->id != IT_WEAPON_CHAINFIST) {
		gitem_t *ammo = GetItemByIndex(drop->item->ammo);
		if (!ammo) {
			FreeEntity(drop);
			return;
		}
		if (ent->client->pers.inventory[ammo->id] <= 0) {
			FreeEntity(drop);
			return;
		}

		drop->count = ammo->quantity;

		if (item->id == IT_WEAPON_RAILGUN) {
			drop->count += 5;
		} else if (item->id == IT_WEAPON_BLASTER) {
			drop->count = AMMO_INFINITE;
		}
		drop->count = clamp(drop->count, drop->count, ent->client->pers.inventory[ammo->id]);

		if (drop->count <= 0) {
			FreeEntity(drop);
			return;
		}

		if (ent->client->pers.inventory[ammo->id] - drop->count < 0) {
			FreeEntity(drop);
			return;
		}

		if (drop->count != AMMO_INFINITE)
			Add_Ammo(ent, ammo, -(drop->count));

		// see if we were already using it
		if ((item == ent->client->pers.weapon) || (item == ent->client->newWeapon)) {
			if (ent->client->pers.inventory[index] < 1 || ent->client->pers.inventory[ammo->id] < 1)
				NoAmmoWeaponChange(ent, true);
		}
	} else {
		drop->count = AMMO_INFINITE;

		// see if we were already using it
		if ((item == ent->client->pers.weapon) || (item == ent->client->newWeapon)) {
			if (ent->client->pers.inventory[index] == 1)
				NoAmmoWeaponChange(ent, true);
		}
	}
		//ent->client->pers.inventory[ammo->id] -= drop->count;
	ent->client->pers.inventory[index]--;
}

/*
================
Weapon_PowerupSound
================
*/
void Weapon_PowerupSound(gentity_t *ent) {
	if (!Tech_ApplyPowerAmpSound(ent)) {
		if (ent->client->pu_time_quad > level.time && ent->client->pu_time_double > level.time)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech2x.wav"), 1, ATTN_NORM, 0);
		else if (ent->client->pu_time_quad > level.time)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
		else if (ent->client->pu_time_double > level.time)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"), 1, ATTN_NORM, 0);
		else if (ent->client->pu_time_haste > level.time
				&& ent->client->tech_sound_time < level.time) {
			ent->client->tech_sound_time = level.time + 1_sec;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech3.wav"), 1, ATTN_NORM, 0);
		}
	}

	Tech_ApplyTimeAccelSound(ent);
}

/*
================
Weapon_CanAnimate
================
*/
static inline bool Weapon_CanAnimate(gentity_t *ent) {
	// VWep animations screw up corpses
	return !ent->deadFlag && ent->s.modelindex == MODELINDEX_PLAYER;
}

/*
================
Weapon_SetFinished

[Paril-KEX] called when finished to set time until
we're allowed to switch to fire again
================
*/
static inline void Weapon_SetFinished(gentity_t *ent) {
	ent->client->weaponFireFinished = level.time + Weapon_AnimationTime(ent);
}

/*
================
Weapon_HandleDropping
================
*/
static inline bool Weapon_HandleDropping(gentity_t *ent, int FRAME_DEACTIVATE_LAST) {
	if (ent->client->weaponState == WEAPON_DROPPING) {
		if (ent->client->weaponThinkTime <= level.time) {
			if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST) {
				Change_Weapon(ent);
				return true;
			} else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4) {
				ent->client->animPriority = ANIM_ATTACK | ANIM_REVERSED;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->animEnd = FRAME_crpain1;
				} else {
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->animEnd = FRAME_pain301;
				}
				ent->client->animTime = 0_ms;
			}

			ent->client->ps.gunframe++;
			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
		}
		return true;
	}

	return false;
}

/*
================
Weapon_HandleActivating
================
*/
static inline bool Weapon_HandleActivating(gentity_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_IDLE_FIRST) {
	if (ent->client->weaponState == WEAPON_ACTIVATING) {
		if (ent->client->weaponThinkTime <= level.time || g_instant_weapon_switch->integer || g_frenzy->integer) {
			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

			if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST || g_instant_weapon_switch->integer || g_frenzy->integer) {
				ent->client->weaponState = WEAPON_READY;
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				ent->client->weaponFireBuffered = false;
				if (!g_instant_weapon_switch->integer || g_frenzy->integer)
					Weapon_SetFinished(ent);
				else
					ent->client->weaponFireFinished = 0_ms;
				return true;
			}

			ent->client->ps.gunframe++;
			return true;
		}
	}

	return false;
}

/*
================
Weapon_HandleNewWeapon
================
*/
static inline bool Weapon_HandleNewWeapon(gentity_t *ent, int FRAME_DEACTIVATE_FIRST, int FRAME_DEACTIVATE_LAST) {
	bool is_holstering = false;

	if (!g_instant_weapon_switch->integer || g_frenzy->integer)
		is_holstering = ((ent->client->latchedButtons | ent->client->buttons) & BUTTON_HOLSTER);

	if ((ent->client->newWeapon || is_holstering) && (ent->client->weaponState != WEAPON_FIRING)) {
		if (g_instant_weapon_switch->integer || g_frenzy->integer || ent->client->weaponThinkTime <= level.time) {
			if (!ent->client->newWeapon)
				ent->client->newWeapon = ent->client->pers.weapon;

			ent->client->weaponState = WEAPON_DROPPING;

			if (g_instant_weapon_switch->integer || g_frenzy->integer) {
				Change_Weapon(ent);
				return true;
			}

			ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

			if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4) {
				ent->client->animPriority = ANIM_ATTACK | ANIM_REVERSED;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->animEnd = FRAME_crpain1;
				} else {
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->animEnd = FRAME_pain301;
				}
				ent->client->animTime = 0_ms;
			}

			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
		}
		return true;
	}

	return false;
}

/*
================
Weapon_HandleReady
================
*/
enum weapon_ready_state_t {
	READY_NONE,
	READY_CHANGING,
	READY_FIRING
};

static inline weapon_ready_state_t Weapon_HandleReady(gentity_t *ent, int FRAME_FIRE_FIRST, int FRAME_IDLE_FIRST, int FRAME_IDLE_LAST, const int *pause_frames) {
	if (ent->client->weaponState == WEAPON_READY) {
		bool request_firing;

		if (CombatIsDisabled()) {
			request_firing = false;
			ent->client->latchedButtons &= ~BUTTON_ATTACK;
		} else
			request_firing = ent->client->weaponFireBuffered || ((ent->client->latchedButtons | ent->client->buttons) & BUTTON_ATTACK);

		if (request_firing && ent->client->weaponFireFinished <= level.time) {
			ent->client->latchedButtons &= ~BUTTON_ATTACK;
			ent->client->weaponThinkTime = level.time;

			if ((!ent->client->pers.weapon->ammo) ||
				(ent->client->pers.inventory[ent->client->pers.weapon->ammo] >= ent->client->pers.weapon->quantity)) {
				ent->client->weaponState = WEAPON_FIRING;
				ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
				return READY_FIRING;
			} else {
				NoAmmoWeaponChange(ent, true);
				return READY_CHANGING;
			}
		} else if (ent->client->weaponThinkTime <= level.time) {
			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

			if (ent->client->ps.gunframe == FRAME_IDLE_LAST) {
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return READY_CHANGING;
			}

			if (pause_frames)
				for (int n = 0; pause_frames[n]; n++)
					if (ent->client->ps.gunframe == pause_frames[n])
						if (irandom(16))
							return READY_CHANGING;

			ent->client->ps.gunframe++;
			return READY_CHANGING;
		}
	}

	return READY_NONE;
}

/*
================
Weapon_HandleFiring
================
*/
static inline void Weapon_HandleFiring(gentity_t *ent, int32_t FRAME_IDLE_FIRST, std::function<void()> fire_handler) {
	Weapon_SetFinished(ent);

	if (ent->client->weaponFireBuffered) {
		ent->client->buttons |= BUTTON_ATTACK;
		ent->client->weaponFireBuffered = false;

		if (ent->client->pu_time_spawn_protection > level.time)
			ent->client->pu_time_spawn_protection = 0_ms;
	}

	fire_handler();

	if (ent->client->ps.gunframe == FRAME_IDLE_FIRST) {
		ent->client->weaponState = WEAPON_READY;
		ent->client->weaponFireBuffered = false;
	}

	ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
}

/*
================
Weapon_Generic
================
*/
void Weapon_Generic(gentity_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, const int *pause_frames, const int *fire_frames, void (*fire)(gentity_t *ent)) {
	int FRAME_FIRE_FIRST = (FRAME_ACTIVATE_LAST + 1);
	int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);
	int FRAME_DEACTIVATE_FIRST = (FRAME_IDLE_LAST + 1);

	if (!Weapon_CanAnimate(ent))
		return;

	if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
		return;
	else if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
		return;
	else if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
		return;
	else if (auto state = Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pause_frames)) {
		if (state == READY_FIRING) {
			ent->client->ps.gunframe = FRAME_FIRE_FIRST;
			ent->client->weaponFireBuffered = false;

			if (ent->client->weaponThunk)
				ent->client->weaponThinkTime += FRAME_TIME_S;

			ent->client->weaponThinkTime += Weapon_AnimationTime(ent);
			Weapon_SetFinished(ent);

			for (int n = 0; fire_frames[n]; n++) {
				if (ent->client->ps.gunframe == fire_frames[n]) {
					Weapon_PowerupSound(ent);
					fire(ent);
					break;
				}
			}

			// start the animation
			ent->client->animPriority = ANIM_ATTACK;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
				ent->s.frame = FRAME_crattak1 - 1;
				ent->client->animEnd = FRAME_crattak9;
			} else {
				ent->s.frame = FRAME_attack1 - 1;
				ent->client->animEnd = FRAME_attack8;
			}
			ent->client->animTime = 0_ms;
		}

		return;
	}

	if (ent->client->weaponState == WEAPON_FIRING && ent->client->weaponThinkTime <= level.time) {
		ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
		ent->client->ps.gunframe++;
		Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, [&]() {
			for (int n = 0; fire_frames[n]; n++) {
				if (ent->client->ps.gunframe == fire_frames[n]) {
					Weapon_PowerupSound(ent);
					fire(ent);
					break;
				}
			}
			});
	}
}

/*
================
Weapon_Repeating
================
*/
void Weapon_Repeating(gentity_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, const int *pause_frames, void (*fire)(gentity_t *ent)) {
	int FRAME_FIRE_FIRST = (FRAME_ACTIVATE_LAST + 1);
	int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);
	int FRAME_DEACTIVATE_FIRST = (FRAME_IDLE_LAST + 1);

	if (!Weapon_CanAnimate(ent))
		return;

	if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
		return;
	else if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
		return;
	else if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
		return;
	else if (Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pause_frames) == READY_CHANGING)
		return;

	if (ent->client->weaponState == WEAPON_FIRING && ent->client->weaponThinkTime <= level.time) {
		ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
		Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, [&]() { fire(ent); });

		if (ent->client->weaponThunk)
			ent->client->weaponThinkTime += FRAME_TIME_S;
	}
}

/*
======================================================================

HAND GRENADES

======================================================================
*/

static void Weapon_HandGrenade_Fire(gentity_t *ent, bool held) {
	int	  damage = 125;
	int	  speed;
	float radius = (float)(damage + 40);

	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't throw behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->vAngle[PITCH]), ent->client->vAngle[YAW], ent->client->vAngle[ROLL] }, { 2, 0, -14 }, start, dir);

	gtime_t timer = ent->client->grenade_time - level.time;
	speed = (int)(ent->health <= 0 ? GRENADE_MINSPEED : min(GRENADE_MINSPEED + (GRENADE_TIMER - timer).seconds() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.seconds()), GRENADE_MAXSPEED));

	ent->client->grenade_time = 0_ms;

	fire_handgrenade(ent, start, dir, damage, speed, timer, radius, held);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_HAND_GRENADES]++;
	RemoveAmmo(ent, 1);
}

void Throw_Generic(gentity_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_PRIME_SOUND,
	const char *prime_sound,
	int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, const int *pause_frames, int EXPLODE,
	const char *primed_sound,
	void (*fire)(gentity_t *ent, bool held), bool extra_idle_frame) {
	// when we die, just toss what we had in our hands.
	if (ent->health <= 0) {
		fire(ent, true);
		return;
	}

	int n;
	int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);

	if (ent->client->newWeapon && (ent->client->weaponState == WEAPON_READY)) {
		if (ent->client->weaponThinkTime <= level.time) {
			Change_Weapon(ent);
			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
		}
		return;
	}

	if (ent->client->weaponState == WEAPON_ACTIVATING) {
		if (ent->client->weaponThinkTime <= level.time) {
			ent->client->weaponState = WEAPON_READY;
			if (!extra_idle_frame)
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			else
				ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;
			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
			Weapon_SetFinished(ent);
		}
		return;
	}

	if (ent->client->weaponState == WEAPON_READY) {
		bool request_firing;

		if (CombatIsDisabled()) {
			request_firing = false;
			ent->client->latchedButtons &= ~BUTTON_ATTACK;
		} else
			request_firing = ent->client->weaponFireBuffered || ((ent->client->latchedButtons | ent->client->buttons) & BUTTON_ATTACK);

		if (request_firing && ent->client->weaponFireFinished <= level.time) {
			ent->client->latchedButtons &= ~BUTTON_ATTACK;

			if (ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
				ent->client->ps.gunframe = 1;
				ent->client->weaponState = WEAPON_FIRING;
				ent->client->grenade_time = 0_ms;
				ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
			} else
				NoAmmoWeaponChange(ent, true);
			return;
		} else if (ent->client->weaponThinkTime <= level.time) {
			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

			if (ent->client->ps.gunframe >= FRAME_IDLE_LAST) {
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames) {
				for (n = 0; pause_frames[n]; n++) {
					if (ent->client->ps.gunframe == pause_frames[n]) {
						if (irandom(16))
							return;
					}
				}
			}

			ent->client->ps.gunframe++;
		}
		return;
	}

	if (ent->client->weaponState == WEAPON_FIRING) {
		ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;

		if (ent->client->weaponThinkTime <= level.time) {
			if (prime_sound && ent->client->ps.gunframe == FRAME_PRIME_SOUND)
				gi.sound(ent, CHAN_WEAPON, gi.soundindex(prime_sound), 1, ATTN_NORM, 0);

			// [Paril-KEX] dualfire/time accel
			gtime_t grenade_wait_time = 1_sec;

			if (Tech_ApplyTimeAccel(ent))
				grenade_wait_time *= 0.5f;
			if (is_haste)
				grenade_wait_time *= 0.5f;
			if (g_frenzy->integer)
				grenade_wait_time *= 0.5f;

			if (ent->client->ps.gunframe == FRAME_THROW_HOLD) {
				if (!ent->client->grenade_time && !ent->client->grenade_finished_time)
					ent->client->grenade_time = level.time + GRENADE_TIMER + 200_ms;

				if (primed_sound && !ent->client->grenade_blew_up)
					ent->client->weapon_sound = gi.soundindex(primed_sound);

				// they waited too long, detonate it in their hand
				if (EXPLODE && !ent->client->grenade_blew_up && level.time >= ent->client->grenade_time) {
					Weapon_PowerupSound(ent);
					ent->client->weapon_sound = 0;
					fire(ent, true);

					ent->client->grenade_blew_up = true;

					ent->client->grenade_finished_time = level.time + grenade_wait_time;
				}

				if (ent->client->buttons & BUTTON_ATTACK) {
					ent->client->weaponThinkTime = level.time + 1_ms;
					return;
				}

				if (ent->client->grenade_blew_up) {
					if (level.time >= ent->client->grenade_finished_time) {
						ent->client->ps.gunframe = FRAME_FIRE_LAST;
						ent->client->grenade_blew_up = false;
						ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
					} else {
						return;
					}
				} else {
					ent->client->ps.gunframe++;

					Weapon_PowerupSound(ent);
					ent->client->weapon_sound = 0;
					fire(ent, false);

					if (!EXPLODE || !ent->client->grenade_blew_up)
						ent->client->grenade_finished_time = level.time + grenade_wait_time;

					if (!ent->deadFlag && ent->s.modelindex == MODELINDEX_PLAYER && ent->health > 0) // VWep animations screw up corpses
					{
						if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
							ent->client->animPriority = ANIM_ATTACK;
							ent->s.frame = FRAME_crattak1 - 1;
							ent->client->animEnd = FRAME_crattak3;
						} else {
							ent->client->animPriority = ANIM_ATTACK | ANIM_REVERSED;
							ent->s.frame = FRAME_wave08;
							ent->client->animEnd = FRAME_wave01;
						}
						ent->client->animTime = 0_ms;
					}
				}
			}

			ent->client->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

			if ((ent->client->ps.gunframe == FRAME_FIRE_LAST) && (level.time < ent->client->grenade_finished_time))
				return;

			ent->client->ps.gunframe++;

			if (ent->client->ps.gunframe == FRAME_IDLE_FIRST) {
				ent->client->grenade_finished_time = 0_ms;
				ent->client->weaponState = WEAPON_READY;
				ent->client->weaponFireBuffered = false;
				Weapon_SetFinished(ent);

				if (extra_idle_frame)
					ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;

				// Paril: if we ran out of the throwable, switch
				// so we don't appear to be holding one that we
				// can't throw
				if (!ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
					NoAmmoWeaponChange(ent, false);
					Change_Weapon(ent);
				}
			}
		}
	}
}

void Weapon_HandGrenade(gentity_t *ent) {
	constexpr int pause_frames[] = { 29, 34, 39, 48, 0 };

	Throw_Generic(ent, 15, 48, 5, "weapons/hgrena1b.wav", 11, 12, pause_frames, true, "weapons/hgrenc1b.wav", Weapon_HandGrenade_Fire, true);

	// [Paril-KEX] skip the duped frame
	if (ent->client->ps.gunframe == 1)
		ent->client->ps.gunframe = 2;
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

static void Weapon_GrenadeLauncher_Fire(gentity_t *ent) {
	int		damage;
	float	splashRadius;
	int		speed;

	if (RS(RS_Q3A)) {
		damage = 100;
		splashRadius = 150;
		speed = 700;
	} else {
		damage = 120;
		splashRadius = (float)(damage + 40);
		speed = 600;
	}

	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// muffmode: but why is this the exception? reverted
	// limit upwards angle so you don't fire it behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->vAngle[PITCH]), ent->client->vAngle[YAW], ent->client->vAngle[ROLL] }, { 8, 0, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->vForward * -2, { -1.f, 0.f, 0.f });

	fire_grenade(ent, start, dir, damage, speed, 2.5_sec, splashRadius, (crandom_open() * 10.0f), (200 + crandom_open() * 10.0f), false);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_GRENADE_LAUNCHER]++;
	RemoveAmmo(ent, 1);
}

void Weapon_GrenadeLauncher(gentity_t *ent) {
	constexpr int pause_frames[] = { 34, 51, 59, 0 };
	constexpr int fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, Weapon_GrenadeLauncher_Fire);
}

/*
======================================================================

ROCKET LAUNCHER

======================================================================
*/

static void Weapon_RocketLauncher_Fire(gentity_t *ent) {
	int damage = 100;	// 120;
	int splashRadius = 100;	//120;
	int splashDamage = damage;
	int speed;

	switch (game.ruleset) {
	case RS_Q1:
		speed = 1000;
		break;
	case RS_Q3A:
		speed = 900;
		break;
	default:
		speed = 800;	// 650;
		break;
	}
	if (g_frenzy->integer)
		speed *= 1.5;

	if (is_quad) {
		damage *= damage_multiplier;
		splashDamage *= damage_multiplier;
	}

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 8, 8, -8 }, start, dir);
	fire_rocket(ent, start, dir, damage, speed, splashRadius, splashDamage);

	P_AddWeaponKick(ent, ent->client->vForward * -2, { -1.f, 0.f, 0.f });

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_ROCKET_LAUNCHER]++;
	RemoveAmmo(ent, 1);
}

void Weapon_RocketLauncher(gentity_t *ent) {
	constexpr int pause_frames[] = { 25, 33, 42, 50, 0 };
	constexpr int fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}


/*
======================================================================

GRAPPLE

======================================================================
*/

// self is grapple, not player
static void Weapon_Grapple_Reset(gentity_t *self) {
	if (!self || !self->owner->client || !self->owner->client->grapple_ent)
		return;

	gi.sound(self->owner, CHAN_WEAPON, gi.soundindex("weapons/grapple/grreset.wav"), self->owner->client->silencer_shots ? 0.2f : 1.0f, ATTN_NORM, 0);

	gclient_t *cl;
	cl = self->owner->client;
	cl->grapple_ent = nullptr;
	cl->grapple_release_time = level.time + 1_sec;
	cl->grapple_state = GRAPPLE_STATE_FLY; // we're firing, not on hook
	self->owner->flags &= ~FL_NO_KNOCKBACK;
	FreeEntity(self);
}

void Weapon_Grapple_DoReset(gclient_t *cl) {
	if (cl && cl->grapple_ent)
		Weapon_Grapple_Reset(cl->grapple_ent);
}

static TOUCH(Weapon_Grapple_Touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	float volume = 1.0;

	if (other == self->owner)
		return;

	if (self->owner->client->grapple_state != GRAPPLE_STATE_FLY)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY)) {
		Weapon_Grapple_Reset(self);
		return;
	}

	self->velocity = {};

	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takeDamage) {
		if (self->dmg)
			Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_NONE | DAMAGE_STAT_ONCE, MOD_GRAPPLE);
		Weapon_Grapple_Reset(self);
		return;
	}

	self->owner->client->grapple_state = GRAPPLE_STATE_PULL; // we're on hook
	self->enemy = other;

	self->solid = SOLID_NOT;

	if (self->owner->client->silencer_shots)
		volume = 0.2f;

	gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/grapple/grhit.wav"), volume, ATTN_NORM, 0);
	self->s.sound = gi.soundindex("weapons/grapple/grpull.wav");

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPARKS);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(tr.plane.normal);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

// draw beam between grapple and self
static void Weapon_Grapple_DrawCable(gentity_t *self) {
	if (self->owner->client->grapple_state == GRAPPLE_STATE_HANG)
		return;

	vec3_t start, dir;
	P_ProjectSource(self->owner, self->owner->client->vAngle, { 7, 2, -9 }, start, dir);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_GRAPPLE_CABLE_2);
	gi.WriteEntity(self->owner);
	gi.WritePosition(start);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

// pull the player toward the grapple
void Weapon_Grapple_Pull(gentity_t *self) {
	vec3_t hookdir, v;
	float  vlen;

	if (self->owner->client->pers.weapon && self->owner->client->pers.weapon->id == IT_WEAPON_GRAPPLE &&
		!(self->owner->client->newWeapon || ((self->owner->client->latchedButtons | self->owner->client->buttons) & BUTTON_HOLSTER)) &&
		self->owner->client->weaponState != WEAPON_FIRING &&
		self->owner->client->weaponState != WEAPON_ACTIVATING) {
		if (!self->owner->client->newWeapon)
			self->owner->client->newWeapon = self->owner->client->pers.weapon;

		Weapon_Grapple_Reset(self);
		return;
	}

	if (self->enemy) {
		if (self->enemy->solid == SOLID_NOT) {
			Weapon_Grapple_Reset(self);
			return;
		}
		if (self->enemy->solid == SOLID_BBOX) {
			v = self->enemy->size * 0.5f;
			v += self->enemy->s.origin;
			self->s.origin = v + self->enemy->mins;
			gi.linkentity(self);
		} else
			self->velocity = self->enemy->velocity;

		if (self->enemy->deadFlag) { // he died
			Weapon_Grapple_Reset(self);
			return;
		}
	}

	Weapon_Grapple_DrawCable(self);

	if (self->owner->client->grapple_state > GRAPPLE_STATE_FLY) {
		// pull player toward grapple
		vec3_t forward, up;

		AngleVectors(self->owner->client->vAngle, forward, nullptr, up);
		v = self->owner->s.origin;
		v[2] += self->owner->viewHeight;
		hookdir = self->s.origin - v;

		vlen = hookdir.length();

		if (self->owner->client->grapple_state == GRAPPLE_STATE_PULL &&
			vlen < 64) {
			self->owner->client->grapple_state = GRAPPLE_STATE_HANG;
			self->s.sound = gi.soundindex("weapons/grapple/grhang.wav");
		}

		hookdir.normalize();
		hookdir = hookdir * g_grapple_pull_speed->value;
		self->owner->velocity = hookdir;
		self->owner->flags |= FL_NO_KNOCKBACK;
		G_AddGravity(self->owner);
	}
}

static DIE(Weapon_Grapple_Die) (gentity_t *self, gentity_t *other, gentity_t *inflictor, int damage, const vec3_t &point, const mod_t &mod) -> void {
	if (mod.id == MOD_CRUSH)
		Weapon_Grapple_Reset(self);
}

static bool Weapon_Grapple_FireHook(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, effects_t effect) {
	gentity_t	*grapple;
	trace_t	tr;
	vec3_t	normalized = dir.normalized();

	grapple = Spawn();
	grapple->s.origin = start;
	grapple->s.old_origin = start;
	grapple->s.angles = vectoangles(normalized);
	grapple->velocity = normalized * speed;
	grapple->moveType = MOVETYPE_FLYMISSILE;
	grapple->clipMask = MASK_PROJECTILE;
	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		grapple->clipMask &= ~CONTENTS_PLAYER;
	grapple->solid = SOLID_BBOX;
	grapple->s.effects |= effect;
	grapple->s.modelindex = gi.modelindex("models/weapons/grapple/hook/tris.md2");
	grapple->owner = self;
	grapple->touch = Weapon_Grapple_Touch;
	grapple->dmg = damage;
	grapple->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;
	grapple->takeDamage = true;
	grapple->die = Weapon_Grapple_Die;
	self->client->grapple_ent = grapple;
	self->client->grapple_state = GRAPPLE_STATE_FLY; // we're firing, not on hook
	gi.linkentity(grapple);

	tr = gi.traceline(self->s.origin, grapple->s.origin, grapple, grapple->clipMask);
	if (tr.fraction < 1.0f) {
		grapple->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		grapple->touch(grapple, tr.ent, tr, false);
		return false;
	}

	grapple->s.sound = gi.soundindex("weapons/grapple/grfly.wav");

	return true;
}

static void Weapon_Grapple_DoFire(gentity_t *ent, const vec3_t &g_offset, int damage, effects_t effect) {
	float volume = 1.0;

	if (ent->client->grapple_state > GRAPPLE_STATE_FLY)
		return; // it's already out

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, vec3_t{ 24, 8, -8 + 2 } + g_offset, start, dir);

	if (ent->client->silencer_shots)
		volume = 0.2f;

	if (Weapon_Grapple_FireHook(ent, start, dir, damage, g_grapple_fly_speed->value, effect))
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/grapple/grfire.wav"), volume, ATTN_NORM, 0);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

static void Weapon_Grapple_Fire(gentity_t *ent) {
	Weapon_Grapple_DoFire(ent, vec3_origin, g_grapple_damage->integer, EF_NONE);
}

void Weapon_Grapple(gentity_t *ent) {
	constexpr int pause_frames[] = { 10, 18, 27, 0 };
	constexpr int fire_frames[] = { 6, 0 };
	int			  prevstate;

	// if the the attack button is still down, stay in the firing frame
	if ((ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->weaponState == WEAPON_FIRING &&
		ent->client->grapple_ent)
		ent->client->ps.gunframe = 6;

	if (!(ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->grapple_ent) {
		Weapon_Grapple_Reset(ent->client->grapple_ent);
		if (ent->client->weaponState == WEAPON_FIRING)
			ent->client->weaponState = WEAPON_READY;
	}

	if ((ent->client->newWeapon || ((ent->client->latchedButtons | ent->client->buttons) & BUTTON_HOLSTER)) &&
		ent->client->grapple_state > GRAPPLE_STATE_FLY &&
		ent->client->weaponState == WEAPON_FIRING) {
		// he wants to change weapons while grappled
		if (!ent->client->newWeapon)
			ent->client->newWeapon = ent->client->pers.weapon;
		ent->client->weaponState = WEAPON_DROPPING;
		ent->client->ps.gunframe = 32;
	}

	prevstate = ent->client->weaponState;
	Weapon_Generic(ent, 5, 10, 31, 36, pause_frames, fire_frames,
		Weapon_Grapple_Fire);

	// if the the attack button is still down, stay in the firing frame
	if ((ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->weaponState == WEAPON_FIRING &&
		ent->client->grapple_ent)
		ent->client->ps.gunframe = 6;

	// if we just switched back to grapple, immediately go to fire frame
	if (prevstate == WEAPON_ACTIVATING &&
		ent->client->weaponState == WEAPON_READY &&
		ent->client->grapple_state > GRAPPLE_STATE_FLY) {
		if (!(ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)))
			ent->client->ps.gunframe = 6;
		else
			ent->client->ps.gunframe = 5;
		ent->client->weaponState = WEAPON_FIRING;
	}
}


/*
======================================================================

OFF-HAND HOOK

======================================================================
*/

static void Weapon_Hook_DoFire(gentity_t *ent, const vec3_t &g_offset, int damage, effects_t effect) {
	if (ent->client->grapple_state > GRAPPLE_STATE_FLY)
		return; // it's already out

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, vec3_t{ 24, 0, 0 } + g_offset, start, dir);

	if (Weapon_Grapple_FireHook(ent, start, dir, damage, g_grapple_fly_speed->value, effect))
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/grapple/grfire.wav"), ent->client->silencer_shots ? 0.2f : 1.0f, ATTN_NORM, 0);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

void Weapon_Hook(gentity_t *ent) {
	Weapon_Hook_DoFire(ent, vec3_origin, g_grapple_damage->integer, EF_NONE);
}

/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

static void Weapon_Blaster_Fire(gentity_t *ent, const vec3_t &g_offset, int damage, bool hyper, effects_t effect) {
	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, vec3_t{ 24, 8, -8 } + g_offset, start, dir);

	if (hyper)
		P_AddWeaponKick(ent, ent->client->vForward * -2, { crandom() * 0.7f, crandom() * 0.7f, crandom() * 0.7f });
	else
		P_AddWeaponKick(ent, ent->client->vForward * -2, { -1.f, 0.f, 0.f });

	// let the regular blaster projectiles travel a bit faster because it is a completely useless gun
	int speed;
	if (RS(RS_Q3A))
		speed = hyper ? 2000 : 2500;
	else
		speed = hyper ? 1000 : 1500;

	fire_blaster(ent, start, dir, damage, speed, effect, hyper ? MOD_HYPERBLASTER : MOD_BLASTER);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_BLASTER]++;
}

static void Weapon_Blaster_DoFire(gentity_t *ent) {
	// give the blaster 15 across the board instead of just in dm
	int damage = 15;
	Weapon_Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
}

void Weapon_Blaster(gentity_t *ent) {
	constexpr int pause_frames[] = { 19, 32, 0 };
	constexpr int fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_DoFire);
}

static void Weapon_HyperBlaster_Fire(gentity_t *ent) {
	float	rotation;
	int		damage;

	// start on frame 6
	if (ent->client->ps.gunframe > 20)
		ent->client->ps.gunframe = 6;
	else
		ent->client->ps.gunframe++;

	// if we reached end of loop, have ammo & holding attack, reset loop
	// otherwise play wind down
	if (ent->client->ps.gunframe == 12) {
		if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] && (ent->client->buttons & BUTTON_ATTACK))
			ent->client->ps.gunframe = 6;
		else
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
	}

	// play weapon sound for firing loop
	if (ent->client->ps.gunframe >= 6 && ent->client->ps.gunframe <= 11)
		ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");
	else
		ent->client->weapon_sound = 0;
	
	// fire frames
	bool request_firing = ent->client->weaponFireBuffered || (ent->client->buttons & BUTTON_ATTACK);

	if (request_firing) {
		if (ent->client->ps.gunframe >= 6 && ent->client->ps.gunframe <= 11) {
			vec3_t offset = { 0 };

			ent->client->weaponFireBuffered = false;

			if (!ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
				NoAmmoWeaponChange(ent, true);
				return;
			}

			rotation = (ent->client->ps.gunframe - 5) * 2 * PIf / 6;
			offset[0] = -4 * sinf(rotation);
			offset[2] = 0;
			offset[1] = 4 * cosf(rotation);

			if (RS(RS_Q3A)) {
				damage = deathmatch->integer ? 20 : 25;
			} else {
				damage = deathmatch->integer ? 15 : 20;
			}

			Weapon_Blaster_Fire(ent, offset, damage, true, (ent->client->ps.gunframe % 4) ? EF_NONE : EF_HYPERBLASTER);
			Weapon_PowerupSound(ent);

			ent->client->sess.match.totalShots++;
			ent->client->sess.match.totalShotsPerWeapon[WEAP_HYPERBLASTER]++;
			RemoveAmmo(ent, 1);

			ent->client->animPriority = ANIM_ATTACK;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
				ent->s.frame = FRAME_crattak1 - (int)(frandom() + 0.25f);
				ent->client->animEnd = FRAME_crattak9;
			} else {
				ent->s.frame = FRAME_attack1 - (int)(frandom() + 0.25f);
				ent->client->animEnd = FRAME_attack8;
			}
			ent->client->animTime = 0_ms;
		}
	}
}

void Weapon_HyperBlaster(gentity_t *ent) {
	constexpr int pause_frames[] = { 0 };

	Weapon_Repeating(ent, 5, 20, 49, 53, pause_frames, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

static void Weapon_Machinegun_Fire(gentity_t *ent) {
	int damage = 8;
	int kick = 2;
	int vs, hs;

	if (RS(RS_Q3A)) {
		damage = GT(GT_TDM) ? 5 : 7;
		vs = 200;
		hs = 200;
	} else {
		damage = 8;
		vs = DEFAULT_BULLET_VSPREAD;
		hs = DEFAULT_BULLET_HSPREAD;
	}

	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		ent->client->ps.gunframe = 6;
		return;
	}

	if (ent->client->ps.gunframe == 4)
		ent->client->ps.gunframe = 5;
	else
		ent->client->ps.gunframe = 4;

	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < 1) {
		ent->client->ps.gunframe = 6;
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad) {
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t kickOrigin{}, kick_angles{};
	for (size_t i = 0; i < 3; i++) {
		kickOrigin[i] = crandom() * 0.35f;
		kick_angles[i] = crandom() * 0.7f;
	}
	P_AddWeaponKick(ent, kickOrigin, kick_angles);

	// get start / end positions
	vec3_t start, dir;
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, -8 }, start, dir);
	G_LagCompensate(ent, start, dir);


	fire_bullet(ent, start, dir, damage, kick, hs, vs, MOD_MACHINEGUN);
	G_UnLagCompensate();
	Weapon_PowerupSound(ent);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_MACHINEGUN]++;
	RemoveAmmo(ent, 1);

	ent->client->animPriority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - (int)(frandom() + 0.25f);
		ent->client->animEnd = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - (int)(frandom() + 0.25f);
		ent->client->animEnd = FRAME_attack8;
	}
	ent->client->animTime = 0_ms;
}

void Weapon_Machinegun(gentity_t *ent) {
	constexpr int pause_frames[] = { 23, 45, 0 };

	Weapon_Repeating(ent, 3, 5, 45, 49, pause_frames, Weapon_Machinegun_Fire);
}

static void Weapon_Chaingun_Fire(gentity_t *ent) {
	int	  i;
	int	  shots;
	int	  damage = deathmatch->integer ? 6 : 8;
	int	  kick = 2;

	if (ent->client->ps.gunframe > 31) {
		ent->client->ps.gunframe = 5;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
	} else if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK)) {
		ent->client->ps.gunframe = 32;
		ent->client->weapon_sound = 0;
		return;
	} else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->pers.weapon->ammo]) {
		ent->client->ps.gunframe = 15;
	} else {
		ent->client->ps.gunframe++;
	}

	if (ent->client->ps.gunframe == 22) {
		ent->client->weapon_sound = 0;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}

	if (ent->client->ps.gunframe < 5 || ent->client->ps.gunframe > 21)
		return;

	ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

	ent->client->animPriority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - (ent->client->ps.gunframe & 1);
		ent->client->animEnd = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - (ent->client->ps.gunframe & 1);
		ent->client->animEnd = FRAME_attack8;
	}
	ent->client->animTime = 0_ms;

	if (ent->client->ps.gunframe <= 9)
		shots = 1;
	else if (ent->client->ps.gunframe <= 14) {
		if (ent->client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	} else
		shots = 3;

	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < shots)
		shots = ent->client->pers.inventory[ent->client->pers.weapon->ammo];

	if (!shots) {
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad) {
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t kickOrigin{}, kick_angles{};
	for (i = 0; i < 3; i++) {
		kickOrigin[i] = crandom() * 0.35f;
		kick_angles[i] = crandom() * (0.5f + (shots * 0.15f));
	}
	P_AddWeaponKick(ent, kickOrigin, kick_angles);

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, -8 }, start, dir);

	G_LagCompensate(ent, start, dir);
	for (i = 0; i < shots; i++) {
		// get start / end positions
		// Paril: kill sideways angle on hitscan
		P_ProjectSource(ent, ent->client->vAngle, { 0, 0, -8 }, start, dir);

		fire_bullet(ent, start, dir, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}
	G_UnLagCompensate();

	Weapon_PowerupSound(ent);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots += shots;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_CHAINGUN] += shots;
	RemoveAmmo(ent, shots);
}

void Weapon_Chaingun(gentity_t *ent) {
	constexpr int pause_frames[] = { 38, 43, 51, 61, 0 };

	Weapon_Repeating(ent, 4, 31, 61, 64, pause_frames, Weapon_Chaingun_Fire);
}

/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

static void Weapon_Shotgun_Fire(gentity_t *ent) {
	int damage = 4;
	int kick = 4;	// 8;

	vec3_t start, dir;
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->vForward * -2, { -2.f, 0.f, 0.f });

	if (RS(RS_Q3A))
		damage = 10;

	if (is_quad) {
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	int count = RS(RS_Q3A) ? 11 : 12;

	G_LagCompensate(ent, start, dir);
	fire_shotgun(ent, start, dir, damage, kick, 500, 500, count, MOD_SHOTGUN);
	G_UnLagCompensate();

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots += count;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_SHOTGUN] += count;
	RemoveAmmo(ent, 1);
}

void Weapon_Shotgun(gentity_t *ent) {
	constexpr int pause_frames[] = { 22, 28, 34, 0 };
	constexpr int fire_frames[] = { 8, 0 };

	Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, Weapon_Shotgun_Fire);
}

static void Weapon_SuperShotgun_Fire(gentity_t *ent) {
	int damage = 6;
	int kick = 6;	// 12;

	if (is_quad) {
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t start, dir;
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, -8 }, start, dir);
	G_LagCompensate(ent, start, dir);
	vec3_t v;
	v[PITCH] = ent->client->vAngle[PITCH];
	v[YAW] = ent->client->vAngle[YAW] - 5;
	v[ROLL] = ent->client->vAngle[ROLL];
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, v, { 0, 0, -8 }, start, dir);
	fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
	v[YAW] = ent->client->vAngle[YAW] + 5;
	P_ProjectSource(ent, v, { 0, 0, -8 }, start, dir);
	fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
	G_UnLagCompensate();

	P_AddWeaponKick(ent, ent->client->vForward * -2, { -2.f, 0.f, 0.f });

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots += DEFAULT_SSHOTGUN_COUNT;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_SUPER_SHOTGUN] += DEFAULT_SSHOTGUN_COUNT;
	RemoveAmmo(ent, 2);
}

void Weapon_SuperShotgun(gentity_t *ent) {
	constexpr int pause_frames[] = { 29, 42, 57, 0 };
	constexpr int fire_frames[] = { 7, 0 };

	Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, Weapon_SuperShotgun_Fire);
}

/*
======================================================================

RAILGUN

======================================================================
*/

static void Weapon_Railgun_Fire(gentity_t *ent) {
	// normal damage too extreme for DM
	int damage = deathmatch->integer ? 100 : 150;
	int kick = damage;	// RS(RS_Q3A) ? damage : damage * 2;

	if (is_quad) {
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 0, 7, -8 }, start, dir);
	G_LagCompensate(ent, start, dir);
	fire_rail(ent, start, dir, damage, kick);
	G_UnLagCompensate();

	P_AddWeaponKick(ent, ent->client->vForward * -3, { -3.f, 0.f, 0.f });

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_RAILGUN]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Railgun(gentity_t *ent) {
	constexpr int pause_frames[] = { 56, 0 };
	constexpr int fire_frames[] = { 4, 0 };

	Weapon_Generic(ent, 3, 18, 56, 61, pause_frames, fire_frames, Weapon_Railgun_Fire);
}

/*
======================================================================

BFG10K

======================================================================
*/
static void Weapon_BFG_Fire(gentity_t *ent) {
	bool	q3 = RS(RS_Q3A);
	int		damage, speed;
	float	splashRadius;

	if (q3) {
		damage = 100;
		splashRadius = 120;
		speed = 1000;
	} else {
		damage = deathmatch->integer ? 200 : 500;
		splashRadius = 1000;
		speed = 400;
	}

	if (ent->client->ps.gunframe == 9) {
		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < 50)
		return;

	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 8, 8, -8 }, start, dir);
	fire_bfg(ent, start, dir, damage, speed, splashRadius);

	if (q3) {
		P_AddWeaponKick(ent, ent->client->vForward * -2, { -1.f, 0.f, 0.f });
	} else {
		P_AddWeaponKick(ent, ent->client->vForward * -2, { -20.f, 0, crandom() * 8 });
		ent->client->kick.total = DAMAGE_TIME();
		ent->client->kick.time = level.time + ent->client->kick.total;
	}

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_BFG2 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_BFG]++;
	RemoveAmmo(ent, q3 ? 10 : 50);
}

void Weapon_BFG(gentity_t *ent) {
	constexpr int pause_frames[] = { 39, 45, 50, 55, 0 };
	constexpr int fire_frames[] = { 9, 17, 0 };
	constexpr int fire_frames_q3a[] = { 15, 17, 0 };

	Weapon_Generic(ent, 8, 32, 54, 58, pause_frames, RS(RS_Q3A) ? fire_frames_q3a : fire_frames, Weapon_BFG_Fire);
}

/*
======================================================================

PROX MINES

======================================================================
*/
static void Weapon_ProxLauncher_Fire(gentity_t *ent) {
	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't fire behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->vAngle[PITCH]), ent->client->vAngle[YAW], ent->client->vAngle[ROLL] }, { 8, 8, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->vForward * -2, { -1.f, 0.f, 0.f });

	fire_prox(ent, start, dir, damage_multiplier, 600);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_PROX | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_PROX_LAUNCHER]++;
	RemoveAmmo(ent, 1);
}

void Weapon_ProxLauncher(gentity_t *ent) {
	constexpr int pause_frames[] = { 34, 51, 59, 0 };
	constexpr int fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, Weapon_ProxLauncher_Fire);
}


/*
======================================================================

TESLA MINES

======================================================================
*/
static void Weapon_Tesla_Fire(gentity_t *ent, bool held) {
	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't throw behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->vAngle[PITCH]), ent->client->vAngle[YAW], ent->client->vAngle[ROLL] }, { 0, 0, -22 }, start, dir);

	gtime_t timer = ent->client->grenade_time - level.time;
	int	  speed = (int)(ent->health <= 0 ? GRENADE_MINSPEED : min(GRENADE_MINSPEED + (GRENADE_TIMER - timer).seconds() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.seconds()), GRENADE_MAXSPEED));

	ent->client->grenade_time = 0_ms;

	fire_tesla(ent, start, dir, damage_multiplier, speed);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_TESLA_MINE]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Tesla(gentity_t *ent) {
	constexpr int pause_frames[] = { 21, 0 };

	Throw_Generic(ent, 8, 32, -1, nullptr, 1, 2, pause_frames, false, nullptr, Weapon_Tesla_Fire, false);
}

/*
======================================================================

CHAINFIST

======================================================================
*/
constexpr int32_t CHAINFIST_REACH = 32;	// 24;

static void Weapon_ChainFist_Fire(gentity_t *ent) {
	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		if (ent->client->ps.gunframe == 13 ||
			ent->client->ps.gunframe == 23 ||
			ent->client->ps.gunframe >= 32) {
			ent->client->ps.gunframe = 33;
			return;
		}
	}

	int damage = deathmatch->integer ? 15 : 7;

	if (is_quad)
		damage *= damage_multiplier;

	// set start point
	vec3_t start, dir;

	if (GT(GT_BALL) && ent->client->pers.inventory[IT_BALL] > 0) {
		//fire_grenade(ent, start, dir, damage, 800, 25_sec, 0, (crandom_open() * 10.0f), (200 + crandom_open() * 10.0f), false);

		constexpr int pause_frames[] = { 29, 34, 39, 48, 0 };
		Throw_Generic(ent, 15, 48, 5, "weapons/hgrena1b.wav", 11, 12, pause_frames, true, "weapons/hgrenc1b.wav", Weapon_HandGrenade_Fire, true);

		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_GRENADE | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		PlayerNoise(ent, start, PNOISE_WEAPON);

		ent->client->pers.inventory[IT_BALL] = 0;
		return;
	}

	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, -4 }, start, dir);

	if (fire_player_melee(ent, start, dir, CHAINFIST_REACH, damage, 100, MOD_CHAINFIST)) {
		if (ent->client->empty_click_sound < level.time) {
			ent->client->empty_click_sound = level.time + 500_ms;
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sawslice.wav"), 1.f, ATTN_NORM, 0.f);
		}
	}

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->ps.gunframe++;

	if (ent->client->buttons & BUTTON_ATTACK) {
		if (ent->client->ps.gunframe == 12)
			ent->client->ps.gunframe = 14;
		else if (ent->client->ps.gunframe == 22)
			ent->client->ps.gunframe = 24;
		else if (ent->client->ps.gunframe >= 32)
			ent->client->ps.gunframe = 7;
	}

	// start the animation
	if (ent->client->animPriority != ANIM_ATTACK || frandom() < 0.25f) {
		ent->client->animPriority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
			ent->s.frame = FRAME_crattak1 - 1;
			ent->client->animEnd = FRAME_crattak9;
		} else {
			ent->s.frame = FRAME_attack1 - 1;
			ent->client->animEnd = FRAME_attack8;
		}
		ent->client->animTime = 0_ms;
	}
}

// this spits out some smoke from the motor. it's a two-stroke, you know.
static void Weapon_ChainFist_smoke(gentity_t *ent) {
	vec3_t tempVec, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 8, 8, -4 }, tempVec, dir);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(tempVec);
	gi.unicast(ent, 0);
}

void Weapon_ChainFist(gentity_t *ent) {
	constexpr int pause_frames[] = { 0 };

	Weapon_Repeating(ent, 4, 32, 57, 60, pause_frames, Weapon_ChainFist_Fire);

	// smoke on idle sequence
	if (ent->client->ps.gunframe == 42 && irandom(8)) {
		if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
			Weapon_ChainFist_smoke(ent);
	} else if (ent->client->ps.gunframe == 51 && irandom(8)) {
		if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
			Weapon_ChainFist_smoke(ent);
	}

	// set the appropriate weapon sound.
	if (ent->client->weaponState == WEAPON_FIRING)
		ent->client->weapon_sound = gi.soundindex("weapons/sawhit.wav");
	else if (ent->client->weaponState == WEAPON_DROPPING)
		ent->client->weapon_sound = 0;
	else if (ent->client->pers.weapon->id == IT_WEAPON_CHAINFIST)
		ent->client->weapon_sound = gi.soundindex("weapons/sawidle.wav");
}

/*
======================================================================

DISRUPTOR

======================================================================
*/
static void Weapon_Disruptor_Fire(gentity_t *ent) {
	vec3_t	 end;
	gentity_t *enemy;
	trace_t	 tr;
	int		 damage;
	vec3_t	 mins, maxs;

	// PMM - felt a little high at 25
	damage = deathmatch->integer ? 45 : 135;

	if (is_quad)
		damage *= damage_multiplier; // pgm

	mins = { -16, -16, -16 };
	maxs = { 16, 16, 16 };

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 24, 8, -8 }, start, dir);

	end = start + (dir * 8192);
	enemy = nullptr;
	// PMM - doing two traces .. one point and one box.
	contents_t mask = MASK_PROJECTILE;

	// [Paril-KEX]
	if (!G_ShouldPlayersCollide(true))
		mask &= ~CONTENTS_PLAYER;

	G_LagCompensate(ent, start, dir);
	tr = gi.traceline(start, end, ent, mask);
	G_UnLagCompensate();
	if (tr.ent != world) {
		if ((tr.ent->svFlags & SVF_MONSTER) || tr.ent->client || (tr.ent->flags & FL_DAMAGEABLE)) {
			if (tr.ent->health > 0)
				enemy = tr.ent;
		}
	} else {
		tr = gi.trace(start, mins, maxs, end, ent, mask);
		if (tr.ent != world) {
			if ((tr.ent->svFlags & SVF_MONSTER) || tr.ent->client || (tr.ent->flags & FL_DAMAGEABLE)) {
				if (tr.ent->health > 0)
					enemy = tr.ent;
			}
		}
	}

	P_AddWeaponKick(ent, ent->client->vForward * -2, { -1.f, 0.f, 0.f });

	fire_disruptor(ent, start, dir, damage, 1000, enemy);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_TRACKER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_DISRUPTOR]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Disruptor(gentity_t *ent) {
	constexpr int pause_frames[] = { 14, 19, 23, 0 };
	constexpr int fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 9, 29, 34, pause_frames, fire_frames, Weapon_Disruptor_Fire);
}

/*
======================================================================

ETF RIFLE

======================================================================
*/
static void Weapon_ETF_Rifle_Fire(gentity_t *ent) {
	int	   damage = 10;
	int	   kick = 3;
	int	   i;
	vec3_t offset;

	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		ent->client->ps.gunframe = 8;
		return;
	}

	if (ent->client->ps.gunframe == 6)
		ent->client->ps.gunframe = 7;
	else
		ent->client->ps.gunframe = 6;

	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < ent->client->pers.weapon->quantity) {
		ent->client->ps.gunframe = 8;
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad) {
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t kickOrigin{}, kick_angles{};
	for (i = 0; i < 3; i++) {
		kickOrigin[i] = crandom() * 0.85f;
		kick_angles[i] = crandom() * 0.85f;
	}
	P_AddWeaponKick(ent, kickOrigin, kick_angles);

	// get start / end positions
	if (ent->client->ps.gunframe == 6)
		offset = { 15, 8, -8 };
	else
		offset = { 15, 6, -8 };

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle + kick_angles, offset, start, dir);
	fire_flechette(ent, start, dir, damage, 1150, kick);
	Weapon_PowerupSound(ent);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte((ent->client->ps.gunframe == 6 ? MZ_ETF_RIFLE : MZ_ETF_RIFLE_2) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_ETF_RIFLE]++;
	RemoveAmmo(ent, 1);

	ent->client->animPriority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - (int)(frandom() + 0.25f);
		ent->client->animEnd = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - (int)(frandom() + 0.25f);
		ent->client->animEnd = FRAME_attack8;
	}
	ent->client->animTime = 0_ms;
}

void Weapon_ETF_Rifle(gentity_t *ent) {
	constexpr int pause_frames[] = { 18, 28, 0 };

	Weapon_Repeating(ent, 4, 7, 37, 41, pause_frames, Weapon_ETF_Rifle_Fire);
}

/*
======================================================================

PLASMA BEAM

======================================================================
*/

static void Weapon_PlasmaBeam_Fire(gentity_t *ent) {
	bool firing = (ent->client->buttons & BUTTON_ATTACK) && !CombatIsDisabled();
	bool has_ammo = ent->client->pers.inventory[ent->client->pers.weapon->ammo] >= ent->client->pers.weapon->quantity;

	if (!firing || !has_ammo) {
		ent->client->ps.gunframe = 13;
		ent->client->weapon_sound = 0;
		ent->client->ps.gunskin = 0;

		if (firing && !has_ammo)
			NoAmmoWeaponChange(ent, true);
		return;
	}

	// start on frame 8
	if (ent->client->ps.gunframe > 12)
		ent->client->ps.gunframe = 8;
	else
		ent->client->ps.gunframe++;

	if (ent->client->ps.gunframe == 12)
		ent->client->ps.gunframe = 8;

	// play weapon sound for firing
	ent->client->weapon_sound = gi.soundindex("weapons/tesla.wav");
	ent->client->ps.gunskin = 1;

	int damage;
	int kick;

	// for comparison, the hyperblaster is 15/20
	// jim requested more damage, so try 15/15 --- PGM 07/23/98
	// muffmode: jim you are a silly boy, 15 is way OP for DM
	switch (game.ruleset) {
	case RS_Q3A:
		damage = deathmatch->integer ? 8 : 15;
		kick = damage;
		break;
	case RS_Q1:
		damage = 30;
		kick = damage;
		break;
	default:
		damage = deathmatch->integer ? 8 : 15;
		kick = damage;
		break;
	}

	if (is_quad) {
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	ent->client->kick.time = 0_ms;

	// This offset is the "view" offset for the beam start (used by trace)
	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 7, 2, -3 }, start, dir);

	// This offset is the entity offset
	G_LagCompensate(ent, start, dir);
	fire_plasmabeam(ent, start, dir, { 2, 7, -3 }, damage, kick, false);
	G_UnLagCompensate();
	Weapon_PowerupSound(ent);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_HEATBEAM | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_PLASMABEAM]++;
	RemoveAmmo(ent, RS(RS_Q1) ? 2 : 1);

	ent->client->animPriority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - (int)(frandom() + 0.25f);
		ent->client->animEnd = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - (int)(frandom() + 0.25f);
		ent->client->animEnd = FRAME_attack8;
	}
	ent->client->animTime = 0_ms;
}

void Weapon_PlasmaBeam(gentity_t *ent) {
	constexpr int pause_frames[] = { 35, 0 };

	Weapon_Repeating(ent, 8, 12, 42, 47, pause_frames, Weapon_PlasmaBeam_Fire);
}


/*
======================================================================

ION RIPPER

======================================================================
*/
static void Weapon_IonRipper_Fire(gentity_t *ent) {
	vec3_t tempAngle;
	int	   damage = deathmatch->integer ? 20 : 50;

	if (is_quad)
		damage *= damage_multiplier;

	tempAngle = ent->client->vAngle;
	tempAngle[YAW] += crandom();

	vec3_t start, dir;
	P_ProjectSource(ent, tempAngle, { 16, 7, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->vForward * -3, { -3.f, 0.f, 0.f });

	fire_ionripper(ent, start, dir, damage, 500, EF_IONRIPPER);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_IONRIPPER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_IONRIPPER]++;
	RemoveAmmo(ent, 1);
}

void Weapon_IonRipper(gentity_t *ent) {
	constexpr int pause_frames[] = { 36, 0 };
	constexpr int fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 7, 36, 39, pause_frames, fire_frames, Weapon_IonRipper_Fire);
}

/*
======================================================================

PHALANX

======================================================================
*/
static void Weapon_Phalanx_Fire(gentity_t *ent) {
	vec3_t v;
	int	   damage;
	float  splashRadius;
	int	   splashDamage;

	damage = irandom(70, 80);
	splashDamage = 120;
	splashRadius = 120;

	if (is_quad) {
		damage *= damage_multiplier;
		splashDamage *= damage_multiplier;
	}

	vec3_t dir;

	if (ent->client->ps.gunframe == 8) {
		v[PITCH] = ent->client->vAngle[PITCH];
		v[YAW] = ent->client->vAngle[YAW] - 1.5f;
		v[ROLL] = ent->client->vAngle[ROLL];

		vec3_t start;
		P_ProjectSource(ent, v, { 0, 8, -8 }, start, dir);

		splashDamage = 30;
		splashRadius = 120;

		fire_phalanx(ent, start, dir, damage, 725, splashRadius, splashDamage);

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_PHALANX2 | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		ent->client->sess.match.totalShots += 2;
		ent->client->sess.match.totalShotsPerWeapon[WEAP_PHALANX] += 2;
		RemoveAmmo(ent, 1);
	} else {
		v[PITCH] = ent->client->vAngle[PITCH];
		v[YAW] = ent->client->vAngle[YAW] + 1.5f;
		v[ROLL] = ent->client->vAngle[ROLL];

		vec3_t start;
		P_ProjectSource(ent, v, { 0, 8, -8 }, start, dir);

		fire_phalanx(ent, start, dir, damage, 725, splashRadius, splashDamage);

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_PHALANX | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		PlayerNoise(ent, start, PNOISE_WEAPON);
	}

	P_AddWeaponKick(ent, ent->client->vForward * -2, { -2.f, 0.f, 0.f });
}

void Weapon_Phalanx(gentity_t *ent) {
	constexpr int pause_frames[] = { 29, 42, 55, 0 };
	constexpr int fire_frames[] = { 7, 8, 0 };

	Weapon_Generic(ent, 5, 20, 58, 63, pause_frames, fire_frames, Weapon_Phalanx_Fire);
}

/*
======================================================================

TRAP

======================================================================
*/

constexpr gtime_t TRAP_TIMER = 5_sec;
constexpr float TRAP_MINSPEED = 300.f;
constexpr float TRAP_MAXSPEED = 700.f;

static void Weapon_Trap_Fire(gentity_t *ent, bool held) {
	int	  speed;

	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't throw behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->vAngle[PITCH]), ent->client->vAngle[YAW], ent->client->vAngle[ROLL] }, { 8, 0, -8 }, start, dir);

	gtime_t timer = ent->client->grenade_time - level.time;
	speed = (int)(ent->health <= 0 ? TRAP_MINSPEED : min(TRAP_MINSPEED + (TRAP_TIMER - timer).seconds() * ((TRAP_MAXSPEED - TRAP_MINSPEED) / TRAP_TIMER.seconds()), TRAP_MAXSPEED));

	ent->client->grenade_time = 0_ms;

	fire_trap(ent, start, dir, speed);

	ent->client->sess.match.totalShots++;
	ent->client->sess.match.totalShotsPerWeapon[WEAP_TRAP]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Trap(gentity_t *ent) {
	constexpr int pause_frames[] = { 29, 34, 39, 48, 0 };

	Throw_Generic(ent, 15, 48, 5, "weapons/trapcock.wav", 11, 12, pause_frames, false, "weapons/traploop.wav", Weapon_Trap_Fire, false);
}
