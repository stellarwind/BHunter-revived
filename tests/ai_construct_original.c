/* Complete constructors with explicit real pool/query/locate dependencies.
   Runtime resource tables are fixture inputs, not fabricated game definitions. */
#include "../src/bh_actor_construct.h"
#include "../src/bh_route_query.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHEntityPool *oracle_pool;
static BHEntity *oracle_entity;
static BHWorldGeometry *oracle_world;
static void *oracle_world_image;
static BHEntityStep oracle_route_step,oracle_draw_step;
static unsigned long locate_calls;
static void __cdecl oracle_unavailable(void){puts("Excluded constructor fatal path executed");exit(3);}
static BHEntity *__cdecl oracle_allocate(void){oracle_entity=bh_entity_acquire(oracle_pool);return oracle_entity;}
static void __cdecl oracle_set_query(BHEntity *e,BHEntityCallback callback){if(!bh_entity_set_query(oracle_pool,e,callback)){puts("Real query binding failed");exit(4);}}
static void *__cdecl oracle_locate(void *hint,const BHVec3 *p,float margin){BHWorldGeometry *found;++locate_calls;if(hint)oracle_unavailable();found=bh_world_locate(oracle_world,1,0,p,margin);if(found&&found!=oracle_world)oracle_unavailable();return found?oracle_world_image:0;}
#include "../build/ai-construct-match/original-construct.h"
static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static unsigned long value(void *p,unsigned long o){unsigned long v;memcpy(&v,(char*)p+o,4);return v;}
int main(void){
 BHMathEnvironment env;BHEntityPool source_pool,reference_pool;BHSoundQueue sound;
 BHActorConstructContext c;BHActorConstructRoom binding;BHPickupRoom room;BHWorldGeometry world;
 BHActorResponseContext response;BHActorDamageContext damage;BHActorRouteContext route;
 BHActorAttackContext attack;BHActorVehicleLifetimeContext vehicles;BHActorVehicleDrawContext draw;
 BHEffectProjectileContext effects;BHHomingProjectileContext homing;
 BHEntity *source,*player,*source_head,*reference_head,ref;unsigned char world_image[0x240],*code;
 unsigned long models[25],sounds[25],cycle[4],special=400,type,type_index,i,j,mode,phase,seed,expected,bad[2]={0,0},rng_bad=0,list_bad=0,lookup_bad=0,query_bad=0;
 static const unsigned long speed_words[]={0,0x80000000UL,1,0x80000001UL,0x007fffffUL,0x807fffffUL,
  0x00800000UL,0x80800000UL,0x3f7fffffUL,0x3f800000UL,0xbf800000UL,0x419fffffUL,0x41a00000UL,
  0x41a00001UL,0xc1a00000UL,0x46fffffeUL,0x4affffffUL,0x4b000001UL,0x4effffffUL,0x4f000000UL,
  0xcf000000UL,0x5effffffUL,0x5f000000UL,0xdf000000UL,0x7f7fffffUL,0xff7fffffUL,0x41200000UL,
  0xc1200000UL,0x3eaaaaabUL,0xbeaaaaabUL,0x43000000UL,0xc3000000UL};
 long token;float speed;BHVec3 point={0,0,0};
 void (__cdecl *original_room)(void*,unsigned long,long,float,unsigned long,unsigned long);
 void (__cdecl *original_position)(const BHVec3*,unsigned long,long,float,unsigned long,unsigned long);
 memset(&c,0,sizeof(c));memset(&binding,0,sizeof(binding));memset(&room,0,sizeof(room));memset(&world,0,sizeof(world));memset(&response,0,sizeof(response));memset(&damage,0,sizeof(damage));memset(&route,0,sizeof(route));memset(&attack,0,sizeof(attack));memset(&vehicles,0,sizeof(vehicles));memset(&draw,0,sizeof(draw));memset(&effects,0,sizeof(effects));memset(&homing,0,sizeof(homing));memset(world_image,0,sizeof(world_image));
 if(!bh_math_environment_enter(&env)||!bh_entities_init(&source_pool,4,0)||!bh_entities_init(&reference_pool,4,0)||!bh_sound_queue_init(&sound,0,0,0))return 2;
 player=bh_entity_acquire(&source_pool);if(!player)return 2;
 source_head=bh_entity_acquire(&source_pool);reference_head=bh_entity_acquire(&reference_pool);
 if(!source_head||!reference_head)return 2;
 route.tick=0xfff9;route.actor_limit=16;route.rooms=&room;route.room_count=1;route.sound=&sound;room.world=&world;
 world.minimum.x=-10;world.minimum.y=-20;world.minimum.z=-30;world.maximum.x=30;world.maximum.y=40;world.maximum.z=50;world.navigation.next=&world.navigation;
 memcpy(world_image+0x1b0,&world.minimum,12);memcpy(world_image+0x1bc,&world.maximum,12);word(world_image,0x11c,902);
 damage.pool=&source_pool;damage.route=&route;vehicles.pool=&source_pool;vehicles.route=&route;homing.pool=&source_pool;effects.effects=&homing;attack.effects=&effects;
 response.damage=&damage;response.attack=&attack;response.vehicles=&vehicles;response.player=player;
 binding.room=&room;binding.bound_resource=902;c.response=&response;c.draw=&draw;c.rooms=&binding;c.room_count=1;c.resource_count=25;c.models=models;c.sounds=sounds;c.cycle_models=cycle;c.cycle_count=4;c.special_model=special;c.first=&world;c.world_count=1;
 for(i=0;i<25;++i){models[i]=100+i;sounds[i]=200+i;}for(i=0;i<4;++i)cycle[i]=300+i;
 source=bh_actor_construct_room(&c,&world,0,1,25,77,1);if(!source||c.error)return 2;
 oracle_route_step=source->step;memcpy(&oracle_draw_step,source->payload+4,4);bh_entity_release(&source_pool,source);room.actors=0;
 oracle_pool=&reference_pool;oracle_world=&world;oracle_world_image=world_image;
 code=VirtualAlloc(0,32768,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;memset(code,0xcc,32768);install_original_construct(code);
 memcpy(code+20500,&route.tick,2);memcpy(code+20504,&player,4);memcpy(code+24576,models,sizeof(models));memcpy(code+24704,sounds,sizeof(sounds));memcpy(code+24832,cycle,sizeof(cycle));memcpy(code+24848,&special,4);
 FlushInstructionCache(GetCurrentProcess(),code,32768);original_room=(void (__cdecl *)(void*,unsigned long,long,float,unsigned long,unsigned long))code;original_position=(void (__cdecl *)(const BHVec3*,unsigned long,long,float,unsigned long,unsigned long))(code+4096);
 for(mode=0;mode<2;++mode)for(phase=0;phase<2;++phase)for(type_index=0;type_index<18;++type_index)for(i=0;i<256;++i){
  type=type_index==17?0xffffffffUL:type_index;
  seed=i*0x9e3779b9UL+type;speed=i%4==0?0:i%4==1?19.5f:i%4==2?25:50;token=i&1?123:0;
  if(phase){memcpy(&speed,&speed_words[i%32],4);seed^=0xa563198dUL;route.tick=(unsigned short)(i*257UL+type);}
  else route.tick=0xfff9;
  memcpy(code+20500,&route.tick,2);
  c.cycle=i&3;memcpy(code+20516,&c.cycle,4);vehicles.count=i&7;
  room.actors=(i&4)?source_head:0;vehicles.head=(i&8)?source_head:0;
  word(code,20508,(i&8)?(unsigned long)reference_head:0);word(code,20512,vehicles.count);
  word(world_image,0x128,(i&4)?(unsigned long)reference_head:0);
  word(source_head,0xec,0);word(source_head,0xf8,0);word(reference_head,0xec,0);word(reference_head,0xf8,0);
  /* Pure query-list registration only; no callback is dispatched by this fixture. */
  if(!bh_entity_set_query(&source_pool,source_head,(i&16)?bh_actor_damage_query:0)||
     !bh_entity_set_query(&reference_pool,reference_head,(i&16)?bh_actor_damage_query:0))return 2;
  oracle_entity=0;locate_calls=0;
  memset(source_pool.free_slots->payload,0xa5,sizeof(source_pool.free_slots->payload));memset(reference_pool.free_slots->payload,0xa5,sizeof(reference_pool.free_slots->payload));
  point.x=(i&1)?30.5f:0;
  srand(seed);expected=rand();srand(seed);
  source=mode?bh_actor_construct_position(&c,&point,type,token,speed,77,seed):bh_actor_construct_room(&c,&world,type,token,speed,77,seed);
  if(!source||c.error){puts("Source constructor rejected valid fixture");return 2;}if((unsigned long)rand()!=expected)++rng_bad;
  srand(seed);if(mode)original_position(&point,type,token,speed,77,seed);else original_room(world_image,type,token,speed,77,seed);
  if(!oracle_entity){puts("Original constructor failed allocation");return 2;}if((unsigned long)rand()!=expected)++rng_bad;
  ref=*oracle_entity;word(&ref,0xc4,(unsigned long)&world);word(&ref,0x1a4,(unsigned long)&world);word(&ref,0x1a8,(unsigned long)&world);
  if(value(&ref,0xf0)==(unsigned long)reference_head)word(&ref,0xf0,(unsigned long)source_head);
  if(value(&ref,0xf4)==(unsigned long)reference_head)word(&ref,0xf4,(unsigned long)source_head);
  if(source->flags!=ref.flags||memcmp(&source->position,&ref.position,12)||memcmp((char*)source+0x3c,(char*)&ref+0x3c,12)||memcmp((char*)source+0xc4,(char*)&ref+0xc4,16)||memcmp((char*)source+0xe4,(char*)&ref+0xe4,0x108)){
   if(bad[mode]<4){for(j=0xe4;j<0x1ec;++j)if(((unsigned char*)source)[j]!=((unsigned char*)&ref)[j])break;printf("construct mode=%lu type=%lu seed=%lu offset=%lx\n",mode,type,seed,j);}++bad[mode];
  }
  if(room.actors!=source||value(world_image,0x128)!=(unsigned long)oracle_entity||vehicles.count!=value(code,20512)||(vehicles.head==source)!=(value(code,20508)==(unsigned long)oracle_entity)||c.cycle!=value(code,20516))++list_bad;
  if((value(source_head,0xec)==(unsigned long)source)!=(value(reference_head,0xec)==(unsigned long)oracle_entity)||
     (value(source_head,0xf8)==(unsigned long)source)!=(value(reference_head,0xf8)==(unsigned long)oracle_entity))++list_bad;
  if(source_pool.query_head!=source||reference_pool.query_head!=oracle_entity||source->query_previous||oracle_entity->query_previous||
     source->query_next!=((i&16)?source_head:0)||oracle_entity->query_next!=((i&16)?reference_head:0)||
     source_head->query_previous!=((i&16)?source:0)||reference_head->query_previous!=((i&16)?oracle_entity:0))++query_bad;
  if(locate_calls!=(mode?((i&1)?2UL:1UL):0UL))++lookup_bad;
  if(!bh_entity_release(&source_pool,source)||!bh_entity_release(&reference_pool,oracle_entity))return 2;
 }
 printf("{\"constructor_cases_each\":9216,\"room_differences\":%lu,\"position_differences\":%lu,\"rng_differences\":%lu,\"list_cycle_differences\":%lu,\"lookup_count_differences\":%lu,\"query_link_differences\":%lu,\"finite_speed_patterns\":32,\"nonempty_heads_and_counts\":true,\"clock_wraparound\":true,\"precision\":\"PC53\"}\n",bad[0],bad[1],rng_bad,list_bad,lookup_bad,query_bad);
 VirtualFree(code,0,MEM_RELEASE);bh_sound_queue_dispose(&sound);bh_entities_dispose(&reference_pool);bh_entities_dispose(&source_pool);bh_math_environment_leave(&env);return bad[0]||bad[1]||rng_bad||list_bad||lookup_bad||query_bad?1:0;
}
