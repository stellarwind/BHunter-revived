#include "bh_script_actor_tick.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
static void get(const BHEntity *entity, unsigned long at, void *p)
{
    memcpy(p, (const char *)entity + at, 4);
}
static void put(BHEntity *entity, unsigned long at, const void *p)
{
    memcpy((char *)entity + at, p, 4);
}
static int fail(BHActorRouteContext *ctx, BHActorRouteError e)
{
    if (ctx)
        ctx->error = e;
    return 0;
}
static int tick(BHActorRouteContext *ctx, BHEntity *entity, int phase, unsigned long pulse)
{
    BHActorRouteState state;
    BHScriptActorPath path;
    BHVec3 point, velocity, zero = {0, 0, 0};
    float t;
    unsigned long resource, bits;
    long counter, sound_frames;
    unsigned short elapsed, delta, stamp;
    short signed_elapsed, signed_duration;
    if (!ctx || !entity || !ctx->live_tick)
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    bh_actor_route_read(entity, &state);
    memcpy(&sound_frames, &state.sound_frames, 4);
    if (sound_frames > 0)
    {
        if (!ctx->sound)
            return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
        path = bh_script_actor_route_path(entity);
        if (path != bh_script_actor_curve && path != bh_script_actor_linear)
            return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
        if (phase)
            bh_round_float(&t, (double)state.phase / state.duration);
        else
        {
            elapsed = (unsigned short)(bh_actor_route_now(ctx) - state.tick);
            memcpy(&signed_elapsed, &elapsed, 2);
            memcpy(&signed_duration, &state.duration_ticks, 2);
            bh_round_float(&t, (double)signed_elapsed / signed_duration);
        }
        path(t, &point, state.controls, state.controls + 1, state.controls + 2, state.controls + 3);
        velocity = entity->velocity;
        if (state.sound == BH_SOUND_INVALID)
        {
            get(entity, 0x10c, &resource);
            state.sound = bh_sound_request(ctx->sound, resource, 1, phase ? .7f : 1, &point, &velocity);
            put(entity, 0x104, &state.sound);
        }
        else
            bh_sound_update(ctx->sound, state.sound, 1, phase ? .7f : 1, &point, &velocity);
        if (phase)
        {
            get(entity, 0x118, &counter);
            bits = (unsigned long)counter - 1UL;
            memcpy(&counter, &bits, 4);
            put(entity, 0x118, &counter);
            if (counter < 45)
            {
                if (!(bits & 3))
                    bh_sound_request(ctx->sound, pulse, 1, 1.5f, &point, &zero);
                if (counter < 0)
                {
                    bits = (unsigned long)counter + 120UL;
                    memcpy(&counter, &bits, 4);
                    put(entity, 0x118, &counter);
                }
            }
        }
    }
    delta = (unsigned short)(bh_actor_route_now(ctx) - state.duration_ticks - state.tick);
    if (!(delta & 0x8000))
    {
        if (!bh_script_actor_route_advance(ctx, entity))
            return 0;
        stamp = (unsigned short)(bh_actor_route_now(ctx) + delta);
        memcpy((char *)entity + 0x1e4, &stamp, 2);
    }
    ctx->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
int bh_script_actor_clock_tick(BHActorRouteContext *ctx, BHEntity *entity)
{
    return tick(ctx, entity, 0, 0);
}
int bh_script_actor_phase_tick(BHActorRouteContext *ctx, BHEntity *entity, unsigned long pulse)
{
    return tick(ctx, entity, 1, pulse);
}
