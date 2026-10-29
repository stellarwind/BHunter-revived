/* Whole original query body and complete selector, selected events. Unexercised
   owner-dependent calls trap loudly; no level, game or renderer is loaded. */
#include "../src/bh_actor_response.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void __cdecl oracle_unavailable(void){puts("Excluded oracle dependency executed");exit(3);}
#include "../build/ai-query-match/original-query.h"
static unsigned long rng=0x43a1d0UL;
static float sample(void){rng=rng*1664525UL+1013904223UL;return (float)((long)(rng>>8)-8388608)/65536;}
static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static void number(void *p,unsigned long o,float v){memcpy((char*)p+o,&v,4);}
int main(void){
 BHMathEnvironment env;BHEntityPool pool;BHEntity *e,*player,*other,baseline,ref;
 BHActorResponseContext c;BHActorDamageContext damage;BHActorRouteContext route;
 BHActorAttackContext attack;BHActorVehicleLifetimeContext vehicles;BHActorRouteState state;
 unsigned char *code;void (__cdecl *original)(BHEntity*,void*);
 static const unsigned long events[]={0,1,2,3,11,13,15,16,17};
 unsigned long event,i,j,bad[9]={0,0,0,0,0,0,0,0,0},errors=0,total=0,targets[3]={0,0,0};
 float packet[10],reference[10];unsigned long *w=(unsigned long*)packet;
 memset(&c,0,sizeof(c));memset(&damage,0,sizeof(damage));memset(&route,0,sizeof(route));memset(&attack,0,sizeof(attack));memset(&vehicles,0,sizeof(vehicles));
 if(!bh_math_environment_enter(&env)||!bh_entities_init(&pool,8,0))return 2;
 e=bh_entity_acquire(&pool);player=bh_entity_acquire(&pool);other=bh_entity_acquire(&pool);if(!e||!player||!other)return 2;
 route.tick=0x9abc;route.actor_limit=8;damage.pool=&pool;damage.route=&route;vehicles.pool=&pool;vehicles.route=&route;vehicles.head=other;
 c.damage=&damage;c.attack=&attack;c.vehicles=&vehicles;c.player=player;
 if(!bh_actor_response_bind(&c,e)||!bh_actor_response_bind(&c,other))return 2;baseline=*e;
 code=VirtualAlloc(0,16384,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memset(code,0xcc,16384);install_original_query(code);memcpy(code+12288,&route.tick,2);memcpy(code+12292,&player,4);memcpy(code+12296,&other,4);
 FlushInstructionCache(GetCurrentProcess(),code,16384);original=(void (__cdecl *)(BHEntity*,void*))code;
 for(event=0;event<9;++event){
  for(i=0;i<8192;++i){
   *e=baseline;memset(&state,0,sizeof(state));
   for(j=0;j<12;++j)((float*)state.controls)[j]=sample();
   state.duration=(sample()+129)/32;state.phase=(sample()+128)/64;
   state.speed=(sample()+129)/8;state.speed_scale=(sample()+129)/128;state.sound=BH_SOUND_INVALID;
   bh_actor_route_write(e,&state);
   if(events[event]==15){
    BHActorRouteState target_state=state;unsigned long eligible=i&1,weight=i%7?100:0,kind=i%3?2:6;
    target_state.phase=(sample()+128)/64;for(j=0;j<12;++j)((float*)target_state.controls)[j]=sample();
    bh_actor_route_write(other,&target_state);word(other,0xfc,kind);word(other,0x100,i%11?100:0xffffffffUL);word(other,0xf0,0);
    memset((char*)other+0x15c,0,12);word(player,0x238,eligible);word(player,0x234,weight);
    player->position.x=sample();player->position.y=sample();player->position.z=sample();
   }
   for(j=0;j<6;++j)number(e,0x15c+4*j,sample());
   number(e,0x194,(sample()+128)/512);number(e,0x19c,(sample()+128)/256);
   word(e,0xfc,i%4==0?6:i%4==1?2:i%4==2?1:11);word(e,0x118,i&1);
   word(e,0x1b8,(unsigned long)other);word(e,0x1ac,(unsigned long)player);
   for(j=0;j<10;++j)packet[j]=sample();w[0]=events[event];
   if(w[0]==0){packet[1]=(sample()+128)/256;w[2]=i%3==0?0:i%3==1?(unsigned long)player:(unsigned long)other;}
   if(w[0]==1){packet[4]=sample()+128;packet[5]=(sample()+129)*4;}
   if(w[0]==17)w[1]=i&1?(unsigned long)other:(unsigned long)player;
   ref=*e;memcpy(reference,packet,40);original(&ref,reference);bh_actor_response_query(e,packet);
   if(c.error){++errors;continue;}
   if(events[event]==15){unsigned long target;memcpy(&target,(char*)e+0x1b8,4);++targets[target==(unsigned long)other?2:target==(unsigned long)player?1:0];}
   if(memcmp(e,&ref,sizeof(ref))||memcmp(packet,reference,40)){
    if(bad[event]==0){unsigned long at;for(at=0;at<sizeof(ref);++at)if(((unsigned char*)e)[at]!=((unsigned char*)&ref)[at])break;printf("first_difference event=%lu actor_offset=%lx\n",events[event],at);}
    ++bad[event];
   }
  }
  total+=bad[event];printf("{\"event\":%lu,\"cases\":8192,\"differences\":%lu}\n",events[event],bad[event]);
 }
 {
  static const unsigned long ignored[]={5,6,7,8,9,10,12,14,18,19,21,0xffffffffUL};
  unsigned long ignored_bad=0;
  for(i=0;i<sizeof(ignored)/sizeof(ignored[0]);++i){
   *e=baseline;ref=*e;memset(packet,0xa5,sizeof(packet));w[0]=ignored[i];memcpy(reference,packet,sizeof(packet));
   original(&ref,reference);bh_actor_response_query(e,packet);
   if(c.error||memcmp(e,&ref,sizeof(ref))||memcmp(packet,reference,sizeof(packet)))++ignored_bad;
  }
  total+=ignored_bad;
  printf("{\"default_event_cases\":12,\"default_event_differences\":%lu}\n",ignored_bad);
 }
 printf("{\"query_differences\":%lu,\"source_errors\":%lu,\"precision\":\"PC53\",\"targets_none_player_actor\":[%lu,%lu,%lu],\"excluded_events\":[4,20]}\n",total,errors,targets[0],targets[1],targets[2]);
 VirtualFree(code,0,MEM_RELEASE);bh_entities_dispose(&pool);bh_math_environment_leave(&env);return total||errors?1:0;
}
