#ifndef BH_VM_QUERIES_H
#define BH_VM_QUERIES_H
#include "bh_debris_response.h"
#include "bh_script_lifetime.h"
#include "bh_world_geometry.h"
typedef struct BHBlockDirectDamagePacket {
  long event;
  float amount;
  BHEntity *source;
} BHBlockDirectDamagePacket;
typedef struct BHBlockDamageContext {
  const BHWorldGeometry *world;
  BHEntity *player;
} BHBlockDamageContext;
typedef struct BHBlockDamageState {
  float health;
  BHEntity *owner;
} BHBlockDamageState;
/* Count-zero binding to real entity/world/health owners, not a placed-world
   constructor. State corresponds to original +148/+14c. World supplies actual
   original min/max bounds. Persistent context uses private payload+600. */
int __cdecl bh_block_damage_bind(BHEntityPool *, BHEntity *,
                                 BHBlockDamageContext *,
                                 const BHBlockDamageState *, int player_only);
int __cdecl bh_block_damage_read(const BHEntity *, BHBlockDamageState *);
void __cdecl bh_block_damage_query(BHEntity *, void *);        /*whole0040fe80*/
void __cdecl bh_block_player_damage_query(BHEntity *, void *); /*whole00410020*/
typedef struct BHRadarQueryContext {
  BHEntityPool *pool;
  BHEntity *head;
  unsigned long count;
  BHScriptLifetimeContext *lifetime;
  unsigned long retired_sound;
  int error;
} BHRadarQueryContext;
/* Bind real list member at head; original +ec(previous),+f0(next),+11c(event
   removal ID),+1b8(watched owner), private payload+596(context). List/pool and
   lifetime/sound bindings must outlive every member. This constructor adapter
   carries zero original count; whole radar constructor remains unrestored. */
int __cdecl bh_radar_query_bind(BHRadarQueryContext *, BHEntity *,
                                BHEntity *watched, long remove_id);
void __cdecl bh_radar_entity_query(BHEntity *, void *); /*whole0043c220*/
#endif
