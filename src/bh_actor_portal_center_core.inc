#include "bh_actor_portal_image.h"
#include <stddef.h>

typedef char center_portal_offset[offsetof(BHActorPortalImage, portals) == 0xe4 ? 1 : -1];
typedef char center_portal_plane[offsetof(BHPortal, plane) == 4 ? 1 : -1];
typedef char center_portal_aperture[offsetof(BHPortal, scratch) == 40 ? 1 : -1];
typedef char center_portal_stride[sizeof(BHPortal) == 60 ? 1 : -1];

#define CENTER_F(p, o) (*(float *)((char *)(p) + (o)))
#define CENTER_I(p, o) (*(int *)((char *)(p) + (o)))

/* Complete 00433f70 source variant. The two aperture sums use the original
   single-precision stores; the integer plane remains on the x87 stack until
   the selected output store. Instruction scheduling still differs. */
void __cdecl bh_actor_portal_center_core(void *geometry_image, int index, float *out)
{
    char *at = (char *)geometry_image + index * 60;
    float u = (CENTER_F(geometry_image, index * 60 + 0x110) +
               CENTER_F(geometry_image, index * 60 + 0x10c)) * .5f;
    float v = (CENTER_F(geometry_image, index * 60 + 0x118) +
               CENTER_F(geometry_image, index * 60 + 0x114)) * .5f;
    float plane = (float)CENTER_I(at, 0xe8);
    switch (CENTER_I(at, 0xe4))
    {
    case 0: case 4:
        out[0] = plane; out[1] = u; out[2] = v;
        return;
    case 1: case 5:
        out[0] = u; out[1] = plane; out[2] = v;
        return;
    case 2: case 6:
        out[0] = u; out[1] = v; out[2] = plane;
        return;
    }
}
#undef CENTER_F
#undef CENTER_I
