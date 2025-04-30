// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

TURRET

==============================================================================
*/

#include "../g_local.h"
#include "m_turret.h"

constexpr spawnflags_t SPAWNFLAG_TURRET_BLASTER = 0x0008_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TURRET_MACHINEGUN = 0x0010_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TURRET_ROCKET = 0x0020_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TURRET_HEATBEAM = 0x0040_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TURRET_WEAPONCHOICE = SPAWNFLAG_TURRET_HEATBEAM | SPAWNFLAG_TURRET_ROCKET | SPAWNFLAG_TURRET_MACHINEGUN | SPAWNFLAG_TURRET_BLASTER;
constexpr spawnflags_t SPAWNFLAG_TURRET_WALL_UNIT = 0x0080_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TURRET_NO_LASERSIGHT = 18_spawnflag_bit;

bool FindTarget(gentity_t *self);

void TurretAim(gentity_t *self);
void turret_ready_gun(gentity_t *self);
void turret_run(gentity_t *self);

extern const mmove_t turret_move_fire;
extern const mmove_t turret_move_fire_blind;

static cached_soundindex sound_moved, sound_moving;

void TurretAim(gentity_t *self) {
	vec3_t end, dir;
	vec3_t ang;
	float  move, idealPitch, idealYaw, current, speed;
	int	   orientation;

	if (!self->enemy || self->enemy == world) {
		if (!FindTarget(self))
			return;
	}

	// if turret is still in inactive mode, ready the gun, but don't aim
	if (self->s.frame < FRAME_active01) {
		turret_ready_gun(self);
		return;
	}
	// if turret is still readying, don't aim.
	if (self->s.frame < FRAME_run01)
		return;

	// PMM - blindfire aiming here
	if (self->monsterInfo.active_move == &turret_move_fire_blind) {
		end = self->monsterInfo.blind_fire_target;
		if (self->enemy->s.origin[2] < self->monsterInfo.blind_fire_target[2])
			end[2] += self->enemy->viewHeight + 10;
		else
			end[2] += self->enemy->mins[2] - 10;
	} else {
		end = self->enemy->s.origin;
		if (self->enemy->client)
			end[2] += self->enemy->viewHeight;
	}

	dir = end - self->s.origin;
	ang = vectoangles(dir);

	//
	// Clamp first
	//

	idealPitch = ang[PITCH];
	idealYaw = ang[YAW];

	orientation = (int)self->offset[1];
	switch (orientation) {
	case -1: // up		pitch: 0 to 90
		if (idealPitch < -90)
			idealPitch += 360;
		if (idealPitch > -5)
			idealPitch = -5;
		break;
	case -2: // down		pitch: -180 to -360
		if (idealPitch > -90)
			idealPitch -= 360;
		if (idealPitch < -355)
			idealPitch = -355;
		else if (idealPitch > -185)
			idealPitch = -185;
		break;
	case 0: // +X		pitch: 0 to -90, -270 to -360 (or 0 to 90)
		if (idealPitch < -180)
			idealPitch += 360;

		if (idealPitch > 85)
			idealPitch = 85;
		else if (idealPitch < -85)
			idealPitch = -85;

		//			yaw: 270 to 360, 0 to 90
		//			yaw: -90 to 90 (270-360 == -90-0)
		if (idealYaw > 180)
			idealYaw -= 360;
		if (idealYaw > 85)
			idealYaw = 85;
		else if (idealYaw < -85)
			idealYaw = -85;
		break;
	case 90: // +Y	pitch: 0 to 90, -270 to -360 (or 0 to 90)
		if (idealPitch < -180)
			idealPitch += 360;

		if (idealPitch > 85)
			idealPitch = 85;
		else if (idealPitch < -85)
			idealPitch = -85;

		//			yaw: 0 to 180
		if (idealYaw > 270)
			idealYaw -= 360;
		if (idealYaw > 175)
			idealYaw = 175;
		else if (idealYaw < 5)
			idealYaw = 5;

		break;
	case 180: // -X	pitch: 0 to 90, -270 to -360 (or 0 to 90)
		if (idealPitch < -180)
			idealPitch += 360;

		if (idealPitch > 85)
			idealPitch = 85;
		else if (idealPitch < -85)
			idealPitch = -85;

		//			yaw: 90 to 270
		if (idealYaw > 265)
			idealYaw = 265;
		else if (idealYaw < 95)
			idealYaw = 95;

		break;
	case 270: // -Y	pitch: 0 to 90, -270 to -360 (or 0 to 90)
		if (idealPitch < -180)
			idealPitch += 360;

		if (idealPitch > 85)
			idealPitch = 85;
		else if (idealPitch < -85)
			idealPitch = -85;

		//			yaw: 180 to 360
		if (idealYaw < 90)
			idealYaw += 360;
		if (idealYaw > 355)
			idealYaw = 355;
		else if (idealYaw < 185)
			idealYaw = 185;
		break;
	}

	//
	// adjust pitch
	//
	current = self->s.angles[PITCH];
	speed = self->yaw_speed / (gi.tick_rate / 10);

	if (idealPitch != current) {
		move = idealPitch - current;

		while (move >= 360)
			move -= 360;
		if (move >= 90) {
			move = move - 360;
		}

		while (move <= -360)
			move += 360;
		if (move <= -90) {
			move = move + 360;
		}

		if (move > 0) {
			if (move > speed)
				move = speed;
		} else {
			if (move < -speed)
				move = -speed;
		}

		self->s.angles[PITCH] = anglemod(current + move);
	}

	//
	// adjust yaw
	//
	current = self->s.angles[YAW];

	if (idealYaw != current) {
		move = idealYaw - current;

		if (move >= 180)
			move = move - 360;
		if (move <= -180)
			move = move + 360;

		if (move > 0) {
			if (move > speed)
				move = speed;
		} else {
			if (move < -speed)
				move = -speed;
		}

		self->s.angles[YAW] = anglemod(current + move);
	}

	if (self->spawnflags.has(SPAWNFLAG_TURRET_NO_LASERSIGHT))
		return;

	// Paril: improved turrets; draw lasersight
	if (!self->target_ent) {
		self->target_ent = Spawn();
		self->target_ent->s.modelindex = MODELINDEX_WORLD;
		self->target_ent->s.renderfx = RF_BEAM;
		self->target_ent->s.frame = 1;
		self->target_ent->s.skinnum = 0xf0f0f0f0;
		self->target_ent->className = "turret_lasersight";
		self->target_ent->s.origin = self->s.origin;
	}

	vec3_t forward;
	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	end = self->s.origin + (forward * 8192);
	trace_t tr = gi.traceline(self->s.origin, end, self, MASK_SOLID);

	float scan_range = 64.f;

	if (visible(self, self->enemy))
		scan_range = 12.f;

	tr.endpos[0] += sinf(level.time.seconds() + self->s.number) * scan_range;
	tr.endpos[1] += cosf((level.time.seconds() - self->s.number) * 3.f) * scan_range;
	tr.endpos[2] += sinf((level.time.seconds() - self->s.number) * 2.5f) * scan_range;

	forward = tr.endpos - self->s.origin;
	forward.normalize();

	end = self->s.origin + (forward * 8192);
	tr = gi.traceline(self->s.origin, end, self, MASK_SOLID);

	self->target_ent->s.old_origin = tr.endpos;
	gi.linkentity(self->target_ent);
}

