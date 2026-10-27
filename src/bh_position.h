#ifndef BH_POSITION_H
#define BH_POSITION_H
#include "bh_navigation.h"

typedef struct BHPackedPositions
{
    const unsigned char *bytes;
    unsigned long size, cursor;
    unsigned long rejected, accepted; /* original accepted/rejected packet counters */
} BHPackedPositions;

typedef enum BHPositionResult
{
    BH_POSITION_ERROR = -1,
    BH_POSITION_REJECTED = 0,
    BH_POSITION_OK = 1
} BHPositionResult;

/* 00423840: one little-endian world index followed by three coordinate words.
   Navigation order and placements must remain live. Each coordinate is
   max*t + min*(1-t), t=(word-16384)/32768, with one binary32 output store.
   Values outside [16384,49152] extrapolate; do not clamp them to the AABB.
   Invalid index consumes only its word, increments rejected and decrements
   accepted modulo 2^32, preserving outputs. A valid item consumes eight bytes.
   Truncated data, invalid source metadata or unsupported FP state return ERROR
   without changing cursor/counters/outputs. Finite bounds may overflow a float
   result as in the original masked PC53 environment. No stream ownership. */
BHPositionResult __cdecl bh_position_read(BHPackedPositions *stream, const BHNavigation *navigation,
                                          BHWorldGeometry **world, BHVec3 *point);
#endif
