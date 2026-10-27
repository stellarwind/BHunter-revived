#ifndef BH_CALLBACK_HELPERS_H
#define BH_CALLBACK_HELPERS_H
#include "bh_entities.h"
#include "bh_network_codec.h"
/* Whole event owners; ordinary opaque payload offsets retain original x86 ABI.
   Packets and entities are borrowed. Source NULL checks reject before access. */
void __cdecl bh_actor_target_removed(BHEntity *entity, void *packet);  /* 00402e20 */
void __cdecl bh_effect_target_removed(BHEntity *entity, void *packet); /* 00407180 */
void __cdecl bh_marker_query(BHEntity *entity, void *packet);          /* 00407b00 */
void __cdecl bh_pickup_target_removed(BHEntity *entity, void *packet); /* 00442330 */
/* 00436670, factor first in original ABI. PC53 intermediate arithmetic;
   exact source/destination aliases supported. Partial vector overlap excluded. */
void __cdecl bh_vec3_interpolate(float factor, BHVec3 *out, const BHVec3 *a, const BHVec3 *b);
/* 00423a10 using the existing source reader context. A short buffer is a source
   rejection without cursor/output mutation. Original valid input consumes3 bytes. */
int __cdecl bh_packet_read_vector(BHPacketReader *reader, BHVec3 *out, float scale);
#endif
