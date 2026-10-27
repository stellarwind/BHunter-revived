#include "../src/bh_weapon_fire.h"
#include "../src/bh_message_panel.h"
#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-fire %lu: %s\n",checks,s);}}while(0)
static float number(const BHEntity *e,unsigned long o){float v;memcpy(&v,(const unsigned char*)e+o,4);return v;}
static unsigned long word(const BHEntity *e,unsigned long o){unsigned long v;memcpy(&v,(const unsigned char*)e+o,4);return v;}
static void put(BHEntity *e,unsigned long o,unsigned long v){memcpy((unsigned char*)e+o,&v,4);}
static float data(const BHDrawCommand *d,unsigned int i){float f;memcpy(&f,d->words+i,4);return f;}
static int equal(float a,double b){return fabs((double)a-b)<.00001;}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static void launch_original_cases(BHWeaponFireContext *fire,BHEntity *weapon,BHEntity *source,BHEntityPool *pool){
 static const struct {unsigned long input[18],output[6];} cases[]={
#include "ballistic_launch_golden.h"
 };
 unsigned long offsets[]={12,36,48,24,72,640},i,j;BHEntity *shot;BHWeaponCommonState *state=bh_weapon_common_state(weapon);
 for(i=0;i<128;++i){
  for(j=0;j<6;++j)memcpy((char*)source+offsets[j],cases[i].input+j*3,12);
  state->ammo=1;state->word_e8=(i&1)*2;srand(i+17);
  CHECK(bh_weapon_fire_ballistic(fire,weapon,source),"original arithmetic case creates connected projectile");shot=pool->active;
  CHECK(!memcmp(&shot->position,cases[i].output,12),"muzzle words match original retained x87 arithmetic");
  CHECK(!memcmp(&shot->origin,cases[i].output+3,12),"velocity words match original X retention and Y/Z spills");
  CHECK(bh_entity_release(pool,shot),"release checked projectile");
 }
}
int __cdecl bh_weapon_fire_checkpoint(unsigned long *count){BHEntityPool pool;BHSoundQueue sound;BHDrawQueue queue;BHProjectileLifetime lifetime;BHProjectileDrawContext draw;BHProjectileCreateContext create;BHBallisticProjectileContext ballistic;BHHomingProjectileContext homing;BHScriptParticleContext particles;BHContactEffects contacts;BHFlashFlareContext flash;BHHitMarkerContext markers;BHWorldGeometry world;BHWorldSegmentRoom room;BHWorldSegmentContext segment;BHMathEnvironment fp;
 BHEntity *source,*target,*e,*recycled;BHVec3 p={3,6,9},v={30,60,90};BHSoundResource *resource;BHSoundVoice *voice;BHDrawCommand *d;unsigned long next=77,before,generation,handle;unsigned int i;int expected,actual;long packet[2];BHWeaponFireContext fire;BHAreaProjectileContext area;BHNetworkTransport transport;BHMessageState messages;BHMessagePanel panel;BHTextQueue text;BHAtlasPage page;BHWeaponCommonState *state;BHEntity *weapon,*shot;unsigned long warned=0;BHVec3 basis={1,0,0};
 memset(&ballistic,0,sizeof(ballistic));memset(&homing,0,sizeof(homing));memset(&draw,0,sizeof(draw));memset(&create,0,sizeof(create));memset(&world,0,sizeof(world));memset(&room,0,sizeof(room));memset(&lifetime,0,sizeof(lifetime));memset(&flash,0,sizeof(flash));memset(&markers,0,sizeof(markers));
 CHECK(bh_math_environment_enter(&fp),"PC53");CHECK(bh_entities_init(&pool,20,NULL),"actual pool");CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"actual source sound");CHECK(bh_draw_queue_init(&queue,40,NULL),"actual draw queue");
 source=bh_entity_acquire(&pool);target=bh_entity_acquire(&pool);target->position.x=100;target->position.y=200;target->position.z=300;CHECK(bh_entity_set_query(&pool,target,bh_marker_query),"actual target query");
 world.navigation.next=&world.navigation;world.previous=&world;world.minimum.x=world.minimum.y=world.minimum.z=-1000;world.maximum.x=world.maximum.y=world.maximum.z=1000;world.query_flags=2;identity(&world.transform);identity(&world.inverse);room.world=&world;room.kind=1;
 CHECK(bh_world_segment_init(&segment,&world,1,&room,1,NULL,0,20),"actual empty world segment owner");CHECK(bh_script_particles_init(&particles,&pool,&queue)&&bh_contact_effects_init(&contacts,&pool,&queue,1,2,3,4),"real effect owners");flash.pool=&pool;flash.queue=&queue;markers.pool=&pool;markers.queue=&queue;
 homing.pool=&pool;homing.sound=&sound;homing.segment=&segment;homing.particles=&particles;homing.contacts=&contacts;homing.flash=&flash;homing.markers=&markers;ballistic.bindings=homing;lifetime.pool=&pool;lifetime.sound=&sound;
 for(i=0;i<2;i++){resource=bh_sound_resource_allocate(&sound);CHECK(resource!=NULL,"real source resource");resource->streaming=1;resource->base_frequency=8000;}
 draw.queue=&queue;draw.ballistic_flare=31;draw.ballistic_trail=32;draw.homing_flare=33;draw.homing_trail=34;draw.pulse_image=35;create.lifetime=&lifetime;create.ballistic=&ballistic;create.homing=&homing;create.draw=&draw;create.next_identity=&next;create.loop_sound=0;create.launch_sound=1;
 memset(&fire,0,sizeof(fire));memset(&area,0,sizeof(area));memset(&panel,0,sizeof(panel));memset(&page,0,sizeof(page));CHECK(bh_network_transport_init(&transport,NULL,0),"actual NULL-peer transport");bh_message_state_init(&messages);bh_text_queue_init(&text);panel.pool=&pool;panel.messages=&messages;panel.text=&text;panel.sound=&sound;panel.draw=&queue;panel.page=&page;panel.screen_width=640;CHECK(bh_message_panel_create(&panel)!=NULL,"actual message panel lifecycle");
 area.effects=&homing;area.draw=&draw;area.identity=&next;area.loop_sound=0;area.hit_sound=1;fire.projectiles=&create;fire.area=&area;fire.sound=&sound;fire.transport=&transport;fire.messages=&messages;fire.empty_warning=&warned;fire.local_id=3;fire.sender=29;fire.peers=2;fire.ballistic_sound=fire.homing_sound=fire.empty_sound=fire.area_sound=0;weapon=bh_entity_acquire(&pool);state=bh_weapon_common_state(weapon);memset(state,0,sizeof(*state));state->ammo=4;
 memcpy((unsigned char*)source+0x18,&basis,12);basis.x=0;basis.y=1;memcpy((unsigned char*)source+0x24,&basis,12);basis.y=0;basis.z=1;memcpy((unsigned char*)source+0x30,&basis,12);basis.z=0;basis.x=1;memcpy((unsigned char*)source+0x280,&basis,12);memcpy((unsigned char*)source+0xc4,&room.world,4);
 srand(17);for(i=0;i<3;i++)rand();expected=rand();srand(17);before=pool.used;CHECK(bh_weapon_fire_ballistic(&fire,weapon,source)&&pool.used==before+1,"actual ballistic firing constructs real projectile");shot=pool.active;CHECK(state->ammo==3&&state->word_e8==1&&state->delay==8&&fire.shots==1,"original ammo phase and network delay");CHECK(rand()==expected,"ballistic firing consumes exactly three shared random draws");CHECK(shot->step==bh_ballistic_projectile_tick&&shot->position.x==-1&&shot->position.y==.5f&&shot->position.z==1.5f,"actual alternating muzzle basis and whole projectile callback");CHECK(fire.packet_bytes==13&&fire.packet[8]==3&&fire.packet[9]==4&&fire.packet[10]==2&&fire.packet[11]==1&&fire.packet[12]==255&&transport.native_calls==0,"actual bounded NULL-peer shot envelope");CHECK(number(source,0x2a4)==2.5f,"original directional plus side recoil");
 before=pool.used;CHECK(bh_weapon_fire_ballistic(&fire,weapon,source)&&pool.used==before&&state->word_e8==2,"odd phase advances without firing");CHECK(bh_weapon_fire_ballistic(&fire,weapon,source)&&pool.active->position.x==1&&fire.packets==1,"alternate muzzle and retained network throttle");state->word_e8=0;state->ammo=0;before=pool.used;CHECK(bh_weapon_fire_ballistic(&fire,weapon,source)&&state->word_e8==0&&pool.used==before,"empty ballistic gate preserves phase");
 state->ammo=2;state->hud.target=target;state->delay=0;fire.remote[2]=target;before=pool.used;CHECK(bh_weapon_fire_homing(&fire,weapon,source)&&pool.used==before+1,"actual homing constructor closure");shot=pool.active;CHECK(shot->step==bh_homing_projectile_tick&&word(shot,0x12c)==(unsigned long)target&&state->ammo==1&&state->word_e8==1,"actual target and phase");CHECK(fire.packet[9]==2&&fire.packet[10]==4&&fire.packet[11]==2&&fire.packet[12]==2,"actual eight-peer target resolution");CHECK(number(source,0x2a4)==2,"homing original four-unit recoil");before=pool.used;for(i=0;i<3;i++)CHECK(bh_weapon_fire_homing(&fire,weapon,source),"homing idle phases");CHECK(pool.used==before&&state->word_e8==4,"homing skips three phases");
 state->word_e8=0;state->ammo=0;CHECK(bh_weapon_area_start(&fire,weapon,source)&&warned==1&&messages.ring_count==1,"actual empty-ammo cue and actual panel-bound message owner");CHECK(strstr(messages.ring[0],"fresh out of ammo")!=NULL,"exact original empty-ammo message");CHECK(bh_weapon_area_start(&fire,weapon,source)&&messages.ring_count==1&&fire.empty_cues==2,"one-time message while each attempt cues");state->ammo=2;CHECK(bh_weapon_area_start(&fire,weapon,source)&&state->ammo==1&&state->word_e8==5&&fire.packet[9]==5&&fire.packet[10]==1&&fire.packet[11]==4,"actual burst arm and envelope");CHECK(bh_weapon_area_start(&fire,weapon,source)&&state->ammo==1,"active burst rejects retrigger");
 for(i=5;i;i--){before=pool.used;CHECK(bh_weapon_area_tick(&fire,weapon,source)&&pool.used==before+1&&state->word_e8==i-1,"actual burst creates one area projectile per phase");shot=pool.active;CHECK(shot->step==bh_area_projectile_tick&&shot->origin.x==300,"real source area callback and heading scaling");CHECK(number(shot,0xe8)==(i==5||i==1?.5f:i==4||i==2?.75f:1.0f),"five-point original strength envelope");}
 CHECK(fire.bursts==5,"exact five area burst deliveries");before=pool.used;CHECK(bh_weapon_area_tick(&fire,weapon,source)&&pool.used==before,"zero burst state has no effects");launch_original_cases(&fire,weapon,source,&pool);CHECK(bh_message_panel_close(&panel)&&bh_network_transport_dispose(&transport),"real panel and transport lifetimes close");CHECK(bh_entities_dispose(&pool)&&bh_sound_queue_dispose(&sound)&&bh_draw_queue_dispose(&queue),"actual owners dispose");bh_math_environment_leave(&fp);if(count)*count+=checks;return !failures;
}
#ifdef BH_WEAPON_FIRE_STANDALONE
int main(void){unsigned long n=0;int ok=bh_weapon_fire_checkpoint(&n);printf("weapon-fire: %lu checks, %lu failures\n",n,failures);return !ok;}
#endif
