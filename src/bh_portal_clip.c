#include "bh_portal_clip.h"
#include "bh_clipper.h"
#include <string.h>
static int project(const BHDrawQueue *q, unsigned long at, const BHMainCamera *camera, const BHViewConfig *view,
                   BHPortalClipCache *cache, float bounds[4], unsigned long depth)
{
    const BHDrawCommand *command;
    BHVec3 corners[4];
    BHClipVertex vertices[4], *ring, *p;
    BHClipContext clip;
    unsigned long i, parent;
    float near_z, far_z, enclosing[4];
    if (!q || !q->commands || at >= q->count || !camera || !view || depth > 64)
        return 0;
    command = q->commands + at;
    if (command->kind != BH_DRAW_CLIP_RECT || command->resource != BH_PORTAL_CLIP_WORLD)
        return 0;
    if (cache && at < BH_PORTAL_CLIP_CAPACITY && cache->ready[at])
    {
        memcpy(bounds, cache->bounds[at], 4 * sizeof(float));
        return 1;
    }
    parent = command->words[12];
    if (parent != BH_DRAW_END)
    {
        /* A strict backward link bounds recursion and rejects cycles. */
        if (parent >= at || !project(q, parent, camera, view, cache, enclosing, depth + 1))
            return 0;
    }
    else
    {
        enclosing[0] = -view->margin_x;
        enclosing[1] = -view->margin_y;
        enclosing[2] = view->width + view->margin_x;
        enclosing[3] = view->height + view->margin_y;
    }
    memset(bounds, 0, 4 * sizeof(float));
    if (enclosing[0] < enclosing[2] && enclosing[1] < enclosing[3])
    {
        memcpy(corners, command->words, sizeof(corners));
        memcpy(&near_z, command->words + 13, 4);
        memcpy(&far_z, command->words + 14, 4);
        if (!bh_clip_init(&clip, near_z, far_z, enclosing[0], enclosing[2], enclosing[1], enclosing[3]))
            return 0;
        memset(vertices, 0, sizeof(vertices));
        for (i = 0; i < 4; ++i)
        {
            bh_mat34_transform_point(&camera->projected, corners + i, &vertices[i].point);
            vertices[i].next = vertices + (i + 1) % 4;
            vertices[i].previous = vertices + (i + 3) % 4;
        }
        ring = bh_clip_polygon(&clip, vertices, 0);
        if (!ring && clip.error != BH_CLIP_OK)
            return 0;
        if (ring)
        {
            bounds[0] = bounds[1] = 9e37f;
            bounds[2] = bounds[3] = -9e37f;
            p = ring;
            do
            {
                if (p->sx < bounds[0])
                    bounds[0] = p->sx;
                if (p->sy < bounds[1])
                    bounds[1] = p->sy;
                if (bounds[2] < p->sx)
                    bounds[2] = p->sx;
                if (bounds[3] < p->sy)
                    bounds[3] = p->sy;
                p = p->next;
            } while (p != ring);
        }
    }
    if (cache && at < BH_PORTAL_CLIP_CAPACITY)
    {
        memcpy(cache->bounds[at], bounds, 4 * sizeof(float));
        cache->ready[at] = 1;
        ++cache->projections;
    }
    return 1;
}
void __cdecl bh_portal_clip_reset(BHPortalClipCache *cache)
{
    if (cache)
    {
        memset(cache->ready, 0, sizeof(cache->ready));
        cache->projections = 0;
    }
}
int __cdecl bh_portal_clip_project(const BHDrawQueue *q, unsigned long at, const BHMainCamera *camera,
                                   const BHViewConfig *view, BHPortalClipCache *cache, float bounds[4])
{
    return bounds && project(q, at, camera, view, cache, bounds, 0);
}
