#include "bh_field_access.h"
#include "bh_actor_response.h"
#include "bh_actor_vehicle_step.h"
#include "bh_camera_curve.h"
#include "bh_target_selection.h"
#include "bh_remote_prediction.h"
#include "bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

/* Source-context adapters for the original one-argument dependency ABI.
   They live outside the matched owner and retain its checked caller's
   early-return behavior on a source dependency/lifetime guard failure. */
static BHActorAttackContext *vehicle_tick_context(BHEntity *entity)
{
    BHActorAttackContext *ctx;
    memcpy(&ctx, &entity->bindings.actor_attack, 4);
    return ctx;
}
void bh_actor_attack_bound(BHEntity *entity)
{
    BHActorAttackContext *ctx = vehicle_tick_context(entity);
    if (!bh_actor_attack(ctx, entity) && ctx && ctx->step_abort)
        longjmp(*(jmp_buf *)ctx->step_abort, 1);
}
void bh_actor_pursuit_bound(BHEntity *entity)
{
    BHActorAttackContext *ctx = vehicle_tick_context(entity);
    if (!bh_actor_pursuit_route(ctx->pursuit, entity) && ctx->step_abort)
        longjmp(*(jmp_buf *)ctx->step_abort, 1);
}
/* These are source field views, not distinct original owners. Emit the direct
   loads/stores used throughout 0043a1d0/00439910. */
static __inline BHActorResponseContext *context(BHEntity *entity)
{
    BHActorResponseContext *ctx;
    memcpy(&ctx, &entity->bindings.actor_response, 4);
    return ctx;
}
static int live(BHActorResponseContext *ctx, BHEntity *entity)
{
    return bh_entity_is_active(ctx->vehicles->pool, entity);
}
static int unlink_actor(BHActorResponseContext *ctx, BHEntity *entity)
{
    BHActorRouteState state;
    BHPickupRoom *room = 0;
    BHEntity *p, *n;
    unsigned long i;
    bh_actor_route_read(entity, &state);
    room = bh_actor_route_room(ctx->damage->route, state.current);
    if (!room || !ctx->vehicles->count)
    {
        ctx->error = 2;
        return 0;
    }
    p = (BHEntity *)bh_field_u32(&entity->actor.room_previous);
    n = (BHEntity *)bh_field_u32(&entity->actor.room_next);
    if ((p ? (BHEntity *)bh_field_u32(&p->actor.room_next) : room->actors) != entity ||
        (n && (BHEntity *)bh_field_u32(&n->actor.room_previous) != entity))
    {
        ctx->error = 2;
        return 0;
    }
    if (p)
        bh_field_store_u32(&p->actor.room_next, (unsigned long)n);
    else
        room->actors = n;
    if (n)
        bh_field_store_u32(&n->actor.room_previous, (unsigned long)p);
    p = (BHEntity *)bh_field_u32(&entity->actor.global_previous);
    n = (BHEntity *)bh_field_u32(&entity->actor.global_next);
    if ((p ? (BHEntity *)bh_field_u32(&p->actor.global_next) : ctx->vehicles->head) != entity ||
        (n && (BHEntity *)bh_field_u32(&n->actor.global_previous) != entity))
    {
        ctx->error = 2;
        return 0;
    }
    if (p)
        bh_field_store_u32(&p->actor.global_next, (unsigned long)n);
    else
        ctx->vehicles->head = n;
    if (n)
        bh_field_store_u32(&n->actor.global_previous, (unsigned long)p);
    --ctx->vehicles->count;
    return 1;
}
static void release_tick(BHEntity *entity)
{
    BHActorResponseContext *ctx = context(entity);
    if (!bh_actor_vehicle_release(ctx->vehicles, entity))
        ctx->error = 2;
}
int bh_actor_response_bind(BHActorResponseContext *ctx, BHEntity *entity)
{
    if (!ctx || !ctx->damage || !ctx->damage->route || !ctx->attack || !ctx->vehicles || !ctx->vehicles->pool ||
        !entity || ctx->damage->pool != ctx->vehicles->pool || ctx->vehicles->route != ctx->damage->route ||
        !live(ctx, entity))
        return 0;
    memcpy(&entity->bindings.actor_response, &ctx, 4);
    return bh_entity_set_query(ctx->vehicles->pool, entity, bh_actor_response_query);
}
#include "bh_actor_response_query_core.inc"
#include "bh_actor_response_motion.inc"
#include "bh_actor_response_smoke.inc"
#include "bh_actor_response_finish.inc"
void bh_actor_response_query(BHEntity *entity, void *packet)
{
    BHActorResponseContext *ctx = context(entity);
    unsigned long event;
    if (!ctx || !packet || !live(ctx, entity))
        return;
    ctx->error = 0;
    event = *(unsigned long *)packet;
    if (event == 0 || event == 1 || event == 3 || event == 15 || event == 16)
    {
        if (!bh_math_environment_supported() || bh_field_f32(&entity->actor.duration) <= 0 ||
            !_finite(bh_field_f32(&entity->actor.duration)))
        {
            ctx->error = 3;
            return;
        }
    }
    bh_actor_response_query_core(entity, packet, ctx);
}
void bh_actor_response_tick(BHEntity *entity)
{
    BHActorResponseContext *ctx = context(entity);
    BHActorDamageContext *d;
    BHVec3 death_position;
    int smoke;
    if (!ctx || !live(ctx, entity))
        return;
    ctx->error = 0;
    d = ctx->damage;
    if (!d->particles || !d->fragments || !d->targets || !d->loot_rows || !ctx->mission ||
        !bh_math_environment_supported())
    {
        ctx->error = 1;
        return;
    }
    if (!bh_actor_attack(ctx->attack, entity))
    {
        ctx->error = 2;
        return;
    }
    if (bh_field_f32(&entity->actor.duration) <= 0 || !bh_field_f32(&entity->actor.speed))
    {
        ctx->error = 3;
        return;
    }
    if (!bh_actor_response_motion(entity, ctx))
        return;
    smoke = bh_actor_response_route_smoke(entity, ctx, &death_position);
    if (smoke < 0)
        return;
    bh_actor_response_finish(entity, ctx, &death_position, smoke);
}
