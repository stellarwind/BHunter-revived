#include "../Entities/bh_field_access.h"
#include "bh_hit_marker.h"
#include "../../Core/bh_callback_helpers.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4, "marker layout");
static BHEntity **owner_head(BHHitMarkerContext *marker_context, BHEntity *owner)
{
    BHEntity **head = NULL;
    if (marker_context->owner_head)
        head = marker_context->owner_head(owner, marker_context->owner_head_user);
    return head ? head : &owner->placement.attachments;
}
static int unlink_owner(BHHitMarkerContext *marker_context, BHEntity *marker)
{
    BHEntity *owner = marker->marker.owner, *older = marker->marker.owner_older, *newer = marker->marker.owner_newer;
    if (!owner)
        return 0;
    if (older)
        (older->marker.owner_newer = newer);
    if (newer)
        (newer->marker.owner_older = older);
    else
        *owner_head(marker_context, owner) = older;
    return 1;
}
BHEntity *bh_hit_marker_create(BHHitMarkerContext *marker_context, const BHVec3 *hit_position, const BHVec3 *hit_normal,
                               float size, unsigned long flags, BHEntity *owner)
{
    BHEntity *marker, *prior, *old, **head;
    BHVec3 normal, position, up;
    BHHitMarkerDraw draw = bh_hit_marker_draw;
    double dot;
    if (!marker_context || !marker_context->pool || !marker_context->queue || !hit_position || !hit_normal || !owner ||
        marker_context->count > 200 || !bh_math_environment_supported())
    {
        if (marker_context)
            marker_context->error = BH_HIT_MARKER_ARGUMENT;
        return NULL;
    }
    if ((marker_context->count && (!marker_context->oldest || !marker_context->newest)) ||
        (!marker_context->count && (marker_context->oldest || marker_context->newest)))
    {
        marker_context->error = BH_HIT_MARKER_TOPOLOGY;
        return NULL;
    }
    normal = *hit_normal;
    position = *hit_position;
    marker = bh_entity_acquire(marker_context->pool);
    if (!marker)
    {
        marker_context->error = BH_HIT_MARKER_POOL;
        return NULL;
    }
    bh_field_store_f32(&marker->marker.age, 0);
    bh_field_store_f32(&marker->marker.lifetime, 30);
    bh_field_store_f32(&marker->marker.size, size);
    bh_field_store_f32(&marker->marker.u, (float)(flags & 1) * .5f);
    bh_field_store_f32(&marker->marker.v, (float)(flags & 2) * .25f);
    memcpy((char *)&marker->marker.normal, &normal, 12);
    dot = ((double)normal.x * 0.0 + (double)normal.z * 0.0) + (double)normal.y;
    if (dot < .5 && dot > -.5)
    {
        up.x = -0.0f;
        up.y = -1;
        up.z = -0.0f;
    }
    else
    {
        up.x = 0;
        up.y = 0;
        up.z = 1;
    }
    memcpy((char *)&marker->marker.up, &up, 12);
    marker->position = position;
    memcpy((char *)&marker->marker.draw, &draw, 4);
    memcpy(&marker->bindings.hit_marker, &marker_context, 4);
    head = owner_head(marker_context, owner);
    (marker->marker.owner = owner);
    prior = *head;
    (marker->marker.owner_newer = NULL);
    (marker->marker.owner_older = prior);
    *head = marker;
    if (prior)
        (prior->marker.owner_newer = marker);
    marker->step = NULL;
    marker->refresh = NULL;
    bh_entity_set_query(marker_context->pool, marker, bh_marker_query);
    if (marker_context->count == 200)
    {
        old = marker_context->oldest;
        marker_context->oldest = old->marker.newer;
        (marker_context->oldest->marker.older = NULL);
        unlink_owner(marker_context, old);
        bh_entity_release(marker_context->pool, old);
    }
    else
        ++marker_context->count;
    if (marker_context->count == 1)
        marker_context->oldest = marker;
    (marker->marker.older = marker_context->newest);
    if (marker_context->newest)
        (marker_context->newest->marker.newer = marker);
    marker_context->newest = marker;
    marker_context->error = BH_HIT_MARKER_OK;
    return marker;
}
void bh_hit_marker_draw(BHEntity *marker)
{
    BHHitMarkerContext *marker_context;
    BHVec3 up, normal;
    float size;
    int result;
    if (!marker)
        return;
    memcpy(&marker_context, &marker->bindings.hit_marker, 4);
    if (!marker_context)
        return;
    size = bh_field_f32(&marker->marker.size);
    if (!(size > 0))
        return;
    memcpy(&up, (char *)&marker->marker.up, 12);
    memcpy(&normal, (char *)&marker->marker.normal, 12);
    result = bh_quad_queue(marker_context->queue, 0, marker_context->image, &marker->position, &up, &normal, size, size,
                           0, 0, 1, bh_field_f32(&marker->marker.u), .5f, bh_field_f32(&marker->marker.v), .5f);
    marker_context->error = result == 1 ? BH_HIT_MARKER_OK : BH_HIT_MARKER_QUEUE;
}
int bh_hit_marker_dispose(BHHitMarkerContext *marker_context)
{
    BHEntity *marker, *next;
    if (!marker_context || !marker_context->pool)
        return 0;
    while (marker_context->count)
    {
        marker = marker_context->oldest;
        if (!marker || !unlink_owner(marker_context, marker))
        {
            marker_context->error = BH_HIT_MARKER_TOPOLOGY;
            return 0;
        }
        next = marker_context->count > 1 ? marker->marker.newer : NULL;
        if (!bh_entity_release(marker_context->pool, marker))
        {
            marker_context->error = BH_HIT_MARKER_POOL;
            return 0;
        }
        marker_context->oldest = next;
        --marker_context->count;
    }
    marker_context->newest = NULL;
    marker_context->error = BH_HIT_MARKER_OK;
    return 1;
}
