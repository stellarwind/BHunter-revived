#include "bh_entity_schedule.h"
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif
#include "bh_draw_submit.h"
#include "bh_draw_controls.h"
#include <float.h>
#include <math.h>
#include <string.h>

/* 00409530's portal use is an untextured convex polygon. The already
   recovered 00452140 owner performs the same six ordered half-space cuts and
   writes the same projected screen coordinates. Keep its scratch on the
   caller's stack because recursive portals need separate rings. */
BHClipVertex *__cdecl bh_entity_schedule_clip_portal(BHClipContext *clip, BHClipVertex vertices[4],
                                                     const BHVec3 projected[4])
{
    unsigned long i;
    if (!clip || !vertices || !projected)
        return NULL;
    memset(vertices, 0, 4 * sizeof(*vertices));
    for (i = 0; i < 4; ++i)
    {
        vertices[i].point = projected[i];
        vertices[i].next = vertices + (i + 1) % 4;
        vertices[i].previous = vertices + (i + 3) % 4;
    }
    return bh_clip_polygon(clip, vertices, NULL);
}
void __cdecl bh_entity_schedule_tick(BHEntityScheduleRoom *room)
{
    if (!room || !room->frame_active)
        return;
    if (room->frame_remaining)
        room->frame_remaining--;
    if (!room->frame_remaining)
        room->frame_active = 0;
    if (room->activity)
        *room->activity = (long)room->frame_remaining;
}
static void __cdecl activity_tick(BHEntity *entity)
{
    BHEntityScheduleRoom *room;
    memcpy(&room, entity->payload + 608, 4);
    if (!room || room->activity_entity != entity)
        return;
    bh_entity_schedule_tick(room);
    memcpy((char *)entity + 0x104, &room->frame_remaining, 4);
    if (!room->frame_active)
        entity->step = NULL;
}
static int fail(BHEntityScheduleContext *c, BHEntityScheduleError e)
{
    if (c)
        c->error = e;
    return 0;
}
static int queued(BHEntityScheduleContext *c, int status)
{
    return status < 0 ? fail(c, BH_SCHEDULE_QUEUE) : 1;
}
static int resource_draw(BHEntityScheduleContext *c, BHEntityScheduleRoom *room, int linked)
{
    unsigned long flags = room->room->world->query_flags, bucket;
    const BHVec3 *point = &room->room->world->position;
    if (!(flags & 4))
        return 1;
    bucket = (flags & 1) ? 3 : 0;
    if (!(flags & 1) && (room->resource_flags & 0x40))
    {
        if (!queued(c, bh_draw_controls_state(c->queue, 0, 1)) ||
            !queued(c, bh_draw_submit_resource(c->queue, 0, room->resource, point, &room->angles, 1)) ||
            !queued(c, bh_draw_controls_state(c->queue, 0, -1)) || !queued(c, bh_draw_controls_state(c->queue, 3, 2)) ||
            !queued(c, bh_draw_submit_resource(c->queue, 3, room->resource, point, &room->angles, 1)) ||
            !queued(c, bh_draw_controls_state(c->queue, 3, -1)))
            return 0;
        if (!linked)
            return 1; /* original root special case jumps past default */
    }
    return queued(c, bh_draw_submit_resource(c->queue, bucket, room->resource, point, &room->angles, 1));
}
static int resource(BHEntityScheduleContext *c, BHEntityScheduleRoom *room, int linked)
{
    int result;
#ifdef BH_SDL_BACKEND
    unsigned long mark = bh_interpolation_mark();
#endif
    result = resource_draw(c, room, linked);
#ifdef BH_SDL_BACKEND
    bh_interpolation_stamp(mark, (unsigned long)room->room->world, 0);
#endif
    return result;
}
static BHEntity *actor_next(BHEntity *e)
{
    BHEntity *next;
    memcpy(&next, e->payload + 16, sizeof(next));
    return next;
}
static BHEntityStep draw_method(BHEntity *e)
{
    BHEntityStep method;
    memcpy(&method, e->payload + 4, sizeof(method));
    return method;
}
static int draw_children(BHEntityScheduleContext *c, BHEntity *head, int pickups)
{
    unsigned long count = 0;
    BHEntity *e = head, *next;
    BHEntityStep draw;
#ifdef BH_SDL_BACKEND
    unsigned long mark, identity, generation;
#endif
    while (e)
    {
        if (++count > c->max_entities)
            return fail(c, BH_SCHEDULE_TOPOLOGY);
        draw = draw_method(e);
        if (!draw)
            return fail(c, BH_SCHEDULE_ARGUMENT);
#ifdef BH_SDL_BACKEND
        mark = bh_interpolation_mark();
        identity = (unsigned long)e;
        generation = e->generation;
#endif
        draw(e); /* original +e8 indirect method */
#ifdef BH_SDL_BACKEND
        bh_interpolation_stamp(mark, identity, generation);
#endif
        next = pickups ? bh_pickup_state(e)->room_next : actor_next(e);
        e = next; /* deliberately read after callback: mutation is observable */
    }
    return 1;
}
static int ghost_draw(BHEntityScheduleContext *c, BHEntityScheduleRoom *r)
{
    BHWorldGeometry *w = r->room->world;
    BHVec3 center, point;
    float distance, blend, alpha;
    const BHVec3 *eye = c->overlay_position;
    double dx, dy, dz;
    if (!(w->query_flags & 4) || !(w->query_flags & 0x180))
        return 1;
    if (!eye)
        return fail(c, BH_SCHEDULE_ARGUMENT);
    center.x = (w->minimum.x + w->maximum.x) * .5f;
    center.y = (w->query_flags & 0x80) ? w->maximum.y : w->minimum.y;
    center.z = (w->minimum.z + w->maximum.z) * .5f;
    dx = (double)center.x - eye->x;
    dy = (double)center.y - eye->y;
    dz = (double)center.z - eye->z;
    distance = (float)sqrt(dx * dx + dy * dy + dz * dz);
    if (!(30.0f < distance && distance < 180.0f))
        return 1;
    blend = 1.0f - 30.0f / distance;
    alpha = 1.0f;
    if (distance < 60.0f)
        alpha = (distance - 30.0f) * (1.0f / 30.0f);
    else if (150.0f < distance)
        alpha = 1.0f - (distance - 150.0f) * (1.0f / 30.0f);
    point.x = eye->x * blend + center.x * (1.0f - blend) - (center.x - w->position.x);
    point.y = eye->y * blend + center.y * (1.0f - blend) - (center.y - w->position.y);
    point.z = eye->z * blend + center.z * (1.0f - blend) - (center.z - w->position.z);
    return queued(c, bh_draw_submit_resource(c->queue, 3, r->resource, &point, &r->angles, alpha));
}
static int ghost(BHEntityScheduleContext *c, BHEntityScheduleRoom *r)
{
    int result;
#ifdef BH_SDL_BACKEND
    unsigned long mark = bh_interpolation_mark();
#endif
    result = ghost_draw(c, r);
#ifdef BH_SDL_BACKEND
    bh_interpolation_stamp(mark, (unsigned long)r->room->world, 1);
#endif
    return result;
}
static int faces_camera(const BHPortal *p, const BHVec3 *position)
{
    double plane = (double)p->plane; /* original FILD keeps integer precision */
    switch (p->direction)
    {
    case 0:
        return plane < position->x;
    case 1:
        return plane < position->y;
    case 2:
        return plane < position->z;
    case 4:
        return position->x < plane;
    case 5:
        return position->y < plane;
    case 6:
        return position->z < plane;
    default:
        return 0;
    }
}
static void portal_quad(const BHPortal *p, BHVec3 q[4])
{
    float plane = (float)p->plane, a = p->overlap[0], b = p->overlap[1], lo = p->overlap[2], hi = p->overlap[3];
    unsigned long i;
    for (i = 0; i < 4; ++i)
    {
        float u = (i == 0 || i == 3) ? a : b, v = i < 2 ? lo : hi;
        if (p->direction == 0 || p->direction == 4)
        {
            q[i].x = plane;
            q[i].y = u;
            q[i].z = v;
        }
        else if (p->direction == 1 || p->direction == 5)
        {
            q[i].x = u;
            q[i].y = plane;
            q[i].z = v;
        }
        else
        {
            q[i].x = u;
            q[i].y = v;
            q[i].z = plane;
        }
    }
}
static int schedule(BHEntityScheduleContext *c, BHEntityScheduleRoom *r, unsigned long depth, unsigned long parent_clip)
{
    BHEntityScheduleRoom *other;
    BHWorldGeometry *world;
    BHPortal *portal;
    BHClipContext clip;
    BHClipVertex input[4], *ring, *v;
    BHVec3 corners[4], projected[4];
    BHViewConfig saved;
    float left, right, top, bottom;
    unsigned long i, j, n, child_clip;
    if (depth > c->max_depth)
        return fail(c, BH_SCHEDULE_DEPTH);
    if (!r || !r->room || !r->room->world)
        return fail(c, BH_SCHEDULE_ARGUMENT);
    world = r->room->world;
    if (!resource(c, r, 0))
        return 0;
    r->frame_active = 1;
    r->frame_remaining = 30;
    if (r->activity)
        *r->activity = 30;
    if (r->activity_entity)
    {
        memcpy(r->activity_entity->payload + 608, &r, 4);
        memcpy((char *)r->activity_entity + 0x104, &r->frame_remaining, 4);
        r->activity_entity->step = activity_tick;
    }
    if (!r->frame_seen || r->last_frame != c->frame)
    {
        r->last_frame = c->frame;
        r->frame_seen = 1;
        if (!draw_children(c, r->room->actors, 0))
            return 0;
        if ((world->query_flags & 4) && !draw_children(c, r->room->pickups, 1))
            return 0;
    }
    other = r->next;
    n = 0;
    while (other)
    {
        if (++n > c->max_rooms)
            return fail(c, BH_SCHEDULE_TOPOLOGY);
        if (!other->linked_active)
            break;
        if (!other->room || !other->room->world)
            return fail(c, BH_SCHEDULE_TOPOLOGY);
        if (!resource(c, other, 1) || !ghost(c, other))
            return 0;
        if ((other->room->world->query_flags & 4) && !draw_children(c, other->room->pickups, 1))
            return 0;
        other = other->next;
    }
    if (world->navigation.count > 6)
        return fail(c, BH_SCHEDULE_TOPOLOGY);
    for (i = 0; i < world->navigation.count; ++i)
    {
        portal = world->navigation.portals + i;
        other = r->portal_targets[i];
        if (!portal->neighbor || !other || !faces_camera(portal, &c->camera->position))
            continue;
        if (portal->neighbor && other->room && other->room->world &&
            portal->neighbor != &other->room->world->navigation)
            return fail(c, BH_SCHEDULE_TOPOLOGY);
        portal_quad(portal, corners);
        for (j = 0; j < 4; ++j)
            bh_mat34_transform_point(&c->camera->projected, corners + j, projected + j);
        if (!bh_clip_init(&clip, c->viewport->near_z, c->viewport->far_z, c->viewport->left, c->viewport->right,
                          c->viewport->top, c->viewport->bottom))
            return fail(c, BH_SCHEDULE_CLIP);
        ring = bh_entity_schedule_clip_portal(&clip, input, projected);
        if (!ring)
        {
            if (clip.error != BH_CLIP_OK)
                return fail(c, BH_SCHEDULE_CLIP);
            continue;
        }
        left = top = 9e37f;
        right = bottom = -9e37f;
        v = ring;
        do
        {
            if (v->sx < left)
                left = v->sx;
            if (right < v->sx)
                right = v->sx;
            if (v->sy < top)
                top = v->sy;
            if (bottom < v->sy)
                bottom = v->sy;
            v = v->next;
        } while (v != ring);
        saved = *c->viewport;
        c->viewport->left = left;
        c->viewport->right = right;
        c->viewport->top = top;
        c->viewport->bottom = bottom;
        child_clip = c->queue->count;
#ifdef BH_SDL_BACKEND
        if (!queued(c, bh_draw_controls_portal_clip(c->queue, 0, corners, parent_clip, c->viewport->near_z,
                                                    c->viewport->far_z)) ||
            !queued(c, bh_draw_controls_portal_clip(c->queue, 3, corners, parent_clip, c->viewport->near_z,
                                                    c->viewport->far_z)))
#else
        if (!queued(c, bh_draw_controls_clip_rect(c->queue, 0, left, top, right - left, bottom - top)) ||
            !queued(c, bh_draw_controls_clip_rect(c->queue, 3, left, top, right - left, bottom - top)))
#endif
        {
            *c->viewport = saved;
            return 0;
        }
        j = schedule(c, other, depth + 1, child_clip);
        *c->viewport = saved;
        if (!j)
            return 0;
    }
    return 1;
}
int __cdecl bh_entity_schedule(BHEntityScheduleContext *c, BHEntityScheduleRoom *room)
{
    if (!c || !c->queue || !c->viewport || !c->camera || !room || !c->max_rooms || !c->max_entities || !c->max_depth)
        return fail(c, BH_SCHEDULE_ARGUMENT);
    c->error = BH_SCHEDULE_OK;
    return schedule(c, room, 0, BH_DRAW_END);
}
