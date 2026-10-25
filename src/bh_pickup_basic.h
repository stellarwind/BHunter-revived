#ifndef BH_PICKUP_BASIC_H
#define BH_PICKUP_BASIC_H
#include "bh_pickup.h"

/* Whole00441b80 with explicit borrowed owner and returned-entity convenience.
   The real pool/world/resource table/render binding outlive every pickup.
   Unlike00441a00 this always uses variant0 and the default draw, even type9;
   resource0 is retained. Bind through bh_pickup_render_bind before spawning.
   Phase/rotation use the admitted PC53/nearest math profile. Fade is copied as
   raw bits. Invalid source bindings fail; original startup/global ABI is not
   supplied. The no-room recycled link words are explicitly cleared for safety. */
BHEntity *__cdecl bh_pickup_basic_spawn(BHPickupContext *,const BHVec3 *,
    long type,long remaining,long offer,long token,float fade);
#endif
