#include "bh_route_curve_length.h"
#include "bh_camera_curve.h"
#include "bh_actor_curve_length_core.inc"
float __cdecl bh_route_curve_length(const BHVec3 controls[4])
{
    BHActorCurveLengthImage image;
    image.first = controls[0];
    image.first_tangent = controls[1];
    image.last_tangent = controls[2];
    image.last = controls[3];
    return bh_actor_curve_length_core(&image);
}
