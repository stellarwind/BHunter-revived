#ifndef BH_EFFECT_NETWORK_H
#define BH_EFFECT_NETWORK_H
#include "bh_network_transport.h"
typedef struct BHEffectWorldIndex {BHWorldGeometry *world;unsigned short index;}BHEffectWorldIndex;
typedef struct BHEffectNetworkContext {
 BHNetworkTransport *transport;const BHEffectWorldIndex *world_indices;unsigned long world_count,peers;
 unsigned char packet[272];unsigned long bytes,packets;int error;
} BHEffectNetworkContext;
/* Count-zero exact packet9 call-site bridge to actual codec/transport owners.
   Direction is the original input vector (already normalized at kind4 sites).
   Registry and native owner are borrowed. NULLworld returns without sending. */
int __cdecl bh_effect_network_emit(BHEffectNetworkContext *,const BHEntity *,unsigned long world_offset,unsigned long sync_offset,unsigned char kind,const BHVec3 *direction);
#endif
