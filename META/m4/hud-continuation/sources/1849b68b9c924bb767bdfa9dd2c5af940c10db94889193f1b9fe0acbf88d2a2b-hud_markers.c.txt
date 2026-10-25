#include "../src/bh_hud_markers.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures,calls,events[8];
static BHEntity *stop,*retag;
static int mutate_packet;
static unsigned char flags;static int mutate_flags;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL hud-markers: %s\n",s);}}while(0)
static void put_long(BHEntity *e,unsigned long offset,long value) {memcpy(e->payload+offset,&value,4);}
static void next_actor(BHEntity *e,BHEntity *next) {memcpy(e->payload+12,&next,4);}
/* Isolated observable query fixture. It is never installed by production code. */
static void __cdecl query(BHEntity *e,void *raw) {
    BHEntityPositionPacket *p=(BHEntityPositionPacket *)raw;
    if(calls<8)events[calls]=(unsigned long)p->event;
    ++calls;p->position=e->position;
    if(mutate_packet)p->event=99;
    if(mutate_flags)flags=2;
    if(e==stop)next_actor(e,NULL);
    if(e==retag)put_long(e,24,6);
}
static float word(const BHDrawCommand *c,unsigned long index) {
    float f;memcpy(&f,c->words+index,4);return f;
}
static unsigned long bits(float f) {unsigned long n;memcpy(&n,&f,4);return n;}
static int close(float a,double b) {return fabs((double)a-b)<.000002;}
static void setup(BHHudMarkers *m,BHRadarContext *r,BHRadarFrame *f,
    BHPickupDrawContext *p,BHMainCamera *camera,BHViewConfig *view,BHDrawQueue *q) {
    memset(m,0,sizeof(*m));memset(f,0,sizeof(*f));memset(p,0,sizeof(*p));
    memset(camera,0,sizeof(*camera));memset(view,0,sizeof(*view));
    camera->projected.m[0][0]=camera->projected.m[1][1]=camera->projected.m[2][2]=1;
    camera->focal=8;view->near_z=.25f;
    p->queue=q;p->camera=camera;p->view=view;
    f->flat=1;f->near_z=.25f;f->unit=2;f->screen_radius=16;f->screen_x=40;f->screen_y=30;
    f->yaw_x.x=f->yaw_y.z=1;
    CHECK(bh_radar_bind(r,q,f,17,18),"real marker owner binding");
    m->projection=p;m->radar=r;m->display_width=320;m->brackets_image=19;m->status_image=20;m->limit=8;
}
static void actors(BHEntity *e,unsigned long count,long kind) {
    unsigned long i;
    memset(e,0,count*sizeof(*e));
    for(i=0;i<count;++i) {
        e[i].query=query;e[i].position.x=20;e[i].position.y=12;e[i].position.z=4;
        put_long(e+i,24,kind);if(i+1<count)next_actor(e+i,e+i+1);
    }
    calls=0;stop=retag=NULL;mutate_packet=mutate_flags=0;flags=0;
}
static void payloads(BHHudMarkers *m,BHEntity *e,BHVec3 *player) {
    static const long kinds[4]={1,2,6,8};
    static const float u[4]={0,.25f,.5f,0},v[4]={.25f,.375f,.5f,-.25f};
    static const float xs[4]={1,9,1,9},ys[4]={-1,-1,7,7};
    static const unsigned long angles[4]={0,0x3fc90fdbUL,0xbfc90fdbUL,0xc0490fdbUL};
    unsigned long k,j,base;short status=-2;BHDrawQueue *q=m->projection->queue;
    for(k=0;k<4;++k) {
        actors(e,1,kinds[k]);memcpy(e->payload+48,&status,2);bh_draw_queue_begin(q);
        CHECK(bh_hud_markers_draw(m,e,player,&flags) && !m->error && calls==1 && events[0]==3,
            "real event3 query, class-specific marker and projected bracket owner");
        CHECK(m->visited==1 && m->projected==1 && m->attempted==(k<2?6UL:7UL) &&
            m->queued==m->attempted && !m->dropped,"all selected real producer calls accounted");
        base=k<2?10:11;
        for(j=0;j<4;++j) {
            BHDrawCommand *c=q->commands+base+j;
            CHECK(c->kind==BH_DRAW_RECT && c->resource==19 && word(c,0)==xs[j] && word(c,1)==ys[j] &&
                word(c,2)==8 && word(c,3)==8 && bits(word(c,4))==angles[j] && word(c,5)==.625f &&
                word(c,6)==u[k] && word(c,7)==.25f && word(c,8)==0 && word(c,9)==.25f &&
                close(word(c,10),.26) && !c->words[11],"four independent corner coordinates rotations atlas quadrants alpha depth");
        }
        {BHDrawCommand *c=q->commands+base+4;
         CHECK(c->resource==(k==3?20UL:19UL) && word(c,2)==(k==3?48:32) && word(c,3)==8 &&
             word(c,6)==0 && word(c,7)==(k==3?.75f:.5f) && word(c,8)==v[k] && word(c,9)==.125f,
             "kind8 signed status uses separate three-quarter atlas; other labels retain kind-specific rows");
         CHECK(word(c,0)==(k==3?20:13) && word(c,1)==(k==3?16:17),"independent unit-scaled label placement");}
    }
    actors(e,1,6);flags=2;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && m->attempted==6 && q->commands[9].resource==18,
        "player bit2 suppresses only kind6 main radar marker; its bar and all brackets remain");
    actors(e,1,1);e->position.z=1;
    bh_draw_queue_begin(q);CHECK(bh_hud_markers_draw(m,e,player,&flags) && word(q->commands+10,5)==1,
        "raw scale alpha above1 clamps to exactly1");
}
static void gates(BHHudMarkers *m,BHEntity *e,BHVec3 *player) {
    BHDrawQueue *q=m->projection->queue;BHViewConfig *view=(BHViewConfig *)m->projection->view;
    unsigned long original_capacity=q->capacity;
    actors(e,1,2);e->position.z=32;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && m->projected==1 && m->attempted==1 && q->count==10,
        "stored alpha below double0.1 suppresses brackets after radar submission");
    e->position.z=.125f;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && !m->projected && m->attempted==1,
        "real point projector rejects near-plane point while radar marker remains");
    e->position.z=4;view->near_z=1.125f;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && close(word(q->commands+10,10),1.135),
        "live non-default near plane plus0.01 depth clamp, not stale initial near");
    view->near_z=.25f;e->position.z=16;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && word(q->commands+10,10)==8,
        "depth above near clamp remains projected z minus8");
    actors(e,2,1);q->capacity=11;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && m->visited==2 && calls==2 &&
        m->queued==2 && m->dropped==10 && q->count==11,"full queue retains early appends and continues query traversal");
    q->capacity=original_capacity;
}
static void mutation(BHHudMarkers *m,BHEntity *e,BHVec3 *player) {
    BHDrawQueue *q=m->projection->queue;
    actors(e,2,2);mutate_packet=1;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && calls==2 && events[0]==3 && events[1]==99,
        "original packet event is initialized once; next query observes previous mutation");
    actors(e,2,2);stop=e;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && m->visited==1 && calls==1,
        "successor loaded after callback rather than cached before it");
    actors(e,1,2);retag=e;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && m->attempted==7 && word(q->commands+11,6)==.5f,
        "kind selected after query mutation");
    actors(e,1,6);mutate_flags=1;bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,e,player,&flags) && m->attempted==6 && q->commands[9].resource==18,
        "player flag byte is read after callback mutation rather than captured before dispatch");
    actors(e,2,1);e[1].query=NULL;bh_draw_queue_begin(q);
    CHECK(!bh_hud_markers_draw(m,e,player,&flags) && m->error==BH_HUD_MARKERS_CALLBACK &&
        m->visited==2 && q->count==15,"missing query guard preserves preceding actor's real draw submissions");
    actors(e,1,9);bh_draw_queue_begin(q);
    CHECK(!bh_hud_markers_draw(m,e,player,&flags) && m->error==BH_HUD_MARKERS_KIND && calls==1 && q->count==9,
        "unsupported original stale-atlas domain diagnosed after query; no invented UV defaults");
    actors(e,1,2);next_actor(e,e);m->limit=2;bh_draw_queue_begin(q);
    CHECK(!bh_hud_markers_draw(m,e,player,&flags) && m->error==BH_HUD_MARKERS_LIMIT && calls==2,
        "cyclic actor list is a bounded source error after prior effects");
    m->limit=8;actors(e,1,2);bh_draw_queue_begin(q);
    CHECK(bh_hud_markers_draw(m,NULL,player,&flags) && !m->visited && q->count==9,"empty list leaves real queue unchanged");
}
int bh_hud_markers_checkpoint(unsigned long *count) {
    BHDrawQueue q;BHHudMarkers m;BHRadarContext radar;BHRadarFrame frame;
    BHPickupDrawContext projection;BHMainCamera camera;BHViewConfig view;
    BHEntity e[2];BHVec3 player={0,0,0};BHMathEnvironment fp;
    checks=failures=0;memset(&q,0,sizeof(q));
    if(!bh_math_environment_enter(&fp))return 2;
    CHECK(bh_draw_queue_init(&q,80,NULL),"real source draw queue allocation");
    if(q.commands){setup(&m,&radar,&frame,&projection,&camera,&view,&q);payloads(&m,e,&player);gates(&m,e,&player);mutation(&m,e,&player);}
    CHECK(bh_draw_queue_dispose(&q),"real queue lifetime release");
    CHECK(bh_math_environment_supported(),"PC53 source profile retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"hud-markers\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
