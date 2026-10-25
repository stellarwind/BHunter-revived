#include "../src/bh_weapon_hud_arc.h"
#include "../src/bh_fp.h"
#include "weapon_hud_arc_cases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned long checks,failures,query_calls;
#define CHECK(x,label) do{++checks;if(!(x)){++failures;printf("FAIL weapon-hud-arc %d: %s\n",__LINE__,label);}}while(0)
static float word(unsigned long value){float f;memcpy(&f,&value,4);return f;}
static int close_to(float f,double x){return fabs((double)f-x)<.000002;}
static unsigned long random_step(unsigned long *seed){*seed=*seed*214013UL+2531011UL;return (*seed>>16)&32767UL;}
static void context(BHArcContext *c,BHDrawQueue *q){memset(c,0,sizeof(*c));c->queue=q;c->image=77;}
static void vectors(BHVec3 p[4]) {
    p[0].x=p[0].y=0;p[0].z=10;p[1]=p[0];p[1].y=10;
    p[2]=p[0];p[2].x=10;p[3]=p[2];p[3].y=10;
}
static void curve_geometry(void) {
    BHDrawQueue q;BHArcContext c;BHVec3 p[4];BHDrawCommand *last,*initial,*first;unsigned long i,j,seed=1,r[5],expected_next;
    memset(&q,0,sizeof(q));CHECK(bh_draw_queue_init(&q,40,NULL),"real queue allocation");context(&c,&q);vectors(p);
    for(i=9;i<40;++i)for(j=0;j<18;++j)q.commands[i].words[j]=0xa5a5a5a5UL;
    for(i=0;i<5;++i)r[i]=random_step(&seed);
    srand(1);CHECK(bh_arc_queue(&c,p,p+1,p+2,p+3,1) && c.path_points==17 && c.attempted==18 && c.queued==18 && c.random_calls==62,"complete initial plus16-step arc and actual RNG call count");
    CHECK(q.count==27 && q.commands[9].kind==BH_DRAW_PATH_HEADER && q.commands[9].resource==77 && q.commands[9].words[0]==17,"real bucket3 header path count/image publication");
    CHECK(q.commands[9].next==10 && q.commands[26].next==5,"actual source queue chain wraps to following bucket header");
    initial=q.commands+10;first=q.commands+11;last=q.commands+26;
    CHECK(initial->kind==BH_DRAW_PATH_POINT && word(initial->resource)==0 && word(initial->words[0])==0 && word(initial->words[1])==10 && word(initial->words[2])==.5f,"initial point exact XYZ and narrow width");
    CHECK(word(initial->words[3])==0 && word(initial->words[4])==0 && word(initial->words[5])==100 && initial->words[6]==(r[0]&3),"initial point texture parameters modulation and first CRT random tile");
    CHECK(close_to(word(first->resource),.1123046875+((double)r[1]-16384)*(15.0/2097152.0)) &&
          close_to(word(first->words[0]),.625+((double)r[2]-16384)*(15.0/2097152.0)) &&
          close_to(word(first->words[1]),10+((double)r[3]-16384)*(15.0/2097152.0)),"independent first smoothstep/interpolation and three jitter samples");
    CHECK(first->words[6]==(r[4]&3) && word(first->words[2])==1.25f && word(first->words[3])==1 && word(first->words[4])==1 && word(first->words[5])==93.75f,"generated point payload index sqrt strength and remaining modulation");
    CHECK(word(last->resource)==10 && word(last->words[0])==10 && word(last->words[1])==10 && word(last->words[5])==0 && word(last->words[4])==16,"last endpoint has no jitter, full progress reaches exact target");
    for(i=9;i<27;++i)CHECK(q.commands[i].words[17]==0xa5a5a5a5UL,"unused command payload remains untouched");
    seed=1;for(i=0;i<62;++i)random_step(&seed);expected_next=random_step(&seed);
    CHECK((unsigned long)rand()==expected_next,"global linked CRT stream advanced by62, not replacement RNG");
    bh_draw_queue_begin(&q);srand(1);CHECK(bh_arc_queue(&c,p,p+1,p+2,p+3,.5f),"half charge curve");last=q.commands+26;
    CHECK(word(last->resource)==5 && word(last->words[0])==5 && word(last->words[1])==10 && close_to(word(last->words[3]),sqrt(.5)) && word(last->words[5])==0,"partial strength stops at partial smoothstep with correct sqrt attribute");
    bh_draw_queue_begin(&q);srand(1);CHECK(bh_arc_queue(&c,p,p+1,p+2,p+3,0) && c.path_points==17 && c.random_calls==62,"zero strength retains draws and random stream calls");
    for(i=10;i<27;++i)CHECK(word(q.commands[i].resource)==0 && word(q.commands[i].words[0])==0 && word(q.commands[i].words[1])==10 && word(q.commands[i].words[5])==0,"zero strength all points collapse to original start");
    bh_draw_queue_begin(&q);srand(1);CHECK(bh_arc_queue(&c,p,p+1,p+2,p+3,2) && word(q.commands[26].resource)==10 && word(q.commands[26].words[3])==1,"upper strength clamp only");
    bh_draw_queue_dispose(&q);
}
static void exhaustion(void) {
    static const unsigned long capacities[6]={9,10,11,12,26,27},points[6]={0,0,1,2,16,17},calls[6]={0,0,1,5,61,62};
    BHDrawQueue q;BHArcContext c;BHVec3 p[4];unsigned long i;vectors(p);
    for(i=0;i<6;++i) {
        memset(&q,0,sizeof(q));CHECK(bh_draw_queue_init(&q,capacities[i],NULL),"actual bounded queue");context(&c,&q);srand(3);
        CHECK(bh_arc_queue(&c,p,p+1,p+2,p+3,1) && c.path_points==points[i] && c.random_calls==calls[i],"full queue suppresses exact random draws while preserving success count");
        CHECK(c.queued==capacities[i]-9 && c.dropped==(capacities[i]==9?1UL:27-capacities[i]) && c.attempted==(capacities[i]==9?1UL:18UL),"header failure stops, later point failures continue all16 steps");
        if(capacities[i]>9)CHECK(q.commands[9].kind==BH_DRAW_PATH_HEADER && q.commands[9].words[0]==points[i],"partial header committed with actual allocated points including zero");
        bh_draw_queue_dispose(&q);
    }
}
/* Isolated callback fixture, not a recovered game query implementation. It
   establishes event dispatch/output/mutation order without supplying a stub
   to any source/shipping target. */
