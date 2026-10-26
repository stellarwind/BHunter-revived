#include "../src/bh_actor_route.h"
#include "../src/bh_actor_pursuit_route.h"
#include "../src/bh_actor_damage.h"
#include <stdlib.h>
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL actor-route: %s\n",s);}} while(0)
static void link(BHEntity *a,unsigned long offset,BHEntity *p){memcpy((unsigned char*)a+offset,&p,4);}
static BHEntity *linked(BHEntity *a,unsigned long offset){BHEntity *p;memcpy(&p,(unsigned char*)a+offset,4);return p;}
static unsigned long next_state(unsigned long state){unsigned long p=state*58199UL;return ((p<<16)|(p>>16))^state;}
static float stored(double x){float f;bh_round_float(&f,x);return f;}
static unsigned long bits(float f){unsigned long b;memcpy(&b,&f,4);return b;}
typedef struct Fixture {
    BHWorldGeometry worlds[3];BHGeometryModel model;
    BHPickupRoom rooms[3];BHActorRouteContext context;BHActorRouteState state;
} Fixture;
static void fixture(Fixture *f,BHEntity *actor,unsigned long flags,unsigned long axis) {
    unsigned long i;BHPortal *p;long kind=2;
    memset(f,0,sizeof(*f));f->model.portal_count=2;
    for(i=0;i<3;++i){f->rooms[i].world=f->worlds+i;f->worlds[i].model=&f->model;f->worlds[i].navigation.count=2;}
    f->worlds[1].query_flags=flags;
    f->worlds[1].minimum.x=f->worlds[1].minimum.y=f->worlds[1].minimum.z=-20;
    f->worlds[1].maximum.x=f->worlds[1].maximum.y=f->worlds[1].maximum.z=20;
    ((float*)&f->worlds[1].minimum)[axis]=0;((float*)&f->worlds[1].maximum)[axis]=100;
    for(i=0;i<2;++i){p=f->worlds[1].navigation.portals+i;p->direction=axis+(i?0:4);p->plane=i?100:0;
        p->scratch[0]=p->scratch[2]=-20;p->scratch[1]=p->scratch[3]=20;
        p->neighbor=&f->worlds[i?2:0].navigation;}
    f->context.rooms=f->rooms;f->context.room_count=3;f->context.actor_limit=16;f->context.tick=0xbeef;
    f->state.current=f->worlds;f->state.next=f->worlds+1;f->state.speed=10;f->state.speed_scale=1;
    f->state.u=f->state.v=.5;f->state.random=0x12345678;f->state.sound=BH_SOUND_INVALID;f->state.phase=7;
    f->rooms[0].actors=actor;link(actor,0xf4,NULL);link(actor,0xf8,NULL);memcpy(actor->payload+24,&kind,4);
    CHECK(bh_actor_route_write(actor,&f->state),"real actor field-view binding");
}
static void navigation(BHNavigation *n,BHNavigationNode *nodes,BHSceneRoute *cells,Fixture *f){unsigned long i;memset(n,0,sizeof(*n));memset(nodes,0,sizeof(*nodes)*3);memset(cells,0,sizeof(*cells)*9);for(i=0;i<3;++i)nodes[i].placement=f->worlds+i;for(i=0;i<9;++i)cells[i].exit_slot=1;n->nodes=nodes;n->node_count=n->junction_count=3;n->routes.cells=cells;n->routes.node_count=3;}
int bh_actor_pursuit_route_checkpoint(unsigned long *count){
 Fixture f;BHEntity e,*target;BHEntityPool pool;BHActorRouteState s;BHActorPursuitRouteContext c;BHNavigation nav;BHNavigationNode nodes[3];BHSceneRoute cells[9];BHMathEnvironment env;BHActorDamageContext damage;BHSoundQueue sound;BHMessageState messages;long cities[3]={0,1,2};const char *names[3]={"Old Town","Harbor","City"};unsigned long expected,i,state;long kind,city,forced;BHVec3 saved,oldend;
 checks=failures=0;CHECK(bh_math_environment_enter(&env),"PC53");CHECK(bh_entities_init(&pool,8,0),"pool");CHECK(bh_sound_queue_init(&sound,0,0,0),"sound");
 memset(&e,0,sizeof(e));fixture(&f,&e,0,0);navigation(&nav,nodes,cells,&f);memset(&c,0,sizeof(c));c.route=&f.context;c.navigation=&nav;c.actor_limit=16;f.context.sound=&sound;
 srand(23);rand();rand();expected=rand();srand(23);CHECK(bh_actor_pursuit_route(&c,&e),"whole ordinary route");CHECK(rand()==expected,"exact two CRT bias draws");bh_actor_route_read(&e,&s);CHECK(s.current==f.worlds+1&&s.next==f.worlds+2&&s.phase==0&&s.duration==10,"real route promotion owners");CHECK(f.rooms[0].actors==0&&f.rooms[1].actors==&e,"real room unlink/attach");
 memset(&e,0,sizeof(e));fixture(&f,&e,8,0);navigation(&nav,nodes,cells,&f);c.route=&f.context;f.context.sound=&sound;CHECK(bh_actor_pursuit_route(&c,&e),"guided route owner");bh_actor_route_read(&e,&s);CHECK(s.duration==20&&s.next==f.worlds+2,"guided straight route duration");
 /* Real damage query supplies event16 target motion for a pursuit interception. */
 target=bh_entity_acquire(&pool);memset(&damage,0,sizeof(damage));damage.pool=&pool;damage.route=&f.context;CHECK(bh_actor_damage_bind(&damage,target),"actual target motion query");memset(&s,0,sizeof(s));s.current=s.next=f.worlds+1;s.duration_ticks=30;s.controls[0].x=s.controls[1].x=s.controls[2].x=s.controls[3].x=50;s.controls[0].y=s.controls[1].y=s.controls[2].y=s.controls[3].y=5;s.sound=BH_SOUND_INVALID;bh_actor_route_write(target,&s);memcpy(target->opaque2b,&s.current,4);
 memset(&e,0,sizeof(e));fixture(&f,&e,0,0);navigation(&nav,nodes,cells,&f);c.route=&f.context;f.context.sound=&sound;memcpy((char*)&e+0x1b8,&target,4);{float p=1;memcpy((char*)&e+0x1b4,&p,4);}CHECK(bh_actor_pursuit_route(&c,&e),"pursuit target path/query");bh_actor_route_read(&e,&s);memcpy(&forced,(char*)&e+0x1d4,4);CHECK(forced==1&&s.controls[3].x==50&&s.controls[3].y==5,"actual target query intercept endpoint");memcpy(&saved,(char*)&e+0x1bc,12);CHECK(saved.x==100,"old portal endpoint retained");oldend=s.controls[3];CHECK(bh_actor_pursuit_route(&c,&e),"forced return half route");bh_actor_route_read(&e,&s);memcpy(&forced,(char*)&e+0x1d4,4);CHECK(!forced&&s.controls[0].x==oldend.x&&s.controls[3].x==saved.x&&s.phase==0,"forced half restores saved endpoint");
 /* Exact static city strings, shared throttle and actual source message arena. */
 memset(&e,0,sizeof(e));fixture(&f,&e,0,0);navigation(&nav,nodes,cells,&f);c.route=&f.context;f.context.sound=&sound;kind=6;city=0;memcpy((char*)&e+0xfc,&kind,4);memcpy((char*)&e+0x110,&city,4);bh_message_state_init(&messages);c.city_ids=cities;c.city_names=names;c.city_count=3;c.messages=&messages;c.clock=1801;c.last_report=0;srand(14);state=rand()&3;CHECK(bh_actor_pursuit_route(&c,&e),"city transition report");CHECK(c.last_report==1801&&messages.ring_count==1&&strstr(messages.ring[0],"Harbor")&&strchr(messages.ring[0],'\n'),"actual complete report text and city binding");
 /* Class1 reacquires a real eligible actor through the restored target selector. */
 memset(&e,0,sizeof(e));fixture(&f,&e,0,0);navigation(&nav,nodes,cells,&f);c.route=&f.context;f.context.sound=&sound;c.city_ids=0;c.actor_head=target;kind=1;memcpy((char*)&e+0xfc,&kind,4);kind=2;memcpy((char*)target+0xfc,&kind,4);kind=1;memcpy((char*)target+0x100,&kind,4);CHECK(bh_actor_pursuit_route(&c,&e),"class1 reacquires actual target");CHECK(linked(&e,0x1b8)==target,"target selector installed real actor reference");
 /* Sound expiry stops the actual source voice before routing. */
 {BHSoundResource *resource=bh_sound_resource_allocate(&sound);unsigned long handle;CHECK(resource!=0,"source streaming resource");resource->streaming=1;resource->base_frequency=100;handle=bh_sound_request(&sound,resource->index,1,1,0,0);CHECK(handle!=BH_SOUND_INVALID,"source voice queued");memset(&e,0,sizeof(e));fixture(&f,&e,0,0);navigation(&nav,nodes,cells,&f);f.context.sound=&sound;c.route=&f.context;f.state.sound=handle;f.state.sound_frames=1;bh_actor_route_write(&e,&f.state);CHECK(bh_actor_pursuit_route(&c,&e),"sound countdown plus route");bh_actor_route_read(&e,&s);CHECK(s.sound==BH_SOUND_INVALID&&!s.sound_frames&&!bh_sound_voice(&sound,handle),"sound stopped/generation invalidated");}
 CHECK(bh_entity_release(&pool,target),"target cleanup");CHECK(bh_entities_dispose(&pool),"pool dispose");CHECK(bh_sound_queue_dispose(&sound),"sound dispose");bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"actor-pursuit-route\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ACTOR_PURSUIT_ROUTE_STANDALONE
int main(void){return bh_actor_pursuit_route_checkpoint(0);}
#endif