MONSTERINFO_SIGHT(turret_sight) (gentity_t *self, gentity_t *other) -> void {}

MONSTERINFO_SEARCH(turret_search) (gentity_t *self) -> void {}

mframe_t turret_frames_stand[] = {
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(turret_move_stand) = { FRAME_stand01, FRAME_stand02, turret_frames_stand, nullptr };

MONSTERINFO_STAND(turret_stand) (gentity_t *self) -> void {
	M_SetAnimation(self, &turret_move_stand);
	if (self->target_ent) {
		FreeEntity(self->target_ent);
		self->target_ent = nullptr;
	}
}

mframe_t turret_frames_ready_gun[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand }
};
MMOVE_T(turret_move_ready_gun) = { FRAME_active01, FRAME_run01, turret_frames_ready_gun, turret_run };

void turret_ready_gun(gentity_t *self) {
	if (self->monsterInfo.active_move != &turret_move_ready_gun) {
		M_SetAnimation(self, &turret_move_ready_gun);
		self->monsterInfo.weapon_sound = sound_moving;
	}
}

mframe_t turret_frames_seek[] = {
	{ ai_walk, 0, TurretAim },
	{ ai_walk, 0, TurretAim }
};
MMOVE_T(turret_move_seek) = { FRAME_run01, FRAME_run02, turret_frames_seek, nullptr };

