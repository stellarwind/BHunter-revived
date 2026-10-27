#ifndef BH_ACTOR_LAUNCH_H
#define BH_ACTOR_LAUNCH_H
#include "bh_actor_construct.h"
#include "bh_network_transport.h"
#include "bh_navigation.h"
typedef struct BHActorLaunchContext
{
    BHActorConstructContext *actors;
    BHNetworkTransport *transport;
    const BHNavigation *navigation;
    BHPacketStats *stats;
    unsigned long *good;
    long mode;
    unsigned short tick;
    unsigned char packet[28];
    BHPacketWriter writer;
    BHPacketReader reader;
    unsigned long sent, received;
    HRESULT send_result;
    int error;
} BHActorLaunchContext;
/* Whole0043bca0 /00426d50. Launch consumes two real shared CRT draws even
   when mode3 suppresses type0/3. Mode2 quantizes token/speed and sends type10
   using actual position/checksum/transport owners, then constructs locally.
   Receive restores the borrowed route tick after constructor failure as well.
   Transport can be a real NULL-peer context; no sessions are created here. */
BHEntity *__cdecl bh_actor_launch(BHActorLaunchContext *, BHWorldGeometry *, unsigned long type, long token,
                                  float speed, long death_tag);
int __cdecl bh_actor_launch_receive(BHActorLaunchContext *, void *, unsigned long size, unsigned long capacity);
#endif
