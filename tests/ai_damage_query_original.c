/* Complete ordinary damage query 00435e80 and original curve. Owner-dependent
   removal calls trap; no game, level or rendering code is executed. */
#include "../src/bh_actor_damage.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void __cdecl oracle_unavailable(void){puts("Excluded damage query dependency executed");exit(3);}
#include "../build/ai-damage-query-match/original-query.h"
static unsigned long rng=0x435e80UL;
static float sample(void){rng=rng*1664525UL+1013904223UL;return(float)((long)(rng>>8)-8388608)/65536;}
static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static void number(void *p,unsigned long o,float v){memcpy((char*)p+o,&v,4);}
int main(void){
 static const unsigned long events[]={0,1,2,3,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,21,0xffffffffUL};
 static const unsigned short clocks[][3]={{100,90,30},{0,65535,1},{65535,0,65535},{32768,0,32768},{1000,1000,30},{65530,5,30},{50,0,101},{32767,0,32767}};
 static const unsigned long fades[]={0x7fc12345UL,0xffc54321UL,0x7f812345UL,0x7f800000UL,0xff800000UL};
 BHMathEnvironment env;BHEntityPool pool;BHSoundQueue sound;BHActorRouteContext route;BHActorDamageContext c;
 BHEntity *e,*other,*owner,baseline,ref;BHActorRouteState s;unsigned char *code;
 void(__cdecl *original)(BHEntity*,void*);float packet[10],reference[10];unsigned long *w=(unsigned long*)packet;
 unsigned long event,i,j,bad[sizeof(events)/sizeof(events[0])]={0},total=0,errors=0;
 memset(&route,0,sizeof(route));memset(&c,0,sizeof(c));
 if(!bh_math_environment_enter(&env)||!bh_entities_init(&pool,4,0)||!bh_sound_queue_init(&sound,0,0,0))return 2;
 e=bh_entity_acquire(&pool);other=bh_entity_acquire(&pool);owner=bh_entity_acquire(&pool);if(!e||!other||!owner)return 2;
 route.sound=&sound;c.pool=&pool;c.route=&route;if(!bh_actor_damage_bind(&c,e))return 2;baseline=*e;
 code=VirtualAlloc(0,16384,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!code)return 2;memset(code,0xcc,16384);install_original_query(code);FlushInstructionCache(GetCurrentProcess(),code,16384);original=(void(__cdecl *)(BHEntity*,void*))code;
 for(event=0;event<sizeof(events)/sizeof(events[0]);++event){
  for(i=0;i<8192;++i){
   *e=baseline;c.error=BH_ACTOR_ROUTE_OK;memset(&s,0,sizeof(s));s.sound=BH_SOUND_INVALID;s.duration=1;s.speed=25;s.speed_scale=1;
   for(j=0;j<12;++j)((float*)s.controls)[j]=sample();route.tick=clocks[i%8][0];s.tick=clocks[i%8][1];s.duration_ticks=clocks[i%8][2];word(code,12288,route.tick);bh_actor_route_write(e,&s);
   for(j=0;j<6;++j)number(e,0x15c+4*j,sample());number(e,0x194,(sample()+128)/512);number(e,0x19c,(sample()+128)/256);
   word(e,0x1ac,(unsigned long)owner);word(e,0x1b8,(unsigned long)other);
   for(j=0;j<10;++j)packet[j]=sample();w[0]=events[event];
   if(w[0]==0){packet[1]=(sample()+128)/256;w[2]=(unsigned long)other;if(i%4==0){number(e,0x194,1.0f-1.0f/16777216.0f);packet[1]=1.0f/33554432.0f;}}
   if(w[0]==1){packet[4]=sample()+128;packet[5]=(sample()+129)*4;}
   if(w[0]==11&&i%32==0)word(e,0x19c,fades[(i/32)%5]);
   if(w[0]==17)w[1]=(unsigned long)(i&1?owner:other);
   ref=*e;memcpy(reference,packet,sizeof(packet));original(&ref,reference);bh_actor_damage_query(e,packet);
   if(c.error){++errors;continue;}
   if(memcmp(e,&ref,sizeof(ref))||memcmp(packet,reference,sizeof(packet))){
    if(!bad[event]){unsigned long at;for(at=0;at<sizeof(ref);++at)if(((unsigned char*)e)[at]!=((unsigned char*)&ref)[at])break;printf("first_difference event=%lu actor_offset=%lx\n",events[event],at);}++bad[event];
   }
  }
  total+=bad[event];printf("{\"event\":%lu,\"cases\":8192,\"differences\":%lu}\n",events[event],bad[event]);
 }
 printf("{\"damage_query_differences\":%lu,\"source_errors\":%lu,\"precision\":\"PC53\",\"excluded_lifecycle_events\":[4,20]}\n",total,errors);
 VirtualFree(code,0,MEM_RELEASE);bh_entities_dispose(&pool);bh_sound_queue_dispose(&sound);bh_math_environment_leave(&env);return total||errors?1:0;
}
