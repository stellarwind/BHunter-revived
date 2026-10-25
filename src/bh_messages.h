#ifndef BH_MESSAGES_H
#define BH_MESSAGES_H
#include "bh_entities.h"

typedef enum BHMessageError {
    BH_MESSAGE_OK,BH_MESSAGE_ARGUMENT,BH_MESSAGE_RING_SIZE,
    BH_MESSAGE_ARENA_FULL,BH_MESSAGE_PANEL_BINDING
} BHMessageError;
/* Source ownership of original 004d0488 ring (8 x 512) and 004ce458 arena
   (8192 bytes). The panel entity and its live +cc method identity are borrowed
   from the as-yet-separate panel lifecycle, not created by this queue. */
typedef struct BHMessageState {
    char ring[8][512];
    char arena[8192];
    unsigned long arena_used;
    long ring_start,ring_count,last_index;
    long display_ticks,display_phase;
    BHEntity *panel_entity;
    const unsigned long *panel_step_identity;
    BHMessageError error;
} BHMessageState;
/* Source initialization of original static ring/arena state: empty counters,
   arena start and original last index -1 at 0048fb9c. */
void __cdecl bh_message_state_init(BHMessageState *);
/* Complete valid-string 00442a30 ring/arena effects. A string longer than one
   ring slot is rejected rather than reproducing the original adjacent-memory
   overflow. On arena exhaustion, ring/display mutations remain and result 0.
   Panel +e8 is clamped to3 only for verified identity 00442950 and value>3.
   Missing panel binding is explicitly reported after queue mutations. */
int __cdecl bh_message_enqueue(BHMessageState *,const char *text);
#endif
