#include "../../src/bh_render_interpolation.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static float get(unsigned long *word)
{
    float f;
    memcpy(&f, word, 4);
    return f;
}
static void put(unsigned long *word, float f)
{
    memcpy(word, &f, 4);
}
static void frame(BHDrawQueue *q, unsigned long owner, unsigned long generation, float x)
{
    bh_interpolation_bind(q);
    bh_interpolation_reset();
    q->count = 10;
    memset(q->commands, 0, q->count * sizeof(BHDrawCommand));
    q->commands[9].kind = BH_DRAW_QUAD;
    q->commands[9].resource = 42;
    put(q->commands[9].words, x);
    put(q->commands[9].words + 15, .75f);
    put(q->commands[9].words + 16, .125f);
    put(q->commands[9].words + 17, .5f);
    bh_interpolation_stamp(9, owner, generation);
}
int main(void)
{
    unsigned rates[] = {30, 60, 75, 120, 144, 165, 240}, r, i;
    BHSimulationClock clock;
    unsigned long ticks;
    BHDrawCommand storage[16], saved;
    BHDrawQueue q = {0}, *sample;
    BHRenderInterpolation *s = calloc(1, sizeof(*s));
    assert(s);
    q.commands = storage;
    q.capacity = 16;
    for (r = 0; r < sizeof(rates) / sizeof(rates[0]); ++r)
    {
        memset(&clock, 0, sizeof(clock));
        ticks = 0;
        for (i = 0; i <= rates[r] * 100; ++i)
            ticks += bh_simulation_advance(&clock, 100000.0 + i / (double)rates[r]);
        assert(ticks == 3001);
        assert(clock.remainder < 1e-7);
    }
    memset(&clock, 0, sizeof(clock));
    assert(bh_simulation_advance(&clock, 0) == 1);
    assert(bh_simulation_advance(&clock, 10) <= 8);
    assert(bh_simulation_advance(&clock, 10) == 0);
    frame(&q, 100, 1, 10);
    bh_interpolation_capture(s, &q, 0);
    frame(&q, 100, 1, 20);
    saved = storage[9];
    bh_interpolation_capture(s, &q, 0);
    sample = bh_interpolation_sample(s, .5f);
    assert(get(sample->commands[9].words) == 15);
    assert(get(sample->commands[9].words + 15) == .75f);
    assert(get(sample->commands[9].words + 16) == .125f);
    assert(get(sample->commands[9].words + 17) == .5f);
    sample->commands[9].words[0] = 0;
    assert(!memcmp(&storage[9], &saved, sizeof(saved)));
    assert(get(bh_interpolation_sample(s, .5f)->commands[9].words) == 15);
    /* Reusing an entity slot must not interpolate from the dead entity. */
    frame(&q, 100, 2, 40);
    bh_interpolation_capture(s, &q, 0);
    assert(get(bh_interpolation_sample(s, .5f)->commands[9].words) == 40);
    frame(&q, 100, 2, 60);
    bh_interpolation_capture(s, &q, 1);
    assert(get(bh_interpolation_sample(s, .5f)->commands[9].words) == 60);
    frame(&q, 100, 2, 3000);
    bh_interpolation_capture(s, &q, 1);
    assert(get(bh_interpolation_sample(s, .5f)->commands[9].words) == 3000);
    /* Camera position occupies resource as well as words; angle wraps at pi. */
    memset(s, 0, sizeof(*s));
    frame(&q, 200, 1, 0);
    storage[9].kind = BH_DRAW_CAMERA;
    put(&storage[9].resource, 10);
    put(storage[9].words + 2, 3.1f);
    bh_interpolation_capture(s, &q, 0);
    frame(&q, 200, 1, 0);
    storage[9].kind = BH_DRAW_CAMERA;
    put(&storage[9].resource, 20);
    put(storage[9].words + 2, -3.1f);
    bh_interpolation_capture(s, &q, 0);
    sample = bh_interpolation_sample(s, .5f);
    assert(get(&sample->commands[9].resource) == 15);
    assert(fabs(get(sample->commands[9].words + 2) - 3.14159265f) < .0001);
    free(s);
    puts("SDL timing/interpolation: cadence, catch-up, payload preservation, immutable snapshots, generations, "
         "cuts, teleports and angle wrapping passed");
    return 0;
}
