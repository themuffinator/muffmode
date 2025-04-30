// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_combat.c

#include "g_local.h"

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool CanDamage(gentity_t *targ, gentity_t *inflictor) {
	vec3_t	dest;
	trace_t trace;

	// bmodels need special checking because their origin is 0,0,0
	vec3_t inflictor_center;

	if (inflictor->linked)
		inflictor_center = (inflictor->absMin + inflictor->absMax) * 0.5f;
	else
		inflictor_center = inflictor->s.origin;

	if (targ->solid == SOLID_BSP) {
		dest = closest_point_to_box(inflictor_center, targ->absMin, targ->absMax);

		trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0f)
			return true;
	}

	vec3_t targ_center;

	if (targ->linked)
		targ_center = (targ->absMin + targ->absMax) * 0.5f;
	else
		targ_center = targ->s.origin;

	trace = gi.traceline(inflictor_center, targ_center, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] += 15.0f;
	dest[1] += 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] += 15.0f;
	dest[1] -= 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] -= 15.0f;
	dest[1] += 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] -= 15.0f;
	dest[1] -= 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	return false;
}

/*
============
Killed
============
*/
void Killed(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, mod_t mod) {
	if (targ->health < -999)
		targ->health = -999;

	// [Paril-KEX]
	if ((targ->svFlags & SVF_MONSTER) && targ->monsterInfo.aiflags & AI_MEDIC) {
		if (targ->enemy && targ->enemy->inUse && (targ->enemy->svFlags & SVF_MONSTER)) // god, I hope so
			M_CleanupHealTarget(targ->enemy);

		// clean up self
		targ->monsterInfo.aiflags &= ~AI_MEDIC;
	}

	targ->enemy = attacker;
	targ->lastMOD = mod;

	// [Paril-KEX] monsters call die in their damage handler
	if (targ->svFlags & SVF_MONSTER)
		return;

	targ->die(targ, inflictor, attacker, damage, point, mod);

	if (targ->monsterInfo.setskin)
		targ->monsterInfo.setskin(targ);
}

/*
================
SpawnDamage
================
*/
void SpawnDamage(int type, const vec3_t &origin, const vec3_t &normal, int damage) {
	if (damage > 255)
		damage = 255;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	gi.WritePosition(origin);
	gi.WriteDir(normal);
	gi.multicast(origin, MULTICAST_PVS, false);
}

