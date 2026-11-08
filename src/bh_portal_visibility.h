#ifndef BH_PORTAL_VISIBILITY_H
#define BH_PORTAL_VISIBILITY_H
#include "bh_math.h"
/* Visibility apertures touching the swept near-camera volume inherit their
   parent's window. They are not opaque geometry to discard at the near plane. */
double bh_portal_camera_radius(const BHMat34 *projected, float near_z, const float bounds[4], float motion);
int bh_portal_camera_touches(const BHVec3 corners[4], const BHVec3 *eye, double radius_squared);
#endif
