#include "../src/bh_force_feedback_update.h"
#include <stdio.h>
#include <string.h>
int bh_force_feedback_update_checkpoint(unsigned long *count){
    BHForceFeedback force;BHForceFeedbackUpdate c,saved;BHForceFeedbackParameters p;
    unsigned long checks=0,failures=0,i;float x,y;
#define CHECK(x) do{++checks;if(!(x)){++failures;printf("FAIL force update %d\n",__LINE__);}}while(0)
    memset(&force,0,sizeof(force));memset(&c,0,sizeof(c));c.force=&force;
    CHECK(bh_force_feedback_update(&c,1,1,1)&&!c.calls&&!c.last_tick);
    CHECK(bh_force_feedback_prepare(&c,1,1,1,&p)==0);
    force.enabled=1;
    CHECK(!bh_force_feedback_update(&c,1,1,1)&&!c.calls&&!c.last_tick);
    CHECK(bh_force_feedback_prepare(&c,0,1,0,&p)==0&&!c.last_tick);
    CHECK(bh_force_feedback_prepare(&c,1,1,0,&p)==1&&c.last_tick==1&&c.x==1);
    CHECK(p.direction[0]==10000&&p.direction[1]==0&&p.magnitude.lMagnitude==10000);
    CHECK(p.effect.dwSize==52&&p.effect.dwFlags==0x12&&p.effect.cAxes==2);
    CHECK(p.effect.rglDirection==p.direction&&p.effect.lpvTypeSpecificParams==&p.magnitude&&p.effect.cbTypeSpecificParams==4);
    saved=c;CHECK(bh_force_feedback_prepare(&c,999,1,0,&p)==0&&!memcmp(&c,&saved,sizeof(c)));
    CHECK(bh_force_feedback_prepare(&c,2,2,-2,&p)==1&&c.x==1&&c.y==-1);
    CHECK(p.direction[0]==10000&&p.direction[1]==-10000&&p.magnitude.lMagnitude==14142);
    c.x=c.y=0;c.last_tick=0;
    CHECK(bh_force_feedback_prepare(&c,1,.25f,0,&p)==0);
    CHECK(bh_force_feedback_prepare(&c,2,.25f,0,&p)==1&&p.magnitude.lMagnitude==2500);
    c.x=c.y=0;c.last_tick=0xffffffffUL;
    CHECK(bh_force_feedback_prepare(&c,0,1,0,&p)==1);
    c.x=c.y=0;c.last_tick=0;
    CHECK(bh_force_feedback_prepare(&c,0x80000000UL,1,0,&p)==0);
    for(i=0;i<=16;i++){
        x=((float)i-8)*.125f;y=-x;c.x=c.y=0;c.last_tick=0;
        if(i==8){CHECK(bh_force_feedback_prepare(&c,1000,x,y,&p)==0);continue;}
        CHECK(bh_force_feedback_prepare(&c,1000,x,y,&p)==1);
        CHECK(p.direction[0]==((long)i-8)*1250&&p.direction[1]==-p.direction[0]);
    }
    CHECK(bh_force_feedback_prepare(NULL,1,1,1,&p)==-1);
    CHECK(bh_force_feedback_prepare(&c,1,1,1,NULL)==-1);
    CHECK(!c.calls&&!force.effect&&!force.device&&!force.input);
    if(count)*count=checks;
    printf("{\"scenario\":\"force-feedback-update\",\"checks\":%lu,\"failures\":%lu,\"native_set_parameters\":0,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
#undef CHECK
}