MONSTERINFO_WALK(turret_walk) (gentity_t *self) -> void {
	if (self->s.frame < FRAME_run01)
		turret_ready_gun(self);
	else
		M_SetAnimation(self, &turret_move_seek);
}

mframe_t turret_frames_run[] = {
	{ ai_run, 0, TurretAim },
	{ ai_run, 0, TurretAim }
};
MMOVE_T(turret_move_run) = { FRAME_run01, FRAME_run02, turret_frames_run, turret_run };

MONSTERINFO_RUN(turret_run) (gentity_t *self) -> void {
	if (self->s.frame < FRAME_run01)
		turret_ready_gun(self);
	else {
		self->monsterInfo.aiflags |= AI_HIGH_TICK_RATE;
		M_SetAnimation(self, &turret_move_run);

		if (self->monsterInfo.weapon_sound) {
			self->monsterInfo.weapon_sound = 0;
			gi.sound(self, CHAN_WEAPON, sound_moved, 1.0f, ATTN_NORM, 0.f);
		}
	}
}

// **********************
//  ATTACK
// **********************

constexpr int32_t TURRET_BLASTER_DAMAGE = 8;
constexpr int32_t TURRET_BULLET_DAMAGE = 2;
// unused
// constexpr int32_t TURRET_HEAT_DAMAGE	= 4;

static void TurretFire(gentity_t *self) {
	vec3_t	forward;
	vec3_t	start, end, dir;
	float	dist, chance;
	trace_t trace;
	int		rocketSpeed;

	TurretAim(self);

	if (!self->enemy || !self->enemy->inUse)
		return;

	if (self->monsterInfo.aiflags & AI_LOST_SIGHT)
		end = self->monsterInfo.blind_fire_target;
	else
		end = self->enemy->s.origin;
	dir = end - self->s.origin;
	dir.normalize();
	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	chance = dir.dot(forward);
	if (chance < 0.98f)
		return;

	chance = frandom();

	if (self->spawnflags.has(SPAWNFLAG_TURRET_ROCKET))
		rocketSpeed = 650;
	else if (self->spawnflags.has(SPAWNFLAG_TURRET_BLASTER))
		rocketSpeed = 800;
	else
		rocketSpeed = 0;

	if (self->spawnflags.has(SPAWNFLAG_TURRET_MACHINEGUN) || visible(self, self->enemy)) {
		start = self->s.origin;

		// aim for the head.
		if (!(self->monsterInfo.aiflags & AI_LOST_SIGHT)) {
			if ((self->enemy) && (self->enemy->client))
				end[2] += self->enemy->viewHeight;
			else
				end[2] += 22;
		}

		dir = end - start;
		dist = dir.length();

		// check for predictive fire
		// Paril: adjusted to be a bit more fair
		if (!(self->monsterInfo.aiflags & AI_LOST_SIGHT)) {
			// on harder difficulties, randomly fire directly at enemy
			// more often; makes them more unpredictable
			if (self->spawnflags.has(SPAWNFLAG_TURRET_MACHINEGUN))
				PredictAim(self, self->enemy, start, 0, true, 0.3f, &dir, nullptr);
			else if (frandom() < skill->integer / 5.f)
				PredictAim(self, self->enemy, start, (float)rocketSpeed, true, (frandom(3.f - skill->integer) / 3.f) - frandom(0.05f * (3.f - skill->integer)), &dir, nullptr);
		}

		dir.normalize();
		trace = gi.traceline(start, end, self, MASK_PROJECTILE);
		if (trace.ent == self->enemy || trace.ent == world) {
			if (self->spawnflags.has(SPAWNFLAG_TURRET_BLASTER))
				monster_fire_blaster(self, start, dir, TURRET_BLASTER_DAMAGE, rocketSpeed, MZ2_TURRET_BLASTER, EF_BLASTER);
			else if (self->spawnflags.has(SPAWNFLAG_TURRET_MACHINEGUN)) {
				if (!(self->monsterInfo.aiflags & AI_HOLD_FRAME)) {
					self->monsterInfo.aiflags |= AI_HOLD_FRAME;
					self->monsterInfo.duck_wait_time = level.time + 2_sec + gtime_t::from_sec(frandom(skill->value));
					self->monsterInfo.next_duck_time = level.time + 1_sec;
					gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_NORM, 0);
				} else {
					if (self->monsterInfo.next_duck_time < level.time &&
						self->monsterInfo.melee_debounce_time <= level.time) {
						monster_fire_bullet(self, start, dir, TURRET_BULLET_DAMAGE, 0, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_TURRET_MACHINEGUN);
						self->monsterInfo.melee_debounce_time = level.time + 10_hz;
					}

					if (self->monsterInfo.duck_wait_time < level.time)
						self->monsterInfo.aiflags &= ~AI_HOLD_FRAME;
				}
			} else if (self->spawnflags.has(SPAWNFLAG_TURRET_ROCKET)) {
				if (dist * trace.fraction > 72)
					monster_fire_rocket(self, start, dir, 40, rocketSpeed, MZ2_TURRET_ROCKET);
			}
		}
	}
}

