#include "bh_projectile_create.h"
#include "bh_round.h"
#include <string.h>
#include <stdlib.h>
static float scalar(const BHEntity *entity, unsigned long o)
{
    float f;
    memcpy(&f, (const unsigned char *)entity + o, 4);
    return f;
}
static void put(BHEntity *entity, unsigned long o, const void *v, unsigned long n)
{
    memcpy((unsigned char *)entity + o, v, n);
}
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static void endpoint(BHVec3 *p, const BHEntity *entity, double scale)
{
    unsigned int i;
    for (i = 0; i < 3; i++)
        ((float *)p)[i] =
            stored((double)((const float *)&entity->position)[i] - (double)((const float *)&entity->origin)[i] * scale);
}
static void result(BHProjectileDrawContext *ctx, int n)
{
    ++ctx->attempted;
    if (n > 0)
        ++ctx->queued;
    else if (n < 0)
        ctx->error = -1;
    else if (!ctx->error)
        ctx->error = 1;
}
int bh_projectile_ballistic_draw(BHProjectileDrawContext *ctx, const BHEntity *entity)
{
    BHVec3 a, b;
    if (!ctx || !ctx->queue || !entity)
        return -1;
    ctx->error = 0;
    result(ctx, bh_flare_queue(ctx->queue, &entity->position, 2, 50, ctx->ballistic_flare, 0, 0, 0, 0));
    endpoint(&a, entity, -1.0 / 30.0);
    endpoint(&b, entity, 1.0 / 30.0);
    result(ctx, bh_draw_segment(ctx->queue, 3, ctx->ballistic_trail, &a, &b, .3f, .3f, 1, 1, 0));
    return ctx->error < 0 ? -1 : !ctx->error;
}
int bh_projectile_homing_draw(BHProjectileDrawContext *ctx, const BHEntity *entity)
{
    BHVec3 a, b;
    float angle;
    int r;
    if (!ctx || !ctx->queue || !entity)
        return -1;
    ctx->error = 0;
    result(ctx, bh_flare_queue(ctx->queue, &entity->position, 2, 50, ctx->homing_flare, 0, 0, 0, 0));
    endpoint(&a, entity, -1.0 / 30.0);
    endpoint(&b, entity, 1.0 / 30.0);
    result(ctx, bh_draw_segment(ctx->queue, 3, ctx->homing_trail, &a, &b, .5f, .5f, 1, 1, 0));
    r = rand();
    ++ctx->random_calls;
    angle = stored((double)r * (1.0 / 4096.0));
    result(ctx, bh_draw_billboard(ctx->queue, 3, ctx->homing_trail, &a, 1.5f, 1.5f, angle, 0, 1));
    return ctx->error < 0 ? -1 : !ctx->error;
}
int bh_projectile_pulse_draw(BHProjectileDrawContext *ctx, const BHEntity *entity)
{
    BHVec3 up = {0, -1, 0};
    float alpha, size, angle;
    int r;
    if (!ctx || !ctx->queue || !entity)
        return -1;
    ctx->error = 0;
    alpha = stored((double)scalar(entity, 0xe8) * .5);
    size = scalar(entity, 0xec);
    r = rand();
    ++ctx->random_calls;
    angle = stored((double)r * (1.0 / 4096.0));
    result(ctx, bh_draw_oriented(ctx->queue, 3, ctx->pulse_image, &entity->position, &up, &entity->origin, size, size,
                                 angle, 8, alpha));
    return ctx->error < 0 ? -1 : !ctx->error;
}
static BHProjectileDrawContext *draw_context(BHEntity *entity)
{
    BHProjectileDrawContext *ctx;
    memcpy(&ctx, entity->payload + 616, 4);
    return ctx;
}
static void ballistic_draw(BHEntity *entity, void *unused)
{
    (void)unused;
    bh_projectile_ballistic_draw(draw_context(entity), entity);
}
static void homing_draw(BHEntity *entity, void *unused)
{
    (void)unused;
    bh_projectile_homing_draw(draw_context(entity), entity);
}
static int valid(BHProjectileCreateContext *ctx, int homing)
{
    BHHomingProjectileContext *b;
    if (!ctx || !ctx->lifetime || !ctx->lifetime->pool || !ctx->lifetime->sound || !ctx->draw || !ctx->draw->queue ||
        !ctx->next_identity)
        return 0;
    b = homing ? ctx->homing : (ctx->ballistic ? &ctx->ballistic->bindings : NULL);
    return b && b->pool == ctx->lifetime->pool && b->sound == ctx->lifetime->sound && b->particles && b->contacts &&
           b->flash && b->markers && b->segment;
}
static BHEntity *create(BHProjectileCreateContext *ctx, BHWorldGeometry *world, BHEntity *source, const BHVec3 *p,
                        const BHVec3 *v, float lifetime, BHEntity *target, int homing)
{
    BHEntity *entity;
    BHProjectileQueryState state;
    unsigned long zero = 0;
    BHEntityPositionPacket packet;
    if (!valid(ctx, homing) || !p || !v || (target && !target->query))
    {
        if (ctx)
            ctx->error = 1;
        return NULL;
    }
    ctx->error = 0;
    entity = bh_entity_acquire(ctx->lifetime->pool);
    if (!entity)
        return NULL;
    bh_projectile_query_state(entity, &state);
    state.countdown = lifetime;
    state.identity = *ctx->next_identity;
    *ctx->next_identity = 0;
    state.secondary = source;
    if (homing)
        state.target = target;
    entity->position = *p;
    entity->origin = *v;
    if (!(homing ? bh_homing_projectile_bind(ctx->homing, entity)
                 : bh_ballistic_projectile_bind(ctx->ballistic, entity)))
    {
        ctx->error = 1;
        bh_entity_release(ctx->lifetime->pool, entity);
        return NULL;
    }
    memcpy(entity->payload + 616, &ctx->draw, 4);
    entity->refresh = homing ? homing_draw : ballistic_draw;
    if (homing)
        state.sound = BH_SOUND_INVALID;
    /* Install actual query before sound requests as in original constructor. */
    if (!bh_projectile_lifetime_bind(ctx->lifetime, entity, &state))
    {
        ctx->error = 1;
        bh_entity_release(ctx->lifetime->pool, entity);
        return NULL;
    }
    if (!homing)
    {
        state.sound = bh_sound_request(ctx->lifetime->sound, ctx->loop_sound, 1, 1.5f, p, v);
        put(entity, 0x118, &state.sound, 4);
        bh_sound_request(ctx->lifetime->sound, ctx->launch_sound, .75f, 4, p, v);
    }
    put(entity, 0x128, &source, 4);
    put(entity, 0x124, &world, 4);
    if (homing && target)
    {
        packet.event = 3;
        target->query(target, &packet);
        put(entity, 0x100, &packet.position, 12);
    }
    put(entity, 0x120, &zero, 4);
    return entity;
}
BHEntity *bh_projectile_create_ballistic(BHProjectileCreateContext *ctx, BHWorldGeometry *world, BHEntity *entity,
                                         const BHVec3 *p, const BHVec3 *v, float t)
{
    return create(ctx, world, entity, p, v, t, NULL, 0);
}
BHEntity *bh_projectile_create_homing(BHProjectileCreateContext *ctx, BHWorldGeometry *world, BHEntity *entity,
                                      const BHVec3 *p, const BHVec3 *v, float t, BHEntity *target)
{
    return create(ctx, world, entity, p, v, t, target, 1);
}
