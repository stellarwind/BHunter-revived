#include "bh_field_access.h"
#include "bh_hit_marker.h"
#include "bh_callback_helpers.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4, "marker layout");
static BHEntity **owner_head(BHHitMarkerContext *c, BHEntity *owner)
{
    BHEntity **head = NULL;
    if (c->owner_head)
        head = c->owner_head(owner, c->owner_head_user);
    return head ? head : &owner->placement.attachments;
}
static int unlink_owner(BHHitMarkerContext *c, BHEntity *e)
{
    BHEntity *owner = e->marker.owner, *older = e->marker.owner_older, *newer = e->marker.owner_newer;
    if (!owner)
        return 0;
    if (older)
        (older->marker.owner_newer = newer);
    if (newer)
        (newer->marker.owner_older = older);
    else
        *owner_head(c, owner) = older;
    return 1;
}
BHEntity *bh_hit_marker_create(BHHitMarkerContext *c, const BHVec3 *p, const BHVec3 *n, float size, unsigned long flags,
                               BHEntity *owner)
{
    BHEntity *e, *prior, *old, **head;
    BHVec3 normal, position, up;
    BHHitMarkerDraw draw = bh_hit_marker_draw;
    double dot;
    if (!c || !c->pool || !c->queue || !p || !n || !owner || c->count > 200 || !bh_math_environment_supported())
    {
        if (c)
            c->error = BH_HIT_MARKER_ARGUMENT;
        return NULL;
    }
    if ((c->count && (!c->oldest || !c->newest)) || (!c->count && (c->oldest || c->newest)))
    {
        c->error = BH_HIT_MARKER_TOPOLOGY;
        return NULL;
    }
    normal = *n;
    position = *p;
    e = bh_entity_acquire(c->pool);
    if (!e)
    {
        c->error = BH_HIT_MARKER_POOL;
        return NULL;
    }
    bh_field_store_f32(&e->marker.age, 0);
    bh_field_store_f32(&e->marker.lifetime, 30);
    bh_field_store_f32(&e->marker.size, size);
    bh_field_store_f32(&e->marker.u, (float)(flags & 1) * .5f);
    bh_field_store_f32(&e->marker.v, (float)(flags & 2) * .25f);
    memcpy((char *)&e->marker.normal, &normal, 12);
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
    memcpy((char *)&e->marker.up, &up, 12);
    e->position = position;
    memcpy((char *)&e->marker.draw, &draw, 4);
    memcpy(&e->bindings.hit_marker, &c, 4);
    head = owner_head(c, owner);
    (e->marker.owner = owner);
    prior = *head;
    (e->marker.owner_newer = NULL);
    (e->marker.owner_older = prior);
    *head = e;
    if (prior)
        (prior->marker.owner_newer = e);
    e->step = NULL;
    e->refresh = NULL;
    bh_entity_set_query(c->pool, e, bh_marker_query);
    if (c->count == 200)
    {
        old = c->oldest;
        c->oldest = old->marker.newer;
        (c->oldest->marker.older = NULL);
        unlink_owner(c, old);
        bh_entity_release(c->pool, old);
    }
    else
        ++c->count;
    if (c->count == 1)
        c->oldest = e;
    (e->marker.older = c->newest);
    if (c->newest)
        (c->newest->marker.newer = e);
    c->newest = e;
    c->error = BH_HIT_MARKER_OK;
    return e;
}
void bh_hit_marker_draw(BHEntity *e)
{
    BHHitMarkerContext *c;
    BHVec3 up, normal;
    float size;
    int r;
    if (!e)
        return;
    memcpy(&c, &e->bindings.hit_marker, 4);
    if (!c)
        return;
    size = bh_field_f32(&e->marker.size);
    if (!(size > 0))
        return;
    memcpy(&up, (char *)&e->marker.up, 12);
    memcpy(&normal, (char *)&e->marker.normal, 12);
    r = bh_quad_queue(c->queue, 0, c->image, &e->position, &up, &normal, size, size, 0, 0, 1,
                      bh_field_f32(&e->marker.u), .5f, bh_field_f32(&e->marker.v), .5f);
    c->error = r == 1 ? BH_HIT_MARKER_OK : BH_HIT_MARKER_QUEUE;
}
int bh_hit_marker_dispose(BHHitMarkerContext *c)
{
    BHEntity *e, *next;
    if (!c || !c->pool)
        return 0;
    while (c->count)
    {
        e = c->oldest;
        if (!e || !unlink_owner(c, e))
        {
            c->error = BH_HIT_MARKER_TOPOLOGY;
            return 0;
        }
        next = c->count > 1 ? e->marker.newer : NULL;
        if (!bh_entity_release(c->pool, e))
        {
            c->error = BH_HIT_MARKER_POOL;
            return 0;
        }
        c->oldest = next;
        --c->count;
    }
    c->newest = NULL;
    c->error = BH_HIT_MARKER_OK;
    return 1;
}