/*
============
Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dFlags		these flags are used to control how Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
static int CheckPowerArmor(gentity_t *ent, const vec3_t &point, const vec3_t &normal, int damage, damageflags_t dFlags) {
	gclient_t	*client;
	int			save;
	item_id_t	powerArmorType;
	int			damagePerCell;
	int			*power;
	int			power_used;

	if (ent->health <= 0)
		return 0;

	if (!damage)
		return 0;

	client = ent->client;

	if (dFlags & (DAMAGE_NO_ARMOR | DAMAGE_NO_POWER_ARMOR))
		return 0;

	if (client) {
		powerArmorType = PowerArmorType(ent);
		power = &client->pers.inventory[IT_AMMO_CELLS];
	} else if (ent->svFlags & SVF_MONSTER) {
		powerArmorType = ent->monsterInfo.powerArmorType;
		power = &ent->monsterInfo.powerArmorPower;
	} else
		return 0;

	if (powerArmorType == IT_NULL)
		return 0;
	if (!*power)
		return 0;

	if (powerArmorType == IT_POWER_SCREEN) {
		vec3_t vec;
		float  dot;
		vec3_t forward;

		// only works if damage point is in front
		AngleVectors(ent->s.angles, forward, nullptr, nullptr);
		vec = point - ent->s.origin;
		vec.normalize();
		dot = vec.dot(forward);
		if (dot <= 0.3f)
			return 0;

		damagePerCell = 1;
		damage = damage / 3;
	} else {
		damagePerCell = deathmatch->integer ? 1 : 2; // power armor is weaker in DM
		damage = (2 * damage) / 3;
	}

	// Paril: fix small amounts of damage not
	// being absorbed
	damage = max(1, damage);

	save = *power * damagePerCell;

	if (!save)
		return 0;

	// [Paril-KEX] energy damage should do more to power armor, not ETF Rifle shots.
	if (dFlags & DAMAGE_ENERGY)
		save = max(1, save / 2);

	if (save > damage)
		save = damage;

	// [Paril-KEX] energy damage should do more to power armor, not ETF Rifle shots.
	if (dFlags & DAMAGE_ENERGY)
		power_used = (save / damagePerCell) * 2;
	else
		power_used = save / damagePerCell;

	power_used = max(1, power_used);

	SpawnDamage(TE_SCREEN_SPARKS, point, normal, save);
	ent->powerarmor_time = level.time + 200_ms;

	// Paril: adjustment so that power armor
	// always uses damagePerCell even if it does
	// only a single point of damage
	*power = max(0, *power - max(damagePerCell, power_used));

	// check power armor turn-off states
	if (ent->client)
		CheckPowerArmorState(ent);
	else if (!*power) {
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/mon_power2.wav"), 1.f, ATTN_NORM, 0.f);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_POWER_SPLASH);
		gi.WriteEntity(ent);
		gi.WriteByte((powerArmorType == IT_POWER_SCREEN) ? 1 : 0);
		gi.multicast(ent->s.origin, MULTICAST_PHS, false);
	}

	return save;
}

static int CheckArmor(gentity_t *ent, const vec3_t &point, const vec3_t &normal, int damage, int te_sparks,
	damageflags_t dFlags) {
	gclient_t *client;
	int		   save;
	item_id_t  index;
	gitem_t *armor;
	int *power;

	if (!damage)
		return 0;

	if (dFlags & (DAMAGE_NO_ARMOR | DAMAGE_NO_REG_ARMOR))
		return 0;

	client = ent->client;
	index = ArmorIndex(ent);

	if (!index)
		return 0;

	armor = GetItemByIndex(index);
	
	if (dFlags & DAMAGE_ENERGY)
		save = (int)ceilf(armor_stats[game.ruleset][armor->quantity].energy_protection * damage);
	else
		save = (int)ceilf(armor_stats[game.ruleset][armor->quantity].normal_protection * damage);

	if (client)
		power = &client->pers.inventory[index];
	else
		power = &ent->monsterInfo.armor_power;

	if (save >= *power)
		save = *power;

	if (!save)
		return 0;

	*power -= save;

	if (!client && !ent->monsterInfo.armor_power)
		ent->monsterInfo.armor_type = IT_NULL;

	SpawnDamage(te_sparks, point, normal, save);

	return save;
}

static void M_ReactToDamage(gentity_t *targ, gentity_t *attacker, gentity_t *inflictor) {
	if (!(attacker->client) && !(attacker->svFlags & SVF_MONSTER))
		return;

	// logic for tesla - if you are hit by a tesla, and can't see who you should be mad at (attacker)
	// attack the tesla
	// also, target the tesla if it's a "new" tesla
	if ((inflictor) && (!strcmp(inflictor->className, "tesla_mine"))) {
		if ((MarkTeslaArea(targ, inflictor) || brandom()) && (!targ->enemy || !targ->enemy->className || strcmp(targ->enemy->className, "tesla_mine")))
			TargetTesla(targ, inflictor);
		return;
	}

	if (attacker == targ || attacker == targ->enemy)
		return;

	// if we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (targ->monsterInfo.aiflags & AI_GOOD_GUY) {
		if (attacker->client || (attacker->monsterInfo.aiflags & AI_GOOD_GUY))
			return;
	}

	//  if we're currently mad at something a target_anger made us mad at, ignore
	//  damage
	if (targ->enemy && targ->monsterInfo.aiflags & AI_TARGET_ANGER) {
		float percentHealth;

		// make sure whatever we were pissed at is still around.
		if (targ->enemy->inUse) {
			percentHealth = (float)(targ->health) / (float)(targ->max_health);
			if (targ->enemy->inUse && percentHealth > 0.33f)
				return;
		}

		// remove the target anger flag
		targ->monsterInfo.aiflags &= ~AI_TARGET_ANGER;
	}

	// we recently switched from reacting to damage, don't do it
	if (targ->monsterInfo.react_to_damage_time > level.time)
		return;

	// if we're healing someone, do like above and try to stay with them
	if ((targ->enemy) && (targ->monsterInfo.aiflags & AI_MEDIC)) {
		float percentHealth;

		percentHealth = (float)(targ->health) / (float)(targ->max_health);
		// ignore it some of the time
		if (targ->enemy->inUse && percentHealth > 0.25f)
			return;

		// remove the medic flag
		M_CleanupHealTarget(targ->enemy);
		targ->monsterInfo.aiflags &= ~AI_MEDIC;
	}

	// we now know that we are not both good guys
	targ->monsterInfo.react_to_damage_time = level.time + random_time(3_sec, 5_sec);

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker->client) {
		targ->monsterInfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ->enemy != attacker) {
			if (targ->enemy && targ->enemy->client) {
				if (visible(targ, targ->enemy)) {
					targ->oldEnemy = attacker;
					return;
				}
				targ->oldEnemy = targ->enemy;
			}

			// [Paril-KEX]
			if ((targ->svFlags & SVF_MONSTER) && targ->monsterInfo.aiflags & AI_MEDIC) {
				if (targ->enemy && targ->enemy->inUse && (targ->enemy->svFlags & SVF_MONSTER)) // god, I hope so
				{
					M_CleanupHealTarget(targ->enemy);
				}

				// clean up self
				targ->monsterInfo.aiflags &= ~AI_MEDIC;
			}

			targ->enemy = attacker;
			if (!(targ->monsterInfo.aiflags & AI_DUCKED))
				FoundTarget(targ);
		}
		return;
	}

	if (attacker->enemy == targ // if they *meant* to shoot us, then shoot back
		// it's the same base (walk/swim/fly) type and both don't ignore shots,
		// get mad at them
		|| (((targ->flags & (FL_FLY | FL_SWIM)) == (attacker->flags & (FL_FLY | FL_SWIM))) &&
			(strcmp(targ->className, attacker->className) != 0) && !(attacker->monsterInfo.aiflags & AI_IGNORE_SHOTS) &&
			!(targ->monsterInfo.aiflags & AI_IGNORE_SHOTS))) {
		if (targ->enemy != attacker) {
			// [Paril-KEX]
			if ((targ->svFlags & SVF_MONSTER) && targ->monsterInfo.aiflags & AI_MEDIC) {
				if (targ->enemy && targ->enemy->inUse && (targ->enemy->svFlags & SVF_MONSTER)) { // god, I hope so
					M_CleanupHealTarget(targ->enemy);
				}

				// clean up self
				targ->monsterInfo.aiflags &= ~AI_MEDIC;
			}

			if (targ->enemy && targ->enemy->client)
				targ->oldEnemy = targ->enemy;
			targ->enemy = attacker;
			if (!(targ->monsterInfo.aiflags & AI_DUCKED))
				FoundTarget(targ);
		}
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker->enemy && attacker->enemy != targ && targ->enemy != attacker->enemy) {
		if (targ->enemy != attacker->enemy) {
			// [Paril-KEX]
			if ((targ->svFlags & SVF_MONSTER) && targ->monsterInfo.aiflags & AI_MEDIC) {
				if (targ->enemy && targ->enemy->inUse && (targ->enemy->svFlags & SVF_MONSTER)) // god, I hope so
				{
					M_CleanupHealTarget(targ->enemy);
				}

				// clean up self
				targ->monsterInfo.aiflags &= ~AI_MEDIC;
			}

			if (targ->enemy && targ->enemy->client)
				targ->oldEnemy = targ->enemy;
			targ->enemy = attacker->enemy;
			if (!(targ->monsterInfo.aiflags & AI_DUCKED))
				FoundTarget(targ);
		}
	}
}

// check if the two given entities are on the same team
bool OnSameTeam(gentity_t *ent1, gentity_t *ent2) {
	// monsters are never on our team atm
	if (!ent1->client || !ent2->client)
		return false;

	// we're never on our own team
	else if (ent1 == ent2)
		return false;

	if (!ent1->client || !ent2->client)
		return false;

	if (g_quadhog->integer) {
		if (ent1->client->pu_time_quad > level.time || ent2->client->pu_time_quad > level.time)
			return false;
		return true;
	}

	// [Paril-KEX] coop 'team' support
	if (CooperativeModeOn())
		return ent1->client && ent2->client;
	else if (Teams() && ent1->client && ent2->client) {
		if (ent1->client->sess.team == ent2->client->sess.team)
			return true;
	}

	return false;
}

// check if the two entities are on a team and that
// they wouldn't damage each other
bool CheckTeamDamage(gentity_t *targ, gentity_t *attacker) {
	// always damage teammates if friendly fire is enabled
	if (g_friendly_fire->integer)
		return false;

	return OnSameTeam(targ, attacker);
}

void Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec3_t &dir, const vec3_t &point,
	const vec3_t &normal, int damage, int knockback, damageflags_t dFlags, mod_t mod) {
	gclient_t *client;
	int			take, save;
	int			asave, psave;
	int			te_sparks;
	bool		sphere_notified;

	if (!targ->takeDamage)
		return;

	if (g_instagib->integer && attacker->client && targ->client) {
		// [Kex] always kill no matter what on instagib
		damage = 9999;
	}
	sphere_notified = false;

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((targ != attacker) && !(dFlags & DAMAGE_NO_PROTECTION)) {
		// mark as friendly fire
		if (OnSameTeam(targ, attacker)) {
			mod.friendly_fire = true;

			// if we're not a nuke & friendly fire is disabled, just kill the damage
			if (!g_friendly_fire->integer && (mod.id != MOD_NUKE)) {
				damage = 0;
			}
		}
	}

	// easy mode takes half damage
	if (skill->integer == 0 && !deathmatch->integer && targ->client && damage) {
		damage *= 0.5;
		if (damage < 1)
			damage = 1;
	}

	if ((targ->svFlags & SVF_MONSTER) != 0) {
		damage *= ai_damage_scale->integer;
	} else {
		damage *= g_damage_scale->integer;
	}

	client = targ->client;

	// defender sphere takes half damage
	if (damage && (client) && (client->ownedSphere) && (client->ownedSphere->spawnflags == SF_SPHERE_DEFENDER)) {
		damage *= 0.5;
		if (damage < 1)
			damage = 1;
	}

	if (dFlags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	// bonus damage for surprising a monster
	if (!(dFlags & DAMAGE_RADIUS) && (targ->svFlags & SVF_MONSTER) && (attacker->client) &&
		(!targ->enemy || targ->monsterInfo.surprise_time == level.time) && (targ->health > 0)) {
		damage *= 2;
		targ->monsterInfo.surprise_time = level.time;
	}

	// apply knockback cap in Q3
	if (RS(RS_Q3A)) {
		knockback = damage;
		if (knockback > 200)
			knockback = 200;
	}

/*freeze*/
	if (GT(GT_FREEZE) && client && client->eliminated)
		knockback *= 2;
	else
