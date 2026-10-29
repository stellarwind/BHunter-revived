/* Complete original pose owner, explicit resource/clock bindings. Queue,
   sound and math dependencies are shared real source owners, no rendering. */
#include "../src/bh_actor_vehicle_draw.h"
#include "../src/bh_draw_submit.h"
#include "../src/bh_camera_curve.h"
#include "../src/bh_rotation.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHDrawQueue *queue;
static BHSoundQueue *sound;
static void __cdecl oracle_model(unsigned long b,unsigned long r,const BHVec3 *p,const BHVec3 *a,float m){bh_draw_submit_resource(queue,b,r,p,a,m);}
static void __cdecl oracle_billboard(unsigned long b,unsigned long r,const BHVec3 *p,float w,float h,float a,unsigned long f,float alpha){bh_draw_billboard(queue,b,r,p,w,h,a,f,alpha);}
static unsigned long __cdecl oracle_request(unsigned long r,float l,float f,const BHVec3 *p,const BHVec3 *v){return bh_sound_request(sound,r,l,f,p,v);}
static void __cdecl oracle_update(unsigned long h,float l,float f,const BHVec3 *p,const BHVec3 *v){bh_sound_update(sound,h,l,f,p,v);}
#include "../build/ai-pose-match/original-pose.h"
static unsigned long rng=0x434920;
static float sample(void){rng=rng*1664525UL+1013904223UL;return(float)((long)(rng>>8)-8388608)/65536;}
static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static unsigned long value(void *p,unsigned long o){unsigned long v;memcpy(&v,(char*)p+o,4);return v;}
static void number(void *p,unsigned long o,float v){memcpy((char*)p+o,&v,4);}
int main(void){
 BHMathEnvironment env;BHDrawQueue q[2];BHSoundQueue sq[2];BHSoundResource *r[2];
 BHActorVehicleDrawContext c;BHActorRouteState s;BHEntity a,b;BHSoundVoice *va,*vb;
 unsigned char *code;void(__cdecl *original)(BHEntity*);unsigned long i,j,k,seed,next,draw_bad=0,sound_bad=0,rng_bad=0,body_bad=0,pose_bad=0,glow_bad=0;
 memset(&c,0,sizeof(c));memset(q,0,sizeof(q));memset(sq,0,sizeof(sq));
 if(!bh_math_environment_enter(&env))return 2;
 for(j=0;j<2;++j){if(!bh_draw_queue_init(q+j,24,0)||!bh_sound_queue_init(sq+j,0,0,0))return 2;r[j]=bh_sound_resource_allocate(sq+j);if(!r[j])return 2;r[j]->streaming=1;r[j]->base_frequency=22050;}
 c.queue=q;c.sound=sq;c.damage_resource=31;c.glowing_model=17;c.glow_first=41;c.glow_second=42;
 queue=q+1;sound=sq+1;code=VirtualAlloc(0,16384,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memset(code,0xcc,16384);install_original_pose(code);word(code,8200,31);word(code,8204,17);word(code,8208,41);word(code,8212,42);
 FlushInstructionCache(GetCurrentProcess(),code,16384);original=(void(__cdecl *)(BHEntity*))code;
 for(i=0;i<8192;++i){
  memset(&a,0,sizeof(a));memset(&s,0,sizeof(s));s.tick=(unsigned short)(i*29);s.duration_ticks=(unsigned short)((i&4096)?-(long)(i%511+1):i%511+1);s.sound=BH_SOUND_INVALID;
  c.tick=(unsigned short)(s.tick+(short)((i*13)%1600-400));c.animation_clock=(long)(i*37)-100000;
  for(j=0;j<12;++j)((float*)s.controls)[j]=sample();
  if(i%32<4){memset(s.controls,0,sizeof(s.controls));if(i%32==1){s.controls[1].y=10;s.controls[2].y=20;s.controls[3].y=30;}if(i%32==2){s.controls[0].x=s.controls[1].x=10;s.controls[2].x=s.controls[3].x=20;}if(i%32==3){s.controls[1].x=10;s.controls[2].z=10;s.controls[3].x=20;}}
  bh_actor_route_write(&a,&s);for(j=0;j<3;++j)number(&a,0x15c+4*j,sample());number(&a,0x19c,i%3?1:(sample()+128)/256);
  word(&a,0x114,i&1?17:18);word(&a,0x10c,r[0]->index);
  if(i%4==0)word(&a,0x104,bh_sound_request(sq,r[0]->index,1,1,0,0));
  b=a;if(i%4==0){unsigned long h=bh_sound_request(sq+1,r[1]->index,1,1,0,0);if(h!=value(&a,0x104))return 2;}
  bh_draw_queue_begin(q);bh_draw_queue_begin(q+1);memset(q[0].commands,0,24*sizeof(BHDrawCommand));memset(q[1].commands,0,24*sizeof(BHDrawCommand));bh_draw_queue_begin(q);bh_draw_queue_begin(q+1);
  word(code,8192,c.tick);word(code,8216,(unsigned long)c.animation_clock);seed=i*0x9e3779b9UL+7;
  srand(seed);original(&b);next=rand();srand(seed);if(!bh_actor_vehicle_draw(&c,&a))return 3;if((unsigned long)rand()!=next)++rng_bad;
  if(q[0].count!=q[1].count||memcmp(q[0].commands,q[1].commands,q[0].count*sizeof(BHDrawCommand))){++draw_bad;
   if(memcmp(q[0].commands[9].words,q[1].commands[9].words,24)){++pose_bad;if(pose_bad<3){printf("pose %lu",i);for(k=0;k<6;++k)printf(" %08lx/%08lx",q[0].commands[9].words[k],q[1].commands[9].words[k]);puts("");}}
   if(i&1)++glow_bad;
  }
  va=bh_sound_voice(sq,value(&a,0x104));vb=bh_sound_voice(sq+1,value(&b,0x104));if(!va||!vb||memcmp(&va->position,&vb->position,24))++sound_bad;
  if(memcmp(&a,&b,sizeof(a)))++body_bad;
  if(va)bh_sound_stop(sq,value(&a,0x104));if(vb)bh_sound_stop(sq+1,value(&b,0x104));
 }
 printf("{\"scenario\":\"complete-pose-owner\",\"cases\":8192,\"draw_differences\":%lu,\"pose_differences\":%lu,\"glow_cases_with_differences\":%lu,\"sound_differences\":%lu,\"rng_differences\":%lu,\"entity_differences\":%lu,\"shared_source_dependencies\":true}\n",draw_bad,pose_bad,glow_bad,sound_bad,rng_bad,body_bad);
 for(j=0;j<2;++j){bh_draw_queue_dispose(q+j);bh_sound_queue_dispose(sq+j);}VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);
 return draw_bad||sound_bad||rng_bad||body_bad;
}