static BHWeaponHudState *mutate_state;static BHWeaponHudArcPlayer *mutate_player;static BHVec3 target_position;
static void __cdecl query(BHEntity *entity,void *raw) {
    BHEntityPositionPacket *packet=(BHEntityPositionPacket *)raw;++query_calls;
    CHECK(packet->event==3,"actual synchronous position event3");packet->position=target_position;packet->event=99;
    if(mutate_state){mutate_state->target=NULL;mutate_state->charge=48;}
    if(mutate_player){memset(mutate_player,0,sizeof(*mutate_player));mutate_player->position.x=999;mutate_player->aim.z=-999;}
}
static void callback(void) {
    BHDrawQueue q,expected;BHArcContext c,d;BHWeaponHudState state;BHWeaponHudArcPlayer player;
    BHEntity entity;BHVec3 start={1,2,4},control={1,2,104},jitter;int rx,ry,rz,after;unsigned long i;
    memset(&q,0,sizeof(q));memset(&expected,0,sizeof(expected));memset(&entity,0,sizeof(entity));memset(&player,0,sizeof(player));
    CHECK(bh_draw_queue_init(&q,40,NULL) && bh_draw_queue_init(&expected,40,NULL),"callback comparison real queues");context(&c,&q);context(&d,&expected);
    entity.query=query;state.target=&entity;state.charge=1;player.position.x=1;player.position.y=2;player.position.z=3;player.forward.z=1;player.aim.z=1;
    target_position.x=10;target_position.y=20;target_position.z=30;mutate_state=&state;mutate_player=&player;query_calls=0;
    srand(17);CHECK(bh_weapon_hud_arc(&c,&state,&player) && query_calls==1 && state.charge==48 && !state.target && c.random_calls==65,"callback sees target, reads mutated charge afterward, preserves all side effects");
    after=rand();srand(17);rx=rand();ry=rand();rz=rand();jitter=target_position;
    jitter.x=(float)((double)jitter.x+((double)rx-16384.0)/2048.0);jitter.y=(float)((double)jitter.y+((double)ry-16384.0)/2048.0);jitter.z=(float)((double)jitter.z+((double)rz-16384.0)/2048.0);
    CHECK(bh_arc_queue(&d,&start,&control,&jitter,&target_position,1) && rand()==after,"callback's3 jitter draws precede real curve producer draws");
    CHECK(q.count==expected.count && !memcmp(q.commands+9,expected.commands+9,(q.count-9)*sizeof(BHDrawCommand)),"captured pre-query player pose/aim and post-query target/charge compose exact actual curve payload");
    mutate_state=NULL;mutate_player=NULL;bh_draw_queue_begin(&q);
    CHECK(bh_weapon_hud_arc(&c,&state,NULL) && !c.random_calls && q.count==9,"null target no-op does not read player or advance random stream");
    state.target=&entity;entity.query=NULL;CHECK(!bh_weapon_hud_arc(&c,&state,&player) && c.error==BH_ARC_CALLBACK && !c.random_calls,"absent real installed target query reports error");
    entity.query=query;state.charge=-1;query_calls=0;
    CHECK(!bh_weapon_hud_arc(&c,&state,&player) && c.error==BH_ARC_NUMERICAL && query_calls==1 && c.random_calls==3 && q.count==9,"negative charge diagnosed after observable query and3 random draws");
    for(i=0;i<2;++i){state.charge=48;bh_draw_queue_begin(&q);q.capacity=9;CHECK(bh_weapon_hud_arc(&c,&state,&player) && c.random_calls==3 && c.dropped==1,"full draw queue still consumes initial3 callback random values");}
    q.capacity=40;bh_draw_queue_dispose(&q);bh_draw_queue_dispose(&expected);
}
static void precision(void) {
    BHDrawQueue q;BHArcContext c;BHVec3 vectors[4];float strength;const ArcCase *t;const ArcProbe *p;BHDrawCommand *command;
    unsigned long i,j,got;memset(&q,0,sizeof(q));CHECK(bh_draw_queue_init(&q,40,NULL),"precision fixture real queue");context(&c,&q);
    for(i=0;i<sizeof(arc_cases)/sizeof(arc_cases[0]);++i) {
        t=arc_cases+i;memcpy(vectors,t->vectors,sizeof(vectors));memcpy(&strength,&t->strength,4);bh_draw_queue_begin(&q);srand(t->seed);
        CHECK(bh_arc_queue(&c,vectors,vectors+1,vectors+2,vectors+3,strength),"static curve precision draw");
        for(j=0;j<t->count;++j) {
            p=t->probes+j;command=q.commands+10+p->point;got=p->axis?command->words[p->axis-1]:command->resource;
            if(got!=p->expected)printf("arc precision %s point%lu axis%lu got%08lx expected%08lx\n",t->name,p->point,p->axis,got,p->expected);
            CHECK(got==p->expected,"exact PC53 retained X, stored YZ, retained blend complement");
        }
    }
    bh_draw_queue_dispose(&q);
}
int __cdecl bh_weapon_hud_arc_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;unsigned long seed=1,i;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    srand(1);for(i=0;i<12;++i)CHECK((unsigned long)rand()==random_step(&seed),"positive control actual linked VC5 CRT rand stream");
    curve_geometry();exhaustion();callback();precision();
    CHECK(bh_math_environment_supported(),"PC53 retained");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"weapon-hud-arc\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_WEAPON_HUD_ARC_STANDALONE
int main(void){unsigned long count;return bh_weapon_hud_arc_checkpoint(&count);}
#endif