/*freeze*/
		if ((targ->flags & FL_NO_KNOCKBACK) ||
			((targ->flags & FL_ALIVE_KNOCKBACK_ONLY) && (!targ->deadFlag || targ->dead_time != level.time)))
			knockback = 0;

	// figure momentum add, even if the damage won't be taken
	if (!(dFlags & DAMAGE_NO_KNOCKBACK)) {
		if ((knockback) && (targ->moveType != MOVETYPE_NONE) && (targ->moveType != MOVETYPE_BOUNCE) &&
			(targ->moveType != MOVETYPE_PUSH) && (targ->moveType != MOVETYPE_STOP)) {
			vec3_t normalized = dir.normalized();
			int32_t mass = std::clamp(targ->mass, 50, targ->mass);
			vec3_t kvel;
			float factor = targ->client && attacker == targ ? 1200.0f : 1000.0f;		//just 1000.0f in q3
			
			kvel = normalized * (factor * (float)knockback / (float)mass); // the rocket jump hack...
			kvel *= g_knockback_scale->value;

			// arena gives a bit more knockback
			if (GTF(GTF_ARENA))
				kvel *= 1.125f;

			targ->velocity += kvel;

			// set the timer so that the other client can't cancel
			// out the movement immediately
			if (targ->client && !targ->client->ps.pmove.pm_time) {
				int t = knockback * 2;

				targ->client->ps.pmove.pm_time = std::clamp(t, 50, 200);
				targ->client->ps.pmove.pm_time |= PMF_TIME_KNOCKBACK;
			}
		}
	}
