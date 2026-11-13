#include "bh_entities.h"
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif
#include "bh_fp.h"
#include "bh_round.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>

static_assert(sizeof(void *) == 4, "entity layout requires 32-bit pointers");
static_assert(sizeof(BHEntity) == 864, "entity slot size");
static_assert(sizeof(BHRemotePose) == 32, "remote pose size");

#define ENTITY_OFFSET(member, offset) static_assert(offsetof(BHEntity, member) == (offset), #member " offset")
ENTITY_OFFSET(refresh, 200);
ENTITY_OFFSET(step, 204);
ENTITY_OFFSET(query, 208);
ENTITY_OFFSET(query_next, 212);
ENTITY_OFFSET(next, 224);
ENTITY_OFFSET(position, 12);
ENTITY_OFFSET(origin, 72);
ENTITY_OFFSET(lifetime, 180);
ENTITY_OFFSET(basis, 0x18);
ENTITY_OFFSET(euler, 0x3c);
ENTITY_OFFSET(velocity, 0x48);
ENTITY_OFFSET(angular_velocity, 0x54);
ENTITY_OFFSET(acceleration, 0x60);
ENTITY_OFFSET(angular_acceleration, 0x6c);
ENTITY_OFFSET(inertia, 0x78);
ENTITY_OFFSET(last_clear, 0x84);
ENTITY_OFFSET(collision_geometry, 0xbc);
ENTITY_OFFSET(world, 0xc4);
ENTITY_OFFSET(payload, 0xe4);
ENTITY_OFFSET(actor.draw, 0xe8);
ENTITY_OFFSET(actor.global_next, 0xf0);
ENTITY_OFFSET(actor.room_next, 0xf4);
ENTITY_OFFSET(actor.kind, 0xfc);
ENTITY_OFFSET(flyby.camera, 0xe8);
ENTITY_OFFSET(flyby.route_phase, 0x1dc);
ENTITY_OFFSET(flyby.orbit_time, 0x1f8);
static_assert(sizeof(BHFlybyFields) == 0x1a4, "flyby payload size");
ENTITY_OFFSET(actor.radar_id, 0x11c);
ENTITY_OFFSET(actor.radar_owner, 0x1b8);
ENTITY_OFFSET(flyby.previous_position, 0x23c);
ENTITY_OFFSET(flyby.waypoint_index, 0x284);
ENTITY_OFFSET(remote.pose_flags, 0x11a);
ENTITY_OFFSET(remote.oldest, 0x13c);
ENTITY_OFFSET(remote.middle, 0x15c);
ENTITY_OFFSET(remote.newest, 0x17c);
ENTITY_OFFSET(remote.newest.world, 0x198);
ENTITY_OFFSET(remote.propulsion, 0x2c0);
ENTITY_OFFSET(player.camera, 0xe8);
ENTITY_OFFSET(player.input_list, 0x188);
ENTITY_OFFSET(player.input, 0x18c);
ENTITY_OFFSET(player.model_resource, 0x21c);
ENTITY_OFFSET(player.engine_sound, 0x220);
ENTITY_OFFSET(player.cash, 0x230);
ENTITY_OFFSET(player.damage, 0x23c);
ENTITY_OFFSET(player.weapons, 0x258);
ENTITY_OFFSET(player.aim_direction, 0x280);
ENTITY_OFFSET(player.recoil_position, 0x298);
ENTITY_OFFSET(player.camera_mode, 0x2b0);
ENTITY_OFFSET(player.disabled_frames, 0x2b4);
ENTITY_OFFSET(player.selected_weapon, 0x2b6);
ENTITY_OFFSET(player.vehicle, 0x2bb);
ENTITY_OFFSET(player.propulsion.channels, 0x2bc);
ENTITY_OFFSET(player.propulsion.thrust.nozzles, 0x2c0);
ENTITY_OFFSET(player.propulsion.delayed.creation, 0x2e8);
ENTITY_OFFSET(player.engine_output, 0x2f8);
ENTITY_OFFSET(player.nozzle_trail, 0x2fc);

