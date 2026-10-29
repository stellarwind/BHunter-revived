#include "bh_actor_portal_image.h"
#include <stddef.h>

typedef char actor_portal_offset[offsetof(BHActorPortalImage, portals) == 0xe4 ? 1 : -1];
typedef char actor_portal_stride[sizeof(BHPortal) == 60 ? 1 : -1];

void __cdecl bh_actor_portal_normal_core(void *geometry_image, int index, float *out)
{
    switch (*(int *)((unsigned char *)geometry_image + 0xe4 + index * 60))
    {
    case 0:
        out[0] = 1; out[1] = 0; out[2] = 0;
        return;
    case 1:
        out[0] = 0; out[1] = 1; out[2] = 0;
        return;
    case 2:
        out[0] = 0; out[1] = 0; out[2] = 1;
        return;
    case 4:
        out[0] = -1; out[1] = 0; out[2] = 0;
        return;
    case 5:
        out[0] = 0; out[1] = -1; out[2] = 0;
        return;
    case 6:
        out[0] = 0; out[1] = 0; out[2] = -1;
        return;
    }
}
