/* Complete ordinary query00435e80 lifecycle paths and original435720 release.
   Real source pool/query/sound owners are shared dependencies, never playback. */
#include "../src/bh_actor_damage.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHEntityPool pools[2];
static BHSoundQueue sound[2];
static BHActorRouteContext route[2];
static BHActorDamageContext damage[2];
static BHPickupRoom room[2];
static BHWorldGeometry world;
static unsigned char raw_world[0x340],*code;
static BHEntity *slots[2][8],*actors[2][4];
static BHEntityStep release_step;
static unsigned long dependency_errors,original_stops;
static void word(void *p,unsigned long o,unsigned long v){memcpy((char*)p+o,&v,4);}
static unsigned long value(const void *p,unsigned long o){unsigned long v;memcpy(&v,(const char*)p+o,4);return v;}
static void __cdecl oracle_unavailable(void){puts("Unexpected original damage lifecycle dependency");exit(3);}
static int __cdecl oracle_set_query(BHEntity *e,BHEntityCallback q){int r=bh_entity_set_query(pools+1,e,q);if(!r)++dependency_errors;return r;}
static int __cdecl oracle_sound_stop(unsigned long h){++original_stops;return bh_sound_stop(sound+1,h);}
static int __cdecl oracle_release(BHEntity *e){int r=bh_entity_release(pools+1,e);if(!r)++dependency_errors;return r;}
#include "../build/ai-damage-query-match/original-query.h"
static unsigned long canonical(unsigned long v){
 unsigned long i;
 for(i=0;i<8;++i)if(v==(unsigned long)slots[1][i])return(unsigned long)slots[0][i];
 if(v==(unsigned long)raw_world)return(unsigned long)&world;
 if(v==(unsigned long)(damage+1))return(unsigned long)damage;
 if(v==(unsigned long)(code+14336))return(unsigned long)release_step;
 return v;
}
static int same_actor(BHEntity *a,BHEntity *b){
 static const unsigned long offsets[]={0xc4,0xcc,0xd4,0xd8,0xdc,0xe0,0xec,0xf0,0xf4,0xf8,0x1a4,0x1a8,0x1ac,0x1b8,0x33c};
 BHEntity copy=*b;unsigned long j;
 for(j=0;j<sizeof(offsets)/sizeof(offsets[0]);++j)word(&copy,offsets[j],canonical(value(&copy,offsets[j])));
 return !memcmp(a,&copy,sizeof(copy));
}
static int same_pool(void){
 return pools[0].used==pools[1].used&&pools[0].focused==pools[1].focused&&
 canonical((unsigned long)pools[1].active)==(unsigned long)pools[0].active&&
 canonical((unsigned long)pools[1].free_slots)==(unsigned long)pools[0].free_slots&&
 canonical((unsigned long)pools[1].query_head)==(unsigned long)pools[0].query_head&&
 canonical((unsigned long)pools[1].focus)==(unsigned long)pools[0].focus;
}
int main(void){
 BHMathEnvironment env;BHSoundResource *resource[2];BHEntity *e,*actor[2];BHEntity blank;
 void(__cdecl *original)(BHEntity*,void*);unsigned long side,i,j,mode,shape,event,packet[10],reference[10],handle[2];
 unsigned long actor_bad=0,list_bad=0,pool_bad=0,sound_bad=0,error=0,live_sounds=0,stale_sounds=0,query_bad=0,deferred_releases=0;
 memset(&world,0,sizeof(world));memset(raw_world,0,sizeof(raw_world));
 if(!bh_math_environment_enter(&env))return 2;
 code=VirtualAlloc(0,16384,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 2;
 memset(code,0xcc,16384);install_original_query(code);FlushInstructionCache(GetCurrentProcess(),code,16384);original=(void(__cdecl *)(BHEntity*,void*))code;
 for(side=0;side<2;++side){
  if(!bh_entities_init(pools+side,8,0)||!bh_sound_queue_init(sound+side,0,0,0))return 2;
  resource[side]=bh_sound_resource_allocate(sound+side);if(!resource[side])return 2;resource[side]->streaming=1;resource[side]->base_frequency=44100;
  room[side].world=side?(BHWorldGeometry*)raw_world:&world;route[side].rooms=room+side;route[side].room_count=1;route[side].actor_limit=8;route[side].sound=sound+side;
  damage[side].pool=pools+side;damage[side].route=route+side;
  for(j=0;j<8;++j){
   slots[side][j]=bh_entity_acquire(pools+side);e=slots[side][j];if(!e)return 2;
   memset(&blank,0,sizeof(blank));blank.generation=e->generation;blank.previous=e->previous;blank.next=e->next;blank.lifetime=-1;blank.scale=1;*e=blank;
  }
  if(!bh_actor_damage_bind(damage+side,slots[side][0]))return 2;
  packet[0]=20;bh_actor_damage_query(slots[side][0],packet);
  if(side&&release_step!=slots[side][0]->step)return 2;release_step=slots[side][0]->step;
  for(j=8;j>0;--j)if(!bh_entity_release(pools+side,slots[side][j-1]))return 2;
 }
 for(mode=0;mode<4;++mode)for(i=0;i<2048;++i){
  shape=i%4;event=mode?20:4;
  for(side=0;side<2;++side){
   for(j=0;j<4;++j){
    e=actors[side][j]=bh_entity_acquire(pools+side);if(!e)return 2;
    memset(e->payload,0,sizeof(e->payload));
    if(!bh_actor_damage_bind(damage+side,e))return 2;
    word(e,0x1a4,(unsigned long)room[side].world);word(e,0xc4,(unsigned long)room[side].world);
    word(e,0xf4,0);word(e,0xf8,0);word(e,0x104,BH_SOUND_INVALID);
    e->step=bh_actor_damage_tick;e->refresh=bh_actor_damage_query;
   }
   if(shape==0){actor[side]=actors[side][3];room[side].actors=actor[side];}
   else{
    for(j=0;j<3;++j){word(actors[side][j],0xf8,j?(unsigned long)actors[side][j-1]:0);word(actors[side][j],0xf4,j<2?(unsigned long)actors[side][j+1]:0);}
    room[side].actors=actors[side][0];actor[side]=actors[side][shape==1?2:shape==2?0:1];
   }
   handle[side]=BH_SOUND_INVALID;
   if(i%4){handle[side]=bh_sound_request(sound+side,resource[side]->index,1,1,0,0);if(handle[side]==BH_SOUND_INVALID)return 2;if(i%4==3)bh_sound_stop(sound+side,handle[side]);}
   word(actor[side],0x104,handle[side]);
   damage[side].error=BH_ACTOR_ROUTE_OK;route[side].error=BH_ACTOR_ROUTE_OK;
   if(i&1)if(!bh_entities_focus(pools+side,actor[side]))return 2;
  }
  if(i%4==3)++stale_sounds;else if(i%4)++live_sounds;
  word(raw_world,0x128,(unsigned long)room[1].actors);memset(packet,0xa5,sizeof(packet));packet[0]=event;memcpy(reference,packet,sizeof(packet));
  original(actor[1],reference);bh_actor_damage_query(actor[0],packet);
  if(mode==3){original(actor[1],reference);bh_actor_damage_query(actor[0],packet);}
  if(mode){
   if(actor[0]->query||actor[1]->query||actor[0]->refresh||actor[1]->refresh||actor[0]->step!=release_step||actor[1]->step!=(BHEntityStep)(code+14336))++query_bad;
   if(mode>=2){actor[1]->step(actor[1]);actor[0]->step(actor[0]);++deferred_releases;}
  }
  for(j=0;j<8;++j)if(!same_actor(slots[0][j],slots[1][j])){++actor_bad;break;}
  if(canonical(value(raw_world,0x128))!=(unsigned long)room[0].actors)++list_bad;
  if(!same_pool()||!bh_entities_valid(pools)||!bh_entities_valid(pools+1))++pool_bad;
  if((bh_sound_voice(sound,handle[0])!=0)!=(bh_sound_voice(sound+1,handle[1])!=0)||sound[0].voice_count!=sound[1].voice_count)++sound_bad;
  if(memcmp(packet,reference,sizeof(packet))||damage[0].error||route[0].error)++error;
  for(side=0;side<2;++side){
   if(bh_sound_voice(sound+side,handle[side]))bh_sound_stop(sound+side,handle[side]);
   for(j=4;j>0;--j)if(bh_entity_is_active(pools+side,actors[side][j-1])&&!bh_entity_release(pools+side,actors[side][j-1]))return 2;
  }
 }
 printf("{\"scenario\":\"complete-damage-query-lifecycle\",\"cases\":8192,\"cases_each_direct_defer_release_double_defer\":2048,\"actor_differences\":%lu,\"list_differences\":%lu,\"pool_differences\":%lu,\"sound_differences\":%lu,\"query_differences\":%lu,\"source_errors\":%lu,\"dependency_errors\":%lu,\"live_sound_cases\":%lu,\"stale_sound_cases\":%lu,\"original_stop_requests\":%lu,\"deferred_release_calls\":%lu}\n",actor_bad,list_bad,pool_bad,sound_bad,query_bad,error,dependency_errors,live_sounds,stale_sounds,original_stops,deferred_releases);
 for(side=0;side<2;++side){bh_entities_dispose(pools+side);bh_sound_queue_dispose(sound+side);}VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);
 return actor_bad||list_bad||pool_bad||sound_bad||query_bad||error||dependency_errors;
}
