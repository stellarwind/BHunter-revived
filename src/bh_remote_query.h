#ifndef BH_REMOTE_QUERY_H
#define BH_REMOTE_QUERY_H
#include "bh_remote_prediction.h"
#include "bh_effect_network.h"
#include "bh_pickup.h"
typedef union BHRemoteQueryPacket {
    long words[10];
    float floats[10];
    struct
    {
        long event;
        float severity;
        BHEntity *attacker;
    } damage;
    struct
    {
        long event;
        BHEntity *actor;
    } removed;
} BHRemoteQueryPacket;
typedef struct BHRemoteQuery
{
    BHEntityPool *pool;
    BHPickupRoom *rooms;
    unsigned long room_count;
    BHEntity **head;
    unsigned long *count;
    BHEntity **remote;
    BHEntity *local;
    unsigned long local_id;
    const BHEffectWorldIndex *worlds;
    unsigned long world_count;
    BHNetworkTransport *transport;
    unsigned short tick;
    unsigned char packet[32];
    unsigned long wire_size, sends;
    HRESULT send_result;
    int error;
} BHRemoteQuery;
/* Whole00423e70. Events0/3/16/17/20: actual prediction/packet codec/transport,
   nine target-reference removals and dual-list/remote-slot/pool retirement.
   All bindings remain borrowed. No sound Stop or global query is added to
   retirement (the original owner calls neither). Output packet >=40bytes.
   Source guards reject bad bindings/list reciprocity before list mutation. */
int __cdecl bh_remote_query(BHRemoteQuery *, BHEntity *, BHRemoteQueryPacket *);
#endif
