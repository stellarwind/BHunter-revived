#ifndef BH_PICKUP_H
#define BH_PICKUP_H
#include "bh_entities.h"
#include "bh_world_geometry.h"
#include "bh_sound_queue.h"

/* Original entity +ec..+120. The four list links have distinct ownership:
   room head at +12c and global head at 004dfb3c. */
typedef struct BHPickupState {
    BHEntity *room_next,*room_previous;
    float phase,fade,rotation;
    long type,variant,remaining,offer,token;
    unsigned long resource;
    BHEntity *global_next,*global_previous;
    BHWorldGeometry *room;
} BHPickupState;
typedef struct BHPickupRoom {
    BHWorldGeometry *world;
    BHEntity *actors; /* original room +128, linked by actor +f4 */
    BHEntity *pickups; /* original room +12c */
} BHPickupRoom;
typedef struct BHPickupTakePacket {
    long event,type,variant,amount,token,reserved;
    long accepted,consumed;
} BHPickupTakePacket;
typedef struct BHPickupContext {
    BHEntityPool *pool;
    BHWorldGeometry *first_world;
    unsigned long world_count;
    BHPickupRoom *rooms;
    unsigned long room_count;
    const unsigned long *resources; /* loaded table, 10 entries per variant */
    unsigned long resource_count;
    BHEntity *head;
    BHEntity **outside_actors; /* NULL-terminated original 004df278 sequence */
    BHSoundQueue *sound;
    unsigned long take_sound;
    BHVec3 sound_velocity;
    void *draw_user; /* borrowed renderer context for +e8 callback bridges */
    BHEntityStep default_draw,special_draw;
} BHPickupContext;

/* Callback owner and bindings outlive every spawned pickup. The loaded resource
   table and room actor bindings are borrowed. Spawn consumes two CRT rand calls
   after a successful pool allocation. The selected draw method must be bound;
   missing draw bindings fail before allocation or random consumption. */
BHEntity *__cdecl bh_pickup_spawn(BHPickupContext *context,const BHVec3 *position,
    long type,long variant,long remaining,long offer,long token,float fade);
BHPickupContext *__cdecl bh_pickup_context(BHEntity *entity);
BHPickupState *__cdecl bh_pickup_state(BHEntity *entity);
void __cdecl bh_pickup_tick(BHEntity *entity); /* 004415a0 */
void __cdecl bh_pickup_query(BHEntity *entity,void *packet); /* 00441570 */
void __cdecl bh_pickup_cleanup(BHEntity *entity); /* 004414d0 */
#endif
