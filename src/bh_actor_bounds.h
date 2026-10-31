#ifndef BH_ACTOR_BOUNDS_H
#define BH_ACTOR_BOUNDS_H
#include "bh_math.h"

/* Only these original 004340f0 input fields are read. The prefix is unused;
   this is a local bounds adapter, never a replacement world/global owner. */
typedef struct BHActorBoundsImage
{
    unsigned char unused[0x1b0];
    BHVec3 minimum, maximum;
} BHActorBoundsImage;

#ifdef __cplusplus
extern "C"
{
#endif
    void __cdecl bh_actor_bounds_clamp(const void *bounds_image, void *point);
#ifdef __cplusplus
}
#endif
#endif
