#include "../src/bh_actor_basis_control.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;if(failures<12)printf("FAIL basis control line%d\n",__LINE__);}} while(0)
typedef struct Fixture {
 BHActorBasisContext c;BHActorBasisSample samples[15];BHActorBasisGroup a[7],b[7];
 BHActorBasisIndices ia[7],ib[7];BHActorBasisScale sa[7],sb[7];
} Fixture;
static void init(Fixture *f) {
 memset(f,0,sizeof(*f));f->c.samples=f->samples;f->c.sample_capacity=15;
 f->c.groups_a=f->a;f->c.groups_b=f->b;f->c.indices_a=f->ia;f->c.indices_b=f->ib;
 f->c.scales_a=f->sa;f->c.scales_b=f->sb;f->c.a_capacity=f->c.b_capacity=7;
 CHECK(bh_actor_basis_acquire(&f->c));
}
static int close_value(double a,double b) {return fabs(a-b)<1e-5*(1+fabs(b));}
static double residual(const BHVec3 *v,const BHVec3 *u,const BHVec3 *w,double a,double b) {
 double x=v->x+a*u->x+b*w->x,y=v->y+a*u->y+b*w->y,z=v->z+a*u->z+b*w->z;
 return sqrt(x*x+y*y+z*z);
}
/* Independent two-column normal equations using raw vectors, rather than the
   production normalized-vector and stored reciprocal representation. */
