#include "../src/bh_script_motion.h"
#include "../src/bh_fp.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL script-motion %lu: %s\n",checks,s);}}while(0)
typedef struct Fixture {BHEntityPool pool;BHSoundQueue sounds;BHScriptLifetimeContext life;BHScriptMotionContext ctx;BHEntity *actor,*child[2],*step;} Fixture;
static int init(Fixture *f){memset(f,0,sizeof(*f));
 if(!bh_entities_init(&f->pool,4,NULL)||!bh_sound_queue_init(&f->sounds,NULL,NULL,NULL))return 0;
 f->actor=bh_entity_acquire(&f->pool);f->child[0]=bh_entity_acquire(&f->pool);f->child[1]=bh_entity_acquire(&f->pool);f->step=bh_entity_acquire(&f->pool);
 if(!f->actor||!f->child[0]||!f->child[1]||!f->step)return 0;
 if(!bh_script_timers_init(&f->life.timers,&f->pool,NULL,0))return 0;
 f->life.sounds=&f->sounds;f->ctx.lifetime=&f->life;return 1;
}
static void finish(Fixture *f){CHECK(bh_sound_queue_dispose(&f->sounds),"actual sound owner disposed without Play");CHECK(bh_entities_dispose(&f->pool),"actual entity ownership disposed");}
static BHScriptLifetimeState initial(Fixture *f){BHScriptLifetimeState s;memset(&s,0,sizeof(s));s.actor=f->actor;s.script_loaded=1;s.sound=BH_SOUND_INVALID;s.countdown=1;s.delta.x=1;s.delta.y=2;s.delta.z=4;return s;}
static BHScriptMotionActor target(Fixture *f){BHScriptMotionActor a;memset(&a,0,sizeof(a));
 a.minimum.x=-10;a.minimum.y=-20;a.minimum.z=-30;a.maximum.x=10;a.maximum.y=20;a.maximum.z=30;
 a.local_to_world.m[0][2]=1;a.local_to_world.m[1][1]=1;a.local_to_world.m[2][0]=-1;
 a.local_to_world.m[0][3]=5;a.local_to_world.m[1][3]=6;a.local_to_world.m[2][3]=7;
 a.world_to_local.m[0][2]=-1;a.world_to_local.m[1][1]=1;a.world_to_local.m[2][0]=1;
 a.world_to_local.m[0][3]=7;a.world_to_local.m[1][3]=-6;a.world_to_local.m[2][3]=-5;
 a.attachments=f->child[0];bh_script_motion_attachment_link(f->child[0],f->child[1]);bh_script_motion_attachment_link(f->child[1],NULL);
 f->actor->position.x=1;f->actor->position.y=2;f->actor->position.z=3;
 f->actor->origin.x=11;f->actor->origin.y=22;f->actor->origin.z=34;
 f->child[0]->position.x=10;f->child[0]->position.y=20;f->child[0]->position.z=30;
 f->child[1]->position.x=40;f->child[1]->position.y=50;f->child[1]->position.z=60;
 return a;
}
static int vec(BHVec3 v,float x,float y,float z){return v.x==x&&v.y==y&&v.z==z;}
static void translation(void){Fixture f;BHScriptLifetimeState s,out;BHScriptMotionActor a,b;BHScriptTimerPacket p;unsigned long used;BHScriptMotionBinding binding[2];BHWorldGeometry world[2];BHVec3 draw_angles;
 CHECK(init(&f),"owned translation fixture");a=target(&f);CHECK(bh_script_motion_actor_write(f.actor,&a),"write exact movable-target field view");s=initial(&f);
 memset(world,0,sizeof(world));memset(binding,0,sizeof(binding));binding[0].entity=f.actor;binding[0].world=world;binding[0].draw_angles=&draw_angles;binding[1].entity=f.child[0];binding[1].world=world+1;binding[1].draw_angles=0;f.ctx.bindings=binding;f.ctx.binding_count=2;
 CHECK(bh_script_motion_bind(&f.ctx,f.step,&s,0)&&f.step->step==bh_script_translate_tick&&f.step->query==bh_script_motion_query,"bind actual original translation step and cleanup query");
 f.step->step(f.step);bh_script_motion_actor_read(f.actor,&b);bh_script_lifetime_read(f.step,&out);
 CHECK(vec(b.minimum,-9,-18,-26)&&vec(b.maximum,11,22,34),"both bounding corners translated");CHECK(vec(f.actor->position,2,4,7),"target position translated");
 CHECK(b.local_to_world.m[0][3]==6&&b.local_to_world.m[1][3]==8&&b.local_to_world.m[2][3]==11,"real world-translation owner changes forward transform");
 CHECK(b.world_to_local.m[0][3]==11&&b.world_to_local.m[1][3]==-8&&b.world_to_local.m[2][3]==-6,"real negative local-translation owner changes inverse using its nonidentity basis");
 CHECK(vec(f.child[0]->position,11,22,34)&&vec(f.child[1]->position,41,52,64),"all actual linked attachments translated");CHECK(vec(f.actor->origin,11,22,34),"positive duration retains parser-added origin delta");
 CHECK(!memcmp(&world[0].position,&f.actor->position,12)&&!memcmp(&world[0].transform,&b.local_to_world,48)&&!memcmp(&world[0].inverse,&b.world_to_local,48)&&!memcmp(&world[0].minimum,&b.minimum,24),"placed semantic collision state observes motion before the next entity callback");
 CHECK(!memcmp(&draw_angles,&b.angles,12)&&vec(world[1].position,11,22,34),"draw angles and attached placement position publish through actual typed bindings");
 CHECK(out.countdown<1&&out.countdown>.96f&&b.attachments==f.child[0]&&!memcmp(&b.angles,&a.angles,12),"countdown stores and unrelated target fields retained");
 s.countdown=.03125f;CHECK(bh_script_motion_bind(&f.ctx,f.step,&s,0),"near-expiry bind");used=f.pool.used;f.step->step(f.step);
 CHECK(f.pool.used==used-1&&vec(f.actor->origin,10,20,30),"positive crossing expires through real pool and undoes origin once");CHECK(vec(f.actor->position,3,6,11),"expiry tick still applies complete translation before release");
 finish(&f);
 CHECK(init(&f),"query/deferred ownership fixture");a=target(&f);bh_script_motion_actor_write(f.actor,&a);s=initial(&f);bh_script_motion_bind(&f.ctx,f.step,&s,0);
 p.event=10;p.actor=f.actor;used=f.pool.used;f.step->query(f.step,&p);
 CHECK(f.step->step==bh_script_timer_delete_step()&&f.pool.used==used&&vec(f.actor->origin,10,20,30),"actual actor cleanup replaces movement by deferred delete and reverses origin");
 f.step->step(f.step);CHECK(f.pool.used==used-1&&vec(f.actor->position,1,2,3),"real deferred owner releases without extra movement");finish(&f);
}
static void durations(void){Fixture f;BHScriptLifetimeState s,out;BHScriptMotionActor a;float times[5]={0,-1,1,.03125f,0};unsigned long i,bits=0x7fc01234UL,used;
 memcpy(times+4,&bits,4);
 for(i=0;i<5;++i){CHECK(init(&f),"duration fixture");a=target(&f);a.attachments=NULL;bh_script_motion_actor_write(f.actor,&a);s=initial(&f);s.countdown=times[i];bh_script_motion_bind(&f.ctx,f.step,&s,0);used=f.pool.used;f.step->step(f.step);
  CHECK(f.pool.used==used-(i==3),"only initially positive duration crossing zero expires");CHECK(vec(f.actor->position,2,4,7),"zero/negative/unordered duration still moves target");
  if(i!=3){bh_script_lifetime_read(f.step,&out);CHECK(i==4?_isnan(out.countdown):(i<2?out.countdown==times[i]:out.countdown<times[i]),"unlimited and NaN durations retain their original bits/class");}
  finish(&f);
 }
}
static unsigned long float_bits(float value){unsigned long bits;memcpy(&bits,&value,4);return bits;}
static void rotation(void){Fixture f;BHScriptLifetimeState s;BHScriptMotionActor a,b;BHVec3 old_position,old_origin;unsigned long i,bits=0x7fc00001UL,used;float values[9]={0,3.1415927f,-3.1415927f,7,-7,20,-20,0,1};
 static const unsigned long expected_x[9]={0,0xc0490fdaUL,0x40490fdaUL,0x3f37812bUL,0xbf37812bUL,0x415b7813UL,0xc15b7813UL,0,0x3f800000UL};
 static const unsigned long expected_y[9]={0,0x40490fdaUL,0xc0490fdaUL,0xbf37812bUL,0x3f37812bUL,0xc15b7813UL,0x415b7813UL,0,0xbf800000UL};
 memcpy(values+7,&bits,4);
 CHECK(init(&f),"rotation fixture");a=target(&f);s=initial(&f);s.delta.x=s.delta.y=s.delta.z=0;s.countdown=0;
 for(i=0;i<9;++i){a.angles.x=values[i];a.angles.y=-values[i];a.angles.z=values[i];bh_script_motion_actor_write(f.actor,&a);old_position=f.actor->position;old_origin=f.actor->origin;
  CHECK(bh_script_motion_bind(&f.ctx,f.step,&s,1)&&f.step->step==bh_script_rotate_tick&&f.step->query==bh_script_sensor_query,"actual rotation and sensor query binding");f.step->step(f.step);bh_script_motion_actor_read(f.actor,&b);
  CHECK(i==7?_isnan(b.angles.x):float_bits(b.angles.x)==expected_x[i],"fixed binary32 fixtures for exact pi boundaries and single correction positive/negative/large values");
  CHECK(i==7?_isnan(b.angles.y):float_bits(b.angles.y)==expected_y[i],"second angle wraps independently");
  CHECK(!memcmp(&a.minimum,&b.minimum,24)&&!memcmp(&a.local_to_world,&b.local_to_world,96)&&!memcmp(&old_position,&f.actor->position,12)&&!memcmp(&old_origin,&f.actor->origin,12),"rotation retains target bounds/transforms/position/origin");
 }
 a.angles.x=1;a.angles.y=2;a.angles.z=3;bh_script_motion_actor_write(f.actor,&a);s.delta.x=.125f;s.delta.y=-.25f;s.delta.z=.5f;s.countdown=.03125f;bh_script_motion_bind(&f.ctx,f.step,&s,1);used=f.pool.used;f.step->step(f.step);bh_script_motion_actor_read(f.actor,&b);
 CHECK(f.pool.used==used-1&&b.angles.x==1.125f&&b.angles.y==1.75f&&b.angles.z==-2.783185307179586f,"rotation applies delta and wrapping before duration crossing frees it");
 finish(&f);
}
static void guards(void){Fixture f;BHScriptLifetimeState s;BHScriptMotionActor a;BHEntity saved,foreign;unsigned long used;
 CHECK(init(&f),"guard fixture");a=target(&f);bh_script_motion_actor_write(f.actor,&a);s=initial(&f);
 bh_script_motion_attachment_link(f.child[1],f.child[0]);saved=*f.actor;used=f.pool.used;
 CHECK(!bh_script_motion_bind(&f.ctx,f.step,&s,0)&&f.ctx.error==BH_MOTION_ATTACHMENTS&&!memcmp(&saved,f.actor,sizeof(saved))&&f.pool.used==used,"cyclic source attachment rejected before mutation");
 bh_script_motion_attachment_link(f.child[1],NULL);CHECK(bh_script_motion_bind(&f.ctx,f.step,&s,0),"valid actual list accepted");bh_script_motion_attachment_link(f.child[1],&foreign);f.step->step(f.step);
 CHECK(f.ctx.error==BH_MOTION_ATTACHMENTS&&!memcmp(&saved,f.actor,sizeof(saved))&&f.pool.used==used,"step revalidates foreign attachment before reading its fields or mutating target");
 bh_script_motion_attachment_link(f.child[1],NULL);s.actor=f.step;CHECK(!bh_script_motion_bind(&f.ctx,f.step,&s,0)&&f.ctx.error==BH_MOTION_LIFETIME,"self target rejected before mutation");
 s.actor=&foreign;CHECK(!bh_script_motion_bind(&f.ctx,f.step,&s,1)&&f.ctx.error==BH_MOTION_LIFETIME,"foreign rotation target rejected");
 s.actor=f.actor;CHECK(bh_script_motion_bind(&f.ctx,f.step,&s,1),"restore actual target");bh_entity_release(&f.pool,f.actor);f.step->step(f.step);CHECK(f.ctx.error==BH_MOTION_LIFETIME,"released target stops source step with explicit lifetime error");finish(&f);
}
static void semantic_attachments(void){Fixture f;BHScriptLifetimeState s;BHScriptMotionActor a,b;BHScriptMotionBinding binding;BHWorldGeometry world;BHEntity *head;BHVec3 before;
 CHECK(init(&f),"semantic attachment fixture");a=target(&f);bh_script_motion_attachment_link(f.child[0],NULL);bh_script_motion_actor_write(f.actor,&a);s=initial(&f);memset(&binding,0,sizeof(binding));memset(&world,0,sizeof(world));head=f.child[1];binding.entity=f.actor;binding.world=&world;binding.attachments_owner=&head;f.ctx.bindings=&binding;f.ctx.binding_count=1;
 CHECK(bh_script_motion_bind(&f.ctx,f.step,&s,0),"semantic head overrides unrelated raw+12c for validation");f.step->step(f.step);
 CHECK(f.ctx.error==BH_MOTION_OK&&vec(f.child[0]->position,10,20,30)&&vec(f.child[1]->position,41,52,64),"only current semantic ec chain moves with world");bh_script_motion_actor_read(f.actor,&b);CHECK(b.attachments==f.child[0],"semantic list does not overwrite original raw owner storage");
 head=f.child[0];bh_script_motion_attachment_link(head,NULL);f.step->step(f.step);CHECK(f.ctx.error==BH_MOTION_OK&&vec(f.child[0]->position,11,22,34)&&vec(f.child[1]->position,41,52,64),"next tick observes semantic head changed by pickup/marker owner");
 head=NULL;f.step->step(f.step);CHECK(f.ctx.error==BH_MOTION_OK&&vec(f.actor->position,4,8,15)&&vec(f.child[0]->position,11,22,34)&&vec(f.child[1]->position,41,52,64),"empty semantic list suppresses stale raw attachments");
 head=f.child[0];bh_script_motion_attachment_link(head,head);before=f.actor->position;f.step->step(f.step);CHECK(f.ctx.error==BH_MOTION_ATTACHMENTS&&!memcmp(&before,&f.actor->position,12),"cyclic semantic head rejected before world mutation");finish(&f);
}
int __cdecl bh_script_motion_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
 translation();durations();rotation();guards();semantic_attachments();bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"script-motion\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_MOTION_STANDALONE
int main(void){unsigned long n;return bh_script_motion_checkpoint(&n);}
#endif
