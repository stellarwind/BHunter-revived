#include "bh_scene_bindings.h"
#include <string.h>

static int fail(BHSceneBindings *b,BHSceneBindingsError error) {
    if(b)b->error=error;
    return 0;
}
static long find_world(const BHSceneBindingInput *inputs,unsigned long count,
                       const BHWorldGeometry *world) {
    unsigned long i;
    for(i=0;i<count;++i)if(inputs[i].world==world)return (long)i;
    return -1;
}
int __cdecl bh_scene_bindings_init(BHSceneBindings *b,
    const BHSceneBindingInput *inputs,unsigned long count,
    BHPickupRoom *rooms,BHEntityScheduleRoom *scheduled,
    BHRenderResources *resources,BHEntityScheduleContext *context) {
    BHWorldGeometry *cursor;unsigned long i,j,roots=0;
    if(!b || !inputs || !count || count>65535UL || !rooms || !scheduled ||
       !resources || !context || !context->queue || !context->viewport ||
       !context->camera || !resources->models || b->rooms || b->scheduled ||
       b->scene_count)return fail(b,BH_BINDINGS_ARGUMENT);
    cursor=inputs[0].world;
    for(i=0;i<count;++i) {
        const BHSceneBindingInput *item=inputs+i;
        BHWorldGeometry *w=item->world;
        if(!w || w!=cursor || !w->navigation.next || !w->previous ||
           w->previous->navigation.next!=&w->navigation ||
           w->navigation.count>6)
            return fail(b,BH_BINDINGS_TOPOLOGY);
        for(j=0;j<i;++j)if(inputs[j].world==w)return fail(b,BH_BINDINGS_TOPOLOGY);
        if(w->owner) {
            if(w->owner==w || find_world(inputs,count,w->owner)<0 ||
               w->owner->owner)return fail(b,BH_BINDINGS_TOPOLOGY);
        } else ++roots;
        if((w->query_flags&4) &&
           !bh_render_resources_lookup(item->resource,resources))
            return fail(b,BH_BINDINGS_RESOURCE);
        for(j=0;j<w->navigation.count;++j) {
            const BHPortalNode *neighbor=w->navigation.portals[j].neighbor;
            if(neighbor && find_world(inputs,count,(const BHWorldGeometry *)neighbor)<0)
                return fail(b,BH_BINDINGS_TOPOLOGY);
        }
        cursor=(BHWorldGeometry *)w->navigation.next;
    }
    if(cursor!=inputs[0].world || !roots)return fail(b,BH_BINDINGS_TOPOLOGY);
    memset(rooms,0,count*sizeof(*rooms));
    memset(scheduled,0,count*sizeof(*scheduled));
    for(i=0;i<count;++i) {
        const BHSceneBindingInput *item=inputs+i;
        rooms[i].world=item->world;
        rooms[i].actors=item->actors;rooms[i].pickups=item->pickups;
        scheduled[i].room=rooms+i;
        scheduled[i].resource=item->resource;
        scheduled[i].resource_flags=item->resource_flags;
        scheduled[i].angles=item->angles;
        scheduled[i].linked_active=item->world->owner!=NULL;
        scheduled[i].next=scheduled+find_world(inputs,count,
            (const BHWorldGeometry *)item->world->navigation.next);
    }
    for(i=0;i<count;++i) {
        const BHWorldGeometry *w=inputs[i].world;
        for(j=0;j<w->navigation.count;++j) {
            const BHPortalNode *neighbor=w->navigation.portals[j].neighbor;
            if(neighbor)scheduled[i].portal_targets[j]=scheduled+
                find_world(inputs,count,(const BHWorldGeometry *)neighbor);
        }
    }
    b->first=inputs[0].world;b->cached=NULL;b->scene_count=count;
    b->rooms=rooms;b->scheduled=scheduled;b->schedule_context=context;
    b->error=BH_BINDINGS_OK;
    return 1;
}
BHEntityScheduleRoom *__cdecl bh_scene_bindings_schedule(
    const BHSceneBindings *b,const BHWorldGeometry *world) {
    unsigned long i;
    if(!b || !world || !b->rooms || !b->scheduled)return NULL;
    for(i=0;i<b->scene_count;++i)
        if(b->rooms[i].world==world)return b->scheduled+i;
    return NULL;
}
void __cdecl bh_scene_bindings_dispose(BHSceneBindings *b) {
    if(b)memset(b,0,sizeof(*b));
}
