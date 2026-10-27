#include "bh_world_load.h"
#include "bh_vfile.h"
#include "bh_round.h"
#include "bh_fp.h"
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
        memcpy((char *)n->entity + 0x150, &g->transform, 48);
        memcpy((char *)n->entity + 0x180, &g->inverse, 48);
        memcpy((char *)n->entity + 0x1b0, &g->minimum, 12);
        memcpy((char *)n->entity + 0x1bc, &g->maximum, 12);
        o = (BHEditorNode *)g->owner;
        setword(n->entity, 0x1c8, (unsigned long)(o ? o->entity : 0));
    }
}
int __cdecl bh_world_load(BHWorldLoad *c, const char *name)
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
            setword(e, 0xe8 + 4 * j, (unsigned long)coordinates[j]);
        }
        n->placement.geometry.position = e->position;
        setword(e, 0xf4, n->placement.geometry.flags);
        n->placement.angles.x = rf((n->placement.geometry.flags & 3) * 1.5707963267948966);
        memcpy((char *)e + 0x3c, &n->placement.angles, 12);
        setword(e, 0x134, 0);
        setword(e, 0x138, 0);
        setword(e, 0x13c, 0x437a0000);
        setword(e, 0x128, 0);
        setword(e, 0x12c, 0);
        setword(e, 0x100, 0);
        setword(e, 0x104, 0);
        setword(e, 0x148, 0);
        setword(e, 0x14c, 0);
        setword(e, 0x11c, 0xffffffffUL);
        setword(e, 0xf8, index);
        setword(e, 0xfc, 0);
        setword(e, 0x118, i);
        setword(e, 0x114, block->definition.flags);
        memcpy((char *)e + 0x334, block->definition.values, 16);
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
            setword(e, 0x120, (unsigned long)last->entity);
            setword(last->entity, 0x124, (unsigned long)e);
        }
        last = n;
        if (!bh_rail_placement(c->rails, model, &e->position, &n->placement.angles))
            return end(c, file, 5);
    }
    if (!first)
        return end(c, file, 3);
    last->placement.geometry.navigation.next = &first->placement.geometry.navigation;
    first->placement.geometry.previous = &last->placement.geometry;
    setword(last->entity, 0x124, (unsigned long)first->entity);
    setword(first->entity, 0x120, (unsigned long)last->entity);
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
            memcpy(&j, (char *)w->nodes[index]->entity + 0x118, 4);
            if (j == id)
                break;
            index = (index + 1) % w->count;
        }
        cursor = index;
        if (k < w->count)
        {
            memcpy((char *)w->nodes[index]->entity + 0x334, vars, 16);
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
        memcpy(c->states[i].variables, (char *)n->entity + 0x334, 16);
        actor = vm->actors + i;
        actor->state = c->states + i;
        actor->world = &n->placement.geometry;
        actor->orientation = n->placement.geometry.flags & 3;
        actor->resource = w->library->blocks[n->model_index].resource;
    }
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
                setword(timer, 0xf8, 1);
                setword(timer, 0x100, 0);
                setword(timer, 0x114, 0);
                setword(timer, 0xe8, (unsigned long)block->definition.script);
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
        setword(w->nodes[(i + 1) % w->count]->entity, 0xb4, 0x47c35000UL);
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
