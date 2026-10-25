#include "bh_pickup_basic.h"
#include "bh_pickup_draw.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char basic_pickup_layout[
    sizeof(void*)==4 && sizeof(BHPickupState)==56 &&
    offsetof(BHEntity,payload)+8==0xec &&
    offsetof(BHEntity,payload)+64==0x124 ? 1:-1];

static BHPickupRoom *find_room(BHPickupContext *context,BHWorldGeometry *world) {
    unsigned long i;
    for(i=0;i<context->room_count;++i)
        if(context->rooms[i].world==world)return context->rooms+i;
    return NULL;
}

BHEntity *__cdecl bh_pickup_basic_spawn(BHPickupContext *context,
    const BHVec3 *position,long type,long remaining,long offer,long token,float fade) {
    BHEntity *entity;
    BHPickupState *state;
    BHWorldGeometry *world;
    BHPickupRoom *room;
    BHEntityStep draw=bh_pickup_default_step;
    int random_value;

    /* These checks validate the explicit source binding before side effects. */
    if(!context || !context->pool || !position || !context->resources ||
       !context->first_world || !context->world_count ||
       (context->room_count && !context->rooms) || type<0 || type>=10 ||
       (unsigned long)type>=context->resource_count ||
       context->default_draw!=bh_pickup_default_step || !context->draw_user)return NULL;
    entity=bh_entity_acquire(context->pool);
    if(!entity)return NULL;
    state=bh_pickup_state(entity);
    memset(state,0,sizeof(*state)); /* Clears otherwise unwritten no-room links. */
    memcpy(entity->payload+64,&context,sizeof(context));
    state->resource=context->resources[type];
    random_value=rand();
    memcpy(&state->fade,&fade,sizeof(fade)); /* Original integer MOV, not FP load. */
    bh_round_float(&state->phase,(double)random_value*(1.0/131072.0));
    random_value=rand();
    state->type=type;
    state->variant=0;
    state->remaining=remaining;
    state->offer=offer;
    bh_round_float(&state->rotation,(double)random_value*0.0001953125);
    if(remaining && remaining<offer)state->offer=remaining;
    state->token=token;

    world=bh_world_locate(context->first_world,context->world_count,NULL,position,0);
    room=world?find_room(context,world):NULL;
    if(world && !room){bh_entity_release(context->pool,entity);return NULL;}
    state->room=world;
    memcpy(&entity->position,position,sizeof(*position));
    entity->step=bh_pickup_tick;
    entity->refresh=NULL; /* Verified shared RET sentinel. */
    if(!bh_entity_set_query(context->pool,entity,bh_pickup_query)){
        bh_entity_release(context->pool,entity);return NULL;
    }
    memcpy(entity->payload+4,&draw,sizeof(draw));
    if(room){
        state->room_next=room->pickups;
        room->pickups=entity;
        if(state->room_next)bh_pickup_state(state->room_next)->room_previous=entity;
    }
    state->global_next=context->head;
    context->head=entity;
    if(state->global_next)bh_pickup_state(state->global_next)->global_previous=entity;
    return entity;
}
