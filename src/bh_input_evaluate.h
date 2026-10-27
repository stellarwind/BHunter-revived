#ifndef BH_INPUT_EVALUATE_H
#define BH_INPUT_EVALUATE_H
#include "bh_player_input.h"
/* Exact byte relationships of original current/previous/history globals:
   previous begins255 bytes after current; history starts510 bytes after it.
   Index255 deliberately aliases the next region, preserving original reads.
   Snapshot collection/device ownership is separate and unrecovered. */
typedef struct BHInputSnapshot
{
    unsigned char current[255], previous[255], history[32], history_head;
    float axes[8], mouse_axes[2];
    unsigned short pair3[2], pair4[2];
} BHInputSnapshot;
/* Complete00474020 pure binding evaluation/packet production. No device/API
   calls occur. full_state selects all key states instead of edge transitions.
   Returns1 success,0 invalid key/name/count/history or packet capacity. Source
   rejection preserves list/snapshot. Only history/head are modified on success.
   List and snapshot do not overlap. Packet size and record counts retain
   original byte wrapping. Snapshot collection is not a restored device owner. */
int __cdecl bh_input_evaluate(BHPlayerInputList *, BHInputSnapshot *, int full_state);
/* Complete004756b0: nonzero mode sets flags2 without evaluation; zero sets
   flags0 and evaluates full state using the actual owner above. */
int __cdecl bh_input_set_enabled(BHPlayerInputList *, BHInputSnapshot *, int mode);
#endif
