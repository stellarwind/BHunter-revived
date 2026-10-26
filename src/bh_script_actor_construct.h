#ifndef BH_SCRIPT_ACTOR_CONSTRUCT_H
#define BH_SCRIPT_ACTOR_CONSTRUCT_H
#include "bh_script_actor_query.h"
typedef struct BHScriptActorConstructorContext {
 BHScriptActorContext callbacks;
 BHWorldGeometry *first_world;unsigned long world_count;
 unsigned long model_resources[4],sound_resource;
} BHScriptActorConstructorContext;
/* Whole0043bec0. Locate source world, acquire real entity, initialize/attach
   all fields and actual query/draw/tick callbacks, then real route promotion.
   Loaded resource handles and contexts remain borrowed until cleanup. The
   defined type domain is0..3; other values reject before allocation/RNG. */
BHEntity *__cdecl bh_script_actor_construct(BHScriptActorConstructorContext *,const BHVec3 *,unsigned long type);
/* Count-zero callback bridge/read API for actual original+e8 render slot. */
void __cdecl bh_script_actor_present(BHEntity *);
#endif