static void TurretFireBlind(gentity_t *self) {
	vec3_t forward;
	vec3_t start, end, dir;
	float  chance;
	int	   rocketSpeed = 550;

	TurretAim(self);

	if (!self->enemy || !self->enemy->inUse)
		return;

	dir = self->monsterInfo.blind_fire_target - self->s.origin;
	dir.normalize();
	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	chance = dir.dot(forward);
	if (chance < 0.98f)
		return;

	if (self->spawnflags.has(SPAWNFLAG_TURRET_ROCKET))
		rocketSpeed = 650;
	else if (self->spawnflags.has(SPAWNFLAG_TURRET_BLASTER))
		rocketSpeed = 800;
	else
		rocketSpeed = 0;

	start = self->s.origin;
	end = self->monsterInfo.blind_fire_target;

	if (self->enemy->s.origin[2] < self->monsterInfo.blind_fire_target[2])
		end[2] += self->enemy->viewHeight + 10;
	else
		end[2] += self->enemy->mins[2] - 10;

	dir = end - start;

	dir.normalize();

	if (self->spawnflags.has(SPAWNFLAG_TURRET_BLASTER))
		monster_fire_blaster(self, start, dir, TURRET_BLASTER_DAMAGE, rocketSpeed, MZ2_TURRET_BLASTER, EF_BLASTER);
	else if (self->spawnflags.has(SPAWNFLAG_TURRET_ROCKET))
		monster_fire_rocket(self, start, dir, 40, rocketSpeed, MZ2_TURRET_ROCKET);
}

mframe_t turret_frames_fire[] = {
	{ ai_run, 0, TurretFire },
	{ ai_run, 0, TurretAim },
	{ ai_run, 0, TurretAim },
	{ ai_run, 0, TurretAim }
};
MMOVE_T(turret_move_fire) = { FRAME_pow01, FRAME_pow04, turret_frames_fire, turret_run };

// PMM

// the blind frames need to aim first
mframe_t turret_frames_fire_blind[] = {
	{ ai_run, 0, TurretAim },
	{ ai_run, 0, TurretAim },
	{ ai_run, 0, TurretAim },
	{ ai_run, 0, TurretFireBlind }
};
MMOVE_T(turret_move_fire_blind) = { FRAME_pow01, FRAME_pow04, turret_frames_fire_blind, turret_run };
// pmm

