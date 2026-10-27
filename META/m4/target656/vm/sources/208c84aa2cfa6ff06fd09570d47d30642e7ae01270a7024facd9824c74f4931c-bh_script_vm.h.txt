#ifndef BH_SCRIPT_VM_H
#define BH_SCRIPT_VM_H
#include "bh_actor_launch.h"
#include "bh_script_actor_construct.h"
#include "bh_script_attached_sound.h"
#include "bh_script_motion.h"
#include "bh_script_sensor.h"
#include "bh_vm_queries.h"
#include "bh_rail_wagon.h"
#include "bh_pickup_basic.h"
typedef struct BHScriptVMActor {
 const BHScriptTimerActor *state;BHWorldGeometry *world;
 unsigned long orientation,resource;
 BHBlockDamageContext damage;
} BHScriptVMActor;
typedef struct BHScriptVMWaypoint {BHVec3 position,angles;float speed,delay;} BHScriptVMWaypoint;
typedef struct BHScriptVMState {
 BHEntity *next;long loaded,opcode,args[5],selectors[5],variable_sum;
} BHScriptVMState;
typedef struct BHScriptVMContext {
 BHScriptLifetimeContext *lifetime;BHScriptMotionContext *motion;
 BHScriptSensorContext *sensors;BHScriptEmitterContext *emitters;
 BHScriptExplosionContext *explosions;BHContactEffects *contacts;
 BHFlashFlareContext *flash;BHScriptActorConstructorContext *script_actors;
 BHTargetConstructContext *targets;BHPickupContext *pickups;
 BHActorLaunchContext *launch;BHRailWagonContext *rails;BHRadarQueryContext *radar;
 BHScriptVMActor *actors;unsigned long actor_count;
 BHScriptVMWaypoint *waypoints;unsigned long waypoint_count;
 long script_loading;unsigned long depth,executed;int error;
} BHScriptVMContext;
/* Whole00443ff0 all25 opcodes and factory004457c0. Actual resource, actor,
   world, timer-variable and callback owners are borrowed. Per-actor records
   explicitly relate semantic world geometry to real source entity storage.
   Context payload+576/580 is private. Radar callbacks synchronize the same
   real global vehicle list, including removals. No original globals/vtables.
   Cyclic activation depth, absent typed owners and malformed selectors reject
   explicitly after any earlier effects. Children retain original loaded flag.
   Unknown opcode still forwards the chain; allocation failure returns early.
   PC53 profile, moderate finite numeric state and stable callbacks required. */
int __cdecl bh_script_vm_bind(BHScriptVMContext *,BHEntity *,const BHScriptVMState *);
int __cdecl bh_script_vm_read(BHEntity *,BHScriptVMState *);
BHEntity *__cdecl bh_script_vm_create(BHScriptVMContext *,BHEntity **out,long opcode,const long args[5],const long selectors[5]);
void __cdecl bh_script_vm_query(BHEntity *,void *);
#endif
