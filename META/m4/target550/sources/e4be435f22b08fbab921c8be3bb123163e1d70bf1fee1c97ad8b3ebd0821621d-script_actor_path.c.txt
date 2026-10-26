#include "../src/bh_script_actor_path.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure line %d\n",__LINE__);}}while(0)
static int close3(BHVec3 a,BHVec3 b){return fabs(a.x-b.x)<.00001&&fabs(a.y-b.y)<.00001&&fabs(a.z-b.z)<.00001;}
int bh_script_actor_path_checkpoint(unsigned long *count){
 BHVec3 p[4]={{2,4,-6},{10,-8,2},{-4,16,8},{20,-2,-10}},q,r,a,b;
 BHMathEnvironment env;unsigned long i,j;float t;double angle,u,v,l,rr;BHVec3 expected;
 checks=failures=0;CHECK(bh_math_environment_enter(&env));
 bh_script_actor_curve(0,&q,p,p+1,p+2,p+3);CHECK(close3(q,p[0]));
 bh_script_actor_curve(1,&q,p,p+1,p+2,p+3);CHECK(close3(q,p[3]));
 bh_script_actor_curve(-.5f,&q,p,p+1,p+2,p+3);expected.x=-2;expected.y=10;expected.z=-10;CHECK(close3(q,expected));
 bh_script_actor_curve(1.5f,&q,p,p+1,p+2,p+3);expected.x=32;expected.y=-11;expected.z=-19;CHECK(close3(q,expected));
 for(i=1;i<1000;++i){
  t=(float)(i/1000.0);bh_script_actor_curve(t,&q,p,p+1,p+2,p+3);
  angle=(float)(t*1.5707963267948966);u=(1-sin(angle))/cos(angle);v=(1-cos(angle))/sin(angle);
  for(j=0;j<3;++j){l=((float*)p)[j]*u+((float*)(p+1))[j]*(1-u);rr=((float*)(p+3))[j]*v+((float*)(p+2))[j]*(1-v);
   CHECK(fabs(((float*)&q)[j]-((1-t)*l+t*rr))<.00001);}
  a=p[0];b=p[1];bh_script_actor_linear(t,&r,&a,&b,0,0);bh_script_actor_linear(t,&a,&a,&b,0,0);CHECK(close3(a,r));
  a=p[0];b=p[1];bh_script_actor_linear(t,&b,&a,&b,0,0);CHECK(close3(b,r));
  a=p[0];bh_script_actor_curve(t,&a,&a,p+1,p+2,p+3);CHECK(close3(a,q));
 }
 bh_script_actor_linear(0,&q,p,p+1,0,0);CHECK(close3(q,p[0]));
 bh_script_actor_linear(1,&q,p,p+1,0,0);CHECK(close3(q,p[1]));
 bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"script-actor-path\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCRIPT_ACTOR_PATH_STANDALONE
int main(void){return bh_script_actor_path_checkpoint(0);}
#endif
