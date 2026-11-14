#include "bh_rail_paths.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#undef isdigit
static_assert(sizeof(BHRailSegment) == 44 && offsetof(BHRailSegment, visited) == 40, "rail segment size");
static void *allocate(unsigned long n, void *u)
{
    return malloc(n);
}
static void release(void *p, void *u)
{
    free(p);
}
static int fail(BHRailPaths *rail_paths)
{
    if (rail_paths)
        ++rail_paths->failures;
    return 0;
}
int bh_rail_paths_init(BHRailPaths *rail_paths, const BHSceneAllocator *scene_allocator)
{
    if (!rail_paths || (scene_allocator && (!scene_allocator->allocate || !scene_allocator->release)))
        return 0;
    memset(rail_paths, 0, sizeof(*rail_paths));
    if (scene_allocator)
        rail_paths->allocator = *scene_allocator;
    else
    {
        rail_paths->allocator.allocate = allocate;
        rail_paths->allocator.release = release;
    }
    rail_paths->segments = (BHRailSegment *)rail_paths->allocator.allocate(200UL * sizeof(BHRailSegment), rail_paths->allocator.user);
    if (!rail_paths->segments)
        return fail(rail_paths);
    memset(rail_paths->segments, 0, 200UL * sizeof(BHRailSegment));
    return 1;
}
void bh_rail_paths_dispose(BHRailPaths *rail_paths)
{
    if (!rail_paths)
        return;
    if (rail_paths->segments)
        rail_paths->allocator.release(rail_paths->segments, rail_paths->allocator.user);
    rail_paths->segments = NULL;
    rail_paths->count = rail_paths->chain_count = 0;
}
static int valid(const BHRailPaths *rail_paths)
{
    return rail_paths && rail_paths->segments && rail_paths->count >= 0 && rail_paths->count <= 200 && rail_paths->chain_count >= 0 && rail_paths->chain_count <= rail_paths->count;
}
static double metric(const BHVec3 *first_vector, const BHVec3 *second_vector)
{
    double x = (double)first_vector->x - second_vector->x, y = (double)first_vector->y - second_vector->y, z = (double)first_vector->z - second_vector->z;
    float rx, rz;
    bh_round_float(&rx, x);
    bh_round_float(&rz, z);
    return (x * rx + y * y) + z * rz;
}
int bh_rail_near(const BHVec3 *a, const BHVec3 *b)
{
    double d;
    if (!a || !b)
        return 0;
    d = metric(a, b);
    return _isnan(d) || d < 0.25;
}
static int alias(const BHRailPaths *rail_paths, const void *v)
{
    unsigned long at = (unsigned long)v, base = (unsigned long)rail_paths->segments;
    return at >= base && at < base + 200UL * sizeof(BHRailSegment);
}
static void transform(BHVec3 *out, const BHVec3 *vector, const BHVec3 *position, double sine, double cosine)
{
    bh_round_float(&out->x, (double)vector->x * cosine + (double)vector->z * sine);
    out->y = vector->y;
    bh_round_float(&out->z, (double)vector->z * cosine - (double)vector->x * sine);
    bh_round_float(&out->x, (double)out->x + position->x);
    bh_round_float(&out->y, (double)out->y + position->y);
    bh_round_float(&out->z, (double)out->z + position->z);
}
int bh_rail_add(BHRailPaths *rail_paths, const BHVec3 *start, const BHVec3 *end, const BHVec3 *position, const BHVec3 *rotation)
{
    BHRailSegment *rail_segment;
    double sine, cosine;
    if (!valid(rail_paths) || !start || !end || !position || !rotation || !bh_math_environment_supported())
        return fail(rail_paths);
    if (rail_paths->count == 200)
    {
        ++rail_paths->dropped;
        return 1;
    }
    if (!_finite(rotation->x) || fabs(rotation->x) > 1000000 || alias(rail_paths, start) || alias(rail_paths, end) ||
        alias(rail_paths, position) || alias(rail_paths, rotation))
        return fail(rail_paths);
    sine = sin((double)rotation->x);
    cosine = cos((double)rotation->x);
    rail_segment = rail_paths->segments + rail_paths->count++;
    transform(&rail_segment->start, start, position, sine, cosine);
    transform(&rail_segment->end, end, position, sine, cosine);
    rail_segment->next = rail_segment->previous = NULL;
    rail_segment->visited = 0;
    ++rail_paths->additions;
    return 1;
}
static int part(BHRailPaths *p, const BHVec3 *position, const BHVec3 *rotation, float x1, float y1, float z1, float x2,
                float y2, float z2)
{
    BHVec3 a, b;
    a.x = x1;
    a.y = y1;
    a.z = z1;
    b.x = x2;
    b.y = y2;
    b.z = z2;
    return bh_rail_add(p, &a, &b, position, rotation);
}
int bh_rail_placement(BHRailPaths *rail_paths, const char *name, const BHVec3 *position, const BHVec3 *rotation)
{
    if (!rail_paths || !name || !position || !rotation)
        return fail(rail_paths);
    ++rail_paths->placements;
    if (_strnicmp(name, "RC_", 3) || !isdigit((int)(signed char)name[3]))
        return 1;
    switch (name[3])
    {
    case '0':
        return part(rail_paths, position, rotation, 5, 0, 0, 5, 0, 10);
    case '1':
        return part(rail_paths, position, rotation, 5, 0, 0, 5, 0, 20);
    case '2':
        return part(rail_paths, position, rotation, 5, -10, 0, 5, 0, 20);
    case '3':
        return part(rail_paths, position, rotation, 5, 0, 0, 5, -10, 10);
    case '4':
        return part(rail_paths, position, rotation, 5, -20, 10, 5, 0, 20);
    case '5':
        return part(rail_paths, position, rotation, 5, 0, 0, 5, -20, 0);
    case '6':
        return part(rail_paths, position, rotation, 5, -20, 10, 5, 0, 20);
    case '7':
        if (!part(rail_paths, position, rotation, 5, 0, 0, 10, -6.66f, 18))
            return 0;
        if (!part(rail_paths, position, rotation, 10, -6.66f, 18, 22, -13.33f, 30))
            return 0;
        return part(rail_paths, position, rotation, 22, -13.33f, 30, 40, -20, 35);
    case '8':
        if (!part(rail_paths, position, rotation, 35, 0, 0, 30, -6.66f, 18))
            return 0;
        if (!part(rail_paths, position, rotation, 30, -6.66f, 18, 18, -13.33f, 30))
            return 0;
        return part(rail_paths, position, rotation, 18, -13.33f, 30, 0, -20, 35);
    case '9':
        if (!part(rail_paths, position, rotation, 5, 0, 0, 10, 0, 18))
            return 0;
        if (!part(rail_paths, position, rotation, 10, 0, 18, 22, 0, 30))
            return 0;
        return part(rail_paths, position, rotation, 22, 0, 30, 40, 0, 35);
    }
    return 1;
}
static int pointer(const BHRailPaths *rail_paths, const BHRailSegment *v)
{
    unsigned long at = (unsigned long)v, base = (unsigned long)rail_paths->segments;
    return !v || (at >= base && at - base < (unsigned long)rail_paths->count * sizeof(*v) && (at - base) % sizeof(*v) == 0);
}
int bh_rail_link(BHRailPaths *rail_paths)
{
    long i;
    BHRailSegment temporary, *head, *current, *candidate;
    BHVec3 point;
    int found;
    if (!valid(rail_paths) || !bh_math_environment_supported())
        return fail(rail_paths);
    for (i = 0; i < rail_paths->count; ++i)
        if (!pointer(rail_paths, rail_paths->segments[i].next) || !pointer(rail_paths, rail_paths->segments[i].previous))
            return fail(rail_paths);
    for (;;)
    {
        for (i = 0; i < rail_paths->count && rail_paths->segments[i].visited; ++i)
        {
        }
        if (i == rail_paths->count)
            break;
        if (rail_paths->chain_count >= rail_paths->count)
            return fail(rail_paths);
        head = rail_paths->segments + rail_paths->chain_count;
        candidate = rail_paths->segments + i;
        temporary = *head;
        *head = *candidate;
        *candidate = temporary;
        if (candidate->next)
            candidate->next->previous = candidate;
        if (candidate->previous)
            candidate->previous->next = candidate;
        ++rail_paths->chain_count;
        head->visited = 1;
        current = head;
        for (;;)
        {
            found = 0;
            for (i = 0; i < rail_paths->count; ++i)
            {
                candidate = rail_paths->segments + i;
                if (candidate->visited)
                    continue;
                if (bh_rail_near(&current->end, &candidate->start) && !bh_rail_near(&current->start, &candidate->end))
                {
                    candidate->visited = 1;
                    found = 1;
                    break;
                }
                if (bh_rail_near(&current->end, &candidate->end) && !bh_rail_near(&current->start, &candidate->start))
                {
                    candidate->visited = 1;
                    point = candidate->start;
                    candidate->start = candidate->end;
                    candidate->end = point;
                    found = 1;
                    break;
                }
            }
            if (!found)
                break;
            candidate->previous = current;
            current->next = candidate;
            current = candidate;
        }
        if (bh_rail_near(&current->end, &head->start))
        {
            current->next = head;
            head->previous = current;
        }
    }
    for (i = 0; i < rail_paths->count; ++i)
    {
        current = rail_paths->segments + i;
        bh_round_float(&current->length, sqrt(metric(&current->end, &current->start)));
        bh_round_float(&current->rise, (double)current->end.y - current->start.y);
    }
    return 1;
}
