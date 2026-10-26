#include "../src/bh_force_feedback_init.h"
#include "ff_format_cases.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL force-feedback-init %d: %s\n",__LINE__,s);}}while(0)
/* Isolated native discovery only, never a production effect/input callback. */
static BOOL CALLBACK count_devices(LPCDIDEVICEINSTANCEA instance,LPVOID context){unsigned long *count=(unsigned long*)context;(void)instance;++*count;return DIENUM_CONTINUE;}
static void format_cases(void){unsigned long i;
    CHECK(c_dfDIJoystick.dwSize==24&&c_dfDIJoystick.dwObjSize==16&&c_dfDIJoystick.dwFlags==1&&c_dfDIJoystick.dwDataSize==80&&c_dfDIJoystick.dwNumObjs==44,"reused official library data format header matches whole original");
    for(i=0;i<44;++i){const DIOBJECTDATAFORMAT *o=c_dfDIJoystick.rgodf+i;const FFFormatCase *e=ff_format_cases+i;
        CHECK(o->dwOfs==e->offset&&o->dwType==e->type&&o->dwFlags==e->flags,"each original object offset/type/flags matches reused library");
        CHECK((o->pguid!=NULL)==e->present&&(!e->present||!memcmp(o->pguid,e->guid,16)),"each original object GUID matches reused library");
    }
}
static void native_cases(void){LPDIRECTINPUTA root=NULL;unsigned long count=0,iteration;HRESULT hr;HINSTANCE instance=GetModuleHandleA(NULL);
    BHForceFeedback force;BHForceFeedbackStart start;BHForceFeedbackInit init,saved;
    hr=DirectInputCreateA(instance,0x500,&root,NULL);CHECK(SUCCEEDED(hr)&&root,"actual version500 native root for safe hardware preflight");if(!root)return;
    hr=IDirectInput_EnumDevices(root,DIDEVTYPE_JOYSTICK,count_devices,&count,DIEDFL_ATTACHEDONLY|DIEDFL_FORCEFEEDBACK);
    CHECK(SUCCEEDED(hr),"actual force-feedback-only native inventory");CHECK(IDirectInput_Release(root)==0,"preflight native root released");
    /* This fixture refuses physical acquisition/effect Start on a machine with
       feedback hardware. Such a host needs a separate human-authorized test. */
    CHECK(count==0,"host has no force-feedback device; do not automate physical I/O");if(count)return;
    for(iteration=0;iteration<2;++iteration){
        memset(&force,0,sizeof(force));memset(&start,0,sizeof(start));memset(&init,0,sizeof(init));start.force=&force;start.started=iteration?7:0;init.start=&start;
        CHECK(!bh_force_feedback_init(&init,instance,GetDesktopWindow()),"whole constructor actual no-device result");
        CHECK(init.creates==1&&SUCCEEDED(init.create_result)&&init.enumerations==1&&SUCCEEDED(init.enumeration_result),"real create and admitted native enum callback registration");
        CHECK(force.input&&!force.device&&!force.effect&&!force.enabled,"partial failure retains original owned input root only");
        CHECK(!init.data_formats&&!init.cooperative_levels&&!init.effects&&!start.acquires&&!start.starts&&start.started==(iteration?7UL:0UL),"no physical configuration/acquisition/Start and original started flag retained");
        saved=init;CHECK(!bh_force_feedback_init(&init,instance,GetDesktopWindow())&&!memcmp(&saved,&init,sizeof(init)),"source occupied owner guard");
        bh_force_feedback_stop(&force);CHECK(!force.input&&!force.device&&!force.effect&&force.input_releases==1&&!force.unacquires,"actual admitted owning stop releases retained root without input I/O");
    }
}
int bh_force_feedback_init_checkpoint(unsigned long *count){checks=0;failures=0;format_cases();native_cases();*count=checks;printf("{\"scenario\":\"force-feedback-init\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false,\"physical_acquires\":0,\"effect_starts\":0}\n",checks,failures);return failures;}
