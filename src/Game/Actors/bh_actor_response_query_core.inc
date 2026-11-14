/* Complete0043a1d0 candidate; source context, guard and selection adapters are
   explicit. Original globals are never reconstructed as invented pointers. */
static __inline float response_query_stored(double value)
{
    union {
        float value;
        volatile unsigned long bits;
    } written, read;
    written.value = (float)value;
    read.bits = written.bits;
    return read.value;
}
static int response_query_timing_valid(BHActorResponseContext *context, double duration, double phase)
{
    double ticks = duration * 30;
    double start = (short)bh_actor_route_now(context->damage->route) - phase * 30;
    if (!_finite(ticks) || !_finite(start) || fabs(ticks) >= 9223372036854775808.0 ||
        fabs(start) >= 9223372036854775808.0)
    {
        context->error = 3;
        return 0;
    }
    return 1;
}
static BHEntity *response_query_select(BHActorResponseContext *context, BHEntity *e, const BHVec3 *position)
{
    BHTargetSelection result;
    if (!bh_target_select(context->player, context->vehicles->head, e, position, context->damage->route->actor_limit, &result))
    {
        context->error = 2;
        return NULL;
    }
    return result.target;
}
#define RESPONSE_QUERY_CURVE(phase, destination)                                                                       \
    bh_camera_curve((phase), (destination), &actor->actor.route_start, &actor->actor.route_first, &actor->actor.route_second,      \
                    &actor->actor.route_end)

