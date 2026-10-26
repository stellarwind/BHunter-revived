#ifndef BH_IMPACT_RING_H
#define BH_IMPACT_RING_H
#include "bh_contact_effects.h"
/* Whole 00407930. Borrow the actual entity's BHImpactState at +e8 and
   caller-owned queue/resource/camera yaw. No allocation or lifetime mutation.
   Return queue result (1 queued, 0 full, -1 invalid). PC53 finite inputs with
   nonzero duration; source validation rejects malformed input before enqueue. */
int __cdecl bh_impact_ring_draw(BHDrawQueue *,unsigned long texture,
    const BHEntity *,const float *camera_yaw);
#endif
