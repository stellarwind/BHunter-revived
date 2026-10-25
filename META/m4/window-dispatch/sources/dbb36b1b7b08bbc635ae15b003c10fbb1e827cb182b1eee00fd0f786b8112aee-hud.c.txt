#include "../src/bh_hud.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "hud_fixtures.h"

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL hud: %s\n",s);}}while(0)
static float value(const unsigned long *word){float f;memcpy(&f,word,4);return f;}
static void hud_bind(BHHudContext *c,BHAttachmentContext *a,BHRadarFrame *f,BHDrawQueue *q,BHHudPlayer *p) {
    memset(c,0,sizeof(*c));memset(a,0,sizeof(*a));memset(f,0,sizeof(*f));memset(p,0,sizeof(*p));
    c->frame=f;c->attachment=a;c->background_image=99;a->frame=f;a->queue=q;a->viewport_width=640;
    a->default_image=17;a->flag200_image=18;a->flag400_image=19;a->flag800_image=20;
    p->right.x=p->up.y=p->forward.z=1;f->near_z=.125f;f->visibility_word=0x12345678UL;
}
static void precision(void) {
    BHHudContext c;BHAttachmentContext a;BHRadarFrame f;BHDrawQueue q;BHHudPlayer p,before;
    unsigned long i,j,words[21];
    CHECK(sizeof(BHHudPlayer)==0xec && offsetof(BHHudPlayer,position)==0xc &&
        offsetof(BHHudPlayer,right)==0x18 && offsetof(BHHudPlayer,up)==0x24 &&
        offsetof(BHHudPlayer,forward)==0x30 && offsetof(BHHudPlayer,yaw)==0x3c &&
        offsetof(BHHudPlayer,root)==0xc4 && offsetof(BHHudPlayer,flat_raw)==0xe8,
        "verified separate player prefix offsets and extent");
    CHECK(bh_draw_queue_init(&q,20,NULL),"precision real queue owner");hud_bind(&c,&a,&f,&q,&p);
    for(i=0;i<sizeof(hud_fixtures)/sizeof(hud_fixtures[0]);++i){
        memset(p.opaque0,0xa5,sizeof(p.opaque0));memset(p.opaque1,0x3c,sizeof(p.opaque1));memset(p.opaque2,0x7e,sizeof(p.opaque2));
        memcpy(&p.position,hud_fixtures[i].input,sizeof(hud_fixtures[i].input));before=p;
        f.unit=23;f.screen_radius=67;f.screen_x=101;f.screen_y=-31;c.screen_diameter=83;
        a.player_position.x=42;a.player_position.y=-7;a.player_position.z=13;a.yaw=3;
        CHECK(bh_hud_setup(&c,&p),"whole setup with null world root");
        memcpy(words,&f.origin,18*4);memcpy(words+18,&c.world_center,3*4);
        for(j=0;j<21;++j){
            ++checks;if(words[j]!=hud_fixtures[i].expected[j]){
                ++failures;printf("FAIL hud precision fixture%lu word%lu actual%08lx expected%08lx\n",i,j,words[j],hud_fixtures[i].expected[j]);
            }
        }
        CHECK(!memcmp(&p,&before,sizeof(p)),"player and opaque input bytes retained");
        CHECK(!f.flat && f.unit==23 && f.screen_radius==67 && f.screen_x==101 && f.screen_y==-31 && c.screen_diameter==83,
            "world path retains all prior screen metrics");
        CHECK(f.near_z==.125f && f.visibility_word==0x12345678UL,"live near plane and visibility inputs preserved");
        CHECK(a.player_position.x==42 && a.player_position.y==-7 && a.player_position.z==13 && a.yaw==3,
            "null root retains attachment position and yaw globals");
        CHECK(q.count==9 && !c.attempted,"world null-root setup queues no commands");
    }
    bh_draw_queue_dispose(&q);
}
static void metrics(void) {
    BHHudContext c;BHAttachmentContext a;BHRadarFrame f;BHDrawQueue q;BHHudPlayer p;
    static const long widths[]={0,1,79,80,81,639,640,641,800,1920,-1,-79,-80,-81,-640,2147483647L,(-2147483647L-1)};
    static const long units[]={0,0,0,1,1,7,8,8,10,24,0,0,-1,-1,-8,26843545,-26843545};
    unsigned long i;BHDrawCommand *command;float expected_x,expected_y,expected_diameter;
    CHECK(bh_draw_queue_init(&q,12,NULL),"flat background queue");hud_bind(&c,&a,&f,&q,&p);
    p.flat_raw=(long)0x80000000UL;
    for(i=0;i<sizeof(widths)/sizeof(widths[0]);++i){
        bh_draw_queue_begin(&q);a.viewport_width=widths[i];
        CHECK(bh_hud_setup(&c,&p) && f.flat==1 && c.attempted==1 && c.queued==1 && !c.dropped,
            "any raw nonzero flag produces flat background");
        CHECK(f.unit==units[i] && f.screen_radius==units[i]*8 && f.screen_x==widths[i]-units[i]*9 &&
            f.screen_y==units[i]*9 && c.screen_diameter==units[i]*16,"signed width/80 truncation and all screen-global stores");
        command=q.commands+9;
        /* VC5 may retain an unrounded FILD value across a C float cast in a
           comparison. Expected payloads require actual binary32 stores. */
        bh_round_float(&expected_x,(double)f.screen_x);bh_round_float(&expected_y,(double)f.screen_y);
        bh_round_float(&expected_diameter,(double)c.screen_diameter);
        CHECK(command->kind==BH_DRAW_SCREEN_QUAD && command->resource==99 && q.commands[8].next==9 && command->next==BH_DRAW_END,
            "real bucket8 full-image background producer");
        CHECK(value(command->words)==expected_x && value(command->words+1)==expected_y &&
            value(command->words+2)==expected_diameter && value(command->words+3)==expected_diameter &&
            value(command->words+4)==0 && value(command->words+5)==1 && !command->words[6],"exact signed background payload and opacity");
    }
    bh_draw_queue_dispose(&q);CHECK(bh_draw_queue_init(&q,9,NULL),"header-only full background queue");a.queue=&q;a.viewport_width=640;
    CHECK(bh_hud_setup(&c,&p) && c.dropped==1 && !c.queued && f.flat && f.unit==8 && q.count==9,
        "background full queue silently drops but all setup state completes");bh_draw_queue_dispose(&q);
}
static void graph(void) {
    BHHudContext c;BHAttachmentContext a;BHRadarFrame f;BHDrawQueue q;BHHudPlayer p;
    BHAttachmentNode root,child;BHRadarContext radar;BHVec3 target={100,0,0};unsigned long mode;
    memset(&root,0,sizeof(root));memset(&child,0,sizeof(child));
    root.minimum.x=root.minimum.y=root.minimum.z=-5;root.maximum.x=root.maximum.y=root.maximum.z=5;
    child.minimum=root.minimum;child.maximum=root.maximum;child.minimum.x=15;child.maximum.x=25;
    root.navigation.portals[0].neighbor=&child.navigation;root.navigation.portals[0].direction=0;root.navigation.portals[0].plane=20;
    child.navigation.portals[5].neighbor=&root.navigation;child.navigation.portals[5].direction=0;child.navigation.portals[5].plane=0;
    CHECK(bh_draw_queue_init(&q,20,NULL),"real HUD/recursive-graph/radar-marker queue");hud_bind(&c,&a,&f,&q,&p);p.root=&root;p.yaw=.25f;
    CHECK(bh_radar_bind(&radar,&q,&f,27,28),"real downstream marker binding consumes HUD-produced frame");
    for(mode=0;mode<2;++mode){
        bh_draw_queue_begin(&q);p.flat_raw=mode;
        CHECK(bh_hud_setup(&c,&p) && a.visited==2 && a.attempted==2 && a.queued==2,"whole setup dispatches real recursive graph in both modes");
        CHECK(!root.radar_visited && !child.radar_visited && root.navigation.portals[0].neighbor==&child.navigation &&
            child.navigation.portals[5].neighbor==&root.navigation,"HUD invokes guarded reset after graph drawing, including cycle");
        CHECK(a.yaw==.25f && !memcmp(&a.player_position,&p.position,sizeof(BHVec3)),"HUD binds exact player origin/yaw before graph draw");
        CHECK(q.count==11+mode && q.commands[9+mode].resource==17 && q.commands[9+mode].kind==(mode?BH_DRAW_SCREEN_QUAD:BH_DRAW_ORIENTED),
            "background precedes actual world/flat graph connectors");
        if(mode)CHECK(q.commands[8].next==9 && q.commands[9].resource==99 && q.commands[9].next==10,"flat bucket preserves background then graph order");
        CHECK(bh_radar_marker(&radar,&p.position,&target,0,0,0) && radar.queued==1 && q.commands[q.count-1].resource==27,
            "real radar marker consumes setup world/yaw/flat state after graph reset");
    }
    bh_draw_queue_dispose(&q);CHECK(bh_draw_queue_init(&q,9,NULL),"full recursive HUD queue");a.queue=&q;p.flat_raw=1;
    CHECK(bh_hud_setup(&c,&p) && c.dropped==1 && a.dropped==2 && a.visited==2 && !root.radar_visited && !child.radar_visited,
        "full queue retains graph traversal and post-render reset");
    root.navigation.portals[0].direction=3;child.radar_visited=1;
    CHECK(!bh_hud_setup(&c,&p) && c.error==BH_HUD_ATTACHMENT && a.error==BH_ATTACHMENT_PORTAL &&
        !root.radar_visited && !child.radar_visited,"source render failure still executes whole guarded graph reset");
    root.navigation.portals[0].direction=0;root.radar_visited=1;child.radar_visited=1;
    CHECK(bh_hud_setup(&c,&p) && !a.visited && !a.attempted && !root.radar_visited && !child.radar_visited,
        "already-marked root suppresses render then recursively resets dirty graph");
    bh_draw_queue_dispose(&q);
}
static void guards(void) {
    BHHudContext c;BHAttachmentContext a;BHRadarFrame f,before;BHDrawQueue q;BHHudPlayer p;
    unsigned long nan=0x7fc00000UL;
    CHECK(bh_draw_queue_init(&q,12,NULL),"source argument guard queue");hud_bind(&c,&a,&f,&q,&p);before=f;
    CHECK(!bh_hud_setup(NULL,&p) && !bh_hud_setup(&c,NULL) && c.error==BH_HUD_ARGUMENT,"missing source bindings rejected");
    a.frame=NULL;CHECK(!bh_hud_setup(&c,&p) && c.error==BH_HUD_ARGUMENT && !memcmp(&f,&before,sizeof(f)),"different frame binding rejected before mutation");a.frame=&f;
    memcpy(&p.yaw,&nan,4);CHECK(!bh_hud_setup(&c,&p) && c.error==BH_HUD_NUMERICAL && !memcmp(&f,&before,sizeof(f)) && q.count==9,
        "nonfinite source guard rejects before setup or queue mutation");p.yaw=0;p.flat_raw=1;q.commands[8].resource=100;
    CHECK(!bh_hud_setup(&c,&p) && c.error==BH_HUD_QUEUE && f.flat && f.unit==8,"source queue topology error still completes setup state");
    bh_draw_queue_dispose(&q);
}
int bh_hud_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    precision();metrics();graph();guards();CHECK(bh_math_environment_supported(),"required PC53 profile retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"hud\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_HUD_STANDALONE
int main(void){unsigned long count;return bh_hud_checkpoint(&count);}
#endif

