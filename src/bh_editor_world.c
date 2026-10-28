#include "bh_editor_world.h"
#include "bh_camera_queue.h"
#include "bh_horizon_queue.h"
#include "bh_gdi_text.h"
#include "bh_draw_submit.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static unsigned long word(BHEntity *e, unsigned long o)
{
    unsigned long v;
    memcpy(&v, (char *)e + o, 4);
    return v;
}
static void setword(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static float number(BHEntity *e, unsigned long o)
{
    float f;
    memcpy(&f, (char *)e + o, 4);
    return f;
}
static void real(BHEntity *e, unsigned long o, float v)
{
    memcpy((char *)e + o, &v, 4);
}
static long signed_word(unsigned long x)
{
    long n;
    memcpy(&n, &x, 4);
    return n;
}
static int live(BHEditorWorld *c, BHEntity *e)
{
    return bh_entity_is_active(c->pool, e);
}
static int member(BHEditorWorld *c, const BHEditorNode *n)
{
    unsigned long i;
    for (i = 0; i < c->count; ++i)
        if (c->nodes[i] == n)
            return 1;
    return 0;
}
static int prototype_valid(BHEditorWorld *c, const BHEditorNode *n)
{
    unsigned long i;
    if (member(c, n))
        return live(c, n->entity);
    if (!c->clipboard || c->clipboard_count < 0 || (unsigned long)c->clipboard_count > c->clipboard_capacity)
        return 0;
    for (i = 0; i < (unsigned long)c->clipboard_count; ++i)
        if (n == &c->clipboard[i].node)
            return n->context == c && n->entity == &c->clipboard[i].entity;
    return 0;
}
static BHEditorNode *geometry_node(BHEditorWorld *c, BHWorldGeometry *w)
{
    BHEditorNode *n = (BHEditorNode *)w;
    return member(c, n) ? n : 0;
}
static void refresh(BHEntity *e, void *packet)
{
    BHEditorNode *n;
    (void)packet;
    memcpy(&n, e->payload + 560, 4);
    if (n)
        bh_editor_world_draw(n->context, n);
}
BHEditorNode *__cdecl bh_editor_world_node(BHEntity *e)
{
    BHEditorNode *n;
    if (!e)
        return 0;
    memcpy(&n, e->payload + 560, 4);
    return n;
}
BHEntityCallback __cdecl bh_editor_world_draw_callback(void)
{
    return refresh;
}
int __cdecl bh_editor_world_attach(BHEditorWorld *c, BHEditorNode *n)
{
    if (!c || !c->pool || !c->library || !c->nodes || c->count >= c->capacity || !n || !n->entity ||
        !live(c, n->entity) || n->model_index >= c->library->count || member(c, n))
        return 0;
    n->context = c;
    n->heap_owned = 0;
    memcpy(n->entity->payload + 560, &n, 4);
    n->entity->refresh = refresh;
    c->nodes[c->count++] = n;
    return 1;
}
static BHVec3 center(BHEditorNode *n)
{
    BHVec3 v;
    BHWorldGeometry *w = &n->placement.geometry;
    v.x = stored(((double)w->minimum.x + w->maximum.x) * .5);
    v.y = stored(((double)w->minimum.y + w->maximum.y) * .5);
    v.z = stored(((double)w->minimum.z + w->maximum.z) * .5);
    return v;
}
static int segment(BHEditorWorld *c, unsigned long bucket, unsigned long image, const BHVec3 *a, const BHVec3 *b)
{
    if (bh_draw_segment(c->queue, bucket, image, a, b, .5f, .5f, 1, 1, 0) != 1)
    {
        c->error = 2;
        return 0;
    }
    return 1;
}
static int text(BHEditorWorld *c, short x, short y, const char *message, unsigned long color)
{
    if (bh_gdi_text_queue(c->queue, x, y, message, color) != 1)
    {
        c->error = 2;
        return 0;
    }
    return 1;
}
static int portal_plane(BHEditorWorld *c, const BHPortal *p, const float *uv, unsigned long image)
{
    BHVec3 point, up = {0, 0, 1}, normal = {0, 0, 0};
    float u = stored(((double)uv[0] + uv[1]) * .5), v = stored(((double)uv[2] + uv[3]) * .5);
    switch (p->direction)
    {
    case 0:
    case 4:
        point.x = (float)p->plane;
        point.y = u;
        point.z = v;
        normal.x = 1;
        break;
    case 1:
    case 5:
        point.x = u;
        point.y = (float)p->plane;
        point.z = v;
        normal.y = 1;
        break;
    case 2:
    case 6:
        point.x = u;
        point.y = v;
        point.z = (float)p->plane;
        up.y = 1;
        up.z = 0;
        normal.z = 1;
        break;
    default:
        c->error = 1;
        return 0;
    }
    if (bh_draw_oriented(c->queue, 3, image, &point, &up, &normal, stored((double)uv[1] - uv[0]),
                         stored((double)uv[3] - uv[2]), 0, 8, .5f) != 1)
    {
        c->error = 2;
        return 0;
    }
    return 1;
}
int __cdecl bh_editor_world_draw(BHEditorWorld *c, BHEditorNode *n)
{
    BHWorldGeometry *w;
    BHScriptBlock *b;
    BHEditorNode *owner, *next;
    BHVec3 at, selected, from, to, eye, angles, up, vertices[4];
    float distance, modulation, yaw, pitch, radius;
    double dx, dy, dz;
    long text_y = 15, attachments;
    unsigned long i, j, bucket;
    short value;
    char buffer[128];
    static const BHVec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    /* Original0040d140 checks004df250 before touching the entity. Keep this
    before source ownership scans: normal gameplay refreshes every placement. */
    if (c && c->mode != 1)
    {
        c->error = 0;
        return 1;
    }
    if (!c || !c->pool || !c->library || !c->queue || !n || !member(c, n) || !live(c, n->entity) || !c->selection ||
        !member(c, c->selection) || n->model_index >= c->library->count || !bh_math_environment_supported())
    {
        if (c)
            c->error = 1;
        return 0;
    }
    c->error = 0;
    if (c->mode != 1)
        return 1;
    if (c->catalog_open)
    {
        if (n != c->selection)
            return 1;
        if (!c->browser || !bh_script_block_browser(c->browser))
        {
            c->error = 2;
            return 0;
        }
        return 1;
    }
    w = &n->placement.geometry;
    b = c->library->blocks + n->model_index;
    at = center(n);
    selected = center(c->selection);
    owner = w->owner ? geometry_node(c, w->owner) : 0;
    if (w->owner && !owner)
    {
        c->error = 1;
        return 0;
    }
    if (n != c->selection)
    {
        dx = (double)at.x - ((double)selected.x + c->camera_offset.x);
        dy = (double)at.y - ((double)selected.y + c->camera_offset.y);
        dz = (double)at.z - ((double)selected.z + c->camera_offset.z);
        from.x = stored(dx);
        from.y = stored(dy);
        from.z = stored(dz);
        distance = stored(sqrt((double)from.x * from.x + (double)from.y * from.y + (double)from.z * from.z));
        radius = number(c->selection->entity, 0x13c);
        if (400.0 - (double)radius * (-.5) < distance)
            return 1;
        if (200.0 - (double)radius * (-.25) < distance && owner)
            return 1;
    }
    if (owner)
    {
        to = center(owner);
        if (!segment(c, 3, c->selection_image, &at, &to))
            return 0;
    }
    bucket = (word(n->entity, 0x114) & 1) ? 3 : 0;
    modulation = n == c->selection ? stored(.75 - sin((double)number(n->entity, 0x130)) * (-.25)) : 1;
    memcpy(&angles, (char *)n->entity + 0x3c, 12);
    if (bh_draw_submit_resource(c->queue, bucket, b->resource, &n->entity->position, &angles, modulation) != 1)
    {
        c->error = 2;
        return 0;
    }
    if (n != c->selection && w != c->selection->placement.geometry.owner)
        return 1;
    if (n == c->selection)
    {
        yaw = number(n->entity, 0x134);
        pitch = number(n->entity, 0x138);
        radius = number(n->entity, 0x13c);
        if (!_finite(yaw) || !_finite(pitch) || !_finite(radius))
        {
            c->error = 1;
            return 0;
        }
        at.x = stored((double)at.x + c->camera_offset.x);
        at.y = stored((double)at.y + c->camera_offset.y);
        at.z = stored((double)at.z + c->camera_offset.z);
        eye.x = stored(-(double)radius * sin(yaw) * cos(pitch) + at.x);
        eye.y = stored((double)stored((double)radius * sin(pitch)) + at.y);
        eye.z = stored(cos(yaw) * (-(double)radius) * cos(pitch) + at.z);
        angles.x = yaw;
        angles.y = pitch;
        angles.z = 0;
        if (bh_camera_queue_pose(c->queue, &eye, &angles) != 1 ||
            bh_horizon_queue(c->queue, c->horizon_image, 5000, 5000, 25000, 12500, 0x804000) != 1)
        {
            c->error = 2;
            return 0;
        }
        if (c->axes_enabled)
            for (i = 0; i < 3; ++i)
            {
                for (j = 0; j < 3; ++j)
                {
                    ((float *)&from)[j] = stored((double)((float *)&at)[j] - ((float *)(axes + i))[j] * (-10.0));
                    ((float *)&to)[j] = stored((double)((float *)&at)[j] - ((float *)(axes + i))[j] * 10.0);
                }
                if (!segment(c, 3, c->line_image, &from, &to))
                    return 0;
            }
    }
    if (!b->model.portal_count && n == c->selection)
    {
        sprintf(buffer, owner ? "Attached model: %s" : "Floating model: %s", b->definition.name);
        if (!text(c, 0, 15, buffer, 0xff00))
            return 0;
        text_y = 30;
    }
    if (b->model.portal_count)
    {
        next = geometry_node(c, (BHWorldGeometry *)w->navigation.next);
        attachments = 0;
        for (i = 0; next && i < c->count; ++i)
        {
            if (c->library->blocks[next->model_index].model.portal_count)
                break;
            ++attachments;
            next = geometry_node(c, (BHWorldGeometry *)next->placement.geometry.navigation.next);
        }
        if (!next || i == c->count)
        {
            c->error = 1;
            return 0;
        }
        sprintf(buffer, "Segment: %s - %d portals, %d attached models", b->definition.name, (int)b->model.portal_count,
                (int)attachments);
        if (!text(c, 0, 0, buffer, 0xff0000))
            return 0;
        if (c->clipboard_count)
        {
            if (c->clipboard_count <= 1)
                strcpy(buffer, "Clipboard: 1 portal segment");
            else
                sprintf(buffer, "Clipboard: 1 portal segment + %d attached", (int)(c->clipboard_count - 1));
            if (!text(c, 0, 455, buffer, 0x80ff))
                return 0;
        }
    }
    for (i = 0; i < 8; ++i)
        if (b->definition.used_mask & (1UL << i))
        {
            memcpy(&value, (char *)n->entity + 0x334 + i * 2, 2);
            sprintf(buffer, "%s=%d (default %d)", b->definition.names[i], (int)value, (int)b->definition.values[i]);
            if (!text(c, 0, (short)text_y, buffer, c->numeric && c->numeric->selected == (long)i ? 0xffffff : 0x808080))
                return 0;
            text_y += 15;
        }
    if (w->navigation.count > 6)
    {
        c->error = 1;
        return 0;
    }
    for (i = 0; i < w->navigation.count; ++i)
    {
        const BHPortal *p = w->navigation.portals + i;
        for (j = 0; j < 4; ++j)
        {
            float u = p->aperture[j == 0 || j == 3 ? 0 : 1], v = p->aperture[j < 2 ? 2 : 3];
            switch (p->direction)
            {
            case 0:
            case 4:
                vertices[j].x = (float)p->plane;
                vertices[j].y = u;
                vertices[j].z = v;
                break;
            case 1:
            case 5:
                vertices[j].x = u;
                vertices[j].y = (float)p->plane;
                vertices[j].z = v;
                break;
            case 2:
            case 6:
                vertices[j].x = u;
                vertices[j].y = v;
                vertices[j].z = (float)p->plane;
                break;
            default:
                c->error = 1;
                return 0;
            }
        }
        for (j = 0; j < 4; ++j)
            if (!segment(c, 0, c->line_image, vertices + j, vertices + (j + 1) % 4))
                return 0;
        if (p->neighbor && !portal_plane(c, p, p->overlap, c->selection_image))
            return 0;
        for (j = 0; j < 4 && p->overlap[j] == p->scratch[j]; ++j)
        {
        }
        if (j < 4 && !portal_plane(c, p, p->scratch, c->line_image))
            return 0;
    }
    return 1;
}
BHEditorNode *__cdecl bh_editor_world_clone(BHEditorWorld *c, long x, long y, long z, BHEditorNode *after,
                                            const BHEditorNode *prototype)
{
    BHEditorNode *n, *next;
    BHEntity *e;
    const BHScriptBlock *block;
    long delta[3], coordinate, grid;
    unsigned long i;
    float angle;
    if (!c || !c->pool || !c->library || !c->nodes || c->count >= c->capacity || !after || !prototype ||
        !member(c, after) || !prototype_valid(c, prototype) || !live(c, after->entity) ||
        prototype->model_index >= c->library->count)
    {
        if (c)
            c->error = 1;
        return 0;
    }
    block = c->library->blocks + prototype->model_index;
    grid = block->definition.grid;
    next = geometry_node(c, (BHWorldGeometry *)after->placement.geometry.navigation.next);
    if (grid <= 0 || !next)
    {
        c->error = 1;
        return 0;
    }
    e = bh_entity_acquire(c->pool);
    if (!e)
    {
        c->error = 2;
        return 0;
    }
    n = (BHEditorNode *)calloc(1, sizeof(*n));
    if (!n)
    {
        bh_entity_release(c->pool, e);
        c->error = 2;
        return 0;
    }
    n->entity = e;
    n->model_index = prototype->model_index;
    n->placement.geometry.model = &block->model;
    n->placement.geometry.flags = prototype->placement.geometry.flags;
    n->placement.geometry.query_flags = block->definition.flags;
    e->kind = 1;
    memcpy((char *)e + 0x134, (char *)prototype->entity + 0x134, 12);
    setword(e, 0x128, 0);
    setword(e, 0x12c, 0);
    setword(e, 0x100, 0);
    setword(e, 0x104, 0);
    setword(e, 0x148, 0);
    setword(e, 0x14c, 0);
    setword(e, 0x11c, 0xffffffffUL);
    setword(e, 0xfc, 0);
    setword(e, 0xf8, n->model_index);
    setword(e, 0xf4, n->placement.geometry.flags);
    setword(e, 0x114, block->definition.flags);
    memcpy((char *)e + 0x334, (char *)prototype->entity + 0x334, 16);
    angle = stored((n->placement.geometry.flags & 3UL) * 1.5707963267948966);
    n->placement.angles.x = angle;
    real(e, 0x3c, angle);
    real(e, 0x40, 0);
    real(e, 0x44, 0);
    delta[0] = x;
    delta[1] = y;
    delta[2] = z;
    for (i = 0; i < 3; ++i)
    {
        coordinate = signed_word((unsigned long)prototype->placement.coordinates[i] + (unsigned long)delta[i]);
        coordinate = (coordinate / grid) * grid;
        n->placement.coordinates[i] = coordinate;
        setword(e, 0xe8 + 4 * i, (unsigned long)coordinate);
        ((float *)&e->position)[i] = stored((double)((coordinate / grid) * grid));
    }
    n->placement.geometry.position = e->position;
    e->step = 0;
    if (!bh_editor_world_attach(c, n))
    {
        bh_entity_release(c->pool, e);
        free(n);
        c->error = 2;
        return 0;
    }
    n->heap_owned = 1;
    n->placement.geometry.previous = &after->placement.geometry;
    n->placement.geometry.navigation.next = &next->placement.geometry.navigation;
    after->placement.geometry.navigation.next = &n->placement.geometry.navigation;
    next->placement.geometry.previous = &n->placement.geometry;
    setword(e, 0x120, (unsigned long)after->entity);
    setword(e, 0x124, (unsigned long)next->entity);
    setword(after->entity, 0x124, (unsigned long)e);
    setword(next->entity, 0x120, (unsigned long)e);
    if (c->numeric)
        c->numeric->selected = -1;
    c->error = 0;
    return n;
}
int __cdecl bh_editor_world_dispose(BHEditorWorld *c)
{
    unsigned long i;
    BHEditorNode *n;
    if (!c || !c->pool || c->pool->dispatch_depth)
        return 0;
    for (i = 0; i < c->count; ++i)
    {
        n = c->nodes[i];
        if (live(c, n->entity))
            bh_entity_release(c->pool, n->entity);
        if (n->heap_owned)
            free(n);
    }
    c->count = 0;
    c->selection = 0;
    return 1;
}
