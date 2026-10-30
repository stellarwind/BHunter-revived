#include "../../src/bh_flash_flare.h"
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
/* A player refresh contains camera, changing portal visibility, nested NPC
   draws and finally the player model. A flat command ordinal is not identity. */
static unsigned long composed_frame(BHDrawQueue *q, unsigned long scenery, float camera_x, float car_x)
{
    unsigned long i, at;
    bh_interpolation_bind(q);
    bh_interpolation_reset();
    memset(q->commands, 0, q->capacity * sizeof(BHDrawCommand));
    q->count = 10;
    q->commands[9].kind = BH_DRAW_CAMERA;
    put(&q->commands[9].resource, camera_x);
    bh_interpolation_stamp(9, 0x1000, 5);
    for (i = 0; i < scenery; ++i)
    {
        at = q->count++;
        q->commands[at].kind = BH_DRAW_MODEL_RESOURCE;
        q->commands[at].resource = 99;
        put(q->commands[at].words, 100 + i * 100);
        bh_interpolation_stamp(at, 0x2000 + i, 0);
    }
    at = q->count++;
    q->commands[at].kind = BH_DRAW_MODEL_RESOURCE;
    q->commands[at].resource = 42; /* Same model as player, different entity. */
    put(q->commands[at].words, car_x + 100);
    bh_interpolation_stamp(at, 0x3000, 5);
    at = q->count++;
    q->commands[at].kind = BH_DRAW_MODEL_RESOURCE;
    q->commands[at].resource = 42;
    put(q->commands[at].words, car_x);
    bh_interpolation_stamp(at, 0x4000, 5);
    /* Entity dispatch stamps the enclosing refresh after its nested owners. */
    bh_interpolation_stamp(9, 0x4000, 11);
    return at;
}
int main(void)
{
    unsigned rates[] = {30, 60, 75, 120, 144, 165, 240}, r, i;
    BHSimulationClock clock;
    unsigned long ticks, car;
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
    memset(s, 0, sizeof(*s));
    composed_frame(&q, 1, 10, 20);
    bh_interpolation_capture(s, &q, 1);
    car = composed_frame(&q, 3, 20, 30);
    bh_interpolation_capture(s, &q, 1);
    sample = bh_interpolation_sample(s, .5f);
    assert(get(&sample->commands[9].resource) == 15);
    assert(get(sample->commands[car].words) == 25);
    assert(get(sample->commands[car - 1].words) == 125);
    assert(get(sample->commands[car].words) - get(&sample->commands[9].resource) == 10);
    /* Consecutive samples must move camera/car together even when the number
       of visible rooms changes and the NPC shares the player's model. */
    for (i = 0; i <= 4; ++i)
    {
        sample = bh_interpolation_sample(s, i * .25f);
        assert(get(sample->commands[car].words) - get(&sample->commands[9].resource) == 10);
    }
    /* Motion snapshots stay immutable, but repeated rendering must retain the
       flare consumer's normalized strength until the next producer tick. */
    memset(s, 0, sizeof(*s));
    frame(&q, 0, 0, 0);
    storage[9].kind = BH_DRAW_FLARE;
    put(storage[9].words + 4, 50);
    saved = storage[9];
    bh_interpolation_capture(s, &q, 1);
    sample = bh_interpolation_sample(s, 0);
    assert(get(sample->commands[9].words + 4) == 50);
    put(sample->commands[9].words + 4, 50.f / 320);
    bh_interpolation_rendered(s);
    sample = bh_interpolation_sample(s, .5f);
    assert(get(sample->commands[9].words + 4) == 50.f / 320);
    put(sample->commands[9].words + 4, (50.f / 320) / 320);
    bh_interpolation_rendered(s);
    assert(!memcmp(&storage[9], &saved, sizeof(saved)));
    assert(!memcmp(&s->current[9], &saved, sizeof(saved)));
    /* An unexecuted sample cannot consume a newly published flare. */
    sample = bh_interpolation_sample(s, 1);
    put(sample->commands[9].words + 4, 0);
    bh_interpolation_capture(s, &q, 1);
    bh_interpolation_rendered(s);
    sample = bh_interpolation_sample(s, 0);
    assert(get(sample->commands[9].words + 4) == 50);
    bh_interpolation_rendered(s); /* Offscreen cull leaves strength unchanged. */
    assert(get(bh_interpolation_sample(s, .5f)->commands[9].words + 4) == 50);
    free(s);
    puts("SDL timing/interpolation: cadence, catch-up, payload preservation, immutable snapshots, generations, "
         "cuts, teleports, angle wrapping, nested draw identities and persistent flare consumer state passed");
    return 0;
}
