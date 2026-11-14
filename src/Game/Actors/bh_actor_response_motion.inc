#include "../Entities/bh_field_access.h"
/* Arithmetic prefix of00439910, not a separate original function. The checked
   caller supplies a live actor, positive duration and nonzero speed. */
static int response_tick_phase_valid(BHActorResponseContext *context, double phase)
{
    double start = (short)bh_actor_route_now(context->damage->route) - phase * 30;
    if (!_finite(start) || fabs(start) >= 9223372036854775808.0)
    {
        context->error = 3;
        return 0;
    }
    return 1;
}
int bh_actor_response_motion(BHEntity *actor, BHActorResponseContext *context)
{
    double ratio, scale, numerator, duration, phase, fade, damage;
    double dx, dy, dz, distance, damping;
    float stored_scale, x, y, z, length;
    if (bh_field_u32(&actor->actor.speed_scale) != 0x3f800000UL)
    {
        ratio = (double)actor->actor.phase / actor->actor.duration;
        scale = (double)actor->actor.speed_scale * .97;
        numerator = ((double)actor->actor.speed * actor->actor.speed_scale) * actor->actor.duration;
        stored_scale = response_query_stored(scale);
        if (scale < 1)
            stored_scale = 1;
        duration = numerator / ((double)actor->actor.speed * stored_scale);
        phase = ratio * duration;
        if (!response_query_timing_valid(context, duration, phase))
            return 0;
        (*(volatile float *)&actor->actor.speed_scale = response_query_stored(scale));
        if (scale < 1)
            (*(volatile float *)&actor->actor.speed_scale = response_query_stored(1));
        (*(volatile float *)&actor->actor.duration = response_query_stored(duration));
        (*(volatile float *)&actor->actor.phase = response_query_stored(phase));
        actor->actor.duration_ticks = (unsigned short)(__int64)(duration * 30);
        actor->actor.tick = (unsigned short)(__int64)((short)bh_actor_route_now(context->damage->route) - phase * 30);
    }
    fade = (double)actor->actor.fade * 1.008;
    (*(volatile float *)&actor->actor.fade = response_query_stored(fade));
    if (fade > 1)
        (*(volatile float *)&actor->actor.fade = response_query_stored(1));
    if (actor->actor.damage > 0)
    {
        damage = (double)actor->actor.damage * .998;
        (*(volatile float *)&actor->actor.damage = response_query_stored(damage));
        if (bh_field_u32(&actor->actor.kind) == 6)
            (*(volatile float *)&actor->actor.damage = response_query_stored(damage * .998));
    }
    dx = (double)actor->actor.offset.x - actor->actor.offset_velocity.x * (-1.0 / 30.0);
    dy = (double)actor->actor.offset.y - actor->actor.offset_velocity.y * (-1.0 / 30.0);
    dz = (double)actor->actor.offset.z - actor->actor.offset_velocity.z * (-1.0 / 30.0);
    x = response_query_stored(dx);
    y = response_query_stored(dy);
    z = response_query_stored(dz);
    distance = sqrt((dx * x + dy * y) + dz * z);
    (*(volatile float *)&actor->actor.offset.x = response_query_stored(x));
    length = response_query_stored(distance);
    (*(volatile float *)&actor->actor.offset.y = response_query_stored(y));
    (*(volatile float *)&actor->actor.offset.z = response_query_stored(z));
    damping = (.99 - distance * distance * .001) - (double)length * .01;
    if (!(damping >= 0))
        damping = 0;
    (*(volatile float *)&actor->actor.offset.x = response_query_stored(damping * dx));
    (*(volatile float *)&actor->actor.offset.y = response_query_stored(damping * y));
    (*(volatile float *)&actor->actor.offset.z = response_query_stored(damping * z));
    (*(volatile float *)&actor->actor.offset_velocity.x = response_query_stored((double)actor->actor.offset_velocity.x * .7));
    (*(volatile float *)&actor->actor.offset_velocity.y = response_query_stored((double)actor->actor.offset_velocity.y * .7));
    (*(volatile float *)&actor->actor.offset_velocity.z = response_query_stored((double)actor->actor.offset_velocity.z * .7));
    if (bh_field_u32(&actor->actor.fade) == 0x3f800000UL)
        (*(volatile float *)&actor->actor.phase = response_query_stored((double)actor->actor.phase - (-1.0 / 30.0)));
    else
    {
        phase = (double)actor->actor.phase - actor->actor.fade * (-1.0 / 30.0);
        if (!response_tick_phase_valid(context, phase))
            return 0;
        (*(volatile float *)&actor->actor.phase = response_query_stored(phase));
        actor->actor.tick = (unsigned short)(__int64)((short)bh_actor_route_now(context->damage->route) - phase * 30);
    }
    return 1;
}
