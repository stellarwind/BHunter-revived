#ifndef BH_ACTOR_PORTAL_IMAGE_H
#define BH_ACTOR_PORTAL_IMAGE_H
#include "bh_portals.h"

/* Original helper input is room+e8; its portal array starts at +e4.
   These local ABI images borrow only the copied portal records. */
typedef struct BHActorPortalImage
{
    unsigned char unused[0xe4];
    BHPortal portals[6];
} BHActorPortalImage;
void __cdecl bh_actor_portal_normal_core(void *geometry_image, int index, float *out);
void __cdecl bh_actor_portal_center_core(void *geometry_image, int index, float *out);
#endif