/* All recovered actor fields share the original payload layout. */
ENTITY_OFFSET(actor.draw, 0xe8);
ENTITY_OFFSET(actor.global_previous, 0xec);
ENTITY_OFFSET(actor.global_next, 0xf0);
ENTITY_OFFSET(actor.room_next, 0xf4);
ENTITY_OFFSET(actor.room_previous, 0xf8);
ENTITY_OFFSET(actor.kind, 0xfc);
ENTITY_OFFSET(actor.bounty_token, 0x100);
ENTITY_OFFSET(actor.sound, 0x104);
ENTITY_OFFSET(actor.sound_frames, 0x108);
ENTITY_OFFSET(actor.sound_resource, 0x10c);
ENTITY_OFFSET(actor.bound_resource, 0x110);
ENTITY_OFFSET(actor.model_resource, 0x114);
ENTITY_OFFSET(actor.destroy_when_faded, 0x118);
ENTITY_OFFSET(actor.death_tag, 0x11c);
ENTITY_OFFSET(actor.route_start, 0x120);
ENTITY_OFFSET(actor.route_end, 0x12c);
ENTITY_OFFSET(actor.portal_center, 0x138);
ENTITY_OFFSET(actor.route_first, 0x144);
ENTITY_OFFSET(actor.route_second, 0x150);
ENTITY_OFFSET(actor.offset, 0x15c);
ENTITY_OFFSET(actor.offset_velocity, 0x168);
ENTITY_OFFSET(actor.draw_offset, 0x174);
ENTITY_OFFSET(actor.phase, 0x180);
ENTITY_OFFSET(actor.duration, 0x184);
ENTITY_OFFSET(actor.speed, 0x188);
ENTITY_OFFSET(actor.u, 0x18c);
ENTITY_OFFSET(actor.v, 0x190);
ENTITY_OFFSET(actor.damage, 0x194);
ENTITY_OFFSET(actor.speed_scale, 0x198);
ENTITY_OFFSET(actor.fade, 0x19c);
ENTITY_OFFSET(actor.script_path, 0x1a0);
ENTITY_OFFSET(actor.current, 0x1a4);
ENTITY_OFFSET(actor.next, 0x1a8);
ENTITY_OFFSET(actor.damage_source, 0x1ac);
ENTITY_OFFSET(actor.jitter, 0x1b0);
ENTITY_OFFSET(actor.pursuit, 0x1b4);
ENTITY_OFFSET(actor.target, 0x1b8);
ENTITY_OFFSET(actor.saved_end, 0x1bc);
ENTITY_OFFSET(actor.saved_tangent, 0x1c8);
ENTITY_OFFSET(actor.forced_pursuit, 0x1d4);
ENTITY_OFFSET(actor.burst_frames, 0x1d8);
ENTITY_OFFSET(actor.primary_weapon, 0x1dc);
ENTITY_OFFSET(actor.secondary_weapon, 0x1de);
ENTITY_OFFSET(actor.attack_frame, 0x1e0);
ENTITY_OFFSET(actor.attack_cooldown, 0x1e2);
ENTITY_OFFSET(actor.tick, 0x1e4);
ENTITY_OFFSET(actor.duration_ticks, 0x1e6);
ENTITY_OFFSET(actor.random, 0x1e8);
static_assert(sizeof(BHActorFields) == 0x108, "actor field view size");
ENTITY_OFFSET(mass, 0xb4);
ENTITY_OFFSET(friction, 0xb8);