#if 0
	if (targ != attacker && attacker->client && !OnSameTeam(targ, attacker)) {
		if ((!inflictor->skip)) {	// && (dFlags & DAMAGE_STAT_ONCE)) || !(dFlags & DAMAGE_STAT_ONCE)) {

			attacker ->client->sess.match.totalHits++;
			attacker->client->sess.match.totalHitsPerWeapon[modr[mod.id].weapon]++;
			inflictor->skip = true;
		}
	}
#endif
	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if (targ == attacker)
		damage *= 0.5;

	if (damage < 1)
		damage = 1;

	take = damage;
	save = 0;

	// check for completely getting out of the damage
	if (!(dFlags & DAMAGE_NO_PROTECTION)) {
		if (CombatIsDisabled() || GT(GT_BALL)) {
			take = 0;
			save = damage;
		}

/*freeze*/
#if 0
		if (GT(GT_FREEZE) && playerDamage(targ, attacker, damage)) {
			take = 0;
			save = damage;
			SpawnDamage(te_sparks, point, normal, save);
			return;
		}
#endif
/*freeze*/

		// instagib railgun splash never inflicts damage
		if (mod.id == MOD_RAILGUN_SPLASH) {
			take = 0;
			save = damage;
		}

		// protection does not take splash damage
		if (client && client->pu_time_battlesuit > level.time && (dFlags & DAMAGE_RADIUS)) {
			gi.sound(targ, CHAN_AUX, gi.soundindex("items/protect3.wav"), 1, ATTN_NORM, 0);
			take = 0;
			save = damage;
		}

		// check for godmode
		if (targ->flags & FL_GODMODE) {
			take = 0;
			save = damage;
			SpawnDamage(te_sparks, point, normal, save);
		}

		// check for monster invincibility
		if ((targ->svFlags & SVF_MONSTER) && targ->monsterInfo.invincibility_time > level.time) {
			if (targ->pain_debounce_time < level.time) {
				gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
				targ->pain_debounce_time = level.time + 2_sec;
			}
			take = 0;
			save = damage;
		}
	}

	// check for getting out of self damage
	if (targ == attacker && deathmatch->integer && g_dm_no_self_damage->integer) {
		take = 0;
		save = damage;
	}

	if (g_vampiric_damage->integer && targ->health > 0 && attacker != targ && !OnSameTeam(targ, attacker) && take > 0) {
		int vtake = take;
		int hmax = clamp(g_vampiric_health_max->integer, 100, 9999);

		if (vtake > targ->health)
			vtake = targ->health;

		vtake = max(1, (int)ceil((float)vtake * g_vampiric_percentile->value));

		if (attacker->health < hmax)
			attacker->health += vtake;
		if (attacker->health > hmax)
			attacker->health = hmax;
	}

	// team armor protect
	if (Teams() && targ->client && attacker->client &&
			targ->client->sess.team == attacker->client->sess.team && targ != attacker &&
			g_teamplay_armor_protect->integer) {
		psave = asave = 0;
	} else {
		if (targ == attacker && GTF(GTF_ARENA) && !g_arena_dmg_armor->integer) {
			take = 0;
			save = damage;
		} else {
			psave = CheckPowerArmor(targ, point, normal, take, dFlags);
			take -= psave;

			asave = CheckArmor(targ, point, normal, take, te_sparks, dFlags);
			take -= asave;
		}
	}

	// treat cheat/powerup savings the same as armor
	asave += save;

	if (!(dFlags & DAMAGE_NO_PROTECTION)) {
		if (targ == attacker && GTF(GTF_ARENA)) {
			take = 0;
			save = 0;	// damage;
		}

		// check for spawn protection
		if (take && client && client->pu_time_spawn_protection > level.time) {
			gi.sound(targ, CHAN_AUX, gi.soundindex("items/protect3.wav"), 1, ATTN_NORM, 0);
			take = 0;
			client->pu_time_spawn_protection_blip = level.time + 100_ms;
		}

		// check for battle suit powerup
		if (take && client && client->pu_time_battlesuit > level.time) {
			gi.sound(targ, CHAN_AUX, gi.soundindex("items/protect3.wav"), 1, ATTN_NORM, 0);
			take = ceil(take/2);
		}
	}

	// resistance tech
	take = Tech_ApplyDisruptorShield(targ, take);

	CTF_CheckHurtCarrier(targ, attacker);

	// this option will do damage both to the armor and person. originally for DPU rounds
	if ((dFlags & DAMAGE_DESTROY_ARMOR)) {
		if (!(targ->flags & FL_GODMODE) && !(dFlags & DAMAGE_NO_PROTECTION) &&
				!(client && client->pu_time_battlesuit > level.time)) {
			take = damage;
		}
	}

	if (targ != attacker && attacker->client && targ->health > 0) {
		int stat_take = take;
		if (stat_take > targ->health)
			stat_take = targ->health;

		// arena player scoring: 1 point per 100 damage dealt to opponents, capped to 0 health
		if (GTF(GTF_ARENA) && !OnSameTeam(targ, attacker)) {
			attacker->client->pers.dmg_scorer += stat_take + psave + asave;

			if (attacker->client->pers.dmg_scorer >= 100) {
				int32_t score_add = floor(attacker->client->pers.dmg_scorer / 100);
				attacker->client->pers.dmg_scorer -= score_add * 100;

				G_AdjustPlayerScore(attacker->client, score_add, false, 0);
			}
		}

		// team damage checks and warnings
		if (OnSameTeam(targ, attacker)) {
			attacker->client->pers.dmg_team += take + psave + asave;

			if (attacker->client->pers.dmg_team >= 100) {
				int32_t score_add = floor(attacker->client->pers.dmg_team / 100);
				attacker->client->pers.dmg_team -= score_add * 100;

				gi.LocClient_Print(attacker, PRINT_CENTER, "You are on {} Team,\nstop attacking your team mates!\n", Teams_TeamName(attacker->client->sess.team));
			}
		}

		// [Paril-KEX] player hit markers
		if (!((targ->svFlags & SVF_DEADMONSTER) || (targ->flags & FL_NO_DAMAGE_EFFECTS)) && mod.id != MOD_TARGET_LASER) {
			attacker->client->ps.stats[STAT_HIT_MARKER] += take + psave + asave;
		}
		attacker->client->sess.match.totalDmgDealt += stat_take + psave + asave;
		attacker->client->sess.match.modTotalDmgD[mod.id] += stat_take + psave + asave;
		if (!inflictor || (inflictor && !inflictor->skip)) {

			attacker->client->sess.match.totalHits++;
			attacker->client->sess.match.totalHitsPerWeapon[modr[mod.id].weapon]++;

			// skip MG and CG as it mucks things up for those
			if (inflictor && mod.id != MOD_MACHINEGUN && mod.id != MOD_CHAINGUN)
				inflictor->skip = true;
		}
		
		if (targ->client) {
			targ->client->sess.match.totalDmgReceived += stat_take + psave + asave;
			targ->client->sess.match.modTotalDmgR[mod.id] += stat_take + psave + asave;
		}
	}

	// do the damage
	if (take) {
		if (!(targ->flags & FL_NO_DAMAGE_EFFECTS)) {
			if (targ->flags & FL_MECHANICAL)
				SpawnDamage(TE_ELECTRIC_SPARKS, point, normal, take);
			else if ((targ->svFlags & SVF_MONSTER) || (client)) {
				if (strcmp(targ->className, "monster_gekk") == 0)
					SpawnDamage(TE_GREENBLOOD, point, normal, take);
				else if (mod.id == MOD_CHAINFIST)
					SpawnDamage(TE_MOREBLOOD, point, normal, 255);
				else
					SpawnDamage(TE_BLOOD, point, normal, take);
			} else
				SpawnDamage(te_sparks, point, normal, take);
		}

		if (!CombatIsDisabled()) {
			targ->health -= take;

			if (targ->client && targ->client->pers.health_bonus) {
				targ->client->pers.health_bonus -= take;
				if (targ->client->pers.health_bonus < 0)
					targ->client->pers.health_bonus = 0;

				if (targ->health <= 0 && targ->client->pers.health_bonus > 0)
					targ->client->pers.health_bonus = 0;
			}
		}

		if ((targ->flags & FL_IMMORTAL) && targ->health <= 0)
			targ->health = 1;

		// spheres need to know who to shoot at
		if (client && client->ownedSphere) {
			sphere_notified = true;
			if (client->ownedSphere->pain)
				client->ownedSphere->pain(client->ownedSphere, attacker, 0, 0, mod);
		}

		if (targ->health <= 0) {
			if ((targ->svFlags & SVF_MONSTER) || (client)) {
				targ->flags |= FL_ALIVE_KNOCKBACK_ONLY;
				targ->dead_time = level.time;

				// don't gib in freeze tag unless thawing
				if (GT(GT_FREEZE) && mod.id != MOD_THAW && targ->health <= targ->gibHealth) {
					targ->health = targ->gibHealth + 1;
				}
			}
			targ->monsterInfo.damageBlood += take;
			targ->monsterInfo.damage_attacker = attacker;
			targ->monsterInfo.damage_inflictor = inflictor;
			targ->monsterInfo.damageFrom = point;
			targ->monsterInfo.damage_mod = mod;
			targ->monsterInfo.damageKnockback += knockback;
			Killed(targ, inflictor, attacker, take, point, mod);
			return;
		}
	}

	// spheres need to know who to shoot at
	if (!sphere_notified) {
		if (client && client->ownedSphere) {
			sphere_notified = true;
			if (client->ownedSphere->pain)
				client->ownedSphere->pain(client->ownedSphere, attacker, 0, 0, mod);
		}
	}

	if (targ->client)
		targ->client->last_attacker_time = level.time;

	if (targ->svFlags & SVF_MONSTER) {
		if (damage > 0) {
			M_ReactToDamage(targ, attacker, inflictor);

			targ->monsterInfo.damage_attacker = attacker;
			targ->monsterInfo.damage_inflictor = inflictor;
			targ->monsterInfo.damageBlood += take;
			targ->monsterInfo.damageFrom = point;
			targ->monsterInfo.damage_mod = mod;
			targ->monsterInfo.damageKnockback += knockback;
		}

		if (targ->monsterInfo.setskin)
			targ->monsterInfo.setskin(targ);
	} else if (take && targ->pain)
		targ->pain(targ, attacker, (float)knockback, take, mod);

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client) {
		client->damagePArmor += psave;
		client->damageArmor += asave;
		client->damageBlood += take;
		client->damageKnockback += knockback;
		client->damageFrom = point;
		client->last_damage_time = level.time + COOP_DAMAGE_RESPAWN_TIME;

		if (!(dFlags & DAMAGE_NO_INDICATOR) && inflictor != world && attacker != world && (take || psave || asave)) {
			damage_indicator_t *indicator = nullptr;
			size_t i;

			for (i = 0; i < client->numDamageIndicators; i++) {
				if ((point - client->damageIndicators[i].from).length() < 32.f) {
					indicator = &client->damageIndicators[i];
					break;
				}
			}

			if (!indicator && i != MAX_DAMAGE_INDICATORS) {
				indicator = &client->damageIndicators[i];
				// for projectile direct hits, use the attacker; otherwise
				// use the inflictor (rocket splash should point to the rocket)
				indicator->from = (dFlags & DAMAGE_RADIUS) ? inflictor->s.origin : attacker->s.origin;
				indicator->health = indicator->armor = indicator->power = 0;
				client->numDamageIndicators++;
			}

			if (indicator) {
				indicator->health += take;
				indicator->power += psave;
				indicator->armor += asave;
			}
		}
	}
}

