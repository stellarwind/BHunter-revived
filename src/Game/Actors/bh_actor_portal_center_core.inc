#include "bh_actor_portal_image.h"
#include <stddef.h>

static_assert(offsetof(BHActorPortalImage, portals) == 0xe4, "BHActorPortalImage.portals offset");
static_assert(offsetof(BHPortal, plane) == 4, "BHPortal.plane offset");
static_assert(offsetof(BHPortal, scratch) == 40, "BHPortal.scratch offset");
static_assert(sizeof(BHPortal) == 60, "BHPortal size");

/* Complete 00433f70 source variant. The two aperture sums use the original
   single-precision stores; the integer plane remains on the x87 stack until
   the selected output store. Instruction scheduling still differs. */
void bh_actor_portal_center_core(void *geometry_image, int index, float *out)
{
    BHPortal *portal = &((BHActorPortalImage *)geometry_image)->portals[index];
    float u = (portal->scratch[1] + portal->scratch[0]) * .5f;
    float v = (portal->scratch[3] + portal->scratch[2]) * .5f;
    float plane = (float)portal->plane;
    switch (portal->direction)
    {
    case 0:
    case 4:
        out[0] = plane;
        out[1] = u;
        out[2] = v;
        return;
    case 1:
    case 5:
        out[0] = u;
        out[1] = plane;
        out[2] = v;
        return;
    case 2:
    case 6:
        out[0] = u;
        out[1] = v;
        out[2] = plane;
        return;
    }
}
