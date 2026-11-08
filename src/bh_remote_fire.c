#include "bh_remote_fire.h"
#include "bh_vm_queries.h"
#include "bh_weapon_path.h"
#include "bh_physics_response.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
static const BHVec3 zero = {0, 0, 0}, acceleration = {0, -.002f, 0};
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static void get(const BHEntity *entity, unsigned long a, void *p, unsigned long n)
{
    memcpy(p, (const unsigned char *)entity + a, n);
}
static void put(BHEntity *entity, unsigned long a, const void *p, unsigned long n)
{
    memcpy((unsigned char *)entity + a, p, n);
}
static void offset(BHVec3 *out, const BHVec3 *p, const BHVec3 *n, double factor)
{
    bh_round_float(&out->x, (double)p->x - (double)n->x * factor);
    bh_round_float(&out->y, (double)p->y - (double)n->y * factor);
    bh_round_float(&out->z, (double)p->z - (double)n->z * factor);
}
static BHEntity *receiver(BHWeaponPulseContext *ctx, const BHWorldSegmentHit *h, int *world)
{
    unsigned long i;
    *world = h->object.kind == BH_WORLD_HIT_GEOMETRY;
    if (!*world)
        return h->object.object.actor;
    {
        const BHHomingWorldResponse *binding = bh_homing_world_response(&ctx->effects, h->object.object.geometry);
        if (binding)
            return binding->receiver;
    }
    ctx->error = 2;
    return NULL;
}
static void impact_particles(BHWeaponPulseContext *ctx, const BHVec3 *p)
{
    float spin, angle;
    int r;
    r = rand();
    bh_round_float(&spin, ((double)r - 16384) * (1.0 / 8192));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096));
    bh_script_particle_create(ctx->effects.particles, ctx->effects.first_image, 3, p, NULL, NULL, .2f, angle, 5, spin,
                              .8f, 1, .5f, 1);
    r = rand();
    bh_round_float(&spin, ((double)r - 16384) * (1.0 / 8192));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096));
    bh_script_particle_create(ctx->effects.particles, ctx->effects.first_image, 3, p, NULL, NULL, .1f, angle, 8, spin,
                              .8f, 1, .4f, 1);
    bh_script_particle_create(ctx->effects.particles, ctx->flare_image, 3, p, NULL, NULL, .1f, 0, 12, 0, .8f, 1, .3f,
                              1);
}
static void flash(BHWeaponPulseContext *ctx, const BHVec3 *p)
{
    BHEntity *entity = bh_entity_acquire(ctx->effects.pool);
    if (entity)
    {
        entity->position = *p;
        bh_flash_flare_bind(ctx->effects.flash, entity, .1f, 30, 1);
    }
}
static int hitscan(BHWeaponPulseContext *ctx, BHEntity *player, const BHVec3 *position, const BHVec3 *up,
                   const BHVec3 *heading, const BHVec3 *base_velocity)
{
    BHHomingProjectileContext *b;
    BHVec3 muzzle, velocity, direction, end, trail, trail_end, a, point, normal;
    BHWorldGeometry *cached;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    BHPhysicsContactPacket packet;
    BHEntity *target, *contact;
    BHImpactState *impact;
    unsigned long i, image;
    float *out, *r, *h, spin, angle, duration, distance = 125;
    double factor, dot;
    int random, world = 0, result;
    if (!ctx || !ctx->debris)
        return 0;
    b = &ctx->effects;
    if (!b->pool || !b->segment || !b->sound || !b->particles || !b->contacts || !b->flash || !b->markers ||
        ctx->debris->pool != b->pool || b->response_count > 65535 || (b->response_count && !b->responses))
        return 0;
    ctx->error = 0;
    for (i = 0; i < 3; ++i)
        ((float *)&muzzle)[i] =
            rf(((double)((float *)position)[i] - ((float *)heading)[i] * (-1.2)) - ((float *)up)[i] * (-.5));
    random = rand();
    bh_sound_request(b->sound, ctx->shot_sound, 1, rf(.9 - (double)random * (-.000006103515625)), &muzzle,
                     base_velocity);
    random = rand();
    bh_flare_queue(b->particles->queue, &muzzle, 2, 30, ctx->flare_image, 0, 0, 0, rf((double)random * (1.0 / 5200)));
    out = &velocity.x;
    h = (float *)heading;
    for (i = 0; i < 3; ++i)
    {
        random = rand();
        out[i] = rf(((double)random - 16384) * (1.0 / 262144));
    }
    for (i = 0; i < 3; ++i)
        out[i] = rf((double)out[i] - h[i] * (-.4));
    random = rand();
    spin = rf(((double)random - 16384) * (1.0 / 16384));
    random = rand();
    angle = rf((double)random * (1.0 / 512));
    bh_script_particle_create(b->particles, ctx->smoke_image, 3, &muzzle, &velocity, &acceleration, .1f, angle, 8, spin,
                              .9f, .95f, .5f, .2f);
    direction = *heading;
    out = &direction.x;
    for (i = 0; i < 3; ++i)
    {
        random = rand();
        out[i] = rf((double)out[i] - ((double)random - 16384) * (-.000001220703125));
    }
    bh_vec3_normalize(&direction);
    out = &trail.x;
    r = &direction.x;
    h = &muzzle.x;
    for (i = 0; i < 3; ++i)
    {
        random = rand();
        out[i] = rf((3 - (double)random * (-1.0 / 8192)) * r[i] + h[i]);
    }
    bh_draw_segment(b->particles->queue, 3, ctx->trail_image, &muzzle, &trail, .3f, .01f, 1, 0, 0);
    offset(&end, &muzzle, &direction, -125);
    get(player, 0x134, &cached, 4);
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = player;
    memset(&hit, 0, sizeof(hit));
    result = bh_world_segment_run(b->segment, &self, &cached, &muzzle, &end, 3, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return 0;
    }
    put(player, 0x134, &cached, 4);
    if (result)
    {
        point = hit.point;
        normal = hit.normal;
        bh_round_float(&distance, (double)hit.fraction * 125);
        target = receiver(ctx, &hit, &world);
        if (ctx->error)
            return 0;
        if (!world || !(hit.material_flags & 4))
        {
            bh_sound_request(b->sound, b->hit_sound, 1, 2, &point, &zero);
            impact_particles(ctx, &point);
            flash(ctx, &point);
            bh_contact_spark_create(b->contacts, &point, .75f, 3);
            bh_contact_spark_create(b->contacts, &point, 1, 2);
            if (world && !(hit.material_flags & 2))
            {
                dot =
                    ((double)direction.x * normal.x + (double)direction.y * normal.y) + (double)direction.z * normal.z;
                if (dot > 0)
                {
                    normal.x = -normal.x;
                    normal.y = -normal.y;
                    normal.z = -normal.z;
                }
                offset(&a, &point, &normal, -.1);
                if (!target)
                {
                    ctx->error = 2;
                    return 0;
                }
                bh_hit_marker_create(b->markers, &a, &normal, .7f, 3, target);
            }
            out = &velocity.x;
            for (i = 0; i < 3; ++i)
            {
                random = rand();
                bh_round_float(out + i, ((double)random - 16384) * (1.0 / 262144));
            }
            random = rand();
            image = (random & 1) ? ctx->smoke_image : b->first_image;
            duration = (random & 1) ? 1.0f : .5f;
            factor = (random & 1) ? .75 : .5;
            random = rand();
            bh_round_float(&spin, ((double)random - 16384) * (1.0 / 16384));
            random = rand();
            bh_round_float(&angle, (double)random * (1.0 / 512));
            bh_script_particle_create(b->particles, image, 3, &point, &velocity, &acceleration, .1f, angle, 8, spin,
                                      .9f, .95f, duration, (float)factor);
            random = rand();
            if (random & 1)
                bh_script_debris_create(ctx->debris, 1, &point, 3, 5, .5f);
            if (!target || !target->query)
            {
                ctx->error = 2;
                return 0;
            }
            if (target && target->query)
            {
                packet.event = 2;
                packet.point = point;
                packet.normal.x = -direction.x;
                packet.normal.y = -direction.y;
                packet.normal.z = -direction.z;
                packet.impulse = .1f;
                packet.severity = 0;
                target->query(target, &packet);
                if (world && target->query && bh_weapon_hits_host_prop(b, target))
                {
                    BHBlockDirectDamagePacket damage = {0, .3f, player};
                    target->query(target, &damage);
                }
            }
        }
        else
        {
            offset(&a, &point, &normal, -.5);
            random = rand();
            bh_round_float(&duration, .7 - (double)random * (-1.52587890625e-6));
            contact = bh_contact_impact_create(b->contacts, &a, &normal, 4);
            if (contact)
            {
                impact = bh_contact_impact_state(b->contacts, contact);
                impact->duration = duration;
                impact->size = 7;
                bh_sound_request(b->sound, b->liquid_sound, (float)(7.0 / 6.0), (float)(1.0 / duration), &a, &zero);
            }
        }
    }
    random = rand();
    factor = ((double)distance * random) * (1.0 / 32768);
    out = &trail.x;
    r = &direction.x;
    h = &muzzle.x;
    for (i = 0; i < 3; ++i)
        bh_round_float(out + i, factor * r[i] + h[i]);
    offset(&trail_end, &trail, &direction, -3);
    bh_draw_segment(b->particles->queue, 3, ctx->trail_image, &trail, &trail_end, .3f, .01f, .5f, 0, 0);
    offset(&trail_end, &trail, &direction, 3);
    bh_draw_segment(b->particles->queue, 3, ctx->trail_image, &trail, &trail_end, .3f, .01f, .5f, 0, 0);
    return ctx->error == 0;
}