MONSTERINFO_ATTACK(turret_attack) (gentity_t *self) -> void {
	float r, chance;

	if (self->s.frame < FRAME_run01)
		turret_ready_gun(self);
	// PMM
	else if (self->monsterInfo.attack_state != AS_BLIND) {
		M_SetAnimation(self, &turret_move_fire);
	} else {
		// setup shot probabilities
		if (self->monsterInfo.blind_fire_delay < 1_sec)
			chance = 1.0;
		else if (self->monsterInfo.blind_fire_delay < 7.5_sec)
			chance = 0.4f;
		else
			chance = 0.1f;

		r = frandom();

		// minimum of 3 seconds, plus 0-4, after the shots are done - total time should be max less than 7.5
		self->monsterInfo.blind_fire_delay += random_time(3.4_sec, 7.4_sec);
		// don't shoot at the origin
		if (!self->monsterInfo.blind_fire_target)
			return;

		// don't shoot if the dice say not to
		if (r > chance)
			return;

		M_SetAnimation(self, &turret_move_fire_blind);
	}
	// pmm
}

// **********************
//  PAIN
// **********************

static PAIN(turret_pain) (gentity_t *self, gentity_t *other, float kick, int damage, const mod_t &mod) -> void {}

// **********************
//  DEATH
// **********************

static DIE(turret_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	vec3_t	 forward;
	gentity_t *base;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	self->s.origin += (forward * 1);

	ThrowGibs(self, 2, {
		{ 2, "models/objects/debris1/tris.md2", GIB_METALLIC | GIB_DEBRIS }
		});
	ThrowGibs(self, 1, {
		{ 2, "models/objects/debris1/tris.md2", GIB_METALLIC | GIB_DEBRIS }
		});

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PLAIN_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	if (self->teamChain) {
		base = self->teamChain;
		base->solid = SOLID_NOT;
		base->takeDamage = false;
		base->moveType = MOVETYPE_NONE;
		base->teamMaster = base;
		base->teamChain = nullptr;
		base->flags &= ~FL_TEAMSLAVE;
		base->flags |= FL_TEAMMASTER;
		gi.linkentity(base);

		self->teamMaster = self->teamChain = nullptr;
		self->flags &= ~(FL_TEAMSLAVE | FL_TEAMMASTER);
	}

	if (self->target) {
		if (self->enemy && self->enemy->inUse)
			UseTargets(self, self->enemy);
		else
			UseTargets(self, self);
	}

	if (self->target_ent) {
		FreeEntity(self->target_ent);
		self->target_ent = nullptr;
	}

	gentity_t *gib = ThrowGib(self, "models/monsters/turret/tris.md2", damage, GIB_SKINNED | GIB_METALLIC | GIB_HEAD | GIB_DEBRIS, self->s.scale);
	gib->s.frame = 14;
}

// **********************
//  WALL SPAWN
// **********************

static void turret_wall_spawn(gentity_t *turret) {
	gentity_t *ent;
	int		 angle;

	ent = Spawn();
	ent->s.origin = turret->s.origin;
	ent->s.angles = turret->s.angles;

	angle = (int)ent->s.angles[YAW];
	if (ent->s.angles[PITCH] == 90)
		angle = -1;
	else if (ent->s.angles[PITCH] == 270)
		angle = -2;
	switch (angle) {
	case -1:
		ent->mins = { -16, -16, -8 };
		ent->maxs = { 16, 16, 0 };
		break;
	case -2:
		ent->mins = { -16, -16, 0 };
		ent->maxs = { 16, 16, 8 };
		break;
	case 0:
		ent->mins = { -8, -16, -16 };
		ent->maxs = { 0, 16, 16 };
		break;
	case 90:
		ent->mins = { -16, -8, -16 };
		ent->maxs = { 16, 0, 16 };
		break;
	case 180:
		ent->mins = { 0, -16, -16 };
		ent->maxs = { 8, 16, 16 };
		break;
	case 270:
		ent->mins = { -16, 0, -16 };
		ent->maxs = { 16, 8, 16 };
		break;
	}

	ent->moveType = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;

	ent->teamMaster = turret;
	turret->flags |= FL_TEAMMASTER;
	turret->teamMaster = turret;
	turret->teamChain = ent;
	ent->teamChain = nullptr;
	ent->flags |= FL_TEAMSLAVE;
	ent->owner = turret;

	ent->s.modelindex = gi.modelindex("models/monsters/turretbase/tris.md2");

	gi.linkentity(ent);
}

