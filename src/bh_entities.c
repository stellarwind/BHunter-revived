#include "bh_entities.h"
#include "bh_mission_profile.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>

typedef char slot_size[sizeof(BHEntity) == 864 ? 1 : -1];
typedef char callback_offsets[offsetof(BHEntity, refresh) == 200 && offsetof(BHEntity, step) == 204 &&
                                      offsetof(BHEntity, query) == 208
                                  ? 1
                                  : -1];
typedef char link_offsets[offsetof(BHEntity, query_next) == 212 && offsetof(BHEntity, next) == 224 ? 1 : -1];
typedef char field_offsets[offsetof(BHEntity, position) == 12 && offsetof(BHEntity, origin) == 72 &&
                                   offsetof(BHEntity, lifetime) == 180
                               ? 1
                               : -1];
typedef char pointer_size[sizeof(void *) == 4 ? 1 : -1];
typedef struct Chunk
{
    struct Chunk *next;
    unsigned long count;
    BHEntity *slots;
    unsigned char *live;
} Chunk;
static void *__cdecl allocate(unsigned long size, void *user)
{
    return malloc(size);
}
static void __cdecl release(void *pointer, void *user)
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
int __cdecl bh_entity_is_active(const BHEntityPool *pool, const BHEntity *entity)
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
int __cdecl bh_entities_init(BHEntityPool *pool, unsigned long count, const BHSceneAllocator *allocator)
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
int __cdecl bh_entities_dispose(BHEntityPool *pool)
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
BHEntity *__cdecl bh_entity_acquire(BHEntityPool *pool)
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
int __cdecl bh_entity_set_query(BHEntityPool *pool, BHEntity *entity, BHEntityCallback callback)
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
int __cdecl bh_entity_release(BHEntityPool *pool, BHEntity *entity)
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
    if (pool->focus == entity)
    {
        pool->focused = 0;
        pool->focus = NULL;
    }
    pool->error = BH_ENTITY_OK;
    return 1;
}
int __cdecl bh_entities_focus(BHEntityPool *pool, BHEntity *entity)
{
    if (!pool || (entity && !active(pool, entity)))
        return fail(pool, BH_ENTITY_ARGUMENT);
    pool->focus = entity;
    pool->focused = entity != NULL;
    pool->error = BH_ENTITY_OK;
    return 1;
}
static int successor(BHEntityPool *pool, BHEntity *entity, unsigned long generation)
{
    return !entity || (active(pool, entity) && entity->generation == generation) || fail(pool, BH_ENTITY_MUTATION);
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
    unsigned long generation, callback;
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
        {
            unsigned long identity = (unsigned long)entity->step;
            BH_MP_BEGIN(marker);
            entity->step(entity);
            BH_MP_END(marker, "entity_step", "focused", identity);
        }
        --pool->dispatch_depth;
        return 1;
    }
    entity = pool->active;
    while (entity)
    {
        next = entity->next;
        generation = next ? next->generation : 0;
        callback = refresh ? (unsigned long)entity->refresh : (unsigned long)entity->step;
        if (refresh)
        {
            if (entity->refresh)
            {
                BH_MP_BEGIN(marker);
                entity->refresh(entity, NULL);
                BH_MP_END(marker, "entity_refresh", "", callback);
            }
        }
        else if (entity->step)
        {
            BH_MP_BEGIN(marker);
            entity->step(entity);
            BH_MP_END(marker, "entity_step", "", callback);
        }
        /* Original00477325/0047734c resumes the cached pointer, not its
           generation. A script may release/reacquire that slot during the
           callback; dispatch its current callback/links as the original does.
           An unreallocated inactive successor remains a source boundary. */
        if (next && !active(pool, next))
        {
            fail(pool, BH_ENTITY_MUTATION);
            pool->failure_current = entity;
            pool->failure_next = next;
            pool->failure_callback = callback;
            pool->failure_expected_generation = generation;
            pool->failure_actual_generation = next ? next->generation : 0;
            pool->failure_refresh = refresh;
            pool->failure_next_active = active(pool, next);
            ++pool->failure_count;
            --pool->dispatch_depth;
            return 0;
        }
        if (next && next->generation != generation)
            ++pool->recycled_successors;
        entity = next;
    }
    --pool->dispatch_depth;
    return 1;
}
int __cdecl bh_entities_step(BHEntityPool *pool)
{
    return dispatch(pool, 0);
}
int __cdecl bh_entities_refresh(BHEntityPool *pool)
{
    return dispatch(pool, 1);
}

int __cdecl bh_entities_query(BHEntityPool *pool, unsigned long mask, unsigned long value, const BHVec3 *center,
                              float radius, void *packet)
{
    BHEntity *entity, *next;
    unsigned long generation, current_generation;
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
        generation = next ? next->generation : 0;
        current_generation = entity->generation;
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
                        if (!successor(pool, next, generation))
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
        if (!successor(pool, next, generation))
        {
            --pool->dispatch_depth;
            return 0;
        }
        entity = next;
    }
    --pool->dispatch_depth;
    return 1;
}
int __cdecl bh_entities_valid(const BHEntityPool *pool)
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
