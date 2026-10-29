/* Original death-velocity arithmetic, not a full tick/game execution. */
#include "../src/bh_actor_damage.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#ifdef RESPONSE_VELOCITY_ORIGINAL
#include "../build/ai-response-velocity-match/original-velocity.h"
#define ORIGINAL_WINDOW_BYTES 102
#else
#include "../build/ai-damage-velocity-match/original-velocity.h"
#define ORIGINAL_WINDOW_BYTES 93
#endif
static unsigned long rng=0x435bf9UL;
static float sample(void){float f;unsigned long bits;rng=rng*1664525UL+1013904223UL;bits=(rng&0x807fffffUL)|(((rng>>24)%120+65)<<23);memcpy(&f,&bits,4);return f;}
static float symmetric_scale(float after,float at){volatile float value=(float)(((double)after-at)*15);return value;}
int main(void){
 static const unsigned long edges[]={0,0x80000000UL,1,0x80000001UL,0x007fffffUL,0x00800000UL,0x3f800000UL,0x3f800001UL,0x33000000UL,0xb3000000UL,0xbf800000UL,0xbf800001UL,0x7f7fffffUL,0xff7fffffUL,0x7f800000UL,0xff800000UL,0x7fc12345UL,0xffc54321UL,0x7f812345UL};
 BHMathEnvironment env;BHVec3 after,at,source,reference;unsigned char *code;unsigned long i,j,k,bad=0,cases=0,axes[3]={0,0,0},symmetric_differences=0;float symmetric_y,symmetric_z;
 void(__cdecl *original)(const BHVec3*,const BHVec3*,BHVec3*);
 if(!bh_math_environment_enter(&env))return 2;code=VirtualAlloc(0,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!code)return 2;install_original_velocity(code);FlushInstructionCache(GetCurrentProcess(),code,4096);original=(void(__cdecl *)(const BHVec3*,const BHVec3*,BHVec3*))code;
 for(i=0;i<8192+sizeof(edges)/sizeof(edges[0])*sizeof(edges)/sizeof(edges[0]);++i){
  for(j=0;j<3;++j){
   if(i<8192){((float*)&after)[j]=sample();((float*)&at)[j]=sample();}
   else{k=i-8192;memcpy((float*)&after+j,edges+k%(sizeof(edges)/sizeof(edges[0])),4);memcpy((float*)&at+j,edges+k/(sizeof(edges)/sizeof(edges[0])),4);}
  }
  original(&after,&at,&reference);bh_actor_damage_death_velocity(&after,&at,&source);++cases;
  symmetric_y=symmetric_scale(after.y,at.y);symmetric_z=symmetric_scale(after.z,at.z);
  if(memcmp(&symmetric_y,&reference.y,4)||memcmp(&symmetric_z,&reference.z,4))++symmetric_differences;
  if(memcmp(&source,&reference,12)){++bad;for(j=0;j<3;++j)if(memcmp((float*)&source+j,(float*)&reference+j,4))++axes[j];}
 }
 printf("{\"death_velocity_cases\":%lu,\"differences\":%lu,\"axis_differences\":[%lu,%lu,%lu],\"symmetric_yz_negative_control_differences\":%lu,\"precision\":\"PC53\",\"original_window_bytes\":%d,\"full_tick_claimed\":false}\n",cases,bad,axes[0],axes[1],axes[2],symmetric_differences,ORIGINAL_WINDOW_BYTES);
 VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);return bad?1:0;
}
