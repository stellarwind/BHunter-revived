/* Complete original type10 receive/position decode, real source constructor.
   Only complete allocated envelopes execute original code. Bounded malformed
   source inputs are checked separately against the existing codec contract. */
#include "../src/bh_actor_launch.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHActorConstructContext reference_constructor;
static BHEntity *reference_entity;
static unsigned long constructor_calls;
static BHEntity *__cdecl oracle_construct_position(const BHVec3 *p,unsigned long type,long token,float speed,unsigned long tag,unsigned long seed){++constructor_calls;reference_entity=bh_actor_construct_position(&reference_constructor,p,type,token,speed,tag,seed);return reference_entity;}
#include "../build/ai-receive-match/original-launch.h"
static void word(void *p,unsigned long at,unsigned long v){memcpy((char*)p+at,&v,4);}
static unsigned long value(void *p,unsigned long at){unsigned long v;memcpy(&v,(char*)p+at,4);return v;}
int main(void){
 BHMathEnvironment env;BHEntityPool pools[2];BHSoundQueue sound;BHActorConstructContext local;
 BHActorConstructRoom binding[2];BHPickupRoom rooms[2];BHWorldGeometry world;BHActorRouteContext route[2];
 BHActorResponseContext response[2];BHActorDamageContext damage[2];BHActorAttackContext attack[2];BHActorVehicleLifetimeContext vehicles[2];BHActorVehicleDrawContext draw;
 BHEffectProjectileContext effects[2];BHHomingProjectileContext homing[2];BHNavigation nav;BHNavigationNode nodes[4];
 BHActorLaunchContext launch;BHPacketWriter writer;BHPacketStats stats;BHVec3 position;BHEntity *entity;
 unsigned char raw_world[0x240],source_bytes[28],reference_bytes[28],payload[9],short_bytes[8],*code;
 unsigned long models[25],sounds[25],cycle[4],live[2],good,wire,i,j,side,mode,seed,expected,rng_bad=0;
 unsigned long actor_bad=0,counter_bad=0,tick_bad=0,packet_bad=0,source_bad=0,constructor_failures=0,invalid_indices=0,checksums=0,bounded_bad=0;
 void(__cdecl *original)(void);
 memset(&local,0,sizeof(local));memset(&reference_constructor,0,sizeof(reference_constructor));memset(&world,0,sizeof(world));memset(binding,0,sizeof(binding));memset(rooms,0,sizeof(rooms));memset(route,0,sizeof(route));memset(response,0,sizeof(response));memset(damage,0,sizeof(damage));memset(attack,0,sizeof(attack));memset(vehicles,0,sizeof(vehicles));memset(&draw,0,sizeof(draw));memset(effects,0,sizeof(effects));memset(homing,0,sizeof(homing));memset(&nav,0,sizeof(nav));memset(&launch,0,sizeof(launch));memset(raw_world,0,sizeof(raw_world));
 if(!bh_math_environment_enter(&env)||!bh_sound_queue_init(&sound,0,0,0))return 2;
 world.minimum.x=world.minimum.y=world.minimum.z=-10;world.maximum.x=world.maximum.y=world.maximum.z=10;world.navigation.next=&world.navigation;
 memcpy(raw_world+0x1b0,&world.minimum,12);memcpy(raw_world+0x1bc,&world.maximum,12);
 for(j=0;j<25;++j){models[j]=100+j;sounds[j]=200+j;}for(j=0;j<4;++j){cycle[j]=300+j;memset(nodes+j,0,sizeof(*nodes));nodes[j].placement=&world;}
 nav.nodes=nodes;nav.node_count=4;
 for(side=0;side<2;++side){BHActorConstructContext *c=side?&reference_constructor:&local;
  if(!bh_entities_init(pools+side,4,0))return 2;rooms[side].world=&world;route[side].rooms=rooms+side;route[side].room_count=1;route[side].actor_limit=8;route[side].sound=&sound;
  damage[side].pool=pools+side;damage[side].route=route+side;vehicles[side].pool=pools+side;vehicles[side].route=route+side;
  homing[side].pool=pools+side;effects[side].effects=homing+side;attack[side].effects=effects+side;
  response[side].damage=damage+side;response[side].vehicles=vehicles+side;response[side].attack=attack+side;
  binding[side].room=rooms+side;binding[side].bound_resource=902;c->response=response+side;c->draw=&draw;c->rooms=binding+side;c->room_count=1;c->models=models;c->sounds=sounds;c->resource_count=25;c->cycle_models=cycle;c->cycle_count=4;c->special_model=400;c->first=&world;c->world_count=1;
 }
 launch.actors=&local;launch.navigation=&nav;launch.stats=&stats;launch.good=&good;
 code=VirtualAlloc(0,32768,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!code)return 2;memset(code,0xcc,32768);install_original_launch(code);FlushInstructionCache(GetCurrentProcess(),code,32768);original=(void(__cdecl *)(void))code;
 word(code,12312,4);word(code,12316,(unsigned long)(code+16384));for(j=0;j<4;++j)word(code,16384+4*j,(unsigned long)raw_world);
 for(i=0;i<4096;++i){
  mode=i%5;seed=i*0x9e3779b9UL+17;position.x=(float)((long)(i%17)-8);position.y=0;position.z=(float)((long)((i/17)%17)-8);if(mode==4)position.x=20;
  for(j=0;j<9;++j)payload[j]=(unsigned char)(seed>>(j%4)*8);payload[6]=(unsigned char)((i/5)%17);
  if(!bh_packet_begin(&writer,source_bytes,28,10,(unsigned short)(i*37UL))||!bh_packet_write_position(&writer,&world,(unsigned short)(mode==2?65535:i%4),&position)||!bh_packet_append(&writer,payload,9)||!bh_packet_finish(&writer,&wire))return 2;
  if(mode==1){source_bytes[16]^=1;++checksums;}if(mode==2)++invalid_indices;
  memset(source_bytes+25,0xa5,3);memcpy(reference_bytes,source_bytes,28);
  stats.accepted=100;stats.rejected=200;good=300;launch.received=0;
  for(side=0;side<2;++side){BHActorConstructContext *c=side?&reference_constructor:&local;
   rooms[side].actors=0;vehicles[side].head=0;vehicles[side].count=0;c->error=0;c->resource_count=mode==3?0:25;c->cycle=i&3;
   live[side]=0xabcd0000UL|(unsigned short)(i*13UL);route[side].live_tick=i&1?live+side:0;route[side].tick=(unsigned short)live[side];memset(pools[side].free_slots->payload,0xa5,sizeof(pools[side].free_slots->payload));
  }
  word(code,12288,(unsigned long)reference_bytes);word(code,12296,100);word(code,12300,300);word(code,12304,200);word(code,12308,live[1]);constructor_calls=0;reference_entity=0;
  /* The real reference constructor consumes the same word the original
     receive owner temporarily overwrites, rather than a copied fixture tick. */
  route[1].live_tick=(unsigned long*)(code+12308);
  srand(seed);expected=(unsigned long)rand();srand(seed);original();if((unsigned long)rand()!=expected)++rng_bad;
  srand(seed);j=bh_actor_launch_receive(&launch,source_bytes,25,28);if((unsigned long)rand()!=expected)++rng_bad;entity=pools[0].used?pools[0].active:0;
  if(stats.accepted!=value(code,12296)||stats.rejected!=value(code,12304)||good!=value(code,12300)||launch.received!=constructor_calls||launch.reader.cursor!=value(code,12292)-(unsigned long)reference_bytes)++counter_bad;
  if((route[0].live_tick?live[0]:(unsigned long)(0xabcd0000UL|route[0].tick))!=value(code,12308))++tick_bad;
  if(memcmp(source_bytes,reference_bytes,28))++packet_bad;
  if((entity!=0)!=(reference_entity!=0))++actor_bad;
  else if(entity&&(entity->flags!=reference_entity->flags||memcmp(entity->payload,reference_entity->payload,0x108)||memcmp((char*)entity+0xc4,(char*)reference_entity+0xc4,16))){
   if(!actor_bad){unsigned long at;for(at=0xe4;at<0x1ec;++at)if(((unsigned char*)entity)[at]!=((unsigned char*)reference_entity)[at])break;printf("first_actor_difference case=%lu mode=%lu offset=%lx source=%lx reference=%lx\n",i,mode,at,value(entity,at&~3UL),value(reference_entity,at&~3UL));}++actor_bad;
  }
  if(vehicles[0].count!=vehicles[1].count||local.cycle!=reference_constructor.cycle)++actor_bad;
  if(constructor_calls&&!reference_entity)++constructor_failures;
  if((j!=0)!=(entity!=0)||(constructor_calls==0?launch.error!=2:reference_entity?launch.error!=0:launch.error!=3))++source_bad;
  if(entity&&!bh_entity_release(pools,entity))return 2;if(reference_entity&&!bh_entity_release(pools+1,reference_entity))return 2;
 }
 /* The documented tri-state source codec must reject a short envelope even
    when its reader still points at a previous checksum-rejected packet. */
 local.resource_count=25;rooms[0].actors=0;vehicles[0].head=0;vehicles[0].count=0;
 position.x=position.y=position.z=0;memset(payload,0,9);payload[6]=1;payload[7]=1;payload[8]=25;
 if(!bh_packet_begin(&writer,source_bytes,28,10,777)||!bh_packet_write_position(&writer,&world,0,&position)||!bh_packet_append(&writer,payload,9)||!bh_packet_finish(&writer,&wire))return 2;
 for(i=0;i<3;++i){unsigned long before_good,before_used,before_received;unsigned short old_tick;
  source_bytes[4]^=1;bh_actor_launch_receive(&launch,source_bytes,25,28);
  before_good=good;before_used=pools[0].used;before_received=launch.received;old_tick=bh_actor_route_now(route);memset(short_bytes,0,8);
  if(bh_actor_launch_receive(&launch,short_bytes,i*2,8)||good!=before_good||pools[0].used!=before_used||launch.received!=before_received||bh_actor_route_now(route)!=old_tick||launch.error!=2)++bounded_bad;
  source_bytes[4]^=1;
  if(pools[0].used){bh_entity_release(pools,pools[0].active);rooms[0].actors=0;vehicles[0].head=0;vehicles[0].count=0;}
 }
 printf("{\"receive_cases\":4096,\"actor_differences\":%lu,\"counter_cursor_differences\":%lu,\"tick_differences\":%lu,\"packet_differences\":%lu,\"rng_differences\":%lu,\"source_result_errors\":%lu,\"checksum_rejections\":%lu,\"index_rejections\":%lu,\"constructor_failures\":%lu,\"bounded_short_cases\":3,\"bounded_short_failures\":%lu}\n",actor_bad,counter_bad,tick_bad,packet_bad,rng_bad,source_bad,checksums,invalid_indices,constructor_failures,bounded_bad);
 for(side=0;side<2;++side)bh_entities_dispose(pools+side);bh_sound_queue_dispose(&sound);VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);
 return actor_bad||counter_bad||tick_bad||packet_bad||rng_bad||source_bad||bounded_bad?1:0;
}