MOVEINFO_ENDFUNC(turret_wake) (gentity_t *ent) -> void {
	// the wall section will call this when it stops moving.
	// just return without doing anything. easiest way to have a null function.
	if (ent->flags & FL_TEAMSLAVE) {
		ent->s.sound = 0;
		return;
	}

	ent->monsterInfo.stand = turret_stand;
	ent->monsterInfo.walk = turret_walk;
	ent->monsterInfo.run = turret_run;
	ent->monsterInfo.dodge = nullptr;
	ent->monsterInfo.attack = turret_attack;
	ent->monsterInfo.melee = nullptr;
	ent->monsterInfo.sight = turret_sight;
	ent->monsterInfo.search = turret_search;
	M_SetAnimation(ent, &turret_move_stand);
	ent->takeDamage = true;
	ent->moveType = MOVETYPE_NONE;
	// prevent counting twice
	ent->monsterInfo.aiflags |= AI_DO_NOT_COUNT;

	gi.linkentity(ent);

	stationarymonster_start(ent);

	if (ent->spawnflags.has(SPAWNFLAG_TURRET_MACHINEGUN)) {
		ent->s.skinnum = 1;
	} else if (ent->spawnflags.has(SPAWNFLAG_TURRET_ROCKET)) {
		ent->s.skinnum = 2;
	}

	// but we do want the death to count
	ent->monsterInfo.aiflags &= ~AI_DO_NOT_COUNT;
}

static USE(turret_activate) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	vec3_t	 endpos;
	vec3_t	 forward = { 0, 0, 0 };
	gentity_t *base;

	self->moveType = MOVETYPE_PUSH;
	if (!self->speed)
		self->speed = 15;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->speed;
	self->moveinfo.decel = self->speed;

	if (self->s.angles[PITCH] == 270) {
		forward = { 0, 0, 1 };
	} else if (self->s.angles[PITCH] == 90) {
		forward = { 0, 0, -1 };
	} else if (self->s.angles[YAW] == 0) {
		forward = { 1, 0, 0 };
	} else if (self->s.angles[YAW] == 90) {
		forward = { 0, 1, 0 };
	} else if (self->s.angles[YAW] == 180) {
		forward = { -1, 0, 0 };
	} else if (self->s.angles[YAW] == 270) {
		forward = { 0, -1, 0 };
	}

	// start up the turret
	endpos = self->s.origin + (forward * 32);
	Move_Calc(self, endpos, turret_wake);

	base = self->teamChain;
	if (base) {
		base->moveType = MOVETYPE_PUSH;
		base->speed = self->speed;
		base->moveinfo.speed = base->speed;
		base->moveinfo.accel = base->speed;
		base->moveinfo.decel = base->speed;

		// start up the wall section
		endpos = self->teamChain->s.origin + (forward * 32);
		Move_Calc(self->teamChain, endpos, turret_wake);

		base->s.sound = sound_moving;
		base->s.loop_attenuation = ATTN_NORM;
	}
}

