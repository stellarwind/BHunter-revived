#include "../src/bh_actor_vehicle_draw.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure %d\n",__LINE__);}}while(0)
int bh_actor_vehicle_draw_checkpoint(unsigned long *count){
 BHActorVehicleDrawContext c;BHDrawQueue q;BHSoundQueue sound;BHSoundResource *r;BHSoundVoice *v;BHEntity e;BHActorRouteState s;BHMathEnvironment env;
 unsigned long model=17,id,handle,frames,i,expected;float fade=1,pose[8];BHVec3 offset={2,3,4};
 checks=failures=0;CHECK(bh_math_environment_enter(&env));CHECK(bh_draw_queue_init(&q,20,0));CHECK(bh_sound_queue_init(&sound,0,0,0));r=bh_sound_resource_allocate(&sound);CHECK(r!=0);r->streaming=1;r->base_frequency=22050;r->base_volume=0;
 memset(&c,0,sizeof(c));memset(&e,0,sizeof(e));memset(&s,0,sizeof(s));c.queue=&q;c.sound=&sound;c.damage_resource=31;c.glowing_model=999;c.glow_first=41;c.glow_second=42;c.tick=15;
 s.controls[1].x=100;s.controls[2].x=100;s.controls[3].x=200;s.duration_ticks=30;s.sound=BH_SOUND_INVALID;bh_actor_route_write(&e,&s);memcpy((char*)&e+0x15c,&offset,12);memcpy((char*)&e+0x19c,&fade,4);memcpy((char*)&e+0x114,&model,4);id=r->index;memcpy((char*)&e+0x10c,&id,4);
 srand(23);expected=rand();srand(23);CHECK(bh_actor_vehicle_draw(&c,&e));CHECK(rand()==expected&&q.count==10&&sound.voice_count==1);memcpy(pose,q.commands[9].words,24);CHECK(fabs(pose[0]-102)<.00001&&pose[1]==3&&pose[2]==4&&fabs(pose[3]-1.5707963)<.00001&&pose[4]==0&&pose[5]==0);
 memcpy(&handle,(char*)&e+0x104,4);v=bh_sound_voice(&sound,handle);CHECK(v&&v->position.x==102&&v->position.y==3&&v->position.z==4&&v->velocity.x>249&&v->velocity.x<250&&v->velocity.y==0&&v->velocity.z==0);memcpy(&frames,(char*)&e+0x108,4);CHECK(frames==2);
 CHECK(bh_draw_queue_begin(&q));fade=.5f;memcpy((char*)&e+0x19c,&fade,4);srand(23);for(i=0;i<3;++i)rand();expected=rand();srand(23);CHECK(bh_actor_vehicle_draw(&c,&e));CHECK(rand()==expected&&q.count==11&&sound.voice_count==1);CHECK(q.commands[10].resource==31);memcpy(pose,q.commands[10].words,32);CHECK(pose[3]>=4&&pose[3]<6&&pose[4]==pose[3]&&pose[6]==.5f&&q.commands[10].words[7]<8);CHECK(v->velocity.x>124.5&&v->velocity.x<125);
 CHECK(bh_draw_queue_begin(&q));fade=1;memcpy((char*)&e+0x19c,&fade,4);c.glowing_model=17;CHECK(bh_actor_vehicle_draw(&c,&e));CHECK(q.count==14);for(i=10;i<14;++i){CHECK(q.commands[i].resource==(i<12?41:42));memcpy(pose,q.commands[i].words,32);CHECK(pose[3]==2&&pose[4]==2&&pose[5]==0&&pose[6]==.5f&&q.commands[i].words[7]==0);CHECK(fabs(pose[0]-101.35)<.00001&&fabs(pose[1]-2.05)<.00001&&fabs(pose[2]-(3.4+.4*(i-10)))<.00001);}
 c.animation_clock=57;CHECK(bh_draw_queue_begin(&q));CHECK(bh_actor_vehicle_draw(&c,&e));for(i=10;i<14;++i){static const double rates[4]={25.5,18.5,28.5,23.5};memcpy(pose,q.commands[i].words,32);CHECK(fabs(pose[6]-(sin(57*.03125*rates[i-10])+1)*.5)<.000001);}
 /* Clock wrap and negative extrapolation use signed sixteen-bit deltas. */
 c.glowing_model=999;c.tick=4;s.tick=65530;s.duration_ticks=30;bh_actor_route_read(&e,&s);s.tick=65530;bh_actor_route_write(&e,&s);CHECK(bh_draw_queue_begin(&q));CHECK(bh_actor_vehicle_draw(&c,&e));CHECK(v->position.x>61&&v->position.x<62);
 c.tick=65529;CHECK(bh_draw_queue_begin(&q));CHECK(bh_actor_vehicle_draw(&c,&e));CHECK(fabs(v->position.x-(-1.33333349))<.00001);
 s.duration_ticks=0;bh_actor_route_write(&e,&s);CHECK(!bh_actor_vehicle_draw(&c,&e)&&c.error==BH_ACTOR_ROUTE_NUMERICAL);CHECK(!bh_actor_vehicle_draw(0,&e));
 CHECK(bh_sound_stop(&sound,handle));CHECK(bh_sound_queue_dispose(&sound));CHECK(bh_draw_queue_dispose(&q));bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"actor-vehicle-draw\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ACTOR_VEHICLE_DRAW_STANDALONE
int main(void){return bh_actor_vehicle_draw_checkpoint(0);}
#endif
