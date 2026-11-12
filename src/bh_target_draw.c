#include "bh_target_draw.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
typedef char target_layout
    [sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 && offsetof(BHEntity, opaque0) == 0x18 ? 1 : -1];
int bh_target_draw_state_read(const BHEntity *entity, BHTargetDrawState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&state->local_anchor, (const unsigned char *)entity + 0xf0, 12);
    memcpy(&state->number, (const unsigned char *)entity + 0x118, 4);
    memcpy(&state->flags, (const unsigned char *)entity + 0x120, 4);
    memcpy(&state->resource, (const unsigned char *)entity + 0x124, 4);
    memcpy(&state->owner, (const unsigned char *)entity + 0x128, 4);
    return 1;
}
int bh_target_draw_state_write(BHEntity *entity, const BHTargetDrawState *state)
{
    if (!entity || !state)
        return 0;
    memcpy((unsigned char *)entity + 0xf0, &state->local_anchor, 12);
    memcpy((unsigned char *)entity + 0x118, &state->number, 4);
    memcpy((unsigned char *)entity + 0x120, &state->flags, 4);
    memcpy((unsigned char *)entity + 0x124, &state->resource, 4);
    memcpy((unsigned char *)entity + 0x128, &state->owner, 4);
    return 1;
}
static float rounded(double value)
{
    float f;
    bh_round_float(&f, value);
    return f;
}
static void result(int *total, int r)
{
    if (r < 0)
        *total = -1;
    else if (!r && *total > 0)
        *total = 0;
}
static int rect(BHTargetDrawContext *ctx, unsigned long image, float x, float y, float w, float h, float u, float ue,
                float v, float ve, float depth)
{
    return bh_pickup_rect_queue(ctx->queue, 8, image, x, y, w, h, 0, 0, 1, u, ue, v, ve, depth);
}
static int greater(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a > b;
}
static int greater_equal(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a >= b;
}
static void marker(BHTargetDrawContext *ctx, float scale, float x, float y, float depth, int fallback, int *status)
{
    if (fallback)
    {
        x = rounded((double)scale * 320);
        y = rounded((double)scale * 27);
    }
    result(status, rect(ctx, ctx->marker_image, x, y, rounded((double)scale * (fallback ? 100 : 80)),
                        rounded((double)scale * (fallback ? 50 : 40)), 0, .5f, .5f, .218f, depth));
}
int bh_target_draw(BHTargetDrawContext *ctx, const BHEntity *entity)
{
    BHTargetDrawState state;
    BHVec3 model, angles;
    float basis[9], x, y, z, scale, depth, partial_x, partial_y, urow = .50390625f, vrow = .81640625f;
    double partial_z;
    long value, divisor, ds[8];
    int i, first = 7, count, status = 1, visible;
    unsigned long bits;
    if (!ctx || !ctx->queue || !ctx->camera || !ctx->view || !entity || ctx->screen_width <= 0 ||
        !bh_math_environment_supported())
        return -1;
    bh_target_draw_state_read(entity, &state);
    memcpy(basis, entity->basis, sizeof(basis));
    angles = entity->euler;
    /* Original X/Y offsets spill before position addition; Z does not. */
    partial_x = rounded(((double)basis[0] * state.local_anchor.x + (double)basis[3] * state.local_anchor.y) +
                        (double)basis[6] * state.local_anchor.z);
    partial_y = rounded(((double)basis[1] * state.local_anchor.x + (double)basis[4] * state.local_anchor.y) +
                        (double)basis[7] * state.local_anchor.z);
    partial_z = ((double)basis[5] * state.local_anchor.y + (double)basis[2] * state.local_anchor.x) +
                (double)basis[8] * state.local_anchor.z;
    model.x = rounded((double)entity->position.x + partial_x);
    model.y = rounded((double)entity->position.y + partial_y);
    model.z = rounded((double)entity->position.z + partial_z);
    result(&status, bh_draw_submit_resource(ctx->queue, 0, state.resource, &model, &angles, 1));
    if (state.owner != ctx->player || !state.number)
        return status;
    scale = rounded((double)ctx->screen_width * .0015625);
    value = state.number;
    if (value < 0)
    {
        urow = .59765625f;
        vrow = .91015625f;
        bits = 0UL - (unsigned long)value;
        memcpy(&value, &bits, 4);
    }
    x = entity->position.x;
    y = entity->position.y;
    z = entity->position.z;
    visible = bh_project_point_scale(ctx->camera, ctx->view, &x, &y, &z);
    depth = rounded((double)ctx->view->near_z - (-.01));
    if (!visible)
    {
        if (state.flags & 4)
            marker(ctx, scale, 0, 0, depth, 1, &status);
        return status;
    }
    result(&status, rect(ctx, ctx->atlas_image, x, rounded((double)y - (double)scale * 24), rounded((double)scale * 64),
                         rounded((double)scale * 14), .00390625f, .4921875f, urow, .1015625f, depth));
    if (state.flags & 4)
    {
        int inside = !greater((double)scale * -50, x) && greater_equal((double)scale * 690, x) &&
                     !greater((double)scale * -25, y) && greater_equal((double)scale * 505, y);
        marker(ctx, scale, x, y, depth, !inside, &status);
    }
    divisor = 10000000;
    for (i = 0; i < 8; ++i)
    {
        ds[i] = value / divisor;
        value -= divisor * ds[i];
        if (i < first && ds[i])
            first = i;
        divisor /= 10;
    }
    count = 8 - first;
    y = rounded((double)y - (double)scale * (-24));
    x = rounded((double)x - ((double)count * scale) * 4);
    for (i = first; i < 8; ++i)
    {
        float u = rounded((.5 - (double)ds[i] * (-8.0)) * .0078125);
        result(&status, rect(ctx, ctx->atlas_image, x, y, rounded((double)scale * 8), rounded((double)scale * 12), u,
                             .0546875f, vrow, .0859375f, depth));
        x = rounded((double)x - rounded((double)scale * (-8)));
    }
    return status;
}
