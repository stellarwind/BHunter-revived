#include "bh_field_access.h"
/* Complete004357d0 source core. Source clock/registry/legacy-angle bindings
   remain explicit adapters. No original image code enters this target. */
#define DAMAGE_TICK_CURVE(t, p)                                                                                        \
    bh_camera_curve((t), (p), &e->actor.route_start, &e->actor.route_first, &e->actor.route_second, &e->actor.route_end)
static float damage_tick_phase(BHActorDamageContext *c, BHEntity *e)
{
    unsigned short elapsed = (unsigned short)(bh_actor_route_now(c->route) - *(unsigned short *)&e->actor.tick);
    return damage_query_stored((double)(short)elapsed / *(short *)&e->actor.duration_ticks);
}
void bh_actor_damage_tick_core(BHEntity *e, BHActorDamageContext *c)
{
    BHVec3 position, death_position, velocity, acceleration, before, at, after, spin;
    BHPickupRoom *room;
    BHEntity *previous, *next, *owner;
    double dx, dy, dz, distance, damping;
    float x, y, z, length, t, angular, angle, strength;
    unsigned long random, material;
    int draw;
    long loot = 0, token;
    (*(volatile float *)&e->actor.damage = damage_query_stored((double)e->actor.damage * .9975));
    dx = (double)e->actor.offset.x - e->actor.offset_velocity.x * (-1.0 / 30.0);
    dy = (double)e->actor.offset.y - e->actor.offset_velocity.y * (-1.0 / 30.0);
    dz = (double)e->actor.offset.z - e->actor.offset_velocity.z * (-1.0 / 30.0);
    x = damage_query_stored(dx);
    y = damage_query_stored(dy);
    z = damage_query_stored(dz);
    distance = sqrt((dx * x + dy * y) + dz * z);
    (*(volatile float *)&e->actor.offset.x = damage_query_stored(x));
    length = damage_query_stored(distance);
    (*(volatile float *)&e->actor.offset.y = damage_query_stored(y));
    (*(volatile float *)&e->actor.offset.z = damage_query_stored(z));
    damping = (.99 - distance * distance * .001) - (double)length * .01;
    if (!(damping >= 0))
        damping = 0;
    (*(volatile float *)&e->actor.offset.x = damage_query_stored(damping * dx));
    (*(volatile float *)&e->actor.offset.y = damage_query_stored(damping * y));
    (*(volatile float *)&e->actor.offset.z = damage_query_stored(damping * z));
    (*(volatile float *)&e->actor.offset_velocity.x = damage_query_stored((double)e->actor.offset_velocity.x * .7));
    (*(volatile float *)&e->actor.offset_velocity.y = damage_query_stored((double)e->actor.offset_velocity.y * .7));
    (*(volatile float *)&e->actor.offset_velocity.z = damage_query_stored((double)e->actor.offset_velocity.z * .7));
    bh_actor_route_due(c->route, e);
    if (e->actor.damage < 1)
    {
        draw = rand();
        if ((double)e->actor.damage * 5000 <= draw)
            goto resume;
    }
    acceleration.x = 0;
    acceleration.y = -.002f;
    acceleration.z = 0;
    velocity.x = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.y = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.z = damage_query_stored((double)(rand() - 16384) * (1.0 / 262144.0));
    t = damage_tick_phase(c, e);
    DAMAGE_TICK_CURVE(t, &position);
    position.x = damage_query_stored((double)e->actor.offset.x + position.x);
    position.y = damage_query_stored((double)e->actor.offset.y + position.y);
    position.z = damage_query_stored((double)e->actor.offset.z + position.z);
    death_position = position;
    angular = damage_query_stored((double)(rand() - 16384) * (1.0 / 16384.0));
    angle = damage_query_stored((double)rand() * (1.0 / 512.0));
    bh_script_particle_create(c->particles, c->smoke_resource, 3, &position, &velocity, &acceleration, .5f, angle, 12,
                              angular, .9f, .95f, 2, .3f);
    if (e->actor.damage >= 1 && c->mode != 3)
    {
        if (!c->legacy_death_angles)
        {
            c->error = BH_ACTOR_ROUTE_ARGUMENT;
            return;
        }
        bh_entity_set_query(c->pool, e, 0);
        bh_sound_stop(c->route->sound, bh_field_u32(&e->actor.sound));
        t = damage_tick_phase(c, e);
        DAMAGE_TICK_CURVE(damage_query_stored((double)t - 1.0 / 30.0), &before);
        DAMAGE_TICK_CURVE(t, &at);
        DAMAGE_TICK_CURVE(damage_query_stored((double)t - (-1.0 / 30.0)), &after);
        bh_actor_damage_death_velocity(&after, &at, &velocity);
        bh_script_debris_create(c->fragments, (rand() & 7) + 1, &death_position, 8, 10, 1);
        strength = damage_query_stored(.75 - (double)rand() * (-1.0 / 65536.0));
        bh_sound_request(c->route->sound, c->death_sound, 2, strength, &death_position, &velocity);
        random = random_step(bh_field_u32(&e->actor.random));
        spin.x = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        spin.y = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        spin.z = damage_query_stored(((double)(random & 32767) - 16384) * (1.0 / 4096.0));
        random = random_step(random);
        bh_field_store_u32(&e->actor.random, random);
        bh_remote_choose_slot(c->loot_rows, c->loot_row_count, 0, -1, -1, &loot);
        random = bh_field_u32(&e->actor.random);
        token = (long)bh_field_u32(&e->actor.bounty_token);
        owner = (BHEntity *)bh_field_u32(&e->actor.damage_source);
        material = bh_field_u32(&e->actor.model_resource);
        strength = damage_query_stored(1.0 - (double)(random & 32767) * (-1.0 / 16384.0));
        bh_target_construct_moving(c->targets, &death_position, c->legacy_death_angles, &velocity, &spin, material,
                                   10000, strength, owner, token, loot, 0);
        room = damage_query_room(c, e);
        if (!room)
        {
            c->error = BH_ACTOR_ROUTE_TOPOLOGY;
            return;
        }
        previous = (BHEntity *)bh_field_u32(&e->actor.room_previous);
        if (previous)
            bh_field_store_u32(&previous->actor.room_next, bh_field_u32(&e->actor.room_next));
        else
            room->actors = (BHEntity *)bh_field_u32(&e->actor.room_next);
        next = (BHEntity *)bh_field_u32(&e->actor.room_next);
        if (next)
            bh_field_store_u32(&next->actor.room_previous, bh_field_u32(&e->actor.room_previous));
        bh_entity_release(c->pool, e);
        c->error = BH_ACTOR_ROUTE_OK;
        return;
    }
resume:
    if (e->actor.damage < .03 && bh_field_u32(&e->actor.speed_scale) == 0x3f800000UL &&
        bh_field_u32(&e->actor.fade) == 0x3f800000UL)
        e->step = normal_tick;
    c->error = BH_ACTOR_ROUTE_OK;
}
#undef DAMAGE_TICK_CURVE
