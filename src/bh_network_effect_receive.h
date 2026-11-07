#ifndef BH_NETWORK_EFFECT_RECEIVE_H
#define BH_NETWORK_EFFECT_RECEIVE_H
#include "bh_network_codec.h"
typedef union BHNetworkEffectPacket {
    long words[10];
    float floats[10];
} BHNetworkEffectPacket;
typedef struct BHNetworkEffectReceive
{
    const BHNavigation *navigation;
    BHEntityPool *pool;
    BHPacketStats *stats;
    unsigned long *good;
    BHPacketReader reader;
    BHNetworkEffectPacket packet;
    BHWorldGeometry *world;
    unsigned long dispatched;
    int error;
} BHNetworkEffectReceive;
/* Whole00427180. Writable padded packet storage stays borrowed synchronously.
   Actual checksum/position owners and real pool query dispatch are invoked.
   Source actor-byte>=8 consumes only that byte; accepted counter is retained.
   A real effect callback acknowledging compositeID by writing-1 moves exactly
   one accepted count to good. Query failure preserves earlier stream/counters.
   Original unused queryword3 is zeroed by this source boundary; no callback
   definition relies on it. Source truncation guards reject before an unsafe
   read. No transport receive, session, game loop or physical input occurs. */
int __cdecl bh_network_effect_receive(BHNetworkEffectReceive *, void *, unsigned long size, unsigned long capacity);
#endif
