#ifndef BH_COLLISION_BOX_H
#define BH_COLLISION_BOX_H
#include "bh_collision_model.h"
/* Whole0044b7a0. Caller initializes model once with the real collision owner.
   extent/padding are borrowed float3 inputs: lower=-padding,
   upper=extent-padding (both original float stores). Extent is nonnegative;
   finite moderate PC53 inputs. Padding may be asymmetric/signed. The six
   original plane constants are -the selected bound, including low planes.
   Do not "correct" the retained original expression to a centered-box policy.
   Source original unspecified bytes are defined zero. Failure leaves previous
   model/geometry ownership intact and reports its real BSP error. Replacing
   while geometry enumeration is active rejects. Dispose existing queries before
   success; successful build replaces owned storage and invalidates old borrows. */
int __cdecl bh_collision_box_build(BHCollisionModel *model,const BHVec3 *extent,const BHVec3 *padding);
#endif
