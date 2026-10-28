/* Bounded reference arithmetic, never linked into the source game. */
#include "../src/bh_actor_attack.h"
#include "../src/bh_target_steering.h"
#include "../src/bh_route_curve_length.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../build/ai-controls/original.h"
static BHVec3 target_point;
static void __cdecl target_query(BHEntity *e,void *raw){long *p=raw;(void)e;if(*p==16){memset(p+1,0,36);memcpy(p+1,&target_point,12);}}
static unsigned long rng=1979;
static float sample(void){rng=rng*1664525UL+1013904223UL;return (float)((long)(rng>>8)-8388608)/65536.0f;}
static void word(void *e,unsigned long o,unsigned long v){memcpy((char*)e+o,&v,4);}
int main(void){
 BHMathEnvironment env;BHEntityPool pool;BHEntity *e,*target,ref;
 BHActorAttackContext attack;BHActorPursuitRouteContext pursuit;BHActorRouteContext route;
 BHEffectProjectileContext effects;BHHomingProjectileContext homing;BHProjectileCreateContext projectiles;BHAreaProjectileContext area;
 BHNavigation nav;BHWorldGeometry world;BHActorRouteState s,got;
 unsigned char raw_world[512],*code;unsigned long i,j,steer_bad=0,pursuit_bad=0,clamp_bad=0,gate_bad=0;
 void (__cdecl *gate)(BHEntity*);void (__cdecl *steer)(BHEntity*,BHVec3*);void (__cdecl *original_pursuit)(BHEntity*);void (__cdecl *clamp)(void*,BHVec3*);
 memset(&attack,0,sizeof(attack));memset(&pursuit,0,sizeof(pursuit));memset(&route,0,sizeof(route));memset(&effects,0,sizeof(effects));memset(&homing,0,sizeof(homing));memset(&projectiles,0,sizeof(projectiles));memset(&area,0,sizeof(area));memset(&nav,0,sizeof(nav));memset(&world,0,sizeof(world));memset(raw_world,0,sizeof(raw_world));
 if(!bh_math_environment_enter(&env)||!bh_entities_init(&pool,16,0))return 2;
 code=VirtualAlloc(0,32768,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memset(code,0xcc,32768);install_original(code);memset(code+12292,0,4);gate=(void (__cdecl *)(BHEntity*))(code+16384);FlushInstructionCache(GetCurrentProcess(),code,32768);
 steer=(void (__cdecl *)(BHEntity*,BHVec3*))code;original_pursuit=(void (__cdecl *)(BHEntity*))(code+1024);clamp=(void (__cdecl *)(void*,BHVec3*))(code+2048);
 e=bh_entity_acquire(&pool);target=bh_entity_acquire(&pool);if(!e||!target)return 2;
 bh_entity_set_query(&pool,target,target_query);homing.pool=&pool;effects.effects=&homing;attack.effects=&effects;attack.pursuit=&pursuit;attack.projectiles=&projectiles;attack.area=&area;pursuit.route=&route;pursuit.navigation=&nav;route.tick=0xbeef;
 world.minimum.x=world.minimum.y=world.minimum.z=-1000;world.maximum.x=world.maximum.y=world.maximum.z=1000;
 memcpy(raw_world+0x1b0,&world.minimum,12);memcpy(raw_world+0x1bc,&world.maximum,12);
 for(i=0;i<8192;++i){
  BHVec3 p,q;float pursuit_factor;
  memset(&s,0,sizeof(s));s.current=s.next=&world;s.duration=1;s.phase=.5f;s.speed=30;s.speed_scale=1;s.sound=BH_SOUND_INVALID;
  for(j=0;j<12;++j)((float*)s.controls)[j]=sample();
  target_point.x=sample();target_point.y=sample();target_point.z=sample();pursuit_factor=(sample()+128)/256;
  bh_actor_route_write(e,&s);word(e,0x1b8,(unsigned long)target);word(e,0x1d4,1);memcpy((char*)e+0x1b4,&pursuit_factor,4);word(e,0x1e0,0xffff);
  ref=*e;steer(&ref,&target_point);
  if(!bh_actor_attack(&attack,e)||memcmp((char*)&ref+0x12c,(char*)e+0x12c,12)||memcmp((char*)&ref+0x150,(char*)e+0x150,12))++steer_bad;
  /* Whole forced-return route branch: real source curve length/clamp/timing. */
  s.speed=(sample()+129)/8;s.speed_scale=(sample()+129)/64;
  bh_actor_route_write(e,&s);word(e,0xfc,6);word(e,0x1b8,0);word(e,0x1d4,1);
  for(j=0;j<6;++j){float v=sample();memcpy((char*)e+0x1bc+j*4,&v,4);}
  ref=*e;word(&ref,0x1a4,(unsigned long)raw_world);original_pursuit(&ref);
  if(!bh_actor_pursuit_route(&pursuit,e)||memcmp((char*)&ref+0x120,(char*)e+0x120,12)||memcmp((char*)&ref+0x12c,(char*)e+0x12c,12)||memcmp((char*)&ref+0x144,(char*)e+0x144,24)||memcmp((char*)&ref+0x180,(char*)e+0x180,8)||memcmp((char*)&ref+0x1e4,(char*)e+0x1e4,4)){
   if(pursuit_bad<3){bh_actor_route_read(&ref,&got);bh_actor_route_read(e,&s);printf("pursuit %lu ticks %u/%u duration %.9g/%.9g\n",i,got.duration_ticks,s.duration_ticks,got.duration,s.duration);}++pursuit_bad;
  }
  p.x=sample()*16;p.y=sample()*16;p.z=sample()*16;q=p;clamp(raw_world,&p);bh_world_clamp_point(&world,&q);if(memcmp(&p,&q,12))++clamp_bad;
 }
 for(i=0;i<65536;++i){
  float f=(sample()+128)/256;double threshold=.98-(double)f*.03;float lateral=(float)sqrt(1-threshold*threshold);unsigned long bits;
  memset(&s,0,sizeof(s));s.current=s.next=&world;s.duration=1;s.speed=30;s.speed_scale=1;s.sound=BH_SOUND_INVALID;
  s.controls[0].z=0;s.controls[1].z=30;s.controls[2].z=10;s.controls[3].z=20;
  target_point.x=lateral*100;target_point.y=0;target_point.z=(float)(threshold*100);
  memcpy(&bits,&target_point.x,4);bits+=(i%9)-4;memcpy(&target_point.x,&bits,4);
  /* Rotate the cone onto all axes. Alternate with range-boundary and
     unconstrained targets, then exercise cooldown/odd-frame returns. */
  if(i%4==1){target_point.x=sample();target_point.y=sample();target_point.z=sample();}
  if(i%4==2){target_point.x=0;target_point.z=200;memcpy(&bits,&target_point.z,4);bits+=(i%9)-4;memcpy(&target_point.z,&bits,4);}
  if(i%16==15)target_point.x=target_point.y=target_point.z=0;
  if(i%3){unsigned long axis=i%3-1;float v=((float*)&target_point)[axis];((float*)&target_point)[axis]=target_point.z;target_point.z=v;for(j=0;j<4;++j){((float*)&s.controls[j])[axis]=s.controls[j].z;s.controls[j].z=0;}}
  bh_actor_route_write(e,&s);word(e,0x1b8,(unsigned long)target);word(e,0x1d4,0);memcpy((char*)e+0x1b4,&f,4);word(e,0x1e0,0);word(e,0x1d8,10);
  if(i%16==3)word(e,0x1e0,0x0014ffff);
  if(i%16==7)word(e,0x1e0,0x00140009);
  if(i%16==11)word(e,0x1e0,0x00140002);
  ref=*e;gate(&ref);
  if(!bh_actor_attack(&attack,e)||memcmp((char*)&ref+0x1e0,(char*)e+0x1e0,2)){if(gate_bad<3)printf("gate %lu pursuit %.9g target %.9g %.9g frame %u/%u\n",i,f,target_point.x,target_point.z,*(unsigned short*)((char*)&ref+0x1e0),*(unsigned short*)((char*)e+0x1e0));++gate_bad;}
 }
 printf("firing_gate_cases=65536 differences=%lu\n",gate_bad);
 printf("{\"cases\":8192,\"steering_differences\":%lu,\"forced_pursuit_differences\":%lu,\"clamp_differences\":%lu}\n",steer_bad,pursuit_bad,clamp_bad);
 VirtualFree(code,0,MEM_RELEASE);bh_entities_dispose(&pool);bh_math_environment_leave(&env);return steer_bad||pursuit_bad||clamp_bad||gate_bad?1:0;
}

