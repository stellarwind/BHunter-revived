#include "bh_project_scale.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>
int __cdecl bh_project_point_scale(const BHMainCamera *camera, const BHViewConfig *view, float *x, float *y, float *z)
{
    BHVec3 world, projected;
    if (!camera || !view || !x || !y || !z || !bh_math_environment_supported())
        return 0;
    /* Original MOV copies all inputs before any output write. */
    memcpy(&world.x, x, 4);
    memcpy(&world.y, y, 4);
    memcpy(&world.z, z, 4);
    bh_mat34_transform_point(&camera->projected, &world, &projected);
    /* The original FCOMP's C0 branch rejects unordered as well as less. */
    if (_isnan(projected.z) || _isnan(view->near_z) || projected.z < view->near_z)
        return 0;
    bh_round_float(x, (double)projected.x / projected.z);
    bh_round_float(y, (double)projected.y / projected.z);
    bh_round_float(z, (double)camera->focal / projected.z);
    return 1;
}
