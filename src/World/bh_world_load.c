#include "../Game/Entities/bh_field_access.h"
#include "bh_world_load.h"
#include "../Assets/bh_vfile.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <stdlib.h>
#include <string.h>
static void setword(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static int read(BHVFile *f, void *p, unsigned long size)
{
    return bh_vfile_read(p, size, 1, f) == 1;
}
static int end(BHWorldLoad *world_load, BHVFile *f, int error)
{
    bh_vfile_close(f);
    world_load->error = error;
    return error == 0;
}
static void sync(BHEditorWorld *editor_world)
{
    unsigned long i;
    BHEditorNode *editor_node, *output_editor_node;
    BHWorldGeometry *world;
    for (i = 0; i < editor_world->count; ++i)
    {
        editor_node = editor_world->nodes[i];
        world = &editor_node->placement.geometry;
        memcpy(&editor_node->entity->placement.transform, &world->transform, 48);
        memcpy(&editor_node->entity->placement.inverse, &world->inverse, 48);
        memcpy(&editor_node->entity->placement.minimum, &world->minimum, 12);
        memcpy(&editor_node->entity->placement.maximum, &world->maximum, 12);
        output_editor_node = (BHEditorNode *)world->owner;
        bh_field_store_u32(&editor_node->entity->placement.owner, (unsigned long)(output_editor_node ? output_editor_node->entity : 0));
    }
}
int bh_world_load(BHWorldLoad *world_load, const char *name)
{
    BHEditorWorld *editor_world;
    BHScriptVMContext *vm;
    BHEntityPool *pool;
    BHVFile *file;
    BHEditorNode *editor_node, *first, *last, *at;
    BHScriptBlock *block;
    BHEntity *entity, *timer;
    BHScriptLifetimeState life;
    BHScriptVMState script;
    BHScriptVMActor *actor;
    long count, length, position, coordinates[4];
    unsigned long i, j, k, index, id, cursor = 0;
    char model[17];
    short vars[8];
    if (!world_load || !name || !world_load->editor || !(editor_world = world_load->editor->world) || !editor_world->pool || !editor_world->library || !editor_world->library->blocks ||
        !editor_world->nodes || editor_world->count || !editor_world->capacity || editor_world->capacity > 65535 || !(vm = world_load->vm) || !vm->lifetime ||
        vm->lifetime->timers.pool != editor_world->pool || !vm->actors || !world_load->states || world_load->actor_capacity < editor_world->capacity ||
        vm->actor_count || vm->lifetime->timers.actor_count || !world_load->rails || !world_load->rails->segments ||
        !bh_math_environment_supported())
    {
        if (world_load)
            world_load->error = 1;
        return 0;
    }
    world_load->error = 0;
    world_load->skipped = world_load->overrides = world_load->timers = world_load->immediate = 0;
    pool = editor_world->pool;
    file = bh_vfile_open(name, "rb");
    if (!file)
    {
        world_load->error = 2;
        return 0;
    }
    if (bh_vfile_seek(file, 0, SEEK_END) || ((length = bh_vfile_tell(file)) < 4) || bh_vfile_seek(file, 0, SEEK_SET) ||
        !read(file, &count, 4))
        return end(world_load, file, 2);
    if (!count)
        return end(world_load, file, 0);
    if (count < 0 || count > (length - 4) / 32)
        return end(world_load, file, 3);
    first = last = 0;
    for (i = 0; i < (unsigned long)count; ++i)
    {
        if (!read(file, model, 16) || !read(file, coordinates, 16))
            return end(world_load, file, 3);
        model[16] = 0;
        index = editor_world->library->count;
        for (j = 0; j < editor_world->library->count; ++j)
            if (!_stricmp(model, editor_world->library->blocks[j].definition.name))
            {
                index = j;
                break;
            }
        if (index == editor_world->library->count)
        {
            ++world_load->skipped;
            continue;
        }
        if (editor_world->count >= editor_world->capacity)
            return end(world_load, file, 4);
        block = editor_world->library->blocks + index;
        entity = bh_entity_acquire(pool);
        if (!entity)
            return end(world_load, file, 4);
        editor_node = (BHEditorNode *)calloc(1, sizeof(*editor_node));
        if (!editor_node)
        {
            bh_entity_release(pool, entity);
            return end(world_load, file, 4);
        }
        editor_node->entity = entity;
        editor_node->model_index = index;
        editor_node->placement.geometry.model = &block->model;
        editor_node->placement.geometry.flags = (unsigned long)coordinates[3];
        editor_node->placement.geometry.query_flags = block->definition.flags;
        entity->kind = 1;
        for (j = 0; j < 3; ++j)
        {
            editor_node->placement.coordinates[j] = coordinates[j];
            ((float *)&entity->position)[j] = rf((double)coordinates[j]);
            bh_field_store_u32(&entity->placement.coordinates[j], (unsigned long)coordinates[j]);
        }
        editor_node->placement.geometry.position = entity->position;
        bh_field_store_u32(&entity->placement.orientation_flags, editor_node->placement.geometry.flags);
        editor_node->placement.angles.x = rf((editor_node->placement.geometry.flags & 3) * 1.5707963267948966);
        entity->euler = editor_node->placement.angles;
        bh_field_store_u32(&entity->placement.editor_yaw, 0);
        bh_field_store_u32(&entity->placement.editor_pitch, 0);
        bh_field_store_u32(&entity->placement.editor_radius, 0x437a0000);
        entity->placement.actors = NULL;
        bh_field_store_u32(&entity->placement.attachments, 0);
        /* +100 is initialized here; no recovered consumer establishes its meaning. */
        setword(entity, 0x100, 0);
        entity->placement.activity_frames = 0;
        bh_field_store_u32(&entity->placement.damage, 0);
        bh_field_store_u32(&entity->placement.damage_source, 0);
        bh_field_store_u32(&entity->placement.city_id, 0xffffffffUL);
        bh_field_store_u32(&entity->placement.model_index, index);
        bh_field_store_u32(&entity->placement.radar_visited, 0);
        bh_field_store_u32(&entity->placement.placement_index, i);
        bh_field_store_u32(&entity->placement.query_flags, block->definition.flags);
        memcpy((char *)&entity->placement.script_values, block->definition.values, 16);
        entity->step = 0;
        if (!bh_editor_world_attach(editor_world, editor_node))
        {
            free(editor_node);
            bh_entity_release(pool, entity);
            return end(world_load, file, 4);
        }
        editor_node->heap_owned = 1;
        if (editor_world->numeric)
            editor_world->numeric->selected = -1;
        if (!first)
            first = editor_node;
        if (last)
        {
            editor_node->placement.geometry.previous = &last->placement.geometry;
            last->placement.geometry.navigation.next = &editor_node->placement.geometry.navigation;
            bh_field_store_u32(&entity->placement.previous, (unsigned long)last->entity);
            bh_field_store_u32(&last->entity->placement.next, (unsigned long)entity);
        }
        last = editor_node;
        if (!bh_rail_placement(world_load->rails, model, &entity->position, &editor_node->placement.angles))
            return end(world_load, file, 5);
    }
    if (!first)
        return end(world_load, file, 3);
    last->placement.geometry.navigation.next = &first->placement.geometry.navigation;
    first->placement.geometry.previous = &last->placement.geometry;
    bh_field_store_u32(&last->entity->placement.next, (unsigned long)first->entity);
    bh_field_store_u32(&first->entity->placement.previous, (unsigned long)last->entity);
    editor_world->selection = first;
    if (!bh_rail_link(world_load->rails))
        return end(world_load, file, 5);
    position = bh_vfile_tell(file);
    while (position < length)
    {
        if (length - position < 20 || !read(file, &id, 4) || !read(file, vars, 16))
            return end(world_load, file, 3);
        index = cursor;
        for (k = 0; k < editor_world->count; ++k)
        {
            memcpy(&j, &editor_world->nodes[index]->entity->placement.placement_index, 4);
            if (j == id)
                break;
            index = (index + 1) % editor_world->count;
        }
        cursor = index;
        if (k < editor_world->count)
        {
            memcpy(&editor_world->nodes[index]->entity->placement.script_values, vars, 16);
            ++world_load->overrides;
        }
        position = bh_vfile_tell(file);
    }
    vm->actor_count = editor_world->count;
    vm->lifetime->timers.actors = world_load->states;
    vm->lifetime->timers.actor_count = editor_world->count;
    for (i = 0; i < editor_world->count; ++i)
    {
        editor_node = editor_world->nodes[i];
        memset(world_load->states + i, 0, sizeof(*world_load->states));
        world_load->states[i].entity = editor_node->entity;
        world_load->states[i].active = 1;
        memcpy(world_load->states[i].variables, &editor_node->entity->placement.script_values, 16);
        actor = vm->actors + i;
        actor->state = world_load->states + i;
        actor->world = &editor_node->placement.geometry;
        actor->orientation = editor_node->placement.geometry.flags & 3;
        actor->resource = editor_world->library->blocks[editor_node->model_index].resource;
    }
    world_load->index_allocator = pool->allocator;
    if (!bh_binding_init(&world_load->actor_index, editor_world->count, &world_load->index_allocator))
        return end(world_load, file, 6);
    for (i = 0; i < editor_world->count; ++i)
        if (!bh_binding_insert(&world_load->actor_index, world_load->states[i].entity, NULL, i))
            return end(world_load, file, 6);
    vm->lifetime->timers.actor_index = &world_load->actor_index;
    for (i = 0; i < editor_world->count; ++i)
    {
        editor_node = editor_world->nodes[i];
        block = editor_world->library->blocks + editor_node->model_index;
        if (block->definition.script)
        {
            timer = bh_entity_acquire(pool);
            if (timer)
            {
                bh_script_lifetime_read(timer, &life);
                life.actor = editor_node->entity;
                life.script_loaded = vm->script_loading;
                life.sound = 0;
                if (!bh_script_lifetime_bind(vm->lifetime, timer, &life, bh_script_unloaded_query))
                    return end(world_load, file, 6);
                bh_script_timer_fields(timer)->remaining = 1;
                bh_script_timer_fields(timer)->high = 0;
                bh_script_timer_fields(timer)->countdown = 0;
                bh_script_timer_fields(timer)->fired = block->definition.script;
                timer->position = pool->origin;
                timer->step = bh_script_timer_unconditional_step();
                ++world_load->timers;
                if (block->definition.script->query == bh_script_vm_query &&
                    bh_script_vm_read(block->definition.script, &script) && script.opcode == 20)
                {
                    ++world_load->immediate;
                    timer->step(timer);
                    if (vm->error)
                        return end(world_load, file, 6);
                }
            }
        }
        bh_field_store_u32(&editor_world->nodes[(i + 1) % editor_world->count]->entity->mass, 0x47c35000UL);
    }
    if (!bh_editor_control_bind(world_load->editor, first))
        return end(world_load, file, 6);
    bh_vfile_close(file);
    editor_world->mode = 2;
    for (i = 0; i < editor_world->count; ++i)
        if (!bh_world_geometry_build(&editor_world->nodes[i]->placement.geometry, editor_world->count))
        {
            world_load->error = 7;
            return 0;
        }
    for (i = 0; i < editor_world->count; ++i)
        if (!bh_portals_match(&editor_world->nodes[i]->placement.geometry.navigation, editor_world->count))
        {
            world_load->error = 7;
            return 0;
        }
    sync(editor_world);
    for (i = 0; i < editor_world->count; ++i)
    {
        at = (BHEditorNode *)editor_world->nodes[i]->placement.geometry.owner;
        if (at)
            for (j = 0; j < editor_world->count; ++j)
                if (editor_world->nodes[j] == at)
                {
                    world_load->states[i].owner = world_load->states + j;
                    break;
                }
    }
    return 1;
}

void bh_world_load_dispose_index(BHWorldLoad *world_load)
{
    if (!world_load)
        return;
    if (world_load->vm && world_load->vm->lifetime && world_load->vm->lifetime->timers.actor_index == &world_load->actor_index)
        world_load->vm->lifetime->timers.actor_index = NULL;
    bh_binding_dispose(&world_load->actor_index, &world_load->index_allocator);
}
