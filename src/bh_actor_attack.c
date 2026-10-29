#include "bh_actor_attack.h"
#include "bh_actor_vehicle_step.inc"
#include "bh_camera_curve.h"
#include "bh_target_steering.h"
#include "bh_draw_queue.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
static const BHVec3 up = {0, 1, 0}, zero = {0, 0, 0}, gravity = {0, -.002f, 0};
static float rf(double d)
{
    float f;
    bh_round_float(&f, d);
    return f;
}
/* Preserve 00437e7f..00437eb5: inv*component precedes the direction
   product on each axis. VC5 otherwise factors inv out of the X/Y sum.
   This call's binary64 argument/spill is an identity in the PC53 profile. */
static double attack_product(double a, double b)
{
    return a * b;
}
static void get(BHEntity *e, unsigned long o, void *p, unsigned long n)
{
    memcpy(p, (char *)e + o, n);
}
static void put(BHEntity *e, unsigned long o, const void *p, unsigned long n)
{
    memcpy((char *)e + o, p, n);
}
static long word(BHEntity *e, unsigned long o)
{
    long v;
    get(e, o, &v, 4);
    return v;
}
static void setword(BHEntity *e, unsigned long o, long v)
{
    put(e, o, &v, 4);
}
static float number(BHEntity *e, unsigned long o)
{
    float v;
    get(e, o, &v, 4);
    return v;
}
static void real(BHEntity *e, unsigned long o, float v)
{
    put(e, o, &v, 4);
}
static short short_value(BHEntity *e, unsigned long o)
{
    short v;
    get(e, o, &v, 2);
    return v;
}
static void setshort_value(BHEntity *e, unsigned long o, long v)
{
    unsigned short u = (unsigned short)v;
    put(e, o, &u, 2);
}
static void scale(BHVec3 *v, double s)
{
    v->x = rf(v->x * s);
    v->y = rf(v->y * s);
    v->z = rf(v->z * s);
}
static void curve(BHActorRouteState *s, float t, BHVec3 *out)
{
    bh_camera_curve(t, out, s->controls, s->controls + 1, s->controls + 2, s->controls + 3);
}
static void offset(BHEntity *e, BHVec3 *p)
{
    unsigned long i;
    for (i = 0; i < 3; ++i)
        ((float *)p)[i] = rf((double)number(e, 0x15c + i * 4) + ((float *)p)[i]);
}
static int live(BHActorAttackContext *c, BHEntity *e, unsigned long gen)
{
    return bh_entity_is_active(c->effects->effects->pool, e) && e->generation == gen;
}
typedef struct Motion
{
    long event;
    BHVec3 position, velocity, acceleration;
} Motion;
static int motion(BHActorAttackContext *c, BHEntity *e, BHEntity *target, Motion *p)
{
    unsigned long gen = e->generation;
    if (!target || !target->query)
    {
        c->error = 2;
        return 0;
    }
    p->event = 16;
    target->query(target, p);
    if (!live(c, e, gen))
    {
        c->error = 3;
        return 0;
    }
    return 1;
}
static void particle(BHActorAttackContext *c, unsigned long image, const BHVec3 *p, const BHVec3 *v, const BHVec3 *a,
                     float size, float rate, float life, float alpha, double spin_scale, double angle_scale,
                     float decay, float angular_decay)
{
    float spin = rf((double)(rand() - 16384) * spin_scale), angle = rf((double)rand() * angle_scale);
    bh_script_particle_create(c->effects->effects->particles, image, 3, p, v, a, size, angle, rate, spin, decay,
                              angular_decay, life, alpha);
}
static BHEntity *response(BHActorAttackContext *c, BHWorldSegmentHit *h)
{
    unsigned long i;
    BHHomingProjectileContext *b = c->effects->effects;
    if (h->object.kind == BH_WORLD_HIT_ACTOR)
        return h->object.object.actor;
    for (i = 0; i < b->response_count; ++i)
        if (b->responses[i].world == h->object.object.geometry)
            return b->responses[i].receiver;
    c->error = 2;
    return 0;
}
static void beam(BHActorAttackContext *c, BHEntity *e, BHWorldGeometry *world, const BHVec3 *fire,
                 const BHVec3 *direction, const BHVec3 *travel)
{
    BHHomingProjectileContext *b = c->effects->effects;
    BHVec3 velocity, end, ray, point, normal, marker_point;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    BHEntity *receiver, *flash, *contact;
    BHImpactState *impact;
    float distance = 125, alpha;
    unsigned long i, flags, gen = e->generation;
    int result;
    double dot, spin;
    struct
    {
        long event;
        BHVec3 point, normal;
        float impulse, severity;
    } packet;
    struct
    {
        long event;
        float amount;
        BHEntity *source;
    } damage;
    bh_sound_request(b->sound, c->ballistic_sound, 1, rf(.9 - (double)rand() * (-.000006103515625)), fire, travel);
    bh_flare_queue(b->flash->queue, fire, 2, 30, c->flash_image, 0, 0, 0, rf((double)rand() * (1.0 / 5200.0)));
    for (i = 0; i < 3; ++i)
        ((float *)&velocity)[i] = rf((double)(rand() - 16384) * (1.0 / 262144.0) - ((float *)direction)[i] * (-.4));
    particle(c, c->smoke_image, fire, &velocity, &gravity, .1f, 8, .5f, .2f, 1.0 / 16384.0, 1.0 / 512.0, .9f, .95f);
    ray = velocity;
    for (i = 0; i < 3; ++i)
        ((float *)&ray)[i] = rf((double)((float *)&ray)[i] - ((double)rand() - 16384) * (-.000001220703125));
    bh_vec3_normalize(&ray);
    for (i = 0; i < 3; ++i)
        ((float *)&end)[i] = rf((3.0 - (double)rand() * (-1.0 / 8192.0)) * ((float *)&ray)[i] + ((float *)fire)[i]);
    bh_draw_segment(b->particles->queue, 3, c->beam_image, fire, &end, .3f, .01f, 1, 0, 0);
    for (i = 0; i < 3; ++i)
        ((float *)&end)[i] = rf((double)((float *)fire)[i] - ((float *)&ray)[i] * (-125.0));
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = e;
    result = bh_world_segment_run(b->segment, &self, &world, fire, &end, 3, &hit);
    if (result < 0)
    {
        c->error = 2;
        return;
    }
    if (result)
    {
        receiver = response(c, &hit);
        if (c->error)
            return;
        point = hit.point;
        normal = hit.normal;
        distance = rf((double)hit.fraction * 125);
        flags = hit.object.kind == BH_WORLD_HIT_GEOMETRY ? (unsigned long)hit.material_flags : 0;
        if (!(flags & 4))
        {
            bh_sound_request(b->sound, c->impact_sound, 1, 2, &point, &zero);
            particle(c, c->impact_image, &point, 0, 0, .2f, 5, .5f, 1, 1.0 / 8192.0, 1.0 / 4096.0, .8f, 1);
            particle(c, c->impact_image, &point, 0, 0, .1f, 8, .4f, 1, 1.0 / 8192.0, 1.0 / 4096.0, .8f, 1);
            bh_script_particle_create(b->particles, c->flash_image, 3, &point, 0, 0, .1f, 0, 12, 0, .8f, 1, .3f, 1);
            flash = bh_entity_acquire(b->pool);
            if (flash)
            {
                flash->position = point;
                bh_flash_flare_bind(b->flash, flash, .1f, 30, 1);
            }
            bh_contact_spark_create(b->contacts, &point, .75f, 3);
            bh_contact_spark_create(b->contacts, &point, 1, 2);
            if (hit.object.kind == BH_WORLD_HIT_GEOMETRY && !(flags & 2))
            {
                /* 004388c1..00438913 sum X/Y before Z; the decompiler's
                   provisional Z/Y/X rendering loses that retained order. */
                dot = ((double)normal.x * ray.x + (double)normal.y * ray.y) + (double)normal.z * ray.z;
                if (dot > 0)
                {
                    normal.x = -normal.x;
                    normal.y = -normal.y;
                    normal.z = -normal.z;
                }
                for (i = 0; i < 3; ++i)
                    ((float *)&marker_point)[i] = rf((double)((float *)&point)[i] - ((float *)&normal)[i] * (-.1));
                if (!receiver)
                {
                    c->error = 2;
                    return;
                }
                bh_hit_marker_create(b->markers, &marker_point, &normal, .7f, 3, receiver);
            }
            for (i = 0; i < 3; ++i)
                ((float *)&velocity)[i] = rf((double)(rand() - 16384) * (1.0 / 262144.0));
            if (!(rand() & 1))
                particle(c, c->impact_image, &point, &velocity, &gravity, .1f, 8, .5f, .5f, 1.0 / 16384.0, 1.0 / 512.0,
                         .9f, .95f);
            else
                particle(c, c->smoke_image, &point, &velocity, &gravity, .1f, 8, 1, .75f, 1.0 / 16384.0, 1.0 / 512.0,
                         .9f, .95f);
            if (rand() & 1)
                bh_script_debris_create(c->effects->debris, 1, &point, 3, 5, .5f);
            if (receiver && receiver->query)
            {
                packet.event = 2;
                packet.point = point;
                packet.normal = ray;
                scale(&packet.normal, -1);
                packet.impulse = .1f;
                packet.severity = c->legacy_contact_severity;
                receiver->query(receiver, &packet);
                if (!live(c, e, gen))
                {
                    c->error = 3;
                    return;
                }
                damage.event = 0;
                damage.amount = .15f;
                damage.source = e;
                if (receiver->query)
                    receiver->query(receiver, &damage);
            }
        }
        else
        {
            for (i = 0; i < 3; ++i)
                ((float *)&marker_point)[i] = rf((double)((float *)&point)[i] - ((float *)&normal)[i] * (-.5));
            alpha = rf(.7 - (double)rand() * (-.00000152587890625));
            contact = bh_contact_impact_create(b->contacts, &marker_point, &normal, 4);
            if (contact)
            {
                impact = bh_contact_impact_state(b->contacts, contact);
                impact->duration = alpha;
                impact->size = 7;
                bh_sound_request(b->sound, c->liquid_sound, rf(7.0 / 6.0), rf(1.0 / alpha), &marker_point, &zero);
            }
        }
    }
    /* 00438c28..00438c38 retain the scaled random distance through every
       position product. It is not stored to binary32 before use. */
    spin = ((double)rand() * (1.0 / 32768.0)) * distance;
    for (i = 0; i < 3; ++i)
    {
        ((float *)&point)[i] = rf((double)spin * ((float *)&ray)[i] + ((float *)fire)[i]);
        ((float *)&end)[i] = rf((double)((float *)&point)[i] - ((float *)&ray)[i] * (-3.0));
    }
    bh_draw_segment(b->particles->queue, 3, c->beam_image, &point, &end, .3f, .01f, .5f, 0, 0);
    for (i = 0; i < 3; ++i)
        ((float *)&end)[i] = rf((double)((float *)&point)[i] - ((float *)&ray)[i] * 3.0);
    bh_draw_segment(b->particles->queue, 3, c->beam_image, &point, &end, .3f, .01f, .5f, 0, 0);
    ++c->beams;
}
void __cdecl bh_actor_attack_fire_vectors(BHEntity *e, float t, BHVec3 *p, BHVec3 *direction,
                                         BHVec3 *side, BHVec3 *travel)
{
    BHActorRouteState s;
    BHVec3 q;
    unsigned long i;
    bh_actor_route_read(e, &s);
    curve(&s, t, p);
    curve(&s, rf((double)t - (-1.0 / 30.0)), &q);
    offset(e, p);
    for (i = 0; i < 3; ++i)
    {
        /* 00437fc6..0043802d retain each following position through its
           FST, subtract the rounded fire point, then retain the difference
           through the direction FST for the travel-velocity product. */
        double delta = ((double)number(e, 0x15c + 4 * i) + ((float *)&q)[i]) - ((float *)p)[i];
        ((float *)direction)[i] = rf(delta);
        ((float *)travel)[i] = rf(delta * 30);
    }
    bh_vec3_normalize(direction);
    bh_vec3_cross(direction, &up, side);
}
int __cdecl bh_actor_attack(BHActorAttackContext *c, BHEntity *e)
{
    BHEntity *target;
    BHActorRouteState s;
    Motion m;
    BHVec3 predicted, change, p, q, direction, side, travel, difference;
    BHHomingProjectileContext *b;
    float t, pursuit, distance;
    short frame, cooldown, secondary, primary;
    unsigned short next;
    unsigned long i, gen;
    long total;
    int forced, have_motion = 0;
    double dot, inv, distance_exact, delta[3];
    if (!c || !e || !c->pursuit || !c->effects || !c->effects->effects || !c->projectiles || !c->area ||
        !bh_math_environment_supported())
        return 0;
    b = c->effects->effects;
    c->error = 0;
    target = (BHEntity *)word(e, 0x1b8);
    bh_actor_route_read(e, &s);
    if (!target)
        return 1;
    if (s.duration == 0)
    {
        c->error = 2;
        return 0;
    }
    t = rf((double)s.phase / s.duration);
    pursuit = number(e, 0x1b4);
    forced = word(e, 0x1d4) != 0;
    if (forced)
    {
        if (!motion(c, e, target, &m))
            return 0;
        have_motion = 1;
        for (i = 0; i < 3; ++i)
        {
            double v = (double)((float *)&m.velocity)[i] - ((float *)&m.acceleration)[i] * (-.5);
            ((float *)&predicted)[i] = rf((double)((float *)&m.position)[i] - v * (-.5));
        }
        bh_world_clamp_point(s.current, &predicted);
        for (i = 0; i < 3; ++i)
        {
            double delta = (double)((float *)&predicted)[i] - ((float *)&s.controls[3])[i];
            double adjustment;
            /* 00437c3e spills Z difference; X/Y remain in x87. X/Y
               endpoint sums retain adjustment, Z reloads its float copy. */
            if (i == 2)
                delta = rf(delta);
            adjustment = (double)pursuit * delta * .03;
            ((float *)&change)[i] = rf(adjustment);
            ((float *)&s.controls[3])[i] = rf((i == 2 ? (double)((float *)&change)[i] : adjustment) + ((float *)&s.controls[3])[i]);
            ((float *)&s.controls[2])[i] = rf((double)((float *)&change)[i] + ((float *)&s.controls[2])[i]);
        }
        bh_world_clamp_point(s.current, s.controls + 3);
        bh_world_clamp_point(s.current, s.controls + 2);
        bh_actor_route_write(e, &s);
    }
    frame = short_value(e, 0x1e0);
    if (frame < 0)
    {
        setshort_value(e, 0x1e0, frame + 1);
        return 1;
    }
    if (!frame)
    {
        short protected_player = target == c->pursuit->player ? short_value(target, 0x2b4) : 0;
        if (protected_player)
            return 1;
        if (!have_motion && !motion(c, e, target, &m))
            return 0;
        curve(&s, t, &p);
        offset(e, &p);
        /* FST retains each difference for its product and the range
           comparison; only the later square root reloads distance. */
        for (i = 0; i < 3; ++i)
        {
            delta[i] = (double)((float *)&m.position)[i] - ((float *)&p)[i];
            ((float *)&difference)[i] = rf(delta[i]);
        }
        distance_exact = (delta[0] * difference.x + delta[1] * difference.y) + delta[2] * difference.z;
        distance = rf(distance_exact);
        if (_isnan(distance_exact) || distance_exact <= 40000)
        {
            curve(&s, rf((double)t - (-1.0 / 30.0)), &q);
            for (i = 0; i < 3; ++i)
                ((float *)&direction)[i] = rf((double)((float *)&q)[i] - ((float *)&p)[i]);
            bh_vec3_normalize(&direction);
            inv = 1.0 / sqrt(distance);
            dot = (attack_product(inv, difference.x) * direction.x +
                   attack_product(inv, difference.y) * direction.y) +
                  attack_product(inv, difference.z) * direction.z;
            /* Original TEST AH,41 accepts unordered too: coincident
               target/actor positions must retain that firing decision. */
            if (_isnan(dot) || .98 - (double)pursuit * .03 <= dot)
                setshort_value(e, 0x1e0, 1);
        }
        return 1;
    }
    next = (unsigned short)(frame + 1);
    setshort_value(e, 0x1e0, next);
    total = word(e, 0x1d8);
    if ((short)next == total)
    {
        cooldown = short_value(e, 0x1e2);
        setshort_value(e, 0x1e0, -(long)cooldown);
        return 1;
    }
    if (next & 1)
        return 1;
    bh_actor_attack_fire_vectors(e, t, &p, &direction, &side, &travel);
    /* Original emitters reload these live fields after the synchronous motion
       query (004380e8/00438176 and corresponding primary-weapon branches). */
    get(e, 0x1a4, &s.current, 4);
    target = (BHEntity *)word(e, 0x1b8);
    ++c->attacks;
    if (next == 2 && rand() < 0x1000)
    {
        secondary = short_value(e, 0x1de);
        setshort_value(e, 0x1e0, total - 1);
        ++c->specials;
        if (secondary == 0)
        {
            if (target == c->pursuit->player)
                bh_sound_request(b->sound, c->orbit_cue, 1, 1, 0, 0);
            bh_effect_orbit_create(c->effects, s.current, e, &p, &travel, &side, &up, &direction, 8, target);
        }
        else if (secondary == 1)
        {
            scale(&direction, 60);
            bh_effect_split_create(c->effects, s.current, e, &p, &direction, 3, 10, c->split_model, 1);
        }
        else if (secondary == 2)
        {
            bh_sound_request(b->sound, c->area_sound, 1, 1, &p, &travel);
            bh_sound_request(b->sound, c->area_sound, 1, .5f, &p, &travel);
            bh_area_projectile_create(c->area, s.current, &p, &direction, e, .75f);
        }
        return 1;
    }
    primary = short_value(e, 0x1dc);
    if (primary == 0)
    {
        for (i = 0; i < 3; ++i)
            ((float *)&p)[i] =
                rf(((next & 2) ? (double)((float *)&side)[i] : -((double)((float *)&side)[i])) + ((float *)&p)[i]);
        scale(&direction, 150);
        for (i = 0; i < 3; ++i)
            ((float *)&direction)[i] =
                rf((double)((float *)&direction)[i] - ((double)rand() - 16384) * (-1.0 / 8192.0));
        bh_projectile_create_ballistic(c->projectiles, s.current, e, &p, &direction, 2);
        bh_sound_request(b->sound, c->ballistic_sound, .75f, 1.5f, &p, &travel);
    }
    else if (primary == 1)
    {
        if (!(next & 2))
        {
            scale(&direction, 120);
            for (i = 0; i < 3; ++i)
                ((float *)&direction)[i] =
                    rf((double)((float *)&direction)[i] - ((double)rand() - 16384) * (-1.0 / 8192.0));
            bh_projectile_create_homing(c->projectiles, s.current, e, &p, &direction, 2, target);
            bh_sound_request(b->sound, c->homing_sound, .75f, 2, &p, &travel);
        }
    }
    else if (primary == 2)
    {
        gen = e->generation;
        beam(c, e, s.current, &p, &direction, &travel);
        if (!live(c, e, gen))
            c->error = 3;
    }
    return !c->error;
}
int __cdecl bh_actor_vehicle_tick_bind(BHActorAttackContext *c, BHEntity *e)
{
    if (!c || !c->effects || !c->effects->effects || !e || !live(c, e, e->generation))
        return 0;
    memcpy(e->payload + 596, &c, 4);
    e->step = bh_actor_vehicle_tick;
    return 1;
}
void __cdecl bh_actor_vehicle_tick(BHEntity *e)
{
    BHActorAttackContext *c;
    jmp_buf frame;
    void *previous_abort;
    memcpy(&c, e->payload + 596, 4);
    if (!c)
        return;
    previous_abort = c->step_abort;
    if (!setjmp(frame))
    {
        c->step_abort = frame;
        bh_actor_vehicle_step(e);
    }
    c->step_abort = previous_abort;
}
