#include "bh_field_access.h"
#include "bh_remote_tick.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
int bh_remote_tick(BHRemoteTick *tick, BHEntity *entity)
{
    unsigned long i, flags, remaining;
    short countdown;
    BHEntity *target;
    BHVec3 position, start, end, p, velocity, acceleration = {0, -.002f, 0};
    BHWorldGeometry *cached;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    int result, n, r, j;
    float fraction, radius, width, fall, life, angle, stored_angle, spin;
    double cs, sn, raw_angle;
    if (!tick || !entity || !tick->fire || !tick->fire->tick || !bh_math_environment_supported())
    {
        if (tick)
            tick->error = 1;
        return 0;
    }
    tick->error = 0;
    ++tick->calls;
    for (i = 0; i < 9; ++i)
    {

        if (bh_field_i16(&entity->remote.firing[i].remaining))
        {
            countdown = (short)(unsigned short)((unsigned short)bh_field_i16(&entity->remote.firing[i].countdown) - 1);
            bh_field_store_u16(&entity->remote.firing[i].countdown, (unsigned short)countdown);
            if (countdown < 1)
            {
                bh_field_store_u16(&entity->remote.firing[i].countdown,
                                   (unsigned short)((unsigned short)bh_field_i16(&entity->remote.firing[i].spacing) +
                                                    (unsigned short)countdown));
                bh_field_store_u16(
                    &entity->remote.firing[i].remaining,
                    (unsigned short)((unsigned short)bh_field_i16(&entity->remote.firing[i].remaining) - 1));
                target = entity->remote.firing[i].target;
                ++tick->scheduled;
                if (!bh_remote_fire(tick->fire, entity, (short)(unsigned short)*tick->fire->tick, i, target))
                {
                    tick->error = 2;
                    return 0;
                }
            }
        }
    }
    remaining = bh_field_u32(&entity->remote.sound_frames);
    if (remaining)
    {
        flags = (unsigned char)entity->remote.pose_flags;
        if (flags & 6)
        {
            if (!tick->particles || !tick->particles->pool || !tick->particles->queue ||
                !bh_remote_predict_position(entity, (unsigned short)*tick->fire->tick, &position))
            {
                tick->error = 1;
                return 0;
            }
        }
        if (flags & 2)
        {
            if (!tick->world)
            {
                tick->error = 1;
                return 0;
            }
            cached = (BHWorldGeometry *)bh_field_u32(&entity->world);
            start = end = position;
            start.y = rf((double)position.y - (-.3));
            end.y = rf((double)position.y - (-8.3));
            self.kind = BH_WORLD_HIT_ACTOR;
            self.object.actor = entity;
            result = bh_world_segment_run(tick->world, &self, &cached, &start, &end, -1, &hit);
            if (result < 0)
            {
                tick->error = 3;
                return 0;
            }
            if (result)
            {
                r = rand();
                fraction = hit.fraction;
                if (!_finite(fraction) || fraction < 0 || fraction > 1)
                {
                    tick->error = 4;
                    return 0;
                }
                n = (int)(1 - ((1 - (double)fraction) * r) * (-1.0 / 8192));
                radius = rf((1 - (double)fraction) * .25);
                width = rf(2.5 - ((double)fraction + fraction));
                fall = rf((1 - (double)fraction) * (-.02));
                life = rf((1 - (double)fraction) * .5);
                for (j = 0; j < n; ++j)
                {
                    r = rand();
                    raw_angle = (double)r * .00019175345108273588;
                    stored_angle = rf(raw_angle);
                    cs = cos(raw_angle);
                    sn = sin((double)stored_angle);
                    velocity.x = rf(cs * radius);
                    velocity.y = 0;
                    velocity.z = rf(sn * radius);
                    acceleration.x = rf(cs * radius * (-1.0 / 15));
                    acceleration.y = fall;
                    acceleration.z = rf(sn * radius * (-1.0 / 15));
                    p.x = rf((double)rf(cs) * width + hit.point.x);
                    p.y = hit.point.y;
                    p.z = rf((double)rf(sn) * width + hit.point.z);
                    r = rand();
                    angle = rf((double)r * .00019175345108273588);
                    bh_script_particle_create(tick->particles, tick->smoke_image, 3, &p, &velocity, &acceleration, .5f,
                                              angle, 20, 0, .8f, .9f, life, radius);
                    ++tick->ring_particles;
                }
            }
        }
        flags = (unsigned char)entity->remote.pose_flags;
        if (flags & 4)
        {
            r = rand();
            if (r < 16384)
            {
                for (i = 0; i < 3; ++i)
                {
                    r = rand();
                    ((float *)&velocity)[i] = rf(((double)r - 16384) * (1.0 / 262144));
                }
                r = rand();
                spin = rf(((double)r - 16384) * (1.0 / 16384));
                r = rand();
                angle = rf((double)r * (1.0 / 512));
                acceleration.x = acceleration.z = 0;
                acceleration.y = -.002f;
                bh_script_particle_create(tick->particles, tick->smoke_image, 3, &position, &velocity, &acceleration,
                                          .5f, angle, 12, spin, .9f, .95f, 2, .3f);
                ++tick->smoke_particles;
            }
        }
        remaining = bh_field_u32(&entity->remote.sound_frames) - 1;
        bh_field_store_u32(&entity->remote.sound_frames, remaining);
        if (!remaining && bh_field_u32(&entity->remote.engine_sound) != BH_SOUND_INVALID)
        {
            if (!tick->sound)
            {
                tick->error = 1;
                return 0;
            }
            bh_sound_stop(tick->sound, bh_field_u32(&entity->remote.engine_sound));
            bh_field_store_u32(&entity->remote.engine_sound, BH_SOUND_INVALID);
        }
    }
    for (i = 0; i < 6; ++i)
    {
        BHVec3 *value = i < 3 ? &entity->acceleration : &entity->angular_acceleration;
        bh_vec3_set_component(value, i % 3, rf((double)bh_vec3_component(value, i % 3) * .8));
    }
    for (i = 0; i < 6; ++i)
    {
        BHVec3 *value = i < 3 ? &entity->velocity : &entity->angular_velocity;
        bh_vec3_set_component(value, i % 3, rf((double)bh_vec3_component(value, i % 3) * .965));
    }
    return 1;
}
void bh_remote_step(BHEntity *entity)
{
    BHRemoteTick *tick;
    memcpy(&tick, &entity->bindings.remote_tick, 4);
    if (tick)
        bh_remote_tick(tick, entity);
}
int bh_remote_tick_bind(BHRemoteTick *tick, BHEntity *entity)
{
    BHEntity *at;
    BHEntityPool *pool;
    unsigned long n = 0;
    if (!tick || !entity || !tick->fire || !tick->fire->pulse || !(pool = tick->fire->pulse->effects.pool))
        return 0;
    for (at = pool->active; at && n++ < pool->capacity; at = at->next)
        if (at == entity)
        {
            memcpy(&entity->bindings.remote_tick, &tick, 4);
            entity->step = bh_remote_step;
            return 1;
        }
    return 0;
}
