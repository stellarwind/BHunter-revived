#include "../Entities/bh_field_access.h"
/* Arithmetic prefix of00439910, not a separate original function. The checked
   caller supplies a live actor, positive duration and nonzero speed. */
static int response_tick_phase_valid(BHActorResponseContext *c, double phase)
{
    double start = (short)bh_actor_route_now(c->damage->route) - phase * 30;
    if (!_finite(start) || fabs(start) >= 9223372036854775808.0)
    {
        c->error = 3;
        return 0;
    }
    return 1;
}
int bh_actor_response_motion(BHEntity *e, BHActorResponseContext *c)
{
    double ratio, scale, numerator, duration, phase, fade, damage;
    double dx, dy, dz, distance, damping;
    float stored_scale, x, y, z, length;
    if (bh_field_u32(&e->actor.speed_scale) != 0x3f800000UL)
    {
        ratio = (double)e->actor.phase / e->actor.duration;
        scale = (double)e->actor.speed_scale * .97;
        numerator = ((double)e->actor.speed * e->actor.speed_scale) * e->actor.duration;
        stored_scale = response_query_stored(scale);
        if (scale < 1)
            stored_scale = 1;
        duration = numerator / ((double)e->actor.speed * stored_scale);
        phase = ratio * duration;
        if (!response_query_timing_valid(c, duration, phase))
            return 0;
        (*(volatile float *)&e->actor.speed_scale = response_query_stored(scale));
        if (scale < 1)
            (*(volatile float *)&e->actor.speed_scale = response_query_stored(1));
        (*(volatile float *)&e->actor.duration = response_query_stored(duration));
        (*(volatile float *)&e->actor.phase = response_query_stored(phase));
        e->actor.duration_ticks = (unsigned short)(__int64)(duration * 30);
        e->actor.tick = (unsigned short)(__int64)((short)bh_actor_route_now(c->damage->route) - phase * 30);
    }
    fade = (double)e->actor.fade * 1.008;
    (*(volatile float *)&e->actor.fade = response_query_stored(fade));
    if (fade > 1)
        (*(volatile float *)&e->actor.fade = response_query_stored(1));
    if (e->actor.damage > 0)
    {
        damage = (double)e->actor.damage * .998;
        (*(volatile float *)&e->actor.damage = response_query_stored(damage));
        if (bh_field_u32(&e->actor.kind) == 6)
            (*(volatile float *)&e->actor.damage = response_query_stored(damage * .998));
    }
    dx = (double)e->actor.offset.x - e->actor.offset_velocity.x * (-1.0 / 30.0);
    dy = (double)e->actor.offset.y - e->actor.offset_velocity.y * (-1.0 / 30.0);
    dz = (double)e->actor.offset.z - e->actor.offset_velocity.z * (-1.0 / 30.0);
    x = response_query_stored(dx);
    y = response_query_stored(dy);
    z = response_query_stored(dz);
    distance = sqrt((dx * x + dy * y) + dz * z);
    (*(volatile float *)&e->actor.offset.x = response_query_stored(x));
    length = response_query_stored(distance);
    (*(volatile float *)&e->actor.offset.y = response_query_stored(y));
    (*(volatile float *)&e->actor.offset.z = response_query_stored(z));
    damping = (.99 - distance * distance * .001) - (double)length * .01;
    if (!(damping >= 0))
        damping = 0;
    (*(volatile float *)&e->actor.offset.x = response_query_stored(damping * dx));
    (*(volatile float *)&e->actor.offset.y = response_query_stored(damping * y));
    (*(volatile float *)&e->actor.offset.z = response_query_stored(damping * z));
    (*(volatile float *)&e->actor.offset_velocity.x = response_query_stored((double)e->actor.offset_velocity.x * .7));
    (*(volatile float *)&e->actor.offset_velocity.y = response_query_stored((double)e->actor.offset_velocity.y * .7));
    (*(volatile float *)&e->actor.offset_velocity.z = response_query_stored((double)e->actor.offset_velocity.z * .7));
    if (bh_field_u32(&e->actor.fade) == 0x3f800000UL)
        (*(volatile float *)&e->actor.phase = response_query_stored((double)e->actor.phase - (-1.0 / 30.0)));
    else
    {
        phase = (double)e->actor.phase - e->actor.fade * (-1.0 / 30.0);
        if (!response_tick_phase_valid(c, phase))
            return 0;
        (*(volatile float *)&e->actor.phase = response_query_stored(phase));
        e->actor.tick = (unsigned short)(__int64)((short)bh_actor_route_now(c->damage->route) - phase * 30);
    }
    return 1;
}