static_assert(sizeof(BHRemoteFireSchedule) == 12, "remote firing schedule size");
static_assert(offsetof(BHRemoteFireSchedule, target) == 8, "remote firing target offset");
static_assert(sizeof(BHRemoteFields) == 0x1e8, "remote payload view size");
ENTITY_OFFSET(remote.engine_sound, 0x104);
ENTITY_OFFSET(remote.sound_frames, 0x108);
ENTITY_OFFSET(remote.engine_resource, 0x10c);
ENTITY_OFFSET(remote.reserved_sound, 0x110);
ENTITY_OFFSET(remote.slot, 0x114);
ENTITY_OFFSET(remote.roster_slot, 0x116);
ENTITY_OFFSET(remote.muzzle_phase, 0x118);
ENTITY_OFFSET(remote.pose_flags, 0x11a);
ENTITY_OFFSET(remote.score, 0x11c);
ENTITY_OFFSET(remote.name, 0x11e);
ENTITY_OFFSET(remote.cached_world, 0x134);
ENTITY_OFFSET(remote.player_id, 0x138);
ENTITY_OFFSET(remote.firing[0].remaining, 0x19c);
ENTITY_OFFSET(remote.firing[0].spacing, 0x19e);
ENTITY_OFFSET(remote.firing[0].countdown, 0x1a0);
ENTITY_OFFSET(remote.firing[0].packet_count, 0x1a2);
ENTITY_OFFSET(remote.firing[0].target, 0x1a4);
ENTITY_OFFSET(remote.firing[1].remaining, 0x1a8);
ENTITY_OFFSET(remote.firing[1].spacing, 0x1aa);
ENTITY_OFFSET(remote.firing[1].countdown, 0x1ac);
ENTITY_OFFSET(remote.firing[1].packet_count, 0x1ae);
ENTITY_OFFSET(remote.firing[1].target, 0x1b0);
ENTITY_OFFSET(remote.firing[2].remaining, 0x1b4);
ENTITY_OFFSET(remote.firing[2].spacing, 0x1b6);
ENTITY_OFFSET(remote.firing[2].countdown, 0x1b8);
ENTITY_OFFSET(remote.firing[2].packet_count, 0x1ba);
ENTITY_OFFSET(remote.firing[2].target, 0x1bc);
ENTITY_OFFSET(remote.firing[3].remaining, 0x1c0);
ENTITY_OFFSET(remote.firing[3].spacing, 0x1c2);
ENTITY_OFFSET(remote.firing[3].countdown, 0x1c4);
ENTITY_OFFSET(remote.firing[3].packet_count, 0x1c6);
ENTITY_OFFSET(remote.firing[3].target, 0x1c8);
ENTITY_OFFSET(remote.firing[4].remaining, 0x1cc);
ENTITY_OFFSET(remote.firing[4].spacing, 0x1ce);
ENTITY_OFFSET(remote.firing[4].countdown, 0x1d0);
ENTITY_OFFSET(remote.firing[4].packet_count, 0x1d2);
ENTITY_OFFSET(remote.firing[4].target, 0x1d4);
ENTITY_OFFSET(remote.firing[5].remaining, 0x1d8);
ENTITY_OFFSET(remote.firing[5].spacing, 0x1da);
ENTITY_OFFSET(remote.firing[5].countdown, 0x1dc);
ENTITY_OFFSET(remote.firing[5].packet_count, 0x1de);
ENTITY_OFFSET(remote.firing[5].target, 0x1e0);
ENTITY_OFFSET(remote.firing[6].remaining, 0x1e4);
ENTITY_OFFSET(remote.firing[6].spacing, 0x1e6);
ENTITY_OFFSET(remote.firing[6].countdown, 0x1e8);
ENTITY_OFFSET(remote.firing[6].packet_count, 0x1ea);
ENTITY_OFFSET(remote.firing[6].target, 0x1ec);
ENTITY_OFFSET(remote.firing[7].remaining, 0x1f0);
ENTITY_OFFSET(remote.firing[7].spacing, 0x1f2);
ENTITY_OFFSET(remote.firing[7].countdown, 0x1f4);
ENTITY_OFFSET(remote.firing[7].packet_count, 0x1f6);
ENTITY_OFFSET(remote.firing[7].target, 0x1f8);
ENTITY_OFFSET(remote.firing[8].remaining, 0x1fc);
ENTITY_OFFSET(remote.firing[8].spacing, 0x1fe);
ENTITY_OFFSET(remote.firing[8].countdown, 0x200);
ENTITY_OFFSET(remote.firing[8].packet_count, 0x202);
ENTITY_OFFSET(remote.firing[8].target, 0x204);
#undef ENTITY_OFFSET

