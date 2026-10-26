#include "../src/bh_impact_ring.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
static float word(unsigned long value){float out;memcpy(&out,&value,4);return out;}
int bh_impact_ring_checkpoint(unsigned long *count){
    BHDrawQueue q;BHEntity entity,saved;BHImpactState *state;BHDrawCommand *d;
    unsigned long checks=0,failures=0,i;float yaw;unsigned int previous;
#define CHECK(x) do{++checks;if(!(x)){++failures;printf("FAIL impact ring %d\n",__LINE__);}}while(0)
    previous=_controlfp(_PC_53|_RC_NEAR,_MCW_PC|_MCW_RC);
    memset(&entity,0,sizeof(entity));state=(BHImpactState *)(entity.payload+4);
    entity.position.x=2;entity.position.y=3;entity.position.z=4;
    state->normal.x=0;state->normal.y=1;state->normal.z=0;
    state->duration=4;state->size=8;
    CHECK(bh_draw_queue_init(&q,10,NULL));
    for(i=0;i<=8;i++){
        CHECK(bh_draw_queue_begin(&q));state->age=(float)i*.5f;saved=entity;
        CHECK(bh_impact_ring_draw(&q,73,&entity,NULL)==1);
        d=q.commands+9;
        CHECK(d->kind==BH_DRAW_ORIENTED&&d->resource==73&&q.commands[4].next==9);
        CHECK(word(d->words[0])==2&&word(d->words[1])==3&&word(d->words[2])==4);
        CHECK(word(d->words[3])==0&&word(d->words[4])==0&&word(d->words[5])==1);
        CHECK(word(d->words[6])==0&&word(d->words[7])==1&&word(d->words[8])==0);
        CHECK(word(d->words[9])==(float)i*4&&word(d->words[10])==(float)i*4);
        CHECK(fabs(word(d->words[12])-(.7-.0875*i))<.0000001);
        CHECK(d->words[11]==0&&d->words[13]==0&&!memcmp(&entity,&saved,sizeof(entity)));
        CHECK(bh_impact_ring_draw(&q,73,&entity,NULL)==0&&q.count==10);
    }
    CHECK(bh_draw_queue_begin(&q));yaw=1.5707963705062866f;
    CHECK(bh_impact_ring_draw(&q,91,&entity,&yaw)==1);
    CHECK(word(q.commands[9].words[3])==1&&fabs(word(q.commands[9].words[5]))<.000001);
    CHECK(bh_draw_queue_begin(&q));state->duration=0;
    CHECK(bh_impact_ring_draw(&q,73,&entity,NULL)==-1&&q.count==9);
    CHECK(bh_impact_ring_draw(NULL,0,&entity,NULL)==-1);
    CHECK(bh_impact_ring_draw(&q,0,NULL,NULL)==-1);
    CHECK(bh_draw_queue_dispose(&q));
    _controlfp(previous,_MCW_PC|_MCW_RC);if(count)*count=checks;
    printf("{\"scenario\":\"impact-ring\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
#undef CHECK
}
