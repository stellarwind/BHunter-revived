#include "bh_hud_world.h"
#include "bh_round.h"
#include <float.h>
#include <stddef.h>
#include <math.h>

typedef char hud_weapon_ammo_offset[offsetof(BHHudWeaponAmmo, amount) == 0x10c ? 1 : -1];

static int vector(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int fail(BHHudWorldContext *c, BHHudWorldError error)
{
    if (c)
        c->error = error;
    return 0;
}

static void quad(BHHudWorldContext *c, unsigned long image, double horizontal, double vertical, float width,
                 float height, float opacity, float u, float ue, float v, float ve)
{
    const BHRadarFrame *f = c->hud->frame;
    BHVec3 center;
    int result;
    bh_round_float(&center.x, ((double)c->hud->world_center.x - (double)f->axis_x.x * horizontal) -
                                  (double)f->axis_y.x * vertical);
    bh_round_float(&center.y, ((double)c->hud->world_center.y - (double)f->axis_x.y * horizontal) -
                                  (double)f->axis_y.y * vertical);
    bh_round_float(&center.z, ((double)c->hud->world_center.z - (double)f->axis_x.z * horizontal) -
                                  (double)f->axis_y.z * vertical);
    result = bh_quad_queue(c->hud->attachment->queue, 3, image, &center, &f->axis_y, &f->normal, width, height, 0, 0,
                           opacity, u, ue, v, ve);
    ++c->attempted;
    if (result > 0)
        ++c->queued;
    else if (!result)
        ++c->dropped;
    else
        c->error = BH_HUD_WORLD_QUEUE;
}
static void gauge(BHHudWorldContext *c, float amount, unsigned char flags, double horizontal, float background_u)
{
    double remainder, placement;
    float stored_remainder, height, v, ve;
    quad(c, c->atlas_image, horizontal, -.094, .026f, .13f, 1, background_u, .1171875f, .00390625f, .6953125f);
    if (amount >= 0 && ((double)amount < .8 || (flags & 8)))
    {
        remainder = 1.0 - amount;
        bh_round_float(&stored_remainder, remainder);
        placement = .159 - (1.0 - (double)stored_remainder * .5) * .13;
        bh_round_float(&height, (double)stored_remainder * .13);
        bh_round_float(&v, .00390625 - (double)amount * -.6953125);
        bh_round_float(&ve, remainder * .6953125);
        quad(c, c->atlas_image, horizontal, -placement, .026f, height, 1, .87890625f, .1171875f, v, ve);
    }
}
static void ammo_digit(BHHudWorldContext *c, long digit, double horizontal, double vertical, float v)
{
    float u;
    bh_round_float(&u, (.5 - (double)digit * -8.0) * .0078125);
    quad(c, c->atlas_image, horizontal, vertical, .015454545617103577f, .0225f, .8f, u, .0546875f, v, .0859375f);
}
static void ammunition(BHHudWorldContext *c, long amount, unsigned char selected, int secondary)
{
    static const double horizontal[2][3] = {{-.12772727272727272, -.14318181818181816, -.15863636363636363},
                                            {-.2513636363636364, -.26681818181818184, -.2822727272727273}};
    double vertical = secondary ? -.06250000000000001 : -.10750000000000001;
    long hundreds = amount / 100, remainder = amount % 100, tens = remainder / 10;
    float v = secondary ? .09765625f : .00390625f, icon_v;
    if (hundreds)
        ammo_digit(c, hundreds, horizontal[secondary][0], vertical, v);
    if (hundreds || tens)
        ammo_digit(c, tens, horizontal[secondary][1], vertical, v);
    ammo_digit(c, remainder % 10, horizontal[secondary][2], vertical, v);
    bh_round_float(&icon_v, (.5 - (double)((long)selected - (secondary ? 5 : 0)) * -25.0) * .0078125);
    quad(c, c->weapon_icons_image, secondary ? -.18181818181818182 : -.22818181818181818,
         secondary ? -.0625 : -.10750000000000001, .12363636493682861f, .045f, .8f, secondary ? .50390625f : .00390625f,
         .4921875f, icon_v, .1875f);
}
static void counter(BHHudWorldContext *c, long amount, unsigned char countdown, int second)
{
    double vertical = second ? -.05166666666666667 : -.07666666666666667;
    long divisor, digit, seen = 0;
    float horizontal = -.289333343505859375f, u;
    quad(c, c->atlas_image, .3373333333333333, vertical, .08533333241939545f, .01666666753590107f, 1, .00390625f,
         .4921875f, second ? .39453125f : .28515625f, .1015625f);
    if (countdown & 8)
        return;
    /* Verified CRT __CIpow receives x=10,y=6, followed by __ftol. Reuse CRT
       pow and the truncating cast; neither helper is a gameplay target. */
    for (divisor = (long)pow(10.0, 6.0); divisor >= 1; divisor /= 10)
    {
        digit = amount / divisor;
        amount -= divisor * digit;
        seen += digit;
        if (seen || divisor == 1)
        {
            bh_round_float(&u, (.5 - (double)digit * -8.0) * .0078125);
            quad(c, c->atlas_image, -(double)horizontal, vertical, .01066666655242443f, .01666666753590107f, 1, u,
                 .0546875f, second ? .91015625f : .19140625f, .0859375f);
        }
        bh_round_float(&horizontal, (double)horizontal - (-.010666666666666666));
    }
}
int __cdecl bh_hud_world_draw(BHHudWorldContext *c, BHHudWorldState *state)
{
    const BHRadarFrame *f;
    if (!c || !state || !c->hud || !(f = c->hud->frame) || !c->hud->attachment || !c->hud->attachment->queue ||
        !state->weapons || state->primary >= state->weapon_count || state->secondary >= state->weapon_count ||
        !state->weapons[state->primary] || !state->weapons[state->secondary])
        return fail(c, BH_HUD_WORLD_ARGUMENT);
    if (!_finite(state->gauge_a) || !_finite(state->gauge_b) || !vector(&c->hud->world_center) || !vector(&f->axis_x) ||
        !vector(&f->axis_y) || !vector(&f->normal))
        return fail(c, BH_HUD_WORLD_NUMERICAL);
    c->error = BH_HUD_WORLD_OK;
    c->attempted = c->queued = c->dropped = 0;
    gauge(c, state->gauge_a, state->flags, .183, .62890625f);
    gauge(c, state->gauge_b, state->flags, -.081, .75390625f);
    ammunition(c, state->weapons[state->primary]->amount, state->primary, 0);
    ammunition(c, state->weapons[state->secondary]->amount, state->secondary, 1);
    counter(c, state->counter_a, state->countdown_a, 0);
    counter(c, state->counter_b, state->countdown_b, 1);
    if (state->countdown_a)
        --state->countdown_a;
    if (state->countdown_b)
        --state->countdown_b;
    if (!state->countdown_b && state->pending)
        state->countdown_b = 16;
    return c->error == BH_HUD_WORLD_OK;
}
