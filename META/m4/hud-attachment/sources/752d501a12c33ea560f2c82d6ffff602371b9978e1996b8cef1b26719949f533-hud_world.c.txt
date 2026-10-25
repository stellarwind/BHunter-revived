#include "../src/bh_hud_world.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL hud-world: %s\n",s);}}while(0)
static float value(const unsigned long *p){float x;memcpy(&x,p,4);return x;}
static int close_to(float a,double b){return fabs((double)a-b)<.000003;}
typedef struct Fixture {
    BHHudWorldContext world;BHHudWorldState state;BHHudContext hud;
    BHAttachmentContext attachment;BHRadarFrame frame;BHDrawQueue queue;
    BHHudWeaponAmmo ammo[2];const BHHudWeaponAmmo *weapons[256];
} Fixture;
static int fixture(Fixture *f,unsigned long capacity) {
    unsigned long i;memset(f,0,sizeof(*f));if(!bh_draw_queue_init(&f->queue,capacity,NULL))return 0;
    f->hud.frame=&f->frame;f->hud.attachment=&f->attachment;f->attachment.frame=&f->frame;
    f->attachment.queue=&f->queue;f->attachment.viewport_width=640;
    f->hud.world_center.x=10;f->hud.world_center.y=20;f->hud.world_center.z=30;
    f->frame.axis_x.x=f->frame.axis_y.y=f->frame.normal.z=1;f->frame.near_z=.125f;
    f->world.hud=&f->hud;f->world.atlas_image=21;f->world.weapon_icons_image=22;
    f->state.weapons=f->weapons;f->state.weapon_count=256;f->state.primary=2;f->state.secondary=8;
    f->state.gauge_a=f->state.gauge_b=-1;
    for(i=0;i<256;++i)f->weapons[i]=f->ammo;
    f->weapons[8]=f->ammo+1;return 1;
}
static BHDrawCommand *command(Fixture *f,unsigned long n){return f->queue.commands+9+n;}
static void payloads(void) {
    Fixture f;BHHudWorldState before;BHDrawCommand *p;unsigned long i;
    static const long digits[4]={1,0,0,2};
    CHECK(sizeof(BHHudWeaponAmmo)==0x110 && offsetof(BHHudWeaponAmmo,amount)==0x10c,"verified selected weapon ammo prefix");
    CHECK(fixture(&f,64),"real world HUD queue");f.state.gauge_a=.25f;f.state.gauge_b=.9f;
    f.ammo[0].amount=305;f.ammo[1].amount=42;f.state.counter_a=1002;f.state.counter_b=0;
    f.state.countdown_a=2;f.state.countdown_b=1;f.state.pending=1;before=f.state;
    CHECK(bh_hud_world_draw(&f.world,&f.state) && f.world.attempted==17 && f.world.queued==17 && !f.world.dropped,
        "complete owner command count for gauges, ammo/icons and two decimal counters");
    CHECK(f.queue.count==26 && f.queue.commands[4].next==9 && f.queue.commands[4].resource==25,"all draws use actual bucket3 queue owner");
    for(i=0;i<17;++i){p=command(&f,i);CHECK(p->kind==BH_DRAW_QUAD && p->next==(i==16?5:10+i) &&
        !p->words[13] && value(p->words+11)==0 && value(p->words+2)==30 && value(p->words+4)==1 && value(p->words+8)==1,
        "whole quad command order, world plane, flags and angle");}
    p=command(&f,0);CHECK(p->resource==21 && close_to(value(p->words),9.817) && close_to(value(p->words+1),20.094) &&
        value(p->words+9)==.026f && value(p->words+10)==.13f && value(p->words+12)==1 && value(p->words+14)==.62890625f &&
        value(p->words+15)==.1171875f && value(p->words+16)==.00390625f && value(p->words+17)==.6953125f,"independent gauge background geometry and atlas payload");
    p=command(&f,1);CHECK(close_to(value(p->words),9.817) && close_to(value(p->words+1),20.07775) &&
        close_to(value(p->words+10),.0975) && value(p->words+14)==.87890625f && value(p->words+16)==.177734375f &&
        value(p->words+17)==.521484375f,"gauge fill placement, retained remainder UV and stored height");
    p=command(&f,2);CHECK(close_to(value(p->words),10.081) && value(p->words+14)==.75390625f,"second gauge uses its actual separate horizontal offset");
    CHECK(value(command(&f,3)->words+14)==.19140625f && value(command(&f,4)->words+14)==.00390625f &&
        value(command(&f,5)->words+14)==.31640625f,"primary305 preserves embedded zero digit");
    p=command(&f,6);CHECK(p->resource==22 && value(p->words+14)==.00390625f && value(p->words+15)==.4921875f &&
        value(p->words+16)==.39453125f && value(p->words+17)==.1875f && value(p->words+12)==.8f &&
        close_to(value(p->words),10.2281818181818) && close_to(value(p->words+1),20.1075),"primary selected weapon icon sheet and position");
    CHECK(value(command(&f,7)->words+14)==.25390625f && value(command(&f,8)->words+14)==.12890625f &&
        value(command(&f,7)->words+16)==.09765625f,"secondary42 suppresses only leading hundreds");
    p=command(&f,9);CHECK(p->resource==22 && value(p->words+14)==.50390625f && value(p->words+16)==.58984375f &&
        close_to(value(p->words),10.1818181818) && value(p->words+1)==20.0625f,"secondary weapon index subtracts five and uses separate icon half");
    for(i=0;i<4;++i){p=command(&f,11+i);CHECK(value(p->words+14)==.00390625f+digits[i]*.0625f &&
        value(p->words+16)==.19140625f && value(p->words+9)==.01066666655242443f,"seven-position first counter emits1002 with interior zeros");}
    CHECK(value(command(&f,10)->words+16)==.28515625f && value(command(&f,15)->words+16)==.39453125f &&
        value(command(&f,16)->words+16)==.91015625f && value(command(&f,16)->words+14)==.00390625f,
        "two panel atlases and mandatory final zero of an all-zero counter");
    CHECK(f.state.countdown_a==1 && f.state.countdown_b==16,"post-draw countdown decrement and pending reload");
    before.countdown_a=1;before.countdown_b=16;CHECK(!memcmp(&before,&f.state,sizeof(before)),"all other player status fields preserved");
    bh_draw_queue_dispose(&f.queue);
}
static void gauge_gates(void) {
    Fixture f;unsigned long i;static const float amount[]={-1,0,.79999995f,.8f,1,1.25f,1.25f};
    static const unsigned char flag[]={0,0,0,0,0,0,8};static const unsigned long count[]={10,11,11,10,10,10,11};
    CHECK(fixture(&f,64),"gauge gate queue");
    for(i=0;i<7;++i){bh_draw_queue_begin(&f.queue);f.state.gauge_a=amount[i];f.state.flags=flag[i];
        CHECK(bh_hud_world_draw(&f.world,&f.state) && f.world.queued==count[i],"exact double.8 upper gate and nonnegative requirement");
        if(i==6)CHECK(close_to(value(command(&f,1)->words+10),-.0325) && value(command(&f,1)->words+17)==-.173828125f,
            "flag8 retains negative out-of-range fill size and UV extent");}
    f.state.gauge_a=-1;f.state.gauge_b=-1;f.state.flags=8;bh_draw_queue_begin(&f.queue);
    CHECK(bh_hud_world_draw(&f.world,&f.state) && f.world.queued==10,"override flag cannot bypass negative lower gate");
    {unsigned long amount_word=0x3e3d7a5eUL;memcpy(&f.state.gauge_a,&amount_word,4);}
    f.state.flags=0;memset(&f.hud.world_center,0,sizeof(f.hud.world_center));bh_draw_queue_begin(&f.queue);
    CHECK(bh_hud_world_draw(&f.world,&f.state) && command(&f,1)->words[1]==0x3da7e13eUL &&
        command(&f,1)->words[10]==0x3dd8f9c8UL && command(&f,1)->words[16]==0x3e07bf15UL &&
        command(&f,1)->words[17]==0x3f11103bUL,"raw FST contract: stored remainder controls placement/height while retained remainder controls UV extent");
    bh_draw_queue_dispose(&f.queue);
}
static void countdowns(void) {
    Fixture f;unsigned long raw,pending,expected;BHHudWorldState before;
    CHECK(fixture(&f,9),"full queue preserves owner side effects");
    for(pending=0;pending<2;++pending)for(raw=0;raw<256;++raw){
        f.state.countdown_a=f.state.countdown_b=(unsigned char)raw;f.state.pending=(long)pending;before=f.state;
        expected=raw?raw-1:0;if(!expected && pending)expected=16;
        CHECK(bh_hud_world_draw(&f.world,&f.state) && f.state.countdown_a==(raw?raw-1:0) &&
            f.state.countdown_b==expected,"all256 unsigned countdown values with and without pending reload");
        CHECK(f.world.attempted==8+((raw&8)?0:2) && f.world.dropped==f.world.attempted && !f.world.queued && f.queue.count==9,
            "bit8 snapshot is consumed before decrement and full queue never stops later work");
        before.countdown_a=f.state.countdown_a;before.countdown_b=f.state.countdown_b;
        CHECK(!memcmp(&before,&f.state,sizeof(before)),"countdown-only mutation with full queue");
    }
    bh_draw_queue_dispose(&f.queue);CHECK(bh_draw_queue_init(&f.queue,12,NULL),"real invalid-topology queue storage");
    f.queue.commands[4].resource=100;f.state.countdown_a=8;f.state.countdown_b=1;f.state.pending=1;
    CHECK(!bh_hud_world_draw(&f.world,&f.state) && f.world.error==BH_HUD_WORLD_QUEUE && f.world.attempted==9 &&
        f.state.countdown_a==7 && f.state.countdown_b==16,"invalid source queue continues all draw attempts and countdown side effects");
    bh_draw_queue_dispose(&f.queue);
}
static void integers(void) {
    Fixture f;unsigned long i,j,n;float expected_u;
    static const struct {long amount;unsigned long n;long digits[3];} ammo[]={
        {-2147483647L-1,3,{-21474836,-4,-8}},{-101,3,{-1,0,-1}},{-100,3,{-1,0,0}},
        {-99,2,{-9,-9,0}},{-10,2,{-1,0,0}},{-1,1,{-1,0,0}},{0,1,{0,0,0}},
        {9,1,{9,0,0}},{10,2,{1,0,0}},{99,2,{9,9,0}},{100,3,{1,0,0}},
        {101,3,{1,0,1}},{999,3,{9,9,9}},{1000,3,{10,0,0}},{2147483647L,3,{21474836,4,7}}};
    static const struct {long amount;unsigned long n;long digits[7];} counters[]={
        {0,1,{0}},{1,1,{1}},{10,2,{1,0}},{1000000,7,{1,0,0,0,0,0,0}},
        {1000000000,7,{1000,0,0,0,0,0,0}},{2147483647L,7,{2147,4,8,3,6,4,7}},
        {-2147483647L-1,7,{-2147,-4,-8,-3,-6,-4,-8}}};
    CHECK(fixture(&f,64),"signed numeric HUD queue");f.state.countdown_a=f.state.countdown_b=8;
    for(i=0;i<sizeof(ammo)/sizeof(ammo[0]);++i){bh_draw_queue_begin(&f.queue);f.ammo[0].amount=ammo[i].amount;
        f.state.countdown_a=f.state.countdown_b=8;
        CHECK(bh_hud_world_draw(&f.world,&f.state) && f.world.queued==7+ammo[i].n,"signed ammo truncation, leading zero suppression and unclamped hundreds");
        for(j=0;j<ammo[i].n;++j){bh_round_float(&expected_u,.00390625+(double)ammo[i].digits[j]*.0625);
            CHECK(value(command(&f,2+j)->words+14)==expected_u,"independent signed ammo atlas digit sequence");}}
    f.ammo[0].amount=0;f.state.countdown_b=8;
    for(i=0;i<sizeof(counters)/sizeof(counters[0]);++i){bh_draw_queue_begin(&f.queue);f.state.counter_a=counters[i].amount;
        f.state.countdown_a=0;f.state.countdown_b=8;
        CHECK(bh_hud_world_draw(&f.world,&f.state) && f.world.queued==8+counters[i].n,"signed seven-position counter is not an unrestricted decimal formatter");
        for(j=0;j<counters[i].n;++j){bh_round_float(&expected_u,.00390625+(double)counters[i].digits[j]*.0625);
            CHECK(value(command(&f,7+j)->words+14)==expected_u,"independent counter digits including large first digit and signed minima");}}
    bh_draw_queue_begin(&f.queue);f.state.primary=f.state.secondary=255;f.state.countdown_a=f.state.countdown_b=8;
    CHECK(bh_hud_world_draw(&f.world,&f.state) && value(command(&f,3)->words+16)==49.80859375f &&
        value(command(&f,5)->words+16)==48.83203125f,"selected bytes zero extend to real supplied table bounds; icon UVs remain unclamped");
    bh_draw_queue_dispose(&f.queue);
}
static void guards(void) {
    Fixture f;BHHudWorldState before;unsigned long nan=0x7fc00000UL;
    CHECK(fixture(&f,64),"guard queue");f.state.countdown_a=f.state.countdown_b=1;before=f.state;
    CHECK(!bh_hud_world_draw(NULL,&f.state) && !bh_hud_world_draw(&f.world,NULL),"absent source arguments");
    f.state.weapon_count=2;before.weapon_count=2;
    CHECK(!bh_hud_world_draw(&f.world,&f.state) && f.world.error==BH_HUD_WORLD_ARGUMENT && !memcmp(&before,&f.state,sizeof(before)) &&
        f.queue.count==9,"out-of-bounds selected weapon rejected before draws or mutations");
    f.state.weapon_count=256;f.weapons[2]=NULL;
    CHECK(!bh_hud_world_draw(&f.world,&f.state) && f.queue.count==9 && f.state.countdown_a==1,"missing selected weapon lifetime binding rejected");
    f.weapons[2]=f.ammo;memcpy(&f.state.gauge_a,&nan,4);before=f.state;
    CHECK(!bh_hud_world_draw(&f.world,&f.state) && f.world.error==BH_HUD_WORLD_NUMERICAL && f.queue.count==9 &&
        !memcmp(&before,&f.state,sizeof(before)),"source nonfinite guard preserves countdowns and queue");
    bh_draw_queue_dispose(&f.queue);
}
int bh_hud_world_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    payloads();gauge_gates();countdowns();integers();guards();CHECK(bh_math_environment_supported(),"PC53 retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"hud-world\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_HUD_WORLD_STANDALONE
int main(void){unsigned long count;return bh_hud_world_checkpoint(&count);}
#endif
