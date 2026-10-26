#include "../src/bh_script_actor_tick.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure line %d\n",__LINE__);}}while(0)
static void field(BHEntity *a,unsigned long off,unsigned long v){memcpy((char*)a+off,&v,4);}
static unsigned long read(BHEntity *a,unsigned long off){unsigned long v;memcpy(&v,(char*)a+off,4);return v;}
int bh_script_actor_tick_checkpoint(unsigned long *count){
 BHEntity a,before;BHActorRouteState s;BHActorRouteContext c;BHScriptActorPath path=bh_script_actor_linear;
 BHSoundQueue sound;BHSoundResource *r;BHSoundVoice *v;BHMathEnvironment env;unsigned long handle,voices;
 BHWorldGeometry w[3];BHGeometryModel model;BHPickupRoom rooms[3];unsigned long i;
 checks=failures=0;CHECK(bh_math_environment_enter(&env));memset(&a,0,sizeof(a));memset(&c,0,sizeof(c));memset(&s,0,sizeof(s));
 CHECK(bh_sound_queue_init(&sound,0,0,0));r=bh_sound_resource_allocate(&sound);CHECK(r!=0);
 if(r){r->streaming=1;r->base_frequency=8000;c.sound=&sound;c.tick=15;
  s.duration_ticks=30;s.duration=1;s.phase=.25f;s.sound_frames=3;s.sound=BH_SOUND_INVALID;
  s.controls[1].x=100;bh_actor_route_write(&a,&s);memcpy((char*)&a+0x1a0,&path,4);field(&a,0x10c,r->index);
  CHECK(bh_script_actor_clock_tick(&c,&a));handle=read(&a,0x104);v=bh_sound_voice(&sound,handle);CHECK(v&&v->position.x==50&&v->frequency==8000);
  CHECK(read(&a,0x108)==3);c.tick=20;voices=sound.voice_count;CHECK(bh_script_actor_clock_tick(&c,&a));CHECK(sound.voice_count==voices&&v->position.x>66&&v->position.x<67);
  field(&a,0x118,1);CHECK(bh_script_actor_phase_tick(&c,&a,r->index));CHECK(read(&a,0x118)==0&&sound.voice_count==voices+1&&v->position.x==25&&v->frequency==5600);
  CHECK(r->stream_head!=v&&r->stream_head->frequency==12000&&r->stream_head->velocity.x==0);
  CHECK(bh_script_actor_phase_tick(&c,&a,r->index));CHECK(read(&a,0x118)==119&&sound.voice_count==voices+1);
  field(&a,0x118,46);CHECK(bh_script_actor_phase_tick(&c,&a,r->index));CHECK(read(&a,0x118)==45&&sound.voice_count==voices+1);
  field(&a,0x108,0);before=a;CHECK(bh_script_actor_phase_tick(&c,&a,r->index));CHECK(!memcmp(&a,&before,sizeof(a)));
  field(&a,0x108,0xffffffffUL);before=a;CHECK(bh_script_actor_clock_tick(&c,&a));CHECK(!memcmp(&a,&before,sizeof(a)));
  field(&a,0x108,1);c.sound=0;before=a;CHECK(!bh_script_actor_clock_tick(&c,&a));CHECK(c.error==BH_ACTOR_ROUTE_ARGUMENT&&!memcmp(&a,&before,sizeof(a)));
  c.sound=&sound;c.tick=30;field(&a,0x108,0);CHECK(!bh_script_actor_clock_tick(&c,&a));CHECK(c.error==BH_ACTOR_ROUTE_ARGUMENT);
  memset(w,0,sizeof(w));memset(&model,0,sizeof(model));memset(rooms,0,sizeof(rooms));memset(&a,0,sizeof(a));memset(&s,0,sizeof(s));
  model.portal_count=2;for(i=0;i<3;++i){w[i].model=&model;rooms[i].world=w+i;}
  w[1].navigation.count=2;w[1].navigation.portals[0].direction=0;w[1].navigation.portals[0].neighbor=&w[0].navigation;
  w[1].navigation.portals[1].direction=4;w[1].navigation.portals[1].plane=100;w[1].navigation.portals[1].neighbor=&w[2].navigation;
  rooms[0].actors=&a;c.rooms=rooms;c.room_count=3;c.actor_limit=3;c.tick=34;
  s.current=w;s.next=w+1;s.speed=50;s.speed_scale=1;s.duration_ticks=30;s.sound=BH_SOUND_INVALID;bh_actor_route_write(&a,&s);
  CHECK(bh_script_actor_clock_tick(&c,&a));bh_actor_route_read(&a,&s);
  CHECK(s.tick==38&&s.duration_ticks==90&&s.current==w+1&&rooms[1].actors==&a&&!rooms[0].actors);
 }
 CHECK(bh_sound_queue_dispose(&sound));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"script-actor-tick\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCRIPT_ACTOR_TICK_STANDALONE
int main(void){return bh_script_actor_tick_checkpoint(0);}
#endif
