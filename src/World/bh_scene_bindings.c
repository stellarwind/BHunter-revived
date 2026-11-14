#include "bh_scene_bindings.h"
#include <string.h>

static int fail(BHSceneBindings *scene_bindings, BHSceneBindingsError error)
{
    if (scene_bindings)
        scene_bindings->error = error;
    return 0;
}
static long find_world(const BHSceneBindingInput *inputs, unsigned long count, const BHWorldGeometry *world)
{
    unsigned long i;
    for (i = 0; i < count; ++i)
        if (inputs[i].world == world)
            return (long)i;
    return -1;
}
int bh_scene_bindings_init(BHSceneBindings *scene_bindings, const BHSceneBindingInput *inputs, unsigned long count,
                           BHPickupRoom *rooms, BHEntityScheduleRoom *scheduled, BHRenderResources *resources,
                           BHEntityScheduleContext *context)
{
    BHWorldGeometry *cursor;
    unsigned long i, j, roots = 0;
    if (!scene_bindings || !inputs || !count || count > 65535UL || !rooms || !scheduled || !resources || !context ||
        !context->queue || !context->viewport || !context->camera || !resources->models || scene_bindings->rooms || scene_bindings->scheduled ||
        scene_bindings->scene_count)
        return fail(scene_bindings, BH_BINDINGS_ARGUMENT);
    cursor = inputs[0].world;
    for (i = 0; i < count; ++i)
    {
        const BHSceneBindingInput *item = inputs + i;
        BHWorldGeometry *world = item->world;
        if (!world || world != cursor || !world->navigation.next || !world->previous ||
            world->previous->navigation.next != &world->navigation || world->navigation.count > 6)
            return fail(scene_bindings, BH_BINDINGS_TOPOLOGY);
        for (j = 0; j < i; ++j)
            if (inputs[j].world == world)
                return fail(scene_bindings, BH_BINDINGS_TOPOLOGY);
        if (world->owner)
        {
            if (world->owner == world || find_world(inputs, count, world->owner) < 0 || world->owner->owner)
                return fail(scene_bindings, BH_BINDINGS_TOPOLOGY);
        }
        else
            ++roots;
        if ((world->query_flags & 4) && !bh_render_resources_lookup(item->resource, resources))
            return fail(scene_bindings, BH_BINDINGS_RESOURCE);
        for (j = 0; j < world->navigation.count; ++j)
        {
            const BHPortalNode *neighbor = world->navigation.portals[j].neighbor;
            if (neighbor && find_world(inputs, count, (const BHWorldGeometry *)neighbor) < 0)
                return fail(scene_bindings, BH_BINDINGS_TOPOLOGY);
        }
        cursor = (BHWorldGeometry *)world->navigation.next;
    }
    if (cursor != inputs[0].world || !roots)
        return fail(scene_bindings, BH_BINDINGS_TOPOLOGY);
    memset(rooms, 0, count * sizeof(*rooms));
    memset(scheduled, 0, count * sizeof(*scheduled));
    for (i = 0; i < count; ++i)
    {
        const BHSceneBindingInput *item = inputs + i;
        rooms[i].world = item->world;
        rooms[i].actors = item->actors;
        rooms[i].pickups = item->pickups;
        scheduled[i].room = rooms + i;
        scheduled[i].resource = item->resource;
        scheduled[i].resource_flags = item->resource_flags;
        scheduled[i].angles = item->angles;
        scheduled[i].linked_active = item->world->owner != NULL;
        scheduled[i].next =
            scheduled + find_world(inputs, count, (const BHWorldGeometry *)item->world->navigation.next);
    }
    for (i = 0; i < count; ++i)
    {
        const BHWorldGeometry *world_geometry = inputs[i].world;
        for (j = 0; j < world_geometry->navigation.count; ++j)
        {
            const BHPortalNode *neighbor = world_geometry->navigation.portals[j].neighbor;
            if (neighbor)
                scheduled[i].portal_targets[j] =
                    scheduled + find_world(inputs, count, (const BHWorldGeometry *)neighbor);
        }
    }
    scene_bindings->first = inputs[0].world;
    scene_bindings->cached = NULL;
    scene_bindings->scene_count = count;
    scene_bindings->rooms = rooms;
    scene_bindings->scheduled = scheduled;
    scene_bindings->schedule_context = context;
    scene_bindings->error = BH_BINDINGS_OK;
    return 1;
}
BHEntityScheduleRoom *bh_scene_bindings_schedule(const BHSceneBindings *scene_bindings, const BHWorldGeometry *world)
{
    unsigned long i;
    if (!scene_bindings || !world || !scene_bindings->rooms || !scene_bindings->scheduled)
        return NULL;
    for (i = 0; i < scene_bindings->scene_count; ++i)
        if (scene_bindings->rooms[i].world == world)
            return scene_bindings->scheduled + i;
    return NULL;
}
void bh_scene_bindings_dispose(BHSceneBindings *b)
{
    if (b)
        memset(b, 0, sizeof(*b));
}
