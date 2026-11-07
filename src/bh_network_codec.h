#ifndef BH_NETWORK_CODEC_H
#define BH_NETWORK_CODEC_H
#include "bh_world_geometry.h"
#include "bh_entities.h"
#include "bh_position.h"
typedef enum BHPacketError
{
    BH_PACKET_OK,
    BH_PACKET_ARGUMENT,
    BH_PACKET_CAPACITY,
    BH_PACKET_TRUNCATED,
    BH_PACKET_NUMERICAL,
    BH_PACKET_INDEX,
    BH_PACKET_CHECKSUM
} BHPacketError;
typedef struct BHPacketWriter
{
    unsigned char *data;
    unsigned long capacity, cursor;
    int finished;
    BHPacketError error;
} BHPacketWriter;
typedef struct BHPacketStats
{
    unsigned long accepted, rejected;
} BHPacketStats;
typedef struct BHPacketReader
{
    unsigned char *data;
    unsigned long size, cursor;
    BHPacketStats *stats;
    BHPacketError error;
} BHPacketReader;
/* Source buffer adapters for original 8-byte header: type, payload byte count,
   LE sender ushort, LE XOR checksum. Payload <=255; caller owns bytes/context.
   finish returns unpadded wire length; cursor includes zero padding to 4 bytes.
   These adapters do not implement sender 00423710 or DirectPlay transport.
   Append failures leave bytes/cursor unchanged. No appending after finish. */
int __cdecl bh_packet_begin(BHPacketWriter *writer, void *bytes, unsigned long capacity, unsigned char type,
                            unsigned short sender);
int __cdecl bh_packet_append(BHPacketWriter *writer, const void *bytes, unsigned long count);
int __cdecl bh_packet_finish(BHPacketWriter *writer, unsigned long *wire_size);
/* 004234d0: eight bytes (world index, three quantized coordinates). NULL world
   returns 0 without appending. Index is explicit original world +108 binding.
   Position normalization retains PC53, clamps below -.5 to -.5, but >=1.5 to
   binary32 1.49, then truncates 16384 + 32768*t into a LE ushort. Zero extents
   retain original masked unordered/infinite clamp behavior. */
int __cdecl bh_packet_write_position(BHPacketWriter *writer, const BHWorldGeometry *world, unsigned short world_index,
                                     const BHVec3 *position);
/* 00423660: shared retained denominator scale/128; real VC5 CRT __ftol to
   signed 64-bit, signed low EAX clamped [-128,127], three ordered bytes.
   Zero scale keeps masked conversion behavior; no guessed rounding helper. */
int __cdecl bh_packet_write_vector(BHPacketWriter *writer, const BHVec3 *vector, float scale);
/* Complete 004237a0. Received size may omit padding, but capacity must include
   writable space for the aligned declared length. Valid envelope resets cursor
   to 8, zeros padding and increments accepted; checksum failure then decrements
   accepted and increments rejected. Truncated envelopes are a bounded source
   rejection before mutation. Reader/stats are borrowed, initialize stats first. */
int __cdecl bh_packet_receive(BHPacketReader *reader, void *bytes, unsigned long size, unsigned long capacity,
                              BHPacketStats *stats);
/* Source bridge to already-restored 00423840 bh_position_read, using its real
   source navigation registry and original index/counter/interpolation behavior.
   Truncation retains cursor/counters/outputs as the existing bounded contract. */
int __cdecl bh_packet_read_position(BHPacketReader *reader, const BHNavigation *navigation, BHWorldGeometry **world,
                                    BHVec3 *position);
/* 004239a0: consumes one ushort. Full 32-bit local ID resolves local (even NULL), IDs 0..7
   resolve remote slots; absent remote returns 0 without counter changes. Larger
   nonlocal ID rejects and adjusts counters. No entity lifetime is acquired. */
int __cdecl bh_packet_read_actor(BHPacketReader *reader, unsigned long local_id, BHEntity *local,
                                 BHEntity *const remote[8], BHEntity **actor);
#endif
