#include "../src/bh_script_sensor.h"
#include "../src/bh_script_wait.h"
#include "../src/bh_weapon_factory.h"
#include "../src/bh_fp.h"
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<14)printf("FAIL script-sensor %lu: %s\n",checks,s);}}while(0)
typedef struct Fixture {
 BHEntityPool pool;BHSoundQueue sounds;BHScriptLifetimeContext life;
 BHScriptSensorContext ctx;BHScriptTimerActor actors[3];BHEntity *radar[3];
 BHEntity *actor,*owner,*player,*sensor,*wait_template,*wait_child,*timer_template,*weapon;
 BHWeaponFactory factory;int factory_open;
} Fixture;
static void raw(BHEntity *e,unsigned long offset,const void *value,unsigned long n){memcpy((unsigned char*)e+offset,value,n);}
static int fixture(Fixture *f){BHScriptTimerState timer;BHScriptWaitState wait;BHScriptTimerPacket activation;
 memset(f,0,sizeof(*f));if(!bh_entities_init(&f->pool,16,NULL)||!bh_sound_queue_init(&f->sounds,NULL,NULL,NULL))return 0;
 f->actor=bh_entity_acquire(&f->pool);f->owner=bh_entity_acquire(&f->pool);f->player=bh_entity_acquire(&f->pool);f->wait_template=bh_entity_acquire(&f->pool);f->timer_template=bh_entity_acquire(&f->pool);
 if(!f->actor||!f->owner||!f->player||!f->wait_template||!f->timer_template)return 0;
 f->actors[0].entity=f->actor;f->actors[1].entity=f->owner;f->actors[2].entity=f->player;f->actors[0].active=1;
 if(!bh_script_timers_init(&f->life.timers,&f->pool,f->actors,3))return 0;f->life.sounds=&f->sounds;
 memset(&timer,0,sizeof(timer));timer.script_loaded=1;timer.variable_remaining=timer.variable_low=timer.variable_high=-1;timer.low=10;timer.high=10;
 if(!bh_script_timer_bind(&f->life.timers,f->timer_template,&timer,1))return 0;
 memset(&wait,0,sizeof(wait));wait.fired=f->timer_template;wait.expected=345;wait.variable_remaining=wait.variable_expected=-1;wait.script_loaded=1;
 if(!bh_script_wait_bind(&f->life.timers,f->wait_template,&wait))return 0;
 activation.event=8;activation.actor=f->actor;f->wait_template->query(f->wait_template,&activation);f->wait_child=f->pool.active;
 if(f->wait_child==f->timer_template||!bh_script_wait_state(&f->life.timers,f->wait_child))return 0;
 /* As in actual construction, acquire/register the new sensor after its
    existing receivers. Whole004770f0 starts at the newest query registration
    then follows actual active-next links, a retained original ordering rule. */
 f->sensor=bh_entity_acquire(&f->pool);if(!f->sensor)return 0;
 f->ctx.lifetime=&f->life;f->ctx.radar=f->radar;f->ctx.radar_count=1;f->ctx.player=f->player;f->radar[0]=f->player;
 return 1;
}
static void finish(Fixture *f){if(f->factory_open){if(f->weapon)bh_entity_release(&f->pool,f->weapon);CHECK(bh_weapon_factory_close(&f->factory),"close actual weapon factory after borrowed weapon release");}
 CHECK(bh_sound_queue_dispose(&f->sounds),"real sound ownership disposed without Play");CHECK(bh_entities_dispose(&f->pool),"real entity pool disposed");}
