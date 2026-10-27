#include "bh_hud.h"
#include "bh_screen_quad.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>

static int finite_vector(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int fail(BHHudContext *c, BHHudError e)
{
    if (c)
        c->error = e;
    return 0;
}
int __cdecl bh_hud_setup(BHHudContext *c, const BHHudPlayer *p)
{
    BHAttachmentContext *a;
    BHRadarFrame *f;
    BHVec3 base;
    double nz, cx, cy, cz;
    long unit;
    int result;
    if (!c || !p || !(a = c->attachment) || !(f = c->frame) || a->frame != f || !a->queue)
        return fail(c, BH_HUD_ARGUMENT);
    if (!finite_vector(&p->position) || !finite_vector(&p->right) || !finite_vector(&p->up) ||
        !finite_vector(&p->forward) || !_finite(p->yaw))
        return fail(c, BH_HUD_NUMERICAL);
    c->error = BH_HUD_OK;
    c->attempted = c->queued = c->dropped = 0;

    /* First three stores precede all basis construction in the raw body.
       Constants here are binary32, distinct from the later double offsets. */
    bh_round_float(&base.x, ((double)p->forward.x * .8f - (double)p->up.x * -.1f) + p->position.x);
    bh_round_float(&base.y, ((double)p->position.y - (double)p->up.y * -.1f) - (double)p->forward.y * -.8f);
    bh_round_float(&base.z, ((double)p->position.z - (double)p->up.z * -.1f) - (double)p->forward.z * -.8f);
    f->axis_x = p->right;
    bh_round_float(&f->axis_y.x, (double)p->forward.x * .707f - (double)p->up.x * .707f);
    bh_round_float(&f->axis_y.y, (double)p->forward.y * .707f - (double)p->up.y * .707f);
    bh_round_float(&f->axis_y.z, (double)p->forward.z * .707f - (double)p->up.z * .707f);
    bh_round_float(&f->normal.x, ((double)p->up.x + p->forward.x) * -.707f);
    bh_round_float(&f->normal.y, ((double)p->forward.y + p->up.y) * -.707f);
    nz = ((double)p->up.z + p->forward.z) * -.707f;
    bh_round_float(&f->normal.z, nz);
    /* FST retains normal.z and center.x/z, while center.y is stored/reloaded.
       These distinctions are lost in the historical decompiler expression. */
    cx = (double)base.x - (double)f->normal.x * -.02;
    cy = (double)base.y - (double)f->normal.y * -.02;
    cz = (double)base.z - nz * -.02;
    bh_round_float(&c->world_center.x, cx);
    bh_round_float(&c->world_center.y, cy);
    bh_round_float(&c->world_center.z, cz);
    bh_round_float(&f->origin.x, (cx - (double)f->axis_x.x * .043) - (double)f->axis_y.x * -.083);
    bh_round_float(&f->origin.y,
                   ((double)c->world_center.y - (double)f->axis_x.y * .043) - (double)f->axis_y.y * -.083);
    bh_round_float(&f->origin.z, (cz - (double)f->axis_x.z * .043) - (double)f->axis_y.z * -.083);
    bh_round_float(&f->yaw_x.x, cos((double)p->yaw));
    f->yaw_x.y = 0;
    bh_round_float(&f->yaw_x.z, -sin((double)p->yaw));
    bh_round_float(&f->yaw_y.x, sin((double)p->yaw));
    f->yaw_y.y = 0;
    bh_round_float(&f->yaw_y.z, cos((double)p->yaw));

    if (p->flat_raw)
    {
        unit = a->viewport_width / 80;
        f->unit = unit;
        c->screen_diameter = unit * 16;
        f->screen_radius = unit * 8;
        f->screen_y = unit * 9;
        f->screen_x = a->viewport_width - unit * 9;
        result = bh_screen_quad_queue(a->queue, 8, c->background_image, (float)f->screen_x, (float)f->screen_y,
                                      (float)c->screen_diameter, (float)c->screen_diameter, 0, 0, 1);
        ++c->attempted;
        if (result > 0)
            ++c->queued;
        else if (!result)
            ++c->dropped;
        else
            c->error = BH_HUD_QUEUE;
    }
    f->flat = p->flat_raw != 0;
    if (p->root)
    {
        a->player_position = p->position;
        a->yaw = p->yaw;
        if (!bh_attachment_render(a, p->root) && c->error == BH_HUD_OK)
            c->error = BH_HUD_ATTACHMENT;
        bh_attachment_reset(p->root);
    }
    return c->error == BH_HUD_OK;
}
