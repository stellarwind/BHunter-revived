#include "bh_render_interpolation.h"
#include <float.h>
#include <math.h>
#include <string.h>
static BHDrawQueue *tracked;
static BHDrawIdentity identities[BH_INTERPOLATION_CAPACITY];
void __cdecl bh_interpolation_reset(void)
{
    memset(identities, 0, sizeof(identities));
}
void __cdecl bh_interpolation_bind(BHDrawQueue *q)
{
    tracked = q;
}
unsigned long __cdecl bh_interpolation_mark(void)
{
    return tracked ? tracked->count : 0;
}
static int spatial(unsigned long kind)
{
    return kind == BH_DRAW_CAMERA || kind == BH_DRAW_MODEL_RESOURCE || kind == BH_DRAW_BILLBOARD ||
           kind == BH_DRAW_SEGMENT || kind == BH_DRAW_ORIENTED || kind == BH_DRAW_QUAD;
}
void __cdecl bh_interpolation_stamp(unsigned long start, unsigned long identity, unsigned long generation)
{
    unsigned long i, ordinal = 0;
    if (!tracked || !tracked->commands || start > tracked->count)
        return;
    for (i = start; i < tracked->count; ++i)
        if (spatial(tracked->commands[i].kind) && !identities[i].owner)
        {
            identities[i].owner = identity;
            identities[i].generation = generation;
            identities[i].ordinal = ordinal++;
        }
}
static unsigned long key(const BHDrawCommand *c, const BHDrawIdentity *id)
{
    return (id->owner * 2654435761UL ^ id->generation * 2246822519UL ^ id->ordinal * 3266489917UL ^ c->kind) &
           (BH_INTERPOLATION_HASH - 1);
}
static int same(const BHDrawCommand *a, const BHDrawCommand *b, const BHDrawIdentity *ai, const BHDrawIdentity *bi)
{
    return a->kind == b->kind && (a->kind == BH_DRAW_CAMERA || a->resource == b->resource) &&
           ai->owner == bi->owner && ai->generation == bi->generation && ai->ordinal == bi->ordinal;
}
void __cdecl bh_interpolation_capture(BHRenderInterpolation *s, const BHDrawQueue *q, long scene)
{
    unsigned long i, slot;
    if (!s || !q || !q->commands || q->count > BH_INTERPOLATION_CAPACITY)
        return;
    if (s->ready && scene == s->scene)
    {
        s->previous_count = s->current_count;
        memcpy(s->previous, s->current, s->previous_count * sizeof(BHDrawCommand));
        memcpy(s->previous_ids, s->current_ids, s->previous_count * sizeof(BHDrawIdentity));
    }
    else
        s->previous_count = 0;
    memcpy(s->current, q->commands, q->count * sizeof(BHDrawCommand));
    memcpy(s->current_ids, identities, q->count * sizeof(BHDrawIdentity));
    s->current_count = q->count;
    s->scene = scene;
    s->ready = 1;
    memset(s->hash, 0, sizeof(s->hash));
    for (i = 9; i < s->previous_count; ++i)
    {
        BHDrawCommand *c = s->previous + i;
        if (!spatial(c->kind) || (!s->previous_ids[i].owner && c->kind != BH_DRAW_CAMERA))
            continue;
        slot = key(c, s->previous_ids + i);
        while (s->hash[slot])
            slot = (slot + 1) & (BH_INTERPOLATION_HASH - 1);
        s->hash[slot] = i + 1;
    }
}
static float number(const unsigned long *p)
{
    float f;
    memcpy(&f, p, 4);
    return f;
}
static void blend(unsigned long *out, const unsigned long *old, float alpha, int angle)
{
    float a = number(old), b = number(out), delta, value;
    if (!_finite(a) || !_finite(b))
        return;
    delta = b - a;
    if (angle)
    {
        delta = (float)fmod(delta, 6.283185307179586);
        if (delta > 3.141592653589793)
            delta -= 6.283185307179586f;
        if (delta < -3.141592653589793)
            delta += 6.283185307179586f;
    }
    value = a + alpha * delta;
    memcpy(out, &value, 4);
}
BHDrawQueue *__cdecl bh_interpolation_sample(BHRenderInterpolation *s, float alpha)
{
    unsigned long i, j, slot, count, angle_start;
    if (!s || !s->ready)
        return 0;
    if (!_finite(alpha) || alpha < 0)
        alpha = 0;
    if (alpha > 1)
        alpha = 1;
    memcpy(s->output, s->current, s->current_count * sizeof(BHDrawCommand));
    for (i = 9; i < s->current_count; ++i)
    {
        BHDrawCommand *c = s->output + i, *old = 0;
        unsigned long *to, *from;
        if (!spatial(c->kind) || (!s->current_ids[i].owner && c->kind != BH_DRAW_CAMERA))
            continue;
        slot = key(c, s->current_ids + i);
        while (s->hash[slot])
        {
            BHDrawCommand *p = s->previous + s->hash[slot] - 1;
            if (same(c, p, s->current_ids + i, s->previous_ids + s->hash[slot] - 1))
            {
                old = p;
                break;
            }
            slot = (slot + 1) & (BH_INTERPOLATION_HASH - 1);
        }
        if (!old)
            continue;
        to = c->kind == BH_DRAW_CAMERA ? &c->resource : c->words;
        from = old->kind == BH_DRAW_CAMERA ? &old->resource : old->words;
        /* Teleports and scene cuts must snap. Ordinary fast vehicles can still
           move up to 1000 world units per 30 Hz tick without hitting this guard. */
        for (j = 0; j < 3; ++j)
            if (fabs(number(to + j) - number(from + j)) > 1000)
                break;
        if (j != 3)
            continue;
        count = c->kind == BH_DRAW_ORIENTED || c->kind == BH_DRAW_QUAD                                         ? 9
                : c->kind == BH_DRAW_MODEL_RESOURCE || c->kind == BH_DRAW_CAMERA || c->kind == BH_DRAW_SEGMENT ? 6
                                                                                                               : 3;
        angle_start = c->kind == BH_DRAW_MODEL_RESOURCE || c->kind == BH_DRAW_CAMERA ? 3 : count;
        for (j = 0; j < count; ++j)
            blend(to + j, from + j, alpha, j >= angle_start);
        if (c->kind == BH_DRAW_BILLBOARD)
            blend(c->words + 5, old->words + 5, alpha, 1);
        if (c->kind == BH_DRAW_ORIENTED || c->kind == BH_DRAW_QUAD)
            blend(c->words + 11, old->words + 11, alpha, 1);
    }
    s->queue.commands = s->output;
    s->queue.count = s->current_count;
    s->queue.capacity = BH_INTERPOLATION_CAPACITY;
    return &s->queue;
}

unsigned long __cdecl bh_simulation_advance(BHSimulationClock *c, double now)
{
    unsigned long ticks;
    double elapsed;
    if (!c || !_finite(now))
        return 0;
    if (!c->ready)
    {
        c->last = now;
        c->remainder = 0;
        c->ready = 1;
        return 1;
    }
    elapsed = now - c->last;
    c->last = now;
    if (elapsed < 0)
        elapsed = 0;
    /* Bound catch-up after a debugger stop, resize drag or suspended process.
       Audio still advances once per actual simulation tick, never per render. */
    if (elapsed > .25)
        elapsed = .25;
    c->remainder += elapsed;
    ticks = (unsigned long)floor((c->remainder + 1e-9) * 30.0);
    c->remainder -= ticks / 30.0;
    if (c->remainder < 0)
        c->remainder = 0;
    return ticks;
}
