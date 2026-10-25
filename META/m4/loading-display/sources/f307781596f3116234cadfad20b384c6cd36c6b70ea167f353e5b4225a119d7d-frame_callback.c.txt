#include "../src/bh_frame_callback.h"
#include <stdio.h>
int bh_frame_callback_checkpoint(unsigned long *count){
    BHFrameCallback callback;unsigned long checks=0,failures=0;
    callback.render=NULL;
    ++checks;if(!bh_frame_callback_start(&callback)||callback.render!=bh_render_execute)++failures;
    bh_frame_callback_set(&callback,NULL);++checks;if(callback.render)++failures;
    bh_frame_callback_set(&callback,bh_render_execute);++checks;if(callback.render!=bh_render_execute)++failures;
    ++checks;if(bh_frame_callback_start(NULL))++failures;
    /* The complete worker checkpoint installs this wrapper and executes the
       registered actual consumer over native/software offscreen frame owners.
       No dummy callback is supplied by this focused registration fixture. */
    if(count)*count=checks;
    printf("{\"scenario\":\"frame-callback\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_FRAME_CALLBACK_STANDALONE
int main(void){return bh_frame_callback_checkpoint(NULL);}
#endif
