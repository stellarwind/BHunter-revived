#include "../src/bh_radar.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL radar: %s\n",s);}}while(0)
static float value(const unsigned long *p){float x;memcpy(&x,p,4);return x;}
static int close_to(float a,double b){return fabs((double)a-b)<.000002;}
static void frame(BHRadarFrame *f){memset(f,0,sizeof(*f));f->origin.x=1;f->origin.y=2;f->origin.z=3;f->axis_x.x=1;f->axis_y.y=1;f->normal.z=1;f->yaw_x.x=1;f->yaw_y.z=1;f->near_z=1;f->unit=8;f->screen_radius=64;f->screen_x=100;f->screen_y=80;}
static void modes(void) {
    BHDrawQueue q;BHRadarContext c;BHRadarFrame f;BHVec3 player={0,0,0},target={100,0,200};BHDrawCommand *a,*b;unsigned long flat;
    CHECK(bh_draw_queue_init(&q,20,NULL),"real shared queue owner");frame(&f);CHECK(bh_radar_bind(&c,&q,&f,17,18),"explicit live radar bindings");
    for(flat=0;flat<2;++flat){f.flat=flat;bh_draw_queue_begin(&q);
        CHECK(bh_radar_marker(&c,&player,&target,0,0,0) && c.attempted==1 && c.queued==1,"complete ordinary marker in each rendering mode");a=q.commands+9;
        CHECK(a->resource==17 && a->kind==(flat?BH_DRAW_RECT:BH_DRAW_QUAD),"real screen/world producer selection");
        if(flat){CHECK(close_to(value(a->words),116) && close_to(value(a->words+1),48) && close_to(value(a->words+2),11.2),"independent flat placement and signed integer scale");
            CHECK(value(a->words+6)==0 && value(a->words+7)==.5f && value(a->words+8)==0 && value(a->words+9)==.5f && close_to(value(a->words+10),1.001),"original UV pair order and depth1-minus-negative.001");}
        else{CHECK(close_to(value(a->words),1.025) && close_to(value(a->words+1),2.05) && value(a->words+2)==3 && close_to(value(a->words+9),.02),"independent world placement and width");
            CHECK(value(a->words+4)==1 && value(a->words+8)==1 && value(a->words+14)==0 && value(a->words+15)==.5f && value(a->words+16)==0 && value(a->words+17)==.5f,"world up/normal and original UV quadrant pair order");}
        CHECK(close_to(value(a->words+(flat?5:12)),1-sqrt(.3125)),"independent radius-fade result");
        bh_draw_queue_begin(&q);CHECK(bh_radar_marker(&c,&player,&target,1,0,0) && q.count==11 && c.attempted==2 && c.queued==2,"exact kind1 adds secondary range bar");a=q.commands+9;b=q.commands+10;
        CHECK(a->resource==17 && b->resource==18 && value(a->words+(flat?5:12))==1 && close_to(value(b->words+(flat?5:12)),sqrt(.3125)),"kind1 marker opacity versus range-bar radius");
        if(flat){CHECK(close_to(value(a->words+2),5.6) && close_to(value(b->words),110.4) && close_to(value(b->words+1),52.8) && value(b->words+2)==40 && value(b->words+3)==8,"flat half-size and asymmetric signed bar offsets");
            CHECK(value(a->words+6)==.5f && value(a->words+7)==.5f && value(a->words+8)==0 && value(b->words+6)==0 && value(b->words+7)==.5f && value(b->words+8)==.5f && value(b->words+9)==.125f,"flat marker and bar preserve source atlas word order");}
        else{CHECK(close_to(value(a->words+9),.01) && close_to(value(b->words),1.01625) && close_to(value(b->words+1),2.0425) && close_to(value(b->words+9),.075) && close_to(value(b->words+10),.015),"world half-size and independent bar position/dimensions");}
        bh_draw_queue_begin(&q);CHECK(bh_radar_marker(&c,&player,&target,1,0,1) && q.count==10 && q.commands[9].resource==18,"skip-marker retains exact-kind1 bar");
        bh_draw_queue_begin(&q);CHECK(bh_radar_marker(&c,&player,&target,5,0,0) && q.count==10 && close_to(value(q.commands[9].words+(flat?2:9)),flat?11.2:.02),"mask bits do not make kind5 inherit exact-kind1 half-size/bar");
    }
    bh_draw_queue_begin(&q);f.flat=1;f.near_z=.125f;
    CHECK(bh_radar_marker(&c,&player,&target,0,0,0) && close_to(value(q.commands[9].words+10),.126),"flat depth follows live non-one near-z plus.001");
    bh_draw_queue_dispose(&q);
}
static void gates(void) {
    BHDrawQueue q;BHRadarContext c;BHRadarFrame f;BHVec3 player={0,0,0},target={0,101,0};
    CHECK(bh_draw_queue_init(&q,20,NULL),"gate queue");frame(&f);bh_radar_bind(&c,&q,&f,17,18);
    CHECK(bh_radar_marker(&c,&player,&target,0,0,0) && c.culled==1 && q.count==9,"ordinary absolute height over100 culled");
    target.y=-101;CHECK(bh_radar_marker(&c,&player,&target,0,0,0) && c.culled==1 && q.count==9,"negative height has same cutoff");
    target.y=100;CHECK(bh_radar_marker(&c,&player,&target,0,0,0) && c.queued==1 && !value(q.commands[9].words+9) && !value(q.commands[9].words+12),"inclusive height100 boundary retains zero-size/zero-fade command");
    bh_draw_queue_begin(&q);target.y=1000;
    CHECK(bh_radar_marker(&c,&player,&target,0,0x80000000UL,0) && c.queued==1 && close_to(value(q.commands[9].words+9),.006) && close_to(value(q.commands[9].words+12),.3),"any nonzero raw fourth argument clamps height then multiplies.7");
    bh_draw_queue_begin(&q);target.x=400;target.y=0;
    CHECK(bh_radar_marker(&c,&player,&target,0,0,0) && c.culled==1 && q.count==9,"ordinary radial squared1 exceeds original.9 threshold");
    CHECK(bh_radar_marker(&c,&player,&target,0,1,0) && c.queued==1 && close_to(value(q.commands[9].words),1.1) && close_to(value(q.commands[9].words+12),.5),"extended outside marker normalizes to unit direction but stores radius.5");
    bh_draw_queue_begin(&q);f.visibility_word=8;
    CHECK(bh_radar_marker(&c,&player,&target,1,1,0) && c.culled==1 && q.count==9,"visibility bit8 suppresses both outside marker and bar");
    target.x=360;CHECK(bh_radar_marker(&c,&player,&target,0,1,0) && c.queued==1,"visibility word does not suppress in-range marker");
    bh_draw_queue_begin(&q);f.visibility_word=0;target.x=142.8050079345703f;target.z=-214.89566040039062f;
    CHECK(bh_radar_marker(&c,&player,&target,0,0,0) && q.commands[9].words[12]==0x3eb5bc94UL,
        "independent precision fixture distinguishes retained-times-stored radius factors from squared stored coordinates");target.z=0;
    bh_draw_queue_dispose(&q);CHECK(bh_draw_queue_init(&q,9,NULL),"full queue header-only capacity");c.queue=&q;f.visibility_word=0;target.x=0;
    CHECK(bh_radar_marker(&c,&player,&target,1,0,0) && c.attempted==2 && c.dropped==2 && !c.queued && q.count==9,"full queue silently drops both independent publications");
    CHECK(!bh_radar_marker(NULL,&player,&target,0,0,0) && !bh_radar_marker(&c,NULL,&target,0,0,0),"missing typed inputs rejected");
    {unsigned long nan=0x7fc00123UL;memcpy(&f.axis_y.y,&nan,4);}
    CHECK(!bh_radar_marker(&c,&player,&target,0,0,0) && c.error==BH_RADAR_ARGUMENT,"nonfinite context guard does not fabricate a queue command");bh_draw_queue_dispose(&q);
}
int bh_radar_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;modes();gates();
    CHECK(bh_math_environment_supported(),"required PC53 profile retained");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"radar\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_RADAR_STANDALONE
int main(void){unsigned long count;return bh_radar_checkpoint(&count);}
#endif
