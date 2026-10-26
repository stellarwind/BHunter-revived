#include "../src/bh_delayed_expiry.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL delayed expiry %s\n",n);}}while(0)
static float field(BHEntity *e,int at){float v;memcpy(&v,(char*)e+at,4);return v;}
int bh_delayed_expiry_checkpoint(unsigned long *count){BHEntityPool p;BHEntity *e;BHMathEnvironment fp;unsigned long nanbits=0x7fc00000;float nan;memset(&p,0,sizeof(p));checks=0;failures=0;CHECK(bh_math_environment_enter(&fp),"PC53");CHECK(bh_entities_init(&p,2,NULL),"pool");e=bh_entity_acquire(&p);
 CHECK(bh_delayed_expiry_bind(&p,e,0,.01f),"bind two phases");CHECK(bh_entities_step(&p)&&p.active==e&&field(e,0xe8)==0&&field(e,0x198)<0,"positive delay consumes whole tick");CHECK(bh_entities_step(&p)&&!p.active,"next tick decrements zero lifetime and retires");
 e=bh_entity_acquire(&p);CHECK(bh_delayed_expiry_bind(&p,e,1,.1f),"bind positive lifetime");CHECK(bh_entities_step(&p)&&field(e,0xe8)==1&&field(e,0x198)<.1f,"delay leaves lifetime");
 memcpy(&nan,&nanbits,4);CHECK(bh_delayed_expiry_bind(&p,e,nan,nan),"bind unordered");CHECK(bh_entities_step(&p)&&!p.active,"unordered delay advances unordered lifetime retires");
 e=bh_entity_acquire(&p);CHECK(bh_delayed_expiry_bind(&p,e,.1f,-1),"negative delay");CHECK(bh_entities_step(&p)&&field(e,0xe8)>.06f&&field(e,0x198)==-1,"lifetime store and untouched delay");CHECK(bh_entity_release(&p,e)&&bh_entities_dispose(&p),"ownership");bh_math_environment_leave(&fp);*count+=checks;printf("{\"scenario\":\"delayed-expiry\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;}
#ifdef BH_DELAYED_EXPIRY_STANDALONE
int main(void){unsigned long n=0;return bh_delayed_expiry_checkpoint(&n);}
#endif
