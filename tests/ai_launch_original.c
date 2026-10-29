/* Full original 0043bca0 and position encoder 004234d0. The original launcher
   calls real source constructors and real NULL-peer transport. No sessions,
   network writes, game, level, draw consumer or renderer execute. */
#include "../src/bh_actor_launch.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHActorConstructContext constructors[2];
static BHNetworkTransport transports[2];
static BHWorldGeometry world;
static void *world_image;
static BHEntity *reference_entity;
static unsigned char *code;
static unsigned long draws,construct_calls,send_calls,send_size,dependency_errors;
static HRESULT send_result;
static unsigned long __cdecl oracle_rand(void){++draws;return(unsigned long)rand();}
static BHEntity *__cdecl oracle_construct(void *w,unsigned long type,long token,float speed,unsigned long tag,unsigned long seed){
 ++construct_calls;if(w!=world_image){++dependency_errors;return 0;}
 reference_entity=bh_actor_construct_room(constructors+1,&world,type,token,speed,tag,seed);return reference_entity;
}
static HRESULT __cdecl oracle_send(DPID recipient,void *bytes,unsigned long size,int guaranteed){
 ++send_calls;send_size=size;if(recipient||guaranteed||bytes!=code+16384||size!=25)++dependency_errors;
 send_result=bh_network_send(transports+1,recipient,bytes,size,guaranteed);return send_result;
}
#include "../build/ai-launch-match/original-launch.h"
static void word(void *p,unsigned long at,unsigned long v){memcpy((char*)p+at,&v,4);}
int main(void){
 static const unsigned long speed_bits[]={0,0x80000000UL,0x3f7fffffUL,0xbf7fffffUL,0x41cf3333UL,0xc12ccccdUL,0x4effffffUL,0x4f000000UL,0x4f000001UL,0xcf000000UL,0xcf000001UL,0x4f7fffffUL,0x4f800000UL,0x4f800001UL,0xcf800001UL,0x5effffffUL,0x5f000000UL,0x5f000001UL,0xdf000000UL,0x7f7fffffUL,0xff7fffffUL,0x7f800000UL,0xff800000UL,0x7fc12345UL,0x7f812345UL};
 static const float ordinary_speeds[]={0,19.5f,25.9f,-2.75f};
 static const long tokens[]={0,123,-12345,2147483647L,(-2147483647L-1)};
 BHMathEnvironment env;BHEntityPool pools[2];BHSoundQueue sound;
 BHActorLaunchContext launch;BHActorConstructRoom binding[2];BHPickupRoom rooms[2];BHActorRouteContext route[2];
 BHActorResponseContext response[2];BHActorDamageContext damage[2];BHActorAttackContext attack[2];BHActorVehicleLifetimeContext vehicles[2];BHActorVehicleDrawContext draw;
 BHEffectProjectileContext effects[2];BHHomingProjectileContext homing[2];BHNavigation nav;BHNavigationNode nodes[4];
 BHEntity *entity;unsigned char raw[0x240];unsigned long models[25],sounds[25],cycle[4],live[2];
 unsigned long i,j,side,seed,mode,type,index,input_bits,source_next,reference_next,expected_next;
 unsigned long body_bad=0,packet_bad=0,rng_bad=0,list_bad=0,send_bad=0,source_errors=0,network_cases=0,suppressed_cases=0,boundary_cases=0;
 long token,tag;float speed;void *original_world;
 void(__cdecl *original)(void*,unsigned long,long,float,long);
 memset(constructors,0,sizeof(constructors));memset(&world,0,sizeof(world));memset(raw,0,sizeof(raw));memset(binding,0,sizeof(binding));memset(rooms,0,sizeof(rooms));memset(route,0,sizeof(route));memset(response,0,sizeof(response));memset(damage,0,sizeof(damage));memset(attack,0,sizeof(attack));memset(vehicles,0,sizeof(vehicles));memset(&draw,0,sizeof(draw));memset(effects,0,sizeof(effects));memset(homing,0,sizeof(homing));memset(&launch,0,sizeof(launch));memset(&nav,0,sizeof(nav));
 if(!bh_math_environment_enter(&env)||!bh_sound_queue_init(&sound,0,0,0))return 2;
 world_image=raw;world.navigation.next=&world.navigation;
 for(j=0;j<25;++j){models[j]=100+j;sounds[j]=200+j;}for(j=0;j<4;++j)cycle[j]=300+j;
 for(side=0;side<2;++side){
  if(!bh_entities_init(pools+side,4,0)||!bh_network_transport_init(transports+side,0,0))return 2;
  rooms[side].world=&world;route[side].rooms=rooms+side;route[side].room_count=1;route[side].actor_limit=8;route[side].sound=&sound;
  damage[side].pool=pools+side;damage[side].route=route+side;vehicles[side].pool=pools+side;vehicles[side].route=route+side;
  homing[side].pool=pools+side;effects[side].effects=homing+side;attack[side].effects=effects+side;
  response[side].damage=damage+side;response[side].vehicles=vehicles+side;response[side].attack=attack+side;
  binding[side].room=rooms+side;binding[side].bound_resource=902;
  constructors[side].response=response+side;constructors[side].draw=&draw;constructors[side].rooms=binding+side;constructors[side].room_count=1;constructors[side].models=models;constructors[side].sounds=sounds;constructors[side].resource_count=25;constructors[side].cycle_models=cycle;constructors[side].cycle_count=4;constructors[side].special_model=400;
 }
 nav.nodes=nodes;nav.node_count=4;launch.actors=constructors;launch.navigation=&nav;launch.transport=transports;
 code=VirtualAlloc(0,32768,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!code)return 2;memset(code,0xcc,32768);install_original_launch(code);FlushInstructionCache(GetCurrentProcess(),code,32768);original=(void(__cdecl *)(void*,unsigned long,long,float,long))code;
 for(i=0;i<6144;++i){
  mode=i<4096?i%4:2;type=i<4096?(i/4)%17:((i-4096)&1?0:3);
  if(i<4096)speed=ordinary_speeds[(i/68)%4];else{input_bits=speed_bits[((i-4096)/2)%(sizeof(speed_bits)/sizeof(speed_bits[0]))];memcpy(&speed,&input_bits,4);++boundary_cases;}
  token=tokens[(i/3)%5];tag=(long)(i*193UL)-600000;seed=i*0x9e3779b9UL+17;index=(i/7)%4;
  world.minimum.x=(float)((long)(i%31)-15);world.minimum.y=-20;world.minimum.z=-30;world.maximum.x=world.minimum.x+(i%5?40:0);world.maximum.y=40;world.maximum.z=50;
  memcpy(raw+0x1b0,&world.minimum,12);memcpy(raw+0x1bc,&world.maximum,12);word(raw,0x108,index);
  memset(nodes,0,sizeof(nodes));nodes[index].placement=&world;launch.mode=(long)mode;launch.tick=(unsigned short)(i*37UL);launch.sent=0;launch.send_result=E_FAIL;
  for(side=0;side<2;++side){
   rooms[side].actors=0;vehicles[side].head=0;vehicles[side].count=0;constructors[side].cycle=i&3;constructors[side].error=0;
   route[side].tick=launch.tick;live[side]=0x12340000UL|launch.tick;route[side].live_tick=i&2?live+side:0;
   memset(pools[side].free_slots->payload,0xa5,sizeof(pools[side].free_slots->payload));
  }
  memset(launch.packet,0xa5,sizeof(launch.packet));memset(code+16384,0xa5,28);word(code,12288,mode);word(code,12292,live[0]);word(code,12296,(unsigned long)(code+16392));
  draws=construct_calls=send_calls=send_size=0;reference_entity=0;send_result=E_FAIL;
  original_world=(i<4096&&i%13==0&&(mode==2||mode==3)&&(type==0||type==3))?0:raw;
  srand(seed);rand();rand();expected_next=(unsigned long)rand();srand(seed);
  original(original_world,type,token,speed,tag);reference_next=(unsigned long)rand();
  srand(seed);entity=bh_actor_launch(&launch,original_world?&world:0,type,token,speed,tag);source_next=(unsigned long)rand();
  if(reference_next!=expected_next||source_next!=expected_next||draws!=2)++rng_bad;
  if(launch.error||constructors[0].error||constructors[1].error)++source_errors;
  if(launch.sent!=send_calls||(send_calls&&(launch.send_result!=send_result||send_size!=25))||transports[0].native_calls||transports[1].native_calls)++send_bad;
  if(memcmp(launch.packet,code+16384,28))++packet_bad;
  if(send_calls)++network_cases;if(!construct_calls)++suppressed_cases;
  if((entity!=0)!=(reference_entity!=0)||construct_calls>1)++body_bad;
  else if(entity&&(entity->flags!=reference_entity->flags||memcmp(&entity->position,&reference_entity->position,12)||memcmp((char*)entity+0x3c,(char*)reference_entity+0x3c,12)||memcmp((char*)entity+0xc4,(char*)reference_entity+0xc4,16)||memcmp(entity->payload,reference_entity->payload,0x108)))++body_bad;
  if(vehicles[0].count!=vehicles[1].count||constructors[0].cycle!=constructors[1].cycle||(rooms[0].actors==entity)!=(rooms[1].actors==reference_entity)||pools[0].used!=pools[1].used)++list_bad;
  if(entity&&!bh_entity_release(pools,entity))return 2;if(reference_entity&&!bh_entity_release(pools+1,reference_entity))return 2;
 }
 printf("{\"launch_cases\":6144,\"quantization_boundary_cases\":%lu,\"body_differences\":%lu,\"packet_differences\":%lu,\"rng_differences\":%lu,\"list_differences\":%lu,\"send_differences\":%lu,\"source_errors\":%lu,\"dependency_errors\":%lu,\"encoded_packets\":%lu,\"suppressed_cases\":%lu,\"precision\":\"PC53\",\"transport\":\"real NULL peer, no native network calls\"}\n",boundary_cases,body_bad,packet_bad,rng_bad,list_bad,send_bad,source_errors,dependency_errors,network_cases,suppressed_cases);
 for(side=0;side<2;++side){bh_entities_dispose(pools+side);bh_network_transport_dispose(transports+side);}bh_sound_queue_dispose(&sound);VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);
 return body_bad||packet_bad||rng_bad||list_bad||send_bad||source_errors||dependency_errors?1:0;
}
