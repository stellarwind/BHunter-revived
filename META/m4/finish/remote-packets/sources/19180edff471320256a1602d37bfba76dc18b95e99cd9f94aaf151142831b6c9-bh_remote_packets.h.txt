#ifndef BH_REMOTE_PACKETS_H
#define BH_REMOTE_PACKETS_H
#include "bh_remote_tick.h"
#include "bh_remote_query.h"
#include "bh_network_game.h"
typedef struct BHRemotePackets {
 BHEntityPool *pool;BHEntity **remote,**head;unsigned long *count;
 BHNetworkGame *network;BHRemoteQuery *query;BHRemoteRender *render;
 BHRemoteTick *tick;unsigned long *simulation_tick;long *local_slot;
 BHEntity *local;BHWorldGeometry **spawn_world;BHPickupRoom *rooms;
 unsigned long room_count,engine_sounds[8];const BHNavigation *navigation;
 unsigned long pending,processed,bad;BHPacketStats framing;BHPacketReader reader;
 unsigned long spawns,immediate,queued;int error;
} BHRemotePackets;
/* Whole00427920 and00427340. Each validates the real mutable packet envelope,
   retaining original pending/processed/bad counter quirks independently from
   codec diagnostics. Remote spawn initializes actual sparse fields, three pose
   banks,20-byte name, exact global/room lists and real query/tick callbacks.
   Original+e8 draw identity is data; bh_remote_entity_draw is its typed source
   callable binding, not an invented common+C8 callback. Local ack updates tick
   low16 and local slot. Stable caller-owned contexts/lists/pools/roster required.
   Shot receiver retains signed16 lag<61 (including future), type5 lifetime,
   immediate single-shot sync and bounded repeated shots/queued tail.
   Source guards reject truncated fields, invalid slot/type and occupied spawn
   slot rather than original overflow/leak. No original code or game playback. */
int __cdecl bh_remote_spawn_packet(BHRemotePackets *,void *,unsigned long size,unsigned long capacity);
int __cdecl bh_remote_shot_packet(BHRemotePackets *,void *,unsigned long size,unsigned long capacity);
int __cdecl bh_remote_entity_draw(BHRemotePackets *,BHEntity *);
#endif
