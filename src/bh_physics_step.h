#ifndef BH_PHYSICS_STEP_H
#define BH_PHYSICS_STEP_H
#include "bh_physics_response.h"
#include "bh_physics_effects.h"
typedef struct BHPhysicsStepBody
{
    BHEntity *entity;
    unsigned long generation;
    BHPhysicsState *physics;
    BHGeometry *geometry;                       /* own original+bc collision geometry */
    unsigned long model_index;                  /* retained adapter index; actor mesh is geometry */
    const BHMat34 *target_world, *target_local; /* actual rigid actor pose/inverse */
    BHWorldGeometry *world;
    float mass, friction;
    unsigned long flags; /* retained adapter field, not an actor enable gate */
} BHPhysicsStepBody;
typedef struct BHPhysicsStepWorld
{
    BHWorldGeometry *world;
    float mass;
    BHEntity *response;   /* world owner: +48 per-tick translation and query receiver */
    unsigned short index; /* verified packet/world index */
} BHPhysicsStepWorld;
typedef struct BHPhysicsStepPair
{
    BHGeometry *source, *target;
    BHSweepQuery sweep;
} BHPhysicsStepPair;
typedef enum BHPhysicsStepError
{
    BH_PHYSICS_STEP_OK,
    BH_PHYSICS_STEP_ARGUMENT,
    BH_PHYSICS_STEP_MEMORY,
    BH_PHYSICS_STEP_BINDING,
    BH_PHYSICS_STEP_COLLISION,
    BH_PHYSICS_STEP_CONTACT,
    BH_PHYSICS_STEP_MUTATION,
    BH_PHYSICS_STEP_BUSY,
    BH_PHYSICS_STEP_EFFECT
} BHPhysicsStepError;
typedef struct BHPhysicsStepContext
{
    BHEntityPool *pool;
    BHPhysicsStepBody *bodies;
    unsigned long body_count;
    const BHPhysicsStepWorld *worlds;
    unsigned long world_count;
    BHGeometry *const *models;
    unsigned long model_count;
    BHWorldQuery broad;
    BHEntity **actors;
    BHPhysicsStepPair *pairs;
    unsigned long pair_count;
    BHCollisionCandidate *candidates;
    unsigned long candidate_capacity, candidate_count;
    BHSceneAllocator allocator;
    BHPhysicsEffects *effects;
    BHVec3 zero; /* explicit original00495e30 shared vector, MUST remain zero */
    int fallback, busy;
    BHPhysicsStepError error;
    BHSweepHit first, slide;
    BHWorldHit first_selected, selected;
    unsigned long steps, misses, contacts, rollbacks, first_events, second_events;
} BHPhysicsStepContext;
/* Source owner adapter. Bodies/model identities/geometry topology are stable
   until dispose; state/matrix values may change between steps. Unique real
   geometry pairs own prebuilt sweep workspaces, shared across placements.
   Broad/candidate storage is owned once; stepping allocates only actual effects.
   The shared BHPhysicsState is authoritative and is the SAME component passed
   to actual response-query owners such as bh_debris_bind. Caller synchronizes
   external teleports into that component. No callbacks are fabricated here. */
int __cdecl bh_physics_step_init(BHPhysicsStepContext *, BHEntityPool *, BHPhysicsStepBody *, unsigned long,
                                 const BHPhysicsStepWorld *, unsigned long, BHGeometry *const *, unsigned long,
                                 BHPhysicsEffects *, int fallback, const BHSceneAllocator *);
/* Count-zero roster adapter. Scene ring/world bindings and geometry topology of
   surviving identities must be unchanged. Transfers unchanged sweep workspaces
   and scene validation storage; replaces actor/candidate arrays transactionally.
   Drops removed geometry borrows before callers recycle collision slots. Caller
   must not step after a rejected in-place roster edit until sync succeeds. */
int __cdecl bh_physics_step_rebind(BHPhysicsStepContext *, BHPhysicsStepBody *, unsigned long, BHGeometry *const *,
                                   unsigned long);
int __cdecl bh_physics_step_dispose(BHPhysicsStepContext *);
/* Count-zero boundary views, for actual original common physics fields only. */
int __cdecl bh_physics_step_read_entity(BHPhysicsState *, const BHEntity *);
int __cdecl bh_physics_step_publish(BHPhysicsStepBody *);
/* Whole004124d0. Return1 complete (including original ignored effect failures),
   0 source binding/numerical/lifetime/dependency failure with error. Side effects
   already performed remain visible. No playback, provider setup or game loop. */
int __cdecl bh_physics_step(BHPhysicsStepContext *, BHEntity *, float dt);
#endif
