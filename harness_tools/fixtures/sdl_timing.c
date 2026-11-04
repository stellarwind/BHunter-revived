#include "../../src/bh_flash_flare.h"
#include "../../src/bh_pickup_draw.h"
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
    /* A cockpit is rigidly attached even when new radar/gauge sprites appear.
       Linear world-vertex interpolation would shrink/shear it on this turn;
       ordinal matching would additionally blend the newly inserted sprite. */
    memset(s, 0, sizeof(*s));
    for (r = 0; r < 2; ++r)
    {
        float yaw = r * 1.5707963267948966f;
        frame(&q, 800, 1, r * 10.0f);
        storage[9].kind = BH_DRAW_MODEL_RESOURCE;
        put(storage[9].words + 3, yaw);
        q.count = r ? 13 : 12;
        memset(storage + 10, 0, (q.count - 10) * sizeof(*storage));
        for (i = 10; i < q.count - 1; ++i)
        {
            float radius = (float)(i - 9);
            storage[i].kind = BH_DRAW_QUAD;
            /* Presentation anchor flags must survive rigid rebasing, including
               newly appearing sprites, without entering spatial interpolation. */
            storage[i].words[13] = 0x10000000UL;
            put(storage[i].words, r * 10.0f + radius * cos(yaw));
            put(storage[i].words + 2, -radius * sin(yaw));
            put(storage[i].words + 4, 1);
            put(storage[i].words + 6, sin(yaw));
            put(storage[i].words + 8, cos(yaw));
        }
        storage[q.count - 1].kind = BH_DRAW_RECT;
        put(storage[q.count - 1].words, 500);
        bh_interpolation_stamp(10, 900, 1);
        bh_interpolation_attach(10, q.count, 9);
        bh_interpolation_capture(s, &q, 1);
    }
    for (r = 0; r <= 4; ++r)
    {
        float a = r * .25f, yaw = a * 1.5707963267948966f;
        sample = bh_interpolation_sample(s, a);
        for (i = 10; i < 12; ++i)
        {
            float radius = (float)(i - 9);
            assert(fabs(get(sample->commands[i].words) - (a * 10 + radius * cos(yaw))) < .00001);
            assert(fabs(get(sample->commands[i].words + 2) + radius * sin(yaw)) < .00001);
            assert(fabs(get(sample->commands[i].words + 6) - sin(yaw)) < .00001);
            assert(fabs(get(sample->commands[i].words + 8) - cos(yaw)) < .00001);
            assert(sample->commands[i].words[13] == 0x10000000UL);
        }
        assert(get(sample->commands[12].words) == 500);
    }
    /* Target screen coordinates are not lerped. Its world anchor is, using
       entity identity, with atlas UVs/pixel offsets left intact. */
    memset(s, 0, sizeof(*s));
    for (r = 0; r < 2; ++r)
    {
        frame(&q, 700, 3, 50);
        storage[9].kind = BH_DRAW_RECT;
        storage[9].words[12] = BH_RECT_WORLD_MARKER;
        put(storage[9].words + 13, r * 20);
        put(storage[9].words + 15, 100 - r * 40);
        put(storage[9].words + 16, -2);
        bh_interpolation_capture(s, &q, 1);
    }
    sample = bh_interpolation_sample(s, .5f);
    assert(get(sample->commands[9].words + 13) == 10);
    assert(get(sample->commands[9].words + 15) == 80);
    assert(get(sample->commands[9].words) == 50);
    assert(get(sample->commands[9].words + 16) == -2);
    assert(get(storage[9].words + 13) == 20);
    /* Moving flare anchors must not reset consumed strength on the next
       render fraction. Texture changes and entity reuse must snap. */
    memset(s, 0, sizeof(*s));
    for (r = 0; r < 2; ++r)
    {
        frame(&q, 44, 1, r * 20);
        storage[9].kind = BH_DRAW_FLARE;
        storage[9].resource = 0;
        storage[9].words[6] = 123;
        put(storage[9].words + 4, 50);
        bh_interpolation_capture(s, &q, 1);
    }
    sample = bh_interpolation_sample(s, .25f);
    assert(get(sample->commands[9].words) == 5);
    put(sample->commands[9].words + 4, 50.f / 320);
    bh_interpolation_rendered(s);
    sample = bh_interpolation_sample(s, .75f);
    assert(get(sample->commands[9].words) == 15);
    assert(get(sample->commands[9].words + 4) == 50.f / 320);
    assert(get(s->current[9].words + 4) == 50);
    storage[9].words[6] = 456;
    bh_interpolation_capture(s, &q, 1);
    assert(get(bh_interpolation_sample(s, 0)->commands[9].words) == 20);
    assert(get(bh_interpolation_sample(s, 0)->commands[9].words + 4) == 50);
    /* Head insertion and tail removal change list ordinals. Existing physical
       nodes keep identity; the new head grows from its older node's old pose. */
    memset(s, 0, sizeof(*s));
    for (r = 0; r < 2; ++r)
    {
        unsigned long node;
        frame(&q, 0, 0, 0);
        memset(storage + 9, 0, 4 * sizeof(*storage));
        q.count = 13;
        storage[9].kind = BH_DRAW_PATH_HEADER;
        storage[9].resource = 42;
        storage[9].words[0] = 3;
        for (i = 0; i < 3; ++i)
        {
            node = 102 + r - i;
            storage[10 + i].kind = BH_DRAW_PATH_POINT;
            storage[10 + i].next = i == 2 ? BH_DRAW_END : 11 + i;
            put(&storage[10 + i].resource, r ? 20 - i * 10 + (i ? 2 : 0) : 10 - i * 10);
            put(storage[10 + i].words + 2, r ? 2 : 1);
            put(storage[10 + i].words + 3, i ? .5f : 0);
            put(storage[10 + i].words + 4, 123); /* UV/path distance stays current. */
            bh_interpolation_trail_point(10 + i, node, 1, node - 1, 1, 42);
        }
        bh_interpolation_stamp(9, 999, 1); /* Enclosing refresh cannot replace node IDs. */
        bh_interpolation_capture(s, &q, 1);
    }
    sample = bh_interpolation_sample(s, .5f);
    assert(get(&sample->commands[10].resource) == 15); /* New head 10 -> 20. */
    assert(get(&sample->commands[11].resource) == 11); /* Old head 10 -> 12. */
    assert(get(&sample->commands[12].resource) == 1);  /* Older point 0 -> 2. */
    assert(get(sample->commands[11].words + 3) == .25f);
    assert(get(sample->commands[11].words + 2) == 1.5f);
    assert(get(sample->commands[11].words + 4) == 123);
    assert(sample->commands[9].words[0] == 3 && sample->commands[10].next == 11);
    assert(get(&storage[10].resource) == 20);
    /* Recycled slots and a different trail image cannot inherit old motion. */
    bh_interpolation_trail_point(10, 102, 2, 0, 0, 42);
    bh_interpolation_trail_point(11, 102, 1, 0, 0, 99);
    bh_interpolation_capture(s, &q, 1);
    sample = bh_interpolation_sample(s, 0);
    assert(get(&sample->commands[10].resource) == 20);
    assert(get(&sample->commands[11].resource) == 12);
    free(s);
    puts(
        "SDL timing/interpolation: cadence, catch-up, payload preservation, immutable snapshots, generations, "
        "cuts, teleports, angle wrapping, nested draw identities, persistent flare consumer state, rigid cockpit attachments, world target anchors, growing trail topology and moving flare consumption passed");
    return 0;
}
