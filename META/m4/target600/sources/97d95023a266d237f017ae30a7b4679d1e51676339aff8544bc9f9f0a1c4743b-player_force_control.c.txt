#include "../src/bh_player_force_control.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;if(failures<12)printf("FAIL force control line%d\n",__LINE__);}} while(0)
static void put(BHEntity *e,unsigned long at,float value) {memcpy((unsigned char *)e+at,&value,4);}
static float get(BHEntity *e,unsigned long at) {float v;memcpy(&v,(unsigned char *)e+at,4);return v;}
static int close_value(double a,double b) {return fabs(a-b)<3e-5*(1+fabs(b));}
typedef struct Fixture {
 BHEntityPool pool;BHActorBasisContext basis;BHActorBasisSample samples[15];BHActorBasisGroup a[7],b[7];
 BHActorBasisIndices ia[7],ib[7];BHActorBasisScale sa[7],sb[7];BHPhysicsState physics;
 BHPhysicsStepBody body;BHPlayerForceControl control;unsigned long counter;
} Fixture;
static void init(Fixture *f) {
 BHVec3 inertia={2,3,4};memset(f,0,sizeof(*f));CHECK(bh_entities_init(&f->pool,1,NULL));f->body.entity=bh_entity_acquire(&f->pool);CHECK(f->body.entity!=NULL);
 f->basis.samples=f->samples;f->basis.sample_capacity=15;f->basis.groups_a=f->a;f->basis.groups_b=f->b;
 f->basis.indices_a=f->ia;f->basis.indices_b=f->ib;f->basis.scales_a=f->sa;f->basis.scales_b=f->sb;f->basis.a_capacity=f->basis.b_capacity=7;CHECK(bh_actor_basis_acquire(&f->basis));
 f->physics.pose.m[0][0]=f->physics.pose.m[1][1]=f->physics.pose.m[2][2]=1;
 f->physics.pose.m[0][3]=10;f->physics.pose.m[1][3]=20;f->physics.pose.m[2][3]=30;
 f->body.physics=&f->physics;f->body.mass=2;f->control.body=&f->body;f->control.basis=&f->basis;f->control.instability_counter=&f->counter;
 memcpy((unsigned char *)f->body.entity+0x78,&inertia,12);
}
static void visual(void) {
 Fixture f;BHEntity *e;int i;BHPhysicsState saved;init(&f);e=f.body.entity;saved=f.physics;
 for(i=0;i<15;++i)put(e,0x2bc+i*4,(float)i);
 CHECK(bh_player_force_control_b(&f.control,0,1));CHECK(!memcmp(&f.physics,&saved,sizeof(saved)));
 for(i=0;i<15;++i)CHECK(close_value(get(e,0x2bc+i*4),i*.9));
 CHECK(close_value(get(e,0x2f8),94.5));
 CHECK(close_value(get(e,0x2fc),10.5) && close_value(get(e,0x300),20.3) && close_value(get(e,0x304),31.5));
 CHECK(close_value(get(e,0x308),11.4) && close_value(get(e,0x30c),23.9) && close_value(get(e,0x310),34.2));
 CHECK(close_value(get(e,0x314),9.5) && close_value(get(e,0x32c),9.3) && close_value(get(e,0x344),10.7));
 CHECK(close_value(get(e,0x338),9.3-6.3) && close_value(get(e,0x340),28.8-8.1));
 CHECK(f.counter==0);CHECK(bh_entities_dispose(&f.pool));
}
static void forces(void) {
 Fixture f;BHEntity *e;BHVec3 a;int i,next,expected;float smoothed[15],sum=0;init(&f);e=f.body.entity;
 CHECK(bh_player_force_control_b(&f.control,123,0));
 CHECK(close_value(f.physics.acceleration.y,-9.82) && close_value(f.physics.acceleration.x,0) && close_value(f.physics.acceleration.z,0));
 CHECK(get(e,0x250)==1 && !f.counter);
 CHECK(bh_physics_step_publish(&f.body));memcpy(&a,(unsigned char *)e+0x60,12);CHECK(!memcmp(&a,&f.physics.acceleration,12));
 memcpy(smoothed,(unsigned char *)e+0x2bc,60);for(i=0;i<15;++i){CHECK(smoothed[i]>=0);sum+=smoothed[i];}CHECK(close_value(get(e,0x2f8),sum));
 /* Full controller followed by original skip path retains earlier physics and
    decays every flame channel, without consuming RNG. */
 a=f.physics.acceleration;srand(19);expected=rand();srand(19);CHECK(bh_player_force_control_b(&f.control,0,1));next=rand();CHECK(next==expected && !memcmp(&a,&f.physics.acceleration,12));
 for(i=0;i<15;++i)CHECK(close_value(get(e,0x2bc+i*4),smoothed[i]*.9));
 /* Every fourth unstable call uses two draws, with third only above6. */
 f.physics.pose.m[1][1]=-1;put(e,0x250,5);f.counter=2;srand(7);expected=rand();srand(7);
 CHECK(bh_player_force_control_b(&f.control,0,0) && f.counter==3 && close_value(get(e,0x250),5+1.0/30));CHECK(rand()==expected);
 put(e,0x250,5);srand(23);rand();rand();expected=rand();srand(23);CHECK(bh_player_force_control_b(&f.control,0,0) && f.counter==4);CHECK(rand()==expected);
 f.counter=7;put(e,0x250,7);srand(29);rand();rand();rand();expected=rand();srand(29);CHECK(bh_player_force_control_b(&f.control,0,0) && f.counter==8);CHECK(rand()==expected);
 f.basis.indices_a[0].sample[0]=15;f.basis.indices_a[0].count=2;a=f.physics.acceleration;
 CHECK(!bh_player_force_control_b(&f.control,0,0) && !memcmp(&a,&f.physics.acceleration,12));CHECK(bh_entities_dispose(&f.pool));
}
static void variants(void) {
 typedef int (__cdecl *Controller)(BHPlayerForceControl *,float,int);
 static Controller controllers[3]={bh_player_force_control_a,bh_player_force_control_c,bh_player_force_control_d};
 Fixture f;int mode,i,j,expected;BHVec3 acceleration;float reference[40];unsigned char *visuals;
 for(mode=0;mode<3;++mode) {
  init(&f);f.physics.velocity.z=100;
  CHECK(controllers[mode](&f.control,.02f,0));
  CHECK(close_value(f.physics.acceleration.x,0) && close_value(f.physics.acceleration.y,-9.82));
  CHECK(close_value(f.physics.acceleration.z,mode==2?-7.5:-15));
  CHECK(close_value(f.physics.angular_acceleration.x,0) && close_value(f.physics.angular_acceleration.y,0) && close_value(f.physics.angular_acceleration.z,0));
  CHECK(bh_entities_dispose(&f.pool));
  /* Each complete owner retains the always-run smoothing and all24 endpoint
     coordinates. Control updates are skipped without a draw from shared RNG. */
  init(&f);for(i=0;i<15;++i)put(f.body.entity,0x2bc+i*4,(float)i);visuals=(unsigned char *)f.body.entity+0x2bc;
  srand(31);expected=rand();srand(31);CHECK(controllers[mode](&f.control,0,1));CHECK(rand()==expected);
  if(mode==0)memcpy(reference,visuals,sizeof(reference));else for(i=0;i<40;++i)CHECK(close_value(((float *)visuals)[i],reference[i]));
  CHECK(bh_entities_dispose(&f.pool));
  for(j=-2;j<=2;++j)for(i=-2;i<=2;++i) {
   init(&f);f.physics.velocity.x=(float)(j*20);f.physics.velocity.y=(float)(i*5);f.physics.velocity.z=(float)(i*100);
   put(f.body.entity,0x18c,(float)j*.1f);put(f.body.entity,0x190,(float)i*.1f);put(f.body.entity,0x194,(float)j*.1f);
   CHECK(controllers[mode](&f.control,.02f,0));acceleration=f.physics.acceleration;
   CHECK(_finite(acceleration.x) && _finite(acceleration.y) && _finite(acceleration.z));
   CHECK(_finite(f.physics.angular_acceleration.x) && _finite(f.physics.angular_acceleration.y) && _finite(f.physics.angular_acceleration.z));
   CHECK(bh_physics_step_publish(&f.body));CHECK(close_value(get(f.body.entity,0x60),acceleration.x));
   CHECK(bh_entities_dispose(&f.pool));
  }
 }
}
int __cdecl bh_player_force_control_checkpoint(unsigned long *count) {
 BHMathEnvironment env;checks=0;failures=0;CHECK(bh_math_environment_enter(&env));visual();forces();variants();bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"player-force-control\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_PLAYER_FORCE_CONTROL_STANDALONE
int main(void) {return bh_player_force_control_checkpoint(NULL);}
#endif
