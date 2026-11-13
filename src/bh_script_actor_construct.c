#include "bh_field_access.h"
#include "bh_script_actor_construct.h"
#include "bh_script_actor_tick.h"
#include "bh_actor_route_lifetime.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && sizeof(unsigned long) == 4, "constructor abi");
static void real_clock(BHEntity *entity)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    bh_script_actor_clock_tick(ctx->route, entity);
}
static void real_phase(BHEntity *entity)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    bh_script_actor_phase_tick(ctx->route, entity, ctx->pulse_resource);
}
static void real_draw(BHEntity *entity)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    bh_script_actor_draw(ctx->draw, entity);
}
static void real_vehicle_draw(BHEntity *entity)
{
    BHScriptActorContext *ctx = bh_script_actor_context(entity);
    bh_script_actor_vehicle_draw(ctx->draw, entity);
}
void bh_script_actor_present(BHEntity *entity)
{
    BHEntityStep draw;
    memcpy(&draw, (char *)&entity->actor.draw, 4);
    if (draw)
        draw(entity);
}
BHEntity *bh_script_actor_construct(BHScriptActorConstructorContext *ctx, const BHVec3 *position, unsigned long type)
{
    BHScriptActorContext *callbacks;
    BHActorRouteContext *route;
    BHWorldGeometry *world;
    BHPickupRoom *room = NULL;
    BHEntity *entity, *head;
    BHActorRouteState state;
    BHVec3 zero = {0, 0, 0}, center, offset;
    BHEntityStep draw;
    unsigned long i, first, second;
    if (!ctx || !position || type > 3)
        return NULL;
    callbacks = &ctx->callbacks;
    route = callbacks->route;
    if (!callbacks->pool || !route || !callbacks->draw || !callbacks->draw->queue || !callbacks->draw->particles ||
        !route->live_tick || !route->rooms || !route->room_count || !bh_math_environment_supported())
        return NULL;
    world = bh_world_locate(ctx->first_world, ctx->world_count, NULL, position, 0);
    if (!world)
        return NULL;
    room = bh_actor_route_room(route, world);
    if (!room)
        return NULL;
    entity = bh_entity_acquire(callbacks->pool);
    if (!entity)
        return NULL;
    memset(&state, 0, sizeof(state));
    entity->position = zero;
    entity->origin = zero;
    entity->flags |= 0x100;
    state.current = state.next = world;
    memcpy(&entity->world, &world, 4);
    head = room->actors;
    entity->actor.room_next = head;
    room->actors = entity;
    entity->actor.room_previous = 0;
    if (head)
        head->actor.room_previous = entity;
    first = (unsigned long)rand();
    second = (unsigned long)rand();
    state.random = first + second * 65536UL;
    bh_round_float(&center.x, ((double)world->minimum.x + world->maximum.x) * .5);
    bh_round_float(&center.y, ((double)world->minimum.y + world->maximum.y) * .5);
    bh_round_float(&center.z, ((double)world->minimum.z + world->maximum.z) * .5);
    for (i = 0; i < 4; ++i)
        state.controls[i] = center;
    state.portal_center = center;
    state.speed = 80;
    state.u = .5f;
    state.v = .5f;
    state.speed_scale = 1;
    state.duration = 1;
    state.duration_ticks = 30;
    state.tick = (unsigned short)(bh_actor_route_now(route) - 30);
    state.sound = BH_SOUND_INVALID;
    bh_actor_route_write(entity, &state);
    memcpy(&entity->actor.offset, &zero, 12);
    memcpy(&entity->actor.offset_velocity, &zero, 12);
    bh_field_store_u32(&entity->actor.fade, 0x3f800000);
    bh_field_store_u32(&entity->actor.jitter, 0x3e4ccccd);
    bh_field_store_u32(&entity->actor.bounty_token, 0);
    bh_field_store_u32(&entity->actor.destroy_when_faded, 0);
    bh_field_store_u32(&entity->actor.death_tag, 0);
    bh_field_store_u32(&entity->actor.damage_source, 0);
    bh_field_store_u32(&entity->actor.model_resource, ctx->model_resources[type]);
    bh_field_store_u32(&entity->actor.sound_resource, ctx->sound_resource);
    entity->actor.kind = type < 2 ? 4 : 5;
    if (type < 2)
    {
        offset.x = 5;
        offset.y = 5;
        offset.z = 10;
        draw = real_draw;
    }
    else
    {
        offset = zero;
        draw = real_vehicle_draw;
    }
    memcpy(&entity->actor.draw_offset, &offset, 12);
    memcpy(&entity->actor.draw, &draw, 4);
    entity->step = type == 2 ? real_phase : real_clock;
    if (!bh_script_actor_bind(callbacks, entity) || !bh_script_actor_route_advance(route, entity))
    {
        bh_actor_route_release(route, callbacks->pool, entity);
        return NULL;
    }
    return entity;
}