static double ideal_best(const BHVec3 *v,const BHActorBasisGroup *g,int direction) {
 int i,j;const BHVec3 *u,*w;double best,res,uu,ww,uw,uv,wv,det,a,b;
 BHVec3 zero={0,0,0};best=residual(v,&zero,&zero,0,0);
 for(i=0;i<7;++i)for(j=i+1;j<7;++j) {
  u=direction?&g[i].direction:&g[i].torque;w=direction?&g[j].direction:&g[j].torque;
  uu=(double)u->x*u->x+(double)u->y*u->y+(double)u->z*u->z;
  ww=(double)w->x*w->x+(double)w->y*w->y+(double)w->z*w->z;
  uw=(double)u->x*w->x+(double)u->y*w->y+(double)u->z*w->z;
  uv=(double)u->x*v->x+(double)u->y*v->y+(double)u->z*v->z;
  wv=(double)w->x*v->x+(double)w->y*v->y+(double)w->z*v->z;
  det=uu*ww-uw*uw;if(det<1e-8)continue;
  a=(uw*wv-ww*uv)/det;b=(uw*uv-uu*wv)/det;
  if(a<0 || b<0 || a+b==0)continue;
  res=residual(v,u,w,a,b);if(res<best)best=res;
 }
 return best;
}
static void pairs(void) {
 Fixture f;BHVec3 v;int i,j,result,mode,x,y,z;float a,b;double before,after;
 init(&f);
 for(mode=0;mode<2;++mode)for(x=-4;x<=4;++x)for(y=-4;y<=4;++y)for(z=-4;z<=4;++z) {
  v.x=(float)x;v.y=(float)y;v.z=(float)z;
  result=mode?bh_actor_basis_b_pair(&f.c,&v,&i,&j,&a,&b):bh_actor_basis_a_pair(&f.c,&v,&i,&j,&a,&b);
  before=sqrt((double)x*x+(double)y*y+(double)z*z);
  CHECK(result==(mode?-1:1) || result==0);
  if(result) {
   CHECK(i>=0 && i<j && j<7);CHECK(a>=0 && b>=0 && a<=1000000 && b<=1000000);
   after=residual(&v,mode?&f.b[i].direction:&f.a[i].torque,mode?&f.b[j].direction:&f.a[j].torque,a,b);
   CHECK(after<before+1e-5);
  }else {CHECK(i==0 && j==0 && a==0 && b==0);after=before;}
  CHECK(fabs(after-ideal_best(&v,mode?f.b:f.a,mode))<1e-4);
 }
 /* Independent cardinal geometry: unique pair has exact cancellation. */
 memset(f.a,0,sizeof(f.a));memset(f.b,0,sizeof(f.b));f.c.a_count=f.c.b_count=2;
 f.a[0].torque.x=f.a[0].unit_torque.x=1;f.a[1].torque.y=f.a[1].unit_torque.y=1;
 f.b[0].direction=f.b[0].unit_direction=f.a[0].torque;f.b[1].direction=f.b[1].unit_direction=f.a[1].torque;
 f.sa[0].torque=f.sa[1].torque=f.sb[0].direction=f.sb[1].direction=1;
 v.x=-3;v.y=-4;v.z=0;
 CHECK(bh_actor_basis_a_pair(&f.c,&v,&i,&j,&a,&b)==1 && i==0 && j==1 && a==3 && b==4);
 CHECK(bh_actor_basis_b_pair(&f.c,&v,&i,&j,&a,&b)==-1 && a==3 && b==4);
 v.x=3;v.y=4;CHECK(!bh_actor_basis_a_pair(&f.c,&v,&i,&j,&a,&b));
 v.x=-2000000;v.y=-3000000;CHECK(bh_actor_basis_a_pair(&f.c,&v,&i,&j,&a,&b)==1 && a==1000000 && b==1000000);
 f.a[1]=f.a[0];v.x=-1;v.y=0;CHECK(!bh_actor_basis_a_pair(&f.c,&v,&i,&j,&a,&b));
 f.c.a_count=0;CHECK(!bh_actor_basis_a_pair(&f.c,&v,&i,&j,&a,&b) && i==0 && j==0 && a==0 && b==0);
}
static void apply(void) {
 Fixture f;BHPhysicsState state,saved;BHVec3 inertia={2,4,8};float accumulated[15],copy[15];BHActorBasisIndices group;int k;
 init(&f);memset(&state,0,sizeof(state));memset(accumulated,0,sizeof(accumulated));
 state.pose.m[0][0]=state.pose.m[1][1]=state.pose.m[2][2]=1;
 memset(&group,0,sizeof(group));group.count=2;group.sample[0]=1;
 CHECK(bh_actor_basis_apply_group(&f.c,&state,2,&inertia,accumulated,&group,4));
 CHECK(accumulated[1]==4 && state.acceleration.x==-2 && state.acceleration.y==0 && state.acceleration.z==0);
 CHECK(close_value(state.angular_acceleration.y,-1.5) && state.angular_acceleration.x==0 && state.angular_acceleration.z==0);
 /* Rotated matrix and repeated index; inertia divides local torque. */
 memset(&state,0,sizeof(state));state.pose.m[0][1]=-1;state.pose.m[1][0]=1;state.pose.m[2][2]=1;
 group.count=3;group.sample[1]=1;
 CHECK(bh_actor_basis_apply_group(&f.c,&state,2,&inertia,accumulated,&group,2));
 CHECK(accumulated[1]==8 && state.acceleration.x==0 && state.acceleration.y==-2 && close_value(state.angular_acceleration.y,-1.5));
 saved=state;memcpy(copy,accumulated,sizeof(copy));group.count=1;
 CHECK(bh_actor_basis_apply_group(&f.c,&state,2,&inertia,accumulated,&group,7) && !memcmp(&state,&saved,sizeof(state)));
 group.count=3;group.sample[1]=15;
 CHECK(!bh_actor_basis_apply_group(&f.c,&state,2,&inertia,accumulated,&group,7) && !memcmp(copy,accumulated,sizeof(copy)) && !memcmp(&state,&saved,sizeof(state)));
 group=f.ia[1];memset(&state.acceleration,0,12);memset(&state.angular_acceleration,0,12);
 CHECK(bh_actor_basis_apply_group(&f.c,&state,1,&inertia,accumulated,&group,1));
 for(k=0;k<15;++k)CHECK(accumulated[k]>=0);
 CHECK(!bh_actor_basis_apply_group(&f.c,&state,0,&inertia,accumulated,&group,1));
}
int __cdecl bh_actor_basis_control_checkpoint(unsigned long *count) {
 BHMathEnvironment env;checks=0;failures=0;CHECK(bh_math_environment_enter(&env));pairs();apply();bh_math_environment_leave(&env);
 printf("{\"scenario\":\"actor-basis-control\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);if(count)*count=checks;return failures?1:0;
}
#ifdef BH_ACTOR_BASIS_CONTROL_STANDALONE
int main(void) {return bh_actor_basis_control_checkpoint(NULL);}
#endif
