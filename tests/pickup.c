#include "../src/bh_pickup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;printf("pickup check failed at %d\n",__LINE__);}}while(0)
static unsigned long offered,positioned;
/* Test callback identity only; native rendering is exercised separately. */
static void __cdecl draw_probe(BHEntity *entity){(void)entity;}
static long take,accepted_response;
static void __cdecl actor_query(BHEntity *actor,void *raw) {
    long event=*(long*)raw;(void)actor;
    if(event==3){BHEntityPositionPacket *p=(BHEntityPositionPacket*)raw;
        ++positioned;p->position=actor->position;
    }else if(event==13){BHPickupTakePacket *p=(BHPickupTakePacket*)raw;
        ++offered;CHECK(p->type==2 && p->variant==0 && p->token==7);
        p->accepted=accepted_response;p->consumed=take;
    }
}
static void actor_link(BHEntity *actor,BHEntity *next,long kind) {
    memcpy(actor->payload+16,&next,4);memcpy(actor->payload+24,&kind,4);
}
int bh_pickup_checkpoint(unsigned long *count) {
    BHEntityPool pool;BHWorldGeometry world;BHPickupRoom room;
    BHPickupContext context;BHEntity *a,*b,*pickup,*second,*third;
    BHEntity *outside[2];BHPickupState *s;BHVec3 point={0,0,0};
    BHVec3 remote={30,0,0};unsigned long resources[20];
    int random_one,random_two,random_three;
    checks=0;failures=0;
    memset(&world,0,sizeof(world));memset(&room,0,sizeof(room));
    memset(&context,0,sizeof(context));memset(resources,0,sizeof(resources));
    world.navigation.next=(BHPortalNode*)&world;
    world.minimum.x=world.minimum.y=world.minimum.z=-10;
    world.maximum.x=world.maximum.y=world.maximum.z=10;
    room.world=&world;context.first_world=&world;context.world_count=1;
    context.rooms=&room;context.room_count=1;context.resources=resources;
    context.draw_user=&room;context.default_draw=draw_probe;context.special_draw=draw_probe;
    context.resource_count=20;resources[2]=91;resources[12]=0;
    CHECK(bh_entities_init(&pool,8,NULL));context.pool=&pool;
    context.default_draw=NULL;
    CHECK(!bh_pickup_spawn(&context,&point,2,0,1,1,7,0) && pool.used==0);
    context.default_draw=draw_probe;
    a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);CHECK(a && b);
    CHECK(bh_entity_set_query(&pool,a,actor_query));
    CHECK(bh_entity_set_query(&pool,b,actor_query));
    actor_link(a,b,1);actor_link(b,NULL,3);room.actors=a;
    srand(321);random_one=rand();random_two=rand();random_three=rand();
    srand(321);pickup=bh_pickup_spawn(&context,&point,2,1,3,5,7,0);
    CHECK(pickup && rand()==random_three);
    s=bh_pickup_state(pickup);
    CHECK(bh_pickup_context(pickup)==&context &&
          bh_pickup_context(pickup)->draw_user==&room);
    CHECK(s->phase==(float)((double)random_one*(1.0/131072.0)));
    CHECK(s->rotation>(float)((double)random_two*0.0001953125)-0.000001f &&
          s->rotation<(float)((double)random_two*0.0001953125)+0.000001f);
    CHECK(s->type==2 && s->variant==0 && s->resource==91);
    CHECK(s->remaining==3 && s->offer==3 && s->token==7);
    CHECK(s->room==&world && room.pickups==pickup && context.head==pickup);
    CHECK(pickup->step==bh_pickup_tick && pickup->query==bh_pickup_query && !pickup->refresh);
    second=bh_pickup_spawn(&context,&point,2,0,1,1,7,0);
    CHECK(second && room.pickups==second && context.head==second);
    CHECK(bh_pickup_state(second)->room_next==pickup && s->room_previous==second);
    /* Explicit query event 20 changes to cleanup; next pool tick unlinks. */
    {long event=20;bh_pickup_query(second,&event);}
    CHECK(second->step==bh_pickup_cleanup && !second->query && pool.query_head==pickup);
    bh_pickup_cleanup(second);
    CHECK(room.pickups==pickup && context.head==pickup && pool.used==3);
    /* First tick reaches actor kind 1 through event 3 and then event 13. */
    take=1;accepted_response=1;offered=positioned=0;s->phase=0;
    bh_pickup_tick(pickup);
    CHECK(offered==1 && positioned==1 && s->remaining==2 && s->offer==2);
    CHECK(s->phase>1.0f && room.pickups==pickup);
    /* The event 13 callback consumes the remaining amount and frees the slot. */
    s->phase=0;take=2;bh_pickup_tick(pickup);
    CHECK(offered==2 && positioned==2 && !room.pickups && !context.head);
    CHECK(pool.used==2 && bh_entities_valid(&pool));
    /* Timeout removes room/global links even when no actor accepts. */
    third=bh_pickup_spawn(&context,&point,2,0,1,1,7,0.1f);
    CHECK(third && room.pickups==third);bh_pickup_state(third)->phase=0;
    bh_pickup_tick(third);
    CHECK(!room.pickups && !context.head && pool.used==2);
    accepted_response=0;
    third=bh_pickup_spawn(&context,&point,2,0,1,1,7,0.4f);
    CHECK(third && room.pickups==third);bh_pickup_state(third)->phase=0;
    bh_pickup_tick(third);
    CHECK(bh_pickup_state(third)->fade>0.149f &&
          bh_pickup_state(third)->fade<0.151f && pool.used==3);
    bh_pickup_state(third)->phase=0;bh_pickup_tick(third);
    CHECK(!room.pickups && !context.head && pool.used==2);
    /* Outside-room actors are offered directly from stored position. */
    a->position=remote;outside[0]=a;outside[1]=NULL;
    context.outside_actors=outside;positioned=offered=0;take=1;
    accepted_response=1;
    third=bh_pickup_spawn(&context,&remote,2,0,1,1,7,0);
    CHECK(third && !bh_pickup_state(third)->room && !room.pickups);
    bh_pickup_state(third)->phase=0;bh_pickup_tick(third);
    CHECK(offered==1 && positioned==0 && !context.head && pool.used==2);
    /* Original00441730..00441772 FST/FMUL sequence gives
       35.99999983094227 < 36, whereas squaring rounded differences gives
       36.00000036876537. This is an original-derived boundary case. */
    point.x=-1.8308522701263428f;point.y=-1.3621115684509277f;point.z=1.9133549928665161f;
    a->position.x=-3.308609962463379f;a->position.y=-5.031146049499512f;a->position.z=6.4249420166015625f;
    context.outside_actors=NULL;room.actors=a;positioned=offered=0;
    third=bh_pickup_spawn(&context,&point,2,0,1,1,7,0);CHECK(third!=NULL);
    bh_pickup_state(third)->phase=0;bh_pickup_tick(third);
    CHECK(offered==1 && positioned==1 && !context.head && pool.used==2);
    if(context.head)bh_pickup_cleanup(context.head);
    bh_entity_release(&pool,a);bh_entity_release(&pool,b);
    CHECK(bh_entities_valid(&pool));bh_entities_dispose(&pool);
    if(count)*count=checks;
    printf("{\"scenario\":\"pickup\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
#ifdef BH_PICKUP_STANDALONE
int main(void){unsigned long count;return bh_pickup_checkpoint(&count);}
#endif