void bh_actor_response_query_core(BHEntity *actor, void *packet, BHActorResponseContext *context)
{
    unsigned long *p = (unsigned long *)packet;
    float *f = (float *)packet;
    BHVec3 at, before, after, delta;
    BHEntity *selected;
    unsigned long type;
    float t, length, scale;
    double severity, scale_wide, duration, phase, numerator, ratio, retained;
    double dx, dy, dz, root, factor, nx, ny, nz, px, py, pz;
    switch (p[0])
    {
    case 20:
        actor->step = release_tick;
        actor->refresh = 0;
        bh_entity_set_query(context->vehicles->pool, actor, 0);
        return;
    case 4:
        if (unlink_actor(context, actor))
        {
            bh_sound_stop(context->damage->route->sound, actor->actor.sound);
            bh_entity_release(context->vehicles->pool, actor);
        }
        return;
    case 17:
        if (p[1] == actor->actor.target_identity)
            actor->actor.target_identity = 0;
        if (p[1] == actor->actor.damage_source_identity)
            actor->actor.damage_source_identity = 0;
        return;
    case 13:
        p[5] = 1;
        p[6] = p[3];
        return;
    case 2:
        actor->step = bh_actor_response_tick;
        actor->actor.offset_velocity.x = response_query_stored((double)f[4] * f[7] + actor->actor.offset_velocity.x);
        actor->actor.offset_velocity.y = response_query_stored((double)f[5] * f[7] + actor->actor.offset_velocity.y);
        actor->actor.offset_velocity.z = response_query_stored((double)f[6] * f[7] + actor->actor.offset_velocity.z);
        return;
    case 11:
        if (actor->actor.fade >= .15)
        {
            retained = (double)actor->actor.fade - .1;
            actor->actor.fade = response_query_stored(retained);
            if (retained < .1)
                actor->actor.fade = .1f;
            actor->step = bh_actor_response_tick;
        }
        else if (actor->actor.destroy_when_faded)
        {
            actor->actor.damage = 2;
            actor->step = bh_actor_response_tick;
        }
        return;
    case 0:
        type = actor->actor.kind;
        severity = f[1];
        if (type == 6)
        {
            if (p[2] == (unsigned long)context->player)
            {
                severity *= .8;
                if (severity > .45)
                    severity = (double).45f;
            }
            else
                severity = 0;
        }
        if (p[2] && actor->actor.damage + severity > .2 && p[2] != (unsigned long)actor)
            actor->actor.target_identity = p[2];
        if (type == 2 || type == 6)
        {
            if (!actor->actor.speed)
                return;
            ratio = (double)actor->actor.phase / actor->actor.duration;
            numerator = ((double)actor->actor.speed * actor->actor.speed_scale) * actor->actor.duration;
            scale_wide = (double)actor->actor.speed_scale - severity * (-10.0);
            scale = response_query_stored(scale_wide);
            if (scale_wide > 2)
                scale = 2;
            duration = numerator / ((double)actor->actor.speed * scale);
            phase = ratio * duration;
            if (!response_query_timing_valid(context, duration, phase))
                return;
            actor->actor.speed_scale = response_query_stored(scale_wide);
            if (scale_wide > 2)
                actor->actor.speed_scale = 2;
            actor->actor.duration = response_query_stored(duration);
            actor->actor.phase = response_query_stored(phase);
            actor->actor.duration_ticks = (unsigned short)(__int64)(duration * 30);
            actor->actor.tick = (unsigned short)(__int64)((short)bh_actor_route_now(context->damage->route) - phase * 30);
        }
        actor->step = bh_actor_response_tick;
        retained = severity + actor->actor.damage;
        actor->actor.damage = response_query_stored(retained);
        if (retained >= 1)
            actor->actor.damage_source_identity = p[2];
        return;
    case 1:
    case 3:
    case 15:
    case 16:
        break;
    default:
        return;
    }
    ratio = (double)actor->actor.phase / actor->actor.duration;
    t = response_query_stored(ratio);
    if (p[0] == 3)
    {
        RESPONSE_QUERY_CURVE(t, (BHVec3 *)(f + 1));
        f[1] = response_query_stored((double)actor->actor.offset.x + f[1]);
        f[2] = response_query_stored((double)actor->actor.offset.y + f[2]);
        f[3] = response_query_stored((double)actor->actor.offset.z + f[3]);
        return;
    }
    /* The original network query samples before, at, after in that order. */
    if (p[0] == 16)
        RESPONSE_QUERY_CURVE(response_query_stored(ratio - 1.0 / 30.0), &before);
    RESPONSE_QUERY_CURVE(t, &at);
    if (p[0] == 15)
    {
        at.x = response_query_stored((double)actor->actor.offset.x + at.x);
        at.y = response_query_stored((double)actor->actor.offset.y + at.y);
        at.z = response_query_stored((double)actor->actor.offset.z + at.z);
        selected = response_query_select(context, actor, &at);
        if (!context->error)
            actor->actor.target_identity = (unsigned long)selected;
        return;
    }
    if (p[0] == 1)
    {
        dx = ((double)actor->actor.offset.x + at.x) - f[1];
        dy = ((double)actor->actor.offset.y + at.y) - f[2];
        dz = ((double)actor->actor.offset.z + at.z) - f[3];
        delta.x = response_query_stored(dx);
        delta.y = response_query_stored(dy);
        delta.z = response_query_stored(dz);
        root = sqrt((dx * delta.x + dy * delta.y) + dz * delta.z);
        length = response_query_stored(root);
        if (root > 0 && length < f[5])
        {
            factor = (((1.0 - (double)length / f[5]) * f[4]) * .003) / length;
            *(volatile float *)&actor->actor.offset_velocity =
                response_query_stored(factor * delta.x + actor->actor.offset_velocity.x);
            *(volatile float *)(&actor->actor.offset_velocity.y) =
                response_query_stored(factor * delta.y + actor->actor.offset_velocity.y);
            *(volatile float *)(&actor->actor.offset_velocity.z) =
                response_query_stored(factor * delta.z + actor->actor.offset_velocity.z);
            *(BHEntityStep volatile *)&actor->step = bh_actor_response_tick;
        }
        return;
    }
    RESPONSE_QUERY_CURVE(response_query_stored((double)t - (-1.0 / 30.0)), &after);
    nx = ((double)after.x - at.x) * 30;
    ny = ((double)after.y - at.y) * 30;
    nz = ((double)after.z - at.z) * 30;
    px = ((double)at.x - before.x) * 30;
    py = ((double)at.y - before.y) * 30;
    pz = ((double)at.z - before.z) * 30;
    f[4] = response_query_stored(nx);
    f[5] = response_query_stored(ny);
    f[6] = response_query_stored(nz);
    f[7] = response_query_stored((px - (double)f[4]) * 30);
    f[8] = response_query_stored((py - (double)f[5]) * 30);
    f[9] = response_query_stored((pz - nz) * 30);
    f[1] = response_query_stored((double)actor->actor.offset.x + at.x);
    f[2] = response_query_stored((double)actor->actor.offset.y + at.y);
    f[3] = response_query_stored((double)actor->actor.offset.z + at.z);
}
#undef RESPONSE_QUERY_CURVE
