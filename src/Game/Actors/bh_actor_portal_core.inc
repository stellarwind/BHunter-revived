#include "bh_actor_portal_image.h"
#include <stddef.h>

static_assert(offsetof(BHActorPortalImage, portals) == 0xe4, "BHActorPortalImage.portals offset");
static_assert(sizeof(BHPortal) == 60, "BHPortal size");

void bh_actor_portal_normal_core(void *geometry_image, int index, float *out)
{
    switch (((BHActorPortalImage *)geometry_image)->portals[index].direction)
    {
    case 0:
        out[0] = 1;
        out[1] = 0;
        out[2] = 0;
        return;
    case 1:
        out[0] = 0;
        out[1] = 1;
        out[2] = 0;
        return;
    case 2:
        out[0] = 0;
        out[1] = 0;
        out[2] = 1;
        return;
    case 4:
        out[0] = -1;
        out[1] = 0;
        out[2] = 0;
        return;
    case 5:
        out[0] = 0;
        out[1] = -1;
        out[2] = 0;
        return;
    case 6:
        out[0] = 0;
        out[1] = 0;
        out[2] = -1;
        return;
    }
}
