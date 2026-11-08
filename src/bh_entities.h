#ifndef BH_ENTITIES_H
#define BH_ENTITIES_H
#include <stdint.h>
#include "bh_math.h"
#include "bh_scene.h"

typedef struct BHEntity BHEntity;
typedef void (*BHEntityStep)(BHEntity *entity);
typedef void (*BHEntityCallback)(BHEntity *entity, void *packet);
enum
{
    BH_ENTITY_NETWORK_PLAYER = 0x200,
    BH_ENTITY_SCRIPTED_PROP = 0x400
};
struct BHEntity
{
    unsigned long flags, generation, kind;
    BHVec3 position;
    unsigned char opaque0[48];
    BHVec3 origin;
    unsigned char opaque1[96];
    float lifetime, scale;
    unsigned char opaque2a[4];
    float collision_radius; /* verified +c0 broad-phase extent, 0040af20 */
    unsigned char opaque2b[4];
    BHEntityCallback refresh;
    BHEntityStep step;
    BHEntityCallback query;
    BHEntity *query_next, *query_previous, *previous, *next;
    unsigned char payload[636];
};
typedef struct BHEntityPositionPacket
{
    long event;
    BHVec3 position;
} BHEntityPositionPacket;
typedef enum BHEntityError
{
    BH_ENTITY_OK,
    BH_ENTITY_ARGUMENT,
    BH_ENTITY_MEMORY,
    BH_ENTITY_LIMIT,
    BH_ENTITY_BUSY,
    BH_ENTITY_CONTAINER,
    BH_ENTITY_MUTATION,
    BH_ENTITY_NUMERICAL
} BHEntityError;
typedef struct BHEntityMutation
{
    const char *dispatch;
    const BHEntity *entity, *successor;
    uintptr_t callback;
    unsigned long generation, successor_generation;
} BHEntityMutation;
typedef struct BHEntityPool
{
    BHEntity *active, *free_slots, *query_head, *focus;
    unsigned long capacity, used;
    BHVec3 origin;
    BHEntityError error;
    BHSceneAllocator allocator;
    void *chunks; /* source-private ownership, not original game globals */
    unsigned long dispatch_depth;
    int focused;
    unsigned long retirements; /* adapter membership invalidation, increments on release */
    BHEntityMutation step_mutation, query_mutation;
} BHEntityPool;

/* Complete original pool/dispatch behaviors with explicit context and NULL
   representation of verified 0047d680 RET callback. No game callbacks supplied
   by this module. Initial slots zeroed, recycled/growth payload not cleared.
   Initialize a fresh context; dispose before reinitialization. Max 65535 slots.
   Origin is an explicit caller-supplied equivalent of the shared origin vector.
   Generation, links, callbacks and private ownership fields are engine-owned. */
int bh_entities_init(BHEntityPool *pool, unsigned long count, const BHSceneAllocator *allocator); /* 00476da0 */
BHEntity *bh_entity_acquire(BHEntityPool *pool);                                                  /* 00476f20 */
int bh_entity_release(BHEntityPool *pool, BHEntity *entity);                                      /* 00477070 */
int bh_entity_set_query(BHEntityPool *pool, BHEntity *entity, BHEntityCallback callback);         /* 00476e90 */
int bh_entities_query(BHEntityPool *pool, unsigned long mask, unsigned long value, const BHVec3 *center, float radius,
                      void *packet);         /* 004770f0 */
int bh_entities_step(BHEntityPool *pool);    /* 004772f0 */
int bh_entities_refresh(BHEntityPool *pool); /* 00477330 */
/* Source ownership/focus/validation adapters. Dispose rejects active dispatch.
   Callbacks may release the current slot or add new head slots. Changing a
   cached successor to an inactive slot stops dispatch with MUTATION; active
   recycled slots retain the original saved-pointer step/refresh order. Nested
   queries are allowed to depth 16; nested step/refresh are rejected. */
int bh_entities_dispose(BHEntityPool *pool);
int bh_entities_focus(BHEntityPool *pool, BHEntity *entity);
int bh_entities_valid(const BHEntityPool *pool);
/* Source ownership check using existing chunk live bits; no list traversal. */
int bh_entity_is_active(const BHEntityPool *pool, const BHEntity *entity);
#endif
