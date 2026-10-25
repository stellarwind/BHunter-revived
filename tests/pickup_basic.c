#include "../src/bh_pickup_basic.h"
#include "../src/bh_pickup_draw.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,name) do{++checks;if(!(x)){++failures;printf("FAIL pickup-basic %d: %s\n",__LINE__,name);}}while(0)
typedef struct Memory {int fail;unsigned long calls,live;} Memory;
static void *__cdecl allocate(unsigned long bytes,void *user){Memory *m=(Memory*)user;void *p;++m->calls;if(m->fail)return NULL;p=malloc(bytes);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory*)user;if(p){--m->live;free(p);}}
static void setup(BHPickupContext *owner,BHEntityPool *pool,BHWorldGeometry *world,
    BHPickupRoom *room,unsigned long *resources,BHPickupRenderBinding *binding) {
    memset(owner,0,sizeof(*owner));memset(world,0,sizeof(*world));memset(room,0,sizeof(*room));
    world->navigation.next=&world->navigation;
    world->minimum.x=world->minimum.y=world->minimum.z=-10;
    world->maximum.x=world->maximum.y=world->maximum.z=10;
    room->world=world;owner->pool=pool;owner->first_world=world;owner->world_count=1;
    owner->rooms=room;owner->room_count=1;owner->resources=resources;owner->resource_count=10;
    CHECK(bh_pickup_render_bind(owner,binding),"actual admitted renderer bridge binding");
}
static void cleanup_all(BHPickupContext *owner){unsigned long n=0;
    while(owner->head&&n++<100)bh_pickup_cleanup(owner->head);
    CHECK(!owner->head&&!owner->pool->used&&bh_entities_valid(owner->pool),"actual cleanup releases every list/pool owner");
}
static void constructor_cases(BHPickupRenderBinding *binding){
    BHEntityPool pool;BHWorldGeometry world;BHPickupRoom room;BHPickupContext owner;
    unsigned long table[10]={73,0,0,0,0,0,0,0,0,0},bits;BHEntity *a,*b,*c,*e;
    BHPickupState *s;BHEntityStep draw;BHVec3 point={1,2,3},outside={30,2,3};
    int first,second,third,next;long event=20;float expected_phase,expected_rotation,fade;
    long cases[5][3]={{3,5,3},{0,5,5},{-3,5,-3},{3,-5,-5},{3,2,2}};unsigned long i;
    memset(&pool,0,sizeof(pool));CHECK(bh_entities_init(&pool,8,NULL),"actual entity pool allocation");
    setup(&owner,&pool,&world,&room,table,binding);pool.origin.x=4;pool.origin.y=5;pool.origin.z=6;
    e=bh_entity_acquire(&pool);CHECK(e!=NULL,"actual slot prepared for recycled payload discriminator");
    if(e){memset(e->payload,0xa5,sizeof(e->payload));e->flags=0x12345000UL;bh_entity_release(&pool,e);}
    srand(123);first=rand();second=rand();third=rand();
    expected_phase=(float)((double)first/131072.0);expected_rotation=(float)((double)second*0.0001953125);
    srand(123);a=bh_pickup_basic_spawn(&owner,&point,9,3,5,17,0.75f);
    CHECK(a&&a==e&&rand()==third,"two actual CRT draws in order after successful recycled allocation");
    if(!a)goto done;s=bh_pickup_state(a);memcpy(&draw,a->payload+4,4);
    CHECK(bh_pickup_context(a)==&owner&&owner.draw_user==binding,"actual admitted getter reads persistent context at payload+64");
    CHECK(draw==bh_pickup_default_step&&draw!=owner.special_draw,"type9 installs actual default draw rather than special bridge");
    CHECK(s->resource==0&&s->type==9&&s->variant==0,"zero selected resource retained despite nonzero table0; no fallback");
    CHECK(s->phase==expected_phase&&s->rotation==expected_rotation&&s->fade==0.75f,"exact binary32 random phase/rotation and supplied fade");
    CHECK(s->remaining==3&&s->offer==3&&s->token==17,"signed clamp and remaining/token transport");
    CHECK(a->flags==0x12345000UL&&a->kind==0&&a->lifetime==-1&&a->scale==1&&!memcmp(&a->origin,&pool.origin,12),"constructor preserves actual pool-owned fields without extra kind/flags writes");
    CHECK(!memcmp(&a->position,&point,12)&&s->room==&world&&room.pickups==a&&owner.head==a,"actual world location and both initial list heads");
    CHECK(a->step==bh_pickup_tick&&a->query==bh_pickup_query&&!a->refresh,"actual fixed step/query and shared return-sentinel binding");
    b=bh_pickup_basic_spawn(&owner,&point,2,0,5,0,0);c=bh_pickup_basic_spawn(&owner,&point,2,0,5,0,0);
    CHECK(b&&c&&owner.head==c&&room.pickups==c,"two nonempty insertions publish room and global heads");
    if(b&&c){CHECK(bh_pickup_state(c)->room_next==b&&bh_pickup_state(b)->room_next==a&&s->room_previous==b&&
        bh_pickup_state(c)->global_next==b&&bh_pickup_state(b)->global_next==a&&s->global_previous==b,"both intrusive lists repair previous and next links");
        bh_pickup_cleanup(b);CHECK(bh_pickup_state(c)->room_next==a&&s->room_previous==c&&bh_pickup_state(c)->global_next==a&&s->global_previous==c,"actual cleanup unlinks middle entity from both lists");
        bh_pickup_cleanup(c);CHECK(owner.head==a&&room.pickups==a&&!s->room_previous&&!s->global_previous,"actual cleanup unlinks both list heads");}
    a->query(a,&event);CHECK(a->step==bh_pickup_cleanup&&!a->query&&!pool.query_head,"actual event20 schedules cleanup and unregisters query");
    CHECK(bh_entities_step(&pool)&&!pool.used&&!owner.head&&!room.pickups,"actual pool step runs scheduled cleanup");
    for(i=0;i<5;++i){e=bh_pickup_basic_spawn(&owner,&point,2,cases[i][0],cases[i][1],-7,-2);
        CHECK(e&&bh_pickup_state(e)->offer==cases[i][2]&&bh_pickup_state(e)->remaining==cases[i][0],"signed clamp covers zero, negative and ordered positive cases");if(e)bh_pickup_cleanup(e);}
    e=bh_entity_acquire(&pool);if(e){memset(e->payload,0xa5,sizeof(e->payload));bh_entity_release(&pool,e);}
    e=bh_pickup_basic_spawn(&owner,&outside,2,0,1,0,0);CHECK(e&&!bh_pickup_state(e)->room&&!room.pickups&&owner.head==e,"outside-world pickup participates only in actual global list");
    if(e){s=bh_pickup_state(e);CHECK(!s->room_next&&!s->room_previous,"source guard clears original unwritten recycled outside-room links");bh_pickup_cleanup(e);}
    bits=0x80000000UL;memcpy(&fade,&bits,4);e=bh_pickup_basic_spawn(&owner,&point,2,0,1,0,fade);
    if(e){unsigned long got;memcpy(&got,&bh_pickup_state(e)->fade,4);CHECK(got==bits,"fade negative zero preserved by raw bit copy");bh_pickup_cleanup(e);}else CHECK(0,"negative zero constructor");
    bits=0x7fc12345UL;memcpy(&fade,&bits,4);e=bh_pickup_basic_spawn(&owner,&point,2,0,1,0,fade);
    if(e){unsigned long got;memcpy(&got,&bh_pickup_state(e)->fade,4);CHECK(got==bits,"fade NaN payload preserved without floating point normalization");bh_pickup_cleanup(e);}else CHECK(0,"raw NaN constructor");
    e=bh_pickup_basic_spawn(&owner,&point,2,0,1,0,0.1f);CHECK(e!=NULL,"actual tick timeout owner");
    if(e){bh_pickup_state(e)->phase=0;CHECK(bh_entities_step(&pool)&&!owner.head&&!room.pickups&&!pool.used,"actual tick expires fade and cleans both lists without actors or audio");}
    srand(45);next=rand();srand(45);owner.default_draw=NULL;
    CHECK(!bh_pickup_basic_spawn(&owner,&point,2,0,1,0,0)&&rand()==next&&!pool.used,"missing real renderer fails before allocation/random side effects");
    CHECK(bh_pickup_render_bind(&owner,binding),"real renderer rebound after negative control");
    srand(45);CHECK(!bh_pickup_basic_spawn(&owner,&point,10,0,1,0,0)&&rand()==next&&!pool.used,"out-of-table source guard leaves RNG and pool untouched");
    srand(123);owner.room_count=0;e=bh_pickup_basic_spawn(&owner,&point,2,0,1,0,0);
    CHECK(!e&&!pool.used&&!owner.head&&!room.pickups&&rand()==third,"missing actual room binding rolls back acquired owner after exactly two RNG calls");owner.room_count=1;
done:cleanup_all(&owner);CHECK(bh_entities_dispose(&pool),"actual constructor pool disposal");
}
static void allocation_failure(BHPickupRenderBinding *binding){Memory memory;BHSceneAllocator allocator;BHEntityPool pool;BHPickupContext owner;
    BHWorldGeometry world;BHPickupRoom room;unsigned long table[10]={0};BHEntity *filler;BHVec3 point={0,0,0};int next;unsigned long calls;
    memset(&memory,0,sizeof(memory));allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
    CHECK(bh_entities_init(&pool,1,&allocator),"real owned allocation for failure injection");setup(&owner,&pool,&world,&room,table,binding);
    filler=bh_entity_acquire(&pool);CHECK(filler&&pool.used==1,"actual initial pool exhausted");memory.fail=1;calls=memory.calls;
    srand(987);next=rand();srand(987);
    CHECK(!bh_pickup_basic_spawn(&owner,&point,2,1,1,0,0)&&rand()==next,"failed actual growth allocation consumes no CRT random values");
    CHECK(memory.calls==calls+1&&pool.used==1&&pool.capacity==1&&pool.active==filler&&!pool.query_head&&!owner.head&&!room.pickups,"actual failed allocator leaves live pool and pickup lists unchanged");
    memory.fail=0;if(filler)bh_entity_release(&pool,filler);CHECK(bh_entities_dispose(&pool)&&!memory.live,"actual failed-growth fixture releases all owned allocation");
}
int bh_pickup_basic_checkpoint(unsigned long *count){BHMathEnvironment fp;BHDrawQueue queue;BHPickupDrawContext draw;
    BHEntityResourceDrawContext special;BHPickupRenderBinding binding;int entered;
    checks=failures=0;memset(&queue,0,sizeof(queue));memset(&draw,0,sizeof(draw));memset(&special,0,sizeof(special));
    entered=bh_math_environment_enter(&fp);CHECK(entered,"admitted PC53 numeric fixture profile");if(!entered)goto done;
    CHECK(bh_draw_queue_init(&queue,64,NULL)&&bh_draw_queue_begin(&queue),"actual canonical queue owner for real draw bridges");
    draw.queue=&queue;special.queue=&queue;binding.default_draw=&draw;binding.special_draw=&special;
    constructor_cases(&binding);allocation_failure(&binding);
done:if(queue.commands)CHECK(bh_draw_queue_dispose(&queue),"actual renderer queue owner disposal");if(entered)bh_math_environment_leave(&fp);
    if(count)*count=checks;printf("{\"scenario\":\"pickup-basic\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_PICKUP_BASIC_STANDALONE
int main(void){return bh_pickup_basic_checkpoint(NULL);}
#endif
