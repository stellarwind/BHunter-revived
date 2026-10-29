/* Complete original query 0043a1d0 and vehicle release 004397e0. Dependencies
   use real source pools, query dispatch and pool-only sound; no game executes. */
#include "../src/bh_actor_response.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BHEntityPool pools[2];
static BHSoundQueue sound[2];
static BHEntity *slots[2][8], *actors[2][4];
static BHActorResponseContext response[2];
static BHActorDamageContext damage[2];
static BHActorRouteContext route[2];
static BHActorAttackContext attack[2];
static BHActorVehicleLifetimeContext vehicles[2];
static BHPickupRoom rooms[2][2];
static BHWorldGeometry world, moved_world;
static unsigned char raw_world[0x340], raw_moved_world[0x340], *code;
static BHEntityStep source_release_step;
static unsigned long handle[2], notices[2], trace[2][8][8], dependency_errors;
static unsigned long relocate_requested,relocated[2],relocations[2];

static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static unsigned long value(const void *p,unsigned long o){unsigned long v;memcpy(&v,(const char*)p+o,4);return v;}
static void __cdecl oracle_unavailable(void){puts("Unexpected original lifecycle dependency");exit(3);}
static int __cdecl oracle_set_query(BHEntity *e,BHEntityCallback q){int result=bh_entity_set_query(pools+1,e,q);if(!result)++dependency_errors;return result;}
static int __cdecl oracle_sound_stop(unsigned long h){return bh_sound_stop(sound+1,h);}
static int __cdecl oracle_release(BHEntity *e){int result=bh_entity_release(pools+1,e);if(!result)++dependency_errors;return result;}
static int __cdecl oracle_broadcast(unsigned long mask,unsigned long v,const BHVec3 *p,float radius,void *packet){int result=bh_entities_query(pools+1,mask,v,p,radius,packet);if(!result)++dependency_errors;return result;}
#include "../build/ai-query-match/original-query.h"

