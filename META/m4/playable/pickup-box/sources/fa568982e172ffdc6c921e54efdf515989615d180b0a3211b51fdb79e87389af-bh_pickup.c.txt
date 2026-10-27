#include "bh_pickup.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char pickup_state_size[sizeof(BHPickupState)==56 ? 1:-1];
typedef char pickup_state_offset[offsetof(BHEntity,payload)+8==0xec ? 1:-1];
typedef char pickup_context_offset[offsetof(BHEntity,payload)+64==0x124 ? 1:-1];
typedef char pickup_pointer_size[sizeof(void*)==4 ? 1:-1];
/* Event13 is seven words: original004415a0 consumes the reply written by
   original0041d7c0/0043a1d0 at +14/+18. There is no reserved sixth word. */
typedef char pickup_reply_layout[
    sizeof(BHPickupTakePacket)==28 &&
    offsetof(BHPickupTakePacket,accepted)==0x14 &&
    offsetof(BHPickupTakePacket,consumed)==0x18 ? 1:-1];

BHPickupContext *__cdecl bh_pickup_context(BHEntity *entity) {
    BHPickupContext *context;
    if(!entity)return NULL;
    memcpy(&context,entity->payload+64,sizeof(context));
    return context;
}
static void bind_owner(BHEntity *entity,BHPickupContext *context) {
    memcpy(entity->payload+64,&context,sizeof(context));
}
BHPickupState *__cdecl bh_pickup_state(BHEntity *entity) {
    return entity ? (BHPickupState *)(entity->payload+8) : NULL;
}
static BHPickupRoom *room_binding(BHPickupContext *context,BHWorldGeometry *world) {
    unsigned long i;
    for(i=0;i<context->room_count;i++)if(context->rooms[i].world==world)return context->rooms+i;
    return NULL;
}
static int live(BHPickupContext *context,BHEntity *entity,unsigned long generation) {
    BHEntity *at;unsigned long i=0;
    for(at=context->pool->active;at && i++<context->pool->capacity;at=at->next)
        if(at==entity)return at->generation==generation;
    return 0;
}
static void unlink_pickup(BHPickupContext *context,BHEntity *entity) {
    BHPickupState *state=bh_pickup_state(entity);
    BHPickupRoom *room=state->room?room_binding(context,state->room):NULL;
    if(room){
        if(state->room_next)bh_pickup_state(state->room_next)->room_previous=state->room_previous;
        if(state->room_previous)bh_pickup_state(state->room_previous)->room_next=state->room_next;
        else room->pickups=state->room_next;
    }
    if(state->global_next)bh_pickup_state(state->global_next)->global_previous=state->global_previous;
    if(state->global_previous)bh_pickup_state(state->global_previous)->global_next=state->global_next;
    else context->head=state->global_next;
    state->room_next=state->room_previous=state->global_next=state->global_previous=NULL;
    state->room=NULL;
}
void __cdecl bh_pickup_cleanup(BHEntity *entity) {
    BHPickupContext *context=bh_pickup_context(entity);
    if(!context || !context->pool)return;
    unlink_pickup(context,entity);
    bh_entity_release(context->pool,entity);
}
void __cdecl bh_pickup_query(BHEntity *entity,void *packet) {
    long event;
    BHPickupContext *context;
    if(!entity || !packet)return;
    memcpy(&event,packet,sizeof(event));
    if(event!=20)return;
    context=bh_pickup_context(entity);
    entity->step=bh_pickup_cleanup;
    entity->refresh=NULL; /* original 0047d680 RET callback */
    if(context && context->pool)bh_entity_set_query(context->pool,entity,NULL);
}
static void play_take(BHPickupContext *context,BHEntity *entity) {
    if(context->sound)bh_sound_request(context->sound,context->take_sound,
        1.0f,1.0f,&entity->position,&context->sound_velocity);
}
static BHEntity *actor_next(BHEntity *entity) {
    BHEntity *next;
    memcpy(&next,entity->payload+16,sizeof(next)); /* actor +f4 */
    return next;
}
static long actor_kind(BHEntity *entity) {
    long kind;
    memcpy(&kind,entity->payload+24,sizeof(kind)); /* actor +fc */
    return kind;
}
/* A successful event 0xd may mutate the current pickup through actor callbacks;
   the pool generation check prevents writing through a recycled slot. */
