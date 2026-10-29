/* Complete 00438ff0/00421610 with original route/math closure. Target callbacks,
   messages and sound lifetime dependencies are real source definitions on
   source query definitions on real pool entities. */
#include "../src/bh_actor_response.h"
#include "../src/bh_navigation.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHEntityPool *oracle_pool;
static unsigned long source_draws,reference_draws;
static unsigned long source_stops,reference_stops;
static BHSoundQueue *oracle_sound;
static BHMessageState *oracle_messages;
int __cdecl oracle_rand_source(void){++source_draws;return rand();}
static int __cdecl oracle_rand_original(void){++reference_draws;return rand();}
int __cdecl oracle_sound_source(BHSoundQueue *q,unsigned long handle){++source_stops;return bh_sound_stop(q,handle);}
static int __cdecl oracle_sound_stop(unsigned long handle){++reference_stops;return bh_sound_stop(oracle_sound,handle);}
static void __cdecl oracle_message(const char *text){if(!bh_message_enqueue(oracle_messages,text)){puts("Real reference message queue failed");exit(5);}}
static void __cdecl oracle_unavailable(void){puts("Excluded pursuit report/sound path executed");exit(3);}
static void __cdecl oracle_release(BHEntity *e){if(!bh_entity_release(oracle_pool,e))exit(4);}
#include "../build/ai-promotion-match/original-promotion.h"
static unsigned long rng=0x438ff0UL;
static float sample(void){rng=rng*1664525UL+1013904223UL;return(float)((long)(rng>>8)-8388608)/65536;}
static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static unsigned long value(void *p,unsigned long o){unsigned long v;memcpy(&v,(char*)p+o,4);return v;}
static void number(void *p,unsigned long o,float v){memcpy((char*)p+o,&v,4);}
static unsigned long canonical(unsigned long v,BHEntity **a,BHEntity **b,BHWorldGeometry *world,unsigned char raw[4][0x340]){unsigned long i;for(i=0;i<5;++i)if(v==(unsigned long)b[i])return(unsigned long)a[i];for(i=0;i<4;++i)if(v==(unsigned long)raw[i])return(unsigned long)(world+i);return v;}
int main(void){
 BHMathEnvironment env;BHEntityPool pools[2];BHEntity *a[5],*b[5],ref;
 BHWorldGeometry world[4];BHGeometryModel model;BHPickupRoom rooms[4];BHActorRouteContext route;
 BHActorPursuitRouteContext c;BHNavigation nav;BHNavigationNode nodes[4];BHSceneRoute cells[4];long cities[4];
 BHActorResponseContext response[2];BHActorDamageContext damage[2];BHActorVehicleLifetimeContext vehicles[2];BHActorAttackContext attack[2];
 BHActorRouteState s,target_state;unsigned char raw[4][0x340],*code;void(__cdecl *original)(BHEntity*);
 BHSoundQueue sound[2];BHSoundResource *resources[2];unsigned long handles[2];BHMessageState messages[2];BHEntity panels[2];unsigned long identity=0x442950UL;
 static const char *names[]={"CITY_A","CITY_B"};unsigned long now,last,sound_bad=0,message_bad=0,reports=0,report_forms[4]={0,0,0,0};
 static const unsigned long directions[]={0,1,2,4,5,6};
 unsigned long i,j,k,seed,expected1,expected2,actual1,actual2,body_bad=0,list_bad=0,rng_bad=0,errors=0,query_errors=0,forced_cases=0,acquire_cases=0,prediction_cases=0;
 memset(&route,0,sizeof(route));memset(&c,0,sizeof(c));memset(&nav,0,sizeof(nav));memset(&model,0,sizeof(model));memset(world,0,sizeof(world));memset(rooms,0,sizeof(rooms));memset(raw,0,sizeof(raw));memset(response,0,sizeof(response));memset(damage,0,sizeof(damage));memset(vehicles,0,sizeof(vehicles));memset(attack,0,sizeof(attack));
 if(!bh_math_environment_enter(&env)||!bh_entities_init(pools,8,0)||!bh_entities_init(pools+1,8,0))return 2;
 for(i=0;i<2;++i){if(!bh_sound_queue_init(sound+i,0,0,0))return 2;resources[i]=bh_sound_resource_allocate(sound+i);if(!resources[i])return 2;resources[i]->streaming=1;resources[i]->base_frequency=44100;}
 for(i=0;i<5;++i){a[i]=bh_entity_acquire(pools);b[i]=bh_entity_acquire(pools+1);if(!a[i]||!b[i])return 2;}
 model.portal_count=2;route.rooms=rooms;route.room_count=4;route.actor_limit=16;route.tick=0xbeef;route.sound=sound;
 for(i=0;i<4;++i){rooms[i].world=world+i;world[i].model=&model;world[i].navigation.count=2;world[i].minimum.x=-100;world[i].minimum.y=-200;world[i].minimum.z=-300;world[i].maximum.x=100;world[i].maximum.y=200;world[i].maximum.z=300;nodes[i].placement=world+i;cities[i]=i&1;memcpy(raw[i]+0x1b0,&world[i].minimum,12);memcpy(raw[i]+0x1bc,&world[i].maximum,12);word(raw[i],0x108,i);word(raw[i],0x11c,cities[i]);}
 nav.nodes=nodes;nav.node_count=4;nav.routes.cells=cells;c.route=&route;c.navigation=&nav;c.player=a[4];c.actor_head=a[3];c.actor_limit=16;c.city_ids=cities;c.city_names=names;c.city_count=2;c.messages=messages;c.clock=c.last_report=1000;
 for(i=0;i<2;++i){damage[i].pool=pools+i;damage[i].route=&route;vehicles[i].pool=pools+i;vehicles[i].route=&route;response[i].damage=damage+i;response[i].vehicles=vehicles+i;response[i].attack=attack+i;response[i].player=i?b[4]:a[4];for(j=3;j<5;++j)if(!bh_actor_response_bind(response+i,i?b[j]:a[j]))return 2;}
 oracle_pool=pools+1;oracle_sound=sound+1;oracle_messages=messages+1;code=VirtualAlloc(0,32768,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;memset(code,0xcc,32768);install_original_promotion(code);memset(code+24576,0,0x174);word(code,24576+0x18,2);word(code,12292,(unsigned long)b[4]);word(code,12296,(unsigned long)b[3]);word(code,12304,(unsigned long)cells);memcpy(code+12320,names,sizeof(names));FlushInstructionCache(GetCurrentProcess(),code,32768);original=(void(__cdecl *)(BHEntity*))(code+28672);
 for(i=0;i<8192;++i){
  seed=i*0x9e3779b9UL+17;memset(&s,0,sizeof(s));s.current=world;s.next=world+1;s.speed=(sample()+129)/8;s.speed_scale=(sample()+129)/128;s.u=(sample()+128)/256;s.v=(sample()+128)/256;s.jitter=(sample()+128)/256;s.sound=BH_SOUND_INVALID;s.sound_frames=i%3;s.random=rng;
  for(j=0;j<12;++j)((float*)s.controls)[j]=sample();
  handles[0]=handles[1]=BH_SOUND_INVALID;if(i%5==0){for(j=0;j<2;++j)handles[j]=bh_sound_request(sound+j,resources[j]->index,1,1,0,0);if(handles[0]==BH_SOUND_INVALID||handles[0]!=handles[1])return 2;s.sound=handles[0];}
  for(j=0;j<2;++j){bh_message_state_init(messages+j);memset(panels+j,0,sizeof(panels[j]));number(panels+j,0xe8,4);messages[j].panel_entity=panels+j;messages[j].panel_step_identity=&identity;}
  now=(i&1?0x80000000UL:0x7fffff00UL)+i;last=now-((i/3)%3==0?1801UL:(i/3)%3==1?1800UL:0UL);memcpy(&c.clock,&now,4);memcpy(&c.last_report,&last,4);word(code,12308,now);word(code,12312,last);cities[1]=i%11==0?-1:1;word(raw[1],0x11c,cities[1]);
  for(j=0;j<3;++j){memset(a[j]->payload,0xa5,sizeof(a[j]->payload));memset(b[j]->payload,0xa5,sizeof(b[j]->payload));word(a[j],0xf4,0);word(a[j],0xf8,0);word(b[j],0xf4,0);word(b[j],0xf8,0);}
  for(j=0;j<4;++j){rooms[j].actors=0;word(raw[j],0x128,0);nodes[j].junction_first_port=(i+j)%3-1;nodes[j].junction_second_port=(i/3+j)%3-1;word(raw[j],0x10c,nodes[j].junction_first_port);word(raw[j],0x110,nodes[j].junction_second_port);}
  nav.junction_count=i%3;nav.routes.node_count=nav.junction_count;word(code,12300,nav.junction_count);for(j=0;j<4;++j){cells[j].exit_slot=(i+j)%3-1;cells[j].distance=1;}
  world[1].query_flags=i%4<2?8:0;world[0].query_flags=i%5==0?0x10:i%5==1?0x20:0;world[2].query_flags=i%7==0?0x10:i%7==1?0x20:0;for(j=0;j<3;++j)word(raw[j],0x114,world[j].query_flags);
  for(j=0;j<2;++j){BHPortal *p=world[1].navigation.portals+j;memset(p,0,sizeof(*p));p->direction=j?(i&1?directions[(i/3)%6]^4:directions[(i/17)%6]):directions[(i/3)%6];p->plane=(long)sample();p->scratch[0]=sample();p->scratch[1]=p->scratch[0]+(sample()+129)/4;p->scratch[2]=sample();p->scratch[3]=p->scratch[2]+(sample()+129)/4;p->neighbor=(BHPortalNode*)(world+(j?2:0));memcpy(raw[1]+0x1cc+j*60,p,60);word(raw[1],0x204+j*60,(unsigned long)raw[j?2:0]);}
  bh_actor_route_write(a[0],&s);bh_actor_route_write(b[0],&s);word(a[0],0xc4,(unsigned long)world);word(b[0],0xc4,(unsigned long)raw[0]);word(b[0],0x1a4,(unsigned long)raw[0]);word(b[0],0x1a8,(unsigned long)raw[1]);
  word(a[0],0xfc,i%3==0?1:i%3==1?2:6);word(b[0],0xfc,value(a[0],0xfc));word(a[0],0x110,i%7==0?0xffffffffUL:0);word(b[0],0x110,value(a[0],0x110));
  number(a[0],0x1b4,(sample()+128)/256);memcpy((char*)b[0]+0x1b4,(char*)a[0]+0x1b4,4);word(a[0],0x1d4,i%4==0);word(b[0],0x1d4,i%4==0);if(i%4==0)++forced_cases;if(i%3==0)++acquire_cases;
  for(j=0;j<6;++j){float f=sample();number(a[0],0x1bc+4*j,f);number(b[0],0x1bc+4*j,f);}
  for(j=3;j<5;++j){memset(&target_state,0,sizeof(target_state));target_state.duration=1;target_state.phase=.5f;target_state.speed=20;target_state.speed_scale=1;target_state.sound=BH_SOUND_INVALID;for(k=0;k<12;++k)((float*)target_state.controls)[k]=sample();bh_actor_route_write(a[j],&target_state);bh_actor_route_write(b[j],&target_state);word(a[j],0xc4,(unsigned long)(world+(i+j)%4));word(b[j],0xc4,(unsigned long)raw[(i+j)%4]);word(a[j],0xfc,2);word(b[j],0xfc,2);word(a[j],0x100,100+i%100);word(b[j],0x100,100+i%100);word(a[j],0xf0,0);word(b[j],0xf0,0);for(k=0;k<3;++k){float f=sample();number(a[j],0x15c+4*k,f);number(b[j],0x15c+4*k,f);}a[j]->position.x=sample();a[j]->position.y=sample();a[j]->position.z=sample();b[j]->position=a[j]->position;}
  word(a[4],0x234,100);word(b[4],0x234,100);word(a[4],0x238,i%4!=0);word(b[4],0x238,i%4!=0);word(a[4],0x2b4,i&1);word(b[4],0x2b4,i&1);
  word(a[0],0x1b8,i%4==0?0:(unsigned long)a[i&1?3:4]);word(b[0],0x1b8,i%4==0?0:(unsigned long)b[i&1?3:4]);
  if(i%3==0){rooms[0].actors=a[1];word(a[1],0xf4,(unsigned long)a[0]);word(a[0],0xf8,(unsigned long)a[1]);word(raw[0],0x128,(unsigned long)b[1]);word(b[1],0xf4,(unsigned long)b[0]);word(b[0],0xf8,(unsigned long)b[1]);}
  else{rooms[0].actors=a[0];word(raw[0],0x128,(unsigned long)b[0]);if(i%3==1){word(a[0],0xf4,(unsigned long)a[1]);word(a[1],0xf8,(unsigned long)a[0]);word(b[0],0xf4,(unsigned long)b[1]);word(b[1],0xf8,(unsigned long)b[0]);}}
  if(i&1){rooms[1].actors=a[2];word(raw[1],0x128,(unsigned long)b[2]);}
  word(code,12288,route.tick);source_draws=reference_draws=source_stops=reference_stops=0;srand(seed);original(b[0]);expected1=rand();expected2=rand();srand(seed);if(!bh_actor_pursuit_route(&c,a[0])){++errors;continue;}actual1=rand();actual2=rand();if(source_draws!=reference_draws||actual1!=expected1||actual2!=expected2)++rng_bad;
  if(source_stops!=reference_stops||(bh_sound_voice(sound,handles[0])!=0)!=(bh_sound_voice(sound+1,handles[1])!=0))++sound_bad;
  if(memcmp(messages[0].ring,messages[1].ring,sizeof(messages[0].ring))||memcmp(messages[0].arena,messages[1].arena,sizeof(messages[0].arena))||memcmp(&messages[0].arena_used,&messages[1].arena_used,24)||value(panels,0xe8)!=value(panels+1,0xe8)||(unsigned long)c.last_report!=value(code,12312))++message_bad;
  if(messages[0].ring_count){++reports;if(strstr(messages[0].ring[0],"crime fighters!"))++report_forms[0];else if(strstr(messages[0].ring[0],"We have reports"))++report_forms[1];else if(strstr(messages[0].ring[0],"Calling all"))++report_forms[2];else if(strstr(messages[0].ring[0],"Surveillance"))++report_forms[3];else ++message_bad;}
  if(response[0].error||response[1].error)++query_errors;
  ref=*b[0];{static const unsigned long offsets[]={0xc4,0xf4,0xf8,0x1a4,0x1a8,0x1b8,0x1ac};for(j=0;j<7;++j)word(&ref,offsets[j],canonical(value(&ref,offsets[j]),a,b,world,raw));}
  if(memcmp((char*)a[0]+0xe4,(char*)&ref+0xe4,0x108)||value(a[0],0xc4)!=value(&ref,0xc4)){if(body_bad<4){for(k=0xe4;k<0x1ec;++k)if(((unsigned char*)a[0])[k]!=((unsigned char*)&ref)[k])break;printf("pursuit case=%lu first_offset=%lx draws=%lu/%lu\n",i,k,source_draws,reference_draws);}++body_bad;}
  if(value(a[0],0x1d4)==1&&i%4)++prediction_cases;
  for(j=0;j<4;++j)if((unsigned long)rooms[j].actors!=canonical(value(raw[j],0x128),a,b,world,raw))++list_bad;
  for(j=0;j<2;++j)if(handles[j]!=BH_SOUND_INVALID)bh_sound_stop(sound+j,handles[j]);
 }
 printf("{\"pursuit_cases\":8192,\"payload_differences\":%lu,\"list_differences\":%lu,\"crt_rng_differences\":%lu,\"source_errors\":%lu,\"query_errors\":%lu,\"sound_differences\":%lu,\"message_differences\":%lu,\"reports\":%lu,\"report_forms\":[%lu,%lu,%lu,%lu],\"forced_cases\":%lu,\"type1_cases\":%lu,\"prediction_cases\":%lu}\n",body_bad,list_bad,rng_bad,errors,query_errors,sound_bad,message_bad,reports,report_forms[0],report_forms[1],report_forms[2],report_forms[3],forced_cases,acquire_cases,prediction_cases);
 VirtualFree(code,0,MEM_RELEASE);bh_sound_queue_dispose(sound);bh_sound_queue_dispose(sound+1);bh_entities_dispose(pools);bh_entities_dispose(pools+1);bh_math_environment_leave(&env);return body_bad||list_bad||rng_bad||errors||query_errors||sound_bad||message_bad?1:0;
}

