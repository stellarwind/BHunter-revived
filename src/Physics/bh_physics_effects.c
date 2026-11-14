#include "bh_physics_effects.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>

static const BHVec3 zero = {0, 0, 0};
static unsigned char quantize(double value)
{
    return (unsigned char)(__int64)value;
}
static void request(BHPhysicsEffects *physics_effects, unsigned long resource, float volume, float pitch, const BHVec3 *point)
{
    ++physics_effects->sound_requests;
    physics_effects->last_voice = bh_sound_request(physics_effects->sound, resource, volume, pitch, point, &zero);
    if (physics_effects->last_voice == BH_SOUND_INVALID)
        physics_effects->error = 1;
}
static void packet(BHPhysicsEffects *physics_effects, const BHWorldGeometry *world, unsigned short index, const BHVec3 *point,
                   float level, float pitch)
{
    BHPacketWriter writer;
    unsigned char bytes[4];
    unsigned long size;
    if (physics_effects->peer_count <= 1)
        return;
    if (!bh_packet_begin(&writer, physics_effects->last_packet, sizeof(physics_effects->last_packet), 6, physics_effects->sender))
    {
        physics_effects->error = 1;
        return;
    }
    if (bh_packet_write_position(&writer, world, index, point) != 1)
        return;
    if (level > .5)
    {
        bytes[0] = (unsigned char)physics_effects->impact_sound;
        bytes[1] = quantize((double)level * 128);
        bytes[2] = quantize((double)pitch * 64);
    }
    else
    {
        bytes[0] = 255;
        bytes[1] = 0;
        bytes[2] = 64;
    }
    bytes[3] = quantize((double)level * 3);
    if (!bh_packet_append(&writer, bytes, 4) || !bh_packet_finish(&writer, &size))
    {
        physics_effects->error = 1;
        return;
    }
    physics_effects->last_packet_size = size;
    ++physics_effects->packets;
    physics_effects->last_send = bh_network_send(physics_effects->transport, 0, physics_effects->last_packet, size, 0);
    if (physics_effects->last_send < 0)
        physics_effects->error = 1;
}
int bh_physics_effects_run(BHPhysicsEffects *physics_effects, const BHSweepHit *hit, float level, const BHWorldGeometry *world,
                           unsigned short index)
{
    BHEntity *effect;
    const BHVec3 *point;
    unsigned long flags;
    long count, i;
    int random_value;
    float volume, pitch = 1, lifetime;
    double lifetime_adjust;
    if (!physics_effects || !hit || !hit->surface_present || !physics_effects->effects || !physics_effects->sound || !bh_math_environment_supported())
        return 0;
    flags = hit->surface.flags;
    point = &hit->surface.contact.point;
    if (!(flags & 6) && (!_finite(level) || level < 0 || level > 1))
        return 0;
    physics_effects->error = 0;
    physics_effects->last_packet_size = 0;
    if (flags & 6)
    {
        effect = bh_contact_impact_create(physics_effects->effects, point, &hit->surface.normal, flags);
        if (effect)
        {
            if (flags & 2)
                request(physics_effects, physics_effects->quad_sound, 1, 1, point);
            else
            {
                bh_round_float(&volume, 12.0 * (1.0 / 6.0));
                request(physics_effects, physics_effects->rays_sound, volume, 1, point);
            }
        }
        else
            physics_effects->error = 1;
        return !physics_effects->error;
    }
    if (level > .5)
    {
        bh_round_float(&pitch, .9 - (double)level * (-.2));
        bh_round_float(&volume, ((double)level - .25) * .7);
        request(physics_effects, physics_effects->impact_sound, volume, pitch, point);
    }
    packet(physics_effects, world, index, point, level, pitch);
    count = (long)(__int64)((double)level * 3);
    lifetime_adjust = (double)level * (-.0625);
    for (i = 0; i < count; ++i)
    {
        random_value = rand();
        bh_round_float(&lifetime, ((double)random_value * (1.0 / 131072.0) - lifetime_adjust) - (-.5));
        ++physics_effects->spark_attempts;
        if (bh_contact_spark_create(physics_effects->effects, point, lifetime, 1))
            ++physics_effects->sparks_created;
        else
            physics_effects->error = 1;
    }
    return !physics_effects->error;
}