/*
============
RadiusDamage
============
*/
#if 0
bool RadiusDamage(gentity_t *inflictor, gentity_t *attacker, float damage, gentity_t *ignore, float radius, damageflags_t dFlags, mod_t mod) {
	float	 points;
	gentity_t *ent = nullptr;
	vec3_t	 v, dir, origin;

	origin = inflictor->linked ? ((inflictor->absMax + inflictor->absMin) * 0.5f) : inflictor->s.origin;

	while ((ent = FindRadius(ent, origin, radius)) != nullptr) {
		if (ent == ignore)
			continue;
		if (!ent->takeDamage)
			continue;

		if (ent->solid == SOLID_BSP && ent->linked)
			v = closest_point_to_box(origin, ent->absMin, ent->absMax);
		else {
			v = ent->mins + ent->maxs;
			v = ent->s.origin + (v * 0.5f);
		}
		v = origin - v;
		points = damage - 0.5f * v.length();
		if (ent == attacker)
			points *= 0.5f;
		if (points > 0) {
			if (CanDamage(ent, inflictor)) {
				dir = (ent->s.origin - origin).normalized();
				// [Paril-KEX] use closest point on bbox to explosion position
				// to spawn damage effect

				float kb = points;
				if (mod.id == MOD_HYPERBLASTER)
					kb *= 5;

				Damage(ent, inflictor, attacker, dir, closest_point_to_box(origin, ent->absMin, ent->absMax), dir, (int)points, (int)kb, dFlags | DAMAGE_RADIUS, mod);
			}
		}
	}
}
#endif

