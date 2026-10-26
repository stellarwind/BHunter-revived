#ifndef BH_SCRIPT_ACTOR_QUERY_H
#define BH_SCRIPT_ACTOR_QUERY_H
#include "bh_script_actor_draw.h"
typedef struct BHScriptActorContext {
 BHEntityPool *pool;BHActorRouteContext *route;BHScriptActorDrawContext *draw;
 unsigned long pulse_resource;
} BHScriptActorContext;
typedef struct BHScriptActorQueryPacket {
 long event;BHVec3 position,velocity,acceleration;
} BHScriptActorQueryPacket;
/* Count-zero persistent binding for actual callback ABI. Context and all real
   owners remain borrowed until cleanup. Does not construct an actor. */
int __cdecl bh_script_actor_bind(BHScriptActorContext *,BHEntity *);
BHScriptActorContext *__cdecl bh_script_actor_context(BHEntity *);
void __cdecl bh_script_actor_query(BHEntity *,void *); /* whole004376e0 */
void __cdecl bh_script_actor_cleanup(BHEntity *); /* adapter to actual435720 */
#endif
