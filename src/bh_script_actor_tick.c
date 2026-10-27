#include "bh_script_actor_tick.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
static void get(const BHEntity *a, unsigned long at, void *p)
{
    memcpy(p, (const char *)a + at, 4);
}
static void put(BHEntity *a, unsigned long at, const void *p)
{
    memcpy((char *)a + at, p, 4);
}
static int fail(BHActorRouteContext *c, BHActorRouteError e)
{
    if (c)
        c->error = e;
    return 0;
}
static int tick(BHActorRouteContext *c, BHEntity *a, int phase, unsigned long pulse)
{
    BHActorRouteState s;
    BHScriptActorPath path;
    BHVec3 point, velocity, zero = {0, 0, 0};
    float t;
    unsigned long resource, bits;
    long counter, sound_frames;
    unsigned short elapsed, delta, stamp;
    short signed_elapsed, signed_duration;
    if (!c || !a)
        return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(c, BH_ACTOR_ROUTE_NUMERICAL);
    bh_actor_route_read(a, &s);
    memcpy(&sound_frames, &s.sound_frames, 4);
    if (sound_frames > 0)
    {
        if (!c->sound)
            return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
        path = bh_script_actor_route_path(a);
        if (path != bh_script_actor_curve && path != bh_script_actor_linear)
            return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
        if (phase)
            bh_round_float(&t, (double)s.phase / s.duration);
        else
        {
            elapsed = (unsigned short)(bh_actor_route_now(c) - s.tick);
            memcpy(&signed_elapsed, &elapsed, 2);
            memcpy(&signed_duration, &s.duration_ticks, 2);
            bh_round_float(&t, (double)signed_elapsed / signed_duration);
        }
        path(t, &point, s.controls, s.controls + 1, s.controls + 2, s.controls + 3);
        memcpy(&velocity, (char *)a + 0x48, 12);
        if (s.sound == BH_SOUND_INVALID)
        {
            get(a, 0x10c, &resource);
            s.sound = bh_sound_request(c->sound, resource, 1, phase ? .7f : 1, &point, &velocity);
            put(a, 0x104, &s.sound);
        }
        else
            bh_sound_update(c->sound, s.sound, 1, phase ? .7f : 1, &point, &velocity);
        if (phase)
        {
            get(a, 0x118, &counter);
            bits = (unsigned long)counter - 1UL;
            memcpy(&counter, &bits, 4);
            put(a, 0x118, &counter);
            if (counter < 45)
            {
                if (!(bits & 3))
                    bh_sound_request(c->sound, pulse, 1, 1.5f, &point, &zero);
                if (counter < 0)
                {
                    bits = (unsigned long)counter + 120UL;
                    memcpy(&counter, &bits, 4);
                    put(a, 0x118, &counter);
                }
            }
        }
    }
    delta = (unsigned short)(bh_actor_route_now(c) - s.duration_ticks - s.tick);
    if (!(delta & 0x8000))
    {
        if (!bh_script_actor_route_advance(c, a))
            return 0;
        stamp = (unsigned short)(bh_actor_route_now(c) + delta);
        memcpy((char *)a + 0x1e4, &stamp, 2);
    }
    c->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
int __cdecl bh_script_actor_clock_tick(BHActorRouteContext *c, BHEntity *a)
{
    return tick(c, a, 0, 0);
}
int __cdecl bh_script_actor_phase_tick(BHActorRouteContext *c, BHEntity *a, unsigned long pulse)
{
    return tick(c, a, 1, pulse);
}
