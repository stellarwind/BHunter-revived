#ifndef BH_REMOTE_POSITION_H
#define BH_REMOTE_POSITION_H
#include "bh_remote_packets.h"
/* Whole00426330. Three32-byte timestamp/pose/world banks retain modular16
   ordering, equal-stamp and intermediate/old arrivals, including original
   oldest-packet pending-counter quirk. Real packed-position source decoder,
   reciprocal room membership, signed byte angles and angular fmod wrap feed
   smoothed velocity/acceleration, original10/2 clamps and pow age decay.
   Typed room/navigation/entity owners remain borrowed. Source rejects missing
   reciprocal links, truncated packets and nonfinite/extreme damping after
   earlier effects. PC53 finite moderate data; no original/game code executes. */
int __cdecl bh_remote_position_packet(BHRemotePackets *,void *,unsigned long size,unsigned long capacity);
#endif
