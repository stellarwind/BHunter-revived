#include "bh_model_instance.h"
#include "../../Math/bh_rotation.h"
#include "../../Math/bh_round.h"
#include <float.h>
#include <string.h>
static const BHMat34 identity = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};

static int fail(BHModelInstanceContext *context, BHModelInstanceError error)
{
    if (context)
        context->error = error;
    return 0;
}
static int draw_fail(BHModelInstanceContext *context, const BHModelDrawContext *draw)
{
    context->color = draw->color;
    context->faces_marked += draw->faces_marked;
    context->polygons_emitted += draw->polygons_emitted;
    return fail(context, BH_MODEL_INSTANCE_DRAW);
}

/* 0045aa60 uses the six input doubles directly for sphere tests. In
   particular, truncating the position to the model's float translation here
   changes visibility for centres close to a plane. */
static double distance_to(const BHViewPlane *view_plane, double x, double y, double z)
{
    return (((double)view_plane->normal.x * x + (double)view_plane->normal.y * y) + (double)view_plane->normal.z * z) - view_plane->offset;
}

int bh_model_instance_draw(BHModelInstanceContext *context, double x, double y, double z, double rotate_y, double rotate_x,
                           double rotate_z, float opacity)
{
    unsigned long mask = 63, bit, slot, next_frame;
    double depth, d;
    float reference_radius;
    int grayscale;
    BHMat34 world_to_model, model_camera;
    BHVec3 world_reference;
    BHViewCamera view;
    BHModelDrawContext draw;
    BHModelDrawLitContext lit;
    if (!context || !context->model || !context->cache || context->cache->model != context->model || !context->camera || !context->config || !context->clip ||
        !context->emit || !_finite(x) || !_finite(y) || !_finite(z) || !_finite(rotate_y) || !_finite(rotate_x) ||
        !_finite(rotate_z) || !_finite(opacity) || !_finite(context->model->radius) || context->model->radius < 0 ||
        !_finite(context->config->near_z) || !_finite(context->config->far_z))
        return fail(context, BH_MODEL_INSTANCE_ARGUMENT);
    if ((context->light_config && context->light_config->enabled) || context->point_enabled)
    {
        if (!context->light_config || (context->point_enabled && (!context->lights || !context->light_reference)))
            return fail(context, BH_MODEL_INSTANCE_ARGUMENT);
    }
    context->error = BH_MODEL_INSTANCE_OK;
    context->drawn = 0;
    reference_radius = context->model->radius;
    depth = distance_to(context->camera->planes, x, y, z);
    if (depth < (double)context->config->near_z - reference_radius || (double)context->config->far_z + reference_radius < depth)
        return 1;
    if ((double)context->config->near_z + reference_radius < depth)
        mask = 0x3e;
    if (depth < (double)context->config->far_z - reference_radius)
        mask -= 2;
    for (bit = 4, slot = 2; bit <= 32; bit *= 2, ++slot)
    {
        d = distance_to(context->camera->planes + slot, x, y, z);
        if (reference_radius < d)
            return 1;
        if (d < -(double)reference_radius)
            mask -= bit;
    }
    d = (double)opacity * -255.0;
    if (!_finite(d) || d < -2147483648.0 || d > 2147483647.0)
        return fail(context, BH_MODEL_INSTANCE_NUMERICAL);
    grayscale = (int)d;
    next_frame = context->frame + 1;
    if (!next_frame)
        return fail(context, BH_MODEL_INSTANCE_NUMERICAL);
    context->model->transform = identity;
    bh_mat34_translate_local((float)x, (float)y, (float)z, &context->model->transform);
    bh_mat34_rotate_y((float)rotate_y, &context->model->transform);
    bh_mat34_rotate_x((float)rotate_x, &context->model->transform);
    bh_mat34_rotate_z((float)rotate_z, &context->model->transform);
    context->color = context->base_color - (unsigned long)grayscale * 0x10101UL;
    context->opacity = opacity;
    context->frame = next_frame;
    bh_mat34_multiply(&context->camera->view, &context->model->transform, &model_camera);
    if (!bh_view_camera_build(context->config, &model_camera, &view))
        return fail(context, BH_MODEL_INSTANCE_CAMERA);
    if (!bh_model_draw_context_init(&draw, context->model, context->cache, &view, context->clip, context->frame, context->texture_mask, context->color,
                                    context->emit, context->emit_user))
        return fail(context, BH_MODEL_INSTANCE_DRAW);
    if (!context->light_config || (!context->light_config->enabled && !context->point_enabled))
    {
        if (!bh_model_draw_unlit(&draw, 0, mask))
            return draw_fail(context, &draw);
    }
    else
    {
        bh_mat34_inverse_rigid(&context->model->transform, &world_to_model);
        if (context->point_enabled)
        {
            /* The original transforms a fixed model-local reference to world
               space, then prepares each borrowed light using the inverse. */
            bh_mat34_transform_point(&context->model->transform, context->light_reference, &world_reference);
            context->lights->world_to_model = world_to_model;
            if (!bh_light_prepare(context->lights, &world_reference, reference_radius))
                return fail(context, BH_MODEL_INSTANCE_LIGHT);
        }
        if (!bh_model_draw_lit_context_init(&lit, &draw, context->lights, context->light_config, &world_to_model, context->point_enabled,
                                            opacity, context->base_color) ||
            !bh_model_draw_lit(&lit, 0, mask))
            return draw_fail(context, &draw);
    }
    context->color = draw.color;
    context->last_camera_view = context->camera->view;
    context->faces_marked += draw.faces_marked;
    context->polygons_emitted += draw.polygons_emitted;
    context->drawn = 1;
    return 1;
}