bool RadiusDamage(gentity_t *inflictor, gentity_t *attacker, float damage, gentity_t *ignore, float radius, damageflags_t dFlags, mod_t mod) {
	float	 points;
	gentity_t *ent = nullptr;
	vec3_t	 v, dir, origin;
	bool	hitClient = false;

	if (radius < 1)
		radius = 1;

	origin = inflictor->linked ? ((inflictor->absMax + inflictor->absMin) * 0.5f) : inflictor->s.origin;

	while ((ent = FindRadius(ent, origin, radius)) != nullptr) {
		if (ent == ignore)
			continue;
		if (!ent->takeDamage)
			continue;

		if (ent->solid == SOLID_BSP && ent->linked)
			v = closest_point_to_box(origin, ent->absMin, ent->absMax);
		else {
			v = ent->mins + ent->maxs;
			v = ent->s.origin + (v * 0.5f);
		}
		v = origin - v;
		if (v.length() >= radius)
			continue;

		//points = damage - 0.5f * v.length();
		points = damage * (1.0 - v.length() / radius);

		if (points > 0) {
			if (CanDamage(ent, inflictor)) {
				if (LogAccuracyHit(ent, attacker))
					hitClient = true;
				dir = (ent->s.origin - origin).normalized();

				// push the center of mass higher than the origin so players
				// get knocked into the air more
				dir[2] += 24.0f;

				Damage(ent, inflictor, attacker, dir, closest_point_to_box(origin, ent->absMin, ent->absMax), dir, (int)points, (int)points, dFlags | DAMAGE_RADIUS, mod);
			}
		}
	}
	return hitClient;
}

