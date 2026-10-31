#include "bh_render_interpolation.h"
#include "bh_flash_flare.h"
#include "bh_rotation.h"
#include "bh_pickup_draw.h"
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
           kind == BH_DRAW_SEGMENT || kind == BH_DRAW_ORIENTED || kind == BH_DRAW_QUAD || kind == BH_DRAW_RECT ||
           kind == BH_DRAW_FLARE || kind == BH_DRAW_PATH_POINT;
}
void __cdecl bh_interpolation_stamp(unsigned long start, unsigned long identity, unsigned long generation)
{
    unsigned long i, ordinal = 0, flare_ordinal = 0;
    if (!tracked || !tracked->commands || start > tracked->count)
        return;
    for (i = start; i < tracked->count; ++i)
        if (spatial(tracked->commands[i].kind) && tracked->commands[i].kind != BH_DRAW_PATH_POINT &&
            (tracked->commands[i].kind != BH_DRAW_RECT || tracked->commands[i].words[12] == BH_RECT_WORLD_MARKER) &&
            !identities[i].owner)
        {
            identities[i].owner = identity;
            identities[i].generation = generation;
            identities[i].ordinal = tracked->commands[i].kind == BH_DRAW_FLARE ? flare_ordinal++ : ordinal++;
        }
}
/* Path payload starts in resource (xyz), so its image belongs in identity,
   not the ordinary resource comparison. Nodes retain identity as heads change. */
void __cdecl bh_interpolation_trail_point(unsigned long index, unsigned long owner, unsigned long generation,
                                          unsigned long older_owner, unsigned long older_generation,
                                          unsigned long image)
{
    BHDrawIdentity *id;
    if (!tracked || index >= tracked->count || tracked->commands[index].kind != BH_DRAW_PATH_POINT)
        return;
    id = identities + index;
    id->owner = owner;
    id->generation = generation;
    id->ordinal = image;
    id->older_owner = older_owner;
    id->older_generation = older_generation;
}
/* Attach a producer range after its vehicle model has been queued. The HUD's
   current contents may change (ammo digits, radar rooms, gauges), so command
   ordinal matching cannot describe its motion. Rebase it as one rigid object. */
