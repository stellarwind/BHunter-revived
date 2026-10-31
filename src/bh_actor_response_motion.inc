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
#define RESPONSE_MOTION_FLOAT(o) (*(float *)((char *)e + (o)))
#define RESPONSE_MOTION_STORE(o, v) (*(volatile float *)((char *)e + (o)) = response_query_stored(v))
#define RESPONSE_MOTION_SHORT(o) (*(unsigned short *)((char *)e + (o)))
int __cdecl bh_actor_response_motion(BHEntity *e, BHActorResponseContext *c)
{
    double ratio, scale, numerator, duration, phase, fade, damage;
    double dx, dy, dz, distance, damping;
    float stored_scale, x, y, z, length;
    if (word(e, 0x198) != 0x3f800000UL)
    {
        ratio = (double)RESPONSE_MOTION_FLOAT(0x180) / RESPONSE_MOTION_FLOAT(0x184);
        scale = (double)RESPONSE_MOTION_FLOAT(0x198) * .97;
        numerator =
            ((double)RESPONSE_MOTION_FLOAT(0x188) * RESPONSE_MOTION_FLOAT(0x198)) * RESPONSE_MOTION_FLOAT(0x184);
        stored_scale = response_query_stored(scale);
        if (scale < 1)
            stored_scale = 1;
        duration = numerator / ((double)RESPONSE_MOTION_FLOAT(0x188) * stored_scale);
        phase = ratio * duration;
        if (!response_query_timing_valid(c, duration, phase))
            return 0;
        RESPONSE_MOTION_STORE(0x198, scale);
        if (scale < 1)
            RESPONSE_MOTION_STORE(0x198, 1);
        RESPONSE_MOTION_STORE(0x184, duration);
        RESPONSE_MOTION_STORE(0x180, phase);
        RESPONSE_MOTION_SHORT(0x1e6) = (unsigned short)(__int64)(duration * 30);
        RESPONSE_MOTION_SHORT(0x1e4) =
            (unsigned short)(__int64)((short)bh_actor_route_now(c->damage->route) - phase * 30);
    }
    fade = (double)RESPONSE_MOTION_FLOAT(0x19c) * 1.008;
    RESPONSE_MOTION_STORE(0x19c, fade);
    if (fade > 1)
        RESPONSE_MOTION_STORE(0x19c, 1);
    if (RESPONSE_MOTION_FLOAT(0x194) > 0)
    {
        damage = (double)RESPONSE_MOTION_FLOAT(0x194) * .998;
        RESPONSE_MOTION_STORE(0x194, damage);
        if (word(e, 0xfc) == 6)
            RESPONSE_MOTION_STORE(0x194, damage * .998);
    }
    dx = (double)RESPONSE_MOTION_FLOAT(0x15c) - RESPONSE_MOTION_FLOAT(0x168) * (-1.0 / 30.0);
    dy = (double)RESPONSE_MOTION_FLOAT(0x160) - RESPONSE_MOTION_FLOAT(0x16c) * (-1.0 / 30.0);
    dz = (double)RESPONSE_MOTION_FLOAT(0x164) - RESPONSE_MOTION_FLOAT(0x170) * (-1.0 / 30.0);
    x = response_query_stored(dx);
    y = response_query_stored(dy);
    z = response_query_stored(dz);
    distance = sqrt((dx * x + dy * y) + dz * z);
    RESPONSE_MOTION_STORE(0x15c, x);
    length = response_query_stored(distance);
    RESPONSE_MOTION_STORE(0x160, y);
    RESPONSE_MOTION_STORE(0x164, z);
    damping = (.99 - distance * distance * .001) - (double)length * .01;
    if (!(damping >= 0))
        damping = 0;
    RESPONSE_MOTION_STORE(0x15c, damping * dx);
    RESPONSE_MOTION_STORE(0x160, damping * y);
    RESPONSE_MOTION_STORE(0x164, damping * z);
    RESPONSE_MOTION_STORE(0x168, (double)RESPONSE_MOTION_FLOAT(0x168) * .7);
    RESPONSE_MOTION_STORE(0x16c, (double)RESPONSE_MOTION_FLOAT(0x16c) * .7);
    RESPONSE_MOTION_STORE(0x170, (double)RESPONSE_MOTION_FLOAT(0x170) * .7);
    if (word(e, 0x19c) == 0x3f800000UL)
        RESPONSE_MOTION_STORE(0x180, (double)RESPONSE_MOTION_FLOAT(0x180) - (-1.0 / 30.0));
    else
    {
        phase = (double)RESPONSE_MOTION_FLOAT(0x180) - RESPONSE_MOTION_FLOAT(0x19c) * (-1.0 / 30.0);
        if (!response_tick_phase_valid(c, phase))
            return 0;
        RESPONSE_MOTION_STORE(0x180, phase);
        RESPONSE_MOTION_SHORT(0x1e4) =
            (unsigned short)(__int64)((short)bh_actor_route_now(c->damage->route) - phase * 30);
    }
    return 1;
}
#undef RESPONSE_MOTION_FLOAT
#undef RESPONSE_MOTION_STORE
#undef RESPONSE_MOTION_SHORT
