#include "../src/bh_attachment.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "attachment_cases.h"

static unsigned long checks,failures;
#define CHECK(x,label) do{++checks;if(!(x)){++failures;printf("FAIL attachment line %d: %s\n",__LINE__,label);}}while(0)
static int close_to(float x,double expected) {return fabs((double)x-expected)<.00002;}
static float value(const BHDrawCommand *p,unsigned long n) {float f;memcpy(&f,p->words+n,4);return f;}
static float from_word(unsigned long word) {float f;memcpy(&f,&word,4);return f;}
static void nodes(BHAttachmentNode *n,unsigned long count) {
    unsigned long i;memset(n,0,count*sizeof(*n));
    for(i=0;i<count;++i){n[i].minimum.y=-1;n[i].maximum.y=1;}
}
static void edge(BHAttachmentNode *from,unsigned long slot,BHAttachmentNode *to,
                 unsigned long direction,long plane,float u,float v) {
    BHPortal *p=from->navigation.portals+slot;
    p->direction=direction;p->plane=plane;p->neighbor=&to->navigation;
    /* Deliberately disagree with aperture/scratch: render reads overlap. */
    p->aperture[0]=p->aperture[1]=999;p->aperture[2]=p->aperture[3]=-999;
    p->overlap[0]=u-2;p->overlap[1]=u+2;p->overlap[2]=v-4;p->overlap[3]=v+4;
    p->scratch[0]=p->scratch[1]=p->scratch[2]=p->scratch[3]=888;
}
static void context(BHAttachmentContext *c,BHDrawQueue *q,BHRadarFrame *f) {
    memset(c,0,sizeof(*c));memset(f,0,sizeof(*f));
    c->queue=q;c->frame=f;c->viewport_width=640;
    c->default_image=10;c->flag200_image=20;c->flag400_image=30;c->flag800_image=40;
    f->flat=1;f->screen_x=100;f->screen_y=120;f->screen_radius=64;
    f->origin.x=1;f->origin.y=2;f->origin.z=3;
    f->axis_x.x=1;f->axis_y.z=1;f->normal.y=1;
}
static void reset_cases(void) {
    BHAttachmentNode n[5],saved[5];unsigned long i;
    nodes(n,5);edge(n,0,n+1,0,1,0,0);edge(n,5,n+2,0,1,0,0);
    edge(n+1,1,n+3,0,1,0,0);edge(n+2,2,n+3,0,1,0,0);edge(n+3,4,n,0,1,0,0);
    for(i=0;i<5;++i)n[i].radar_visited=9;memcpy(saved,n,sizeof(n));
    bh_attachment_reset(n);
    CHECK(!n[0].radar_visited && !n[1].radar_visited && !n[2].radar_visited && !n[3].radar_visited && n[4].radar_visited==9,"reset cycle and diamond clear reachable marks only");
    for(i=0;i<4;++i)saved[i].radar_visited=0;
    CHECK(!memcmp(n,saved,sizeof(n)),"reset changes only visited fields, not links/resources/bounds");
    n[1].radar_visited=7;bh_attachment_reset(n);
    CHECK(n[1].radar_visited==7,"already-clear parent stops without descendant traversal");
    bh_attachment_reset(n+1);CHECK(!n[1].radar_visited,"independent marked descendant resets");
    bh_attachment_reset(NULL);
}
static void directions_and_images(BHAttachmentContext *c,BHRadarFrame *f,BHDrawQueue *q) {
    BHAttachmentNode n[2];unsigned long d,k;double x,z,r,len,alpha;
    const unsigned long directions[6]={0,1,2,4,5,6};
    const unsigned long flags[8]={0,0x200,0x400,0x800,0x600,0xa00,0xc00,0xe00};
    const unsigned long images[8]={10,20,30,40,20,20,30,20};
    for(d=0;d<6;++d) {
        nodes(n,2);edge(n,5,n+1,directions[d],80,20,40);
        bh_draw_queue_begin(q);
        CHECK(bh_attachment_render(c,n) && c->visited==2 && c->attempted==1 && q->count==10,"all signed portal axes, six-slot scan ignores count and follows slot5");
        x=(directions[d]&3)==0?80:20;z=(directions[d]&3)==2?80:40;
        r=sqrt(x*x+z*z);len=r*.16;alpha=1-r*.00125;
        CHECK(q->commands[9].kind==BH_DRAW_SCREEN_QUAD && q->commands[9].resource==10,"real full screen queue producer and handle");
        CHECK(close_to(value(q->commands+9,0),100+x*.08) && close_to(value(q->commands+9,1),120-z*.08),"portal plane integer and overlap midpoint mapping");
        CHECK(close_to(value(q->commands+9,2),5) && close_to(value(q->commands+9,3),len) && close_to(value(q->commands+9,4),atan2(x,z)) && close_to(value(q->commands+9,5),alpha),"independent width length atan2(x,z) and radial fade");
        bh_attachment_reset(n);
    }
    for(k=0;k<8;++k) {
        nodes(n,2);edge(n,0,n+1,0,100,0,0);n->query_flags=flags[k];bh_draw_queue_begin(q);
        CHECK(bh_attachment_render(c,n) && q->commands[9].resource==images[k],"material image bit200 then400 then800 precedence");bh_attachment_reset(n);
    }
    nodes(n,2);edge(n,0,n+1,0,-100,0,0);c->yaw=1.5707963267948966f;bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && close_to(value(q->commands+9,0),100) && close_to(value(q->commands+9,1),128) && close_to(value(q->commands+9,4),-3.1415926973014556),"negative plane plus yaw rotation preserves angle without wrapping");
    bh_attachment_reset(n);c->yaw=0;
    f->flat=0;nodes(n,2);edge(n,1,n+1,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && q->commands[9].kind==BH_DRAW_ORIENTED,"world branch real oriented producer");
    CHECK(close_to(value(q->commands+9,0),1.0125) && close_to(value(q->commands+9,1),2) && close_to(value(q->commands+9,2),3) &&
          close_to(value(q->commands+9,9),.01) && close_to(value(q->commands+9,10),.025) && close_to(value(q->commands+9,12),.875),"world center axes, .00025 scale, width and fade");
    CHECK(!memcmp(q->commands[9].words+3,&f->axis_y,12) && !memcmp(q->commands[9].words+6,&f->normal,12),"world up and normal borrowed from actual HUD frame");
    bh_attachment_reset(n);f->flat=1;
}
static void graph_cases(BHAttachmentContext *c,BHDrawQueue *q) {
    BHAttachmentNode n[8];unsigned long i;
    nodes(n,8);edge(n,1,n+1,0,-100,0,0);edge(n,5,n+2,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && c->visited==3 && c->attempted==1 && q->count==10,"degree2 one portal-to-portal segment and both visits");
    CHECK(close_to(value(q->commands+9,0),100) && close_to(value(q->commands+9,1),120) && close_to(value(q->commands+9,3),32) && close_to(value(q->commands+9,5),.75),"degree2 center is portal midpoint, not room bounds");
    bh_attachment_reset(n);
    /* Child0 emits handle20 before child5's handle30. Backedges test that mark
       is set before traversal; branches share leaf3 without double visiting. */
    nodes(n,8);edge(n,1,n+1,0,-100,0,0);edge(n,5,n+2,0,100,0,0);
    edge(n+1,0,n,0,-50,0,0);edge(n+1,4,n+3,0,-25,0,0);n[1].query_flags=0x200;
    edge(n+2,2,n+3,0,25,0,0);n[2].query_flags=0x400;
    edge(n+3,0,n,0,0,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && c->visited==4 && c->attempted==4,"cycles/shared nodes terminate with one render per visited node");
    CHECK(q->commands[9].resource==10 && q->commands[10].resource==20 && q->commands[11].resource==10 && q->commands[12].resource==30,"recursive first neighbor before tail second, stable queue order");
    bh_attachment_reset(n);CHECK(!n->radar_visited && !n[1].radar_visited && !n[2].radar_visited && !n[3].radar_visited,"reset restores complete cyclic render family");
    nodes(n,8);for(i=0;i<6;++i)edge(n,i,n+i+1,0,(long)(20+i*10),0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && c->visited==7 && c->attempted==6,"six-slot junction emits six connectors in slot order");
    for(i=0;i<6;++i)CHECK(close_to(value(q->commands+9+i,0),100+(20+i*10)*.08),"junction portal slot output order");
    bh_attachment_reset(n);n->radar_visited=1;n[1].radar_visited=0;bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && !c->visited && !c->attempted && !n[1].radar_visited,"render already-marked root does not traverse or queue");
    bh_attachment_reset(n);
}
static void clipping_and_height(BHAttachmentContext *c,BHRadarFrame *f,BHDrawQueue *q) {
    BHAttachmentNode n[4];
    nodes(n,4);edge(n,0,n+1,0,500,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && c->visited==1 && c->queued==1 && !n[1].radar_visited,"inside to outside clips portal to400 and suppresses visit");
    CHECK(close_to(value(q->commands+9,0),132) && close_to(value(q->commands+9,3),64) && close_to(value(q->commands+9,5),.5),"radial endpoint interpolation expected clipped length midpoint fade");bh_attachment_reset(n);
    nodes(n,4);n->minimum.x=n->maximum.x=500;edge(n,0,n+1,0,100,0,0);edge(n,1,n+2,0,0,0,0);edge(n,2,n+3,0,200,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && c->visited==4 && c->queued==3,"outside room clips center once and continues eligible portal visits");
    CHECK(close_to(value(q->commands+9,0),140) && close_to(value(q->commands+10,0),132) && close_to(value(q->commands+11,0),148),"later junction slots reuse mutated400 center");bh_attachment_reset(n);
    nodes(n,4);edge(n,0,n+1,0,400,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && c->queued==1 && !n[1].radar_visited,"exact radius400 emits endpoint but strict visit excludes equality");bh_attachment_reset(n);
    nodes(n,4);n->minimum.x=n->maximum.x=400;edge(n,0,n+1,0,400,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && !c->queued && c->culled==1 && !n[1].radar_visited,"both radius400 endpoints cull equality");bh_attachment_reset(n);
    nodes(n,4);n->minimum.x=n->maximum.x=500;edge(n,1,n+1,0,-500,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && !c->queued && !n[1].radar_visited,"both endpoints outside cull even when geometric segment crosses circle");bh_attachment_reset(n);
    nodes(n,4);n->minimum.y=n->maximum.y=25;edge(n,0,n+1,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && close_to(value(q->commands+9,2),7.5) && close_to(value(q->commands+9,5),.4375) && n[1].radar_visited,"below-room signed height widens connector while absolute height fades");bh_attachment_reset(n);
    nodes(n,4);n->minimum.y=n->maximum.y=-25;edge(n,0,n+1,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && close_to(value(q->commands+9,2),2.5) && close_to(value(q->commands+9,5),.4375),"above-room height narrows same absolute fade");bh_attachment_reset(n);
    nodes(n,4);n->minimum.y=n->maximum.y=50;edge(n,0,n+1,0,-100,0,0);edge(n,5,n+2,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && !c->queued && n[1].radar_visited && !n[2].radar_visited && c->visited==2,"zero height fade still visits degree2 first endpoint but suppresses second");bh_attachment_reset(n);
    nodes(n,4);n->minimum.y=n->maximum.y=-100;edge(n,5,n+1,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && !c->queued && c->visited==1 && !n[1].radar_visited,"height fade clamp stops general-branch child");bh_attachment_reset(n);
    c->viewport_width=-640;nodes(n,4);edge(n,1,n+1,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && close_to(value(q->commands+9,2),-5),"viewport width remains signed integer");bh_attachment_reset(n);c->viewport_width=640;
    f->flat=0;nodes(n,4);n->minimum.y=n->maximum.y=25;edge(n,1,n+1,0,100,0,0);bh_draw_queue_begin(q);
    CHECK(bh_attachment_render(c,n) && close_to(value(q->commands+9,9),.015) && close_to(value(q->commands+9,12),.4375),"world signed width and absolute fade same contract");bh_attachment_reset(n);f->flat=1;
}
static void failure_cases(BHAttachmentContext *c,BHDrawQueue *q) {
    BHAttachmentNode n[3];BHDrawQueue tight_queue;unsigned long invalid=0x7fc00000UL;
    nodes(n,3);edge(n,0,n+1,0,-100,0,0);edge(n,5,n+2,0,100,0,0);
    memset(&tight_queue,0,sizeof(tight_queue));CHECK(bh_draw_queue_init(&tight_queue,9,NULL),"real full queue allocation");c->queue=&tight_queue;
    CHECK(bh_attachment_render(c,n) && c->dropped==1 && c->visited==3 && !c->queued && c->error==BH_ATTACHMENT_OK,"queue full silently drops while complete graph still visits");bh_attachment_reset(n);bh_draw_queue_dispose(&tight_queue);c->queue=q;
    bh_draw_queue_begin(q);q->commands[8].resource=q->capacity;
    CHECK(!bh_attachment_render(c,n) && c->error==BH_ATTACHMENT_QUEUE && n->radar_visited && !n[1].radar_visited,"malformed queue reports error after root mark, before recursion");bh_attachment_reset(n);
    bh_draw_queue_begin(q);n->navigation.portals[0].direction=3;
    CHECK(!bh_attachment_render(c,n) && c->error==BH_ATTACHMENT_PORTAL && n->radar_visited,"unsupported direction source guard preserves mark and reports failure");bh_attachment_reset(n);
    n->navigation.portals[0].direction=0;memcpy(&n->navigation.portals[0].overlap[0],&invalid,4);
    CHECK(!bh_attachment_render(c,n) && c->error==BH_ATTACHMENT_NUMERICAL,"nonfinite portal source guard");bh_attachment_reset(n);
    CHECK(!bh_attachment_render(c,NULL) && c->error==BH_ATTACHMENT_ARGUMENT,"null root source guard");
}
static void retained_precision_cases(BHAttachmentContext *c,BHRadarFrame *f,BHDrawQueue *q) {
    BHAttachmentNode n[3];unsigned long i,j,expected,got,index;const AttachmentCase *t;
    static const unsigned long world_words[7]={0,1,2,9,10,11,12};
    for(i=0;i<sizeof(attachment_cases)/sizeof(attachment_cases[0]);++i) {
        t=attachment_cases+i;nodes(n,3);
        edge(n,0,n+1,1,0,0,0);edge(n,5,n+2,5,0,0,0);
        for(j=0;j<2;++j) {
            n->navigation.portals[0].overlap[j]=from_word(t->a[0]);
            n->navigation.portals[0].overlap[j+2]=from_word(t->a[1]);
            n->navigation.portals[5].overlap[j]=from_word(t->b[0]);
            n->navigation.portals[5].overlap[j+2]=from_word(t->b[1]);
        }
        c->player_position.x=from_word(t->player[0]);c->player_position.z=from_word(t->player[1]);
        c->yaw=from_word(t->yaw);f->flat=t->screen;bh_draw_queue_begin(q);
        CHECK(bh_attachment_render(c,n) && c->attempted==1,"instruction-derived off-axis precision case submits once");
        for(j=0;j<(t->screen?6UL:7UL);++j) {
            index=t->screen?j:world_words[j];expected=t->expected[j];got=q->commands[9].words[index];
            if(got!=expected)printf("attachment fixture %s field%lu got%08lx expected%08lx\n",t->name,j,got,expected);
            CHECK(got==expected,"exact static PC53 retained-v-stored fixture output word");
        }
        bh_attachment_reset(n);
    }
    c->player_position.x=c->player_position.z=c->yaw=0;f->flat=1;
}
int __cdecl bh_attachment_checkpoint(unsigned long *count) {
    BHDrawQueue q;BHRadarFrame f;BHAttachmentContext c;BHMathEnvironment fp;
    checks=failures=0;memset(&q,0,sizeof(q));if(!bh_math_environment_enter(&fp))return 2;
    CHECK(sizeof(BHPortal)==60 && offsetof(BHPortal,neighbor)==56 && offsetof(BHWorldGeometry,navigation)==0,"real restored portal layout and first component");
    CHECK(bh_draw_queue_init(&q,128,NULL),"real source queue setup");
    if(q.commands){context(&c,&q,&f);reset_cases();directions_and_images(&c,&f,&q);graph_cases(&c,&q);clipping_and_height(&c,&f,&q);failure_cases(&c,&q);retained_precision_cases(&c,&f,&q);bh_draw_queue_dispose(&q);}
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"attachment\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_ATTACHMENT_STANDALONE
int main(void){unsigned long count;return bh_attachment_checkpoint(&count);}
#endif