// checkattack .. ignore range, just attack if available
MONSTERINFO_CHECKATTACK(turret_checkattack) (gentity_t *self) -> bool {
	vec3_t	spot1, spot2;
	float	chance;
	trace_t tr;

	if (self->enemy->health > 0) {
		// see if any entities are in the way of the shot
		spot1 = self->s.origin;
		spot1[2] += self->viewHeight;
		spot2 = self->enemy->s.origin;
		spot2[2] += self->enemy->viewHeight;

		tr = gi.traceline(spot1, spot2, self, CONTENTS_SOLID | CONTENTS_PLAYER | CONTENTS_MONSTER | CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WINDOW);

		// do we have a clear shot?
		if (tr.ent != self->enemy && !(tr.ent->svFlags & SVF_PLAYER)) {
			// we want them to go ahead and shoot at info_notnulls if they can.
			if (self->enemy->solid != SOLID_NOT || tr.fraction < 1.0f) // PGM
			{
				// if we can't see our target, and we're not blocked by a monster, go into blind fire if available
				if ((!(tr.ent->svFlags & SVF_MONSTER)) && (!visible(self, self->enemy))) {
					if ((self->monsterInfo.blindfire) && (self->monsterInfo.blind_fire_delay <= 10_sec)) {
						if (level.time < self->monsterInfo.attack_finished) {
							return false;
						}
						if (level.time < (self->monsterInfo.trail_time + self->monsterInfo.blind_fire_delay)) {
							// wait for our time
							return false;
						} else {
							// make sure we're not going to shoot something we don't want to shoot
							tr = gi.traceline(spot1, self->monsterInfo.blind_fire_target, self, CONTENTS_MONSTER | CONTENTS_PLAYER);
							if (tr.allsolid || tr.startsolid || ((tr.fraction < 1.0f) && (tr.ent != self->enemy && !(tr.ent->svFlags & SVF_PLAYER)))) {
								return false;
							}

							self->monsterInfo.attack_state = AS_BLIND;
							self->monsterInfo.attack_finished = level.time + random_time(500_ms, 2.5_sec);
							return true;
						}
					}
				}
				return false;
			}
		}
	}

	if (level.time < self->monsterInfo.attack_finished)
		return false;

	gtime_t nexttime;

	if (self->spawnflags.has(SPAWNFLAG_TURRET_ROCKET)) {
		chance = 0.10f;
		nexttime = (1.8_sec - (0.2_sec * skill->integer));
	} else if (self->spawnflags.has(SPAWNFLAG_TURRET_BLASTER)) {
		chance = 0.35f;
		nexttime = (1.2_sec - (0.2_sec * skill->integer));
	} else {
		chance = 0.50f;
		nexttime = (0.8_sec - (0.1_sec * skill->integer));
	}

	if (skill->integer == 0)
		chance *= 0.5f;
	else if (skill->integer > 1)
		chance *= 2;

	// go ahead and shoot every time if it's a info_notnull
	// added visibility check
	if (((frandom() < chance) && (visible(self, self->enemy))) || (self->enemy->solid == SOLID_NOT)) {
		self->monsterInfo.attack_state = AS_MISSILE;
		self->monsterInfo.attack_finished = level.time + nexttime;
		return true;
	}

	self->monsterInfo.attack_state = AS_STRAIGHT;

	return false;
}

// **********************
//  SPAWN
// **********************

