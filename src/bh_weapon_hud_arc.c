#include "bh_weapon_hud_arc.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>

typedef char arc_target_query_offset[offsetof(BHEntity, query) == 0xd0 ? 1 : -1];
typedef char arc_random_range[RAND_MAX == 32767 ? 1 : -1];
typedef union BHArcPacket {
    BHEntityPositionPacket position;
    unsigned long words[10];
} BHArcPacket;
static int finite_vec(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int fail(BHArcContext *c, BHArcError e)
{
    if (c)
        c->error = e;
    return 0;
}
static void begin(BHArcContext *c)
{
    c->attempted = c->queued = c->dropped = c->path_points = c->random_calls = 0;
    c->error = BH_ARC_OK;
}
static float stored(double n)
{
    float f;
    bh_round_float(&f, n);
    return f;
}
static int random_value(BHArcContext *c)
{
    ++c->random_calls;
    return rand();
}
static void put(unsigned long *out, float f)
{
    memcpy(out, &f, 4);
}
static BHDrawCommand *reserve(BHArcContext *c)
{
    BHDrawCommand *p = bh_queue_reserve(c->queue, 3);
    ++c->attempted;
    if (p)
        ++c->queued;
    else if (c->queue->error == BH_DRAW_FULL)
        ++c->dropped;
    else
        c->error = BH_ARC_QUEUE;
    return p;
}
static void point(BHDrawCommand *p, const BHVec3 *position, float width, float start, float end, float modulation,
                  unsigned long tile)
{
    put(&p->resource, position->x);
    put(p->words, position->y);
    put(p->words + 1, position->z);
    put(p->words + 2, width);
    put(p->words + 3, start);
    put(p->words + 4, end);
    put(p->words + 5, modulation);
    p->words[6] = tile;
    p->kind = BH_DRAW_PATH_POINT;
}
static int curve(BHArcContext *c, const BHVec3 *start, const BHVec3 *control, const BHVec3 *target_control,
                 const BHVec3 *target, float strength)
{
    BHDrawCommand *header, *p;
    BHVec3 position;
    float progress = 0, complement, blend, other, root, envelope;
    double step, raw_complement, squared, raw_blend, line_a, line_b;
    unsigned long count = 0, axis, tile;
    long i;
    const float *a = (const float *)start, *b = (const float *)control, *d = (const float *)target_control,
                *e = (const float *)target;
    if (strength > 1)
        strength = 1;
    header = reserve(c);
    if (!header)
        return c->error == BH_ARC_OK;
    header->resource = c->image;
    p = reserve(c);
    if (p)
    {
        tile = (unsigned long)random_value(c) & 3;
        point(p, start, .5f, 0, 0, stored((double)strength * 100), tile);
        count = 1;
    }
    root = stored(sqrt((double)strength));
    step = (double)strength * -.0625;
    for (i = 7; i >= -8; --i)
    {
        progress = stored((double)progress - step);
        p = reserve(c);
        if (!p)
            continue;
        raw_complement = 1.0 - progress;
        complement = stored(raw_complement);
        squared = raw_complement * complement;
        raw_blend = squared * 3.0 - (squared * complement + squared * complement);
        blend = stored(raw_blend);
        other = stored(1.0 - raw_blend);
        for (axis = 0; axis < 3; ++axis)
        {
            line_a = (double)complement * a[axis] + (double)progress * b[axis];
            line_b = (double)complement * d[axis] + (double)progress * e[axis];
            /* X retains interpolated lines; Y/Z reload their float stores. */
            if (axis)
            {
                line_a = stored(line_a);
                line_b = stored(line_b);
            }
            ((float *)&position)[axis] = stored(line_a * blend + line_b * other);
        }
        if (i > -8)
        {
            envelope = stored(((double)(64 - i * i) * strength) * .000000476837158203125);
            position.x = stored(((double)random_value(c) - 16384.0) * envelope + position.x);
            position.y = stored(((double)random_value(c) - 16384.0) * envelope + position.y);
            position.z = stored(((double)random_value(c) - 16384.0) * envelope + position.z);
        }
        tile = (unsigned long)random_value(c) & 3;
        point(p, &position, 1.25f, root, (float)count, stored(((double)strength - progress) * 100), tile);
        ++count;
    }
    header->words[0] = count;
    header->kind = BH_DRAW_PATH_HEADER;
    c->path_points = count;
    return c->error == BH_ARC_OK;
}
int __cdecl bh_arc_queue(BHArcContext *c, const BHVec3 *start, const BHVec3 *control, const BHVec3 *target_control,
                         const BHVec3 *target, float strength)
{
    if (!c || !c->queue || !finite_vec(start) || !finite_vec(control) || !finite_vec(target_control) ||
        !finite_vec(target))
        return fail(c, BH_ARC_ARGUMENT);
    begin(c);
    if (!_finite(strength) || strength < 0)
        return fail(c, BH_ARC_NUMERICAL);
    return curve(c, start, control, target_control, target, strength);
}
int __cdecl bh_weapon_hud_arc(BHArcContext *c, BHWeaponHudState *state, const BHWeaponHudArcPlayer *player)
{
    BHVec3 start, control, jitter;
    BHArcPacket packet;
    BHEntity *target;
    float strength;
    unsigned long i;
    if (!c || !state || !c->queue)
        return fail(c, BH_ARC_ARGUMENT);
    begin(c);
    target = state->target;
    if (!target)
        return 1;
    if (!player || !finite_vec(&player->position) || !finite_vec(&player->forward) || !finite_vec(&player->aim))
        return fail(c, BH_ARC_ARGUMENT);
    if (!target->query)
        return fail(c, BH_ARC_CALLBACK);
    for (i = 0; i < 3; ++i)
    {
        ((float *)&start)[i] =
            stored((double)((const float *)&player->position)[i] + ((const float *)&player->forward)[i]);
        ((float *)&control)[i] =
            stored((double)((float *)&start)[i] - (double)((const float *)&player->aim)[i] * -100.0);
    }
    packet.position.event = 3;
    target->query(target, &packet);
    if (!finite_vec(&packet.position.position))
        return fail(c, BH_ARC_NUMERICAL);
    jitter = packet.position.position;
    jitter.x = stored((double)jitter.x - ((double)random_value(c) - 16384.0) * -.00048828125);
    jitter.y = stored((double)jitter.y - ((double)random_value(c) - 16384.0) * -.00048828125);
    {
        int r = random_value(c);
        strength = stored((double)state->charge * .020833333333333332);
        jitter.z = stored((double)jitter.z - ((double)r - 16384.0) * -.00048828125);
    }
    if (!finite_vec(&start) || !finite_vec(&control) || !finite_vec(&jitter) || !_finite(strength) || strength < 0)
        return fail(c, BH_ARC_NUMERICAL);
    return curve(c, &start, &control, &jitter, &packet.position.position, strength);
}
