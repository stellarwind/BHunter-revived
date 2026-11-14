#include "../Entities/bh_field_access.h"
#include "bh_effect_puff.h"
#include "../../Math/bh_round.h"
#include <math.h>
#include <string.h>
static float rf(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static unsigned long advance(unsigned long x)
{
    return x + (((unsigned long)((long)x >> 13) * 3UL) ^ (x * 7UL));
}
static BHEffectPuffContext *context(BHEntity *entity)
{
    BHEffectPuffContext *ctx;
    memcpy(&ctx, &entity->bindings.effect_puff, 4);
    return ctx;
}
int bh_effect_puff_bind(BHEffectPuffContext *ctx, BHEntity *entity, int ring)
{
    if (!ctx || !ctx->queue || !entity)
        return 0;
    memcpy(&entity->bindings.effect_puff, &ctx, 4);
    entity->refresh = ring ? bh_effect_puff_ring_draw : bh_effect_puff_random_draw;
    return 1;
}
static void draw(BHEffectPuffContext *ctx, unsigned long image, const BHVec3 *p, const BHVec3 *up, const BHVec3 *n,
                 float w, float h, float angle, unsigned long control, float alpha)
{
    int r;
    ++ctx->attempted;
    r = bh_draw_oriented(ctx->queue, 3, image, p, up, n, w, h, angle, control, alpha);
    if (r > 0)
        ++ctx->queued;
    else if (r < 0)
        ctx->error = 1;
}
void bh_effect_puff_random_draw(BHEntity *entity, void *ignored)
{
    BHEffectPuffContext *ctx = context(entity);
    float alpha, size, angle;
    BHVec3 v, p, up = {0, -1, 0};
    unsigned long seed, image;
    int i;
    (void)ignored;
    if (!ctx || !(bh_field_f32(&entity->puff.delay) <= 0))
        return;
    alpha = rf((double)bh_field_f32(&entity->puff.lifetime) / bh_field_f32(&entity->puff.duration));
    size = rf((1.0 - alpha) * bh_field_f32(&entity->puff.size));
    memcpy(&seed, &entity->puff.seed, 4);
    memcpy(&image, &entity->puff.image, 4);
    for (i = 0; i < 48; ++i)
    {
        seed = advance(seed);
        v.x = (float)((long)((seed >> 16) & 32767) - 16384);
        seed = advance(seed);
        v.y = (float)((long)((seed >> 16) & 32767) - 16384);
        seed = advance(seed);
        v.z = (float)((long)((seed >> 16) & 32767) - 16384);
        seed = advance(seed);
        bh_vec3_normalize(&v);
        p.x = rf((double)v.x * size + entity->position.x);
        p.y = rf((double)v.y * size + entity->position.y);
        p.z = rf((double)v.z * size + entity->position.z);
        angle = rf((double)((seed >> 16) & 32767) * .0001953125);
        draw(ctx, image, &p, &up, &v, size, size, angle, 8, alpha);
    }
}
void bh_effect_puff_ring_draw(BHEntity *entity, void *ignored)
{
    BHEffectPuffContext *ctx = context(entity);
    float alpha, size, angle = 0, sn, cs, w, h;
    BHVec3 p, up, n = {0, -1, 0};
    unsigned long image;
    int i;
    (void)ignored;
    if (!ctx || !(bh_field_f32(&entity->puff.delay) <= 0))
        return;
    alpha = rf((double)bh_field_f32(&entity->puff.lifetime) / bh_field_f32(&entity->puff.duration));
    size = rf((1.0 - alpha) * bh_field_f32(&entity->puff.size));
    memcpy(&image, &entity->puff.image, 4);
    w = rf((double)size * .10799224746714915);
    h = rf((double)size * (1.0 / 3.0));
    for (i = 0; i < 64; ++i)
    {
        sn = rf(sin(angle));
        cs = rf(cos(angle));
        angle = rf((double)angle + .09817477042468103);
        p.x = rf((double)entity->position.x + (double)size * cs);
        p.y = entity->position.y;
        p.z = rf((double)size * sn + entity->position.z);
        up.x = cs;
        up.y = 0;
        up.z = sn;
        draw(ctx, image, &p, &up, &n, w, h, 0, 12, alpha);
    }
}
