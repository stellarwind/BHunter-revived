#include "bh_hud.h"
#include "../../Graphics/Geometry/bh_screen_quad.h"
#include "../../Math/bh_round.h"
#include <math.h>
#include <float.h>

static int finite_vector(const BHVec3 *vector)
{
    return _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static int fail(BHHudContext *context, BHHudError e)
{
    if (context)
        context->error = e;
    return 0;
}
int bh_hud_setup(BHHudContext *context, const BHHudPlayer *hud_player)
{
    BHAttachmentContext *attachment_context;
    BHRadarFrame *radar_frame;
    BHVec3 base;
    double nz, cx, cy, cz;
    long unit;
    int result;
    if (!context || !hud_player || !(attachment_context = context->attachment) || !(radar_frame = context->frame) || attachment_context->frame != radar_frame || !attachment_context->queue)
        return fail(context, BH_HUD_ARGUMENT);
    if (!finite_vector(&hud_player->position) || !finite_vector(&hud_player->right) || !finite_vector(&hud_player->up) ||
        !finite_vector(&hud_player->forward) || !_finite(hud_player->yaw))
        return fail(context, BH_HUD_NUMERICAL);
    context->error = BH_HUD_OK;
    context->attempted = context->queued = context->dropped = 0;

    /* First three stores precede all basis construction in the raw body.
       Constants here are binary32, distinct from the later double offsets. */
    bh_round_float(&base.x, ((double)hud_player->forward.x * .8f - (double)hud_player->up.x * -.1f) + hud_player->position.x);
    bh_round_float(&base.y, ((double)hud_player->position.y - (double)hud_player->up.y * -.1f) - (double)hud_player->forward.y * -.8f);
    bh_round_float(&base.z, ((double)hud_player->position.z - (double)hud_player->up.z * -.1f) - (double)hud_player->forward.z * -.8f);
    radar_frame->axis_x = hud_player->right;
    bh_round_float(&radar_frame->axis_y.x, (double)hud_player->forward.x * .707f - (double)hud_player->up.x * .707f);
    bh_round_float(&radar_frame->axis_y.y, (double)hud_player->forward.y * .707f - (double)hud_player->up.y * .707f);
    bh_round_float(&radar_frame->axis_y.z, (double)hud_player->forward.z * .707f - (double)hud_player->up.z * .707f);
    bh_round_float(&radar_frame->normal.x, ((double)hud_player->up.x + hud_player->forward.x) * -.707f);
    bh_round_float(&radar_frame->normal.y, ((double)hud_player->forward.y + hud_player->up.y) * -.707f);
    nz = ((double)hud_player->up.z + hud_player->forward.z) * -.707f;
    bh_round_float(&radar_frame->normal.z, nz);
    /* FST retains normal.z and center.x/z, while center.y is stored/reloaded.
       These distinctions are lost in the historical decompiler expression. */
    cx = (double)base.x - (double)radar_frame->normal.x * -.02;
    cy = (double)base.y - (double)radar_frame->normal.y * -.02;
    cz = (double)base.z - nz * -.02;
    bh_round_float(&context->world_center.x, cx);
    bh_round_float(&context->world_center.y, cy);
    bh_round_float(&context->world_center.z, cz);
    bh_round_float(&radar_frame->origin.x, (cx - (double)radar_frame->axis_x.x * .043) - (double)radar_frame->axis_y.x * -.083);
    bh_round_float(&radar_frame->origin.y,
                   ((double)context->world_center.y - (double)radar_frame->axis_x.y * .043) - (double)radar_frame->axis_y.y * -.083);
    bh_round_float(&radar_frame->origin.z, (cz - (double)radar_frame->axis_x.z * .043) - (double)radar_frame->axis_y.z * -.083);
    bh_round_float(&radar_frame->yaw_x.x, cos((double)hud_player->yaw));
    radar_frame->yaw_x.y = 0;
    bh_round_float(&radar_frame->yaw_x.z, -sin((double)hud_player->yaw));
    bh_round_float(&radar_frame->yaw_y.x, sin((double)hud_player->yaw));
    radar_frame->yaw_y.y = 0;
    bh_round_float(&radar_frame->yaw_y.z, cos((double)hud_player->yaw));

    if (hud_player->flat_raw)
    {
        unit = attachment_context->viewport_width / 80;
        radar_frame->unit = unit;
        context->screen_diameter = unit * 16;
        radar_frame->screen_radius = unit * 8;
        radar_frame->screen_y = unit * 9;
        radar_frame->screen_x = attachment_context->viewport_width - unit * 9;
        result = bh_screen_quad_queue(attachment_context->queue, 8, context->background_image, (float)radar_frame->screen_x, (float)radar_frame->screen_y,
                                      (float)context->screen_diameter, (float)context->screen_diameter, 0, BH_HUD_ANCHOR_RIGHT, 1);
        ++context->attempted;
        if (result > 0)
            ++context->queued;
        else if (!result)
            ++context->dropped;
        else
            context->error = BH_HUD_QUEUE;
    }
    radar_frame->flat = hud_player->flat_raw != 0;
    if (hud_player->root)
    {
        attachment_context->player_position = hud_player->position;
        attachment_context->yaw = hud_player->yaw;
        if (!bh_attachment_render(attachment_context, hud_player->root) && context->error == BH_HUD_OK)
            context->error = BH_HUD_ATTACHMENT;
        bh_attachment_reset(hud_player->root);
    }
    return context->error == BH_HUD_OK;
}
