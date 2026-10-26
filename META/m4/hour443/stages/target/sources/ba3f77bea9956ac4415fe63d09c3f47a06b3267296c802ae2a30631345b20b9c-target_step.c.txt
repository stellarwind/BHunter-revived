#include "../src/bh_target_step.h"
#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
static unsigned long checks,failed;
#define CHECK(x,s) do{++checks;if(!(x)){++failed;if(failed<12)printf("FAIL target-step %d: %s\n",__LINE__,s);}}while(0)
static void identity(BHMat34*m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
typedef struct Fixture{BHEntityPool pool;BHEntity *e,*owner,*registry[4];BHDrawQueue queue;BHSoundQueue sound;BHDebrisContext debris;BHDebrisBody body;BHPhysicsState state;BHPhysicsStepContext physics;BHPhysicsStepBody pb;BHPhysicsEffects effects;BHContactEffects contact;BHGeometry*models[1];BHMat34 inverse;BHScriptDebrisContext fragments;BHFlashFlareContext flash;BHScriptExplosionContext explosions;BHScriptParticleContext particles;BHPickupContext pickups;BHPickupRenderBinding render;BHPickupDrawContext draw;BHEntityResourceDrawContext special;BHWorldGeometry world;BHPickupRoom room;unsigned long resources[20];BHTargetStepContext target;BHTargetStepState ts;long mode;}Fixture;
static int setup(Fixture*f){BHVec3 inertia={1,1,1};BHSoundResource*r;unsigned long i;memset(f,0,sizeof(*f));
 if(!bh_entities_init(&f->pool,32,NULL)||!bh_draw_queue_init(&f->queue,64,NULL)||!bh_sound_queue_init(&f->sound,NULL,NULL,NULL))return 0;
 r=bh_sound_resource_allocate(&f->sound);if(!r)return 0;r->streaming=1;r->base_frequency=100;
 f->e=bh_entity_acquire(&f->pool);f->owner=bh_entity_acquire(&f->pool);f->owner->position.x=1000;bh_entity_set_query(&f->pool,f->owner,bh_actor_target_removed);f->registry[0]=f->e;f->registry[1]=f->owner;
 identity(&f->state.pose);f->state.last_clear=f->state.pose;identity(&f->inverse);
 if(!bh_debris_context_init(&f->debris,&f->pool,&f->sound,r->index,f->registry,4)||!bh_debris_bind(&f->debris,&f->body,f->e,&f->state,2,0,&inertia))return 0;f->body.owner=f->owner;
 f->pb.entity=f->e;f->pb.generation=f->e->generation;f->pb.physics=&f->state;f->pb.target_world=&f->state.pose;f->pb.target_local=&f->inverse;f->pb.mass=2;
 if(!bh_contact_effects_init(&f->contact,&f->pool,&f->queue,1,2,3,4))return 0;f->effects.effects=&f->contact;f->effects.sound=&f->sound;
 if(!bh_physics_step_init(&f->physics,&f->pool,&f->pb,1,NULL,0,f->models,1,&f->effects,1,NULL))return 0;
 f->fragments.pool=&f->pool;f->fragments.queue=&f->queue;f->fragments.image=9;f->flash.pool=&f->pool;f->flash.queue=&f->queue;f->flash.textures[0]=10;
 f->explosions.pool=&f->pool;f->explosions.queue=&f->queue;f->explosions.sound=&f->sound;f->explosions.debris=&f->fragments;f->explosions.flash=&f->flash;f->explosions.sound_resource=r->index;f->explosions.image=11;
 if(!bh_script_particles_init(&f->particles,&f->pool,&f->queue))return 0;
 f->world.navigation.next=&f->world.navigation;f->world.minimum.x=f->world.minimum.y=f->world.minimum.z=-100;f->world.maximum.x=f->world.maximum.y=f->world.maximum.z=100;identity(&f->world.transform);f->room.world=&f->world;
 for(i=0;i<20;++i)f->resources[i]=i+1;f->pickups.pool=&f->pool;f->pickups.first_world=&f->world;f->pickups.world_count=1;f->pickups.rooms=&f->room;f->pickups.room_count=1;f->pickups.resources=f->resources;f->pickups.resource_count=20;f->pickups.outside_actors=f->registry;f->pickups.sound=&f->sound;f->draw.queue=&f->queue;f->special.queue=&f->queue;f->render.default_draw=&f->draw;f->render.special_draw=&f->special;if(!bh_pickup_render_bind(&f->pickups,&f->render))return 0;
 f->target.body=&f->body;f->target.physics=&f->physics;f->target.explosions=&f->explosions;f->target.particles=&f->particles;f->target.pickups=&f->pickups;f->target.mode=&f->mode;f->target.smoke_image=13;f->ts.minimum.x=-2;f->ts.minimum.y=-3;f->ts.minimum.z=-4;f->ts.maximum.x=2;f->ts.maximum.y=3;f->ts.maximum.z=4;f->ts.loot=13;f->ts.token=77;
 return bh_target_step_bind(&f->target,&f->ts);
}
static void finish(Fixture*f){CHECK(bh_physics_step_dispose(&f->physics),"fixed physics roster disposed after target retirement");if(f->body.entity)CHECK(bh_debris_unbind(&f->body),"live real debris query detached");CHECK(bh_debris_context_dispose(&f->debris),"real body/query ownership ended");while(f->pickups.head)bh_pickup_cleanup(f->pickups.head);while(f->pool.active)bh_entity_release(&f->pool,f->pool.active);CHECK(bh_contact_effects_dispose(&f->contact)&&bh_sound_queue_dispose(&f->sound)&&bh_draw_queue_dispose(&f->queue)&&bh_entities_dispose(&f->pool),"all actual effect/audio/entity allocations disposed");}
static void ordinary(void){Fixture f;BHTargetStepState s;BHEntity*at;BHScriptParticleState*p;unsigned long next,used;int r[7],i;float expected;
 CHECK(setup(&f),"real typed target dependencies");s=f.ts;s.counter=4;s.emission=32768;bh_target_step_write(f.e,&s);CHECK(f.pb.flags==32768,"state writer synchronizes shared original114 physics roster flags");f.mode=1;srand(42);next=rand();srand(42);used=f.pool.used;bh_target_step(f.e);bh_target_step_read(f.e,&s);CHECK(s.counter==4&&f.pool.used==used&&(unsigned long)rand()==next&&!f.physics.steps,"mode1 returns before all state/random/effects");
 f.mode=0;f.body.sleep_time=3;srand(42);for(i=0;i<7;++i)r[i]=rand();next=rand();srand(42);bh_target_step(f.e);bh_target_step_read(f.e,&s);CHECK(s.counter==5&&f.pool.used==used+1&&(unsigned long)rand()==next&&!f.physics.steps,"exact chance+five emission+actual constructor random draws precede sleeping early return");
 at=f.pool.active;p=bh_script_particle_state(&f.particles,at);bh_round_float(&expected,(double)(r[1]-16384)/262144);CHECK(p&&p->resource==13&&p->bucket==3&&p->velocity.x==expected&&p->acceleration.y==-.002f&&p->size==.5f&&p->lifetime==2&&p->alpha==.3f,"actual smoke constructor receives original velocity, acceleration and full state");
 s.emission=0;s.countdown=1;bh_target_step_write(f.e,&s);f.body.sleep_time=0;f.state.velocity.x=3;bh_target_step(f.e);bh_target_step_read(f.e,&s);CHECK(f.physics.steps==1&&s.countdown<1&&f.state.pose.m[0][3]>.09&&f.state.velocity.x>2.9&&f.body.sleep_time==0,"actual full physics executes at original binary32 dt while active motion suppresses quiet timer");
 f.state.velocity.x=f.state.velocity.y=f.state.velocity.z=0;f.state.acceleration.x=f.state.acceleration.y=f.state.acceleration.z=0;bh_target_step(f.e);CHECK(f.physics.steps==2&&f.body.sleep_time>.066f&&f.body.sleep_time<.067f,"quiet actual physics increases same real debris sleep word by2/30");finish(&f);
}
static void loot(void){Fixture f;BHPickupState*p;unsigned long kind,seed;BHTargetStepState s;static const long amounts[10]={100,50,25,2,50,1,1,1,1,1};for(kind=0;kind<=15;++kind){CHECK(setup(&f),"real target destruction/pickup owner fixture");s=f.ts;s.countdown=.01f;s.loot=kind;bh_target_step_write(f.e,&s);seed=1;CHECK(seed!=0,"deterministic seed for actual pickup/explosion branch");srand(seed);bh_target_step(f.e);
 CHECK(!f.body.entity&&f.registry[0]==f.owner&&!f.registry[1]&&f.pool.used>=3,"whole destruction shifts actual shared outside registry then releases target");
 if(kind==13||kind==15)CHECK(!f.pickups.head,"case13 and out-of-table default skip pickup");else{p=bh_pickup_state(f.pickups.head);CHECK(p&&p->type==(long)(kind<10?kind:kind-10)&&p->variant==(long)(kind<10?0:1)&&p->remaining==(kind<10?amounts[kind]:1)&&p->offer==p->remaining&&p->fade==40&&!p->token,"all actual pickup switch cases use correct type/variant/count/fade");}
 {BHEntity*e;unsigned long n=0;BHScriptExplosionState x;for(e=f.pool.active;e;e=e->next)if(e->step==bh_script_explosion_tick){++n;bh_script_explosion_read(e,&x);CHECK(x.duration==1.2f&&x.magnitude==15&&x.probability==.95f&&x.target==(unsigned long)f.owner,"actual root explosion arguments and owner target retained");}CHECK(n==1,"one whole root explosion in every nonnegative loot branch");}finish(&f);}}
static void negative(void){Fixture f;BHTargetStepState s;BHVec3 expected[6];BHScriptExplosionState x;BHEntity*e;unsigned long seed,i,j,next,n=0;double t,z;float a,b;CHECK(setup(&f),"six transformed explosions fixture");s=f.ts;s.countdown=.01f;s.loot=-1;bh_target_step_write(f.e,&s);f.state.pose.m[0][0]=0;f.state.pose.m[0][1]=-1;f.state.pose.m[1][0]=1;f.state.pose.m[1][1]=0;f.state.pose.m[0][3]=10;bh_physics_step_publish(&f.pb);
 seed=1;CHECK(seed!=0,"independent complete six-constructor random schedule");srand(seed);for(i=0;i<6;++i){t=(double)rand()/32768;bh_round_float(&a,2*(1-t)-2*t);t=(double)rand()/32768;bh_round_float(&b,3*(1-t)-3*t);t=(double)rand()/32768;z=4*(1-t)-4*t;bh_round_float(&expected[i].x,10.0-b);expected[i].y=a;bh_round_float(&expected[i].z,z);for(j=0;j<48;++j)rand();{unsigned long debris=2+(unsigned long)((double)rand()*12/32768);for(j=0;j<debris*9;++j)rand();}}next=rand();srand(seed);bh_target_step(f.e);
 for(e=f.pool.active;e;e=e->next)if(e->step==bh_script_explosion_tick){CHECK(n<6&&!memcmp(&e->position,expected+5-n,12),"random box point transformed with actual old actor basis before explosion");bh_script_explosion_read(e,&x);CHECK(x.duration==1&&x.magnitude==12&&x.target==(unsigned long)f.owner,"negative branch actual explosion duration/strength/owner");++n;}
 CHECK(n==6&&!f.body.entity&&!f.pickups.head&&(unsigned long)rand()==next,"all six complete effects, exact complete CRT random schedule, no pickup, real retirement");finish(&f);
}
static Fixture*observed;static unsigned long events;
/* Explicit isolated packet observer delegates the actual restored receiver;
   this fixture does not implement game event12 semantics. */
static void __cdecl owner_observer(BHEntity*e,void*p){long event,token;memcpy(&event,p,4);if(event==12){memcpy(&token,(char*)p+4,4);++events;CHECK(token==77&&observed->body.entity&&!observed->pickups.head,"borrowed event12 occurs before pickup/explosion/removal");}bh_actor_target_removed(e,p);}
static void ordering(void){Fixture f;BHTargetStepState s;CHECK(setup(&f),"borrowed actual owner query interface");observed=&f;events=0;bh_entity_set_query(&f.pool,f.owner,owner_observer);s=f.ts;s.countdown=.01f;s.loot=13;bh_target_step_write(f.e,&s);srand(1);bh_target_step(f.e);CHECK(events==1&&!f.body.entity,"isolated observing wrapper sees event12 once; actual query ignored branch remains real");observed=NULL;finish(&f);}
int __cdecl bh_target_step_checkpoint(unsigned long*count){BHMathEnvironment env;checks=failed=0;if(!bh_math_environment_enter(&env))return 2;ordinary();loot();negative();ordering();bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"target-step\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failed);return failed!=0;}
#ifdef BH_TARGET_STEP_STANDALONE
int main(void){return bh_target_step_checkpoint(NULL);}
#endif
