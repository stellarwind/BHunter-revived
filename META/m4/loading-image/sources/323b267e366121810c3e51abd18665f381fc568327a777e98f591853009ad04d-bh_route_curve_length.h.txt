#ifndef BH_ROUTE_CURVE_LENGTH_H
#define BH_ROUTE_CURVE_LENGTH_H
#include "bh_math.h"
/* Complete 00434810. Controls bind the original actor vectors in evaluator
   order: +120,+144,+150,+12c. Same order as existing BHCameraRouteState.curve.
   Sums THREE chord lengths through t=0,.25,.5,.75; never samples t=1. Each
   chord retains dx/dy but uses their float stores as multiplicands, retains
   dz*dz, and rounds the accumulating sum to float after each chord. Existing
   camera_curve owns evaluation; caller supplies PC53/nearest/masked exceptions
   and live moderate finite controls. Inputs are borrowed and never changed. */
float __cdecl bh_route_curve_length(const BHVec3 controls[4]);
#endif
