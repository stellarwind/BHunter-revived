#include "../src/bh_draw_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL draw queue %d: %s\n",__LINE__,label);}} while(0)
static unsigned long bits(float value) {unsigned long result;memcpy(&result,&value,4);return result;}
static void cases(void) {
    BHDrawQueue q,saved;BHDrawCommand snapshot[18];
    BHVec3 position={1,2,3},end={4,5,6},normal={7,8,9};
    const unsigned long order[18]={0,14,1,9,2,10,3,11,4,12,5,13,6,15,7,16,8,17};
    unsigned long i,index;float special;unsigned long special_word=0x7fc12345UL;
    memset(&q,0x36,sizeof(q));saved=q;
    CHECK(!bh_draw_queue_init(&q,8,NULL) && !memcmp(&q,&saved,sizeof(q)),"minimum nine headers and transactional failure");
    CHECK(bh_draw_queue_init(&q,18,NULL) && q.count==9,"real owned command arena");
    for(i=0;i<9;++i) {
        CHECK(q.commands[i].resource==i && q.commands[i].next==(i==8 ? BH_DRAW_END:i+1),"global header chain and empty tail index");
        CHECK(q.commands[i].kind==((i==0 || i==6 || i==8) ? BH_DRAW_NOP:BH_DRAW_STATE),"verified control selector map");
    }
    for(i=0;i<9;++i)
        CHECK(bh_draw_billboard(&q,i,100+i,&position,4,5,6,7,8)==1,"all nine source bucket mappings");
    index=0;
    for(i=0;i<18;++i) {CHECK(index==order[i],"independent original FIFO/bucket chain order");index=q.commands[index].next;}
    CHECK(index==BH_DRAW_END,"last header/command terminates global chain");
    memcpy(snapshot,q.commands,sizeof(snapshot));
    CHECK(bh_draw_segment(&q,3,5,&position,&end,1,1,1,1,0)==0 && q.count==18 &&
        !memcmp(snapshot,q.commands,sizeof(snapshot)),"full arena drops without changing queued commands");
    CHECK(bh_draw_queue_begin(&q) && q.count==9,"reset reusable borrowed frame state");
    q.commands[9].words[17]=0xa5b6c7d8UL;memcpy(&special,&special_word,4);
    CHECK(bh_draw_billboard(&q,99,123,&position,4,5,special,7,8)==1 &&
        q.commands[0].next==9 && q.commands[9].next==1 &&
        q.commands[9].words[0]==bits(1) && q.commands[9].words[3]==bits(4) &&
        q.commands[9].words[4]==bits(5) && q.commands[9].words[5]==special_word &&
        q.commands[9].words[6]==bits(8) && q.commands[9].words[7]==7 &&
        q.commands[9].words[17]==0xa5b6c7d8UL,"billboard field ordering, raw NaN bits, default bucket and retained unused bytes");
    CHECK(bh_draw_billboard(&q,0xffffffffUL,124,&position,1,2,3,4,5)==1 &&
        q.commands[9].next==10 && q.commands[10].next==1,"all unmapped bucket codes append FIFO in bucket zero");
    CHECK(bh_draw_segment(&q,3,222,&position,&end,10,11,12,13,14)==1 &&
        q.commands[11].kind==BH_DRAW_SEGMENT && q.commands[11].resource==222 &&
        q.commands[11].words[0]==bits(1) && q.commands[11].words[5]==bits(6) &&
        q.commands[11].words[6]==bits(10) && q.commands[11].words[9]==bits(13) &&
        q.commands[11].words[10]==14,"complete segment payload");
    CHECK(bh_draw_oriented(&q,3,333,&position,&end,&normal,10,11,12,13,14)==1 &&
        q.commands[12].kind==BH_DRAW_ORIENTED && q.commands[12].resource==333 &&
        q.commands[12].words[3]==bits(4) && q.commands[12].words[8]==bits(9) &&
        q.commands[12].words[9]==bits(10) && q.commands[12].words[11]==bits(12) &&
        q.commands[12].words[12]==bits(14) && q.commands[12].words[13]==13,
        "complete oriented payload including original alpha/control reversal");
    index=q.count;
    CHECK(bh_draw_segment(&q,3,2,NULL,&end,1,1,1,1,0)==-1 && q.count==index,"invalid borrowed point rejected before arena mutation");
    q.commands[4].resource=99;
    CHECK(bh_draw_billboard(&q,3,2,&position,1,1,0,0,1)==-1 && q.count==index && q.error==BH_DRAW_TOPOLOGY,
        "foreign tail rejects bounded queue mutation");
    CHECK(bh_draw_queue_begin(&q) && bh_draw_queue_dispose(&q) && !q.commands,"real arena cleanup");
}
int bh_draw_queue_checkpoint(unsigned long *count) {
    checks=0;failed=0;cases();*count=checks;
    printf("{\"scenario\":\"draw-command-queue\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