/*QUAKED monster_turret (1 .5 0) (-16 -16 -16) (16 16 16) AMBUSH TRIGGER_SPAWN SIGHT BLASTER MACHINEGUN ROCKET HEATBEAM WALLUNIT NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP

The automated defense turret that mounts on walls.
Check the weapon you want it to use: blaster, machinegun, rocket, heatbeam.
Default weapon is blaster.
When activated, wall units move 32 units in the direction they're facing.
*/
void SP_monster_turret(gentity_t *self) {
	int angle;

	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	// pre-caches
	sound_moved.assign("turret/moved.wav");
	sound_moving.assign("turret/moving.wav");
	gi.modelindex("models/objects/debris1/tris.md2");

	self->s.modelindex = gi.modelindex("models/monsters/turret/tris.md2");

	self->mins = { -12, -12, -12 };
	self->maxs = { 12, 12, 12 };
	self->moveType = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;

	self->health = 50 * st.health_multiplier;
	self->gibHealth = -100;
	self->mass = 250;
	self->yaw_speed = 10 * skill->integer;

	self->monsterInfo.armor_type = IT_ARMOR_COMBAT;
	self->monsterInfo.armor_power = 50;

	self->flags |= FL_MECHANICAL;

	self->pain = turret_pain;
	self->die = turret_die;

	// map designer didn't specify weapon type. set it now.
	if (!self->spawnflags.has(SPAWNFLAG_TURRET_WEAPONCHOICE))
		self->spawnflags |= SPAWNFLAG_TURRET_BLASTER;

	if (self->spawnflags.has(SPAWNFLAG_TURRET_HEATBEAM)) {
		self->spawnflags &= ~SPAWNFLAG_TURRET_HEATBEAM;
		self->spawnflags |= SPAWNFLAG_TURRET_BLASTER;
	}

	if (!self->spawnflags.has(SPAWNFLAG_TURRET_WALL_UNIT)) {
		self->monsterInfo.stand = turret_stand;
		self->monsterInfo.walk = turret_walk;
		self->monsterInfo.run = turret_run;
		self->monsterInfo.dodge = nullptr;
		self->monsterInfo.attack = turret_attack;
		self->monsterInfo.melee = nullptr;
		self->monsterInfo.sight = turret_sight;
		self->monsterInfo.search = turret_search;
		M_SetAnimation(self, &turret_move_stand);
	}

	self->monsterInfo.checkattack = turret_checkattack;

	self->monsterInfo.aiflags |= AI_MANUAL_STEERING;
	self->monsterInfo.scale = MODEL_SCALE;
	self->gravity = 0;

	self->offset = self->s.angles;
	angle = (int)self->s.angles[YAW];
	switch (angle) {
	case -1: // up
		self->s.angles[PITCH] = 270;
		self->s.angles[YAW] = 0;
		self->s.origin[2] += 2;
		break;
	case -2: // down
		self->s.angles[PITCH] = 90;
		self->s.angles[YAW] = 0;
		self->s.origin[2] -= 2;
		break;
	case 0:
		self->s.origin[0] += 2;
		break;
	case 90:
		self->s.origin[1] += 2;
		break;
	case 180:
		self->s.origin[0] -= 2;
		break;
	case 270:
		self->s.origin[1] -= 2;
		break;
	default:
		break;
	}

	gi.linkentity(self);

	if (self->spawnflags.has(SPAWNFLAG_TURRET_WALL_UNIT)) {
		if (!self->targetname) {
			FreeEntity(self);
			return;
		}

		self->takeDamage = false;
		self->use = turret_activate;
		turret_wall_spawn(self);
		if (!(self->monsterInfo.aiflags & AI_DO_NOT_COUNT)) {
			if (g_debug_monster_kills->integer)
				level.monstersRegistered[level.totalMonsters] = self;
			level.totalMonsters++;
		}
	} else {
		stationarymonster_start(self);
	}

	if (self->spawnflags.has(SPAWNFLAG_TURRET_MACHINEGUN)) {
		gi.soundindex("infantry/infatck1.wav");
		gi.soundindex("weapons/chngnu1a.wav");
		self->s.skinnum = 1;

		self->spawnflags &= ~SPAWNFLAG_TURRET_WEAPONCHOICE;
		self->spawnflags |= SPAWNFLAG_TURRET_MACHINEGUN;
	} else if (self->spawnflags.has(SPAWNFLAG_TURRET_ROCKET)) {
		gi.soundindex("weapons/rockfly.wav");
		gi.modelindex("models/objects/rocket/tris.md2");
		gi.soundindex("chick/chkatck2.wav");
		self->s.skinnum = 2;

		self->spawnflags &= ~SPAWNFLAG_TURRET_WEAPONCHOICE;
		self->spawnflags |= SPAWNFLAG_TURRET_ROCKET;
	} else {
		gi.modelindex("models/objects/laser/tris.md2");
		gi.soundindex("misc/lasfly.wav");
		gi.soundindex("soldier/solatck2.wav");

		self->spawnflags &= ~SPAWNFLAG_TURRET_WEAPONCHOICE;
		self->spawnflags |= SPAWNFLAG_TURRET_BLASTER;
	}

	// turrets don't get mad at monsters, and visa versa
	self->monsterInfo.aiflags |= AI_IGNORE_SHOTS;
	// blindfire
	if (self->spawnflags.has(SPAWNFLAG_TURRET_ROCKET | SPAWNFLAG_TURRET_BLASTER))
		self->monsterInfo.blindfire = true;
}
