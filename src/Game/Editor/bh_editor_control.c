#include "../Entities/bh_field_access.h"
#include "bh_editor_control.h"
#include "../../Graphics/Textures/bh_gdi_text.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static long bits(unsigned long x)
{
    long v;
    memcpy(&v, &x, 4);
    return v;
}
static float number(BHEntity *e, unsigned long o)
{
    float f;
    memcpy(&f, (char *)e + o, 4);
    return f;
}
static void real(BHEntity *e, unsigned long o, float f)
{
    memcpy((char *)e + o, &f, 4);
}
static BHEditorControl *control(BHEntity *entity)
{
    BHEditorControl *c;
    memcpy(&c, &entity->bindings.editor_control, 4);
    return c;
}
static int fail(BHEditorControl *editor_control, int error)
{
    editor_control->error = error;
    return 0;
}
static int member(BHEditorWorld *editor_world, const BHEditorNode *n)
{
    unsigned long i;
    for (i = 0; i < editor_world->count; ++i)
        if (editor_world->nodes[i] == n)
            return 1;
    return 0;
}
static BHEditorNode *next(BHEditorNode *editor_node)
{
    return (BHEditorNode *)editor_node->placement.geometry.navigation.next;
}
static BHEditorNode *previous(BHEditorNode *editor_node)
{
    return (BHEditorNode *)editor_node->placement.geometry.previous;
}
static BHEditorNode *owner(BHEditorNode *editor_node)
{
    return (BHEditorNode *)editor_node->placement.geometry.owner;
}
static int budget(BHEditorControl *editor_control)
{
    if (++editor_control->steps > (editor_control->step_limit ? editor_control->step_limit : 1000000UL))
        return fail(editor_control, 4);
    return 1;
}
static int key(BHEditorControl *editor_control, unsigned long index)
{
    return bh_player_input_transition(editor_control->world->input, index);
}
static BHEditorNode *anchor(BHEditorControl *editor_control, BHEditorNode *n, int backwards)
{
    BHEditorNode *at = backwards ? previous(n) : next(n);
    unsigned long i = 0;
    while (at != n && owner(at))
    {
        if (!member(editor_control->world, at) || ++i > editor_control->world->count)
        {
            fail(editor_control, 1);
            return 0;
        }
        at = backwards ? previous(at) : next(at);
    }
    return at;
}
static void orbit(BHEditorNode *to, const BHEditorNode *from)
{
    memcpy(&to->entity->placement.editor_yaw, &from->entity->placement.editor_yaw, 12);
}
static void install(BHEditorControl *c, BHEditorNode *editor_node)
{
    memcpy(&editor_node->entity->bindings.editor_control, &c, 4);
    editor_node->entity->step = bh_editor_tick;
}
static void sync(BHEditorWorld *editor_world)
{
    unsigned long i, j;
    BHEditorNode *editor_node, *output_editor_node;
    BHWorldGeometry *world;
    for (i = 0; i < editor_world->count; ++i)
    {
        editor_node = editor_world->nodes[i];
        world = &editor_node->placement.geometry;
        editor_node->entity->position = world->position;
        for (j = 0; j < 3; ++j)
            bh_field_store_u32(&editor_node->entity->placement.coordinates[j], (unsigned long)editor_node->placement.coordinates[j]);
        bh_field_store_u32(&editor_node->entity->placement.orientation_flags, world->flags);
        bh_field_store_u32(&editor_node->entity->placement.model_index, editor_node->model_index);
        editor_node->entity->euler = editor_node->placement.angles;
        memcpy(&editor_node->entity->placement.transform, &world->transform, 48);
        memcpy(&editor_node->entity->placement.inverse, &world->inverse, 48);
        memcpy(&editor_node->entity->placement.minimum, &world->minimum, 12);
        memcpy(&editor_node->entity->placement.maximum, &world->maximum, 12);
        output_editor_node = owner(editor_node);
        bh_field_store_u32(&editor_node->entity->placement.owner, (unsigned long)(output_editor_node ? output_editor_node->entity : 0));
        bh_field_store_u32(&editor_node->entity->placement.previous, (unsigned long)previous(editor_node)->entity);
        bh_field_store_u32(&editor_node->entity->placement.next, (unsigned long)next(editor_node)->entity);
    }
}
static int rebuild(BHEditorControl *editor_control, BHEditorNode *editor_node)
{
    if (!member(editor_control->world, editor_node) || editor_node->model_index >= editor_control->world->library->count)
        return fail(editor_control, 1);
    editor_node->placement.geometry.model = &editor_control->world->library->blocks[editor_node->model_index].model;
    if (!bh_world_edit_rebuild(&editor_node->placement.geometry, editor_control->world->count))
        return fail(editor_control, 3);
    sync(editor_control->world);
    return 1;
}
static int select_node(BHEditorControl *editor_control, BHEditorNode *from, BHEditorNode *to)
{
    if (!to || !member(editor_control->world, to))
        return fail(editor_control, 1);
    if (editor_control->world->numeric)
        editor_control->world->numeric->selected = -1;
    orbit(to, from);
    from->entity->step = 0;
    editor_control->world->selection = to;
    install(editor_control, to);
    return rebuild(editor_control, to);
}
static BHVec3 center(BHEditorNode *editor_node)
{
    BHVec3 v;
    BHWorldGeometry *world = &editor_node->placement.geometry;
    v.x = rf(((double)world->minimum.x + world->maximum.x) * .5);
    v.y = rf(((double)world->minimum.y + world->maximum.y) * .5);
    v.z = rf(((double)world->minimum.z + world->maximum.z) * .5);
    return v;
}
static int move(BHEditorControl *editor_control, BHEditorNode *editor_node, unsigned long axis, long delta)
{
    long d[3] = {0, 0, 0};
    int attached = owner(editor_node) != 0;
    BHEditorNode *following;
    d[axis] = delta;
    if (!bh_world_edit_move(&editor_node->placement, editor_control->world->count, d[0], d[1], d[2], 1))
        return fail(editor_control, 3);
    following = next(editor_node);
    if (!attached && owner(following) &&
        !bh_world_edit_move(&following->placement, editor_control->world->count, d[0], d[1], d[2], 0))
        return fail(editor_control, 3);
    sync(editor_control->world);
    return 1;
}
static void zoom(BHEditorNode *editor_node, short y)
{
    double value = (double)bh_field_f32(&editor_node->entity->placement.editor_radius) -
                   (double)y * bh_field_f32(&editor_node->entity->placement.editor_radius) * (-.001);
    bh_field_store_f32(&editor_node->entity->placement.editor_radius, value < 1 ? 1 : value > 10000 ? 10000 : rf(value));
}
static int translate(BHEditorControl *editor_control, BHEditorNode *editor_node, short x, short y)
{
    long grid = editor_control->world->library->blocks[editor_node->model_index].definition.grid;
    float threshold, value;
    double yaw = bh_field_f32(&editor_node->entity->placement.editor_yaw),
           radius = bh_field_f32(&editor_node->entity->placement.editor_radius);
    unsigned long axis, offset;
    if (grid <= 0 || grid > 2147483)
        return fail(editor_control, 1);
    bh_field_store_f32(&editor_node->entity->placement.editor_pan_x, rf((double)bh_field_f32(&editor_node->entity->placement.editor_pan_x) -
                                                              ((double)y * sin(yaw) - (double)x * cos(yaw)) * radius));
    bh_field_store_f32(&editor_node->entity->placement.editor_pan_z, rf((double)bh_field_f32(&editor_node->entity->placement.editor_pan_z) -
                                                              ((double)y * cos(yaw) + (double)x * sin(yaw)) * radius));
    threshold = (float)(grid * 1000);
    for (axis = 0; axis < 2; ++axis)
    {
        offset = 0x140 + axis * 4;
        value = number(editor_node->entity, offset);
        if (!_finite(value))
            return fail(editor_control, 1);
        while (value > threshold)
        {
            if (!budget(editor_control))
                return 0;
            value = rf((double)value - threshold);
            real(editor_node->entity, offset, value);
            if (!move(editor_control, editor_node, axis ? 2 : 0, grid))
                return 0;
        }
        while (value < -threshold)
        {
            if (!budget(editor_control))
                return 0;
            value = rf((double)value + threshold);
            real(editor_node->entity, offset, value);
            if (!move(editor_control, editor_node, axis ? 2 : 0, -grid))
                return 0;
        }
    }
    return 1;
}
static void unlink_node(BHEditorNode *n)
{
    BHEditorNode *first_editor_node = previous(n), *second_editor_node = next(n);
    first_editor_node->placement.geometry.navigation.next = &second_editor_node->placement.geometry.navigation;
    second_editor_node->placement.geometry.previous = &first_editor_node->placement.geometry;
}
static int retire(BHEditorControl *editor_control, BHEditorNode *editor_node)
{
    unsigned long i;
    BHEditorWorld *editor_world = editor_control->world;
    if (!bh_entity_release(editor_world->pool, editor_node->entity))
        return fail(editor_control, 2);
    for (i = 0; i < editor_world->count && editor_world->nodes[i] != editor_node; ++i)
    {
    }
    if (i == editor_world->count)
        return fail(editor_control, 1);
    memmove(editor_world->nodes + i, editor_world->nodes + i + 1, (editor_world->count - i - 1) * sizeof(*editor_world->nodes));
    --editor_world->count;
    if (editor_node->heap_owned)
        free(editor_node);
    else
        editor_node->entity = 0;
    return 1;
}
static void empty_refresh(BHEntity *e, void *packet)
{
    BHEditorControl *editor_control = control(e);
    (void)packet;
    if (editor_control && bh_editor_message_queue(editor_control->world->queue) != 1)
        editor_control->error = 2;
}
static int erase(BHEditorControl *editor_control, BHEditorNode *editor_node)
{
    BHEditorWorld *editor_world = editor_control->world;
    BHEditorNode *fallback = 0, *at = editor_node, *before, *following, *end;
    int whole = !owner(editor_node), stop = !whole;
    unsigned long i;
    if (whole)
    {
        for (i = 0; i < 6; ++i)
            if (editor_node->placement.geometry.navigation.portals[i].neighbor)
                fallback = (BHEditorNode *)editor_node->placement.geometry.navigation.portals[i].neighbor;
        end = anchor(editor_control, editor_node, 0);
        if (!end)
            return 0;
        at = previous(end);
    }
    while (previous(at) != at)
    {
        if (!owner(at))
            stop = 1;
        before = previous(at);
        following = next(at);
        unlink_node(at);
        orbit(before, at);
        editor_world->selection = before;
        install(editor_control, before);
        if (!retire(editor_control, at))
            return 0;
        if (stop)
        {
            if (!whole)
            {
                if (!owner(before))
                {
                    before->entity->step = 0;
                    end = anchor(editor_control, before, 0);
                    if (!end)
                        return 0;
                    before = previous(end);
                    editor_world->selection = before;
                    install(editor_control, before);
                }
                return rebuild(editor_control, before);
            }
            at = before;
            for (i = 0; owner(at) && i < editor_world->count; ++i)
            {
                at->entity->step = 0;
                at = previous(at);
                install(editor_control, at);
            }
            if (owner(at))
                return fail(editor_control, 1);
            editor_world->selection = at;
            if (fallback)
            {
                if (!member(editor_world, fallback))
                    return fail(editor_control, 2);
                at->entity->step = 0;
                editor_world->selection = fallback;
                install(editor_control, fallback);
                return rebuild(editor_control, fallback);
            }
            return rebuild(editor_control, at);
        }
        at = before;
    }
    editor_world->selection = 0;
    at->entity->step = bh_editor_enter;
    at->entity->refresh = empty_refresh;
    sync(editor_world);
    return 1;
}
static int reattach(BHEditorControl *c, BHEditorNode *editor_node, int backwards)
{
    BHEditorNode *old = owner(editor_node), *to, *following;
    if (!old)
        return 1;
    to = anchor(c, old, backwards);
    if (!to)
        return 0;
    if (to != old)
    {
        unlink_node(editor_node);
        following = next(to);
        editor_node->placement.geometry.previous = &to->placement.geometry;
        editor_node->placement.geometry.navigation.next = &following->placement.geometry.navigation;
        to->placement.geometry.navigation.next = &editor_node->placement.geometry.navigation;
        following->placement.geometry.previous = &editor_node->placement.geometry;
    }
    return rebuild(c, editor_node);
}
static int copy_group(BHEditorControl *editor_control, BHEditorNode *n)
{
    BHEditorWorld *editor_world = editor_control->world;
    BHEditorNode *end = anchor(editor_control, n, 0), *start, *at;
    BHEditorClipboard *entry;
    unsigned long i = 0;
    if (!end)
        return 0;
    start = anchor(editor_control, end, 1);
    if (!start)
        return 0;
    if (owner(start))
        return 1;
    if (!editor_world->clipboard || !editor_world->clipboard_capacity)
        return fail(editor_control, 1);
    editor_world->clipboard_count = 0;
    editor_world->clipboard_origin = start;
    for (at = start; at != end; at = next(at))
    {
        if (i >= editor_world->clipboard_capacity)
            return fail(editor_control, 1);
        entry = editor_world->clipboard + i;
        entry->entity = *at->entity;
        entry->node = *at;
        entry->node.entity = &entry->entity;
        entry->node.context = editor_world;
        entry->node.heap_owned = 0;
        ++i;
        editor_world->clipboard_count = (long)i;
        if (i >= 50)
            return 2;
    }
    return 1;
}
static int paste(BHEditorControl *editor_control, BHEditorNode *editor_node)
{
    BHEditorWorld *editor_world = editor_control->world;
    BHEditorNode *after = editor_node, *first = 0, *new_node, *end;
    long grid, dx, dy, dz;
    unsigned long i;
    if (!rebuild(editor_control, editor_node))
        return 0;
    if (editor_world->numeric)
        editor_world->numeric->selected = -1;
    if (!editor_world->clipboard_count)
        return 1;
    if (!editor_world->clipboard || editor_world->clipboard_count < 0 || (unsigned long)editor_world->clipboard_count > editor_world->clipboard_capacity)
        return fail(editor_control, 1);
    editor_node->entity->step = 0;
    grid = editor_world->library->blocks[editor_node->model_index].definition.grid;
    dx = bits((unsigned long)grid + (unsigned long)editor_node->placement.coordinates[0] -
              (unsigned long)editor_world->clipboard[0].node.placement.coordinates[0]);
    dy =
        bits((unsigned long)editor_node->placement.coordinates[1] - (unsigned long)editor_world->clipboard[0].node.placement.coordinates[1]);
    dz = bits((unsigned long)editor_node->placement.coordinates[2] + (unsigned long)grid -
              (unsigned long)editor_world->clipboard[0].node.placement.coordinates[2]);
    if (!owner(editor_node))
    {
        end = anchor(editor_control, editor_node, 0);
        if (!end)
            return 0;
        after = previous(end);
    }
    for (i = 0; i < (unsigned long)editor_world->clipboard_count; ++i)
    {
        new_node = bh_editor_world_clone(editor_world, dx, dy, dz, after, &editor_world->clipboard[i].node);
        if (!new_node)
            return fail(editor_control, 2);
        if (!first)
            first = new_node;
        if (!rebuild(editor_control, new_node))
            return 0;
        after = new_node;
    }
    editor_world->selection = first;
    install(editor_control, first);
    first->entity->refresh = bh_editor_world_draw_callback();
    return rebuild(editor_control, first);
}
static int pan(BHEditorControl *editor_control, BHEditorNode *editor_node, short x, short y)
{
    BHEditorWorld *editor_world = editor_control->world;
    BHVec3 at, p, target;
    BHEditorNode *to;
    double yaw = bh_field_f32(&editor_node->entity->placement.editor_yaw),
           radius = bh_field_f32(&editor_node->entity->placement.editor_radius);
    int first = key(editor_control, 9), second = key(editor_control, 10);
    unsigned long i;
    if (second == 2)
        editor_world->camera_offset.y = rf((double)editor_world->camera_offset.y - (double)y * radius * .001);
    else if (first == 2)
        zoom(editor_node, y);
    else
    {
        editor_world->camera_offset.x = rf((double)editor_world->camera_offset.x - radius * (sin(yaw) * y - cos(yaw) * x) * .001);
        editor_world->camera_offset.z = rf((double)editor_world->camera_offset.z - (sin(yaw) * x + cos(yaw) * y) * radius * .001);
    }
    at = center(editor_node);
    p.x = rf((double)at.x + editor_world->camera_offset.x);
    p.y = rf((double)at.y + editor_world->camera_offset.y);
    p.z = rf((double)at.z + editor_world->camera_offset.z);
    if (editor_node->placement.geometry.minimum.x <= p.x && p.x <= editor_node->placement.geometry.maximum.x &&
        editor_node->placement.geometry.minimum.y <= p.y && p.y <= editor_node->placement.geometry.maximum.y &&
        editor_node->placement.geometry.minimum.z <= p.z && p.z <= editor_node->placement.geometry.maximum.z)
        return 1;
    to = next(editor_node);
    for (i = 0; to != editor_node && i < editor_world->count; ++i, to = next(to))
        if (!owner(to) && to->placement.geometry.minimum.x < p.x && p.x < to->placement.geometry.maximum.x &&
            to->placement.geometry.minimum.y < p.y && p.y < to->placement.geometry.maximum.y &&
            to->placement.geometry.minimum.z < p.z && p.z < to->placement.geometry.maximum.z)
            break;
    if (to == editor_node)
    {
        to = next(editor_node);
        for (i = 0; to != editor_node && i < editor_world->count; ++i, to = next(to))
            if (!owner(to) && to->placement.geometry.minimum.x < p.x && p.x < to->placement.geometry.maximum.x &&
                to->placement.geometry.minimum.z < p.z && p.z < to->placement.geometry.maximum.z)
                break;
    }
    if (to == editor_node)
        return 1;
    if (!member(editor_world, to))
        return fail(editor_control, 1);
    bh_sound_request(editor_world->sound, editor_control->select_sound, 1, 1, 0, 0);
    target = center(to);
    editor_world->camera_offset.x = rf(((double)at.x + editor_world->camera_offset.x) - target.x);
    editor_world->camera_offset.y = rf(((double)at.y + editor_world->camera_offset.y) - target.y);
    editor_world->camera_offset.z = rf(((double)at.z + editor_world->camera_offset.z) - target.z);
    return select_node(editor_control, editor_node, to);
}
static int step(BHEditorControl *editor_control, BHEditorNode *editor_node)
{
    BHEditorWorld *editor_world = editor_control->world;
    BHEditorNode *to, *end;
    BHScriptBlock *block;
    unsigned long raw, i;
    short mouse_x, mouse_y;
    int state, a, b;
    long grid;
    double value;
    float yaw;
    editor_control->error = 0;
    editor_control->steps = 0;
    if (!editor_world || !editor_world->pool || !editor_world->library || !editor_world->input || !editor_world->numeric || !member(editor_world, editor_node) ||
        editor_node->model_index >= editor_world->library->count || !bh_math_environment_supported())
        return fail(editor_control, 1);
    if (editor_world->mode != 1)
        return 1;
    bh_field_store_f32(&editor_node->entity->placement.selection_phase,
                       rf((double)bh_field_f32(&editor_node->entity->placement.selection_phase) - (-1.0)));
    raw = editor_world->input->values[8];
    mouse_x = (short)(unsigned short)raw;
    mouse_y = (short)(unsigned short)(raw >> 16);
    state = key(editor_control, 16);
    if (state == 0)
    {
        if (!editor_world->browser || editor_world->library->columns <= 0)
            return fail(editor_control, 1);
        editor_world->catalog_open = 1;
        editor_world->browser->selected = (long)editor_node->model_index;
        editor_world->browser->active = (long)editor_node->model_index;
        editor_world->browser->cursor_x = (float)(editor_node->model_index % editor_world->library->columns) + .5f;
        editor_world->browser->cursor_y = (float)(editor_node->model_index / editor_world->library->columns) + .5f;
        return 1;
    }
    if (state == 1)
    {
        editor_world->catalog_open = 0;
        if (editor_world->browser && editor_world->browser->selected != editor_world->browser->active)
        {
            if (editor_world->browser->selected < 0 || (unsigned long)editor_world->browser->selected >= editor_world->library->count)
                return fail(editor_control, 1);
            editor_node->model_index = (unsigned long)editor_world->browser->selected;
            editor_world->numeric->selected = -1;
            memcpy(&editor_node->entity->placement.script_values, editor_world->library->blocks[editor_node->model_index].definition.values, 16);
            if (!rebuild(editor_control, editor_node))
                return 0;
        }
    }
    else if (state == 2)
    {
        if (!editor_world->browser)
            return fail(editor_control, 1);
        editor_world->browser->cursor_x = rf((double)editor_world->browser->cursor_x - (double)mouse_x * (-.02));
        editor_world->browser->cursor_y = rf((double)editor_world->browser->cursor_y - (double)mouse_y * (-.02));
        return 1;
    }
    state = key(editor_control, 15);
    if (state == 0)
    {
        editor_world->camera_offset.x = editor_world->camera_offset.y = editor_world->camera_offset.z = 0;
        editor_world->axes_enabled = 1;
        editor_world->numeric->selected = -1;
        if (owner(editor_node))
        {
            end = anchor(editor_control, editor_node, 0);
            if (!end)
                return 0;
            to = anchor(editor_control, end, 1);
            if (!to)
                return 0;
            return select_node(editor_control, editor_node, to);
        }
        return 1;
    }
    if (state == 1)
    {
        editor_world->camera_offset.x = editor_world->camera_offset.y = editor_world->camera_offset.z = 0;
        editor_world->axes_enabled = 0;
    }
    else if (state == 2)
        return pan(editor_control, editor_node, mouse_x, mouse_y);
    a = key(editor_control, 9);
    b = key(editor_control, 10);
    if (b == 0)
    {
        bh_field_store_f32(&editor_node->entity->placement.editor_pan_x, 0);
        bh_field_store_f32(&editor_node->entity->placement.editor_pan_z, 0);
    }
    if (b == 2)
    {
        if (!translate(editor_control, editor_node, mouse_x, mouse_y))
            return 0;
    }
    else if (a == 0 || a == 2)
        zoom(editor_node, mouse_y);
    else
    {
        bh_field_store_f32(&editor_node->entity->placement.editor_yaw, rf((double)bh_field_f32(&editor_node->entity->placement.editor_yaw) -
                                                                (double)mouse_x * (-1.0 / 512.0)));
        value = (double)bh_field_f32(&editor_node->entity->placement.editor_pitch) - (double)mouse_y * (1.0 / 512.0);
        bh_field_store_f32(&editor_node->entity->placement.editor_pitch, value > 1.5707963267948966    ? 1.5707963705062866f
                                                               : value < -1.5707963267948966 ? -1.5707963705062866f
                                                                                             : rf(value));
    }
    if (key(editor_control, 0) == 0)
    {
        editor_node->entity->step = 0;
        to = editor_node;
        if (!owner(editor_node))
        {
            end = anchor(editor_control, editor_node, 0);
            if (!end)
                return 0;
            to = previous(end);
        }
        to = bh_editor_world_clone(editor_world, 1, 0, 1, to, editor_node);
        if (!to)
            return fail(editor_control, 2);
        editor_world->selection = to;
        install(editor_control, to);
        return rebuild(editor_control, to);
    }
    if (key(editor_control, 1) == 0)
        return erase(editor_control, editor_node);
    if (key(editor_control, 11) == 0 && owner(editor_node) && !reattach(editor_control, editor_node, 1))
        return 0;
    if (key(editor_control, 12) == 0 && owner(editor_node) && !reattach(editor_control, editor_node, 0))
        return 0;
    if (key(editor_control, 17) == 0)
    {
        editor_world->numeric->entry_state = 0;
        editor_world->numeric->selected = bits((unsigned long)editor_world->numeric->selected + 1);
        block = editor_world->library->blocks + editor_node->model_index;
        while (editor_world->numeric->selected < 8 && !(block->definition.used_mask & (1UL << editor_world->numeric->selected)))
            ++editor_world->numeric->selected;
        if (editor_world->numeric->selected >= 8)
            editor_world->numeric->selected = -1;
    }
    for (i = 18; i <= 28; ++i)
        if (key(editor_control, i) == 0 && !bh_editor_numeric_key(editor_world->numeric, editor_node->entity, i == 18 ? -1 : (long)i - 19))
            return fail(editor_control, 1);
    if (key(editor_control, 2) == 0)
    {
        to = owner(editor_node) ? previous(editor_node) : editor_node;
        if (!owner(to))
        {
            end = anchor(editor_control, to, 0);
            if (!end)
                return 0;
            to = previous(end);
        }
        return select_node(editor_control, editor_node, to);
    }
    if (key(editor_control, 3) == 0)
    {
        to = next(editor_node);
        if (!owner(editor_node))
        {
            if (!owner(to))
                to = previous(to);
        }
        else if (!owner(to))
        {
            to = anchor(editor_control, to, 1);
            if (!to)
                return 0;
            to = next(to);
        }
        return select_node(editor_control, editor_node, to);
    }
    for (i = 4; i <= 5; ++i)
        if (key(editor_control, i) == 0)
        {
            editor_node->placement.geometry.flags = ((unsigned long)(i == 4 ? -1 : 1) + editor_node->placement.geometry.flags) & 3;
            editor_node->placement.angles.x = rf(editor_node->placement.geometry.flags * 1.5707963267948966);
            editor_node->placement.angles.y = editor_node->placement.angles.z = 0;
            if (!rebuild(editor_control, editor_node))
                return 0;
            if (!owner(editor_node) && !bh_world_edit_rotate(&next(editor_node)->placement, editor_world->count, i == 4 ? -1 : 1,
                                                   editor_node->placement.coordinates[0], editor_node->placement.coordinates[2]))
                return fail(editor_control, 3);
            sync(editor_world);
        }
    for (i = 6; i <= 7; ++i)
        if (key(editor_control, i) == 0)
        {
            grid = editor_world->library->blocks[editor_node->model_index].definition.grid;
            if (grid <= 0)
                return fail(editor_control, 1);
            if (!move(editor_control, editor_node, 1, i == 6 ? -grid : grid))
                return 0;
        }
    if (key(editor_control, 13) == 0)
    {
        a = copy_group(editor_control, editor_node);
        if (!a)
            return 0;
        if (a == 2)
            return 1;
    }
    if (key(editor_control, 14) == 0)
        return paste(editor_control, editor_node);
    return 1;
}
int bh_editor_control_bind(BHEditorControl *editor_control, BHEditorNode *editor_node)
{
    if (!editor_control || !editor_control->world || !editor_node || !member(editor_control->world, editor_node) || !editor_node->entity)
        return 0;
    if (editor_control->world->selection && editor_control->world->selection != editor_node)
        editor_control->world->selection->entity->step = 0;
    editor_control->world->selection = editor_node;
    install(editor_control, editor_node);
    return 1;
}
void bh_editor_tick(BHEntity *e)
{
    BHEditorControl *c = control(e);
    BHEditorNode *n = bh_editor_world_node(e);
    if (c && n)
        step(c, n);
}
void bh_editor_enter(BHEntity *entity)
{
    BHEditorControl *editor_control = control(entity);
    if (editor_control && editor_control->world && editor_control->world->input && key(editor_control, 0) == 0)
    {
        entity->refresh = bh_editor_world_draw_callback();
        entity->step = bh_editor_tick;
    }
}
