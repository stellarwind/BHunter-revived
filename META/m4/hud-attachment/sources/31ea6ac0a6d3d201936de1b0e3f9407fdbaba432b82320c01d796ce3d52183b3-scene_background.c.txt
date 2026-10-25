#include "../src/bh_scene_background.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL scene-background: %s\n",s);}} while(0)
static int close_to(float a,double b) {return fabs((double)a-b)<.003;}
static float word(const unsigned long *p) {float out;memcpy(&out,p,4);return out;}
static void trajectory(BHSceneBackground *c,unsigned long model) {
    BHSceneBackgroundSlot *s=c->slots;memset(s,0,sizeof(*s));
    s->elapsed=4.966666666666667f;s->duration=10;s->model=model;
    s->start.x=0;s->start.z=100;s->end.x=100;s->end.z=200;
    s->height=-20;s->yaw=0;
}
static void paths(void) {
    BHSceneBackground c;BHDrawQueue q;BHVec3 eye={10,20,30};unsigned long i;
    CHECK(bh_draw_queue_init(&q,32,NULL) && bh_scene_background_init(&c,&q,11,12,21,22),"owner initialized");
    for(i=0;i<8;++i)CHECK(c.slots[i].elapsed==-1,"startup negative elapsed sentinel");
    trajectory(&c,11);srand(1);
    CHECK(bh_scene_background_step(&c,0,&eye)==1 && q.count==16,"first model and six flares queued");
    CHECK(rand()==23281,"first model consumes exactly twelve flame random calls");
    CHECK(q.commands[9].kind==BH_DRAW_MODEL_RESOURCE && q.commands[9].resource==11 &&
        close_to(word(q.commands[9].words),59) && close_to(word(q.commands[9].words+1),0) &&
        close_to(word(q.commands[9].words+2),177),"trajectory midpoint parabola and eye parallax");
    CHECK(close_to(word(q.commands[9].words+3),3.141592653589793) &&
        word(q.commands[9].words+6)==1,"model yaw and full modulation");
    CHECK(q.commands[10].kind==BH_DRAW_BILLBOARD && q.commands[10].resource==21 &&
        q.commands[11].resource==22 && q.commands[12].resource==21 && q.commands[15].resource==22,
        "paired flame images retain exact submission order");
    CHECK(close_to(word(q.commands[10].words+2),65) && close_to(word(q.commands[12].words),79) &&
        close_to(word(q.commands[14].words),39),"three engine offsets retain individual float stores");
    CHECK(close_to(word(q.commands[10].words+3),50.02001953125) &&
        close_to(word(q.commands[10].words+6),(.5+18467.0/131072.0)*.5),
        "positive sizes and alpha use verified negative random constants");
    bh_draw_queue_begin(&q);trajectory(&c,12);srand(1);
    CHECK(bh_scene_background_step(&c,0,&eye)==1 && q.count==12 && rand()==19169,
        "second model consumes four random calls and two flames");
    CHECK(close_to(word(q.commands[10].words+2),142) && close_to(word(q.commands[10].words+3),40.02001953125),
        "second model shorter engine offset and flame size");
    bh_draw_queue_begin(&q);trajectory(&c,11);c.slots[0].elapsed=0;srand(1);
    CHECK(bh_scene_background_step(&c,0,&eye)==1 && q.count==10 && rand()==41,
        "early trajectory has one model without flames or random draws");
    CHECK(bh_draw_queue_dispose(&q),"path queue disposed");
}
static void random_lifecycle(void) {
    BHSceneBackground c;BHDrawQueue q;BHVec3 eye={0,0,0};BHSceneBackgroundSlot *s;
    CHECK(bh_draw_queue_init(&q,32,NULL) && bh_scene_background_init(&c,&q,11,12,21,22),"lifecycle fixture");
    s=c.slots;srand(29);
    CHECK(bh_scene_background_step(&c,0,&eye)==1 && q.count==10 && rand()==12930,
        "fresh seed control consumes exactly seven setup draws");
    CHECK(s->model==11 && s->delay==0 && close_to(s->elapsed,52.90978240966797) &&
        close_to(s->duration,132.61810302734375) && close_to(s->yaw,-2.991607904434204) &&
        close_to(s->height,-436.3843688964844),"seed29 independent trajectory fixture");
    CHECK(close_to(s->start.x,64.9341049194336) && close_to(s->start.z,2499.156494140625) &&
        close_to(s->end.x,-676.4439086914062) && close_to(s->end.z,-2406.745361328125),
        "seed29 endpoint float stores");
    bh_draw_queue_begin(&q);trajectory(&c,11);s->delay=1;srand(1);
    CHECK(bh_scene_background_step(&c,0,&eye)==1 && q.count==9 && rand()==41 &&
        close_to(s->delay,.966666666666667) && close_to(s->elapsed,4.966666666666667),
        "waiting decrements timer without random or phase progression");
    bh_draw_queue_begin(&q);trajectory(&c,11);s->elapsed=10;srand(1);
    CHECK(bh_scene_background_step(&c,0,&eye)==1 && q.count==9 && rand()==18467 &&
        close_to(s->delay,.03333333333333333+41.0/512),"completed pass schedules delay with one shared random draw");
    s->delay=.01f;srand(1);
    CHECK(bh_scene_background_step(&c,0,&eye)==1 && q.count==10 && rand()==15724 &&
        s->model==12 && close_to(s->elapsed,1.0/30),"delay expiry generates new trajectory without initial phase/delay draws");
    CHECK(bh_draw_queue_dispose(&q),"lifecycle disposed");
}
static void limits(void) {
    BHSceneBackground c;BHDrawQueue q;BHVec3 eye={0,0,0};
    CHECK(bh_draw_queue_init(&q,9,NULL) && bh_scene_background_init(&c,&q,11,12,21,22),"full fixture");
    trajectory(&c,11);srand(1);
    CHECK(bh_scene_background_step(&c,0,&eye)==0 && q.count==9 && q.error==BH_DRAW_FULL && rand()==23281 &&
        close_to(c.slots[0].elapsed,5),"queue exhaustion still advances state and all twelve random calls");
    CHECK(bh_scene_background_step(&c,8,&eye)==-1 && bh_scene_background_step(NULL,0,&eye)==-1 &&
        !bh_scene_background_init(NULL,&q,1,2,3,4),"invalid source bindings rejected");
    CHECK(bh_draw_queue_dispose(&q),"full fixture disposed");
}
int bh_scene_background_tests(void) {
    checks=failures=0;paths();random_lifecycle();limits();
    printf("scene-background: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_scene_background_checkpoint(unsigned long *count) {
    bh_scene_background_tests();if(count)*count=checks;
    printf("{\"scenario\":\"scene-background\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCENE_BACKGROUND_STANDALONE
int main(void) {unsigned long count=0;return bh_scene_background_checkpoint(&count);}
#endif
