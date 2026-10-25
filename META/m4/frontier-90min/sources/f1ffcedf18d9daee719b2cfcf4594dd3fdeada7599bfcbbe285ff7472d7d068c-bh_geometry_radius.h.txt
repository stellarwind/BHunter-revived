#ifndef BH_GEOMETRY_RADIUS_H
#define BH_GEOMETRY_RADIUS_H
#include "bh_bsp.h"
/* 0044e3f0 over explicit decoded BSP binding: first node minimum-corner
   norm versus float-rounded maximum-corner norm. Minimum norm remains
   double under admitted PC53 scope; strict greater keeps it, else selects
   the rounded maximum norm. This differs from BSP root_radius maxabs.
   Source guards return0 for NULL/empty/nonfinite input or unsupported FP.
   Only first node is consumed; decoded BSP owns all storage. */
double __cdecl bh_geometry_radius(const BHBsp *bsp);
#endif
