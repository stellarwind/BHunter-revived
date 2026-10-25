#include "../src/bh_horizon_queue.h"
#include "../src/bh_camera_queue.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL horizon-queue: %s\n",s);}} while(0)
static void ordering(void) {
    BHDrawQueue q;BHVec3 p={1,2,3},a={0,0,0};BHDrawCommand *c;unsigned long i;
    CHECK(sizeof(BHDrawCommand)==0x54 && offsetof(BHDrawCommand,next)==0x50,"original stride and next offset");
    CHECK(bh_draw_queue_init(&q,14,NULL),"real queue initialized");
    for(i=0;i<18;++i)q.commands[11].words[i]=0xdeadbeefUL;
    CHECK(bh_draw_billboard(&q,0,17,&p,1,1,0,0,1)==1,"numbered bucket has independent command");
    CHECK(bh_camera_queue_pose(&q,&p,&a)==1,"real camera command first");
    CHECK(bh_horizon_queue(&q,42,5000,5000,25000,12500,0x00804000UL)==1,"actual scene horizon parameters");
    c=q.commands+11;
    CHECK(q.commands[0].next==10 && q.commands[10].next==11 && c->next==1 && q.commands[0].resource==11,
        "horizon follows camera in header zero before bucket zero");
    CHECK(c->kind==BH_DRAW_HORIZON && c->resource==42 && c->words[0]==0x459c4000UL &&
        c->words[4]==0x459c4000UL && c->words[5]==0x46c35000UL && c->words[6]==0x46435000UL &&
        c->words[7]==0x00804000UL,"sparse payload offsets and real scene constants");
    CHECK(c->words[1]==0xdeadbeefUL && c->words[2]==0xdeadbeefUL && c->words[3]==0xdeadbeefUL &&
        c->words[8]==0xdeadbeefUL && c->words[17]==0xdeadbeefUL,"unused words remain untouched");
    CHECK(bh_camera_queue_pose(&q,&p,&a)==1 && q.commands[11].next==12 && q.commands[12].next==1,
        "camera may follow horizon without reordering");
    CHECK(q.commands[1].next==9 && q.commands[9].next==2,"numbered bucket chain preserved");
    CHECK(bh_draw_queue_dispose(&q),"ordered fixture disposed");
}
static void controls(void) {
    BHDrawQueue q;BHDrawCommand before[10];float f[4];unsigned long bits[4]={0x80000000UL,0x7f800000UL,0x7fc00123UL,0xff800000UL};
    CHECK(bh_draw_queue_init(&q,10,NULL),"control queue initialized");memcpy(f,bits,16);
    CHECK(bh_horizon_queue(&q,0xffffffffUL,f[0],f[1],f[2],f[3],0xff804000UL)==1 &&
        q.commands[9].resource==0xffffffffUL && q.commands[9].words[0]==bits[0] &&
        !memcmp(q.commands[9].words+4,bits+1,12) && q.commands[9].words[7]==0xff804000UL,
        "all nonfinite/sign/color/handle bits retained");
    memcpy(before,q.commands,sizeof(before));
    CHECK(bh_horizon_queue(&q,0,0,0,0,0,0)==0 && q.error==BH_DRAW_FULL && q.count==10 &&
        !memcmp(before,q.commands,sizeof(before)),"exhaustion drops without topology or payload mutation");
    CHECK(bh_draw_queue_begin(&q),"reset preserves allocation");
    CHECK(bh_horizon_queue(&q,1,1,1,1,1,1)==1 && q.commands[0].next==9 && q.commands[9].next==1,
        "reused storage starts fresh header-zero chain");
    CHECK(bh_draw_queue_begin(&q),"topology reset");q.commands[0].resource=100;
    CHECK(bh_horizon_queue(&q,0,0,0,0,0,0)==-1 && q.error==BH_DRAW_TOPOLOGY && q.count==9,
        "invalid tail rejected before allocation");
    q.commands[0].resource=0;q.commands[0].next=100;
    CHECK(bh_horizon_queue(&q,0,0,0,0,0,0)==-1 && q.error==BH_DRAW_TOPOLOGY && q.count==9,
        "invalid successor rejected before allocation");
    q.count=8;
    CHECK(bh_horizon_queue(&q,0,0,0,0,0,0)==-1 && q.error==BH_DRAW_ARGUMENT,"missing headers rejected");
    q.count=11;
    CHECK(bh_horizon_queue(&q,0,0,0,0,0,0)==-1 && q.error==BH_DRAW_ARGUMENT,"count above capacity rejected");
    CHECK(bh_horizon_queue(NULL,0,0,0,0,0,0)==-1,"null queue rejected");
    CHECK(bh_draw_queue_dispose(&q),"controls disposed");
}
int bh_horizon_queue_tests(void) {
    checks=failures=0;ordering();controls();
    printf("horizon-queue: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_horizon_queue_checkpoint(unsigned long *count) {
    bh_horizon_queue_tests();if(count)*count=checks;
    printf("{\"scenario\":\"horizon-queue\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_HORIZON_QUEUE_STANDALONE
int main(void) {unsigned long count=0;return bh_horizon_queue_checkpoint(&count);}
#endif
