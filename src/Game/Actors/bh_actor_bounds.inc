#include "bh_actor_bounds.h"
#include <stddef.h>

static_assert(offsetof(BHActorBoundsImage, minimum) == 0x1b0, "BHActorBoundsImage.minimum offset");
static_assert(offsetof(BHActorBoundsImage, maximum) == 0x1bc, "BHActorBoundsImage.maximum offset");
/* Byte-addressed inputs preserve the original VC5 unaligned-load codegen and
   unordered comparisons. Typed float arrays generate different instructions. */
#define LOW(i) (*(const float *)((const char *)&((const BHActorBoundsImage *)bounds_image)->minimum + 4 * (i)))
#define HIGH(i) (*(const float *)((const char *)&((const BHActorBoundsImage *)bounds_image)->maximum + 4 * (i)))
#define POINT(i) (*(float *)((char *)point + 4 * (i)))

void bh_actor_bounds_clamp(const void *bounds_image, void *point)
{
    if (POINT(0) < LOW(0))
        POINT(0) = LOW(0);
    else if (POINT(0) > HIGH(0))
        POINT(0) = HIGH(0);
    if (POINT(1) < LOW(1))
        POINT(1) = LOW(1);
    else if (POINT(1) > HIGH(1))
        POINT(1) = HIGH(1);
    if (POINT(2) < LOW(2))
        POINT(2) = LOW(2);
    else if (POINT(2) > HIGH(2))
        POINT(2) = HIGH(2);
}
