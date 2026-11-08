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
static void get(BHEntity *entity, unsigned long o, void *p, unsigned long n)
{
    memcpy(p, (char *)entity + o, n);
}
static void put(BHEntity *entity, unsigned long o, const void *p, unsigned long n)
{
    memcpy((char *)entity + o, p, n);
}
static long word(BHEntity *entity, unsigned long o)
{
    long v;
    get(entity, o, &v, 4);
    return v;
}
static void setword(BHEntity *entity, unsigned long o, long v)
{
    put(entity, o, &v, 4);
}
static float number(BHEntity *entity, unsigned long o)
{
    float v;
    get(entity, o, &v, 4);
    return v;
}
static void real(BHEntity *entity, unsigned long o, float v)
{
    put(entity, o, &v, 4);
}
static short short_value(BHEntity *entity, unsigned long o)
{
    short v;
    get(entity, o, &v, 2);
    return v;
}
static void setshort_value(BHEntity *entity, unsigned long o, long v)
{
    unsigned short u = (unsigned short)v;
    put(entity, o, &u, 2);
}
static void scale(BHVec3 *v, double s)
{
    v->x = rf(v->x * s);
    v->y = rf(v->y * s);
    v->z = rf(v->z * s);
}
static void curve(BHActorRouteState *state, float t, BHVec3 *out)
{
    bh_camera_curve(t, out, state->controls, state->controls + 1, state->controls + 2, state->controls + 3);
}
static void offset(BHEntity *entity, BHVec3 *p)
{
    unsigned long i;
    for (i = 0; i < 3; ++i)
        ((float *)p)[i] = rf((double)number(entity, 0x15c + i * 4) + ((float *)p)[i]);
}
static int live(BHActorAttackContext *ctx, BHEntity *entity, unsigned long gen)
{
    return bh_entity_is_active(ctx->effects->effects->pool, entity) && entity->generation == gen;
}
typedef struct Motion
{
    long event;
    BHVec3 position, velocity, acceleration;
} Motion;
static int motion(BHActorAttackContext *ctx, BHEntity *entity, BHEntity *target, Motion *p)
{
    unsigned long gen = entity->generation;
    if (!target || !target->query)
    {
        ctx->error = 2;
        return 0;
    }
    p->event = 16;
    target->query(target, p);
    if (!live(ctx, entity, gen))
    {
        ctx->error = 3;
        return 0;
    }
    return 1;
}
static void particle(BHActorAttackContext *ctx, unsigned long image, const BHVec3 *p, const BHVec3 *v, const BHVec3 *a,
                     float size, float rate, float life, float alpha, double spin_scale, double angle_scale,
                     float decay, float angular_decay)
{
    float spin = rf((double)(rand() - 16384) * spin_scale), angle = rf((double)rand() * angle_scale);
    bh_script_particle_create(ctx->effects->effects->particles, image, 3, p, v, a, size, angle, rate, spin, decay,
                              angular_decay, life, alpha);
}
static BHEntity *response(BHActorAttackContext *ctx, BHWorldSegmentHit *h)
{
    unsigned long i;
    BHHomingProjectileContext *b = ctx->effects->effects;
    if (h->object.kind == BH_WORLD_HIT_ACTOR)
        return h->object.object.actor;
    {
        const BHHomingWorldResponse *binding = bh_homing_world_response(b, h->object.object.geometry);
        if (binding)
        {
            return binding->receiver;
        }
    }
    ctx->error = 2;
    return 0;
}
static void beam(BHActorAttackContext *ctx, BHEntity *entity, BHWorldGeometry *world, const BHVec3 *fire,
                 const BHVec3 *direction, const BHVec3 *travel)
{
    BHHomingProjectileContext *b = ctx->effects->effects;
    BHVec3 velocity, end, ray, point, normal, marker_point;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    BHEntity *receiver, *flash, *contact;
    BHImpactState *impact;
    float distance = 125, alpha;
    unsigned long i, flags, gen = entity->generation;
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
    bh_sound_request(b->sound, ctx->ballistic_sound, 1, rf(.9 - (double)rand() * (-.000006103515625)), fire, travel);
    bh_flare_queue(b->flash->queue, fire, 2, 30, ctx->flash_image, 0, 0, 0, rf((double)rand() * (1.0 / 5200.0)));
    for (i = 0; i < 3; ++i)
        ((float *)&velocity)[i] = rf((double)(rand() - 16384) * (1.0 / 262144.0) - ((float *)direction)[i] * (-.4));
    particle(ctx, ctx->smoke_image, fire, &velocity, &gravity, .1f, 8, .5f, .2f, 1.0 / 16384.0, 1.0 / 512.0, .9f, .95f);
    ray = velocity;
    for (i = 0; i < 3; ++i)
        ((float *)&ray)[i] = rf((double)((float *)&ray)[i] - ((double)rand() - 16384) * (-.000001220703125));
    bh_vec3_normalize(&ray);
    for (i = 0; i < 3; ++i)
        ((float *)&end)[i] = rf((3.0 - (double)rand() * (-1.0 / 8192.0)) * ((float *)&ray)[i] + ((float *)fire)[i]);
    bh_draw_segment(b->particles->queue, 3, ctx->beam_image, fire, &end, .3f, .01f, 1, 0, 0);
    for (i = 0; i < 3; ++i)
        ((float *)&end)[i] = rf((double)((float *)fire)[i] - ((float *)&ray)[i] * (-125.0));
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = entity;
    result = bh_world_segment_run(b->segment, &self, &world, fire, &end, 3, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return;
    }
    if (result)
    {
        receiver = response(ctx, &hit);
        if (ctx->error)
            return;
        point = hit.point;
        normal = hit.normal;
        distance = rf((double)hit.fraction * 125);
        flags = hit.object.kind == BH_WORLD_HIT_GEOMETRY ? (unsigned long)hit.material_flags : 0;
        if (!(flags & 4))
        {
            bh_sound_request(b->sound, ctx->impact_sound, 1, 2, &point, &zero);
            particle(ctx, ctx->impact_image, &point, 0, 0, .2f, 5, .5f, 1, 1.0 / 8192.0, 1.0 / 4096.0, .8f, 1);
            particle(ctx, ctx->impact_image, &point, 0, 0, .1f, 8, .4f, 1, 1.0 / 8192.0, 1.0 / 4096.0, .8f, 1);
            bh_script_particle_create(b->particles, ctx->flash_image, 3, &point, 0, 0, .1f, 0, 12, 0, .8f, 1, .3f, 1);
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
                    ctx->error = 2;
                    return;
                }
                bh_hit_marker_create(b->markers, &marker_point, &normal, .7f, 3, receiver);
            }
            for (i = 0; i < 3; ++i)
                ((float *)&velocity)[i] = rf((double)(rand() - 16384) * (1.0 / 262144.0));
            if (!(rand() & 1))
                particle(ctx, ctx->impact_image, &point, &velocity, &gravity, .1f, 8, .5f, .5f, 1.0 / 16384.0,
                         1.0 / 512.0, .9f, .95f);
            else
                particle(ctx, ctx->smoke_image, &point, &velocity, &gravity, .1f, 8, 1, .75f, 1.0 / 16384.0,
                         1.0 / 512.0, .9f, .95f);
            if (rand() & 1)
                bh_script_debris_create(ctx->effects->debris, 1, &point, 3, 5, .5f);
            if (receiver && receiver->query)
            {
                packet.event = 2;
                packet.point = point;
                packet.normal = ray;
                scale(&packet.normal, -1);
                packet.impulse = .1f;
                packet.severity = ctx->legacy_contact_severity;
                receiver->query(receiver, &packet);
                if (!live(ctx, entity, gen))
                {
                    ctx->error = 3;
                    return;
                }
                damage.event = 0;
                damage.amount = .15f;
                damage.source = entity;
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
                bh_sound_request(b->sound, ctx->liquid_sound, rf(7.0 / 6.0), rf(1.0 / alpha), &marker_point, &zero);
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
    bh_draw_segment(b->particles->queue, 3, ctx->beam_image, &point, &end, .3f, .01f, .5f, 0, 0);
    for (i = 0; i < 3; ++i)
        ((float *)&end)[i] = rf((double)((float *)&point)[i] - ((float *)&ray)[i] * 3.0);
    bh_draw_segment(b->particles->queue, 3, ctx->beam_image, &point, &end, .3f, .01f, .5f, 0, 0);
    ++ctx->beams;
}
void bh_actor_attack_fire_vectors(BHEntity *entity, float t, BHVec3 *p, BHVec3 *direction, BHVec3 *side, BHVec3 *travel)
{
    BHActorRouteState state;
    BHVec3 q;
    unsigned long i;
    bh_actor_route_read(entity, &state);
    curve(&state, t, p);
    curve(&state, rf((double)t - (-1.0 / 30.0)), &q);
    offset(entity, p);
    for (i = 0; i < 3; ++i)
    {
        /* 00437fc6..0043802d retain each following position through its
           FST, subtract the rounded fire point, then retain the difference
           through the direction FST for the travel-velocity product. */
        double delta = ((double)number(entity, 0x15c + 4 * i) + ((float *)&q)[i]) - ((float *)p)[i];
        ((float *)direction)[i] = rf(delta);
        ((float *)travel)[i] = rf(delta * 30);
    }
    bh_vec3_normalize(direction);
    bh_vec3_cross(direction, &up, side);
}
int bh_actor_attack(BHActorAttackContext *ctx, BHEntity *entity)
{
    BHEntity *target;
    BHActorRouteState state;
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
    if (!ctx || !entity || !ctx->pursuit || !ctx->effects || !ctx->effects->effects || !ctx->projectiles ||
        !ctx->area || !bh_math_environment_supported())
        return 0;
    b = ctx->effects->effects;
    ctx->error = 0;
    target = (BHEntity *)word(entity, 0x1b8);
    bh_actor_route_read(entity, &state);
    if (!target)
        return 1;
    if (state.duration == 0)
    {
        ctx->error = 2;
        return 0;
    }
    t = rf((double)state.phase / state.duration);
    pursuit = number(entity, 0x1b4);
    forced = word(entity, 0x1d4) != 0;
    if (forced)
    {
        if (!motion(ctx, entity, target, &m))
            return 0;
        have_motion = 1;
        for (i = 0; i < 3; ++i)
        {
            double v = (double)((float *)&m.velocity)[i] - ((float *)&m.acceleration)[i] * (-.5);
            ((float *)&predicted)[i] = rf((double)((float *)&m.position)[i] - v * (-.5));
        }
        bh_world_clamp_point(state.current, &predicted);
        for (i = 0; i < 3; ++i)
        {
            double delta = (double)((float *)&predicted)[i] - ((float *)&state.controls[3])[i];
            double adjustment;
            /* 00437c3e spills Z difference; X/Y remain in x87. X/Y
               endpoint sums retain adjustment, Z reloads its float copy. */
            if (i == 2)
                delta = rf(delta);
            adjustment = (double)pursuit * delta * .03;
            ((float *)&change)[i] = rf(adjustment);
            ((float *)&state.controls[3])[i] =
                rf((i == 2 ? (double)((float *)&change)[i] : adjustment) + ((float *)&state.controls[3])[i]);
            ((float *)&state.controls[2])[i] = rf((double)((float *)&change)[i] + ((float *)&state.controls[2])[i]);
        }
        bh_world_clamp_point(state.current, state.controls + 3);
        bh_world_clamp_point(state.current, state.controls + 2);
        bh_actor_route_write(entity, &state);
    }
    frame = short_value(entity, 0x1e0);
    if (frame < 0)
    {
        setshort_value(entity, 0x1e0, frame + 1);
        return 1;
    }
    if (!frame)
    {
        short protected_player = target == ctx->pursuit->player ? short_value(target, 0x2b4) : 0;
        if (protected_player)
            return 1;
        if (!have_motion && !motion(ctx, entity, target, &m))
            return 0;
        curve(&state, t, &p);
        offset(entity, &p);
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
            curve(&state, rf((double)t - (-1.0 / 30.0)), &q);
            for (i = 0; i < 3; ++i)
                ((float *)&direction)[i] = rf((double)((float *)&q)[i] - ((float *)&p)[i]);
            bh_vec3_normalize(&direction);
            inv = 1.0 / sqrt(distance);
            dot = (attack_product(inv, difference.x) * direction.x + attack_product(inv, difference.y) * direction.y) +
                  attack_product(inv, difference.z) * direction.z;
            /* Original TEST AH,41 accepts unordered too: coincident
               target/actor positions must retain that firing decision. */
            if (_isnan(dot) || .98 - (double)pursuit * .03 <= dot)
                setshort_value(entity, 0x1e0, 1);
        }
        return 1;
    }
    next = (unsigned short)(frame + 1);
    setshort_value(entity, 0x1e0, next);
    total = word(entity, 0x1d8);
    if ((short)next == total)
    {
        cooldown = short_value(entity, 0x1e2);
        setshort_value(entity, 0x1e0, -(long)cooldown);
        return 1;
    }
    if (next & 1)
        return 1;
    bh_actor_attack_fire_vectors(entity, t, &p, &direction, &side, &travel);
    /* Original emitters reload these live fields after the synchronous motion
       query (004380e8/00438176 and corresponding primary-weapon branches). */
    get(entity, 0x1a4, &state.current, 4);
    target = (BHEntity *)word(entity, 0x1b8);
    ++ctx->attacks;
    if (next == 2 && rand() < 0x1000)
    {
        secondary = short_value(entity, 0x1de);
        setshort_value(entity, 0x1e0, total - 1);
        ++ctx->specials;
        if (secondary == 0)
        {
            if (target == ctx->pursuit->player)
                bh_sound_request(b->sound, ctx->orbit_cue, 1, 1, 0, 0);
            bh_effect_orbit_create(ctx->effects, state.current, entity, &p, &travel, &side, &up, &direction, 8, target);
        }
        else if (secondary == 1)
        {
            scale(&direction, 60);
            bh_effect_split_create(ctx->effects, state.current, entity, &p, &direction, 3, 10, ctx->split_model, 1);
        }
        else if (secondary == 2)
        {
            bh_sound_request(b->sound, ctx->area_sound, 1, 1, &p, &travel);
            bh_sound_request(b->sound, ctx->area_sound, 1, .5f, &p, &travel);
            bh_area_projectile_create(ctx->area, state.current, &p, &direction, entity, .75f);
        }
        return 1;
    }
    primary = short_value(entity, 0x1dc);
    if (primary == 0)
    {
        for (i = 0; i < 3; ++i)
            ((float *)&p)[i] =
                rf(((next & 2) ? (double)((float *)&side)[i] : -((double)((float *)&side)[i])) + ((float *)&p)[i]);
        scale(&direction, 150);
        for (i = 0; i < 3; ++i)
            ((float *)&direction)[i] =
                rf((double)((float *)&direction)[i] - ((double)rand() - 16384) * (-1.0 / 8192.0));
        bh_projectile_create_ballistic(ctx->projectiles, state.current, entity, &p, &direction, 2);
        bh_sound_request(b->sound, ctx->ballistic_sound, .75f, 1.5f, &p, &travel);
    }
    else if (primary == 1)
    {
        if (!(next & 2))
        {
            scale(&direction, 120);
            for (i = 0; i < 3; ++i)
                ((float *)&direction)[i] =
                    rf((double)((float *)&direction)[i] - ((double)rand() - 16384) * (-1.0 / 8192.0));
            bh_projectile_create_homing(ctx->projectiles, state.current, entity, &p, &direction, 2, target);
            bh_sound_request(b->sound, ctx->homing_sound, .75f, 2, &p, &travel);
        }
    }
    else if (primary == 2)
    {
        gen = entity->generation;
        beam(ctx, entity, state.current, &p, &direction, &travel);
        if (!live(ctx, entity, gen))
            ctx->error = 3;
    }
    return !ctx->error;
}
int bh_actor_vehicle_tick_bind(BHActorAttackContext *ctx, BHEntity *entity)
{
    if (!ctx || !ctx->effects || !ctx->effects->effects || !entity || !live(ctx, entity, entity->generation))
        return 0;
    memcpy(entity->payload + 596, &ctx, 4);
    entity->step = bh_actor_vehicle_tick;
    return 1;
}
void bh_actor_vehicle_tick(BHEntity *entity)
{
    BHActorAttackContext *ctx;
    jmp_buf frame;
    void *previous_abort;
    memcpy(&ctx, entity->payload + 596, 4);
    if (!ctx)
        return;
    previous_abort = ctx->step_abort;
    if (!setjmp(frame))
    {
        ctx->step_abort = frame;
        bh_actor_vehicle_step(entity);
    }
    ctx->step_abort = previous_abort;
}