/*
============
M_CleanupHealTarget

Clean up heal targets for medic
============
*/
void M_SetEffects(gentity_t *self);
void M_CleanupHealTarget(gentity_t *ent) {
	ent->monsterInfo.healer = nullptr;
	ent->takeDamage = true;
	ent->monsterInfo.aiflags &= ~AI_RESURRECTING;
	M_SetEffects(ent);
}

/*
============
G_RadiusNukeDamage

Like RadiusDamage, but ignores walls (skips CanDamage check, among others)
// up to KILLZONE radius, do 10,000 points
// after that, do damage linearly out to KILLZONE2 radius
============
*/

void G_RadiusNukeDamage(gentity_t *inflictor, gentity_t *attacker, float damage, gentity_t *ignore, float radius, mod_t mod) {
	float	 points;
	gentity_t *ent = nullptr;
	vec3_t	 v;
	vec3_t	 dir;
	float	 len;
	float	 killzone, killzone2;
	trace_t	 tr;
	float	 dist;

	killzone = radius;
	killzone2 = radius * 2.0f;

	while ((ent = FindRadius(ent, inflictor->s.origin, killzone2)) != nullptr) {
		// ignore nobody
		if (ent == ignore)
			continue;
		if (!ent->takeDamage)
			continue;
		if (!ent->inUse)
			continue;
		if (!(ent->client || (ent->svFlags & SVF_MONSTER) || (ent->flags & FL_DAMAGEABLE)))
			continue;

		v = ent->mins + ent->maxs;
		v = ent->s.origin + (v * 0.5f);
		v = inflictor->s.origin - v;
		len = v.length();
		if (len <= killzone) {
			if (ent->client)
				ent->flags |= FL_NOGIB;
			points = 10000;
		} else if (len <= killzone2)
			points = (damage / killzone) * (killzone2 - len);
		else
			points = 0;

		if (points > 0) {
			if (ent->client)
				ent->client->nuke_time = level.time + 2_sec;
			dir = ent->s.origin - inflictor->s.origin;
			Damage(ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
		}
	}
	ent = g_entities + 1; // skip the worldspawn
	// cycle through players
	while (ent) {
		if ((ent->client) && (ent->client->nuke_time != level.time + 2_sec) && (ent->inUse)) {
			tr = gi.traceline(inflictor->s.origin, ent->s.origin, inflictor, MASK_SOLID);
			if (tr.fraction == 1.0f)
				ent->client->nuke_time = level.time + 2_sec;
			else {
				dist = realrange(ent, inflictor);
				if (dist < 2048)
					ent->client->nuke_time = max(ent->client->nuke_time, level.time + 1.5_sec);
				else
					ent->client->nuke_time = max(ent->client->nuke_time, level.time + 1_sec);
			}
			ent++;
		} else
			ent = nullptr;
	}
}
