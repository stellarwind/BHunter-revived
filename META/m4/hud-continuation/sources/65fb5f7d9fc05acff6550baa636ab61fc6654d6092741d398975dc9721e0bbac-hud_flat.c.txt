#include "../src/bh_hud_flat.h"
#include "../src/bh_fp.h"
#include "hud_flat_cases.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static unsigned long checks,failures;
#define CHECK(x,label) do{++checks;if(!(x)){++failures;printf("FAIL hud-flat %d: %s\n",__LINE__,label);}}while(0)
typedef struct FlatFixture {
    BHDrawQueue queue;BHRadarFrame frame;BHAttachmentContext attachment;
    BHHudContext hud;BHHudFlatContext flat;BHHudStatusState state;
    BHHudWeaponAmmo weapons[10];const BHHudWeaponAmmo *slots[10];
} FlatFixture;
static float word(const BHDrawCommand *p,unsigned long i){float f;memcpy(&f,p->words+i,4);return f;}
static int close_to(float f,double x){return fabs((double)f-x)<.00005;}
static int setup(FlatFixture *f,unsigned long capacity) {
    unsigned long i;memset(f,0,sizeof(*f));
    if(!bh_draw_queue_init(&f->queue,capacity,NULL))return 0;
    f->attachment.queue=&f->queue;f->attachment.viewport_width=640;f->attachment.frame=&f->frame;
    f->hud.attachment=&f->attachment;f->hud.frame=&f->frame;f->frame.near_z=.125f;f->frame.unit=997;
    f->flat.hud=&f->hud;f->flat.viewport_height=480;f->flat.atlas_image=17;f->flat.weapon_icons_image=19;
    for(i=0;i<10;++i)f->slots[i]=f->weapons+i;
    f->state.weapons=f->slots;f->state.weapon_count=10;f->state.primary=2;f->state.secondary=7;
    f->state.gauge_a=.25f;f->state.gauge_b=.5f;return 1;
}
static unsigned long count_v(const FlatFixture *f,float v) {
    unsigned long i,n=0;for(i=9;i<f->queue.count;++i)if(word(f->queue.commands+i,8)==v)++n;return n;
}
static BHDrawCommand *find_v(FlatFixture *f,float v,unsigned long index) {
    unsigned long i;for(i=9;i<f->queue.count;++i)if(word(f->queue.commands+i,8)==v){if(!index--)return f->queue.commands+i;}return NULL;
}
static void geometry(void) {
    FlatFixture f;BHDrawCommand *p;unsigned long i;
    CHECK(setup(&f,128),"real queue and borrowed HUD bindings");f.weapons[2].amount=307;f.weapons[7].amount=42;
    f.state.counter_a=1002003;f.state.counter_b=0;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state) && f.flat.queued==21 && !f.flat.dropped,"complete status queue including variable digits");
    p=f.queue.commands+9;CHECK(p->kind==BH_DRAW_RECT && p->resource==17 && word(p,0)==626 && word(p,1)==375 && word(p,2)==20 && word(p,3)==150,"gaugeA background integer-scale placement independent radarunit");
    CHECK(word(p,6)==.62890625f && word(p,7)==.1171875f && word(p,8)==.00390625f && word(p,9)==.6953125f,"gauge background exact atlas rectangle");
    p=f.queue.commands+10;CHECK(word(p,0)==626 && word(p,1)==393.75f && word(p,3)==112.5f && word(p,6)==.87890625f && close_to(word(p,8),.177734375) && close_to(word(p,9),.521484375),"gaugeA bottom anchored fill and cropped UV");
    p=f.queue.commands+11;CHECK(word(p,1)==205 && word(p,6)==.75390625f,"gaugeB distinct top and atlas background");
    p=f.queue.commands+12;CHECK(word(p,1)==242.5f && word(p,3)==75 && close_to(word(p,8),.3515625) && close_to(word(p,9),.34765625),"gaugeB fill");
    for(i=0;i<3;++i){p=f.queue.commands+13+i;CHECK(word(p,0)==58+i*12 && word(p,1)==464 && word(p,2)==12 && word(p,3)==18,"primary three digits fixed positions including zero tens");}
    CHECK(word(f.queue.commands+13,6)==.19140625f && word(f.queue.commands+14,6)==.00390625f && word(f.queue.commands+15,6)==.44140625f,"primary decimal307 atlas columns");
    p=f.queue.commands+16;CHECK(p->resource==19 && word(p,0)==136 && word(p,1)==461 && word(p,2)==96 && word(p,3)==38 && word(p,6)==.00390625f && word(p,8)==.39453125f,"primary selected weapon icon image and row");
    CHECK(word(f.queue.commands+17,0)==570 && word(f.queue.commands+18,0)==582 && word(f.queue.commands+17,6)==.25390625f && word(f.queue.commands+18,6)==.12890625f,"secondary leading hundred suppression and digits42");
    p=f.queue.commands+19;CHECK(p->resource==19 && word(p,0)==504 && word(p,1)==461 && word(p,6)==.50390625f && word(p,8)==.39453125f,"secondary icon subtracts selectionbase5");
    p=find_v(&f,.28515625f,0);CHECK(p && word(p,0)==304 && word(p,1)==453 && word(p,2)==64 && word(p,3)==14,"counterA fixed label placement");
    CHECK(count_v(&f,.19140625f)==7 && count_v(&f,.91015625f)==1,"seven-position decimal with internal zeros and one zero digit");
    p=find_v(&f,.91015625f,0);CHECK(p && word(p,0)==388 && word(p,1)==468 && word(p,2)==8 && word(p,3)==12 && word(p,6)==.00390625f,"counterB zero units slot, distinct second row");
    for(i=9;i<f.queue.count;++i){p=f.queue.commands+i;CHECK(word(p,4)==0 && word(p,5)==1 && p->words[11]==0 && close_to(word(p,10),.135),"every rectangle angle alpha flags and live near+.01 depth");}
    bh_draw_queue_dispose(&f.queue);
}
static void gates_and_mutation(void) {
    FlatFixture f;BHDrawCommand *p;unsigned long i;const unsigned char countdowns[5]={0,1,8,16,255};
    CHECK(setup(&f,128),"gating fixture queue");
    f.state.gauge_a=.8f;f.state.gauge_b=-.5f;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state) && f.flat.queued==10,"float0.8 exceeds doublethreshold, negative gauge suppresses both fills");
    bh_draw_queue_begin(&f.queue);f.state.flags=8;f.state.gauge_a=1.5f;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state) && f.flat.queued==11,"bit8 admits overrange positive fill but negative still suppressed");
    p=f.queue.commands+10;CHECK(word(p,3)==-75 && word(p,1)==487.5f && close_to(word(p,9),-.34765625),"no gauge clamp: signed negative height and UV extent");
    for(i=0;i<5;++i) {
        bh_draw_queue_begin(&f.queue);f.state.countdown_a=f.state.countdown_b=countdowns[i];f.state.pending=0;
        CHECK(bh_hud_flat_draw(&f.flat,&f.state) && count_v(&f,.19140625f)==((countdowns[i]&8)?0UL:1UL) && count_v(&f,.91015625f)==((countdowns[i]&8)?0UL:1UL),"counter blink reads bit8 before decrement");
        CHECK(f.state.countdown_a==(countdowns[i]?countdowns[i]-1:0) && f.state.countdown_b==f.state.countdown_a,"both byte countdowns saturate only at zero, including255");
    }
    bh_draw_queue_begin(&f.queue);f.state.countdown_a=1;f.state.countdown_b=1;f.state.pending=-1;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state) && !f.state.countdown_a && f.state.countdown_b==16,"pending nonzero reloads only second after decrement-to-zero");
    bh_draw_queue_begin(&f.queue);f.state.countdown_b=2;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state) && f.state.countdown_b==1,"pending does not restart a nonzero countdown");
    bh_draw_queue_dispose(&f.queue);
}
static void signed_and_scaled(void) {
    FlatFixture f;BHDrawCommand *p;
    CHECK(setup(&f,128),"signed fixture queue");f.state.gauge_a=f.state.gauge_b=-1;
    f.weapons[2].amount=-123;f.weapons[7].amount=1000;f.state.counter_a=-2147483647L-1;f.state.counter_b=2147483647L;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state),"full signed32 counters and negative/over999ammo remain admitted");
    p=f.queue.commands+11;CHECK(word(p,6)==-.05859375f && word(p,8)==.00390625f,"negative signed ammo hundred uses original unbounded atlas coordinate");
    CHECK(count_v(&f,.19140625f)==7 && count_v(&f,.91015625f)==7,"extreme counters still seven positions without division overflow");
    bh_draw_queue_begin(&f.queue);f.attachment.viewport_width=320;f.flat.viewport_height=240;f.state.gauge_a=f.state.gauge_b=0;f.state.counter_a=f.state.counter_b=0;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state) && word(f.queue.commands+9,0)==313 && word(f.queue.commands+9,1)==187.5f && word(f.queue.commands+9,2)==10 && word(f.queue.commands+9,3)==75,"half viewport scales coordinates and sizes independently live radarunit");
    p=find_v(&f,.91015625f,0);CHECK(p && word(p,0)==194 && word(p,1)==234 && word(p,2)==4 && word(p,3)==6,"half viewport decimal placement");
    bh_draw_queue_dispose(&f.queue);
}
static void failures_and_full(void) {
    FlatFixture f;BHHudStatusState saved;unsigned long invalid=0x7fc00000UL;
    CHECK(setup(&f,9),"real exhausted queue");f.state.countdown_a=3;f.state.countdown_b=1;f.state.pending=7;
    CHECK(bh_hud_flat_draw(&f.flat,&f.state) && !f.flat.queued && f.flat.dropped==f.flat.attempted && f.state.countdown_a==2 && f.state.countdown_b==16,"full queue drops do not skip later owner state mutations");bh_draw_queue_dispose(&f.queue);
    CHECK(setup(&f,64),"malformed queue fixture");f.queue.commands[8].resource=64;f.state.countdown_a=f.state.countdown_b=3;
    CHECK(!bh_hud_flat_draw(&f.flat,&f.state) && f.flat.error==BH_HUD_FLAT_QUEUE && f.flat.attempted==12 && f.state.countdown_a==2 && f.state.countdown_b==2,"queue errors continue all publications and countdown mutations");
    bh_draw_queue_begin(&f.queue);saved=f.state;f.state.primary=10;
    CHECK(!bh_hud_flat_draw(&f.flat,&f.state) && f.flat.error==BH_HUD_FLAT_ARGUMENT && f.queue.count==9 && f.state.countdown_a==saved.countdown_a,"missing weapon binding rejects before queue/state mutation");
    f.state=saved;memcpy(&f.state.gauge_a,&invalid,4);
    CHECK(!bh_hud_flat_draw(&f.flat,&f.state) && f.flat.error==BH_HUD_FLAT_NUMERICAL && f.queue.count==9 && f.state.countdown_a==saved.countdown_a,"nonfinite gauge source guard");
    bh_draw_queue_dispose(&f.queue);
}
static void precision_probes(void) {
    FlatFixture f;const HudFlatProbe *p;unsigned long i,got;
    CHECK(setup(&f,128),"odd viewport precision queue");
    f.state.gauge_a=f.state.gauge_b=-1;f.weapons[2].amount=303;f.weapons[7].amount=404;
    f.state.counter_a=1000000;f.state.counter_b=0;
    for(i=0;i<sizeof(hud_flat_probes)/sizeof(hud_flat_probes[0]);++i) {
        p=hud_flat_probes+i;bh_draw_queue_begin(&f.queue);
        f.attachment.viewport_width=p->width;f.flat.viewport_height=p->height;
        memcpy(&f.state.gauge_a,&p->gauge_a,4);
        CHECK(bh_hud_flat_draw(&f.flat,&f.state),"static odd viewport fixture draws");
        got=f.queue.commands[p->index].words[p->word];
        if(got!=p->expected)printf("hud-flat probe %s got%08lx expected%08lx\n",p->name,got,p->expected);
        CHECK(got==p->expected,"exact retained raw half dimension/static PC53 output");
    }
    bh_draw_queue_dispose(&f.queue);
}
int __cdecl bh_hud_flat_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    geometry();gates_and_mutation();signed_and_scaled();failures_and_full();precision_probes();
    CHECK(bh_math_environment_supported(),"PC53 unchanged");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"hud-flat\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_HUD_FLAT_STANDALONE
int main(void){unsigned long n;return bh_hud_flat_checkpoint(&n);}
#endif
