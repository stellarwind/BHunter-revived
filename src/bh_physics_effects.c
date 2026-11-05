#include "bh_physics_effects.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>

static const BHVec3 zero = {0, 0, 0};
static void request(BHPhysicsEffects *c, unsigned long resource, float volume, float pitch, const BHVec3 *point)
{
    ++c->sound_requests;
    c->last_voice = bh_sound_request(c->sound, resource, volume, pitch, point, &zero);
    if (c->last_voice == BH_SOUND_INVALID)
        c->error = 1;
}
int __cdecl bh_physics_effects_run(BHPhysicsEffects *c, const BHSweepHit *hit, float level)
{
    BHEntity *effect;
    const BHVec3 *point;
    unsigned long flags;
    long count, i;
    int random_value;
    float volume, pitch = 1, lifetime;
    double lifetime_adjust;
    if (!c || !hit || !hit->surface_present || !c->effects || !c->sound || !bh_math_environment_supported())
        return 0;
    flags = hit->surface.flags;
    point = &hit->surface.contact.point;
    if (!(flags & 6) && (!_finite(level) || level < 0 || level > 1))
        return 0;
    c->error = 0;
    if (flags & 6)
    {
        effect = bh_contact_impact_create(c->effects, point, &hit->surface.normal, flags);
        if (effect)
        {
            if (flags & 2)
                request(c, c->quad_sound, 1, 1, point);
            else
            {
                bh_round_float(&volume, 12.0 * (1.0 / 6.0));
                request(c, c->rays_sound, volume, 1, point);
            }
        }
        else
            c->error = 1;
        return !c->error;
    }
    if (level > .5)
    {
        bh_round_float(&pitch, .9 - (double)level * (-.2));
        bh_round_float(&volume, ((double)level - .25) * .7);
        request(c, c->impact_sound, volume, pitch, point);
    }
    count = (long)(__int64)((double)level * 3);
    lifetime_adjust = (double)level * (-.0625);
    for (i = 0; i < count; ++i)
    {
        random_value = rand();
        bh_round_float(&lifetime, ((double)random_value * (1.0 / 131072.0) - lifetime_adjust) - (-.5));
        ++c->spark_attempts;
        if (bh_contact_spark_create(c->effects, point, lifetime, 1))
            ++c->sparks_created;
        else
            c->error = 1;
    }
    return !c->error;
}