typedef struct Chunk
{
    struct Chunk *next;
    unsigned long count;
    BHEntity *slots;
    unsigned char *live;
} Chunk;
static void *allocate(unsigned long size, void *user)
{
    return malloc(size);
}
static void release(void *pointer, void *user)
{
    free(pointer);
}
static int fail(BHEntityPool *pool, BHEntityError error)
{
    if (pool)
        pool->error = error;
    return 0;
}
static unsigned char *membership(const BHEntityPool *pool, const BHEntity *entity)
{
    Chunk *chunk = (Chunk *)pool->chunks;
    unsigned long address = (unsigned long)entity, base, offset;
    while (chunk)
    {
        base = (unsigned long)chunk->slots;
        if (address >= base)
        {
            offset = address - base;
            if (offset < chunk->count * 864 && !(offset % 864))
                return chunk->live + offset / 864;
        }
        chunk = chunk->next;
    }
    return NULL;
}
static int active(const BHEntityPool *pool, const BHEntity *entity)
{
    unsigned char *state;
    if (!pool || !entity)
        return 0;
    state = membership(pool, entity);
    return state && *state;
}
int bh_entity_is_active(const BHEntityPool *pool, const BHEntity *entity)
{
    return active(pool, entity);
}
static Chunk *new_chunk(BHEntityPool *pool, unsigned long count, int zero)
{
    Chunk *chunk = (Chunk *)pool->allocator.allocate(sizeof(Chunk) + count * 865, pool->allocator.user);
    if (!chunk)
        return NULL;
    chunk->count = count;
    chunk->slots = (BHEntity *)(chunk + 1);
    chunk->live = (unsigned char *)(chunk->slots + count);
    chunk->next = NULL;
    memset(chunk->live, 0, count);
    if (zero)
        memset(chunk->slots, 0, count * 864);
    return chunk;
}
int bh_entities_init(BHEntityPool *pool, unsigned long count, const BHSceneAllocator *allocator)
{
    BHEntityPool next;
    Chunk *chunk;
    unsigned long i;
    if (!pool || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(&next, 0, sizeof(next));
    if (allocator)
        next.allocator = *allocator;
    else
    {
        next.allocator.allocate = allocate;
        next.allocator.release = release;
    }
    if (!count || count > 65535)
    {
        next.error = BH_ENTITY_LIMIT;
        *pool = next;
        return 0;
    }
    chunk = new_chunk(&next, count, 1);
    if (!chunk)
    {
        next.error = BH_ENTITY_MEMORY;
        *pool = next;
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        chunk->slots[i].previous = i ? chunk->slots + i - 1 : NULL;
        chunk->slots[i].next = i + 1 < count ? chunk->slots + i + 1 : NULL;
    }
    next.chunks = chunk;
    next.capacity = count;
    next.free_slots = chunk->slots;
    *pool = next;
    return 1;
}
int bh_entities_dispose(BHEntityPool *pool)
{
    Chunk *chunk, *next;
    BHSceneAllocator memory;
    if (!pool)
        return 0;
    if (pool->dispatch_depth)
        return fail(pool, BH_ENTITY_BUSY);
    memory = pool->allocator;
    chunk = (Chunk *)pool->chunks;
    while (chunk)
    {
        next = chunk->next;
        memory.release(chunk, memory.user);
        chunk = next;
    }
    memset(pool, 0, sizeof(*pool));
    pool->allocator = memory;
    return 1;
}
BHEntity *bh_entity_acquire(BHEntityPool *pool)
{
    BHEntity *entity;
    Chunk *chunk;
    unsigned long count, i;
    if (!pool || !pool->allocator.allocate)
    {
        fail(pool, BH_ENTITY_ARGUMENT);
        return NULL;
    }
    if (pool->used == pool->capacity)
    {
        count = pool->capacity / 8 + 8;
        if (count > 65535 - pool->capacity)
        {
            fail(pool, BH_ENTITY_LIMIT);
            return NULL;
        }
        chunk = new_chunk(pool, count, 0);
        if (!chunk)
        {
            fail(pool, BH_ENTITY_MEMORY);
            return NULL;
        }
        for (i = 0; i + 1 < count; i++)
        {
            entity = chunk->slots + i;
            entity->generation = 0;
            entity->previous = NULL;
            entity->next = pool->free_slots;
            if (pool->free_slots)
                pool->free_slots->previous = entity;
            pool->free_slots = entity;
        }
        entity = chunk->slots + count - 1;
        entity->generation = 0;
        chunk->next = (Chunk *)pool->chunks;
        pool->chunks = chunk;
        pool->capacity += count;
    }
    else
    {
        entity = pool->free_slots;
        if (!entity || !membership(pool, entity))
        {
            fail(pool, BH_ENTITY_CONTAINER);
            return NULL;
        }
        pool->free_slots = entity->next;
        if (pool->free_slots)
            pool->free_slots->previous = NULL;
        ++entity->generation;
    }
    entity->previous = NULL;
    entity->next = pool->active;
    if (pool->active)
        pool->active->previous = entity;
    entity->refresh = NULL;
    entity->step = NULL;
    entity->query = NULL;
    entity->query_next = NULL;
    entity->query_previous = NULL;
    entity->lifetime = -1;
    entity->scale = 1;
    entity->kind = 0;
    entity->origin = pool->origin;
    *membership(pool, entity) = 1;
    pool->active = entity;
    ++pool->used;
    pool->error = BH_ENTITY_OK;
    return entity;
}
int bh_entity_set_query(BHEntityPool *pool, BHEntity *entity, BHEntityCallback callback)
{
    if (!active(pool, entity))
        return fail(pool, BH_ENTITY_ARGUMENT);
    if (!callback && entity->query)
    {
        if (entity->query_previous)
            entity->query_previous->query_next = entity->query_next;
        else
            pool->query_head = entity->query_next;
        if (entity->query_next)
            entity->query_next->query_previous = entity->query_previous;
        entity->query_next = entity->query_previous = NULL;
    }
    else if (callback && !entity->query)
    {
        entity->query_previous = NULL;
        entity->query_next = pool->query_head;
        if (pool->query_head)
            pool->query_head->query_previous = entity;
        pool->query_head = entity;
    }
    entity->query = callback;
    pool->error = BH_ENTITY_OK;
    return 1;
}
int bh_entity_release(BHEntityPool *pool, BHEntity *entity)
{
    if (!active(pool, entity))
        return fail(pool, BH_ENTITY_ARGUMENT);
    bh_entity_set_query(pool, entity, NULL);
    if (entity->previous)
        entity->previous->next = entity->next;
    else
        pool->active = entity->next;
    if (entity->next)
        entity->next->previous = entity->previous;
    entity->previous = NULL;
    entity->next = pool->free_slots;
    if (pool->free_slots)
        pool->free_slots->previous = entity;
    pool->free_slots = entity;
    ++entity->generation;
    *membership(pool, entity) = 0;
    --pool->used;
    ++pool->retirements;
    if (pool->focus == entity)
    {
        pool->focused = 0;
        pool->focus = NULL;
    }
    pool->error = BH_ENTITY_OK;
    return 1;
}
int bh_entities_focus(BHEntityPool *pool, BHEntity *entity)
{
    if (!pool || (entity && !active(pool, entity)))
        return fail(pool, BH_ENTITY_ARGUMENT);
    pool->focus = entity;
    pool->focused = entity != NULL;
    pool->error = BH_ENTITY_OK;
    return 1;
}
static int successor(BHEntityPool *pool, const BHEntityMutation *query)
{
    const BHEntity *next = query->successor;
    if (!next || (active(pool, next) && next->generation == query->successor_generation))
        return 1;
    pool->query_mutation = *query;
    return fail(pool, BH_ENTITY_MUTATION);
}
static int enter(BHEntityPool *pool, int nested)
{
    if (!pool || !pool->allocator.allocate)
        return fail(pool, BH_ENTITY_ARGUMENT);
    if (pool->dispatch_depth >= 16 || (!nested && pool->dispatch_depth))
        return fail(pool, BH_ENTITY_BUSY);
    ++pool->dispatch_depth;
    pool->error = BH_ENTITY_OK;
    return 1;
}
static int dispatch(BHEntityPool *pool, int refresh)
{
    BHEntity *entity, *next;
#ifdef BH_SDL_BACKEND
    unsigned long render_mark, render_identity, render_generation;
#endif
    if (!enter(pool, 0))
        return 0;
    if (!refresh && pool->focused)
    {
        entity = pool->focus;
        if (!active(pool, entity))
        {
            --pool->dispatch_depth;
            return fail(pool, BH_ENTITY_ARGUMENT);
        }
        if (entity->step)
            entity->step(entity);
        --pool->dispatch_depth;
        return 1;
    }
    entity = pool->active;
    while (entity)
    {
        next = entity->next;
        BHEntityMutation step = {
            refresh ? "refresh" : "step", entity, next, refresh ? (uintptr_t)entity->refresh : (uintptr_t)entity->step,
            entity->generation,           0};
#ifdef BH_SDL_BACKEND
        render_mark = bh_interpolation_mark();
        render_identity = (unsigned long)entity;
        render_generation = entity->generation * 2 + refresh;
#endif
        if (refresh)
        {
            if (entity->refresh)
                entity->refresh(entity, NULL);
        }
        else if (entity->step)
            entity->step(entity);
#ifdef BH_SDL_BACKEND
        bh_interpolation_stamp(render_mark, render_identity, render_generation);
#endif
        /* Original00477325/0047734c resumes the cached pointer, not its
           generation. A script may release/reacquire that slot during the
           callback; dispatch its current callback/links as the original does.
           An unreallocated inactive successor remains a source boundary. */
        if (next && !active(pool, next))
        {
            pool->step_mutation = step;
            fail(pool, BH_ENTITY_MUTATION);
            --pool->dispatch_depth;
            return 0;
        }
        entity = next;
    }
    --pool->dispatch_depth;
    return 1;
}
int bh_entities_step(BHEntityPool *pool)
{
    return dispatch(pool, 0);
}
int bh_entities_refresh(BHEntityPool *pool)
{
    return dispatch(pool, 1);
}

int bh_entities_query(BHEntityPool *pool, unsigned long mask, unsigned long value, const BHVec3 *center, float radius,
                      void *packet)
{
    BHEntity *entity, *next;
    unsigned long current_generation;
    int spatial = radius > 0;
    float low[3], high[3], rx, rz;
    double dx, dy, dz, distance;
    BHEntityPositionPacket position;
    if (spatial && !center)
        return fail(pool, BH_ENTITY_ARGUMENT);
    if (spatial && !bh_math_environment_supported())
        return fail(pool, BH_ENTITY_NUMERICAL);
    if (!enter(pool, 1))
        return 0;
    if (spatial)
    {
        bh_round_float(low, (double)center->x - radius);
        bh_round_float(high, (double)center->x + radius);
        bh_round_float(low + 1, (double)center->y - radius);
        bh_round_float(high + 1, (double)center->y + radius);
        bh_round_float(low + 2, (double)center->z - radius);
        bh_round_float(high + 2, (double)center->z + radius);
    }
    entity = pool->query_head;
    while (entity)
    {
        next = entity->next;
        current_generation = entity->generation;
        BHEntityMutation query = {
            "query", entity, next, (uintptr_t)entity->query, current_generation, next ? next->generation : 0};
        if ((entity->flags & mask) == value)
        {
            if (!spatial)
            {
                if (entity->query)
                    entity->query(entity, packet);
            }
            else
            {
                if (entity->flags & 0x100)
                {
                    position.event = 3;
                    position.position = entity->position;
                    if (entity->query)
                        entity->query(entity, &position);
                    if (!active(pool, entity) || entity->generation != current_generation)
                    {
                        if (!successor(pool, &query))
                        {
                            --pool->dispatch_depth;
                            return 0;
                        }
                        entity = next;
                        continue;
                    }
                    entity->position = position.position;
                }
                if (entity->position.x >= low[0] && entity->position.x <= high[0] && entity->position.y >= low[1] &&
                    entity->position.y <= high[1] && entity->position.z >= low[2] && entity->position.z <= high[2])
                {
                    dx = (double)center->x - entity->position.x;
                    dy = (double)center->y - entity->position.y;
                    dz = (double)center->z - entity->position.z;
                    bh_round_float(&rx, dx);
                    bh_round_float(&rz, dz);
                    distance = sqrt((dx * rx + dy * dy) + dz * rz);
                    if (distance <= (double)radius && entity->query)
                        entity->query(entity, packet);
                }
            }
        }
        if (!successor(pool, &query))
        {
            --pool->dispatch_depth;
            return 0;
        }
        entity = next;
    }
    --pool->dispatch_depth;
    return 1;
}
int bh_entities_valid(const BHEntityPool *pool)
{
    const BHEntity *entity, *previous = NULL;
    Chunk *chunk;
    unsigned long n = 0, total = 0, used = 0, queries = 0, i;
    if (!pool || pool->used > pool->capacity || pool->capacity > 65535)
        return 0;
    for (chunk = (Chunk *)pool->chunks; chunk; chunk = chunk->next)
    {
        total += chunk->count;
        for (i = 0; i < chunk->count; i++)
            if (chunk->live[i])
                ++used;
    }
    if (total != pool->capacity || used != pool->used)
        return 0;
    entity = pool->active;
    while (entity)
    {
        if (++n > pool->used || !active(pool, entity) || entity->previous != previous)
            return 0;
        previous = entity;
        entity = entity->next;
    }
    if (n != pool->used)
        return 0;
    n = 0;
    previous = NULL;
    entity = pool->free_slots;
    while (entity)
    {
        unsigned char *state = membership(pool, entity);
        if (++n > pool->capacity - pool->used || !state || *state || entity->previous != previous)
            return 0;
        previous = entity;
        entity = entity->next;
    }
    if (n != pool->capacity - pool->used)
        return 0;
    entity = pool->query_head;
    previous = NULL;
    while (entity)
    {
        if (++queries > pool->used || !active(pool, entity) || !entity->query || entity->query_previous != previous)
            return 0;
        previous = entity;
        entity = entity->query_next;
    }
    entity = pool->active;
    n = 0;
    while (entity)
    {
        if (entity->query)
            ++n;
        entity = entity->next;
    }
    return n == queries && (!pool->focused || active(pool, pool->focus));
}
