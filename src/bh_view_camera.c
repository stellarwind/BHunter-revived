#include "bh_view_camera.h"
#include "bh_round.h"
#include <string.h>
#include <math.h>
#include <float.h>
int __cdecl bh_view_reset_viewport(BHViewConfig *c, long width, long height)
{
    if (!c || width <= 0 || height <= 0)
        return 0;
    c->origin_x = c->origin_y = c->left = c->top = 0;
    c->width = c->right = (float)width;
    c->height = c->bottom = (float)height;
    return 1;
}
int __cdecl bh_view_config_init(BHViewConfig *c, long width, long height)
{
    BHViewConfig next;
    memset(&next, 0, sizeof(next));
    next.fov = 1.0471975803375244f;
    next.near_z = 1;
    next.far_z = 100000;
    if (!c || !bh_view_reset_viewport(&next, width, height))
        return 0;
    *c = next;
    return 1;
}
int __cdecl bh_view_clip_rect(BHViewConfig *c, float x, float y, float width, float height)
{
    float right, bottom;
    if (!c || !_finite(x) || !_finite(y) || !_finite(width) || !_finite(height))
        return 0;
    bh_round_float(&right, (double)x + width);
    bh_round_float(&bottom, (double)y + height);
    if (x < 0)
        x = 0;
    if (c->width < right)
        right = c->width;
    if (y < 0)
        y = 0;
    if (c->height < bottom)
        bottom = c->height;
    c->left = x;
    c->right = right;
    c->top = y;
    c->bottom = bottom;
    return 1;
}
static void plane(BHViewPlane *out, const BHMat34 *inverse, const BHVec3 *position, float x, float y, float z,
                  int normalize)
{
    BHVec3 local;
    local.x = x;
    local.y = y;
    local.z = z;
    bh_mat34_transform_direction(inverse, &local, &out->normal);
    if (normalize)
        bh_vec3_normalize(&out->normal);
    bh_round_float(&out->offset, ((double)out->normal.z * position->z + (double)out->normal.y * position->y) +
                                     (double)out->normal.x * position->x);
}
int __cdecl bh_view_camera_build(const BHViewConfig *c, const BHMat34 *camera, BHViewCamera *out)
{
    BHViewCamera next;
    BHVec3 local, point;
    unsigned long i, j;
    float half, offset_x, offset_y, cosine, sine, vertical;
    double focal, retained_x, retained_cosine;
    if (!c || !camera || !out || !_finite(c->fov) || c->fov <= 0 || c->fov >= 3.141592653589793 || !_finite(c->width) ||
        !_finite(c->height) || c->width <= 0 || c->height <= 0 || !_finite(c->origin_x) || !_finite(c->origin_y) ||
        !_finite(c->near_z) || !_finite(c->far_z) || c->near_z <= 0 || c->far_z < c->near_z)
        return 0;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)camera)[i]))
            return 0;
    memset(&next, 0, sizeof(next));
    next.projected = *camera;
    bh_round_float(&half, (double)c->fov * .5);
    focal = ((double)c->width * .5) / tan((double)c->fov * .5);
    retained_x = (double)c->origin_x - (double)c->width * -.5;
    bh_round_float(&offset_x, retained_x);
    bh_round_float(&offset_y, (double)c->origin_y - (double)c->height * -.5);
    bh_round_float(&next.focal, focal);
    retained_cosine = cos((double)half);
    bh_round_float(&cosine, retained_cosine);
    bh_round_float(&sine, -sin((double)half));
    for (j = 0; j < 4; ++j)
    {
        bh_round_float(&next.projected.m[0][j],
                       focal * camera->m[0][j] + (j ? offset_x : retained_x) * camera->m[2][j]);
        bh_round_float(&next.projected.m[1][j], focal * camera->m[1][j] + (double)offset_y * camera->m[2][j]);
    }
    bh_round_float(&vertical, (retained_cosine * c->width) / c->height);
    bh_mat34_inverse_rigid(camera, &next.inverse);
    local.x = local.y = local.z = 0;
    bh_mat34_transform_point(&next.inverse, &local, &next.position);
    plane(next.planes + BH_VIEW_NEAR, &next.inverse, &next.position, 0, 0, -1, 0);
    local.z = c->near_z;
    bh_mat34_transform_point(&next.inverse, &local, &point);
    bh_round_float(&next.planes[BH_VIEW_NEAR].offset, ((double)point.z * next.planes[BH_VIEW_NEAR].normal.z +
                                                       (double)point.y * next.planes[BH_VIEW_NEAR].normal.y) +
                                                          (double)point.x * next.planes[BH_VIEW_NEAR].normal.x);
    plane(next.planes + BH_VIEW_FAR, &next.inverse, &next.position, 0, 0, 1, 0);
    local.z = c->far_z;
    bh_mat34_transform_point(&next.inverse, &local, &point);
    bh_round_float(&next.planes[BH_VIEW_FAR].offset, ((double)point.z * next.planes[BH_VIEW_FAR].normal.z +
                                                      (double)point.y * next.planes[BH_VIEW_FAR].normal.y) +
                                                         (double)point.x * next.planes[BH_VIEW_FAR].normal.x);
    plane(next.planes + BH_VIEW_RIGHT, &next.inverse, &next.position, cosine, 0, sine, 0);
    plane(next.planes + BH_VIEW_LEFT, &next.inverse, &next.position, -cosine, 0, sine, 0);
    plane(next.planes + BH_VIEW_TOP, &next.inverse, &next.position, 0, -vertical, sine, 1);
    plane(next.planes + BH_VIEW_BOTTOM, &next.inverse, &next.position, 0, vertical, sine, 1);
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)&next.projected)[i]))
            return 0;
    for (i = 0; i < 6; ++i)
        if (!_finite(next.planes[i].normal.x) || !_finite(next.planes[i].normal.y) ||
            !_finite(next.planes[i].normal.z) || !_finite(next.planes[i].offset))
            return 0;
    *out = next;
    return 1;
}
