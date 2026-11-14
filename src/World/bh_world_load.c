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
static int end(BHWorldLoad *c, BHVFile *f, int error)
{
    bh_vfile_close(f);
    c->error = error;
    return error == 0;
}
static void sync(BHEditorWorld *w)
{
    unsigned long i;
    BHEditorNode *n, *o;
    BHWorldGeometry *g;
    for (i = 0; i < w->count; ++i)
    {
        n = w->nodes[i];
        g = &n->placement.geometry;
        memcpy(&n->entity->placement.transform, &g->transform, 48);
        memcpy(&n->entity->placement.inverse, &g->inverse, 48);
        memcpy(&n->entity->placement.minimum, &g->minimum, 12);
        memcpy(&n->entity->placement.maximum, &g->maximum, 12);
        o = (BHEditorNode *)g->owner;
        bh_field_store_u32(&n->entity->placement.owner, (unsigned long)(o ? o->entity : 0));
    }
}
int bh_world_load(BHWorldLoad *c, const char *name)
{
    BHEditorWorld *w;
    BHScriptVMContext *vm;
    BHEntityPool *pool;
    BHVFile *file;
    BHEditorNode *n, *first, *last, *at;
    BHScriptBlock *block;
    BHEntity *e, *timer;
    BHScriptLifetimeState life;
    BHScriptVMState script;
    BHScriptVMActor *actor;
    long count, length, position, coordinates[4];
    unsigned long i, j, k, index, id, cursor = 0;
    char model[17];
    short vars[8];
    if (!c || !name || !c->editor || !(w = c->editor->world) || !w->pool || !w->library || !w->library->blocks ||
        !w->nodes || w->count || !w->capacity || w->capacity > 65535 || !(vm = c->vm) || !vm->lifetime ||
        vm->lifetime->timers.pool != w->pool || !vm->actors || !c->states || c->actor_capacity < w->capacity ||
        vm->actor_count || vm->lifetime->timers.actor_count || !c->rails || !c->rails->segments ||
        !bh_math_environment_supported())
    {
        if (c)
            c->error = 1;
        return 0;
    }
    c->error = 0;
    c->skipped = c->overrides = c->timers = c->immediate = 0;
    pool = w->pool;
    file = bh_vfile_open(name, "rb");
    if (!file)
    {
        c->error = 2;
        return 0;
    }
    if (bh_vfile_seek(file, 0, SEEK_END) || ((length = bh_vfile_tell(file)) < 4) || bh_vfile_seek(file, 0, SEEK_SET) ||
        !read(file, &count, 4))
        return end(c, file, 2);
    if (!count)
        return end(c, file, 0);
    if (count < 0 || count > (length - 4) / 32)
        return end(c, file, 3);
    first = last = 0;
    for (i = 0; i < (unsigned long)count; ++i)
    {
        if (!read(file, model, 16) || !read(file, coordinates, 16))
            return end(c, file, 3);
        model[16] = 0;
        index = w->library->count;
        for (j = 0; j < w->library->count; ++j)
            if (!_stricmp(model, w->library->blocks[j].definition.name))
            {
                index = j;
                break;
            }
        if (index == w->library->count)
        {
            ++c->skipped;
            continue;
        }
        if (w->count >= w->capacity)
            return end(c, file, 4);
        block = w->library->blocks + index;
        e = bh_entity_acquire(pool);
        if (!e)
            return end(c, file, 4);
        n = (BHEditorNode *)calloc(1, sizeof(*n));
        if (!n)
        {
            bh_entity_release(pool, e);
            return end(c, file, 4);
        }
        n->entity = e;
        n->model_index = index;
        n->placement.geometry.model = &block->model;
        n->placement.geometry.flags = (unsigned long)coordinates[3];
        n->placement.geometry.query_flags = block->definition.flags;
        e->kind = 1;
        for (j = 0; j < 3; ++j)
        {
            n->placement.coordinates[j] = coordinates[j];
            ((float *)&e->position)[j] = rf((double)coordinates[j]);
            bh_field_store_u32(&e->placement.coordinates[j], (unsigned long)coordinates[j]);
        }
        n->placement.geometry.position = e->position;
        bh_field_store_u32(&e->placement.orientation_flags, n->placement.geometry.flags);
        n->placement.angles.x = rf((n->placement.geometry.flags & 3) * 1.5707963267948966);
        e->euler = n->placement.angles;
        bh_field_store_u32(&e->placement.editor_yaw, 0);
        bh_field_store_u32(&e->placement.editor_pitch, 0);
        bh_field_store_u32(&e->placement.editor_radius, 0x437a0000);
        e->placement.actors = NULL;
        bh_field_store_u32(&e->placement.attachments, 0);
        /* +100 is initialized here; no recovered consumer establishes its meaning. */
        setword(e, 0x100, 0);
        e->placement.activity_frames = 0;
        bh_field_store_u32(&e->placement.damage, 0);
        bh_field_store_u32(&e->placement.damage_source, 0);
        bh_field_store_u32(&e->placement.city_id, 0xffffffffUL);
        bh_field_store_u32(&e->placement.model_index, index);
        bh_field_store_u32(&e->placement.radar_visited, 0);
        bh_field_store_u32(&e->placement.placement_index, i);
        bh_field_store_u32(&e->placement.query_flags, block->definition.flags);
        memcpy((char *)&e->placement.script_values, block->definition.values, 16);
        e->step = 0;
        if (!bh_editor_world_attach(w, n))
        {
            free(n);
            bh_entity_release(pool, e);
            return end(c, file, 4);
        }
        n->heap_owned = 1;
        if (w->numeric)
            w->numeric->selected = -1;
        if (!first)
            first = n;
        if (last)
        {
            n->placement.geometry.previous = &last->placement.geometry;
            last->placement.geometry.navigation.next = &n->placement.geometry.navigation;
            bh_field_store_u32(&e->placement.previous, (unsigned long)last->entity);
            bh_field_store_u32(&last->entity->placement.next, (unsigned long)e);
        }
        last = n;
        if (!bh_rail_placement(c->rails, model, &e->position, &n->placement.angles))
            return end(c, file, 5);
    }
    if (!first)
        return end(c, file, 3);
    last->placement.geometry.navigation.next = &first->placement.geometry.navigation;
    first->placement.geometry.previous = &last->placement.geometry;
    bh_field_store_u32(&last->entity->placement.next, (unsigned long)first->entity);
    bh_field_store_u32(&first->entity->placement.previous, (unsigned long)last->entity);
    w->selection = first;
    if (!bh_rail_link(c->rails))
        return end(c, file, 5);
    position = bh_vfile_tell(file);
    while (position < length)
    {
        if (length - position < 20 || !read(file, &id, 4) || !read(file, vars, 16))
            return end(c, file, 3);
        index = cursor;
        for (k = 0; k < w->count; ++k)
        {
            memcpy(&j, &w->nodes[index]->entity->placement.placement_index, 4);
            if (j == id)
                break;
            index = (index + 1) % w->count;
        }
        cursor = index;
        if (k < w->count)
        {
            memcpy(&w->nodes[index]->entity->placement.script_values, vars, 16);
            ++c->overrides;
        }
        position = bh_vfile_tell(file);
    }
    vm->actor_count = w->count;
    vm->lifetime->timers.actors = c->states;
    vm->lifetime->timers.actor_count = w->count;
    for (i = 0; i < w->count; ++i)
    {
        n = w->nodes[i];
        memset(c->states + i, 0, sizeof(*c->states));
        c->states[i].entity = n->entity;
        c->states[i].active = 1;
        memcpy(c->states[i].variables, &n->entity->placement.script_values, 16);
        actor = vm->actors + i;
        actor->state = c->states + i;
        actor->world = &n->placement.geometry;
        actor->orientation = n->placement.geometry.flags & 3;
        actor->resource = w->library->blocks[n->model_index].resource;
    }
    c->index_allocator = pool->allocator;
    if (!bh_binding_init(&c->actor_index, w->count, &c->index_allocator))
        return end(c, file, 6);
    for (i = 0; i < w->count; ++i)
        if (!bh_binding_insert(&c->actor_index, c->states[i].entity, NULL, i))
            return end(c, file, 6);
    vm->lifetime->timers.actor_index = &c->actor_index;
    for (i = 0; i < w->count; ++i)
    {
        n = w->nodes[i];
        block = w->library->blocks + n->model_index;
        if (block->definition.script)
        {
            timer = bh_entity_acquire(pool);
            if (timer)
            {
                bh_script_lifetime_read(timer, &life);
                life.actor = n->entity;
                life.script_loaded = vm->script_loading;
                life.sound = 0;
                if (!bh_script_lifetime_bind(vm->lifetime, timer, &life, bh_script_unloaded_query))
                    return end(c, file, 6);
                bh_script_timer_fields(timer)->remaining = 1;
                bh_script_timer_fields(timer)->high = 0;
                bh_script_timer_fields(timer)->countdown = 0;
                bh_script_timer_fields(timer)->fired = block->definition.script;
                timer->position = pool->origin;
                timer->step = bh_script_timer_unconditional_step();
                ++c->timers;
                if (block->definition.script->query == bh_script_vm_query &&
                    bh_script_vm_read(block->definition.script, &script) && script.opcode == 20)
                {
                    ++c->immediate;
                    timer->step(timer);
                    if (vm->error)
                        return end(c, file, 6);
                }
            }
        }
        bh_field_store_u32(&w->nodes[(i + 1) % w->count]->entity->mass, 0x47c35000UL);
    }
    if (!bh_editor_control_bind(c->editor, first))
        return end(c, file, 6);
    bh_vfile_close(file);
    w->mode = 2;
    for (i = 0; i < w->count; ++i)
        if (!bh_world_geometry_build(&w->nodes[i]->placement.geometry, w->count))
        {
            c->error = 7;
            return 0;
        }
    for (i = 0; i < w->count; ++i)
        if (!bh_portals_match(&w->nodes[i]->placement.geometry.navigation, w->count))
        {
            c->error = 7;
            return 0;
        }
    sync(w);
    for (i = 0; i < w->count; ++i)
    {
        at = (BHEditorNode *)w->nodes[i]->placement.geometry.owner;
        if (at)
            for (j = 0; j < w->count; ++j)
                if (w->nodes[j] == at)
                {
                    c->states[i].owner = c->states + j;
                    break;
                }
    }
    return 1;
}

void bh_world_load_dispose_index(BHWorldLoad *c)
{
    if (!c)
        return;
    if (c->vm && c->vm->lifetime && c->vm->lifetime->timers.actor_index == &c->actor_index)
        c->vm->lifetime->timers.actor_index = NULL;
    bh_binding_dispose(&c->actor_index, &c->index_allocator);
}
