#include "bh_render_interpolation.h"
#include "../Game/Effects/bh_flash_flare.h"
#include "../Math/bh_rotation.h"
#include "../Game/Items/bh_pickup_draw.h"
#include <float.h>
#include <math.h>
#include <string.h>
static BHDrawQueue *tracked;
static BHDrawIdentity identities[BH_INTERPOLATION_CAPACITY];
void bh_interpolation_reset(void)
{
    memset(identities, 0, sizeof(identities));
}
void bh_interpolation_bind(BHDrawQueue *q)
{
    tracked = q;
}
unsigned long bh_interpolation_mark(void)
{
    return tracked ? tracked->count : 0;
}
static int spatial(unsigned long kind)
{
    return kind == BH_DRAW_CAMERA || kind == BH_DRAW_MODEL_RESOURCE || kind == BH_DRAW_BILLBOARD ||
           kind == BH_DRAW_SEGMENT || kind == BH_DRAW_ORIENTED || kind == BH_DRAW_QUAD || kind == BH_DRAW_RECT ||
           kind == BH_DRAW_FLARE || kind == BH_DRAW_PATH_POINT;
}
void bh_interpolation_stamp(unsigned long start, unsigned long identity, unsigned long generation)
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
void bh_interpolation_part(unsigned long index, unsigned long owner, unsigned long generation, unsigned long part)
{
    if (!tracked || !tracked->commands || index >= tracked->count || index >= BH_INTERPOLATION_CAPACITY || !owner ||
        part >= 0x80000000UL || !spatial(tracked->commands[index].kind) || identities[index].owner)
        return;
    identities[index].owner = owner;
    identities[index].generation = generation;
    identities[index].ordinal = 0x80000000UL | part;
}
/* Path payload starts in resource (xyz), so its image belongs in identity,
   not the ordinary resource comparison. Nodes retain identity as heads change. */
