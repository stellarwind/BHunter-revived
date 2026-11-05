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
static unsigned long word(BHEntity *entity, unsigned long o)
{
    unsigned long n;
    memcpy(&n, (char *)entity + o, 4);
    return n;
}
static void setword(BHEntity *entity, unsigned long o, unsigned long n)
{
    memcpy((char *)entity + o, &n, 4);
}
static short half(BHEntity *entity, unsigned long o)
{
    short n;
    memcpy(&n, (char *)entity + o, 2);
    return n;
}
static void sethalf(BHEntity *entity, unsigned long o, unsigned short n)
{
    memcpy((char *)entity + o, &n, 2);
}
static float number(BHEntity *entity, unsigned long o)
{
    float f;
    memcpy(&f, (char *)entity + o, 4);
    return f;
}
static void real(BHEntity *entity, unsigned long o, float f)
{
    memcpy((char *)entity + o, &f, 4);
}
int __cdecl bh_remote_tick(BHRemoteTick *tick, BHEntity *entity)
{
    unsigned long i, offset, flags, remaining;
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
        offset = 0x19c + 12 * i;
        if (half(entity, offset))
        {
            countdown = (short)(unsigned short)((unsigned short)half(entity, offset + 4) - 1);
            sethalf(entity, offset + 4, (unsigned short)countdown);
            if (countdown < 1)
            {
                sethalf(entity, offset + 4,
                        (unsigned short)((unsigned short)half(entity, offset + 2) + (unsigned short)countdown));
                sethalf(entity, offset, (unsigned short)((unsigned short)half(entity, offset) - 1));
                memcpy(&target, (char *)entity + offset + 8, 4);
                ++tick->scheduled;
                if (!bh_remote_fire(tick->fire, entity, (short)(unsigned short)*tick->fire->tick, i, target))
                {
                    tick->error = 2;
                    return 0;
                }
            }
        }
    }
    remaining = word(entity, 0x108);
    if (remaining)
    {
        flags = *((unsigned char *)entity + 0x11a);
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
            cached = (BHWorldGeometry *)word(entity, 0xc4);
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
        flags = *((unsigned char *)entity + 0x11a);
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
        remaining = word(entity, 0x108) - 1;
        setword(entity, 0x108, remaining);
        if (!remaining && word(entity, 0x104) != BH_SOUND_INVALID)
        {
            if (!tick->sound)
            {
                tick->error = 1;
                return 0;
            }
            bh_sound_stop(tick->sound, word(entity, 0x104));
            setword(entity, 0x104, BH_SOUND_INVALID);
        }
    }
    for (i = 0; i < 6; ++i)
        real(entity, 0x60 + 4 * i, rf((double)number(entity, 0x60 + 4 * i) * .8));
    for (i = 0; i < 6; ++i)
        real(entity, 0x48 + 4 * i, rf((double)number(entity, 0x48 + 4 * i) * .965));
    return 1;
}
void __cdecl bh_remote_step(BHEntity *entity)
{
    BHRemoteTick *tick;
    memcpy(&tick, entity->payload + 540, 4);
    if (tick)
        bh_remote_tick(tick, entity);
}
int __cdecl bh_remote_tick_bind(BHRemoteTick *tick, BHEntity *entity)
{
    BHEntity *at;
    BHEntityPool *pool;
    unsigned long n = 0;
    if (!tick || !entity || !tick->fire || !tick->fire->pulse || !(pool = tick->fire->pulse->effects.pool))
        return 0;
    for (at = pool->active; at && n++ < pool->capacity; at = at->next)
        if (at == entity)
        {
            memcpy(entity->payload + 540, &tick, 4);
            entity->step = bh_remote_step;
            return 1;
        }
    return 0;
}
