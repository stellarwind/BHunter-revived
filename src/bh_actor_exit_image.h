#ifndef BH_ACTOR_EXIT_IMAGE_H
#define BH_ACTOR_EXIT_IMAGE_H
#include "bh_portals.h"

/* The selector reads only +18 of each original 174-byte model record. These
   seven immutable adapter records represent source portal counts 0..6. They
   do not stand in for the game's model/resource registry. */
typedef struct BHActorExitModelRecord
{
    unsigned char unused_before[0x18];
    int portal_count;
    unsigned char unused_after[0x174 - 0x1c];
} BHActorExitModelRecord;
extern const BHActorExitModelRecord bh_actor_exit_counts[7];

typedef struct BHActorExitRoomImage
{
    unsigned char unused_before[0xf8];
    int count_record;
    unsigned char unused_middle[0x1cc - 0xfc];
    BHPortal portals[6];
} BHActorExitRoomImage;
typedef struct BHActorExitNeighborImage
{
    unsigned char unused[0x114];
    unsigned long flags;
} BHActorExitNeighborImage;

int __cdecl bh_actor_choose_exit_core(void *actor, void *room_image, void *preferred_image, int excluded);
#endif
