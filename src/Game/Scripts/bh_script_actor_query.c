#include "bh_script_actor_query.h"
#include "../Actors/bh_actor_route_lifetime.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <string.h>
static_assert(sizeof(BHScriptActorQueryPacket) == 40, "BHScriptActorQueryPacket size");
static_assert(sizeof(void *) == 4 && sizeof(unsigned long) == 4, "callback abi");
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static int live(BHEntityPool *pool, BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
BHScriptActorContext *bh_script_actor_context(BHEntity *entity)
{
    BHScriptActorContext *ctx;
    memcpy(&ctx, &entity->bindings.script_actor, 4);
    return ctx;
}
int bh_script_actor_bind(BHScriptActorContext *ctx, BHEntity *entity)
{
    if (!ctx || !ctx->pool || !ctx->route || !ctx->route->live_tick || !entity || !live(ctx->pool, entity))
        return 0;
    memcpy(&entity->bindings.script_actor, &ctx, 4);
    return bh_entity_set_query(ctx->pool, entity, bh_script_actor_query);
}
void bh_script_actor_cleanup(BHEntity *entity)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    bh_actor_route_release(ctx->route, ctx->pool, entity);
}
void bh_script_actor_query(BHEntity *entity, void *packet)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    BHScriptActorQueryPacket *event_packet = (BHScriptActorQueryPacket *)packet;
    BHActorRouteState state;
    BHScriptActorPath path;
    BHVec3 offset, previous, current, next;
    BHEntity *target, *watched;
    unsigned short elapsed;
    short signed_elapsed_ticks, signed_duration_ticks;
    double precise_phase, backward_velocity, forward_velocity;
    float phase, stored_forward_velocity;
    unsigned long axis;
    if (!ctx || !ctx->pool || !ctx->route || !ctx->route->live_tick || !packet || !live(ctx->pool, entity))
        return;
    switch (event_packet->event)
    {
    case 20:
        entity->step = bh_script_actor_cleanup;
        entity->refresh = NULL;
        bh_entity_set_query(ctx->pool, entity, NULL);
        return;
    case 17:
        memcpy(&target, (char *)packet + 4, 4);
        memcpy(&watched, (char *)&entity->actor.target, 4);
        if (watched == target)
        {
            watched = NULL;
            memcpy((char *)&entity->actor.target, &watched, 4);
        }
        memcpy(&watched, (char *)&entity->actor.damage_source, 4);
        if (watched == target)
        {
            watched = NULL;
            memcpy((char *)&entity->actor.damage_source, &watched, 4);
        }
        return;
    case 4:
        bh_script_actor_cleanup(entity);
        return;
    case 3:
    case 16:
        break;
    default:
        return;
    }
    if (!bh_math_environment_supported())
    {
        ctx->route->error = BH_ACTOR_ROUTE_NUMERICAL;
        return;
    }
    path = bh_script_actor_route_path(entity);
    if (path != bh_script_actor_curve && path != bh_script_actor_linear)
    {
        ctx->route->error = BH_ACTOR_ROUTE_ARGUMENT;
        return;
    }
    bh_actor_route_read(entity, &state);
    elapsed = (unsigned short)(bh_actor_route_now(ctx->route) - state.tick);
    memcpy(&signed_elapsed_ticks, &elapsed, 2);
    memcpy(&signed_duration_ticks, &state.duration_ticks, 2);
    precise_phase = (double)signed_elapsed_ticks / signed_duration_ticks;
    phase = stored(precise_phase);
    memcpy(&offset, (char *)&entity->actor.offset, 12);
    if (event_packet->event == 3)
    {
        path(phase, &event_packet->position, state.controls, state.controls + 1, state.controls + 2,
             state.controls + 3);
        event_packet->position.x = stored((double)offset.x + event_packet->position.x);
        event_packet->position.y = stored((double)offset.y + event_packet->position.y);
        event_packet->position.z = stored((double)offset.z + event_packet->position.z);
        event_packet->position.y = stored((double)event_packet->position.y - (-2.0));
        return;
    }
    path(stored(precise_phase - 1.0 / 30.0), &previous, state.controls, state.controls + 1, state.controls + 2,
         state.controls + 3);
    path(phase, &current, state.controls, state.controls + 1, state.controls + 2, state.controls + 3);
    path(stored((double)phase - (-1.0 / 30.0)), &next, state.controls, state.controls + 1, state.controls + 2,
         state.controls + 3);
    for (axis = 0; axis < 3; ++axis)
    {
        backward_velocity = ((double)((float *)&current)[axis] - ((float *)&previous)[axis]) * 30;
        forward_velocity = ((double)((float *)&next)[axis] - ((float *)&current)[axis]) * 30;
        stored_forward_velocity = stored(forward_velocity);
        ((float *)&event_packet->velocity)[axis] = stored_forward_velocity;
        ((float *)&event_packet->acceleration)[axis] =
            stored((backward_velocity - (axis == 1 ? forward_velocity : stored_forward_velocity)) * 30);
        ((float *)&event_packet->position)[axis] = stored((double)((float *)&offset)[axis] + ((float *)&current)[axis]);
    }
}