static BHScriptSensorState initial(Fixture *f){BHScriptSensorState s;memset(&s,0,sizeof(s));s.actor=f->actor;s.script_loaded=1;s.threshold=4;s.expected=345;return s;}
static BHScriptSensorState read(Fixture *f){BHScriptSensorState s;memset(&s,0,sizeof(s));CHECK(bh_script_sensor_read(f->sensor,&s),"actual state field view");return s;}
static void bounds(BHEntity *e,float low,float high){BHVec3 a,b;a.x=a.y=a.z=low;b.x=b.y=b.z=high;raw(e,0x1b0,&a,12);raw(e,0x1bc,&b,12);}
static void proximity(void){Fixture f;BHScriptSensorState s,out;BHScriptTimerState *fired;unsigned long used;float nan;unsigned long bits=0x7fc00001UL;
 CHECK(fixture(&f),"real pool, wait child and timer callback chain");s=initial(&f);f.player->position.x=1;
 CHECK(bh_script_sensor_bind(&f.ctx,f.sensor,&s,0)&&f.sensor->query==bh_script_sensor_query&&f.sensor->step==bh_script_proximity_tick,"bind whole step and real cleanup query");used=f.pool.used;f.sensor->step(f.sensor);out=read(&f);
 fired=bh_script_timer_state(&f.life.timers,f.pool.active);
 CHECK(f.pool.used==used+1&&fired&&fired->actor==f.actor&&fired->low==10,"event14 expected345 reaches actual wait child then actual timer constructor");
 CHECK(out.countdown>.21f&&out.countdown<.22f&&out.latch==0&&out.remaining==0,"due step adds quarter-second and list end resets latch, zero count unlimited");
 s=initial(&f);s.threshold=1;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used,"distance squared equal threshold is outside strict proximity");
 s.threshold=2;s.remaining=2;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);f.sensor->step(f.sensor);out=read(&f);CHECK(f.pool.used==used+1&&out.remaining==1,"positive count decremented after actual query");
 out.countdown=0;bh_script_sensor_bind(&f.ctx,f.sensor,&out,0);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used,"last fire creates timer then actually releases current sensor");
 finish(&f);
 CHECK(fixture(&f),"countdown/latch/player identity fixture");s=initial(&f);s.countdown=1;s.latch=1;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);out=read(&f);CHECK(f.pool.used==used&&out.latch==1&&out.countdown<1,"not-due countdown retains latch without scanning");
 s.countdown=0;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);f.sensor->step(f.sensor);out=read(&f);CHECK(f.pool.used==used&&out.latch==1,"inside player and positive existing latch returns retaining latch");
 f.ctx.radar_count=0;out.countdown=0;bh_script_sensor_bind(&f.ctx,f.sensor,&out,0);f.sensor->step(f.sensor);out=read(&f);CHECK(out.latch==0,"empty list clears latch on due step");
 f.ctx.radar_count=1;f.radar[0]=f.owner;out.countdown=0;bh_script_sensor_bind(&f.ctx,f.sensor,&out,0);f.sensor->step(f.sensor);CHECK(f.pool.used==used,"nonplayer radar identity ignored even at same position");
 f.radar[0]=f.player;memcpy(&nan,&bits,4);s=initial(&f);s.countdown=nan;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);f.sensor->step(f.sensor);out=read(&f);CHECK(f.pool.used==used+1&&_isnan(out.countdown),"unordered countdown follows due query branch");finish(&f);
}
static void box_and_radius(void){Fixture f;BHScriptSensorState s,out;unsigned long used,i;float nan;unsigned long bits=0x7fc00001UL;
 CHECK(fixture(&f),"box owner fixture");bounds(f.actor,10,20);bounds(f.owner,-1,1);f.actors[0].owner=f.actors+1;s=initial(&f);s.threshold=0;
 CHECK(bh_script_sensor_bind(&f.ctx,f.sensor,&s,0),"bind one-level owner box mode");used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used+1,"box uses owner bounds while packet/center retain original actor");
 f.actors[0].owner=NULL;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used,"unowned actor uses own outside bounds");
 bounds(f.actor,-1,1);f.player->position.x=1;f.player->position.y=-1;f.player->position.z=1;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);f.sensor->step(f.sensor);CHECK(f.pool.used==used+1,"all box boundaries inclusive");
 memcpy(&nan,&bits,4);f.player->position.x=f.player->position.y=f.player->position.z=0;
 for(i=0;i<6;++i){bounds(f.actor,-1,1);raw(f.actor,0x1b0+i*4,&nan,4);s.countdown=0;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used+((i==1||i==2||i==3)?1:0),"x87 asymmetric unordered bounds retain their actual C0/C3 branch contracts");}
 bounds(f.actor,-1,1);s=initial(&f);s.broadcast_radius=1;f.wait_child->position.x=100;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used,"actual positive-radius query excludes distant real wait receiver");
 s.broadcast_radius=0;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);f.sensor->step(f.sensor);CHECK(f.pool.used==used+1,"nonpositive query radius selects actual unfiltered dispatch");
 s.expected=346;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used,"actual wait query rejects different expected DWORD+8");
 f.ctx.radar_count=2;f.radar[1]=f.player;s.expected=345;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);f.sensor->step(f.sensor);out=read(&f);CHECK(f.pool.used==used+1&&out.latch==1,"second same-player entry hits newly set latch and returns before end reset");finish(&f);
}
static void damage(void){Fixture f;BHScriptSensorState s,out;BHWeaponObjectState *weapon;unsigned long used;float nan;unsigned long bits=0x7fc00001UL;
 CHECK(fixture(&f),"damage event17 fixture");CHECK(bh_weapon_factory_open(&f.factory,&f.pool,&f.sounds),"actual weapon owner opened");f.factory_open=1;f.weapon=bh_weapon_factory_create(&f.factory,0);CHECK(f.weapon!=NULL,"actual weapon factory registers complete removal query");weapon=bh_weapon_object_state(f.weapon);weapon->common.hud.target=f.actor;weapon->common.word_100=-1;
 s=initial(&f);s.threshold=10;s.remaining=1;bh_script_sensor_set_damage(f.actor,20);CHECK(bh_script_sensor_bind(&f.ctx,f.sensor,&s,1)&&f.sensor->step==bh_script_damage_tick,"bind full damage sensor and real cleanup query");used=f.pool.used;f.sensor->step(f.sensor);
 CHECK(f.pool.used==used&&weapon->common.hud.target==NULL,"damage event14 creates real timer then final event17 reaches actual weapon target-removal callback before source release");CHECK(bh_script_sensor_damage(f.actor)==0,"firing clears original actor damage after query");finish(&f);
 CHECK(fixture(&f),"damage countdown/scale fixture");s=initial(&f);s.threshold=100;s.countdown=0;bh_script_sensor_set_damage(f.actor,10);bh_script_sensor_bind(&f.ctx,f.sensor,&s,1);used=f.pool.used;f.sensor->step(f.sensor);out=read(&f);
 CHECK(f.pool.used==used&&bh_script_sensor_damage(f.actor)==9.8f&&out.latch==0&&out.countdown>.96f&&out.countdown<.97f,"due nontrigger retains rounded98-percent damage and one-second period");
 s.latch=1;s.countdown=1;bh_script_sensor_set_damage(f.actor,99);bh_script_sensor_bind(&f.ctx,f.sensor,&s,1);f.sensor->step(f.sensor);out=read(&f);CHECK(bh_script_sensor_damage(f.actor)==0&&out.latch==1&&out.countdown<1,"positive prior latch clears damage even before a not-due return");
 s=initial(&f);s.threshold=1;s.remaining=0;bh_script_sensor_set_damage(f.actor,2);bh_script_sensor_bind(&f.ctx,f.sensor,&s,1);used=f.pool.used;f.sensor->step(f.sensor);out=read(&f);CHECK(f.pool.used==used+1&&out.latch==1&&out.countdown>4.96f&&out.countdown<4.97f,"unlimited trigger preserves sensor and adds extra four seconds after one-second period");
 memcpy(&nan,&bits,4);s=initial(&f);s.threshold=0;s.countdown=0;bh_script_sensor_set_damage(f.actor,nan);bh_script_sensor_bind(&f.ctx,f.sensor,&s,1);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used&&_isnan(bh_script_sensor_damage(f.actor)),"unordered scaled damage never passes strict threshold");finish(&f);
}
static void numerical_and_guards(void){Fixture f;BHScriptSensorState s,out;unsigned long used,bits=0x7fc00001UL;float nan;BHEntity foreign,snapshot;BHScriptTimerPacket p;
 CHECK(fixture(&f),"numeric and guard fixture");memcpy(&nan,&bits,4);s=initial(&f);s.threshold=9;bh_script_sensor_set_damage(f.actor,9.183673858642578125f);bh_script_sensor_bind(&f.ctx,f.sensor,&s,1);used=f.pool.used;f.sensor->step(f.sensor);out=read(&f);
 CHECK(f.pool.used==used+1&&out.latch==1,"pre-store .98 product9.000000381 exceeds9 even though its binary32 spill equals9");
 s=initial(&f);s.threshold=10;bh_script_sensor_set_damage(f.actor,10.20408153533935546875f);bh_script_sensor_bind(&f.ctx,f.sensor,&s,1);used=f.pool.used;f.sensor->step(f.sensor);out=read(&f);
 CHECK(f.pool.used==used&&bh_script_sensor_damage(f.actor)==10&&out.latch==0,"pre-store .98 product just below10 does not trigger though its spill is10");
 s=initial(&f);s.latch=nan;s.remaining=LONG_MIN;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);out=read(&f);
 CHECK(f.pool.used==used+1&&out.remaining==LONG_MAX&&out.latch==0,"unordered latch permits fire and finite count decrements with exact32-bit wrap");
 f.player->position.x=nan;s=initial(&f);bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.pool.used==used,"unordered squared proximity is outside ordered strict radius");f.player->position.x=0;
 snapshot=*f.sensor;f.ctx.radar=NULL;CHECK(!bh_script_sensor_bind(&f.ctx,f.sensor,&s,0)&&f.ctx.error==BH_SENSOR_ARGUMENT&&!memcmp(&snapshot,f.sensor,sizeof(snapshot)),"invalid source list binding rejected before writes");f.ctx.radar=f.radar;
 s.actor=&foreign;CHECK(!bh_script_sensor_bind(&f.ctx,f.sensor,&s,0)&&f.ctx.error==BH_SENSOR_ACTOR&&!memcmp(&snapshot,f.sensor,sizeof(snapshot)),"foreign actor rejected through actual records/membership before dereference");s.actor=f.actor;
 bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);f.ctx.player=&foreign;f.radar[0]=&foreign;used=f.pool.used;f.sensor->step(f.sensor);CHECK(f.ctx.error==BH_SENSOR_ACTOR&&f.pool.used==used,"foreign matching player rejected before position reads");
 f.ctx.player=f.player;f.radar[0]=f.player;bh_script_sensor_bind(&f.ctx,f.sensor,&s,0);p.event=10;p.actor=f.actor;used=f.pool.used;f.sensor->query(f.sensor,&p);CHECK(f.sensor->step==bh_script_timer_delete_step()&&f.pool.used==used,"real actor event10 cleanup installs actual deferred delete");f.sensor->step(f.sensor);CHECK(f.pool.used==used-1,"actual timer delete closes sensor lifetime");finish(&f);
}
int __cdecl bh_script_sensor_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
 proximity();box_and_radius();damage();numerical_and_guards();bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"script-sensor\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_SENSOR_STANDALONE
int main(void){unsigned long count;return bh_script_sensor_checkpoint(&count);}
#endif
