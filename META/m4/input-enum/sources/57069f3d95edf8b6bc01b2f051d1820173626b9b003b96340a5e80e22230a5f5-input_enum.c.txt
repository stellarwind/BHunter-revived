#include "../src/bh_input_enum.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL input-enum %lu: %s\n",checks,s);}}while(0)
int __cdecl bh_input_enum_checkpoint(unsigned long *count) {
 BHForceFeedback owner;DIDEVICEINSTANCEA invalid;LPDIRECTINPUTA retained_root=NULL;
 LPDIRECTINPUTDEVICE2A retained_device=NULL,original_device;LPDIENUMDEVICESCALLBACKA registered=bh_input_enum_callback;
 HRESULT result;ULONG remaining;unsigned long releases;
 memset(&owner,0,sizeof(owner));memset(&invalid,0,sizeof(invalid));checks=failures=0;
 invalid.dwSize=sizeof(invalid);invalid.guidInstance.Data1=0xfe78b4d1;invalid.guidInstance.Data2=0x9637;
 invalid.guidInstance.Data3=0x4ea6;memset(invalid.guidInstance.Data4,0xd9,8);
 CHECK(bh_input_enum_callback(NULL,NULL)==DIENUM_CONTINUE,"NULL callback arguments source guard");
 CHECK(bh_input_enum_callback(&invalid,NULL)==DIENUM_CONTINUE,"missing explicit owner guard");
 CHECK(bh_input_enum_callback(&invalid,&owner)==DIENUM_CONTINUE&&!owner.device,"missing actual input root guard");
 result=DirectInputCreateA(GetModuleHandleA(NULL),0x0500,&owner.input,NULL);
 CHECK(SUCCEEDED(result)&&owner.input,"actual DirectInput5 root creation without input acquisition");
 if(FAILED(result)||!owner.input)goto done;
 retained_root=owner.input;IDirectInput_AddRef(retained_root);
 CHECK(bh_input_enum_callback(NULL,&owner)==DIENUM_CONTINUE&&!owner.device,"NULL instance guard retains live root");
 CHECK(bh_input_enum_callback(&invalid,&owner)==DIENUM_CONTINUE&&!owner.device&&owner.input==retained_root,"actual invalid-GUID CreateDevice failure continues without publishing device");
 CHECK(!owner.device_releases&&!owner.input_releases&&!owner.unacquires&&!owner.effect,"enumeration failure performs no owner teardown or hardware operation");
 result=IDirectInput_EnumDevices(owner.input,DIDEVTYPE_KEYBOARD,registered,&owner,DIEDFL_ATTACHEDONLY);
 CHECK(SUCCEEDED(result),"actual SDK stdcall callback registered with native keyboard metadata enumeration");
 CHECK(owner.device!=NULL,"callback creates base interface and queries exact Device2A success");
 if(owner.device){
  original_device=owner.device;retained_device=owner.device;IDirectInputDevice2_AddRef(retained_device);
  CHECK(bh_input_enum_callback(&invalid,&owner)==DIENUM_CONTINUE&&owner.device==original_device,"occupied-device guard preserves existing real COM ownership");
  CHECK(bh_input_enum_callback(NULL,&owner)==DIENUM_CONTINUE&&owner.device==original_device,"NULL-instance guard preserves occupied owner");
 }
 owner.enabled=1;bh_force_feedback_stop(&owner);
 CHECK(!owner.input&&!owner.device&&!owner.enabled&&!owner.effect&&owner.input_releases==1,"actual admitted stop owner releases and clears input references");
 if(retained_device){
  CHECK(owner.device_releases==1&&owner.unacquires==1&&SUCCEEDED(owner.last_unacquire),"never-acquired native device teardown through actual stop owner");
  remaining=IDirectInputDevice2_Release(retained_device);retained_device=NULL;
  CHECK(remaining==0,"positive retained COM reference proves temporary base reference and one queried ownership released");
 }
 remaining=IDirectInput_Release(retained_root);retained_root=NULL;
 CHECK(remaining==0,"retained root positive control proves one root ownership release");
 releases=owner.device_releases;bh_force_feedback_stop(&owner);
 CHECK(owner.device_releases==releases&&owner.input_releases==1,"repeat real stop performs no duplicate ownership release");
 CHECK(bh_input_enum_callback(&invalid,&owner)==DIENUM_CONTINUE&&!owner.device,"post-teardown callback guard");
 done:
 bh_force_feedback_stop(&owner);
 if(retained_device)IDirectInputDevice2_Release(retained_device);
 if(retained_root)IDirectInput_Release(retained_root);
 if(count)*count=checks;
 printf("{\"scenario\":\"input-enum\",\"checks\":%lu,\"failures\":%lu,\"devices_acquired\":0,\"input_polled\":false,\"effects_created\":0,\"original_code_required\":false}\n",checks,failures);
 return failures!=0;
}
#ifdef BH_INPUT_ENUM_STANDALONE
int main(void){return bh_input_enum_checkpoint(NULL);}
#endif
