#ifndef BH_PLAYER_INPUT_EVENTS_H
#define BH_PLAYER_INPUT_EVENTS_H
#include "bh_player_input.h"
#include "bh_entities.h"
/* Complete004757e0. Kind4 record retains every field except its active tag;
   count becomes max(count,index+1). Caller supplies index0..255. */
void __cdecl bh_player_input_bind_kind4(BHPlayerInputList *,long index);
/* Complete004215c0 with explicit original004dfb28 list owner. Class at actor+fc
   selects1; real installed query receives event15 in a40-byte stack packet.
   Remaining packet words are original unspecified payload, not zero defaults.
   Successor at+f0 is read after callback, so mutation is observable. Returns1
   on completion,0 for missing callback/cycle bound. visited reports traversed
   members, including nonselected classes; callbacks already run are not undone.
   Actor lifetimes and accessible next links must remain valid through calls.
   No callback is supplied or installed by this module. */
int __cdecl bh_player_input_broadcast(BHEntity *head,unsigned long limit,unsigned long *visited);
#endif
