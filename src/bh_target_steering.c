#include "bh_target_steering.h"
#include "bh_actor_bounds.h"
#include "bh_actor_bounds.inc"
#include <string.h>

void __cdecl bh_world_clamp_point(const BHWorldGeometry *world, BHVec3 *point)
{
    struct
    {
        BHActorBoundsImage image;
        unsigned char tail[sizeof(BHVec3)];
    } bounds;
    unsigned long difference;
    long offset;
    unsigned char *local_point;
    bounds.image.minimum = world->minimum;
    bounds.image.maximum = world->maximum;
    /* Mirror any point/bounds overlap inside the local ABI image, so earlier
       coordinate stores affect later bound reads just as in 004340f0. */
    if ((unsigned long)point < (unsigned long)&world->maximum + sizeof(BHVec3) &&
        (unsigned long)point + sizeof(BHVec3) > (unsigned long)&world->minimum)
    {
        difference = (unsigned long)point - (unsigned long)&world->minimum;
        memcpy(&offset, &difference, sizeof(offset));
        local_point = (unsigned char *)&bounds + 0x1b0 + offset;
        memcpy(local_point, point, sizeof(*point));
        bh_actor_bounds_clamp(&bounds.image, local_point);
        memcpy(point, local_point, sizeof(*point));
        return;
    }
    bh_actor_bounds_clamp(&bounds.image, point);
}