void bh_interpolation_trail_point(unsigned long index, unsigned long owner, unsigned long generation,
                                  unsigned long older_owner, unsigned long older_generation, unsigned long image)
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
void bh_interpolation_attach(unsigned long start, unsigned long end, unsigned long model)
{
    unsigned long i;
    if (!tracked || model >= tracked->count || tracked->commands[model].kind != BH_DRAW_MODEL_RESOURCE || start > end ||
        end > tracked->count)
        return;
    for (i = start; i < end; ++i)
        if (tracked->commands[i].kind == BH_DRAW_QUAD || tracked->commands[i].kind == BH_DRAW_ORIENTED)
            identities[i].attachment = model + 1;
}
static void model_rotation(const BHDrawCommand *command, BHMat34 *m)
{
    static const BHMat34 identity = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
    float pose[6];
    memcpy(pose, command->words, sizeof(pose));
    *m = identity;
    bh_mat34_rotate_y(pose[3], m);
    bh_mat34_rotate_x(pose[4], m);
    bh_mat34_rotate_z(pose[5], m);
}
static unsigned long key(const BHDrawCommand *command, const BHDrawIdentity *id)
{
    return (id->owner * 2654435761UL ^ id->generation * 2246822519UL ^ id->ordinal * 3266489917UL ^ command->kind) &
           (BH_INTERPOLATION_HASH - 1);
}
static int same(const BHDrawCommand *first_command, const BHDrawCommand *second_command, const BHDrawIdentity *ai, const BHDrawIdentity *bi)
{
    return first_command->kind == second_command->kind &&
           (first_command->kind == BH_DRAW_CAMERA || first_command->kind == BH_DRAW_PATH_POINT || first_command->resource == second_command->resource) &&
           (first_command->kind != BH_DRAW_FLARE || !memcmp(first_command->words + 6, second_command->words + 6, 16)) && ai->owner == bi->owner &&
           ai->generation == bi->generation && ai->ordinal == bi->ordinal;
}
void bh_interpolation_capture(BHRenderInterpolation *render_interpolation, const BHDrawQueue *queue, long scene)
{
    unsigned long i, slot;
    if (!render_interpolation || !queue || !queue->commands || queue->count > BH_INTERPOLATION_CAPACITY)
        return;
    if (render_interpolation->ready && scene == render_interpolation->scene)
    {
        render_interpolation->previous_count = render_interpolation->current_count;
        memcpy(render_interpolation->previous, render_interpolation->current, render_interpolation->previous_count * sizeof(BHDrawCommand));
        memcpy(render_interpolation->previous_ids, render_interpolation->current_ids, render_interpolation->previous_count * sizeof(BHDrawIdentity));
    }
    else
        render_interpolation->previous_count = 0;
    memcpy(render_interpolation->current, queue->commands, queue->count * sizeof(BHDrawCommand));
    memcpy(render_interpolation->current_ids, identities, queue->count * sizeof(BHDrawIdentity));
    render_interpolation->current_count = queue->count;
    render_interpolation->scene = scene;
    render_interpolation->ready = 1;
    render_interpolation->sampled = 0;
    for (i = 9; i < render_interpolation->current_count; ++i)
        if (render_interpolation->current[i].kind == BH_DRAW_FLARE)
            render_interpolation->flare_strength[i] = render_interpolation->current[i].words[4];
    memset(render_interpolation->hash, 0, sizeof(render_interpolation->hash));
    for (i = 9; i < render_interpolation->previous_count; ++i)
    {
        BHDrawCommand *command = render_interpolation->previous + i;
        if (!spatial(command->kind) || (!render_interpolation->previous_ids[i].owner && command->kind != BH_DRAW_CAMERA))
            continue;
        slot = key(command, render_interpolation->previous_ids + i);
        while (render_interpolation->hash[slot])
            slot = (slot + 1) & (BH_INTERPOLATION_HASH - 1);
        render_interpolation->hash[slot] = i + 1;
    }
}
static BHDrawCommand *previous_match(BHRenderInterpolation *render_interpolation, const BHDrawCommand *c, const BHDrawIdentity *id)
{
    unsigned long slot = key(c, id);
    while (render_interpolation->hash[slot])
    {
        unsigned long at = render_interpolation->hash[slot] - 1;
        if (same(c, render_interpolation->previous + at, id, render_interpolation->previous_ids + at))
            return render_interpolation->previous + at;
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
BHDrawQueue *bh_interpolation_sample(BHRenderInterpolation *render_interpolation, float alpha)
{
    unsigned long i, j, count, angle_start, last_anchor = BH_DRAW_END;
    BHMat34 delta;
    BHVec3 anchor_current, anchor_rendered;
    if (!render_interpolation || !render_interpolation->ready)
        return 0;
    if (!_finite(alpha) || alpha < 0)
        alpha = 0;
    if (alpha > 1)
        alpha = 1;
    memcpy(render_interpolation->output, render_interpolation->current, render_interpolation->current_count * sizeof(BHDrawCommand));
    for (i = 9; i < render_interpolation->current_count; ++i)
    {
        BHDrawCommand *command = render_interpolation->output + i, *old = 0;
        unsigned long *to, *from;
        if (command->kind == BH_DRAW_FLARE)
            command->words[4] = render_interpolation->flare_strength[i];
        if (render_interpolation->current_ids[i].attachment || !spatial(command->kind) ||
            (command->kind == BH_DRAW_RECT && command->words[12] != BH_RECT_WORLD_MARKER) ||
            (!render_interpolation->current_ids[i].owner && command->kind != BH_DRAW_CAMERA))
            continue;
        old = previous_match(render_interpolation, command, render_interpolation->current_ids + i);
        if (!old && command->kind == BH_DRAW_PATH_POINT && render_interpolation->current_ids[i].older_owner)
        {
            BHDrawIdentity older = render_interpolation->current_ids[i];
            older.owner = older.older_owner;
            older.generation = older.older_generation;
            old = previous_match(render_interpolation, command, &older);
        }
        if (!old)
            continue;
        to = (command->kind == BH_DRAW_CAMERA || command->kind == BH_DRAW_PATH_POINT) ? &command->resource
             : command->kind == BH_DRAW_RECT                                    ? command->words + 13
                                                                          : command->words;
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
        count = command->kind == BH_DRAW_ORIENTED || command->kind == BH_DRAW_QUAD                                         ? 9
                : command->kind == BH_DRAW_MODEL_RESOURCE || command->kind == BH_DRAW_CAMERA || command->kind == BH_DRAW_SEGMENT ? 6
                                                                                                               : 3;
        angle_start = command->kind == BH_DRAW_MODEL_RESOURCE || command->kind == BH_DRAW_CAMERA ? 3 : count;
        for (j = 0; j < count; ++j)
            blend(to + j, from + j, alpha, j >= angle_start);
        /* Only motion is shared by flare samples. Strength remains the exact
           per-publication consumer state above; never lerp or restore it. */
        if (command->kind == BH_DRAW_PATH_POINT)
        {
            blend(command->words + 2, old->words + 2, alpha, 0);
            blend(command->words + 3, old->words + 3, alpha, 0);
        }
        if (command->kind == BH_DRAW_BILLBOARD)
            blend(command->words + 5, old->words + 5, alpha, 1);
        if (command->kind == BH_DRAW_ORIENTED || command->kind == BH_DRAW_QUAD)
            blend(command->words + 11, old->words + 11, alpha, 1);
    }
    /* Rebase current HUD vertices through the same sampled vehicle transform.
       This also preserves newly appearing digits and avoids shearing on turns. */
    for (i = 9; i < render_interpolation->current_count; ++i)
        if (render_interpolation->current_ids[i].attachment)
        {
            BHMat34 current, inverse, rendered;
            BHVec3 point, result;
            unsigned long anchor = render_interpolation->current_ids[i].attachment - 1;
            if (anchor >= render_interpolation->current_count || !memcmp(render_interpolation->current[anchor].words, render_interpolation->output[anchor].words, 24))
                continue;
            if (anchor != last_anchor)
            {
                model_rotation(render_interpolation->current + anchor, &current);
                model_rotation(render_interpolation->output + anchor, &rendered);
                memcpy(&anchor_current, render_interpolation->current[anchor].words, sizeof(anchor_current));
                memcpy(&anchor_rendered, render_interpolation->output[anchor].words, sizeof(anchor_rendered));
                bh_mat34_inverse_rigid(&current, &inverse);
                bh_mat34_multiply(&rendered, &inverse, &delta);
                last_anchor = anchor;
            }
            for (j = 0; j < 9; j += 3)
            {
                memcpy(&point, render_interpolation->current[i].words + j, sizeof(point));
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
                memcpy(render_interpolation->output[i].words + j, &result, sizeof(result));
            }
        }
    render_interpolation->sampled = 1;
    render_interpolation->queue.commands = render_interpolation->output;
    render_interpolation->queue.count = render_interpolation->current_count;
    render_interpolation->queue.capacity = BH_INTERPOLATION_CAPACITY;
    return &render_interpolation->queue;
}

void bh_interpolation_rendered(BHRenderInterpolation *render_interpolation)
{
    unsigned long i;
    if (!render_interpolation || !render_interpolation->sampled)
        return;
    for (i = 9; i < render_interpolation->current_count; ++i)
        if (render_interpolation->current[i].kind == BH_DRAW_FLARE)
            render_interpolation->flare_strength[i] = render_interpolation->output[i].words[4];
    render_interpolation->sampled = 0;
}

unsigned long bh_simulation_advance(BHSimulationClock *simulation_clock, double now)
{
    unsigned long ticks;
    double elapsed;
    if (!simulation_clock || !_finite(now))
        return 0;
    if (!simulation_clock->ready)
    {
        simulation_clock->last = now;
        simulation_clock->remainder = 0;
        simulation_clock->ready = 1;
        return 1;
    }
    elapsed = now - simulation_clock->last;
    simulation_clock->last = now;
    if (elapsed < 0)
        elapsed = 0;
    /* Bound catch-up after a debugger stop, resize drag or suspended process.
       Audio still advances once per actual simulation tick, never per render. */
    if (elapsed > .25)
        elapsed = .25;
    simulation_clock->remainder += elapsed;
    ticks = (unsigned long)floor((simulation_clock->remainder + 1e-9) * 30.0);
    simulation_clock->remainder -= ticks / 30.0;
    if (simulation_clock->remainder < 0)
        simulation_clock->remainder = 0;
    return ticks;
}
