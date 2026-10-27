#include "bh_world_segment.h"
#include "bh_round.h"
#include <string.h>

static BHWorldSegmentRoom *binding(BHWorldSegmentContext *q, BHWorldGeometry *w)
{
    unsigned long i;
    for (i = 0; i < q->room_count; ++i)
        if (q->rooms[i].world == w)
            return q->rooms + i;
    return NULL;
}
int __cdecl bh_camera_correct(BHCameraCorrectionOwner *owner, BHVec3 *eye, const BHVec3 *target, BHEntity *self)
{
    BHWorldSegmentHit hit;
    BHWorldHit excluded;
    BHWorldGeometry *old_cached, *second_cached, *old_selected, *selected, *world;
    BHWorldSegmentRoom *r;
    BHVec3 first, second, result, old_eye;
    double amount;
    int found;
    if (!owner || !owner->query || !eye || !target)
        return 0;
    memset(&hit, 0, sizeof(hit));
    excluded.kind = BH_WORLD_HIT_ACTOR;
    excluded.object.actor = self;
    old_cached = owner->cached;
    old_selected = owner->selected;
    old_eye = *eye;
    selected = owner->selected;
    found = bh_world_segment_run(owner->query, &excluded, &owner->cached, target, eye, -1, &hit);
    if (found < 0)
    {
        owner->error = owner->query->error;
        return 0;
    }
    if (!found)
    {
        owner->error = BH_WORLD_SEGMENT_OK;
        return 1;
    }
    if (hit.object.kind == BH_WORLD_HIT_GEOMETRY)
    {
        world = hit.object.object.geometry;
        r = binding(owner->query, world);
        if (r && r->kind)
        {
            if (world->owner)
                world = world->owner;
            r = binding(owner->query, world);
            if (r && r->kind)
                selected = world;
        }
    }
    bh_round_float(&first.x, (double)hit.point.x - (double)hit.normal.x * -.1);
    bh_round_float(&first.y, (double)hit.point.y - (double)hit.normal.y * -.1);
    bh_round_float(&first.z, (double)hit.point.z - (double)hit.normal.z * -.1);
    bh_round_float(&second.x, (double)hit.point.x - (double)hit.normal.x * -.6);
    bh_round_float(&second.y, (double)hit.point.y - (double)hit.normal.y * -.6);
    bh_round_float(&second.z, (double)hit.point.z - (double)hit.normal.z * -.6);
    owner->selected = selected;
    *eye = hit.point;
    second_cached = owner->cached;
    found = bh_world_segment_run(owner->query, &excluded, &second_cached, &first, &second, -1, &hit);
    if (found < 0)
    {
        owner->cached = old_cached;
        owner->selected = old_selected;
        *eye = old_eye;
        owner->error = owner->query->error;
        return 0;
    }
    if (found)
    {
        amount = .1 - (double)hit.fraction * -.25;
        bh_round_float(&result.x, (double)hit.point.x + (double)hit.normal.x * amount);
        bh_round_float(&result.y, (double)hit.point.y + (double)hit.normal.y * amount);
        bh_round_float(&result.z, (double)hit.point.z + (double)hit.normal.z * amount);
    }
    else
        result = second;
    *eye = result;
    owner->error = BH_WORLD_SEGMENT_OK;
    return 1;
}
void __cdecl bh_camera_correct_callback(BHVec3 *eye, const BHVec3 *target, unsigned long actor_token, void *user)
{
    bh_camera_correct((BHCameraCorrectionOwner *)user, eye, target, (BHEntity *)actor_token);
}
