#include "bh_actor_response.h"
#include "bh_camera_curve.h"
#include "bh_target_selection.h"
#include "bh_remote_prediction.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static unsigned long word(BHEntity *e, unsigned long o)
{
    unsigned long v;
    memcpy(&v, (char *)e + o, 4);
    return v;
}
static void setword(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static float number(BHEntity *e, unsigned long o)
{
    float v;
    memcpy(&v, (char *)e + o, 4);
    return v;
}
static void real(BHEntity *e, unsigned long o, float v)
{
    memcpy((char *)e + o, &v, 4);
}
static short short_value(BHEntity *e, unsigned long o)
{
    short v;
    memcpy(&v, (char *)e + o, 2);
    return v;
}
static BHActorResponseContext *context(BHEntity *e)
{
    BHActorResponseContext *c;
    memcpy(&c, e->payload + 588, 4);
    return c;
}
static int live(BHActorResponseContext *c, BHEntity *e)
{
    return bh_entity_is_active(c->vehicles->pool, e);
}
static void point(BHActorRouteState *s, float t, BHVec3 *p)
{
    bh_camera_curve(t, p, s->controls, s->controls + 1, s->controls + 2, s->controls + 3);
}
static void offset(BHEntity *e, BHVec3 *p)
{
    unsigned long i;
    for (i = 0; i < 3; ++i)
        ((float *)p)[i] = rf((double)number(e, 0x15c + 4 * i) + ((float *)p)[i]);
}
static int timing(BHActorResponseContext *c, BHActorRouteState *s, double duration, double phase)
{
    double ticks = duration * 30, start = (short)bh_actor_route_now(c->damage->route) - phase * 30;
    if (!_finite(ticks) || !_finite(start) || fabs(ticks) >= 9223372036854775808.0 ||
        fabs(start) >= 9223372036854775808.0)
    {
        c->error = 3;
        return 0;
    }
    s->duration = rf(duration);
    s->phase = rf(phase);
    s->duration_ticks = (unsigned short)(__int64)ticks;
    s->tick = (unsigned short)(__int64)start;
    return 1;
}
static int rescale(BHActorResponseContext *c, BHActorRouteState *s, float factor)
{
    double t = (double)s->phase / s->duration,
           d = ((double)s->speed * s->speed_scale * s->duration) / ((double)s->speed * factor);
    s->speed_scale = factor;
    return timing(c, s, d, t * d);
}
static int unlink_actor(BHActorResponseContext *c, BHEntity *e)
{
    BHActorRouteState s;
    BHPickupRoom *r = 0;
    BHEntity *p, *n;
    unsigned long i;
    bh_actor_route_read(e, &s);
    for (i = 0; i < c->damage->route->room_count; ++i)
        if (c->damage->route->rooms[i].world == s.current)
        {
            r = c->damage->route->rooms + i;
            break;
        }
    if (!r || !c->vehicles->count)
    {
        c->error = 2;
        return 0;
    }
    p = (BHEntity *)word(e, 0xf8);
    n = (BHEntity *)word(e, 0xf4);
    if ((p ? (BHEntity *)word(p, 0xf4) : r->actors) != e || (n && (BHEntity *)word(n, 0xf8) != e))
    {
        c->error = 2;
        return 0;
    }
    if (p)
        setword(p, 0xf4, (unsigned long)n);
    else
        r->actors = n;
    if (n)
        setword(n, 0xf8, (unsigned long)p);
    p = (BHEntity *)word(e, 0xec);
    n = (BHEntity *)word(e, 0xf0);
    if ((p ? (BHEntity *)word(p, 0xf0) : c->vehicles->head) != e || (n && (BHEntity *)word(n, 0xec) != e))
    {
        c->error = 2;
        return 0;
    }
    if (p)
        setword(p, 0xf0, (unsigned long)n);
    else
        c->vehicles->head = n;
    if (n)
        setword(n, 0xec, (unsigned long)p);
    --c->vehicles->count;
    return 1;
}
static void release_tick(BHEntity *e)
{
    BHActorResponseContext *c = context(e);
    if (!bh_actor_vehicle_release(c->vehicles, e))
        c->error = 2;
}
int __cdecl bh_actor_response_bind(BHActorResponseContext *c, BHEntity *e)
{
    if (!c || !c->damage || !c->damage->route || !c->attack || !c->vehicles || !c->vehicles->pool || !e ||
        c->damage->pool != c->vehicles->pool || c->vehicles->route != c->damage->route || !live(c, e))
        return 0;
    memcpy(e->payload + 588, &c, 4);
    return bh_entity_set_query(c->vehicles->pool, e, bh_actor_response_query);
}
void __cdecl bh_actor_response_query(BHEntity *e, void *packet)
{
    BHActorResponseContext *c = context(e);
    unsigned long *p = (unsigned long *)packet;
    float *f = (float *)packet;
    BHActorRouteState s;
    BHVec3 a, b, d;
    BHTargetSelection selected;
    double severity, factor;
    float t, length, v, old;
    unsigned long i, type;
    if (!c || !packet || !live(c, e))
        return;
    c->error = 0;
    switch (p[0])
    {
    case 20:
        e->step = release_tick;
        e->refresh = 0;
        bh_entity_set_query(c->vehicles->pool, e, 0);
        return;
    case 4:
        if (unlink_actor(c, e))
        {
            bh_sound_stop(c->damage->route->sound, word(e, 0x104));
            bh_entity_release(c->vehicles->pool, e);
        }
        return;
    case 17:
        if (p[1] == word(e, 0x1b8))
            setword(e, 0x1b8, 0);
        if (p[1] == word(e, 0x1ac))
            setword(e, 0x1ac, 0);
        return;
    case 13:
        p[5] = 1;
        p[6] = p[3];
        return;
    case 2:
        e->step = bh_actor_response_tick;
        for (i = 0; i < 3; ++i)
            real(e, 0x168 + 4 * i, rf((double)f[4 + i] * f[7] + number(e, 0x168 + 4 * i)));
        return;
    case 11:
        v = number(e, 0x19c);
        if (v >= .15)
        {
            v = rf((double)v - .1);
            if (v < .1)
                v = .1f;
            real(e, 0x19c, v);
            e->step = bh_actor_response_tick;
        }
        else if (word(e, 0x118))
        {
            real(e, 0x194, 2);
            e->step = bh_actor_response_tick;
        }
        return;
    case 0:
    case 1:
    case 3:
    case 15:
    case 16:
        break;
    default:
        return;
    }
    if (!bh_math_environment_supported())
    {
        c->error = 3;
        return;
    }
    bh_actor_route_read(e, &s);
    if (s.duration <= 0 || !_finite(s.duration))
    {
        c->error = 3;
        return;
    }
    if (!p[0])
    {
        type = word(e, 0xfc);
        severity = f[1];
        if (type == 6)
        {
            if (p[2] == (unsigned long)c->player)
            {
                severity *= .8;
                if (severity > .45)
                    severity = (double).45f;
            }
            else
                severity = 0;
        }
        if (p[2] && p[2] != (unsigned long)e && number(e, 0x194) + severity > .2)
            setword(e, 0x1b8, p[2]);
        if (type == 2 || type == 6)
        {
            v = rf((double)s.speed_scale - severity * (-10.0));
            if (v > 2)
                v = 2;
            if (!s.speed || !rescale(c, &s, v))
                return;
            bh_actor_route_write(e, &s);
        }
        e->step = bh_actor_response_tick;
        factor = severity + number(e, 0x194);
        real(e, 0x194, rf(factor));
        if (factor >= 1)
            setword(e, 0x1ac, p[2]);
        return;
    }
    t = rf((double)s.phase / s.duration);
    point(&s, t, &a);
    if (p[0] == 3)
    {
        offset(e, &a);
        memcpy(f + 1, &a, 12);
        return;
    }
    if (p[0] == 15)
    {
        offset(e, &a);
        if (!bh_target_select(c->player, c->vehicles->head, e, &a, c->damage->route->actor_limit, &selected))
        {
            c->error = 2;
            return;
        }
        setword(e, 0x1b8, (unsigned long)selected.target);
        return;
    }
    if (p[0] == 1)
    {
        offset(e, &a);
        for (i = 0; i < 3; ++i)
            ((float *)&d)[i] = rf((double)((float *)&a)[i] - f[1 + i]);
        length = rf(sqrt((double)d.x * d.x + (double)d.y * d.y + (double)d.z * d.z));
        if (length > 0 && length < f[5])
        {
            factor = (((1.0 - (double)length / f[5]) * f[4]) * .003) / length;
            for (i = 0; i < 3; ++i)
                real(e, 0x168 + 4 * i, rf(factor * ((float *)&d)[i] + number(e, 0x168 + 4 * i)));
            e->step = bh_actor_response_tick;
        }
        return;
    }
    point(&s, rf((double)t - 1.0 / 30.0), &b);
    point(&s, rf((double)t - (-1.0 / 30.0)), &d);
    for (i = 0; i < 3; ++i)
    {
        v = rf(((double)((float *)&d)[i] - ((float *)&a)[i]) * 30);
        old = rf(((double)((float *)&a)[i] - ((float *)&b)[i]) * 30);
        f[4 + i] = v;
        f[7 + i] = rf(((double)old - v) * 30);
        f[1 + i] = rf((double)number(e, 0x15c + i * 4) + ((float *)&a)[i]);
    }
}
void __cdecl bh_actor_response_tick(BHEntity *e)
{
    BHActorResponseContext *c = context(e);
    BHActorDamageContext *d;
    BHActorRouteState s;
    BHVec3 displacement, impulse, p, before, at, after, velocity, spin, acceleration = {0, -.002f, 0};
    BHScriptLifetimeState mission;
    unsigned long i, type;
    float damage, factor, t, angular, angle, left;
    double phase, value, retained[3], distance, damping;
    long first = -1, second = -1, row, loot = 0;
    BHEntity *a;
    struct
    {
        long event;
        BHEntity *actor;
        long tag;
    } packet;
    if (!c || !live(c, e))
        return;
    c->error = 0;
    d = c->damage;
    if (!d->particles || !d->fragments || !d->targets || !d->loot_rows || !c->mission ||
        !bh_math_environment_supported())
    {
        c->error = 1;
        return;
    }
    if (!bh_actor_attack(c->attack, e))
    {
        c->error = 2;
        return;
    }
    bh_actor_route_read(e, &s);
    if (s.duration <= 0 || !s.speed)
    {
        c->error = 3;
        return;
    }
    type = word(e, 0xfc);
    if (s.speed_scale != 1)
    {
        factor = rf((double)s.speed_scale * .97);
        if (factor < 1)
            factor = 1;
        if (!rescale(c, &s, factor))
            return;
    }
    value = (double)number(e, 0x19c) * 1.008;
    real(e, 0x19c, value > 1 ? 1 : rf(value));
    damage = number(e, 0x194);
    if (damage > 0)
    {
        value = (double)damage * .998;
        real(e, 0x194, rf(value));
        if (type == 6)
            real(e, 0x194, rf(value * .998));
    }
    damage = number(e, 0x194);
    /* 00439a52..00439ba7: FST retains each displacement for its product;
    sqrt is retained for the quadratic term, binary32 for the linear term.
    Damping is not stored before multiplication, and X retains displacement
    while Y/Z reload their binary32 copies. Do not symmetrize these spills. */
    for (i = 0; i < 3; ++i)
    {
        ((float *)&impulse)[i] = number(e, 0x168 + 4 * i);
        retained[i] = (double)number(e, 0x15c + 4 * i) - ((float *)&impulse)[i] * (-1.0 / 30.0);
        ((float *)&displacement)[i] = rf(retained[i]);
    }
    distance = sqrt((retained[0] * displacement.x + retained[1] * displacement.y) + retained[2] * displacement.z);
    damping = (.99 - distance * distance * .001) - (double)rf(distance) * .01;
    if (_isnan(damping) || damping < 0)
        damping = 0;
    for (i = 0; i < 3; ++i)
    {
        real(e, 0x15c + 4 * i, rf(damping * (i ? ((float *)&displacement)[i] : retained[0])));
        real(e, 0x168 + 4 * i, rf((double)((float *)&impulse)[i] * .7));
    }
    phase = (double)s.phase - (number(e, 0x19c) == 1 ? 1.0 : (double)number(e, 0x19c)) * (-1.0 / 30.0);
    s.phase = rf(phase);
    if (number(e, 0x19c) != 1)
    {
        double start = (short)bh_actor_route_now(d->route) - phase * 30;
        if (!_finite(start) || fabs(start) >= 9223372036854775808.0)
        {
            c->error = 3;
            return;
        }
        s.tick = (unsigned short)(__int64)start;
    }
    bh_actor_route_write(e, &s);
    if (s.phase >= s.duration)
    {
        left = rf((double)s.phase - s.duration);
        if (!bh_actor_pursuit_route(c->attack->pursuit, e))
        {
            c->error = 2;
            return;
        }
        real(e, 0x180, left);
        bh_actor_route_read(e, &s);
    }
    if (damage < 1 && damage * 5000 <= rand())
        goto resume;
    velocity.x = rf((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.y = rf((double)(rand() - 16384) * (1.0 / 262144.0));
    velocity.z = rf((double)(rand() - 16384) * (1.0 / 262144.0));
    t = rf((double)s.phase / s.duration);
    point(&s, t, &p);
    offset(e, &p);
    angular = rf((double)(rand() - 16384) * (1.0 / 16384.0));
    angle = rf((double)rand() * (1.0 / 512.0));
    bh_script_particle_create(d->particles, d->smoke_resource, 3, &p, &velocity, &acceleration, .5f, angle, 12, angular,
                              .9f, .95f, 2, .3f);
    if (damage >= 1)
    {
        /* Death-only legacy orientation must not freeze live movement/damage. */
        if (!d->legacy_death_angles)
        {
            c->error = 1;
            return;
        }
        bh_entity_set_query(d->pool, e, 0);
        bh_sound_stop(d->route->sound, s.sound);
        point(&s, rf((double)t - 1.0 / 30.0), &before);
        point(&s, t, &at);
        point(&s, rf((double)t - (-1.0 / 30.0)), &after);
        for (i = 0; i < 3; ++i)
            ((float *)&velocity)[i] = rf(((double)((float *)&after)[i] - ((float *)&at)[i]) * 15);
        bh_script_debris_create(d->fragments, (rand() & 7) + 1, &p, 8, 10, 1);
        bh_sound_request(d->route->sound, d->death_sound, 2, rf(.75 - (double)rand() * (-1.0 / 65536.0)), &p,
                         &velocity);
        for (i = 0; i < 3; ++i)
            ((float *)&spin)[i] = rf(((double)rand() - 16384) * (1.0 / 4096.0));
        i = (unsigned short)short_value(e, 0x1dc);
        if (i == 0)
            first = 10;
        else if (i == 1)
            first = 12;
        else if (i == 2)
            first = 11;
        i = (unsigned short)short_value(e, 0x1de);
        if (i == 0)
            second = 5;
        else if (i == 1)
            second = 3;
        else if (i == 2)
            second = 6;
        row = type == 6 ? 3 : type == 1 ? 1 : 2;
        if (!bh_remote_choose_slot(d->loot_rows, d->loot_row_count, row, first, second, &loot))
        {
            c->error = 1;
            return;
        }
        if (type == 6 && (a = bh_entity_acquire(d->pool)) != 0)
        {
            memset(&mission, 0, sizeof(mission));
            mission.mission_countdown = 1.5f;
            mission.mission_resource = c->mission_resource;
            bh_script_lifetime_bind(c->mission, a, &mission, 0);
            a->step = bh_mission_sound_tick;
        }
        bh_target_construct_moving(d->targets, &p, d->legacy_death_angles, &velocity, &spin, word(e, 0x114), 10000,
                                   rf(1.0 - (double)rand() * (-1.0 / 16384.0)), (BHEntity *)word(e, 0x1ac),
                                   (long)word(e, 0x100), loot, type == 6);
        packet.event = 17;
        packet.actor = e;
        bh_entities_query(d->pool, 0, 0, 0, 0, &packet);
        if (word(e, 0x11c))
        {
            packet.event = 14;
            packet.tag = (long)word(e, 0x11c);
            bh_entities_query(d->pool, 0, 0, 0, 0, &packet);
        }
        if (unlink_actor(c, e))
            bh_entity_release(d->pool, e);
        return;
    }
resume:
    if (damage < .05 && word(e, 0x198) == 0x3f800000UL && word(e, 0x19c) == 0x3f800000UL)
        e->step = bh_actor_vehicle_tick;
}
