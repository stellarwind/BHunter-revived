#ifndef BH_SCENE_BINDINGS_H
#define BH_SCENE_BINDINGS_H
#include "bh_entity_schedule.h"
#include "bh_render_resources.h"

/* Source adapter, not a recovered constructor. Each entry supplies the
   placement's real resource handle/flags and the two borrowed entity heads.
   004084a0 supplies world->owner (+1c8); the +124 successor is the already
   constructed scene ring. Portal neighbors remain a separate relation. */
typedef struct BHSceneBindingInput {
    BHWorldGeometry *world;
    BHEntity *actors,*pickups;
    unsigned long resource,resource_flags;
    BHVec3 angles;
} BHSceneBindingInput;

typedef enum BHSceneBindingsError {
    BH_BINDINGS_OK,BH_BINDINGS_ARGUMENT,BH_BINDINGS_TOPOLOGY,
    BH_BINDINGS_RESOURCE
} BHSceneBindingsError;

typedef struct BHSceneBindings {
    BHWorldGeometry *first,*cached;
    unsigned long scene_count;
    BHPickupRoom *rooms;
    BHEntityScheduleRoom *scheduled;
    BHEntityScheduleContext *schedule_context;
    BHSceneBindingsError error;
} BHSceneBindings;

/* Zero-initialize bindings before first use. Caller owns input worlds, entity
   lists, resources, schedule context and output arrays for the entire scene
   lifetime. Initializes neither model
   resources nor gameplay entities. The world ring must already have been
   built and matched. Failure does not publish partially bound output. */
int __cdecl bh_scene_bindings_init(BHSceneBindings *bindings,
    const BHSceneBindingInput *inputs,unsigned long count,
    BHPickupRoom *rooms,BHEntityScheduleRoom *scheduled,
    BHRenderResources *resources,BHEntityScheduleContext *schedule_context);

BHEntityScheduleRoom *__cdecl bh_scene_bindings_schedule(
    const BHSceneBindings *bindings,const BHWorldGeometry *world);
void __cdecl bh_scene_bindings_dispose(BHSceneBindings *bindings);
#endif
