#include "../Entities/bh_field_access.h"
#include "bh_actor_construct.h"
#include "../../Math/bh_round.h"
#include "../../World/Routes/bh_route_query.h"
#include "../../Math/bh_fp.h"
#include <math.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static unsigned long next_random(unsigned long r)
{
    return ((r * 0xe357UL >> 16) | (r * 0xe3570000UL)) ^ r;
}
static BHActorConstructContext *context(BHEntity *entity)
{
    BHActorConstructContext *ctx;
    memcpy(&ctx, &entity->bindings.actor_construct, 4);
    return ctx;
}
static void route_tick(BHEntity *entity)
{
    BHActorConstructContext *ctx = context(entity);
    if (!bh_actor_route_due(ctx->response->damage->route, entity))
        ctx->error = 2;
}
static void draw_actor(BHEntity *entity)
{
    BHActorConstructContext *ctx = context(entity);
    if (!bh_actor_vehicle_draw(ctx->draw, entity))
        ctx->error = 2;
}
static BHEntity *construct(BHActorConstructContext *ctx, BHWorldGeometry *world, unsigned long type, long token,
                           float speed, unsigned long death_tag, unsigned long seed)
{
    BHActorResponseContext *response;
    BHActorRouteState state;
    BHVec3 center, zero = {0, 0, 0};
    const BHActorConstructRoom *binding = 0;
    BHEntity *entity, *head;
    BHEntityStep draw = draw_actor;
    unsigned long i, index, r;
    float original_speed = speed;
    long bounty;
    unsigned short primary, secondary;
    double v;
    if (!ctx || !world || !ctx->response || !ctx->response->damage || !ctx->response->damage->route ||
        !ctx->response->damage->route->live_tick || !ctx->draw || !ctx->models || !ctx->sounds ||
        ctx->resource_count < 25 || !ctx->rooms || !ctx->cycle_models || ctx->cycle_count < 4 ||
        !bh_math_environment_supported())
    {
        if (ctx)
            ctx->error = 1;
        return 0;
    }
    ctx->error = 0;
    response = ctx->response;
    for (i = 0; i < ctx->room_count; ++i)
        if (ctx->rooms[i].room && ctx->rooms[i].room->world == world)
        {
            binding = ctx->rooms + i;
            break;
        }
    if (!binding || !_finite(speed))
    {
        ctx->error = 1;
        return 0;
    }
    if (type == 4 || type == 5)
        type = 6;
    entity = bh_entity_acquire(response->damage->pool);
    if (!entity)
    {
        ctx->error = 2;
        return 0;
    }
    memcpy(&entity->bindings.actor_construct, &ctx, 4);
    entity->position = zero;
    entity->euler = zero;
    entity->flags |= 0x100;
    entity->world = world;
    memset(&state, 0, sizeof(state));
    state.current = state.next = world;
    center.x = rf(((double)world->minimum.x + world->maximum.x) * .5);
    center.y = rf(((double)world->minimum.y + world->maximum.y) * .5);
    center.z = rf(((double)world->minimum.z + world->maximum.z) * .5);
    for (i = 0; i < 4; ++i)
        state.controls[i] = center;
    state.portal_center = center;
    head = binding->room->actors;
    entity->actor.room_next = head;
    entity->actor.room_previous = 0;
    if (head)
        head->actor.room_previous = entity;
    binding->room->actors = entity;
    memset((char *)&entity->actor.offset, 0, 24);
    bh_field_store_f32(&entity->actor.damage, 0);
    state.duration = state.phase = 1;
    r = next_random(seed);
    state.u = rf(.25 - (r & 32767) * (-1.0 / 65536.0));
    r = next_random(r);
    state.v = rf(.25 - (r & 32767) * (-1.0 / 65536.0));
    state.tick = (unsigned short)(bh_actor_route_now(response->damage->route) - 30);
    state.duration_ticks = 30;
    if (speed == 0)
    {
        r = next_random(r);
        speed = rf(15.0 - (r & 32767) * (-1.0 / 1024.0));
    }
    state.speed = speed;
    state.jitter = .2f;
    state.speed_scale = 1;
    state.sound = BH_SOUND_INVALID;
    state.sound_frames = 0;
    bh_field_store_u32(&entity->actor.bounty_token, token ? (unsigned long)token : (unsigned long)-5000);
    bh_field_store_u32(&entity->actor.death_tag, death_tag);
    bh_field_store_f32(&entity->actor.fade, 1);
    bh_field_store_u32(&entity->actor.damage_source, 0);
    bh_field_store_f32(&entity->actor.pursuit, 0);
    bh_field_store_u32(&entity->actor.target, 0);
    bh_field_store_u32(&entity->actor.bound_resource, binding->bound_resource);
    entity->actor.kind = type;
    bh_field_store_u32(&entity->actor.destroy_when_faded, 0);
    entity->actor.draw = draw;
    r = next_random(r);
    index = ((r & 32767) * 23) >> 15;
    if (index > 7)
        ++index;
    if (index > 14)
        ++index;
    bh_field_store_u32(&entity->actor.model_resource, ctx->models[index]);
    bh_field_store_u32(&entity->actor.sound_resource, ctx->sounds[index]);
    if (type == 12)
    {
        type = 6;
        entity->actor.kind = 6;
        bh_field_store_u32(&entity->actor.destroy_when_faded, 1);
    }
    if (type == 7)
    {
        bh_field_store_u32(&entity->actor.model_resource, ctx->models[8]);
        bh_field_store_u32(&entity->actor.sound_resource, ctx->sounds[8]);
        type = 6;
        entity->actor.kind = 6;
    }
    if (type == 10)
    {
        type = 2;
        entity->actor.kind = 2;
        bh_field_store_f32(&entity->actor.pursuit, .45f);
        bh_field_store_u32(&entity->actor.target, (unsigned long)response->player);
    }
    if (type == 11)
    {
        type = 6;
        entity->actor.kind = 6;
        bh_field_store_u32(&entity->actor.target, (unsigned long)response->player);
    }
    if (type == 13)
    {
        type = 6;
        entity->actor.kind = 6;
        bh_field_store_u32(&entity->actor.model_resource, ctx->cycle_models[ctx->cycle & 3]);
        ++ctx->cycle;
        bh_field_store_u32(&entity->actor.sound_resource, ctx->sounds[14]);
    }
    if (type == 14 || type == 15)
    {
        bh_field_store_u32(&entity->actor.model_resource, type == 15 ? ctx->special_model : ctx->models[8]);
        bh_field_store_u32(&entity->actor.sound_resource, ctx->sounds[8]);
        if (type == 15)
            bh_field_store_f32(&entity->actor.damage, -10);
        type = 6;
        entity->actor.kind = 6;
        bh_field_store_u32(&entity->actor.target, (unsigned long)response->player);
    }
    if (type == 3)
    {
        bh_field_store_u32(&entity->actor.model_resource, ctx->models[8]);
        bh_field_store_u32(&entity->actor.sound_resource, ctx->sounds[8]);
        if (!token)
            bh_field_store_u32(&entity->actor.bounty_token, (unsigned long)-30000);
    }
    if (type == 1 || type == 2 || type == 6)
    {
        head = response->vehicles->head;
        entity->actor.global_next = head;
        entity->actor.global_previous = 0;
        if (head)
            head->actor.global_previous = entity;
        response->vehicles->head = entity;
        ++response->vehicles->count;
        if (type == 1)
        {
            if (original_speed == 0)
                state.speed = rf((double)state.speed - (-10.0));
            bh_field_store_f32(&entity->actor.pursuit, 1);
            if (!token)
                bh_field_store_u32(&entity->actor.bounty_token, (unsigned long)-15000);
        }
        else
        {
            state.jitter = .3f;
            if (!token)
            {
                v = (double)state.speed - 20.0;
                v = v * v * (type == 2 ? -.0078125 : -.015625);
                bounty = (long)(__int64)v;
                bh_field_store_u32(&entity->actor.bounty_token, (1UL - (unsigned long)bounty) * 5000UL);
            }
            if (type == 2 && original_speed == 0)
                state.speed = rf((double)state.speed - (-5.0));
        }
        r = next_random(r);
        if (type == 2)
        {
            bh_field_store_f32(&entity->actor.pursuit, rf((r & 32767) * (1.0 / 131072.0)));
            r = next_random(r);
        }
        else
            bh_field_store_f32(&entity->actor.pursuit, 1);
        primary = (unsigned short)(__int64)((r & 32767) * (1.0 / 11000.0));
        r = next_random(r);
        secondary = (unsigned short)(__int64)((r & 32767) * (1.0 / 11000.0));
        bh_field_store_u16(&entity->actor.primary_weapon, primary);
        bh_field_store_u16(&entity->actor.secondary_weapon, secondary);
        bh_field_store_u16(&entity->actor.attack_frame, 0);
        bh_field_store_u16(&entity->actor.attack_cooldown, type == 6 ? 20 : 10);
        bh_field_store_u32(&entity->actor.burst_frames, (type == 1   ? 30
                                                         : type == 2 ? 40
                                                                     : 60) /
                                                            (primary == 1 ? 2 : 1));
        bh_field_store_u32(&entity->actor.forced_pursuit, 0);
    }
    state.random = r;
    bh_actor_route_write(entity, &state);
    if (!bh_actor_damage_bind(response->damage, entity))
    {
        ctx->error = 2;
        return entity;
    }
    entity->step = route_tick;
    if (type == 1 || type == 2 || type == 6)
    {
        if (!bh_actor_vehicle_tick_bind(response->attack, entity) || !bh_actor_response_bind(response, entity))
            ctx->error = 2;
    }
    return entity;
}
BHEntity *bh_actor_construct_room(BHActorConstructContext *ctx, BHWorldGeometry *world, unsigned long t, long token,
                                  float speed, unsigned long tag, unsigned long seed)
{
    return construct(ctx, world, t, token, speed, tag, seed);
}
BHEntity *bh_actor_construct_position(BHActorConstructContext *ctx, const BHVec3 *p, unsigned long t, long token,
                                      float speed, unsigned long tag, unsigned long seed)
{
    BHWorldGeometry *world;
    if (!ctx || !p)
        return 0;
    world = bh_world_locate(ctx->first, ctx->world_count, 0, p, 0);
    if (!world)
        world = bh_world_locate(ctx->first, ctx->world_count, 0, p, 1);
    return construct(ctx, world, t, token, speed, tag, seed);
}
