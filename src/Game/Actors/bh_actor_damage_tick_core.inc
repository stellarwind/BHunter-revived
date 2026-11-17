#include "../Entities/bh_field_access.h"
/* Complete004357d0 source core. Source clock/registry/legacy-angle bindings
   remain explicit adapters. No original image code enters this target. */
#define DAMAGE_TICK_CURVE(t, p)                                                                                        \
    bh_camera_curve((t), (p), &actor->actor.route_start, &actor->actor.route_first, &actor->actor.route_second, &actor->actor.route_end)
static float damage_tick_phase(BHActorDamageContext *context, BHEntity *actor)
{
    unsigned short elapsed = (unsigned short)(bh_actor_route_now(context->route) - *(unsigned short *)&actor->actor.tick);
    return damage_query_stored((double)(short)elapsed / *(short *)&actor->actor.duration_ticks);
}
void bh_actor_damage_tick_core(BHEntity *actor, BHActorDamageContext *context)
{
    BHVec3 position, death_position, velocity, acceleration, before, at, after, spin;
    BHPickupRoom *room;
    BHEntity *previous, *next, *owner;
    double dx, dy, dz, distance, damping;
    float x, y, z, length, t, angular, angle, strength;
    unsigned long random, material;
    int draw;
    long loot = 0, token;
    (*(volatile float *)&actor->actor.damage = damage_query_stored((double)actor->actor.damage * .9975));
    dx = (double)actor->actor.offset.x - actor->actor.offset_velocity.x * (-1.0 / 30.0);
    dy = (double)actor->actor.offset.y - actor->actor.offset_velocity.y * (-1.0 / 30.0);
    dz = (double)actor->actor.offset.z - actor->actor.offset_velocity.z * (-1.0 / 30.0);
    x = damage_query_stored(dx);
    y = damage_query_stored(dy);
    z = damage_query_stored(dz);
    distance = sqrt((dx * x + dy * y) + dz * z);
    (*(volatile float *)&actor->actor.offset.x = damage_query_stored(x));
    length = damage_query_stored(distance);
    (*(volatile float *)&actor->actor.offset.y = damage_query_stored(y));
    (*(volatile float *)&actor->actor.offset.z = damage_query_stored(z));
    damping = (.99 - distance * distance * .001) - (double)length * .01;
    if (!(damping >= 0))
        damping = 0;
    (*(volatile float *)&actor->actor.offset.x = damage_query_stored(damping * dx));
    (*(volatile float *)&actor->actor.offset.y = damage_query_stored(damping * y));
    (*(volatile float *)&actor->actor.offset.z = damage_query_stored(damping * z));
    (*(volatile float *)&actor->actor.offset_velocity.x = damage_query_stored((double)actor->actor.offset_velocity.x * .7));
    (*(volatile float *)&actor->actor.offset_velocity.y = damage_query_stored((double)actor->actor.offset_velocity.y * .7));
    (*(volatile float *)&actor->actor.offset_velocity.z = damage_query_stored((double)actor->actor.offset_velocity.z * .7));
    bh_actor_route_due(context->route, actor);
    if (actor->actor.damage < 1)
    {
        draw = rand();
        if ((double)actor->actor.damage * 5000 <= draw)
            goto resume;
    }
    acceleration.x = 0;
    acceleration.y = -.002f;
    acceleration.z = 0;
    velocity.x = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.y = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.z = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    t = damage_tick_phase(context, actor);
    DAMAGE_TICK_CURVE(t, &position);
    position.x = damage_query_stored((double)actor->actor.offset.x + position.x);
    position.y = damage_query_stored((double)actor->actor.offset.y + position.y);
    position.z = damage_query_stored((double)actor->actor.offset.z + position.z);
    death_position = position;
    angular = damage_query_stored((double)(rand() - 16384) * (1.0 / 16384.0));
    angle = damage_query_stored((double)rand() * (1.0 / 512.0));
    bh_script_particle_create(context->particles, context->smoke_resource, 3, &position, &velocity, &acceleration, .5f, angle, 12,
                              angular, .9f, .95f, 2, .3f);
    if (actor->actor.damage >= 1)
    {
        if (!context->legacy_death_angles)
        {
            context->error = BH_ACTOR_ROUTE_ARGUMENT;
            return;
        }
        bh_entity_set_query(context->pool, actor, 0);
        bh_sound_stop(context->route->sound, bh_field_u32(&actor->actor.sound));
        t = damage_tick_phase(context, actor);
        DAMAGE_TICK_CURVE(damage_query_stored((double)t - 1.0 / 30.0), &before);
        DAMAGE_TICK_CURVE(t, &at);
        DAMAGE_TICK_CURVE(damage_query_stored((double)t - (-1.0 / 30.0)), &after);
        bh_actor_damage_death_velocity(&after, &at, &velocity);
        bh_script_debris_create(context->fragments, (rand() & 7) + 1, &death_position, 8, 10, 1);
        strength = damage_query_stored(.75 - (double)rand() * (-1.0 / 65536.0));
        bh_sound_request(context->route->sound, context->death_sound, 2, strength, &death_position, &velocity);
        random = random_step(bh_field_u32(&actor->actor.random));
        spin.x = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        spin.y = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        spin.z = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        bh_field_store_u32(&actor->actor.random, random);
        bh_remote_choose_slot(context->loot_rows, context->loot_row_count, 0, -1, -1, &loot);
        random = bh_field_u32(&actor->actor.random);
        token = (long)bh_field_u32(&actor->actor.bounty_token);
        owner = (BHEntity *)bh_field_u32(&actor->actor.damage_source);
        material = bh_field_u32(&actor->actor.model_resource);
        strength = damage_query_stored(1.0 - (double)(random & 32767) * (-1.0 / 16384.0));
        bh_target_construct_moving(context->targets, &death_position, context->legacy_death_angles, &velocity, &spin, material,
                                   10000, strength, owner, token, loot, 0);
        room = damage_query_room(context, actor);
        if (!room)
        {
            context->error = BH_ACTOR_ROUTE_TOPOLOGY;
            return;
        }
        previous = (BHEntity *)bh_field_u32(&actor->actor.room_previous);
        if (previous)
            bh_field_store_u32(&previous->actor.room_next, bh_field_u32(&actor->actor.room_next));
        else
            room->actors = (BHEntity *)bh_field_u32(&actor->actor.room_next);
        next = (BHEntity *)bh_field_u32(&actor->actor.room_next);
        if (next)
            bh_field_store_u32(&next->actor.room_previous, bh_field_u32(&actor->actor.room_previous));
        bh_entity_release(context->pool, actor);
        context->error = BH_ACTOR_ROUTE_OK;
        return;
    }
resume:
    if (actor->actor.damage < .03 && bh_field_u32(&actor->actor.speed_scale) == 0x3f800000UL &&
        bh_field_u32(&actor->actor.fade) == 0x3f800000UL)
        actor->step = normal_tick;
    context->error = BH_ACTOR_ROUTE_OK;
}
#undef DAMAGE_TICK_CURVE
