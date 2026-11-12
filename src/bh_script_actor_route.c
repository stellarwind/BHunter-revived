#include "bh_script_actor_route.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <float.h>
static float rounded(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static BHPickupRoom *binding(BHActorRouteContext *ctx, BHWorldGeometry *world)
{
    return bh_actor_route_room(ctx, world);
}
BHScriptActorPath bh_script_actor_route_path(const BHEntity *entity)
{
    BHScriptActorPath p;
    memcpy(&p, (const char *)entity + 0x1a0, 4);
    return p;
}
static int error(BHActorRouteContext *ctx, BHActorRouteError e)
{
    if (ctx)
        ctx->error = e;
    return 0;
}
int bh_script_actor_route_advance(BHActorRouteContext *ctx, BHEntity *entity)
{
    BHActorRouteState state;
    BHPickupRoom *old, *room;
    BHWorldGeometry *world;
    BHEntity *next, *prev, *at;
    BHVec3 start, end, n1, n2, side, delta, up = {0, 1, 0};
    unsigned long i, n, count = 0;
    long incoming = -1, outgoing = -1;
    double best = 9e37, test, vertical, lateral, dot1, dot2, duration, length;
    BHScriptActorPath path;
    if (!ctx || !entity || !ctx->live_tick || !ctx->rooms || !ctx->room_count || !ctx->actor_limit)
        return error(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_math_environment_supported())
        return error(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    bh_actor_route_read(entity, &state);
    old = binding(ctx, state.current);
    room = binding(ctx, state.next);
    world = state.next;
    if (!old || !room || !world->model || world->navigation.count != world->model->portal_count ||
        world->navigation.count > 6)
        return error(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    next = entity->actor.room_next;
    prev = entity->actor.room_previous;
    if ((prev ? prev->actor.room_next != entity : old->actors != entity) || (next && next->actor.room_previous != entity))
        return error(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    at = old->actors;
    for (n = 0; at && at != entity && n < ctx->actor_limit; ++n)
        at = at->actor.room_next;
    if (at != entity || n >= ctx->actor_limit)
        return error(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    for (i = 0; i < world->navigation.count; ++i)
        if (world->navigation.portals[i].neighbor)
        {
            ++count;
            if (incoming < 0)
                incoming = (long)i;
            if (outgoing < 0)
                outgoing = (long)i;
            if ((BHWorldGeometry *)world->navigation.portals[i].neighbor == state.current)
                incoming = (long)i;
            else
                outgoing = (long)i;
        }
    if (incoming < 0 || outgoing < 0 || !_finite(state.speed) || !_finite(state.speed_scale) || state.speed <= 0 ||
        state.speed_scale <= 0)
        return error(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    if (!bh_portal_center(world->navigation.portals + incoming, &start) ||
        !bh_portal_normal(world->navigation.portals + incoming, &n1))
        return error(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    bh_vec3_cross(&up, &n1, &side);
    if (count > 2)
    {
        outgoing = incoming;
        for (i = 0; i < world->navigation.count; ++i)
        {
            const BHPortal *p = world->navigation.portals + i;
            if ((long)i == incoming || !p->neighbor ||
                p->direction != (world->navigation.portals[incoming].direction ^ 4UL))
                continue;
            if (!bh_portal_center(p, &end))
                return error(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
            delta.x = rounded((double)end.x - start.x);
            delta.y = rounded((double)end.y - start.y);
            delta.z = rounded((double)end.z - start.z);
            vertical = delta.y;
            lateral = (double)delta.z * side.z + (double)delta.y * side.y + (double)delta.x * side.x;
            test = rounded(lateral * lateral + vertical * vertical);
            if (test < best)
            {
                best = test;
                outgoing = (long)i;
            }
        }
    }
    if (!bh_portal_center(world->navigation.portals + outgoing, &end) ||
        !bh_portal_normal(world->navigation.portals + outgoing, &n2) ||
        !binding(ctx, (BHWorldGeometry *)world->navigation.portals[outgoing].neighbor))
        return error(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    delta.x = rounded((double)end.x - start.x);
    delta.y = rounded((double)end.y - start.y);
    delta.z = rounded((double)end.z - start.z);
    dot1 = rounded((double)n1.z * delta.z + (double)n1.y * delta.y + (double)n1.x * delta.x);
    dot2 = rounded(-((double)n2.z * delta.z + (double)n2.y * delta.y + (double)n2.x * delta.x));
    if (dot1 == 0)
        dot1 = 10;
    if (dot2 == 0)
        dot2 = 10;
    length = (dot1 + dot2) * .75;
    if (world->navigation.portals[outgoing].direction == (world->navigation.portals[incoming].direction ^ 4UL))
        path = bh_script_actor_linear;
    else
    {
        length = length * 1.5707963267948966;
        dot1 = dot1 * .7;
        dot2 = rounded(dot2 * .7);
        path = bh_script_actor_curve;
    }
    state.controls[0] = start;
    state.controls[3] = end;
    state.portal_center = end;
    state.controls[1].x = rounded((double)n1.x * dot1 + start.x);
    state.controls[1].y = rounded((double)n1.y * dot1 + start.y);
    state.controls[1].z = rounded((double)n1.z * dot1 + start.z);
    state.controls[2].x = rounded((double)n2.x * dot2 + end.x);
    state.controls[2].y = rounded((double)n2.y * dot2 + end.y);
    state.controls[2].z = rounded((double)n2.z * dot2 + end.z);
    state.phase = 0;
    state.duration = rounded((double)length / ((double)state.speed * state.speed_scale));
    duration = (double)state.duration * 30;
    if (!_finite(duration) || duration < 0 || duration >= 9223372036854775808.0)
        return error(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    state.duration_ticks = (unsigned short)(__int64)duration;
    state.tick = bh_actor_route_now(ctx);
    if (state.sound_frames == 1 && state.sound != BH_SOUND_INVALID && !ctx->sound)
        return error(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    if (state.sound_frames)
    {
        --state.sound_frames;
        if (!state.sound_frames && state.sound != BH_SOUND_INVALID)
        {
            bh_sound_stop(ctx->sound, state.sound);
            state.sound = BH_SOUND_INVALID;
        }
    }
    if (prev)
        prev->actor.room_next = next;
    else
        old->actors = next;
    if (next)
        next->actor.room_previous = prev;
    next = room->actors;
    entity->actor.room_next = next;
    entity->actor.room_previous = 0;
    if (next)
        next->actor.room_previous = entity;
    room->actors = entity;
    state.current = world;
    state.next = (BHWorldGeometry *)world->navigation.portals[outgoing].neighbor;
    bh_actor_route_write(entity, &state);
    entity->world = world;
    memcpy((char *)entity + 0x1a0, &path, 4);
    ctx->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