static unsigned long canonical(unsigned long v){
 unsigned long i;
 for(i=0;i<8;++i)if(v==(unsigned long)slots[1][i])return(unsigned long)slots[0][i];
 if(v==(unsigned long)raw_world)return(unsigned long)&world;
 if(v==(unsigned long)raw_moved_world)return(unsigned long)&moved_world;
 if(v==(unsigned long)(response+1))return(unsigned long)response;
 if(v==(unsigned long)(code+14336))return(unsigned long)source_release_step;
 return v;
}
static unsigned long role(unsigned long side,BHEntity *e){unsigned long i;if(!e)return 0;for(i=0;i<4;++i)if(e==actors[side][i])return i+1;return 99;}
static void relocate_publisher(unsigned long side){
 BHEntity *e=actors[side][0],*previous=(BHEntity*)value(e,0xf8),*next=(BHEntity*)value(e,0xf4);
 if(previous)word(previous,0xf4,(unsigned long)next);else rooms[side][0].actors=next;
 if(next)word(next,0xf8,(unsigned long)previous);
 word(e,0xf4,0);word(e,0xf8,0);rooms[side][1].actors=e;
 word(e,0x1a4,(unsigned long)rooms[side][1].world);word(e,0xc4,(unsigned long)rooms[side][1].world);
 if(side){word(raw_world,0x128,(unsigned long)rooms[side][0].actors);word(raw_moved_world,0x128,(unsigned long)e);}
 relocated[side]=1;++relocations[side];
}
static void __cdecl observed_query(BHEntity *e,void *packet){
 unsigned long side,i,at,*p=(unsigned long*)packet,*row;
 for(side=0;side<2;++side)for(i=0;i<4;++i)if(e==actors[side][i])goto found;
 ++dependency_errors;return;
found:
 if(p[0]==17){
  at=notices[side]++;if(at>=8){++dependency_errors;return;}row=trace[side][at];
  row[0]=i;row[1]=role(side,(BHEntity*)p[1]);
  row[2]=role(side,side?(BHEntity*)value(code,12296):vehicles[0].head);
  row[3]=role(side,side?(BHEntity*)value(raw_world,0x128):rooms[0][0].actors);
  row[4]=side?value(code,12300):vehicles[0].count;
  row[5]=bh_sound_voice(sound+side,handle[side])!=0;
  row[6]=bh_entity_is_active(pools+side,(BHEntity*)p[1]);
  row[7]=((BHEntity*)p[1])->query!=0;
 }
 bh_actor_response_query(e,packet);
 if(p[0]==17&&relocate_requested&&!relocated[side])relocate_publisher(side);
}
static void links(unsigned long side,const unsigned long *order,int global){
 unsigned long i,previous=global?0xec:0xf8,next=global?0xf0:0xf4;
 for(i=0;i<3;++i){BHEntity *e=actors[side][order[i]];word(e,previous,i?(unsigned long)actors[side][order[i-1]]:0);word(e,next,i<2?(unsigned long)actors[side][order[i+1]]:0);}
 if(global)vehicles[side].head=actors[side][order[0]];
 else rooms[side][0].actors=actors[side][order[0]];
}
static int same_actor(BHEntity *a,BHEntity *b){
 static const unsigned long pointer_offsets[]={0xc4,0xcc,0xd4,0xd8,0xdc,0xe0,0xec,0xf0,0xf4,0xf8,0x1a4,0x1a8,0x1ac,0x1b8,0x330};
 BHEntity copy=*b;unsigned long i;
 for(i=0;i<sizeof(pointer_offsets)/sizeof(pointer_offsets[0]);++i)word(&copy,pointer_offsets[i],canonical(value(&copy,pointer_offsets[i])));
 return !memcmp(a,&copy,sizeof(copy));
}
int main(void){
 static const unsigned long permutations[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 BHMathEnvironment env;BHSoundResource *resource[2];BHActorRouteState state;
 void(__cdecl *original_query)(BHEntity*,void*);void(__cdecl *original_release)(BHEntity*);
 unsigned long mode,i,j,k,side,packet[10],reference[10],actor_bad=0,list_bad=0,pool_bad=0,sound_bad=0,notice_bad=0,errors=0;
 unsigned long notices_total=0,valid_sounds=0,source_active,ref_active;
 memset(&world,0,sizeof(world));memset(raw_world,0,sizeof(raw_world));
 if(!bh_math_environment_enter(&env))return 2;
 code=VirtualAlloc(0,16384,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memset(code,0xcc,16384);install_original_query(code);FlushInstructionCache(GetCurrentProcess(),code,16384);
 original_query=(void(__cdecl *)(BHEntity*,void*))code;original_release=(void(__cdecl *)(BHEntity*))(code+14336);
 for(side=0;side<2;++side){
  if(!bh_entities_init(pools+side,8,0)||!bh_sound_queue_init(sound+side,0,0,0))return 2;
  resource[side]=bh_sound_resource_allocate(sound+side);if(!resource[side])return 2;resource[side]->streaming=1;resource[side]->base_frequency=44100;
  rooms[side][0].world=side?(BHWorldGeometry*)raw_world:&world;
  rooms[side][1].world=side?(BHWorldGeometry*)raw_moved_world:&moved_world;
  route[side].rooms=rooms[side];route[side].room_count=2;route[side].actor_limit=8;route[side].sound=sound+side;
  damage[side].pool=pools+side;damage[side].route=route+side;
  vehicles[side].pool=pools+side;vehicles[side].route=route+side;
  response[side].damage=damage+side;response[side].attack=attack+side;response[side].vehicles=vehicles+side;
  for(i=0;i<8;++i){slots[side][i]=bh_entity_acquire(pools+side);if(!slots[side][i])return 2;}
  /* Capture the real source-private release callback, then retire both warm-up
     slots through the same real pool path so generation histories agree. */
  if(!bh_actor_response_bind(response+side,slots[side][0]))return 2;
  packet[0]=20;bh_actor_response_query(slots[side][0],packet);
  if(side&&source_release_step!=slots[side][0]->step)return 2;
  source_release_step=slots[side][0]->step;
  for(i=8;i>0;--i)if(!bh_entity_release(pools+side,slots[side][i-1]))return 2;
 }
 for(mode=0;mode<4;++mode)for(i=0;i<2048;++i){
  relocate_requested=mode==2&&i%7==0;relocated[0]=relocated[1]=0;
  memset(trace,0,sizeof(trace));notices[0]=notices[1]=0;
  memset(&state,0,sizeof(state));state.duration=1;state.speed=10;state.speed_scale=1;
  for(side=0;side<2;++side){
   rooms[side][1].actors=0;
   response[side].error=0;vehicles[side].count=3;
   handle[side]=(i&1)?bh_sound_request(sound+side,resource[side]->index,1,1,0,0):BH_SOUND_INVALID;
   if((i&1)&&handle[side]==BH_SOUND_INVALID)return 2;
   for(j=0;j<4;++j){
    BHEntity *e=actors[side][j]=bh_entity_acquire(pools+side);if(!e)return 2;
    memset(e->payload,0x5a,sizeof(e->payload));e->flags=0;e->kind=j;e->position.x=e->position.y=e->position.z=0;
    state.current=state.next=rooms[side][0].world;state.sound=j==0?handle[side]:BH_SOUND_INVALID;bh_actor_route_write(e,&state);
    word(e,0xc4,(unsigned long)rooms[side][0].world);word(e,0x1ac,0);word(e,0x1b8,0);
    e->step=bh_actor_vehicle_tick;e->refresh=bh_actor_response_query;
    if(!bh_actor_response_bind(response+side,e)||!bh_entity_set_query(pools+side,e,observed_query))return 2;
   }
   for(j=0;j<4;++j){word(actors[side][j],0x1ac,(unsigned long)actors[side][0]);word(actors[side][j],0x1b8,(unsigned long)actors[side][i%3]);}
   links(side,permutations[i%6],0);links(side,permutations[(i/6)%6],1);
   if(!bh_entities_focus(pools+side,i%3==0?actors[side][0]:i%3==1?actors[side][2]:0))return 2;
  }
  word(raw_world,0x128,(unsigned long)rooms[1][0].actors);word(code,12296,(unsigned long)vehicles[1].head);word(code,12300,3);
  word(raw_moved_world,0x128,0);
  memset(packet,0,sizeof(packet));packet[0]=mode==0?4:20;memcpy(reference,packet,sizeof(packet));
  if(mode==2){original_release(actors[1][0]);if(!bh_actor_vehicle_release(vehicles,actors[0][0]))++errors;}
  else{
   original_query(actors[1][0],reference);bh_actor_response_query(actors[0][0],packet);
   if(mode==1){actors[1][0]->step(actors[1][0]);actors[0][0]->step(actors[0][0]);}
  }
  if(response[0].error||response[1].error||memcmp(packet,reference,sizeof(packet)))++errors;
  if(canonical(value(code,12296))!=(unsigned long)vehicles[0].head||value(code,12300)!=vehicles[0].count||canonical(value(raw_world,0x128))!=(unsigned long)rooms[0][0].actors)++list_bad;
  if(canonical(value(raw_moved_world,0x128))!=(unsigned long)rooms[0][1].actors)++list_bad;
  if(notices[0]!=notices[1]||memcmp(trace[0],trace[1],sizeof(trace[0])))++notice_bad;
  notices_total+=notices[0];if(i&1)++valid_sounds;
  if((bh_sound_voice(sound,handle[0])!=0)!=(bh_sound_voice(sound+1,handle[1])!=0)||sound[0].error!=sound[1].error||sound[0].voice_count!=sound[1].voice_count)++sound_bad;
  for(j=0;j<4;++j){
   if(!same_actor(actors[0][j],actors[1][j])){if(!actor_bad)printf("first_actor_difference mode=%lu case=%lu actor=%lu\n",mode,i,j);++actor_bad;}
   source_active=bh_entity_is_active(pools,actors[0][j]);ref_active=bh_entity_is_active(pools+1,actors[1][j]);if(source_active!=ref_active)++pool_bad;
  }
  if(pools[0].used!=pools[1].used||pools[0].focused!=pools[1].focused||canonical((unsigned long)pools[1].focus)!=(unsigned long)pools[0].focus||canonical((unsigned long)pools[1].active)!=(unsigned long)pools[0].active||canonical((unsigned long)pools[1].free_slots)!=(unsigned long)pools[0].free_slots||canonical((unsigned long)pools[1].query_head)!=(unsigned long)pools[0].query_head||!bh_entities_valid(pools)||!bh_entities_valid(pools+1))++pool_bad;
  for(side=0;side<2;++side){
   if(bh_sound_voice(sound+side,handle[side]))bh_sound_stop(sound+side,handle[side]);
   for(j=0;j<4;++j)if(bh_entity_is_active(pools+side,actors[side][j])&&!bh_entity_release(pools+side,actors[side][j]))return 2;
  }
 }
 printf("{\"lifecycle_cases\":8192,\"cases_each_immediate_deferred_release_defer_only\":2048,\"actor_differences\":%lu,\"list_differences\":%lu,\"pool_differences\":%lu,\"sound_differences\":%lu,\"notification_differences\":%lu,\"source_errors\":%lu,\"dependency_errors\":%lu,\"notifications_checked\":%lu,\"valid_sound_cases\":%lu}\n",actor_bad,list_bad,pool_bad,sound_bad,notice_bad,errors,dependency_errors,notices_total,valid_sounds);
 printf("{\"notification_room_relocations_source_original\":[%lu,%lu]}\n",relocations[0],relocations[1]);
 for(side=0;side<2;++side){bh_entities_dispose(pools+side);bh_sound_queue_dispose(sound+side);}
 VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);
 return actor_bad||list_bad||pool_bad||sound_bad||notice_bad||errors||dependency_errors?1:0;
}
