#include "../Entities/bh_field_access.h"
#include "bh_editor_world.h"
#include "../../Camera/bh_camera_queue.h"
#include "../../Graphics/bh_horizon_queue.h"
#include "../../Graphics/Textures/bh_gdi_text.h"
#include "../../Graphics/bh_draw_submit.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
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
static void setword(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static long signed_word(unsigned long x)
{
    long n;
    memcpy(&n, &x, 4);
    return n;
}
static int live(BHEditorWorld *editor_world, BHEntity *e)
{
    return bh_entity_is_active(editor_world->pool, e);
}
static int member(BHEditorWorld *editor_world, const BHEditorNode *n)
{
    unsigned long i;
    for (i = 0; i < editor_world->count; ++i)
        if (editor_world->nodes[i] == n)
            return 1;
    return 0;
}
static int prototype_valid(BHEditorWorld *editor_world, const BHEditorNode *editor_node)
{
    unsigned long i;
    if (member(editor_world, editor_node))
        return live(editor_world, editor_node->entity);
    if (!editor_world->clipboard || editor_world->clipboard_count < 0 || (unsigned long)editor_world->clipboard_count > editor_world->clipboard_capacity)
        return 0;
    for (i = 0; i < (unsigned long)editor_world->clipboard_count; ++i)
        if (editor_node == &editor_world->clipboard[i].node)
            return editor_node->context == editor_world && editor_node->entity == &editor_world->clipboard[i].entity;
    return 0;
}
static BHEditorNode *geometry_node(BHEditorWorld *c, BHWorldGeometry *w)
{
    BHEditorNode *n = (BHEditorNode *)w;
    return member(c, n) ? n : 0;
}
static void refresh(BHEntity *entity, void *packet)
{
    BHEditorNode *editor_node;
    (void)packet;
    memcpy(&editor_node, &entity->bindings.editor_node, 4);
    if (editor_node)
        bh_editor_world_draw(editor_node->context, editor_node);
}
BHEditorNode *bh_editor_world_node(BHEntity *entity)
{
    BHEditorNode *n;
    if (!entity)
        return 0;
    memcpy(&n, &entity->bindings.editor_node, 4);
    return n;
}
BHEntityCallback bh_editor_world_draw_callback(void)
{
    return refresh;
}
int bh_editor_world_attach(BHEditorWorld *editor_world, BHEditorNode *editor_node)
{
    if (!editor_world || !editor_world->pool || !editor_world->library || !editor_world->nodes || editor_world->count >= editor_world->capacity || !editor_node || !editor_node->entity ||
        !live(editor_world, editor_node->entity) || editor_node->model_index >= editor_world->library->count || member(editor_world, editor_node))
        return 0;
    editor_node->context = editor_world;
    editor_node->heap_owned = 0;
    memcpy(&editor_node->entity->bindings.editor_node, &editor_node, 4);
    editor_node->entity->refresh = refresh;
    editor_world->nodes[editor_world->count++] = editor_node;
    return 1;
}
static BHVec3 center(BHEditorNode *editor_node)
{
    BHVec3 v;
    BHWorldGeometry *world = &editor_node->placement.geometry;
    v.x = stored(((double)world->minimum.x + world->maximum.x) * .5);
    v.y = stored(((double)world->minimum.y + world->maximum.y) * .5);
    v.z = stored(((double)world->minimum.z + world->maximum.z) * .5);
    return v;
}
static int segment(BHEditorWorld *editor_world, unsigned long bucket, unsigned long image, const BHVec3 *a, const BHVec3 *b)
{
    if (bh_draw_segment(editor_world->queue, bucket, image, a, b, .5f, .5f, 1, 1, 0) != 1)
    {
        editor_world->error = 2;
        return 0;
    }
    return 1;
}
static int text(BHEditorWorld *editor_world, short x, short y, const char *message, unsigned long color)
{
    if (bh_gdi_text_queue(editor_world->queue, x, y, message, color) != 1)
    {
        editor_world->error = 2;
        return 0;
    }
    return 1;
}
static int portal_plane(BHEditorWorld *editor_world, const BHPortal *portal, const float *uv, unsigned long image)
{
    BHVec3 point, up = {0, 0, 1}, normal = {0, 0, 0};
    float u = stored(((double)uv[0] + uv[1]) * .5), v = stored(((double)uv[2] + uv[3]) * .5);
    switch (portal->direction)
    {
    case 0:
    case 4:
        point.x = (float)portal->plane;
        point.y = u;
        point.z = v;
        normal.x = 1;
        break;
    case 1:
    case 5:
        point.x = u;
        point.y = (float)portal->plane;
        point.z = v;
        normal.y = 1;
        break;
    case 2:
    case 6:
        point.x = u;
        point.y = v;
        point.z = (float)portal->plane;
        up.y = 1;
        up.z = 0;
        normal.z = 1;
        break;
    default:
        editor_world->error = 1;
        return 0;
    }
    if (bh_draw_oriented(editor_world->queue, 3, image, &point, &up, &normal, stored((double)uv[1] - uv[0]),
                         stored((double)uv[3] - uv[2]), 0, 8, .5f) != 1)
    {
        editor_world->error = 2;
        return 0;
    }
    return 1;
}
int bh_editor_world_draw(BHEditorWorld *editor_world, BHEditorNode *editor_node)
{
    BHWorldGeometry *world;
    BHScriptBlock *script_block;
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
    if (editor_world && editor_world->mode != 1)
    {
        editor_world->error = 0;
        return 1;
    }
    if (!editor_world || !editor_world->pool || !editor_world->library || !editor_world->queue || !editor_node || !member(editor_world, editor_node) || !live(editor_world, editor_node->entity) || !editor_world->selection ||
        !member(editor_world, editor_world->selection) || editor_node->model_index >= editor_world->library->count || !bh_math_environment_supported())
    {
        if (editor_world)
            editor_world->error = 1;
        return 0;
    }
    editor_world->error = 0;
    if (editor_world->mode != 1)
        return 1;
    if (editor_world->catalog_open)
    {
        if (editor_node != editor_world->selection)
            return 1;
        if (!editor_world->browser || !bh_script_block_browser(editor_world->browser))
        {
            editor_world->error = 2;
            return 0;
        }
        return 1;
    }
    world = &editor_node->placement.geometry;
    script_block = editor_world->library->blocks + editor_node->model_index;
    at = center(editor_node);
    selected = center(editor_world->selection);
    owner = world->owner ? geometry_node(editor_world, world->owner) : 0;
    if (world->owner && !owner)
    {
        editor_world->error = 1;
        return 0;
    }
    if (editor_node != editor_world->selection)
    {
        dx = (double)at.x - ((double)selected.x + editor_world->camera_offset.x);
        dy = (double)at.y - ((double)selected.y + editor_world->camera_offset.y);
        dz = (double)at.z - ((double)selected.z + editor_world->camera_offset.z);
        from.x = stored(dx);
        from.y = stored(dy);
        from.z = stored(dz);
        distance = stored(sqrt((double)from.x * from.x + (double)from.y * from.y + (double)from.z * from.z));
        radius = bh_field_f32(&editor_world->selection->entity->placement.editor_radius);
        if (400.0 - (double)radius * (-.5) < distance)
            return 1;
        if (200.0 - (double)radius * (-.25) < distance && owner)
            return 1;
    }
    if (owner)
    {
        to = center(owner);
        if (!segment(editor_world, 3, editor_world->selection_image, &at, &to))
            return 0;
    }
    bucket = (bh_field_u32(&editor_node->entity->placement.query_flags) & 1) ? 3 : 0;
    modulation =
        editor_node == editor_world->selection ? stored(.75 - sin((double)bh_field_f32(&editor_node->entity->placement.selection_phase)) * (-.25)) : 1;
    angles = editor_node->entity->euler;
    if (bh_draw_submit_resource(editor_world->queue, bucket, script_block->resource, &editor_node->entity->position, &angles, modulation) != 1)
    {
        editor_world->error = 2;
        return 0;
    }
    if (editor_node != editor_world->selection && world != editor_world->selection->placement.geometry.owner)
        return 1;
    if (editor_node == editor_world->selection)
    {
        yaw = bh_field_f32(&editor_node->entity->placement.editor_yaw);
        pitch = bh_field_f32(&editor_node->entity->placement.editor_pitch);
        radius = bh_field_f32(&editor_node->entity->placement.editor_radius);
        if (!_finite(yaw) || !_finite(pitch) || !_finite(radius))
        {
            editor_world->error = 1;
            return 0;
        }
        at.x = stored((double)at.x + editor_world->camera_offset.x);
        at.y = stored((double)at.y + editor_world->camera_offset.y);
        at.z = stored((double)at.z + editor_world->camera_offset.z);
        eye.x = stored(-(double)radius * sin(yaw) * cos(pitch) + at.x);
        eye.y = stored((double)stored((double)radius * sin(pitch)) + at.y);
        eye.z = stored(cos(yaw) * (-(double)radius) * cos(pitch) + at.z);
        angles.x = yaw;
        angles.y = pitch;
        angles.z = 0;
        if (bh_camera_queue_pose(editor_world->queue, &eye, &angles) != 1 ||
            bh_horizon_queue(editor_world->queue, editor_world->horizon_image, 5000, 5000, 25000, 12500, 0x804000) != 1)
        {
            editor_world->error = 2;
            return 0;
        }
        if (editor_world->axes_enabled)
            for (i = 0; i < 3; ++i)
            {
                for (j = 0; j < 3; ++j)
                {
                    ((float *)&from)[j] = stored((double)((float *)&at)[j] - ((float *)(axes + i))[j] * (-10.0));
                    ((float *)&to)[j] = stored((double)((float *)&at)[j] - ((float *)(axes + i))[j] * 10.0);
                }
                if (!segment(editor_world, 3, editor_world->line_image, &from, &to))
                    return 0;
            }
    }
    if (!script_block->model.portal_count && editor_node == editor_world->selection)
    {
        sprintf(buffer, owner ? "Attached model: %s" : "Floating model: %s", script_block->definition.name);
        if (!text(editor_world, 0, 15, buffer, 0xff00))
            return 0;
        text_y = 30;
    }
    if (script_block->model.portal_count)
    {
        next = geometry_node(editor_world, (BHWorldGeometry *)world->navigation.next);
        attachments = 0;
        for (i = 0; next && i < editor_world->count; ++i)
        {
            if (editor_world->library->blocks[next->model_index].model.portal_count)
                break;
            ++attachments;
            next = geometry_node(editor_world, (BHWorldGeometry *)next->placement.geometry.navigation.next);
        }
        if (!next || i == editor_world->count)
        {
            editor_world->error = 1;
            return 0;
        }
        sprintf(buffer, "Segment: %s - %d portals, %d attached models", script_block->definition.name, (int)script_block->model.portal_count,
                (int)attachments);
        if (!text(editor_world, 0, 0, buffer, 0xff0000))
            return 0;
        if (editor_world->clipboard_count)
        {
            if (editor_world->clipboard_count <= 1)
                strcpy(buffer, "Clipboard: 1 portal segment");
            else
                sprintf(buffer, "Clipboard: 1 portal segment + %d attached", (int)(editor_world->clipboard_count - 1));
            if (!text(editor_world, 0, 455, buffer, 0x80ff))
                return 0;
        }
    }
    for (i = 0; i < 8; ++i)
        if (script_block->definition.used_mask & (1UL << i))
        {
            memcpy(&value, &editor_node->entity->placement.script_values[i], 2);
            sprintf(buffer, "%s=%d (default %d)", script_block->definition.names[i], (int)value, (int)script_block->definition.values[i]);
            if (!text(editor_world, 0, (short)text_y, buffer, editor_world->numeric && editor_world->numeric->selected == (long)i ? 0xffffff : 0x808080))
                return 0;
            text_y += 15;
        }
    if (world->navigation.count > 6)
    {
        editor_world->error = 1;
        return 0;
    }
    for (i = 0; i < world->navigation.count; ++i)
    {
        const BHPortal *portal = world->navigation.portals + i;
        for (j = 0; j < 4; ++j)
        {
            float u = portal->aperture[j == 0 || j == 3 ? 0 : 1], v = portal->aperture[j < 2 ? 2 : 3];
            switch (portal->direction)
            {
            case 0:
            case 4:
                vertices[j].x = (float)portal->plane;
                vertices[j].y = u;
                vertices[j].z = v;
                break;
            case 1:
            case 5:
                vertices[j].x = u;
                vertices[j].y = (float)portal->plane;
                vertices[j].z = v;
                break;
            case 2:
            case 6:
                vertices[j].x = u;
                vertices[j].y = v;
                vertices[j].z = (float)portal->plane;
                break;
            default:
                editor_world->error = 1;
                return 0;
            }
        }
        for (j = 0; j < 4; ++j)
            if (!segment(editor_world, 0, editor_world->line_image, vertices + j, vertices + (j + 1) % 4))
                return 0;
        if (portal->neighbor && !portal_plane(editor_world, portal, portal->overlap, editor_world->selection_image))
            return 0;
        for (j = 0; j < 4 && portal->overlap[j] == portal->scratch[j]; ++j)
        {
        }
        if (j < 4 && !portal_plane(editor_world, portal, portal->scratch, editor_world->line_image))
            return 0;
    }
    return 1;
}
BHEditorNode *bh_editor_world_clone(BHEditorWorld *editor_world, long x, long y, long z, BHEditorNode *after,
                                    const BHEditorNode *prototype)
{
    BHEditorNode *editor_node, *next;
    BHEntity *entity;
    const BHScriptBlock *block;
    long delta[3], coordinate, grid;
    unsigned long i;
    float angle;
    if (!editor_world || !editor_world->pool || !editor_world->library || !editor_world->nodes || editor_world->count >= editor_world->capacity || !after || !prototype ||
        !member(editor_world, after) || !prototype_valid(editor_world, prototype) || !live(editor_world, after->entity) ||
        prototype->model_index >= editor_world->library->count)
    {
        if (editor_world)
            editor_world->error = 1;
        return 0;
    }
    block = editor_world->library->blocks + prototype->model_index;
    grid = block->definition.grid;
    next = geometry_node(editor_world, (BHWorldGeometry *)after->placement.geometry.navigation.next);
    if (grid <= 0 || !next)
    {
        editor_world->error = 1;
        return 0;
    }
    entity = bh_entity_acquire(editor_world->pool);
    if (!entity)
    {
        editor_world->error = 2;
        return 0;
    }
    editor_node = (BHEditorNode *)calloc(1, sizeof(*editor_node));
    if (!editor_node)
    {
        bh_entity_release(editor_world->pool, entity);
        editor_world->error = 2;
        return 0;
    }
    editor_node->entity = entity;
    editor_node->model_index = prototype->model_index;
    editor_node->placement.geometry.model = &block->model;
    editor_node->placement.geometry.flags = prototype->placement.geometry.flags;
    editor_node->placement.geometry.query_flags = block->definition.flags;
    entity->kind = 1;
    memcpy((char *)&entity->placement.editor_yaw, &prototype->entity->placement.editor_yaw, 12);
    entity->placement.actors = NULL;
    bh_field_store_u32(&entity->placement.attachments, 0);
    /* +100 is initialized here; no recovered consumer establishes its meaning. */
    setword(entity, 0x100, 0);
    entity->placement.activity_frames = 0;
    bh_field_store_u32(&entity->placement.damage, 0);
    bh_field_store_u32(&entity->placement.damage_source, 0);
    bh_field_store_u32(&entity->placement.city_id, 0xffffffffUL);
    bh_field_store_u32(&entity->placement.radar_visited, 0);
    bh_field_store_u32(&entity->placement.model_index, editor_node->model_index);
    bh_field_store_u32(&entity->placement.orientation_flags, editor_node->placement.geometry.flags);
    bh_field_store_u32(&entity->placement.query_flags, block->definition.flags);
    memcpy((char *)&entity->placement.script_values, &prototype->entity->placement.script_values, 16);
    angle = stored((editor_node->placement.geometry.flags & 3UL) * 1.5707963267948966);
    editor_node->placement.angles.x = angle;
    bh_field_store_f32(&entity->euler.x, angle);
    bh_field_store_f32(&entity->euler.y, 0);
    bh_field_store_f32(&entity->euler.z, 0);
    delta[0] = x;
    delta[1] = y;
    delta[2] = z;
    for (i = 0; i < 3; ++i)
    {
        coordinate = signed_word((unsigned long)prototype->placement.coordinates[i] + (unsigned long)delta[i]);
        coordinate = (coordinate / grid) * grid;
        editor_node->placement.coordinates[i] = coordinate;
        bh_field_store_u32(&entity->placement.coordinates[i], (unsigned long)coordinate);
        ((float *)&entity->position)[i] = stored((double)((coordinate / grid) * grid));
    }
    editor_node->placement.geometry.position = entity->position;
    entity->step = 0;
    if (!bh_editor_world_attach(editor_world, editor_node))
    {
        bh_entity_release(editor_world->pool, entity);
        free(editor_node);
        editor_world->error = 2;
        return 0;
    }
    editor_node->heap_owned = 1;
    editor_node->placement.geometry.previous = &after->placement.geometry;
    editor_node->placement.geometry.navigation.next = &next->placement.geometry.navigation;
    after->placement.geometry.navigation.next = &editor_node->placement.geometry.navigation;
    next->placement.geometry.previous = &editor_node->placement.geometry;
    bh_field_store_u32(&entity->placement.previous, (unsigned long)after->entity);
    bh_field_store_u32(&entity->placement.next, (unsigned long)next->entity);
    bh_field_store_u32(&after->entity->placement.next, (unsigned long)entity);
    bh_field_store_u32(&next->entity->placement.previous, (unsigned long)entity);
    if (editor_world->numeric)
        editor_world->numeric->selected = -1;
    editor_world->error = 0;
    return editor_node;
}
int bh_editor_world_dispose(BHEditorWorld *editor_world)
{
    unsigned long i;
    BHEditorNode *editor_node;
    if (!editor_world || !editor_world->pool || editor_world->pool->dispatch_depth)
        return 0;
    for (i = 0; i < editor_world->count; ++i)
    {
        editor_node = editor_world->nodes[i];
        if (live(editor_world, editor_node->entity))
            bh_entity_release(editor_world->pool, editor_node->entity);
        if (editor_node->heap_owned)
            free(editor_node);
    }
    editor_world->count = 0;
    editor_world->selection = 0;
    return 1;
}