static int offer_to(BHPickupContext *context,BHEntity *pickup,BHEntity *actor,
                    int locate_actor,int *accepted_sound) {
    BHPickupState *state=bh_pickup_state(pickup);
    BHEntityPositionPacket position;
    BHPickupTakePacket packet;
    BHVec3 point=actor->position;
    float dx,dy,dz;
    unsigned long generation=pickup->generation;
    if(!actor->query)return 0;
    if(locate_actor){
        position.event=3;position.position=actor->position;
        actor->query(actor,&position);
        point=position.position;
    }
    dx=point.x-pickup->position.x;dy=point.y-pickup->position.y;dz=point.z-pickup->position.z;
    if((double)dx*dx+(double)dy*dy+(double)dz*dz>=36.0)return 0;
    memset(&packet,0,sizeof(packet));packet.event=13;
    packet.type=state->type;packet.variant=state->variant;
    packet.amount=state->offer;packet.token=state->token;
    actor->query(actor,&packet);
    if(!live(context,pickup,generation))return -1;
    if(!packet.accepted)return 0;
    if(packet.consumed)*accepted_sound=1;
    if(state->remaining){
        state->remaining-=packet.consumed;
        if(state->remaining<state->offer)state->offer=state->remaining;
        if(state->remaining==0){
            if(*accepted_sound)play_take(context,pickup);
            bh_pickup_cleanup(pickup);
            return 1;
        }
    }
    return 0;
}
void __cdecl bh_pickup_tick(BHEntity *entity) {
    BHPickupContext *context=bh_pickup_context(entity);
    BHPickupState *state=bh_pickup_state(entity);
    BHPickupRoom *room;
    BHEntity *actor;
    int audible=0,result;
    unsigned long i;
    if(!context || !context->pool)return;
    bh_round_float(&state->phase,(double)state->phase-1.0/30.0);
    bh_round_float(&state->rotation,(double)state->rotation+1.0/30.0);
    if(state->phase>0.0f)return;
    if(state->fade>0.0f){
        bh_round_float(&state->fade,(double)state->fade-0.25);
        if(state->fade<=0.0f){bh_pickup_cleanup(entity);return;}
    }
    bh_round_float(&state->phase,(double)state->phase+0.25);
    room=state->room?room_binding(context,state->room):NULL;
    for(actor=room?room->actors:NULL;actor;actor=actor_next(actor)){
        long kind=actor_kind(actor);
        if(kind!=1 && kind!=2 && kind!=8)continue;
        result=offer_to(context,entity,actor,1,&audible);
        if(result)return;
    }
    if(context->outside_actors)for(i=0;context->outside_actors[i];i++){
        result=offer_to(context,entity,context->outside_actors[i],0,&audible);
        if(result)return;
    }
    if(audible){
        if(state->token)bh_round_float(&state->phase,(double)state->phase+1.0);
        play_take(context,entity);
    }
}
BHEntity *__cdecl bh_pickup_spawn(BHPickupContext *context,const BHVec3 *position,
    long type,long variant,long remaining,long offer,long token,float fade) {
    BHEntity *entity;
    BHPickupState *state;
    BHPickupRoom *room;
    BHWorldGeometry *world;
    unsigned long index;
    int first_random,second_random;
    if(!context || !context->pool || !position || !context->resources ||
       !context->first_world || !context->world_count ||
       (context->room_count && !context->rooms) || type<0 || type>=10 || variant<0 ||
       (unsigned long)variant>context->resource_count/10 ||
       (unsigned long)variant*10+(unsigned long)type>=context->resource_count)return NULL;
    index=(unsigned long)variant*10+(unsigned long)type;
    if(!context->resources[index]){variant=0;index=(unsigned long)type;}
    if(!((variant==0 && type==9)?context->special_draw:context->default_draw))return NULL;
    entity=bh_entity_acquire(context->pool);
    if(!entity)return NULL;
    state=bh_pickup_state(entity);memset(state,0,sizeof(*state));
    bind_owner(entity,context);
    state->resource=context->resources[index];
    first_random=rand();second_random=rand();
    bh_round_float(&state->phase,(double)first_random*(1.0/131072.0));
    bh_round_float(&state->rotation,(double)second_random*0.0001953125);
    state->fade=fade;state->type=type;state->variant=variant;
    state->remaining=remaining;state->offer=offer;
    if(remaining && remaining<offer)state->offer=remaining;
    state->token=token;
    world=bh_world_locate(context->first_world,context->world_count,NULL,position,0.0f);
    room=world?room_binding(context,world):NULL;
    if(world && !room){bh_entity_release(context->pool,entity);return NULL;}
    state->room=world;entity->position=*position;
    entity->step=bh_pickup_tick;entity->refresh=NULL;
    if(!bh_entity_set_query(context->pool,entity,bh_pickup_query)){
        bh_entity_release(context->pool,entity);return NULL;
    }
    {BHEntityStep draw=(variant==0 && type==9)?context->special_draw:context->default_draw;
     memcpy(entity->payload+4,&draw,sizeof(draw));} /* entity +e8 */
    if(room){state->room_next=room->pickups;room->pickups=entity;
        if(state->room_next)bh_pickup_state(state->room_next)->room_previous=entity;}
    state->global_next=context->head;context->head=entity;
    if(state->global_next)bh_pickup_state(state->global_next)->global_previous=entity;
    return entity;
}