static unsigned short half(BHEntity *entity, unsigned long o)
{
    unsigned short n;
    get(entity, o, &n, 2);
    return n;
}
static short shalf(BHEntity *entity, unsigned long o)
{
    unsigned short n = half(entity, o);
    short v;
    memcpy(&v, &n, 2);
    return v;
}
static void sethalf(BHEntity *entity, unsigned long o, unsigned short v)
{
    put(entity, o, &v, 2);
}
static void add(BHVec3 *out, const BHVec3 *a, const BHVec3 *b, double factor)
{
    unsigned long i;
    for (i = 0; i < 3; ++i)
        ((float *)out)[i] = rf((double)((const float *)a)[i] + ((const float *)b)[i] * factor);
}
static void identity(BHMat34 *m)
{
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = 1;
}
static int shot(BHRemoteFire *fire, BHEntity *entity, short tick, short previous, unsigned long type, BHEntity *target)
{
    BHVec3 position, angles, velocity, right, up, forward, muzzle, v, a, b, d, point, target_point, jitter;
    BHMat34 matrix;
    BHWorldGeometry *world;
    BHEntity *flash_entity;
    BHEntityPositionPacket packet;
    BHScriptParticleContext *particles;
    BHHomingProjectileContext *effects;
    unsigned long i;
    short phase;
    float elapsed, strength, spin, angle;
    int n;
    if (!bh_remote_predict_pose(entity, (unsigned short)tick, &position, &angles))
        return 0;
    identity(&matrix);
    bh_mat34_rotate_y(angles.x, &matrix);
    bh_mat34_rotate_x(angles.y, &matrix);
    bh_mat34_rotate_z(angles.z, &matrix);
    right.x = matrix.m[0][0];
    right.y = matrix.m[1][0];
    right.z = matrix.m[2][0];
    up.x = matrix.m[0][1];
    up.y = matrix.m[1][1];
    up.z = matrix.m[2][1];
    forward.x = matrix.m[0][2];
    forward.y = matrix.m[1][2];
    forward.z = matrix.m[2][2];
    get(entity, 0x48, &velocity, 12);
    for (i = 0; i < 3; ++i)
        ((float *)&velocity)[i] = rf((double)((float *)&velocity)[i] * 30);
    get(entity, 0xc4, &world, 4);
    if (type > 8)
        return 1;
    if (!fire->pulse || !fire->pulse->effects.sound)
        return 0;
    effects = &fire->pulse->effects;
    particles = effects->particles;
    switch (type)
    {
    case 0:
        return hitscan(fire->pulse, entity, &position, &up, &forward, &velocity);
    case 1:
    case 2:
        if (!fire->projectiles || fire->projectiles->next_identity != fire->sync_identity)
            return 0;
        if (type == 2)
            bh_sound_request(effects->sound, fire->homing_sound, .75f, 2, &position, &velocity);
        add(&muzzle, &position, &up, .5);
        if (type == 1)
        {
            add(&muzzle, &muzzle, &right, (half(entity, 0x118) & 1) ? 1 : -1);
            sethalf(entity, 0x118, (unsigned short)(half(entity, 0x118) + 1));
        }
        add(&v, &velocity, &forward, type == 1 ? 150 : 120);
        for (i = 0; i < 3; ++i)
        {
            n = rand();
            ((float *)&v)[i] = rf((double)((float *)&v)[i] - ((double)n - 16384) * (-1.0 / 8192));
        }
        if (type == 1)
        {
            bh_sound_request(effects->sound, fire->ballistic_sound, .75f, 1.5f, &muzzle, &velocity);
            elapsed = rf(((long)previous - (long)tick) * (1.0 / 30));
            add(&muzzle, &muzzle, &v, elapsed);
        }
        *fire->sync_identity = 1;
        if (type == 1)
            bh_projectile_create_ballistic(fire->projectiles, world, entity, &muzzle, &v, 2);
        else
            bh_projectile_create_homing(fire->projectiles, world, entity, &muzzle, &v, 2, target);
        return !fire->projectiles->error;
    case 3:
    case 6:
        if (!fire->effects || !fire->effects->debris || &fire->effects->debris->origin_tag != fire->sync_identity)
            return 0;
        add(&muzzle, &position, &up, .5);
        sethalf(entity, 0x118, (unsigned short)(half(entity, 0x118) + 1));
        add(&muzzle, &muzzle, &right, (half(entity, 0x118) & 1) ? 1 : -1);
        if (!*fire->sync_identity)
            *fire->sync_identity = 1;
        if (type == 3)
        {
            if (target == fire->local)
                bh_sound_request(effects->sound, fire->target_sound, 1, 1, 0, 0);
            if (!fire->effects->orbit)
                return 0;
            bh_effect_orbit_create(fire->effects, world, entity, &muzzle, &velocity, &right, &up, &forward, 8, target);
        }
        else
        {
            if (!fire->effects->split)
                return 0;
            add(&v, &velocity, &forward, 60);
            bh_effect_split_create(fire->effects, world, entity, &muzzle, &v, 3, 10, fire->split_model, 1);
        }
        return !fire->effects->error;
    case 4:
        if (!fire->area || fire->area->identity != fire->sync_identity)
            return 0;
        phase = shalf(entity, 0x1d2);
        if (phase == 5)
        {
            bh_sound_request(effects->sound, fire->area_sound, 1, 1, &position, &velocity);
            bh_sound_request(effects->sound, fire->area_sound, 1, .5f, &position, &velocity);
        }
        strength = rf((double)(3 - phase));
        if (strength < 0)
            strength = -strength;
        strength = rf(1 - (double)strength * .25);
        *fire->sync_identity = 1;
        bh_area_projectile_create(fire->area, world, &position, &forward, entity, strength);
        if (phase)
            sethalf(entity, 0x1d2, (unsigned short)(half(entity, 0x1d2) - 1));
        return !fire->area->error;
    case 5:
        if (!particles || !effects->flash || !effects->pool || !fire->arc)
            return 0;
        sethalf(entity, 0x1dc, 0);
        sethalf(entity, 0x1da, (unsigned short)(half(entity, 0x1da) + 1));
        strength = rf((60 - (long)shalf(entity, 0x1d8)) * (1.0 / 30));
        if (strength > 1.75)
            strength = rf((2.0 - strength) * 4);
        else if (strength > 1)
            strength = 1;
        if (!(half(entity, 0x1da) & 3) || strength <= .5)
        {
            if (!target)
            {
                for (i = 0; i < 3; ++i)
                    ((float *)&point)[i] = rf((((double)((float *)&position)[i] - ((float *)&forward)[i] * (-3)) -
                                               ((float *)&velocity)[i] * (-.1)) -
                                              ((float *)&up)[i] * (-.75));
            }
            else
            {
                if (!target->query)
                    return 0;
                add(&a, &position, &forward, 1);
                add(&a, &a, &up, .5);
                add(&b, &a, &forward, 100);
                packet.event = 3;
                ++fire->queries;
                target->query(target, &packet);
                target_point = packet.position;
                add(&d, &target_point, &position, -1);
                bh_vec3_normalize(&d);
                add(&d, &d, &target_point, 1);
                bh_weapon_path_blend(&point, &a, &b, &d, &target_point, strength);
            }
            n = rand();
            spin = rf(((double)n - 16384) * (1.0 / 8192));
            n = rand();
            angle = rf((double)n * (1.0 / 4096));
            bh_script_particle_create(particles, fire->arc_image, 3, &point, 0, 0, .8f, angle, 10, spin, .8f, 1, .75f,
                                      1);
            n = rand();
            spin = rf(((double)n - 16384) * (1.0 / 8192));
            n = rand();
            angle = rf((double)n * (1.0 / 4096));
            bh_script_particle_create(particles, fire->arc_flare, 3, &point, 0, 0, .4f, angle, 16, spin, .8f, 1, .65f,
                                      1);
            n = rand();
            angle = rf((double)n * (1.0 / 4096));
            bh_script_particle_create(particles, fire->arc_image, 3, &point, 0, 0, .4f, angle, 24, 0, .8f, 1, .5f, 1);
            flash_entity = bh_entity_acquire(effects->pool);
            if (flash_entity)
            {
                flash_entity->position = point;
                bh_flash_flare_bind(effects->flash, flash_entity, .1f, 30, 2);
            }
            if (target && strength == 1)
            {
                long packet11[10];
                packet11[0] = 11;
                ++fire->queries;
                target->query(target, packet11);
            }
            bh_sound_request(effects->sound, fire->arc_sound, .4f, 3, &point, &zero);
        }
        if (target)
        {
            if (!target->query)
                return 0;
            add(&a, &position, &forward, 1);
            add(&b, &a, &forward, 100);
            packet.event = 3;
            ++fire->queries;
            target->query(target, &packet);
            target_point = packet.position;
            jitter = target_point;
            for (i = 0; i < 3; ++i)
            {
                n = rand();
                ((float *)&jitter)[i] = rf((double)((float *)&jitter)[i] - ((double)n - 16384) * (-1.0 / 2048));
            }
            return bh_arc_queue(fire->arc, &a, &b, &jitter, &target_point, strength);
        }
        return 1;
    case 7:
    case 8:
        bh_sound_request(effects->sound, fire->special_sound, 1, type == 7 ? .6f : .8f, &position, &velocity);
        if (type == 7)
            bh_sound_request(effects->sound, fire->special_sound, 1, 1.3f, &position, &velocity);
        add(&muzzle, &position, &up, 1);
        for (i = 0; i < 3; ++i)
            ((float *)&v)[i] = rf((double)((float *)&forward)[i] * (type == 7 ? 20 : 15));
        if (type == 7)
            v.y = rf((double)forward.y * 20 - (-2));
        if (!*fire->sync_identity)
            *fire->sync_identity = 1;
        if (type == 7)
        {
            if (!fire->effects || !fire->effects->accelerated || !fire->effects->debris ||
                &fire->effects->debris->origin_tag != fire->sync_identity)
                return 0;
            bh_effect_accelerated_create(fire->effects, world, entity, &muzzle, &velocity, &v);
            return !fire->effects->error;
        }
        if (!fire->pulse_projectile || fire->pulse_projectile->identity != fire->sync_identity)
            return 0;
        bh_pulse_projectile_create(fire->pulse_projectile, world, entity, &muzzle, &velocity, &v);
        return !fire->pulse_projectile->error;
    }
    return 1;
}
int bh_remote_fire(BHRemoteFire *fire, BHEntity *entity, short tick, unsigned long type, BHEntity *target)
{
    unsigned short old;
    int result;
    if (!fire || !entity || !fire->tick || !fire->sync_identity || !bh_math_environment_supported())
    {
        if (fire)
            fire->error = 1;
        return 0;
    }
    old = (unsigned short)*fire->tick;
    fire->error = 0;
    ++fire->calls;
    *fire->tick = (*fire->tick & 0xffff0000UL) | (unsigned short)tick;
    result = shot(fire, entity, tick, (short)old, type, target);
    *fire->tick = (*fire->tick & 0xffff0000UL) | old;
    if (!result)
        fire->error = 2;
    else if (type < 9)
        ++fire->shots;
    return result;
}
