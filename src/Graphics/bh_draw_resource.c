#include "bh_draw_resource.h"
#include "../Math/bh_rotation.h"
#include <float.h>
#include <string.h>

static const BHMat34 identity = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};

static int fail(BHDrawResourceContext *context, BHDrawResourceError error)
{
    if (context)
        context->error = error;
    return 0;
}

static int draw(BHDrawResourceContext *context, const BHDrawResource *resource, double x, double y, double z, double ry,
                double rx, double rz, float modulation, unsigned int depth)
{
    unsigned long i, first;
    BHMat34 rotation;
    BHVec3 origin, end, offset;
    BHSpriteOrientedVertex *path;
    float distance;
    double exact_distance;
    if (!resource || depth > 32)
        return fail(context, BH_DRAW_RESOURCE_TOPOLOGY);
    switch (resource->tag)
    {
    case BH_RESOURCE_TAG_MODEL:
        if (!resource->data.model.instance)
            return fail(context, BH_DRAW_RESOURCE_ARGUMENT);
        if (!bh_model_instance_draw(resource->data.model.instance, x, y, z, ry, rx, rz, modulation))
        {
            return fail(context, BH_DRAW_RESOURCE_MODEL);
        }
        ++context->dispatched;
        if (!resource->data.model.instance->drawn)
            ++context->culled;
        return 1;
    case BH_RESOURCE_TAG_LOD:
        if (resource->data.lod.count > 5 || !_finite(resource->data.lod.radius) || resource->data.lod.radius < 0)
            return fail(context, BH_DRAW_RESOURCE_ARGUMENT);
        /* FST [depth] is non-popping: the near comparison still uses the
           extended plane result; far and threshold comparisons reload the
           binary32 stored depth. */
        exact_distance = ((((double)context->camera->planes[0].normal.z * z + (double)context->camera->planes[0].normal.x * x) +
                           (double)context->camera->planes[0].normal.y * y) -
                          (double)context->camera->planes[0].offset);
        distance = (float)exact_distance;
        if ((double)context->config->near_z - resource->data.lod.radius > exact_distance ||
            distance > (double)context->config->far_z + resource->data.lod.radius)
        {
            ++context->culled;
            return 1;
        }
        for (i = 0; i < resource->data.lod.count; ++i)
        {
            if (!_finite(resource->data.lod.thresholds[i]))
                return fail(context, BH_DRAW_RESOURCE_ARGUMENT);
            if (distance <= resource->data.lod.thresholds[i])
                break;
        }
        if (i == resource->data.lod.count)
        {
            ++context->culled;
            return 1;
        }
        if (!resource->data.lod.children[i])
            return fail(context, BH_DRAW_RESOURCE_TOPOLOGY);
        return draw(context, resource->data.lod.children[i], x, y, z, ry, rx, rz, modulation, depth + 1);
    case BH_RESOURCE_TAG_BILLBOARD:
        if (!context->billboard || !resource->data.billboard.image)
            return fail(context, BH_DRAW_RESOURCE_ARGUMENT);
        if (!bh_sprite_billboard_draw(context->billboard, resource->data.billboard.image, (float)x, (float)y, (float)z,
                                      resource->data.billboard.width, resource->data.billboard.height,
                                      resource->data.billboard.angle, resource->data.billboard.flags & 255UL,
                                      modulation))
            return fail(context, BH_DRAW_RESOURCE_BILLBOARD);
        ++context->dispatched;
        if (!context->billboard->emitted)
            ++context->culled;
        return 1;
    case BH_RESOURCE_TAG_RIBBON:
        if (!context->oriented || !context->oriented->path || !resource->data.ribbon.image)
            return fail(context, BH_DRAW_RESOURCE_ARGUMENT);
        if (context->oriented->used > context->oriented->capacity || context->oriented->capacity - context->oriented->used < 2)
            return fail(context, BH_DRAW_RESOURCE_CAPACITY);
        if (!_finite(resource->data.ribbon.local_end.x) || !_finite(resource->data.ribbon.local_end.y) ||
            !_finite(resource->data.ribbon.local_end.z) || !_finite(resource->data.ribbon.start_width) ||
            !_finite(resource->data.ribbon.end_width))
            return fail(context, BH_DRAW_RESOURCE_ARGUMENT);
        rotation = identity;
        bh_mat34_rotate_y((float)ry, &rotation);
        bh_mat34_rotate_x((float)rx, &rotation);
        bh_mat34_rotate_z((float)rz, &rotation);
        bh_mat34_transform_point(&rotation, &resource->data.ribbon.local_end, &offset);
        origin.x = (float)x;
        origin.y = (float)y;
        origin.z = (float)z;
        end.x = (float)((double)offset.x + x);
        end.y = (float)((double)offset.y + y);
        end.z = (float)((double)offset.z + z);
        first = context->oriented->used;
        path = context->oriented->path + first;
        memset(path, 0, 2 * sizeof(*path));
        bh_mat34_transform_point(&context->camera->projected, &origin, &path[0].point);
        bh_mat34_transform_point(&context->camera->projected, &end, &path[1].point);
        if (first)
        {
            path[0].previous = path - 1;
            path[-1].next = path;
        }
        path[0].next = path + 1;
        path[1].previous = path;
        path[0].flags = path[1].flags = resource->data.ribbon.flags;
        path[0].texture_start = path[1].texture_start = modulation;
        path[0].width = resource->data.ribbon.start_width;
        path[1].width = resource->data.ribbon.end_width;
        path[0].texture_end = (float)first;
        path[1].texture_end = (float)(first + 1);
        context->oriented->used = first + 2;
        context->oriented->image = resource->data.ribbon.image;
        if (!bh_sprite_oriented_draw(context->oriented))
            return fail(context, BH_DRAW_RESOURCE_RIBBON);
        ++context->dispatched;
        return 1;
    /* Unsigned jump-table bounds check falls through to the common epilogue
       for every tag outside 0..3, including negative signed bit patterns. */
    default:
        return 1;
    }
}

int bh_draw_resource(BHDrawResourceContext *context, const BHDrawResource *resource, double x, double y, double z,
                     double rotate_y, double rotate_x, double rotate_z, float modulation)
{
    if (!context || !resource || !context->camera || !context->config || !_finite(x) || !_finite(y) || !_finite(z) ||
        !_finite(rotate_y) || !_finite(rotate_x) || !_finite(rotate_z) || !_finite(modulation))
        return fail(context, BH_DRAW_RESOURCE_ARGUMENT);
    context->error = BH_DRAW_RESOURCE_OK;
    context->dispatched = 0;
    context->culled = 0;
    return draw(context, resource, x, y, z, rotate_y, rotate_x, rotate_z, modulation, 0);
}
