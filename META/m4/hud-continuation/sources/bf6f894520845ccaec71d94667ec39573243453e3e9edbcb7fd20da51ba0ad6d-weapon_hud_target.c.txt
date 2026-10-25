#include "../src/bh_weapon_hud_target.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures,query_calls,query_event;
static BHWeaponHudState *mutated_state;static BHWeaponHudTargetContext *mutated_context;
static BHViewConfig *mutated_view;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-hud-target: %s\n",s);}}while(0)
/* Isolated observable query fixture, never installed by production source and
   never counted as recovered target-query behavior. The entire original
   40-byte packet extent is available to an actual installed query method. */
static void __cdecl query(BHEntity *target,void *raw) {
    BHEntityPositionPacket *packet=(BHEntityPositionPacket *)raw;
    ++query_calls;query_event=(unsigned long)packet->event;packet->position=target->position;
    ((unsigned long *)raw)[9]=0x87654321UL;packet->event=99;
    if(mutated_state){mutated_state->charge=52;mutated_state->target=NULL;}
    if(mutated_context)mutated_context->viewport_width=320;
    if(mutated_view)mutated_view->near_z=.25f;
}
typedef struct Fixture {
    BHWeaponHudTargetContext context;BHWeaponHudState state;BHEntity target;
    BHPickupDrawContext projection;BHMainCamera camera;BHViewConfig view;BHDrawQueue queue;
} Fixture;
static int fixture(Fixture *f,unsigned long capacity) {
    memset(f,0,sizeof(*f));if(!bh_draw_queue_init(&f->queue,capacity,NULL))return 0;
    f->camera.projected.m[0][0]=f->camera.projected.m[1][1]=f->camera.projected.m[2][2]=1;f->camera.focal=8;
    f->projection.camera=&f->camera;f->projection.view=&f->view;f->projection.queue=&f->queue;f->view.near_z=.125f;
    f->context.projection=&f->projection;f->context.viewport_width=640;f->context.atlas_image=17;
    f->state.target=&f->target;f->target.query=query;f->target.position.x=40;f->target.position.y=20;f->target.position.z=2;
    query_calls=query_event=0;mutated_state=NULL;mutated_context=NULL;mutated_view=NULL;return 1;
}
static float value(const unsigned long *p){float x;memcpy(&x,p,4);return x;}
static int close_to(float a,double b){return fabs((double)a-b)<.000002;}
static void branches(void) {
    Fixture f;unsigned long i,j,first;BHDrawCommand *p;
    static const struct {long charge,distance,size;unsigned long n;int ready;} cases[]={
        {-2147483647L-1,80,16,4,0},{-52,132,16,5,0},{-1,81,16,5,0},{0,80,16,4,0},
        {3,77,16,4,0},{4,76,16,5,0},{7,73,16,5,0},{8,72,16,4,0},{44,36,16,5,0},{47,33,16,5,0},
        {48,48,24,5,1},{49,48,24,5,1},{52,32,16,5,1},{2147483647L,32,16,5,1}};
    static const unsigned long angles[]={0x3f490fdbUL,0x407b53d1UL,0x40afeddfUL,0x4016cbe4UL};
    CHECK(fixture(&f,32),"real point-projection and rectangle queue definitions");
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        bh_draw_queue_begin(&f.queue);f.state.charge=cases[i].charge;query_calls=0;
        CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.queries==1 && f.context.projected==1 &&
            f.context.attempted==cases[i].n && f.context.queued==cases[i].n && !f.context.dropped && query_calls==1 && query_event==3,
            "complete target query, projection and all signed charge/bit4 branches");
        first=cases[i].n==5?1:0;
        if(first){p=f.queue.commands+9;CHECK(value(p->words)==20 && value(p->words+1)==10 && value(p->words+2)==(cases[i].ready?64:52) &&
            value(p->words+3)==(cases[i].ready?22:12) && value(p->words+4)==0 && value(p->words+6)==0 && value(p->words+7)==.5f &&
            p->words[8]==(cases[i].ready?0x3f53f7cfUL:0x3f381062UL) && p->words[9]==(cases[i].ready?0x3e3020c5UL:0x3ddf3b64UL),
            "blinking/ready central bar has exact dimensions and raw atlas rectangles");}
        for(j=0;j<4;++j){p=f.queue.commands+9+first+j;
            CHECK(p->kind==BH_DRAW_RECT && p->resource==17 && value(p->words)==20+(j==2?cases[i].distance:(j==3?-cases[i].distance:0)) &&
                value(p->words+1)==10+(j==0?cases[i].distance:(j==1?-cases[i].distance:0)) && value(p->words+2)==cases[i].size &&
                value(p->words+3)==cases[i].size && p->words[4]==angles[j] && value(p->words+5)==1 && value(p->words+6)==.25f &&
                value(p->words+7)==.25f && value(p->words+8)==0 && value(p->words+9)==.25f && close_to(value(p->words+10),.135) &&
                !p->words[11] && p->next==(j==3?5:10+first+j),"four exact diamond-corner payloads, raw rotations, live depth and real bucket continuation");}
        CHECK(f.state.target==&f.target && f.state.charge==cases[i].charge,"HUD does not own or modify selected weapon target/charge");
    }
    bh_draw_queue_dispose(&f.queue);
}
static void metrics(void) {
    Fixture f;unsigned long i;float width,height,distance,x,y;
    static const long widths[]={-2147483647L-1,-640,-159,-1,0,159,160,319,320,639,640,641,2147483647L};
    static const long units[]={-13421772,-4,0,0,0,0,1,1,2,3,4,4,13421772};
    CHECK(fixture(&f,32),"signed viewport fixtures");f.state.charge=48;
    for(i=0;i<sizeof(widths)/sizeof(widths[0]);++i){
        bh_draw_queue_begin(&f.queue);f.context.viewport_width=widths[i];
        bh_round_float(&width,(double)(units[i]*16));bh_round_float(&height,(double)units[i]*5.5);
        bh_round_float(&distance,(double)(units[i]*12));bh_round_float(&x,20);bh_round_float(&y,10+(double)distance);
        CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.queued==5 &&
            value(f.queue.commands[9].words+2)==width && value(f.queue.commands[9].words+3)==height &&
            value(f.queue.commands[10].words)==x && value(f.queue.commands[10].words+1)==y,
            "signed width/160 truncation and stored binary32 geometry through int32 extremes");
    }
    bh_draw_queue_dispose(&f.queue);
}
static void mutation_and_culls(void) {
    Fixture f;CHECK(fixture(&f,32),"synchronous query mutation fixture");
    mutated_state=&f.state;mutated_context=&f.context;mutated_view=&f.view;
    CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && query_calls==1 && query_event==3 && f.state.charge==52 && !f.state.target &&
        f.context.viewport_width==320 && f.context.queued==5 && value(f.queue.commands[9].words+2)==64 &&
        value(f.queue.commands[10].words+2)==16 && close_to(value(f.queue.commands[9].words+10),.26),
        "unit is captured before query, charge and live near read afterward, target replacement retains returned packet point");
    mutated_state=NULL;mutated_context=NULL;mutated_view=NULL;bh_draw_queue_begin(&f.queue);
    CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && !f.context.queries && !f.context.projected && !f.context.attempted && f.queue.count==9,
        "null target is an original complete early return");
    f.state.target=&f.target;f.target.position.z=.125f;
    CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.queries==1 && !f.context.projected && f.queue.count==9,
        "real projection near cull after observable target query");
    f.target.position.z=.25f;f.state.charge=48;
    CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.projected==1 && f.context.queued==5,
        "projection includes exact live near-plane equality");
    bh_draw_queue_dispose(&f.queue);
}
static void queues_and_guards(void) {
    Fixture f;unsigned long nan=0x7fc00000UL;
    CHECK(fixture(&f,9),"full queue fixture");f.state.charge=52;
    CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.attempted==5 && f.context.dropped==5 && !f.context.queued &&
        query_calls==1 && f.state.charge==52,"full queue drops every draw without suppressing projection or later publications");
    bh_draw_queue_dispose(&f.queue);CHECK(bh_draw_queue_init(&f.queue,11,NULL),"partial queue fixture");
    CHECK(bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.queued==2 && f.context.dropped==3 && f.context.attempted==5,
        "partial queue retains bar/first corner and attempts all others");
    bh_draw_queue_begin(&f.queue);f.queue.commands[4].resource=100;
    CHECK(!bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.error==BH_WEAPON_HUD_TARGET_QUEUE && f.context.attempted==5 &&
        f.queue.count==9,"source queue topology failure retains all attempted owner work");
    bh_draw_queue_begin(&f.queue);f.target.query=NULL;
    CHECK(!bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.error==BH_WEAPON_HUD_TARGET_CALLBACK && !f.context.queries && f.queue.count==9,
        "missing real target query is diagnosed without a stub");
    f.target.query=query;memcpy(&f.target.position.x,&nan,4);
    CHECK(!bh_weapon_hud_target_draw(&f.context,&f.state) && f.context.error==BH_WEAPON_HUD_TARGET_NUMERICAL && f.context.queries==1 && f.queue.count==9,
        "nonfinite query output guard preserves query side effect and publishes no malformed rectangle");
    CHECK(!bh_weapon_hud_target_draw(NULL,&f.state) && !bh_weapon_hud_target_draw(&f.context,NULL),"absent public source arguments");
    bh_draw_queue_dispose(&f.queue);
}
int bh_weapon_hud_target_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    branches();metrics();mutation_and_culls();queues_and_guards();CHECK(bh_math_environment_supported(),"PC53 retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"weapon-hud-target\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_WEAPON_HUD_TARGET_STANDALONE
int main(void){unsigned long count;return bh_weapon_hud_target_checkpoint(&count);}
#endif
