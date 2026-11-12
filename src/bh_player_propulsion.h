#ifndef BH_PLAYER_PROPULSION_H
#define BH_PLAYER_PROPULSION_H

#include "bh_math.h"

enum
{
    BH_PLAYER_NOZZLE_COUNT = 4,
    BH_PROPULSION_CHANNELS = BH_PLAYER_NOZZLE_COUNT * 3,
    BH_PROPULSION_WIRE_SCALE = 32 /* Five fractional bits per unsigned channel. */
};

extern const BHVec3 bh_player_nozzles[BH_PLAYER_NOZZLE_COUNT];

#endif
