#ifndef BH_ENTITIES_H
#define BH_ENTITIES_H
#include <stdint.h>
#include "bh_entity_payloads.h"
#include "bh_entity_bindings.h"
#include "bh_flyby_fields.h"
#include "bh_math.h"
#include "bh_player_fields.h"
#include "bh_remote_fields.h"
#include "bh_scene.h"

typedef struct BHEntity BHEntity;
struct BHGeometry;
struct BHBsp;
struct BHWorldGeometry;
typedef void (*BHEntityStep)(BHEntity *entity);
typedef void (*BHEntityCallback)(BHEntity *entity, void *packet);
typedef struct BHActorFields
{
    unsigned char reserved[4];
    BHEntityStep draw;
    BHEntity *global_previous, *global_next, *room_next, *room_previous;
    unsigned long kind;
    long bounty_token;
    unsigned long sound, sound_frames, sound_resource;
    union {
        unsigned long bound_resource, city_id;
    };
    unsigned long model_resource, destroy_when_faded;
    union {
        unsigned long death_tag;
        long radar_id;
    };
    BHVec3 route_start, route_end, portal_center, route_first, route_second;
    BHVec3 offset, offset_velocity, draw_offset;
    float phase, duration, speed, u, v, damage, speed_scale, fade;
    void *script_path;
    struct BHWorldGeometry *current, *next;
    union {
        BHEntity *damage_source;
        unsigned long damage_source_identity;
    };
    float jitter, pursuit;
    union {
        BHEntity *target;
        BHEntity *radar_owner;
        unsigned long target_identity;
    };
    BHVec3 saved_end, saved_tangent;
    unsigned long forced_pursuit;
    long burst_frames;
    unsigned short primary_weapon, secondary_weapon, attack_frame, attack_cooldown;
    unsigned short tick, duration_ticks;
    unsigned long random;
} BHActorFields;

struct BHEntity
{
    unsigned long flags, generation, kind;
    BHVec3 position;
    union {
        unsigned char opaque0[48];
        struct
        {
            BHVec3 basis[3], euler;
        };
    };
    union {
        BHVec3 origin;
        BHVec3 velocity;
    };
    union {
        unsigned char opaque1[96];
        struct
        {
            BHVec3 angular_velocity, acceleration, angular_acceleration, inertia;
            BHMat34 last_clear;
        };
    };
    union {
        float lifetime, mass;
    };
    union {
        float scale, friction;
    };
    union {
        unsigned char opaque2a[4];
        struct BHGeometry *collision_geometry;
        const struct BHBsp *collision_bsp;
    };
    float collision_radius; /* verified +c0 broad-phase extent, 0040af20 */
    union {
        unsigned char opaque2b[4];
        struct BHWorldGeometry *world;
    };
    BHEntityCallback refresh;
    BHEntityStep step;
    BHEntityCallback query;
    BHEntity *query_next, *query_previous, *previous, *next;
    /* Recycled slots share the same storage across entity types. */
    union {
        unsigned char payload[636];
        BHEntityBindings bindings;
        BHActorFields actor;
        BHMenuFields menu;
        BHFlashFields flash;
        BHScriptEmitterFields emitter;
        BHAttachedSoundFields attached_sound;
        BHScriptDebrisFields fragment;
        BHPuffFields puff;
        BHWorldEntityFields placement;
        BHScriptLifetimeFields script_lifetime;
        BHScriptSensorFields sensor;
        BHScriptVMFields script_vm;
        BHExplosionFields explosion;
        BHEffectFields effect;
        BHAreaProjectileFields area_projectile;
        BHPulseFields pulse;
        BHProjectileFields projectile;
        BHTrailEmitterFields trail;
        BHTrailNodeFields trail_node;
        BHHitMarkerFields marker;
        BHTargetFields target;
        BHFlybyFields flyby;
        BHPlayerFields player;
        BHRemoteFields remote;
    };
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
