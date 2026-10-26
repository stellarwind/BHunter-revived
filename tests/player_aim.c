#include "../src/bh_player_aim.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;if(failures<10)printf("FAIL player aim line%d\n",__LINE__);}} while(0)
static void put(BHEntity *e,unsigned long at,float v) {memcpy((unsigned char *)e+at,&v,4);}
static BHVec3 read(BHEntity *e,unsigned long at) {BHVec3 v;memcpy(&v,(unsigned char *)e+at,12);return v;}
static int close_value(double a,double b) {return fabs(a-b)<1e-5;}
/* Independent algebraic equivalent of the shaping function, avoiding its
   removable singularity solely in the test's non-singular oracle domain. */
static double shape(double x) {
 double t=fabs(x*3),sum=1,power=1;int k;for(k=1;k<=6;++k){power*=t;sum+=power;}
 return (x<0?-1:1)*(t-power*t/sum);
}
int __cdecl bh_player_aim_checkpoint(unsigned long *count) {
 BHEntityPool pool;BHEntity *e;BHPhysicsStepBody body;BHPhysicsState state;BHMathEnvironment env;
 BHVec3 local,world;float x,y;double a,b;int i,j;
 checks=0;failures=0;CHECK(bh_math_environment_enter(&env));CHECK(bh_entities_init(&pool,1,NULL));e=bh_entity_acquire(&pool);CHECK(e!=NULL);
 memset(&body,0,sizeof(body));memset(&state,0,sizeof(state));body.entity=e;body.physics=&state;
 /* A non-identity orthogonal pose confirms each local coefficient goes to its
    real shared physics basis, including the source's stagedfloat sums. */
 state.pose.m[0][1]=-1;state.pose.m[1][0]=1;state.pose.m[2][2]=1;
 put(e,0x1f0,0);put(e,0x1f4,-0.0f);CHECK(bh_player_aim_update(&body));local=read(e,0x28c);world=read(e,0x280);
 CHECK(local.x==0 && local.y==0 && local.z==1 && world.x==0 && world.y==0 && world.z==1);
 for(i=-9;i<=9;++i)for(j=-9;j<=9;++j) {
  x=(float)i*.125f;y=(float)j*.125f;put(e,0x1f0,x);put(e,0x1f4,y);CHECK(bh_player_aim_update(&body));
  local=read(e,0x28c);world=read(e,0x280);a=shape(x)*.2617993877991494;b=shape(y)*.2617993877991494;
  CHECK(close_value(local.x,sin(a)) && close_value(local.y,-sin(b)*cos(a)) && close_value(local.z,cos(a)*cos(b)));
  CHECK(close_value(world.x,-local.y) && close_value(world.y,local.x) && close_value(world.z,local.z));
 }
 /* Original x87 shape has0/0 at rounded3*input=+/-1. Preserve maskedNaN. */
 put(e,0x1f0,1.0f/3);put(e,0x1f4,0);CHECK(bh_player_aim_update(&body));local=read(e,0x28c);world=read(e,0x280);CHECK(_isnan(world.z) && local.x==0 && local.z==1);
 put(e,0x1f0,-1.0f/3);CHECK(bh_player_aim_update(&body));local=read(e,0x28c);world=read(e,0x280);CHECK(_isnan(world.z) && local.x==0 && local.z==1);
 CHECK(bh_entities_dispose(&pool));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"player-aim\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_PLAYER_AIM_STANDALONE
int main(void) {return bh_player_aim_checkpoint(NULL);}
#endif
