#include "bh_field_access.h"
#include "bh_actor_pursuit_route.h"
#include "bh_target_selection.h"
#include "bh_target_steering.h"
#include "bh_route_choice.h"
#include "bh_route_guided.h"
#include "bh_route_curve_length.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
static float stored(double d)
{
    float f;
    bh_round_float(&f, d);
    return f;
}
static int fail(BHActorPursuitRouteContext *ctx, BHActorRouteError err)
{
    if (ctx && ctx->route)
        ctx->route->error = err;
    return 0;
}
static long elapsed32(long now, long before)
{
    unsigned long v = (unsigned long)now - (unsigned long)before;
    long result;
    memcpy(&result, &v, 4);
    return result;
}
typedef struct TargetMotion
{
    long event;
    BHVec3 position, velocity, acceleration;
} TargetMotion;
static int query(BHEntity *target, TargetMotion *p)
{
    if (!target || !target->query)
        return 0;
    p->event = 16;
    target->query(target, p);
    return 1;
}
static int reset_duration(BHActorPursuitRouteContext *ctx, BHEntity *entity, BHActorRouteState *state)
{
    double ticks, duration;
    float length = bh_route_curve_length(state->controls);
    state->phase = 0;
    duration = ((double)length + length) / ((double)state->speed * state->speed_scale);
    state->duration = stored(duration);
    /* 0043914e/00439795 FST stores duration but retains it for tick conversion. */
    ticks = duration * 30;
    if (!_finite(ticks) || ticks < -9223372036854775808.0 || ticks >= 9223372036854775808.0)
        return fail(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    state->duration_ticks = (unsigned short)(__int64)ticks;
    state->tick = bh_actor_route_now(ctx->route);
    return bh_actor_route_write(entity, state);
}
/* 0043950c..004397c3: target-control subpath, not an independent owner. */
int bh_actor_pursuit_target_controls(BHActorPursuitRouteContext *ctx, BHEntity *entity)
{
    BHActorRouteState state;
    TargetMotion motion;
    BHVec3 saved_end, saved_tangent;
    BHEntity *target = (BHEntity *)bh_field_i32(&entity->actor.target);
    unsigned long i;
    long forced;
    bh_actor_route_read(entity, &state);
    for (i = 0; i < 3; ++i)
        ((float *)&state.controls[1])[i] =
            stored((double)((float *)&state.controls[1])[i] * .4 - ((float *)&state.controls[0])[i] * (-.6));
    for (i = 0; i < 3; ++i)
        ((float *)&state.controls[2])[i] =
            stored((double)((float *)&state.controls[2])[i] * .4 - ((float *)&state.controls[3])[i] * (-.6));
    /* 00439564..00439618 commit all six tangent words before target query.
       Its synchronous callback must observe them in the real entity. */
    memcpy(&entity->actor.route_first, state.controls + 1, 12);
    memcpy(&entity->actor.route_second, state.controls + 2, 12);
    if (!query(target, &motion))
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    bh_actor_route_read(entity, &state);
    saved_end = state.controls[3];
    saved_tangent = state.controls[2];
    memcpy(&entity->actor.saved_end, &saved_end, 12);
    memcpy(&entity->actor.saved_tangent, &saved_tangent, 12);
    for (i = 0; i < 3; ++i)
    {
        double predicted =
            (double)((float *)&motion.position)[i] -
            (((double)((float *)&motion.velocity)[i] - ((float *)&motion.acceleration)[i] * (-1.5)) * (-1.5));
        ((float *)&state.controls[3])[i] = stored(predicted);
        /* X reloads the 00439662 FSTP spill; Y/Z retain their 004396be /
           004396e0 FST values until 00439748/0043974c. */
        ((float *)&state.controls[2])[i] =
            stored((i ? predicted : (double)state.controls[3].x) -
                   (((double)((float *)&state.controls[0])[i] - ((float *)&saved_end)[i]) * (-.25)));
    }
    memcpy(&entity->actor.route_end, state.controls + 3, 12);
    memcpy(&entity->actor.route_second, state.controls + 2, 12);
    bh_world_clamp_point(state.current, (BHVec3 *)&entity->actor.route_end);
    bh_world_clamp_point(state.current, (BHVec3 *)&entity->actor.route_second);
    bh_actor_route_read(entity, &state);
    if (!reset_duration(ctx, entity, &state))
        return 0;
    forced = 1;
    memcpy(&entity->actor.forced_pursuit, &forced, 4);
    return 1;
}
int bh_actor_pursuit_route(BHActorPursuitRouteContext *ctx, BHEntity *entity)
{
    BHActorRouteState state;
    BHWorldGeometry *room, *target_world = 0, *preferred = 0;
    BHEntity *target;
    BHTargetSelection selected;
    long incoming = -1, outgoing, index, target_index, junction, old_city, new_city, forced;
    unsigned long i, first;
    float pursuit;
    double factor;
    char text[256];
    static const char *const report[4] = {
        "1Attention,\ncrime fighters!\n\nA fugitive car\nhas been spotted\nentering\n%s",
        "1Attention!\n\nWe have reports\non a fugitive car\nentering\n%s",
        "1Calling all\nHunters!\n\nA fugitive car\njust entered\n%s",
        "1Incoming\nSurveillance\nReport:\n\nA fugitive car\njust passed through\na gate at\n%s"};
    if (!ctx || !ctx->route || !ctx->route->live_tick || !ctx->navigation || !entity ||
        !bh_math_environment_supported())
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    bh_actor_route_read(entity, &state);
    room = state.next;
    target = (BHEntity *)bh_field_i32(&entity->actor.target);
    if (state.sound_frames)
    {
        --state.sound_frames;
        memcpy(&entity->actor.sound_frames, &state.sound_frames, 4);
        if (!state.sound_frames && state.sound != BH_SOUND_INVALID)
        {
            if (!ctx->route->sound)
                return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
            bh_sound_stop(ctx->route->sound, state.sound);
            state.sound = BH_SOUND_INVALID;
            memcpy(&entity->actor.sound, &state.sound, 4);
        }
    }
    if (bh_field_i32(&entity->actor.kind) == 1 && (!target || rand() < 0x1000))
    {
        if (!bh_target_select(ctx->player, ctx->actor_head_owner ? *ctx->actor_head_owner : ctx->actor_head, entity,
                              state.controls + 3, ctx->actor_limit, &selected))
            return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
        target = selected.target;
        memcpy(&entity->actor.target, &target, 4);
    }
    forced = bh_field_i32(&entity->actor.forced_pursuit);
    if (forced)
    {
        state.controls[0] = state.controls[3];
        for (i = 0; i < 3; ++i)
            ((float *)&state.controls[1])[i] =
                stored(((double)((float *)&state.controls[3])[i] - ((float *)&state.controls[2])[i]) +
                       ((float *)&state.controls[0])[i]);
        bh_world_clamp_point(state.current, state.controls);
        bh_world_clamp_point(state.current, state.controls + 1);
        memcpy(state.controls + 3, &entity->actor.saved_end, 12);
        memcpy(state.controls + 2, &entity->actor.saved_tangent, 12);
        if (!reset_duration(ctx, entity, &state))
            return 0;
        forced = 0;
        memcpy(&entity->actor.forced_pursuit, &forced, 4);
        ctx->route->error = BH_ACTOR_ROUTE_OK;
        return 1;
    }
    if (!room || !room->model || room->navigation.count != room->model->portal_count || room->navigation.count > 6)
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    index = bh_navigation_find(ctx->navigation, room);
    if (index < 0)
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    if (bh_field_i32(&entity->actor.kind) == 6 && ctx->city_ids)
    {
        new_city = ctx->city_ids[index];
        old_city = bh_field_i32(&entity->actor.bound_resource);
        if (new_city != -1 && new_city != old_city)
        {
            memcpy(&entity->actor.bound_resource, &new_city, 4);
            if (old_city != -1 && elapsed32(ctx->clock, ctx->last_report) > 1800)
            {
                if (new_city < 0 || (unsigned long)new_city >= ctx->city_count || !ctx->city_names || !ctx->messages ||
                    !ctx->city_names[new_city] || strlen(ctx->city_names[new_city]) > 160)
                    return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
                first = (unsigned long)rand() & 3;
                ctx->last_report = ctx->clock;
                sprintf(text, report[first], ctx->city_names[new_city]);
                bh_message_enqueue(ctx->messages, text);
            }
        }
    }
    for (i = 0; i < room->navigation.count; ++i)
        if ((BHWorldGeometry *)room->navigation.portals[i].neighbor == state.current || incoming < 0)
            incoming = (long)i;
    pursuit = bh_field_f32(&entity->actor.pursuit);
    if (room->query_flags & 8)
    {
        if (!bh_route_guided_exit(entity, room, incoming, &outgoing))
            return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    else if (!target || (unsigned long)index >= ctx->navigation->junction_count)
    {
        if (!bh_route_choose_exit(entity, room, 0, incoming, &outgoing))
            return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    else
    {
        memcpy(&target_world, &target->world, 4);
        if (rand() > 0xccc)
        {
            if (target_world)
                for (i = 0; i < room->navigation.count && !preferred; ++i)
                    if ((pursuit >= .5 || (long)i != incoming) &&
                        (BHWorldGeometry *)room->navigation.portals[i].neighbor == target_world)
                        preferred = target_world;
            if (!preferred)
            {
                target_index = target_world ? bh_navigation_find(ctx->navigation, target_world) : index;
                if (target_index < 0)
                    return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
                if ((unsigned long)target_index >= ctx->navigation->junction_count)
                {
                    const BHNavigationNode *node = ctx->navigation->nodes + target_index;
                    junction = node->junction_first_port;
                    if (junction == index)
                        junction = node->junction_second_port;
                    else if (node->junction_second_port != index)
                    {
                        if (rand() < 0x4000 && node->junction_second_port >= 0)
                            junction = node->junction_second_port;
                        else
                            junction = node->junction_first_port;
                    }
                    if (junction < 0)
                        junction = index;
                    target_index = junction;
                }
                if ((unsigned long)target_index >= ctx->navigation->junction_count)
                    return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
                if (!ctx->navigation->routes.cells ||
                    ctx->navigation->routes.node_count != ctx->navigation->junction_count)
                    return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
                outgoing =
                    ctx->navigation->routes.cells[target_index * ctx->navigation->junction_count + index].exit_slot;
                if (outgoing >= 0 && (pursuit >= .5 || outgoing != incoming) &&
                    (unsigned long)outgoing < room->navigation.count)
                    preferred = (BHWorldGeometry *)room->navigation.portals[outgoing].neighbor;
            }
        }
        if (!bh_route_choose_exit(entity, room, preferred, incoming, &outgoing))
            return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    /* CRT random bias here is distinct from the actor-local generator used by route selection. */
    factor = 1.0 - state.jitter;
    state.u = stored(factor * state.u - ((double)state.jitter * rand()) * (-1.0 / 32768.0));
    state.v = stored(factor * state.v - ((double)rand() * state.jitter) * (-1.0 / 32768.0));
    memcpy(&entity->actor.u, &state.u, 4);
    memcpy(&entity->actor.v, &state.v, 4);
    if (!bh_actor_route_promote(ctx->route, entity, incoming, outgoing))
        return 0;
    bh_actor_route_read(entity, &state);
    target = (BHEntity *)bh_field_i32(&entity->actor.target);
    if (target)
    {
        short protected_player = 0;
        memcpy(&target_world, &target->world, 4);
        if (target == ctx->player)
            protected_player = target->player.disabled_frames;
        if (target_world == state.current && (!protected_player || target != ctx->player))
        {
            return bh_actor_pursuit_target_controls(ctx, entity);
        }
    }
    ctx->route->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
