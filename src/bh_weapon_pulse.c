#include "bh_weapon_pulse.h"
#include "bh_physics_response.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0}, acceleration = {0, -.002f, 0};
static void get(const BHEntity *e, unsigned long a, void *p, unsigned long n)
{
    memcpy(p, (const unsigned char *)e + a, n);
}
static void put(BHEntity *e, unsigned long a, const void *p, unsigned long n)
{
    memcpy((unsigned char *)e + a, p, n);
}
static void offset(BHVec3 *out, const BHVec3 *p, const BHVec3 *n, double factor)
{
    bh_round_float(&out->x, (double)p->x - (double)n->x * factor);
    bh_round_float(&out->y, (double)p->y - (double)n->y * factor);
    bh_round_float(&out->z, (double)p->z - (double)n->z * factor);
}
static BHEntity *receiver(BHWeaponPulseContext *c, const BHWorldSegmentHit *h, int *world)
{
    unsigned long i;
    *world = h->object.kind == BH_WORLD_HIT_GEOMETRY;
    if (!*world)
        return h->object.object.actor;
    for (i = 0; i < c->effects.response_count; ++i)
        if (c->effects.responses[i].world == h->object.object.geometry)
            return c->effects.responses[i].receiver;
    c->error = 2;
    return NULL;
}
static void impact_particles(BHWeaponPulseContext *c, const BHVec3 *p)
{
    float spin, angle;
    int r;
    r = rand();
    bh_round_float(&spin, ((double)r - 16384) * (1.0 / 8192));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096));
    bh_script_particle_create(c->effects.particles, c->effects.first_image, 3, p, NULL, NULL, .2f, angle, 5, spin, .8f,
                              1, .5f, 1);
    r = rand();
    bh_round_float(&spin, ((double)r - 16384) * (1.0 / 8192));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096));
    bh_script_particle_create(c->effects.particles, c->effects.first_image, 3, p, NULL, NULL, .1f, angle, 8, spin, .8f,
                              1, .4f, 1);
    bh_script_particle_create(c->effects.particles, c->flare_image, 3, p, NULL, NULL, .1f, 0, 12, 0, .8f, 1, .3f, 1);
}
static void flash(BHWeaponPulseContext *c, const BHVec3 *p)
{
    BHEntity *e = bh_entity_acquire(c->effects.pool);
    if (e)
    {
        e->position = *p;
        bh_flash_flare_bind(c->effects.flash, e, .1f, 30, 1);
    }
}
int __cdecl bh_weapon_pulse_fire(BHWeaponPulseContext *c, BHEntity *weapon, BHEntity *player)
{
    BHHomingProjectileContext *b;
    BHWeaponCommonState *s;
    BHVec3 muzzle, axis, up, heading, velocity, direction, a, end, point, normal, trail, trail_end;
    BHWorldGeometry *cached;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    BHPhysicsContactPacket packet;
    struct
    {
        long event;
        float amount;
        BHEntity *source;
    } damage;
    BHEntity *target, *contact;
    BHImpactState *impact;
    unsigned long phase, kind, i, image, generation;
    float *out, *h, *r, spin, angle, duration, distance = 125;
    double recoil, factor, dot;
    int random, world = 0, result;
    unsigned char payload[5];
    BHPacketWriter writer;
    if (!c || !weapon || !player || !player->query || !c->debris || !c->transport || !bh_math_environment_supported())
        return 0;
    b = &c->effects;
    if (!b->pool || !b->segment || !b->sound || !b->particles || !b->contacts || !b->flash || !b->markers)
        return 0;
    if (b->response_count > 65535 || (b->response_count && !b->responses) || c->debris->pool != b->pool)
        return 0;
    s = bh_weapon_common_state(weapon);
    phase = s->word_e8;
    c->error = 0;
    if (phase & 1)
    {
        s->word_e8 = phase + 1;
        return 1;
    }
    if (!s->ammo)
        return 1;
    s->ammo = (long)((unsigned long)s->ammo - 1);
    generation = player->generation;
    get(player, 0x30, &axis, 12);
    get(player, 0x24, &up, 12);
    get(player, 0x280, &heading, 12);
    out = &muzzle.x;
    r = &player->position.x;
    for (i = 0; i < 3; ++i)
        bh_round_float(out + i, ((double)r[i] - ((float *)&axis)[i] * (-1.2)) - ((float *)&up)[i] * (-.5));
    random = rand();
    bh_sound_request(b->sound, c->shot_sound, 1, (float)(.9 - (double)random * (-.000006103515625)), &muzzle,
                     &player->origin);
    get(player, 0xe8, &kind, 4);
    if (kind != 0 && kind != 1)
    {
        random = rand();
        bh_flare_queue(b->particles->queue, &muzzle, 2, 30, c->flare_image, 0, 0, 0,
                       (float)((double)random * (1.0 / 5200.0)));
    }
    out = &velocity.x;
    h = &heading.x;
    for (i = 0; i < 3; ++i)
    {
        random = rand();
        bh_round_float(out + i, ((double)random - 16384) * (1.0 / 262144));
    }
    for (i = 0; i < 3; ++i)
        bh_round_float(out + i, (double)out[i] - (double)h[i] * (-.4));
    random = rand();
    bh_round_float(&spin, ((double)random - 16384) * (1.0 / 16384));
    random = rand();
    bh_round_float(&angle, (double)random * (1.0 / 512));
    bh_script_particle_create(b->particles, c->smoke_image, 3, &muzzle, &velocity, &acceleration, .1f, angle, 8, spin,
                              .9f, .95f, .5f, .2f);
    random = rand();
    recoil = 3 - (double)random * (-.0002);
    if (phase & 2)
        recoil *= .125;
    get(player, 0x2a4, &a, 12);
    out = &a.x;
    for (i = 0; i < 3; ++i)
        bh_round_float(out + i, (double)h[i] * recoil + out[i]);
    put(player, 0x2a4, &a, 12);
    packet.event = 2;
    packet.point = muzzle;
    out = &packet.normal.x;
    for (i = 0; i < 3; ++i)
    {
        random = rand();
        bh_round_float(out + i, ((double)random - 16384) * (1.0 / 65536));
    }
    for (i = 0; i < 3; ++i)
        bh_round_float(out + i, (double)h[i] + out[i]);
    packet.impulse = .05f;
    packet.severity = 0;
    player->query(player, &packet);
    if (player->generation != generation)
    {
        c->error = 3;
        return 0;
    }
    direction = heading;
    out = &direction.x;
    for (i = 0; i < 3; ++i)
    {
        random = rand();
        bh_round_float(out + i, (double)out[i] - ((double)random - 16384) * (-.000001220703125));
    }
    bh_vec3_normalize(&direction);
    out = &trail.x;
    r = &direction.x;
    h = &muzzle.x;
    for (i = 0; i < 3; ++i)
    {
        random = rand();
        bh_round_float(out + i, (3 - (double)random * (-1.0 / 8192)) * r[i] + h[i]);
    }
    bh_draw_segment(b->particles->queue, 3, c->trail_image, &muzzle, &trail, .3f, .01f, 1, 0, 0);
    offset(&end, &muzzle, &direction, -125);
    get(weapon, 0x104, &cached, 4);
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = player;
    memset(&hit, 0, sizeof(hit));
    result = bh_world_segment_run(b->segment, &self, &cached, &muzzle, &end, 3, &hit);
    if (result < 0)
    {
        c->error = 2;
        return 0;
    }
    put(weapon, 0x104, &cached, 4);
    if (result)
    {
        point = hit.point;
        normal = hit.normal;
        bh_round_float(&distance, (double)hit.fraction * 125);
        target = receiver(c, &hit, &world);
        if (c->error)
            return 0;
        if (!world || !(hit.material_flags & 4))
        {
            bh_sound_request(b->sound, b->hit_sound, 1, 2, &point, &zero);
            impact_particles(c, &point);
            flash(c, &point);
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
                    c->error = 2;
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
            image = (random & 1) ? c->smoke_image : b->first_image;
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
                bh_script_debris_create(c->debris, 1, &point, 3, 5, .5f);
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
                damage.event = 0;
                damage.amount = world ? .3f : .15f;
                damage.source = player;
                if (target->query)
                    target->query(target, &damage);
                ++b->damage_packets;
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
    bh_draw_segment(b->particles->queue, 3, c->trail_image, &trail, &trail_end, .3f, .01f, .5f, 0, 0);
    offset(&trail_end, &trail, &direction, 3);
    bh_draw_segment(b->particles->queue, 3, c->trail_image, &trail, &trail_end, .3f, .01f, .5f, 0, 0);
    if (c->peers > 1 && !s->delay)
    {
        if (c->local_id >= 0)
        {
            payload[0] = (unsigned char)c->local_id;
            payload[1] = 4;
            payload[2] = 2;
            payload[3] = 0;
            payload[4] = 255;
            if (bh_packet_begin(&writer, c->packet, sizeof(c->packet), 8, c->sender) &&
                bh_packet_append(&writer, payload, 5) && bh_packet_finish(&writer, &c->packet_bytes))
            {
                c->last_send = bh_network_send(c->transport, 0, c->packet, c->packet_bytes, 0);
                ++c->packets;
            }
            else
                c->error = 2;
        }
        s->delay = 8;
    }
    s->word_e8 = phase + 1;
    ++c->shots;
    return c->error == 0;
}