void __cdecl bh_interpolation_attach(unsigned long start, unsigned long end, unsigned long model)
{
    unsigned long i;
    if (!tracked || model >= tracked->count || tracked->commands[model].kind != BH_DRAW_MODEL_RESOURCE || start > end ||
        end > tracked->count)
        return;
    for (i = start; i < end; ++i)
        if (tracked->commands[i].kind == BH_DRAW_QUAD || tracked->commands[i].kind == BH_DRAW_ORIENTED)
            identities[i].attachment = model + 1;
}
static void model_rotation(const BHDrawCommand *c, BHMat34 *m)
{
    static const BHMat34 identity = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
    float pose[6];
    memcpy(pose, c->words, sizeof(pose));
    *m = identity;
    bh_mat34_rotate_y(pose[3], m);
    bh_mat34_rotate_x(pose[4], m);
    bh_mat34_rotate_z(pose[5], m);
}
static unsigned long key(const BHDrawCommand *c, const BHDrawIdentity *id)
{
    return (id->owner * 2654435761UL ^ id->generation * 2246822519UL ^ id->ordinal * 3266489917UL ^ c->kind) &
           (BH_INTERPOLATION_HASH - 1);
}
static int same(const BHDrawCommand *a, const BHDrawCommand *b, const BHDrawIdentity *ai, const BHDrawIdentity *bi)
{
    return a->kind == b->kind &&
           (a->kind == BH_DRAW_CAMERA || a->kind == BH_DRAW_PATH_POINT || a->resource == b->resource) &&
           (a->kind != BH_DRAW_FLARE || !memcmp(a->words + 6, b->words + 6, 16)) && ai->owner == bi->owner &&
           ai->generation == bi->generation && ai->ordinal == bi->ordinal;
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
    s->sampled = 0;
    for (i = 9; i < s->current_count; ++i)
        if (s->current[i].kind == BH_DRAW_FLARE)
            s->flare_strength[i] = s->current[i].words[4];
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
static BHDrawCommand *previous_match(BHRenderInterpolation *s, const BHDrawCommand *c, const BHDrawIdentity *id)
{
    unsigned long slot = key(c, id);
    while (s->hash[slot])
    {
        unsigned long at = s->hash[slot] - 1;
        if (same(c, s->previous + at, id, s->previous_ids + at))
            return s->previous + at;
        slot = (slot + 1) & (BH_INTERPOLATION_HASH - 1);
    }
    return 0;
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
    unsigned long i, j, count, angle_start, last_anchor = BH_DRAW_END;
    BHMat34 delta;
    BHVec3 anchor_current, anchor_rendered;
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
        if (c->kind == BH_DRAW_FLARE)
            c->words[4] = s->flare_strength[i];
        if (s->current_ids[i].attachment || !spatial(c->kind) ||
            (c->kind == BH_DRAW_RECT && c->words[12] != BH_RECT_WORLD_MARKER) ||
            (!s->current_ids[i].owner && c->kind != BH_DRAW_CAMERA))
            continue;
        old = previous_match(s, c, s->current_ids + i);
        if (!old && c->kind == BH_DRAW_PATH_POINT && s->current_ids[i].older_owner)
        {
            BHDrawIdentity older = s->current_ids[i];
            older.owner = older.older_owner;
            older.generation = older.older_generation;
            old = previous_match(s, c, &older);
        }
        if (!old)
            continue;
        to = (c->kind == BH_DRAW_CAMERA || c->kind == BH_DRAW_PATH_POINT) ? &c->resource
             : c->kind == BH_DRAW_RECT                                    ? c->words + 13
                                                                          : c->words;
        from = (old->kind == BH_DRAW_CAMERA || old->kind == BH_DRAW_PATH_POINT) ? &old->resource
               : old->kind == BH_DRAW_RECT                                      ? old->words + 13
                                                                                : old->words;
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
        /* Only motion is shared by flare samples. Strength remains the exact
           per-publication consumer state above; never lerp or restore it. */
        if (c->kind == BH_DRAW_PATH_POINT)
        {
            blend(c->words + 2, old->words + 2, alpha, 0);
            blend(c->words + 3, old->words + 3, alpha, 0);
        }
        if (c->kind == BH_DRAW_BILLBOARD)
            blend(c->words + 5, old->words + 5, alpha, 1);
        if (c->kind == BH_DRAW_ORIENTED || c->kind == BH_DRAW_QUAD)
            blend(c->words + 11, old->words + 11, alpha, 1);
    }
    /* Rebase current HUD vertices through the same sampled vehicle transform.
       This also preserves newly appearing digits and avoids shearing on turns. */
    for (i = 9; i < s->current_count; ++i)
        if (s->current_ids[i].attachment)
        {
            BHMat34 current, inverse, rendered;
            BHVec3 point, result;
            unsigned long anchor = s->current_ids[i].attachment - 1;
            if (anchor >= s->current_count || !memcmp(s->current[anchor].words, s->output[anchor].words, 24))
                continue;
            if (anchor != last_anchor)
            {
                model_rotation(s->current + anchor, &current);
                model_rotation(s->output + anchor, &rendered);
                memcpy(&anchor_current, s->current[anchor].words, sizeof(anchor_current));
                memcpy(&anchor_rendered, s->output[anchor].words, sizeof(anchor_rendered));
                bh_mat34_inverse_rigid(&current, &inverse);
                bh_mat34_multiply(&rendered, &inverse, &delta);
                last_anchor = anchor;
            }
            for (j = 0; j < 9; j += 3)
            {
                memcpy(&point, s->current[i].words + j, sizeof(point));
                /* Subtract the origin before rotating to avoid cancellation
                   between large world coordinates and tiny cockpit offsets. */
                if (!j)
                {
                    point.x -= anchor_current.x;
                    point.y -= anchor_current.y;
                    point.z -= anchor_current.z;
                }
                bh_mat34_transform_direction(&delta, &point, &result);
                if (!j)
                {
                    result.x += anchor_rendered.x;
                    result.y += anchor_rendered.y;
                    result.z += anchor_rendered.z;
                }
                memcpy(s->output[i].words + j, &result, sizeof(result));
            }
        }
    s->sampled = 1;
    s->queue.commands = s->output;
    s->queue.count = s->current_count;
    s->queue.capacity = BH_INTERPOLATION_CAPACITY;
    return &s->queue;
}

void __cdecl bh_interpolation_rendered(BHRenderInterpolation *s)
{
    unsigned long i;
    if (!s || !s->sampled)
        return;
    for (i = 9; i < s->current_count; ++i)
        if (s->current[i].kind == BH_DRAW_FLARE)
            s->flare_strength[i] = s->output[i].words[4];
    s->sampled = 0;
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
