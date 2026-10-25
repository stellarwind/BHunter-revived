#include "../src/bh_force_feedback_start.h"
#include <stdio.h>
#include <string.h>
int __cdecl bh_force_feedback_start_checkpoint(unsigned long *count) {
 BHForceFeedback f;BHForceFeedbackStart c;unsigned long checks=0,failures=0;
#define CHECK(x) do{++checks;if(!(x))++failures;}while(0)
 memset(&f,0,sizeof(f));memset(&c,0,sizeof(c));CHECK(!bh_force_feedback_start(NULL));CHECK(!bh_force_feedback_start(&c));c.force=&f;
 CHECK(bh_force_feedback_start(&c)&&!c.started&&!c.acquires&&!c.starts);
 c.started=7;CHECK(bh_force_feedback_start(&c)&&c.started==7);
 f.enabled=1;CHECK(bh_force_feedback_start(&c)&&c.started==7&&!c.acquires&&!c.starts);
 c.started=0;CHECK(bh_force_feedback_start(&c)&&c.started==1&&!c.acquires&&!c.starts);
 CHECK(bh_force_feedback_start(&c)&&c.started==1&&!c.failures);
 f.enabled=0;c.started=0x80000000UL;CHECK(bh_force_feedback_start(&c)&&c.started==0x80000000UL);
 if(count)*count=checks;printf("{\"scenario\":\"force-feedback-start\",\"checks\":%lu,\"failures\":%lu,\"hardware_acquired\":false,\"effect_started\":false,\"original_code_required\":false}\n",checks,failures);return failures!=0;
#undef CHECK
}
