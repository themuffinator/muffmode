// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

#include <cstddef>
#include <limits>

/*
==============================================================================

PLAYER TRAIL

==============================================================================

This is a two-way list containing the a list of points of where
the player has been recently. It is used by monsters for pursuit.

This is improved from vanilla; now, the list itself is stored in
client data so it can be stored for multiple clients.

chain = next
enemy = prev

The head node will always have a null "chain", the tail node
will always have a null "enemy".
*/

namespace {

constexpr std::size_t TRAIL_LENGTH = 8;
constexpr const char *PLAYER_TRAIL_CLASSNAME = "player_trail";

[[nodiscard]] std::size_t PlayerTrail_Length(const gclient_t &client) {
        std::size_t length = 0;
        for (auto *node = client.trail_tail; node && length < TRAIL_LENGTH; node = node->chain)
                ++length;

        return length;
}

[[nodiscard]] gentity_t *DetachTrailTail(gclient_t &client) {
        gentity_t *const tail = client.trail_tail;
        if (!tail)
                return nullptr;

        client.trail_tail = tail->chain;
        if (client.trail_tail)
                client.trail_tail->enemy = nullptr;
        else
                client.trail_head = nullptr;

        tail->chain = nullptr;
        tail->enemy = nullptr;
        tail->owner = nullptr;

        return tail;
}

} // namespace

// places a new entity at the head of the player trail.
// the tail entity may be moved to the front if the length
// is at the end.
static gentity_t *PlayerTrail_Spawn(gentity_t *owner) {
        if (!owner || !owner->client)
                return nullptr;

        gentity_t *trail = nullptr;
        auto &client = *owner->client;

        if (PlayerTrail_Length(client) >= TRAIL_LENGTH) {
                trail = DetachTrailTail(client);
        } else {
                trail = G_Spawn();
        }

        if (!trail)
                return nullptr;

        trail->classname = PLAYER_TRAIL_CLASSNAME;
        trail->chain = nullptr;

        if (client.trail_head)
                client.trail_head->chain = trail;

        trail->enemy = client.trail_head;
        client.trail_head = trail;

        if (!client.trail_tail)
                client.trail_tail = trail;

        return trail;
}

// destroys all player trail entities in the map.
// we don't want these to stay around across level loads.
void PlayerTrail_Destroy(gentity_t *player) {
        for (std::size_t i = 0; i < globals.num_entities; ++i) {
                auto &entity = g_entities[i];
                if (!entity.classname || strcmp(entity.classname, PLAYER_TRAIL_CLASSNAME) != 0)
                        continue;

                if (player && entity.owner != player)
                        continue;

                G_FreeEntity(&entity);
        }

        if (player) {
                if (player->client) {
                        player->client->trail_head = nullptr;
                        player->client->trail_tail = nullptr;
                }
        } else {
                for (auto *entity_client : active_clients()) {
                        if (!entity_client->client)
                                continue;

                        entity_client->client->trail_head = nullptr;
                        entity_client->client->trail_tail = nullptr;
                }
        }
}

// check to see if we can add a new player trail spot
// for this player.
void PlayerTrail_Add(gentity_t *player) {
        if (!player || !player->client)
                return;

        // if we can still see the head, we don't want a new one.
        if (player->client->trail_head && visible(player, player->client->trail_head))
                return;
        // don't spawn trails in intermission, if we're dead, if we're noclipping or not on ground yet
        else if (level.intermission_time || player->health <= 0 || player->movetype == MOVETYPE_NOCLIP || player->movetype == MOVETYPE_FREECAM ||
                !player->groundentity)
                return;

        gentity_t *trail = PlayerTrail_Spawn(player);
        if (!trail)
                return;

        trail->s.origin = player->s.old_origin;
        trail->timestamp = level.time;
        trail->owner = player;
}

// pick a trail node that matches the player
// we're hunting that is visible to us.
gentity_t *PlayerTrail_Pick(gentity_t *self, bool next) {
        if (!self || !self->enemy || !self->enemy->client || !self->enemy->client->trail_head)
                return nullptr;

        // find which marker head that was dropped while we
        // were searching for this enemy
        gentity_t *marker;

        for (marker = self->enemy->client->trail_head; marker; marker = marker->enemy) {
                if (marker->timestamp <= self->monsterinfo.trail_time)
                        continue;

                break;
        }

	if (next) {
		// find the marker we're closest to
		float closest_dist = std::numeric_limits<float>::infinity();
		gentity_t *closest = nullptr;

		for (gentity_t *m2 = marker; m2; m2 = m2->enemy) {
			float len = (m2->s.origin - self->s.origin).lengthSquared();

			if (len < closest_dist) {
				closest_dist = len;
				closest = m2;
			}
		}

		// should never happen
		if (!closest)
			return nullptr;

		// use the next one from the closest one
		marker = closest->chain;
	} else {
		// from that marker, find the first one we can see
		for (; marker && !visible(self, marker); marker = marker->enemy)
			continue;
	}

	return marker;
}
