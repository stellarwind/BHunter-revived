#include "bh_hud_flat.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
#include <stddef.h>

typedef char flat_weapon_ammo_offset[offsetof(BHHudWeaponAmmo, amount) == 0x10c ? 1 : -1];
static int fail(BHHudFlatContext *c, BHHudFlatError e)
{
    if (c)
        c->error = e;
    return 0;
}
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static void rect(BHHudFlatContext *c, unsigned long anchor, unsigned long image, double x, double y, float width, float height, float u,
                 float ue, float v, float ve)
{
    float sx = stored(x), sy = stored(y), depth = stored((double)c->hud->frame->near_z + .01);
    int result =
        bh_pickup_rect_queue(c->hud->attachment->queue, 8, image, sx, sy, width, height, 0, anchor, 1, u, ue, v, ve, depth);
    ++c->attempted;
    if (result > 0)
        ++c->queued;
    else if (!result)
        ++c->dropped;
    else
        c->error = BH_HUD_FLAT_QUEUE;
}
static void gauge(BHHudFlatContext *c, float amount, unsigned char flags, float scale, double x, float width,
                  float background_height, double half_background_height, float top, float bottom, float background_u)
{
    double remainder, raw_height;
    float height, v, ve;
    rect(c, BH_HUD_ANCHOR_RIGHT, c->atlas_image, x, (double)scale * top + half_background_height, width, background_height, background_u,
         .1171875f, .00390625f, .6953125f);
    if (amount >= 0 && ((double)amount < .8 || (flags & 8)))
    {
        remainder = 1.0 - amount;
        raw_height = ((double)scale * remainder) * 150.0;
        height = stored(raw_height);
        v = stored(.00390625 - (double)amount * -.6953125);
        ve = stored(remainder * .6953125);
        rect(c, BH_HUD_ANCHOR_RIGHT, c->atlas_image, x, raw_height * .5 + ((double)bottom - remainder * 150.0) * scale, width, height,
             .87890625f, .1171875f, v, ve);
    }
}
static float digit_u(long digit)
{
    return stored((.5 - (double)digit * -8.0) * .0078125);
}
static void ammo(BHHudFlatContext *c, long amount, unsigned char selected, int secondary, float scale,
                 float digit_width, float digit_height, float bottom, float x_offset)
{
    long hundreds = amount / 100, remainder = amount % 100, tens = remainder / 10;
    float v;
    unsigned long anchor = secondary ? BH_HUD_ANCHOR_RIGHT : BH_HUD_ANCHOR_LEFT;
    double half_width = ((double)scale * 12) * .5, half_height = ((double)scale * 18) * .5;
    double y = ((double)c->viewport_height - bottom) + half_height, early_y;
    static const float digit_x[2][3] = {{112, 124, 136}, {552, 564, 576}};
    double x, early_offset;
    float icon_height, icon_width, icon_v;
    /* Hundreds/tens retain scale*25 and scale*60. Units/secondary reload
       the cached binary32 bottom/x-offset; half dimensions remain double. */
    early_y = secondary ? y : ((double)c->viewport_height - (double)scale * 25) + half_height;
    early_offset = secondary ? 0 : (double)scale * 60;
    v = secondary ? .09765625f : .00390625f;
    if (hundreds)
    {
        x = (double)scale * digit_x[secondary][0] - early_offset + half_width;
        rect(c, anchor, c->atlas_image, x, early_y, digit_width, digit_height, digit_u(hundreds), .0546875f, v, .0859375f);
    }
    if (hundreds || tens)
    {
        x = (double)scale * digit_x[secondary][1] - early_offset + half_width;
        rect(c, anchor, c->atlas_image, x, early_y, digit_width, digit_height, digit_u(tens), .0546875f, v, .0859375f);
    }
    x = (double)scale * digit_x[secondary][2] - (secondary ? 0.0 : x_offset) + half_width;
    rect(c, anchor, c->atlas_image, x, y, digit_width, digit_height, digit_u(remainder % 10), .0546875f, v, .0859375f);
    icon_height = stored((double)scale * 38);
    icon_width = stored((double)scale * 96);
    icon_v = stored((.5 - (double)((long)selected - (secondary ? 5 : 0)) * -25.0) * .0078125);
    rect(c, anchor, c->weapon_icons_image,
         (double)scale * (secondary ? 456 : 148) - (secondary ? 0.0 : x_offset) + ((double)scale * 96) * .5,
         ((double)c->viewport_height - icon_height) + ((double)scale * 38) * .5, icon_width, icon_height,
         secondary ? .50390625f : .00390625f, .4921875f, icon_v, .1875f);
}
static void counter(BHHudFlatContext *c, long amount, unsigned char countdown, int second, float scale,
                    float label_width, float label_height, float top, double raw_top, double label_x, float digit_width,
                    double digit_half_width)
{
    long divisor, digit, seen = 0;
    float x, step, width;
    rect(c, 0, c->atlas_image, label_x,
         (second ? (double)top - (double)scale * -15.0 : raw_top) + ((double)scale * 14) * .5, label_width,
         label_height, .00390625f, .4921875f, second ? .39453125f : .28515625f, .1015625f);
    if (countdown & 8)
        return;
    x = stored((double)scale * 336);
    step = stored((double)scale * -8);
    width = stored((double)scale * 8);
    /* __CIpow(10,6) and __ftol are verified CRT, reused rather than counted. */
    for (divisor = (long)pow(10.0, 6.0); divisor >= 1; divisor /= 10)
    {
        digit = amount / divisor;
        amount -= divisor * digit;
        seen += digit;
        if (seen || divisor == 1)
            rect(c, 0, c->atlas_image, (double)x + (double)width * .5,
                 (second ? (double)top - (double)scale * -16.0 : (double)top + scale) + digit_half_width, width,
                 digit_width, digit_u(digit), .0546875f, second ? .91015625f : .19140625f, .0859375f);
        x = stored((double)x - step);
    }
}
int __cdecl bh_hud_flat_draw(BHHudFlatContext *c, BHHudStatusState *s)
{
    const BHAttachmentContext *a;
    float scale, bar_width, bar_height, offset, digit_height, digit_width, bottom, x_offset, label_width, label_height,
        top;
    double raw_scale, raw_bar_height, half_bar_height, bar_x, label_x, raw_top;
    if (!c || !s || !c->hud || !c->hud->frame || !(a = c->hud->attachment) || !a->queue || !s->weapons ||
        s->primary >= s->weapon_count || s->secondary >= s->weapon_count || !s->weapons[s->primary] ||
        !s->weapons[s->secondary])
        return fail(c, BH_HUD_FLAT_ARGUMENT);
    c->error = BH_HUD_FLAT_OK;
    c->attempted = c->queued = c->dropped = 0;
    if (!_finite(s->gauge_a) || !_finite(s->gauge_b) || !_finite(c->hud->frame->near_z))
        return fail(c, BH_HUD_FLAT_NUMERICAL);
    raw_scale = (double)a->viewport_width * .0015625;
    scale = stored(raw_scale);
    raw_bar_height = raw_scale * 150;
    bar_height = stored(raw_bar_height);
    half_bar_height = raw_bar_height * .5;
    bar_width = stored((double)scale * 20);
    offset = stored((double)scale * 24);
    bar_x = ((double)a->viewport_width - offset) + ((double)scale * 20) * .5;
    gauge(c, s->gauge_a, s->flags, scale, bar_x, bar_width, bar_height, half_bar_height, 300, 450, .62890625f);
    gauge(c, s->gauge_b, s->flags, scale, bar_x, bar_width, bar_height, half_bar_height, 130, 280, .75390625f);
    digit_height = stored((double)scale * 18);
    digit_width = stored((double)scale * 12);
    bottom = stored((double)scale * 25);
    x_offset = stored((double)scale * 60);
    ammo(c, s->weapons[s->primary]->amount, s->primary, 0, scale, digit_width, digit_height, bottom, x_offset);
    ammo(c, s->weapons[s->secondary]->amount, s->secondary, 1, scale, digit_width, digit_height, bottom, x_offset);
    label_height = stored((double)scale * 14);
    label_width = stored((double)scale * 64);
    raw_top = (double)c->viewport_height - (double)scale * 34;
    top = stored(raw_top);
    label_x = ((double)scale * 64) * .5 + (double)scale * 272;
    counter(c, s->counter_a, s->countdown_a, 0, scale, label_width, label_height, top, raw_top, label_x, digit_width,
            ((double)scale * 12) * .5);
    counter(c, s->counter_b, s->countdown_b, 1, scale, label_width, label_height, top, raw_top, label_x, digit_width,
            ((double)scale * 12) * .5);
    if (s->countdown_a)
        --s->countdown_a;
    if (s->countdown_b)
        --s->countdown_b;
    if (!s->countdown_b && s->pending)
        s->countdown_b = 16;
    return c->error == BH_HUD_FLAT_OK;
}
