#include "bh_entity_schedule.h"
#ifdef BH_SDL_BACKEND
#include "../../Graphics/bh_render_interpolation.h"
#endif
#include "../../Graphics/bh_draw_submit.h"
#include "../../Graphics/bh_draw_controls.h"
#include "../../Math/bh_round.h"
#include "../../World/bh_portal_visibility.h"
#include <float.h>
#include <math.h>
#include <string.h>

/* 00409530's portal use is an untextured convex polygon. The already
   recovered 00452140 owner performs the same six ordered half-space cuts and
   writes the same projected screen coordinates. Keep its scratch on the
   caller's stack because recursive portals need separate rings. */
BHClipVertex *bh_entity_schedule_clip_portal(BHClipContext *clip, BHClipVertex vertices[4], const BHVec3 projected[4])
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
void bh_entity_schedule_tick(BHEntityScheduleRoom *room)
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
static void activity_tick(BHEntity *entity)
{
    BHEntityScheduleRoom *room;
    memcpy(&room, &entity->bindings.room_activity, 4);
    if (!room || room->activity_entity != entity)
        return;
    bh_entity_schedule_tick(room);
    memcpy(&entity->placement.activity_frames, &room->frame_remaining, 4);
    if (!room->frame_active)
        entity->step = NULL;
}
static int fail(BHEntityScheduleContext *context, BHEntityScheduleError e)
{
    if (context)
        context->error = e;
    return 0;
}
static int queued(BHEntityScheduleContext *c, int status)
{
    return status < 0 ? fail(c, BH_SCHEDULE_QUEUE) : 1;
}
static int resource_draw(BHEntityScheduleContext *context, BHEntityScheduleRoom *room, int linked)
{
    unsigned long flags = room->room->world->query_flags, bucket;
    const BHVec3 *point = &room->room->world->position;
    if (!(flags & 4))
        return 1;
    bucket = (flags & 1) ? 3 : 0;
    if (!(flags & 1) && (room->resource_flags & 0x40))
    {
        if (!queued(context, bh_draw_controls_state(context->queue, 0, 1)) ||
            !queued(context, bh_draw_submit_resource(context->queue, 0, room->resource, point, &room->angles, 1)) ||
            !queued(context, bh_draw_controls_state(context->queue, 0, -1)) || !queued(context, bh_draw_controls_state(context->queue, 3, 2)) ||
            !queued(context, bh_draw_submit_resource(context->queue, 3, room->resource, point, &room->angles, 1)) ||
            !queued(context, bh_draw_controls_state(context->queue, 3, -1)))
            return 0;
        if (!linked)
            return 1; /* original root special case jumps past default */
    }
    return queued(context, bh_draw_submit_resource(context->queue, bucket, room->resource, point, &room->angles, 1));
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
static BHEntityStep draw_method(BHEntity *actor)
{
    BHEntityStep method;
    memcpy(&method, &actor->actor.draw, sizeof(method));
    return method;
}
static int draw_children(BHEntityScheduleContext *context, BHEntity *head, int pickups)
{
    unsigned long count = 0;
    BHEntity *actor = head, *next;
    BHEntityStep draw;
#ifdef BH_SDL_BACKEND
    unsigned long mark, identity, generation;
#endif
    while (actor)
    {
        if (++count > context->max_entities)
            return fail(context, BH_SCHEDULE_TOPOLOGY);
        draw = draw_method(actor);
        if (!draw)
            return fail(context, BH_SCHEDULE_ARGUMENT);
#ifdef BH_SDL_BACKEND
        mark = bh_interpolation_mark();
        identity = (unsigned long)actor;
        generation = actor->generation;
#endif
        draw(actor); /* original +e8 indirect method */
#ifdef BH_SDL_BACKEND
        bh_interpolation_stamp(mark, identity, generation);
#endif
        next = pickups ? bh_pickup_state(actor)->room_next : actor->actor.room_next;
        actor = next; /* deliberately read after callback: mutation is observable */
    }
    return 1;
}
static int ghost_draw(BHEntityScheduleContext *context, BHEntityScheduleRoom *entity_schedule_room)
{
    BHWorldGeometry *world = entity_schedule_room->room->world;
    volatile BHVec3 center;
    BHVec3 point;
    float distance;
    volatile float blend, alpha;
    volatile float dx32, dy32, offset_x, offset_y, offset_z;
    volatile float weighted_x, weighted_y, weighted_z, blended_x, blended_y;
    const BHVec3 *eye = context->overlay_position;
    double dx, dy, dz;
    if (!(world->query_flags & 4) || !(world->query_flags & 0x180))
        return 1;
    if (!eye)
        return fail(context, BH_SCHEDULE_ARGUMENT);
    center.x = (world->minimum.x + world->maximum.x) * .5f;
    center.y = (world->query_flags & 0x80) ? world->maximum.y : world->minimum.y;
    center.z = (world->minimum.z + world->maximum.z) * .5f;
    dx = (double)center.x - eye->x;
    dy = (double)center.y - eye->y;
    dz = (double)center.z - eye->z;
    /* 0040a271/277 store without popping: multiply retained deltas by
       their binary32 spills. The Z delta remains entirely in x87. */
    dx32 = (float)dx;
    dy32 = (float)dy;
    bh_round_float(&distance, sqrt((dx * dx32 + dy * dy32) + dz * dz));
    if (!(30.0f < distance && distance < 180.0f))
        return 1;
    blend = 1.0f - 30.0f / distance;
    alpha = 1.0f;
    if (distance < 60.0f)
        alpha = (distance - 30.0f) * (1.0f / 30.0f);
    else if (150.0f < distance)
        alpha = 1.0f - (distance - 150.0f) * (1.0f / 30.0f);
    /* Preserve the original spill graph, including its asymmetric Z sum:
       X/Y sums reload binary32, while Z subtracts before its final store. */
    offset_x = center.x - world->position.x;
    offset_y = center.y - world->position.y;
    offset_z = center.z - world->position.z;
    weighted_x = (float)((double)center.x * (1.0 - blend));
    weighted_y = (float)((double)center.y * (1.0 - blend));
    weighted_z = (float)((double)center.z * (1.0 - blend));
    blended_x = (float)((double)eye->x * blend + weighted_x);
    blended_y = (float)((double)eye->y * blend + weighted_y);
    point.x = blended_x - offset_x;
    point.y = blended_y - offset_y;
    point.z = (float)(((double)eye->z * blend + weighted_z) - offset_z);
    return queued(context, bh_draw_submit_resource(context->queue, 3, entity_schedule_room->resource, &point, &entity_schedule_room->angles, alpha));
}
static int ghost(BHEntityScheduleContext *c, BHEntityScheduleRoom *entity_schedule_room)
{
    int result;
#ifdef BH_SDL_BACKEND
    unsigned long mark = bh_interpolation_mark();
#endif
    result = ghost_draw(c, entity_schedule_room);
#ifdef BH_SDL_BACKEND
    bh_interpolation_stamp(mark, (unsigned long)entity_schedule_room->room->world, 1);
#endif
    return result;
}
static int faces_camera(const BHPortal *portal, const BHVec3 *position)
{
    double plane = (double)portal->plane; /* original FILD keeps integer precision */
    switch (portal->direction)
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
static void portal_quad(const BHPortal *portal, BHVec3 q[4])
{
    float plane = (float)portal->plane, a = portal->overlap[0], b = portal->overlap[1], lo = portal->overlap[2], hi = portal->overlap[3];
    unsigned long i;
    for (i = 0; i < 4; ++i)
    {
        float u = (i == 0 || i == 3) ? a : b, v = i < 2 ? lo : hi;
        if (portal->direction == 0 || portal->direction == 4)
        {
            q[i].x = plane;
            q[i].y = u;
            q[i].z = v;
        }
        else if (portal->direction == 1 || portal->direction == 5)
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
typedef struct BHPortalPath
{
    BHEntityScheduleRoom *room;
    const struct BHPortalPath *previous;
} BHPortalPath;

static int schedule(BHEntityScheduleContext *context, BHEntityScheduleRoom *entity_schedule_room, unsigned long depth, unsigned long parent_clip,
                    const BHPortalPath *previous, double near_radius_squared)
{
    BHEntityScheduleRoom *other;
    BHWorldGeometry *world;
    BHPortal *portal;
    BHClipContext clip;
    BHClipVertex input[4], *ring, *vertex;
    BHVec3 corners[4], projected[4];
    BHViewConfig saved;
    float left, right, top, bottom;
    unsigned long i, j, n, child_clip;
    BHPortalPath path;
#ifdef BH_SDL_BACKEND
    const BHPortalPath *ancestor;
    int near_camera;
#endif
    if (depth > context->max_depth)
        return fail(context, BH_SCHEDULE_DEPTH);
    if (!entity_schedule_room || !entity_schedule_room->room || !entity_schedule_room->room->world)
        return fail(context, BH_SCHEDULE_ARGUMENT);
    path.room = entity_schedule_room;
    path.previous = previous;
    world = entity_schedule_room->room->world;
    if (!resource(context, entity_schedule_room, 0))
        return 0;
    entity_schedule_room->frame_active = 1;
    entity_schedule_room->frame_remaining = 30;
    if (entity_schedule_room->activity)
        *entity_schedule_room->activity = 30;
    if (entity_schedule_room->activity_entity)
    {
        memcpy(&entity_schedule_room->activity_entity->bindings.room_activity, &entity_schedule_room, 4);
        memcpy(&entity_schedule_room->activity_entity->placement.activity_frames, &entity_schedule_room->frame_remaining, 4);
        entity_schedule_room->activity_entity->step = activity_tick;
    }
    if (!entity_schedule_room->frame_seen || entity_schedule_room->last_frame != context->frame)
    {
        entity_schedule_room->last_frame = context->frame;
        entity_schedule_room->frame_seen = 1;
        if (!draw_children(context, entity_schedule_room->room->actors, 0))
            return 0;
        if ((world->query_flags & 4) && !draw_children(context, entity_schedule_room->room->pickups, 1))
            return 0;
    }
    other = entity_schedule_room->next;
    n = 0;
    while (other)
    {
        if (++n > context->max_rooms)
            return fail(context, BH_SCHEDULE_TOPOLOGY);
        if (!other->linked_active)
            break;
        if (!other->room || !other->room->world)
            return fail(context, BH_SCHEDULE_TOPOLOGY);
        if (!resource(context, other, 1) || !ghost(context, other))
            return 0;
        if ((other->room->world->query_flags & 4) && !draw_children(context, other->room->pickups, 1))
            return 0;
        other = other->next;
    }
    if (world->navigation.count > 6)
        return fail(context, BH_SCHEDULE_TOPOLOGY);
    for (i = 0; i < world->navigation.count; ++i)
    {
        portal = world->navigation.portals + i;
        other = entity_schedule_room->portal_targets[i];
        if (!portal->neighbor || !other)
            continue;
#ifdef BH_SDL_BACKEND
        /* Both render buckets need a portal window. A full queue may drop
           commands normally; never stamp a window that was not allocated. */
        if (context->queue->count >= context->queue->capacity || context->queue->capacity - context->queue->count < 2)
            continue;
#endif
        if (portal->neighbor && other->room && other->room->world &&
            portal->neighbor != &other->room->world->navigation)
            return fail(context, BH_SCHEDULE_TOPOLOGY);
        portal_quad(portal, corners);
#ifdef BH_SDL_BACKEND
        for (ancestor = &path; ancestor; ancestor = ancestor->previous)
            if (ancestor->room == other)
                break;
        if (ancestor)
            continue;
        near_camera = bh_portal_camera_touches(corners, &context->camera->position, near_radius_squared);
        if (near_camera)
        {
            left = context->viewport->left;
            right = context->viewport->right;
            top = context->viewport->top;
            bottom = context->viewport->bottom;
        }
        else
#endif
        {
            if (!faces_camera(portal, &context->camera->position))
                continue;
            for (j = 0; j < 4; ++j)
                bh_mat34_transform_point(&context->camera->projected, corners + j, projected + j);
            if (!bh_clip_init(&clip, context->viewport->near_z, context->viewport->far_z, context->viewport->left, context->viewport->right,
                              context->viewport->top, context->viewport->bottom))
                return fail(context, BH_SCHEDULE_CLIP);
            ring = bh_entity_schedule_clip_portal(&clip, input, projected);
            if (!ring)
            {
                if (clip.error != BH_CLIP_OK)
                    return fail(context, BH_SCHEDULE_CLIP);
                continue;
            }
            left = top = 9e37f;
            right = bottom = -9e37f;
            vertex = ring;
            do
            {
                if (vertex->sx < left)
                    left = vertex->sx;
                if (right < vertex->sx)
                    right = vertex->sx;
                if (vertex->sy < top)
                    top = vertex->sy;
                if (bottom < vertex->sy)
                    bottom = vertex->sy;
                vertex = vertex->next;
            } while (vertex != ring);
        }
        saved = *context->viewport;
        context->viewport->left = left;
        context->viewport->right = right;
        context->viewport->top = top;
        context->viewport->bottom = bottom;
        child_clip = context->queue->count;
#ifdef BH_SDL_BACKEND
        if (!queued(context, bh_draw_controls_portal_clip(context->queue, 0, corners, parent_clip, context->viewport->near_z,
                                                    context->viewport->far_z)) ||
            !queued(context, bh_draw_controls_portal_clip(context->queue, 3, corners, parent_clip, context->viewport->near_z,
                                                    context->viewport->far_z)))
#else
        if (!queued(context, bh_draw_controls_clip_rect(context->queue, 0, left, top, right - left, bottom - top)) ||
            !queued(context, bh_draw_controls_clip_rect(context->queue, 3, left, top, right - left, bottom - top)))
#endif
        {
            *context->viewport = saved;
            return 0;
        }
#ifdef BH_SDL_BACKEND
        memcpy(context->queue->commands[child_clip].words + 15, &context->portal_camera_motion, 4);
        memcpy(context->queue->commands[child_clip + 1].words + 15, &context->portal_camera_motion, 4);
#endif
        j = schedule(context, other, depth + 1, child_clip, &path, near_radius_squared);
        *context->viewport = saved;
        if (!j)
            return 0;
    }
    return 1;
}
int bh_entity_schedule(BHEntityScheduleContext *context, BHEntityScheduleRoom *room)
{
    double radius = 0;
    if (!context || !context->queue || !context->viewport || !context->camera || !room || !context->max_rooms || !context->max_entities || !context->max_depth)
        return fail(context, BH_SCHEDULE_ARGUMENT);
    context->error = BH_SCHEDULE_OK;
#ifdef BH_SDL_BACKEND
    {
        float bounds[4];
        bounds[0] = context->viewport->left;
        bounds[1] = context->viewport->top;
        bounds[2] = context->viewport->right;
        bounds[3] = context->viewport->bottom;
        radius = bh_portal_camera_radius(&context->camera->projected, context->viewport->near_z, bounds, context->portal_camera_motion);
    }
#endif
    return schedule(context, room, 0, BH_DRAW_END, NULL, radius);
}
