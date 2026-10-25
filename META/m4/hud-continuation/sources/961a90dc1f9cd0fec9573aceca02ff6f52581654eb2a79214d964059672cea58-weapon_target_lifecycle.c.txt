#include "../src/bh_weapon_target_lifecycle.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include "weapon_target_lifecycle_cases.h"
static unsigned long checks,failures,queries,events;static BHEntity *replace_next,*mutate_player,*mutate_weapon;static int mutate;static long replace_charge;
#define CHECK(v,s) do{++checks;if(!(v)){++failures;printf("FAIL weapon-target-lifecycle: %s\n",s);}}while(0)
static BHEntity *allocation_weapon,*allocation_player;static int allocation_armed,allocation_seen;
static void *__cdecl allocate(unsigned long bytes,void *user){
 BHWeaponCommonState *s;if(allocation_armed){allocation_armed=0;s=bh_weapon_common_state(allocation_weapon);
  allocation_seen=!s->word_e8&&!s->word_f0&&!s->word_f4&&!s->hud.target&&!s->hud.charge&&s->word_100==-1;
  allocation_player->position.x=31;allocation_player->origin.z=-61;
 }
 return malloc(bytes);
}
static void __cdecl release(void *p,void *user){free(p);}
static void next(BHEntity *e,BHEntity *n){memcpy(e->payload+12,&n,4);}
static float value(unsigned long word){float v;memcpy(&v,&word,4);return v;}
static void set(unsigned long *word,float v){memcpy(word,&v,4);}
/* Isolated fixture receiver; never installed by production lifecycle owner. */
static void __cdecl position(BHEntity *e,void *raw){
 BHEntityPositionPacket *p=(BHEntityPositionPacket*)raw;++queries;if(p->event==99)++events;p->event=99;p->position=e->position;
 if(mutate){mutate=0;next(e,replace_next);if(mutate_player)mutate_player->position.x=10;if(mutate_weapon)bh_weapon_common_state(mutate_weapon)->hud.charge=replace_charge;}
}
typedef struct Fixture {BHEntity weapon,player,actors[8];BHWorldGeometry world;BHWorldSegmentRoom room;BHWorldSegmentContext query;BHWeaponTargetContext context;} Fixture;
static void setup(Fixture *f,BHSoundQueue *sound,unsigned long ready,unsigned long beep,unsigned long locked){
 BHWeaponCommonState *s;unsigned long i;memset(f,0,sizeof(*f));s=bh_weapon_common_state(&f->weapon);s->ammo=1;s->word_100=-1;
 bh_weapon_target_player_aim(&f->player)->x=1;
 f->world.navigation.next=&f->world.navigation;f->world.previous=&f->world;
 f->world.minimum.x=f->world.minimum.y=f->world.minimum.z=-1000;f->world.maximum.x=f->world.maximum.y=f->world.maximum.z=1000;
 f->world.transform.m[0][0]=f->world.transform.m[1][1]=f->world.transform.m[2][2]=1;f->world.inverse=f->world.transform;
 f->room.world=&f->world;
 CHECK(bh_world_segment_init(&f->query,&f->world,1,&f->room,1,NULL,0,16),"actual single-room collision owner");
 f->context.sound=sound;f->context.world=&f->query;f->context.ready_sound=ready;f->context.beep_sound=beep;f->context.lock_sound=locked;f->context.max_actors=16;
 for(i=0;i<8;++i)f->actors[i].query=position;
 queries=events=0;mutate=0;mutate_player=mutate_weapon=replace_next=NULL;
}
static void ready_cancel(BHSoundQueue *sound,unsigned long a,unsigned long b,unsigned long c){
 Fixture f;BHWeaponCommonState *s;unsigned long first,second,h;BHSoundVoice *voice;
 setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);memset(s,0xa5,sizeof(*s));s->hud.target=f.actors;s->word_100=-1;
 f.player.position.x=7;f.player.origin.z=9;srand(33);first=rand();second=rand();srand(33);
 allocation_weapon=&f.weapon;allocation_player=&f.player;allocation_armed=1;allocation_seen=0;
 CHECK(bh_weapon_target_ready(&f.context,&f.weapon,&f.player),"complete target-specific ready reset and real sound request");
 CHECK(!s->word_e8&&!s->word_f0&&!s->word_f4&&!s->hud.target&&!s->hud.charge&&s->word_100==-1&&s->available_ec==0xa5a5a5a5UL,"ready exact reset fields preserve unrelated storage");
 CHECK(f.context.random_calls==1&&(unsigned long)rand()==second,"ready consumes one shared CRT value");
 voice=sound->resources[a]->voices[0];CHECK(voice&&voice->pending&&voice->position.x==31&&voice->velocity.z==-61,"actual sound allocation mutation is visible through live player vectors");
 CHECK(allocation_seen&&!allocation_armed,"original reset fields published before real sound allocation callback");
 h=voice->index+((unsigned long)voice->generation<<16);s->word_100=(long)h;s->hud.target=f.actors;s->hud.charge=48;s->word_f0=123;
 CHECK(bh_weapon_target_cancel(&f.context,&f.weapon)&&!s->hud.target&&!s->hud.charge&&!s->word_f0&&s->word_100==-1&&f.context.stops==1,"cancel clears target before real queued voice stop and invalidates handle");
 CHECK(bh_sound_stop(sound,h)==0,"cancel actually released native-backed pending voice");
 CHECK(bh_weapon_target_cancel(&f.context,&f.weapon)&&f.context.stops==1,"invalid sentinel suppresses stop");
}
static void acquire(BHSoundQueue *sound,unsigned long a,unsigned long b,unsigned long c){
 Fixture f;BHWeaponCommonState *s;unsigned long i;setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);
 f.context.actors=f.actors;for(i=0;i<5;++i)next(f.actors+i,i==4?NULL:f.actors+i+1);
 f.actors[0].position.x=-10;f.actors[1].position.x=150;f.actors[2].position.x=100;f.actors[2].position.y=30;f.actors[3].position.x=f.actors[4].position.x=50;
 s->hud.charge=99;set(&s->word_f0,.01f);set(&s->word_f4,-1);
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player),"acquire through real collision miss");
 CHECK(s->hud.target==f.actors+3&&!s->hud.charge&&queries==5&&events==4,"nearest strict tie retains first and query packet event reused");
 CHECK(f.context.traces==1&&s->word_104==(unsigned long)&f.world,"only eligible candidate reaches real world owner and live cache104");
 CHECK(value(s->word_f0)<0&&value(s->word_f4)==-1,"positive cooldown crosses zero without clamping, negative unchanged");
 setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);f.context.actors=f.actors;f.actors[0].position.x=100;f.actors[1].position.x=90;f.actors[2].position.x=50;
 next(f.actors,f.actors+1);replace_next=f.actors+2;mutate=1;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&queries==2&&s->hud.target==f.actors+2,"query mutation controls later f0 successor read");
 setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);f.context.actors=f.actors;f.actors[0].position.x=160;mutate=1;mutate_player=&f.player;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&!s->hud.target&&queries==1,"postquery player position reloaded before strict150 test");
 s->ammo=0;s->hud.target=f.actors;s->hud.charge=7;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&!s->hud.target&&s->hud.charge==7&&queries==1,"zero ammo clears target without querying or changing charge");
}
typedef struct Obstacle {BHBsp bsp;BHBspVertex vertices[4];BHBspEdge edges[4];BHBspFace face;BHBspNode node;BHBspLeaf leaf;BHBspMaterial material;unsigned char storage;BHGeometry geometry;BHSegmentQuery query;} Obstacle;
static int obstacle(Obstacle *m){
 unsigned long i;memset(m,0,sizeof(*m));m->bsp.vertices=m->vertices;m->bsp.edges=m->edges;m->bsp.faces=&m->face;m->bsp.nodes=&m->node;m->bsp.leaves=&m->leaf;m->bsp.materials=&m->material;m->bsp.storage=&m->storage;
 m->bsp.counts[0]=m->bsp.counts[3]=m->bsp.counts[4]=m->bsp.counts[5]=1;m->bsp.counts[1]=m->bsp.counts[2]=4;
 for(i=0;i<4;++i){m->vertices[i].y=(i==0||i==3)?-100:100;m->vertices[i].z=i<2?-100:100;m->edges[i].vertex=i;m->edges[i].next=i<3?i+1:-1;}
 m->node.normal[0]=1;m->node.front_node=m->node.back_node=-1;m->node.front_leaf=0;m->node.back_leaf=-1;m->leaf.face=0;m->leaf.next=-1;
 return bh_geometry_init(&m->geometry,&m->bsp,NULL)&&bh_segment_query_init(&m->query,&m->geometry);
}
static void occlusion(BHSoundQueue *sound,unsigned long a,unsigned long b,unsigned long c){
 Fixture f;BHWeaponCommonState *s;Obstacle m;setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);CHECK(obstacle(&m),"real source BSP polygon geometry and narrow phase");
 f.room.query=&m.query;f.world.query_flags=2;f.world.transform.m[0][3]=10;f.world.inverse.m[0][3]=-10;
 f.context.actors=f.actors;f.actors[0].position.x=20;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&!s->hud.target&&f.context.traces==1,"real BSP obstruction blocks eligible acquisition");
 f.world.query_flags=0;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors&&f.context.traces==2,"disabled actual wall permits same candidate");
 f.world.query_flags=2;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors&&f.context.traces==2,"existing target maintenance intentionally skips new occlusion test");
 bh_segment_query_dispose(&m.query);bh_geometry_dispose(&m.geometry);
}
static void zero_distance(BHSoundQueue *sound,unsigned long a,unsigned long b,unsigned long c){
 Fixture f;BHWeaponCommonState *s;setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);f.context.actors=f.actors;
 _clear87();CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors&&f.context.traces==1,"unordered acquire angle admits coincident finite candidate through actual world miss");
 CHECK((_status87()&_SW_INVALID)!=0,"acquisition retains actual masked0over0 invalid sticky");_clear87();
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors&&s->hud.charge==1,"unordered maintain angle retains coincident target before lock");
 CHECK((_status87()&_SW_INVALID)!=0,"maintenance retains actual masked0over0 invalid sticky");_clear87();
}
static void maintain(BHSoundQueue *sound,unsigned long a,unsigned long b,unsigned long c){
 Fixture f;BHWeaponCommonState *s;unsigned long i,before;static const long charges[]={15,16,17,18,46,47,48,LONG_MAX,LONG_MIN};
 for(i=0;i<sizeof(charges)/sizeof(charges[0]);++i){
  setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);s->hud.target=f.actors;f.actors[0].position.x=100;s->hud.charge=charges[i];before=f.context.sound_requests;
  CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors&&s->hud.charge==(long)((unsigned long)charges[i]+1),"live target charge increment including signed wrap");
  CHECK(queries==1&&f.context.traces==0,"maintaining target queries once without reacquisition world test");
  if(charges[i]==17||charges[i]==18||charges[i]==46)CHECK(f.context.sound_requests==before+1,"bit2 beep inside signed17..47 interval");
  else if(charges[i]==47)CHECK(f.context.sound_requests==before+1&&s->word_100!=-1,"exact48 stores real lock sound handle");
  else CHECK(f.context.sound_requests==before,"other charge values suppress sound");
  if(s->word_100!=-1)CHECK(bh_weapon_target_cancel(&f.context,&f.weapon),"stop actual acquired lock voice");
 }
 setup(&f,sound,a,b,c);s=bh_weapon_common_state(&f.weapon);s->hud.target=f.actors;s->hud.charge=0;f.actors[0].position.x=100;f.actors[0].position.y=30;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&!s->hud.target,"unlocked target loses outside tight angle");
 s->hud.target=f.actors;s->hud.charge=47;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors&&s->hud.charge==48,"locked target retains wider cone");
 f.actors[0].position.y=50;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&!s->hud.target&&s->word_100==-1&&f.context.stops==1,"wide cone loss clears and stops actual lock sound");
 s->hud.target=f.actors;s->hud.charge=0;f.actors[0].position.y=30;mutate=1;mutate_weapon=&f.weapon;replace_charge=48;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors&&s->hud.charge==48,"query-mutated live charge controls postquery angle retention");
 f.actors[0].position.x=150;f.actors[0].position.y=0;
 CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&s->hud.target==f.actors,"existing target admits exact150 unlike acquisition");
 f.actors[0].position.x=150.1f;CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&!s->hud.target,"existing target rejects beyond150");
}
static void precision(BHSoundQueue *sound,unsigned long a,unsigned long b,unsigned long c){
 Fixture f;BHWeaponCommonState *state;unsigned long i;const TargetPrecisionCase *p;
 for(i=0;i<sizeof(target_precision_cases)/sizeof(target_precision_cases[0]);++i){
  setup(&f,sound,a,b,c);state=bh_weapon_common_state(&f.weapon);p=target_precision_cases+i;
  memcpy(&f.player.position,p->player,12);memcpy(&f.actors[0].position,p->target,12);state->hud.target=f.actors;state->hud.charge=p->charge;
  CHECK(bh_weapon_target_update(&f.context,&f.weapon,&f.player)&&state->hud.target==f.actors,"independent retained raw/stored norm fixture preserves tight or locked boundary; rounded-delta shortcut would lose");
 }
}
int __cdecl bh_weapon_target_lifecycle_checkpoint(unsigned long *count){
 BHMathEnvironment fp;BHSoundQueue sound;LPDIRECTSOUND device=NULL;BHWave wave;unsigned char samples[64];unsigned long ids[3],i;BHSceneAllocator allocator;
 allocator.allocate=allocate;allocator.release=release;allocator.user=NULL;allocation_armed=0;
 checks=failures=0;memset(&sound,0,sizeof(sound));memset(&wave,0,sizeof(wave));
 if(!bh_math_environment_enter(&fp))return 2;
 CHECK(DirectSoundCreate(NULL,&device,NULL)==DS_OK&&device,"actual DirectSound device without playback");if(!device)goto done;
 CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK&&bh_sound_queue_init(&sound,device,NULL,&allocator),"actual sound queue owner");
 memset(samples,0,sizeof(samples));wave.format_tag=1;wave.channels=1;wave.sample_rate=8000;wave.byte_rate=16000;wave.block_align=2;wave.bits=16;wave.sample_bytes=64;wave.samples=samples;
 for(i=0;i<3;++i){ids[i]=bh_sound_register_wave(&sound,&wave,0,1,0);CHECK(ids[i]!=BH_SOUND_INVALID,"real PCM resource without Play");if(ids[i]==BH_SOUND_INVALID)goto done;}
 ready_cancel(&sound,ids[0],ids[1],ids[2]);acquire(&sound,ids[0],ids[1],ids[2]);occlusion(&sound,ids[0],ids[1],ids[2]);zero_distance(&sound,ids[0],ids[1],ids[2]);maintain(&sound,ids[0],ids[1],ids[2]);precision(&sound,ids[0],ids[1],ids[2]);
done:
 bh_sound_queue_dispose(&sound);if(device)IDirectSound_Release(device);CHECK(bh_math_environment_supported(),"PC53 retained");bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"weapon-target-lifecycle\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;
}
#ifdef BH_WEAPON_TARGET_LIFECYCLE_STANDALONE
int main(void){unsigned long n;return bh_weapon_target_lifecycle_checkpoint(&n);}
#endif
