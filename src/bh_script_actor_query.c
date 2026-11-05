#include "bh_script_actor_query.h"
#include "bh_actor_route_lifetime.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
typedef char packet_layout[sizeof(BHScriptActorQueryPacket) == 40 ? 1 : -1];
typedef char callback_abi[sizeof(void *) == 4 && sizeof(unsigned long) == 4 ? 1 : -1];
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
BHScriptActorContext *__cdecl bh_script_actor_context(BHEntity *entity)
{
    BHScriptActorContext *ctx;
    memcpy(&ctx, entity->payload + 560, 4);
    return ctx;
}
int __cdecl bh_script_actor_bind(BHScriptActorContext *ctx, BHEntity *entity)
{
    if (!ctx || !ctx->pool || !ctx->route || !ctx->route->live_tick || !entity || !live(ctx->pool, entity))
        return 0;
    memcpy(entity->payload + 560, &ctx, 4);
    return bh_entity_set_query(ctx->pool, entity, bh_script_actor_query);
}
void __cdecl bh_script_actor_cleanup(BHEntity *entity)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    bh_actor_route_release(ctx->route, ctx->pool, entity);
}
void __cdecl bh_script_actor_query(BHEntity *entity, void *packet)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    BHScriptActorQueryPacket *p = (BHScriptActorQueryPacket *)packet;
    BHActorRouteState state;
    BHScriptActorPath path;
    BHVec3 offset, previous, current, next;
    BHEntity *target, *watched;
    unsigned short elapsed;
    short se, sd;
    double t, v0, v1;
    float phase, forward;
    unsigned long i;
    if (!ctx || !ctx->pool || !ctx->route || !ctx->route->live_tick || !packet || !live(ctx->pool, entity))
        return;
    switch (p->event)
    {
    case 20:
        entity->step = bh_script_actor_cleanup;
        entity->refresh = NULL;
        bh_entity_set_query(ctx->pool, entity, NULL);
        return;
    case 17:
        memcpy(&target, (char *)packet + 4, 4);
        memcpy(&watched, (char *)entity + 0x1b8, 4);
        if (watched == target)
        {
            watched = NULL;
            memcpy((char *)entity + 0x1b8, &watched, 4);
        }
        memcpy(&watched, (char *)entity + 0x1ac, 4);
        if (watched == target)
        {
            watched = NULL;
            memcpy((char *)entity + 0x1ac, &watched, 4);
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
    memcpy(&se, &elapsed, 2);
    memcpy(&sd, &state.duration_ticks, 2);
    t = (double)se / sd;
    phase = stored(t);
    memcpy(&offset, (char *)entity + 0x15c, 12);
    if (p->event == 3)
    {
        path(phase, &p->position, state.controls, state.controls + 1, state.controls + 2, state.controls + 3);
        p->position.x = stored((double)offset.x + p->position.x);
        p->position.y = stored((double)offset.y + p->position.y);
        p->position.z = stored((double)offset.z + p->position.z);
        p->position.y = stored((double)p->position.y - (-2.0));
        return;
    }
    path(stored(t - 1.0 / 30.0), &previous, state.controls, state.controls + 1, state.controls + 2, state.controls + 3);
    path(phase, &current, state.controls, state.controls + 1, state.controls + 2, state.controls + 3);
    path(stored((double)phase - (-1.0 / 30.0)), &next, state.controls, state.controls + 1, state.controls + 2,
         state.controls + 3);
    for (i = 0; i < 3; ++i)
    {
        v0 = ((double)((float *)&current)[i] - ((float *)&previous)[i]) * 30;
        v1 = ((double)((float *)&next)[i] - ((float *)&current)[i]) * 30;
        forward = stored(v1);
        ((float *)&p->velocity)[i] = forward;
        ((float *)&p->acceleration)[i] = stored((v0 - (i == 1 ? v1 : forward)) * 30);
        ((float *)&p->position)[i] = stored((double)((float *)&offset)[i] + ((float *)&current)[i]);
    }
}
