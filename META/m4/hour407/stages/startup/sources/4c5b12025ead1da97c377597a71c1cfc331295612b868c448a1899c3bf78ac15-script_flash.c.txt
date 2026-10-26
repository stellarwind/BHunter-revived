#include "../src/bh_script_flash.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL script-flash %s\n",s);}}while(0)
int __cdecl bh_script_flash_checkpoint(unsigned long*count){BHMathEnvironment env;BHEntityPool p;BHEntity*e;float times[7]={1,.03125f,0,-1,.0333333350718021392822265625f,0,0};unsigned long i,bits=0x7fc01234UL,used;volatile float expected;BHVec3 position;
 checks=failures=0;if(!bh_math_environment_enter(&env))return 2;memcpy(times+5,&bits,4);bits=0x7f800000UL;memcpy(times+6,&bits,4);
 CHECK(bh_entities_init(&p,2,NULL),"actual pool initialization");
 for(i=0;i<7;++i){e=bh_entity_acquire(&p);e->position.x=1;e->position.y=2;e->position.z=3;position=e->position;used=p.used;
  CHECK(bh_script_flash_bind(&p,e,times[i])&&e->step==bh_script_flash_tick,"actual complete step and persistent source pool binding");e->step(e);
  if(i==0||i==4||i==6){expected=(float)((double)times[i]-1.0/30.0);CHECK(p.used==used&&bh_script_flash_countdown(e)==expected,"positive or infinite pre-store result retains actual entity and rounded countdown");CHECK(!memcmp(&position,&e->position,12),"tick leaves position untouched");CHECK(bh_entity_release(&p,e),"explicit real release of retained entity");}
  else CHECK(p.used==used-1,"nonpositive or unordered result actually releases pool ownership");
 }
 e=bh_entity_acquire(&p);CHECK(bh_script_flash_bind(&p,e,.1f),"bind actual VM-style tenth-second flash");used=p.used;
 e->step(e);e->step(e);e->step(e);CHECK(p.used==used&&bh_script_flash_countdown(e)>0,"binary32 tenth remains narrowly positive after3 original decrements");e->step(e);CHECK(p.used==used-1,"fourth fixed-step decrement releases tenth-second flash");
 CHECK(bh_entities_dispose(&p),"actual pool lifetime disposal");bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"script-flash\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_FLASH_STANDALONE
int main(void){return bh_script_flash_checkpoint(NULL);}
#endif
