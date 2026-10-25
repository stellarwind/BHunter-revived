#include "../src/bh_hud_scores.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL hud-scores: %s\n",s);}}while(0)
static void remote(BHEntity *e,short score,const char *name) {
    memset(e,0,sizeof(*e));memcpy(e->payload+56,&score,2);strcpy((char *)e->payload+58,name);
}
static float word(const BHDrawCommand *c,unsigned long index) {float v;memcpy(&v,c->words+index,4);return v;}
static int close(float x,double y) {return fabs((double)x-y)<.000002;}
static void font(BHAtlasPage *page,BHAtlasImage *atlas) {
    unsigned long i;memset(page,0,sizeof(*page));memset(atlas,0,sizeof(*atlas));
    page->atlas=atlas;atlas->rectangle_count=224;
    for(i=0;i<224;++i)atlas->rectangles[i].width=4;
}
static void texts(void) {
    BHTextQueue q;BHAtlasPage page;BHAtlasImage atlas;BHHudScoreText c;
    BHHudScores s;BHEntity a,b;BHEntity *slots[8];BHTextCommand *t;
    unsigned long i;
    memset(slots,0,sizeof(slots));remote(&a,-32768,"Remote A");remote(&b,32767,"Remote B");
    slots[1]=&a;slots[7]=&b;bh_text_queue_init(&q);font(&page,&atlas);
    memset(&c,0,sizeof(c));c.queue=&q;c.page=&page;s.remote=slots;s.local_name="Local";s.local_index=3;s.local_score=(-2147483647L-1);
    CHECK(bh_hud_scores_text(&c,&s) && c.attempted==7 && c.queued==7 && !c.dropped && bh_text_queue_count(&q)==7,
        "single clear and ordered remote/local name/score pairs use actual ring producers");
    CHECK(q.commands[0].kind==BH_TEXT_COMMAND_CLEAR && !q.commands[0].position.color && q.commands[0].page==&page,"clear precedes first present slot even with leading holes");
    CHECK(!strcmp((char *)q.commands[1].text,"Remote A") && q.commands[1].position.xy[0]==1 && q.commands[1].position.xy[1]==18 &&
        !strcmp((char *)q.commands[2].text,"-32768") && q.commands[2].position.xy[0]==102,"remote signed short formatting and exact font-based right alignment");
    CHECK(!strcmp((char *)q.commands[3].text,"Remote B") && q.commands[3].position.xy[1]==114 &&
        !strcmp((char *)q.commands[4].text,"32767") && q.commands[4].position.xy[0]==106,"last of eight slots and positive signed16 score");
    CHECK(!strcmp((char *)q.commands[5].text,"Local") && q.commands[5].position.xy[1]==50 &&
        !strcmp((char *)q.commands[6].text,"-2147483648") && q.commands[6].position.xy[0]==82,"local signed32 minimum is not truncated to remote16 bits");
    for(i=0;i<7;++i)CHECK(q.commands[i].page==&page,"all real queued text borrows live page identity");
    /* Font widths are signed shorts; original subtracts full width before
       producer's signed16 coordinate packing. */
    atlas.rectangles['1'-32].width=0xffff;
    s.local_score=111;s.local_index=4096;memset(slots,0,sizeof(slots));bh_text_queue_init(&q);
    CHECK(bh_hud_scores_text(&c,&s) && c.attempted==3 && q.commands[1].position.xy[1]==2 &&
        q.commands[2].position.xy[0]==129,"empty remote roster still clears once; wrapped slot coordinate and signed metrics preserved");
    bh_text_queue_init(&q);slots[3]=&a;s.local_index=3;s.local_score=7;
    CHECK(bh_hud_scores_text(&c,&s) && c.attempted==5 && !strcmp((char *)q.commands[1].text,"Remote A") &&
        !strcmp((char *)q.commands[3].text,"Local") && q.commands[1].position.xy[1]==q.commands[3].position.xy[1],
        "text builder retains populated local remote slot before overwriting same row with local name");
    q.read=1;q.write=0;
    CHECK(bh_hud_scores_text(&c,&s) && c.attempted==5 && !c.queued && c.dropped==5 && q.read==1 && q.write==0,
        "full text ring drops clear and both row pairs without fictitious publication");
    bh_text_queue_init(&q);s.local_name="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    CHECK(bh_hud_scores_text(&c,&s),"long local name valid borrowed source");t=q.commands+3;
    CHECK(strlen((const char *)t->text)==59 && !strncmp((const char *)t->text,s.local_name,59),"actual strncpy producer limits name to59 bytes and terminates");
    c.queue=NULL;CHECK(!bh_hud_scores_text(&c,&s) && c.error==BH_HUD_SCORES_ARGUMENT,"source missing queue binding guard");
}
static void ranks(void) {
    static const long expected[7]={0,2,1,7,5,6,3};
    BHEntity actors[8],*slots[8];BHHudScores s;BHHudScoreDraw c;BHDrawQueue q;
    unsigned long i;long row;memset(&q,0,sizeof(q));memset(&c,0,sizeof(c));
    for(i=0;i<8;++i){remote(actors+i,0,"Peer");slots[i]=actors+i;}
    remote(actors,100,"First");remote(actors+1,30,"Tie A");remote(actors+2,100,"Second");remote(actors+3,-32768,"Last");
    slots[4]=NULL;remote(actors+5,700,"Local duplicate ignored");remote(actors+6,-4,"Negative");remote(actors+7,30,"Tie B");
    s.remote=slots;s.local_name="Local";s.local_index=5;s.local_score=30;
    CHECK(bh_draw_queue_init(&q,32,NULL),"real scoreboard rectangle queue");c.queue=&q;c.display_width=320;c.near_z=.125f;c.page_image=51;
    CHECK(bh_hud_scores_draw(&c,&s) && c.attempted==7 && c.queued==7 && !c.dropped && q.count==16,"descending distinct ranks and all ties without duplicate local slot");
    for(i=0;i<7;++i){
        BHDrawCommand *p=q.commands+9+i;
        CHECK(p->kind==BH_DRAW_RECT && p->resource==51 && word(p,0)==160 && word(p,1)==4+i*8 && word(p,2)==64 && word(p,3)==8 &&
            word(p,4)==0 && word(p,5)==(i==4?1.0f:.5f) && word(p,6)==0 && word(p,7)==1 && word(p,8)==expected[i]*.125f &&
            word(p,9)==.125f && close(word(p,10),.135) && !p->words[11],"independent rank/slot UVs, tie order, highlight, dimensions and live near depth");
        CHECK(p->next==(i==6?5:10+i),"scoreboard command chain returns to next actual global bucket header");
    }
    CHECK(q.commands[4].next==9 && q.commands[4].resource==15,"actual bucket3 head and tail owner");
    bh_draw_queue_begin(&q);c.display_width=-321;c.near_z=1.125f;
    CHECK(bh_hud_scores_draw(&c,&s) && word(q.commands+9,0)==-160 && word(q.commands+9,1)==-4 &&
        word(q.commands+9,2)==-64 && word(q.commands+9,3)==-8 && close(word(q.commands+9,10),1.135),"signed division truncates toward0 and non-default live near plane flows to depth");
    memset(slots,0,sizeof(slots));c.display_width=320;
    for(row=0;row<4;++row){
        static const long boundary[4]={100000,-100000,2147483647L,(-2147483647L-1)};
        s.local_score=boundary[row];bh_draw_queue_begin(&q);
        CHECK(bh_hud_scores_draw(&c,&s) && !c.attempted && q.count==9,"original strict initial score bounds exclude local boundary/outside values");
    }
    s.local_score=99999;bh_draw_queue_begin(&q);
    CHECK(bh_hud_scores_draw(&c,&s) && c.attempted==1 && word(q.commands+9,8)==.625f && word(q.commands+9,5)==1,"local-only roster draws highlighted source slot row");
    slots[0]=actors;s.local_index=-1;s.local_score=101;bh_draw_queue_begin(&q);
    CHECK(bh_hud_scores_draw(&c,&s) && !c.attempted,"negative local selected index triggers original signed stop guard before any lower ranks");
    s.local_index=5;s.local_score=30;slots[1]=actors+1;slots[2]=actors+2;
    q.capacity=9;bh_draw_queue_begin(&q);
    CHECK(bh_hud_scores_draw(&c,&s) && c.attempted==4 && !c.queued && c.dropped==4 && q.count==9,"full draw queue drops each ranked row and completes every tie group");
    q.capacity=32;bh_draw_queue_begin(&q);q.commands[4].resource=q.capacity;
    CHECK(!bh_hud_scores_draw(&c,&s) && c.error==BH_HUD_SCORES_QUEUE && c.attempted==4 && q.count==9,"queue topology errors recorded while remaining rank selections still execute");
    CHECK(bh_draw_queue_dispose(&q),"actual scoreboard queue lifetime");
}
int bh_hud_scores_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    texts();ranks();CHECK(bh_math_environment_supported(),"scoreboard retains PC53 source profile");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"hud-scores\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
