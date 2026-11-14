#include "bh_actor_damage.h"
#include "../../Camera/bh_camera_curve.h"
#include "bh_remote_prediction.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static BHActorDamageContext *context(BHEntity *entity)
{
    BHActorDamageContext *ctx;
    memcpy(&ctx, &entity->bindings.actor_damage, 4);
    return ctx;
}
static int live(BHActorDamageContext *ctx, BHEntity *entity)
{
    return bh_entity_is_active(ctx->pool, entity);
}
static void normal_tick(BHEntity *entity)
{
    BHActorDamageContext *ctx = context(entity);
    bh_actor_route_due(ctx->route, entity);
}
static void release_tick(BHEntity *entity)
{
    BHActorDamageContext *ctx = context(entity);
    bh_actor_route_release(ctx->route, ctx->pool, entity);
}
int bh_actor_damage_bind(BHActorDamageContext *ctx, BHEntity *entity)
{
    if (!ctx || !ctx->pool || !ctx->route || !ctx->route->live_tick || !ctx->route->sound || !entity ||
        !live(ctx, entity))
        return 0;
    memcpy(&entity->bindings.actor_damage, &ctx, 4);
    return bh_entity_set_query(ctx->pool, entity, bh_actor_damage_query);
}
static unsigned long random_step(unsigned long r)
{
    return ((r * 0xe357UL >> 16) | (r * 0xe3570000UL)) ^ r;
}
void bh_actor_damage_death_velocity(const BHVec3 *after, const BHVec3 *at, BHVec3 *velocity)
{
    /* 00435bf9..00435c52 retains X, but spills Y/Z differences before
       reloading them for the common scale. Do not symmetrize these stores. */
    double x = (double)after->x - at->x;
    float y = stored((double)after->y - at->y);
    float z = stored((double)after->z - at->z);
    velocity->x = stored(x * 15);
    velocity->y = stored((double)y * 15);
    velocity->z = stored((double)z * 15);
}
#include "bh_actor_damage_query_core.inc"
#include "bh_actor_damage_tick_core.inc"
void bh_actor_damage_tick(BHEntity *entity)
{
    BHActorDamageContext *ctx = context(entity);
    if (!ctx || !live(ctx, entity) || !ctx->particles || !ctx->fragments || !ctx->targets || !ctx->loot_rows ||
        !bh_math_environment_supported())
    {
        if (ctx)
            ctx->error = BH_ACTOR_ROUTE_ARGUMENT;
        return;
    }
    if (!*(unsigned short *)&entity->actor.duration_ticks)
    {
        ctx->error = BH_ACTOR_ROUTE_NUMERICAL;
        return;
    }
    bh_actor_damage_tick_core(entity, ctx);
}
void bh_actor_damage_query(BHEntity *entity, void *packet)
{
    BHActorDamageContext *ctx = context(entity);
    unsigned long event;
    unsigned short tick = 0;
    if (!ctx || !packet || !live(ctx, entity))
        return;
    event = *(unsigned long *)packet;
    if (event == 1 || event == 3 || event == 16)
    {
        if (!bh_math_environment_supported() || !*(unsigned short *)&entity->actor.duration_ticks)
        {
            ctx->error = BH_ACTOR_ROUTE_NUMERICAL;
            return;
        }
        tick = bh_actor_route_now(ctx->route);
    }
    bh_actor_damage_query_core(entity, packet, ctx, tick);
}
