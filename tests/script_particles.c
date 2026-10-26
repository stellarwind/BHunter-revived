#include "../src/bh_script_particles.h"
#include "../src/bh_fp.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL script-particles %lu: %s\n",checks,s);}}while(0)
typedef struct Memory{unsigned long calls,live,fail_at;}Memory;
static void *__cdecl allocate(unsigned long n,void*u){Memory*m=(Memory*)u;void*p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void*p,void*u){--((Memory*)u)->live;free(p);}
typedef struct Fixture{BHEntityPool pool;BHDrawQueue queue;BHSoundQueue sounds;BHScriptLifetimeContext lifetime;BHScriptParticleContext particles;BHScriptEmitterContext emitter;BHScriptTimerActor actors[2];BHEntity*actor,*owner;Memory memory;}Fixture;
static int fixture(Fixture*f){BHSceneAllocator a;memset(f,0,sizeof(*f));a.allocate=allocate;a.release=release;a.user=&f->memory;
 if(!bh_entities_init(&f->pool,8,&a)||!bh_draw_queue_init(&f->queue,128,&a)||!bh_sound_queue_init(&f->sounds,NULL,NULL,&a))return 0;
 f->actor=bh_entity_acquire(&f->pool);f->owner=bh_entity_acquire(&f->pool);if(!f->actor||!f->owner)return 0;
 f->actors[0].entity=f->actor;f->actors[0].active=1;f->actors[1].entity=f->owner;
 if(!bh_script_timers_init(&f->lifetime.timers,&f->pool,f->actors,2)||!bh_script_particles_init(&f->particles,&f->pool,&f->queue))return 0;
 f->lifetime.sounds=&f->sounds;f->emitter.lifetime=&f->lifetime;f->emitter.particles=&f->particles;f->emitter.fountain_texture=123;f->emitter.steam_texture=456;
 f->actor->position.x=10;f->actor->position.y=20;f->actor->position.z=-30;return 1;
}
static void finish(Fixture*f){CHECK(bh_sound_queue_dispose(&f->sounds),"real sound source binding disposed without playback");CHECK(bh_draw_queue_dispose(&f->queue),"real draw queue disposed");CHECK(bh_entities_dispose(&f->pool),"real entity pool disposed");CHECK(!f->memory.live,"all actual allocator ownership released");}
static float word(unsigned long*w){float v;memcpy(&v,w,4);return v;}
static int vector(BHVec3 v,float x,float y,float z){return v.x==x&&v.y==y&&v.z==z;}
static int present(Fixture*f,BHEntity*e){BHEntity*p;for(p=f->pool.active;p;p=p->next)if(p==e)return 1;return 0;}
static BHEntity *particle(Fixture*f){BHVec3 p={1,2,3},v={4,5,6},a={.25f,-.5f,1};return bh_script_particle_create(&f->particles,99,3,&p,&v,&a,2,.25f,6,3,.5f,.25f,2,.75f);}
static void particle_cases(void){Fixture f;BHEntity*e;BHScriptParticleState*s;BHDrawCommand*q;BHVec3 p={1,2,3};unsigned long seed,expected,next,used,i,nan_bits=0x7fc01234UL;float nan;
 CHECK(fixture(&f),"actual pool/queue particle fixture");
 for(seed=1;seed<=16;++seed){srand(seed);expected=(unsigned long)rand()&7;next=(unsigned long)rand();srand(seed);e=particle(&f);CHECK(e!=NULL,"whole constructor acquires actual entity");s=bh_script_particle_state(&f.particles,e);
  CHECK(s&&s->resource==99&&s->bucket==3&&s->frame==expected&&(unsigned long)rand()==next,"constructor consumes exactly one shared CRT draw after allocation");
  CHECK(s&&s->size==2&&s->angle==.25f&&s->lifetime==2&&s->alpha==.75f&&s->size_decay==.5f&&s->angular_decay==.25f,"all original scalar fields retained");
  CHECK(s&&s->size_step==.2f&&s->angular_step==.1f&&s->alpha_step==-.0125f,"exact double timestep conversion and opacity division store binary32");
  CHECK(vector(e->position,1,2,3)&&vector(s->velocity,4,5,6)&&vector(s->acceleration,.25f,-.5f,1),"copied point/velocity/acceleration");bh_entity_release(&f.pool,e);
 }
 e=particle(&f);s=bh_script_particle_state(&f.particles,e);s->reserved[0]=0xa1;s->reserved[1]=0xb2;s->reserved[2]=0xc3;e->step(e);
 CHECK(vector(s->velocity,4.25f,4.5f,7)&&vector(e->position,5.25f,6.5f,10),"tick acceleration then motion through real state");
 CHECK(s->lifetime<2&&s->angle==.35f&&s->size==2.2f&&s->angular_step==.025f&&s->size_step==.1f&&s->alpha==.7375f,"ordered angle/size increments then damping and alpha");
 CHECK(s->reserved[0]==0xa1&&s->reserved[1]==0xb2&&s->reserved[2]==0xc3,"unused original state bytes retained");
 e->refresh(e,NULL);q=&f.queue.commands[f.queue.count-1];CHECK(q->kind==BH_DRAW_BILLBOARD&&q->resource==99&&q->words[7]==s->frame,"real billboard queue owner and source resource/frame identity");
 CHECK(!memcmp(q->words,&e->position,12)&&word(q->words+3)==s->size&&word(q->words+4)==s->size&&word(q->words+5)==s->angle&&word(q->words+6)==s->alpha,"all billboard arguments preserve current state");
 s->velocity.x=s->velocity.y=s->velocity.z=16777216;s->acceleration.x=s->acceleration.y=s->acceleration.z=1;e->position.x=e->position.y=e->position.z=-16777216;e->step(e);
 CHECK(vector(s->velocity,16777216,16777216,16777216)&&vector(e->position,1,0,0),"original X unrounded x87 velocity survives store while Y/Z reload stored velocity");
 bh_entity_release(&f.pool,e);e=bh_script_particle_create(&f.particles,1,3,&p,NULL,NULL,1,0,0,0,1,1,0,1);s=bh_script_particle_state(&f.particles,e);
 CHECK(s&&vector(s->velocity,0,0,0)&&vector(s->acceleration,0,0,0)&&!_finite(s->alpha_step)&&s->alpha_step<0,"NULL optional vectors zero; zero-duration original masked divide yields negative infinity");
 used=f.pool.used;e->step(e);CHECK(f.pool.used==used-1&&!present(&f,e)&&vector(e->position,1,2,3),"expired particle releases BEFORE all motion");
 memcpy(&nan,&nan_bits,4);e=particle(&f);s=bh_script_particle_state(&f.particles,e);s->lifetime=nan;used=f.pool.used;e->step(e);CHECK(f.pool.used==used-1,"unordered decremented particle lifetime expires");
 e=particle(&f);s=bh_script_particle_state(&f.particles,e);for(i=0;i<128;++i)e->refresh(e,NULL);CHECK(f.queue.error==BH_DRAW_FULL&&present(&f,e),"full real queue drops draw without deleting particle");
 /* Force actual pool-growth failure; ctor must not consume its frame draw. */
 while(f.pool.free_slots)CHECK(bh_entity_acquire(&f.pool)!=NULL,"fill existing slots");srand(17);next=rand();srand(17);f.memory.fail_at=f.memory.calls+1;used=f.pool.used;
 CHECK(!particle(&f)&&f.pool.used==used&&f.particles.error==BH_PARTICLE_POOL&&(unsigned long)rand()==next,"real allocation failure has no frame draw or fabricated entity");f.memory.fail_at=0;finish(&f);
}
static BHScriptEmitterState initial(Fixture*f){BHScriptEmitterState s;memset(&s,0,sizeof(s));s.actor=f->actor;s.script_loaded=1;s.countdown=.03125f;s.spread=.0001f;s.velocity.x=1;s.velocity.y=2;s.velocity.z=3;s.acceleration.y=-.1f;s.size=7;return s;}
static BHEntity *emitter(Fixture*f,const BHScriptEmitterState*s,int steam){BHEntity*e=bh_entity_acquire(&f->pool);if(!e||!bh_script_emitter_bind(&f->emitter,e,s,steam))return NULL;return e;}
static void emitter_forced(void){Fixture f;BHScriptEmitterState in,out;BHEntity*e,*created;BHScriptParticleState*p;unsigned long seed,used,frame,next;int randoms[6],i,steam;float spin,rate,angle;volatile float spin_step,size_step;BHVec3 v;
 CHECK(fixture(&f),"emitter fixture using actual actor table, lifetime, queue and particle constructor");
 for(steam=0;steam<2;++steam)for(seed=1;seed<=12;++seed){in=initial(&f);e=emitter(&f,&in,steam);CHECK(e&&e->query==bh_script_sensor_query&&e->step==(steam?bh_script_steam_tick:bh_script_fountain_tick),"real emitter step/query installed");
  srand(seed);for(i=0;i<(steam?5:3);++i)randoms[i]=rand();frame=(unsigned long)rand()&7;next=rand();srand(seed);used=f.pool.used;e->step(e);created=f.pool.active;p=bh_script_particle_state(&f.particles,created);
  CHECK(!present(&f,e)&&p&&f.pool.used==used&&created!=e,"forced expiry emits actual particle before releasing emitter");
  CHECK((unsigned long)rand()==next&&p->frame==frame,"forced emission bypasses probability draw and preserves entire shared random stream");
  CHECK(p->resource==(steam?456:123)&&p->bucket==3&&p->lifetime==(steam?3:1)&&p->size==(steam?.2f:.1f)&&p->alpha==1,"distinct complete emitter particle parameters");
  if(steam){v=in.velocity;v.x=(float)((double)(randoms[0]-16384)*in.spread+v.x);v.y=(float)((double)(randoms[1]-16384)*in.spread+v.y);v.z=(float)((double)(randoms[2]-16384)*in.spread+v.z);spin=(float)((double)(randoms[3]-16384)/131072.0);angle=(float)((double)randoms[4]/4096.0);rate=4;}
  else{v=in.velocity;spin=(float)((double)(randoms[0]-16384)/131072.0);rate=(float)(10+(double)randoms[1]/2048.0);angle=(float)((double)randoms[2]/4096.0);}
  CHECK(!memcmp(&p->velocity,&v,12)&&!memcmp(&p->acceleration,&in.acceleration,12)&&!memcmp(&created->position,&f.actor->position,12),"jittered steam/direct fountain motion and actor position exactly propagated");
  spin_step=(float)((double)spin/30.0);size_step=(float)((double)rate/30.0);
  CHECK(p->angle==angle&&p->angular_step==spin_step&&p->size_step==size_step&&p->size_decay==(steam?1:.98f)&&p->angular_decay==.95f,"original random argument order, binary32 argument stores and damping");
  bh_script_emitter_read(e,&out);CHECK(out.countdown<0,"expiry countdown stored before emitter release");bh_entity_release(&f.pool,created);
 }
 finish(&f);
}
static void activity_and_random(void){Fixture f;BHScriptEmitterState in,out;BHEntity*e,*p;unsigned long seed,expected_draw,next,used,count;int steam,gate,j;BHScriptTimerPacket packet;
 CHECK(fixture(&f),"activity/random fixture");
 for(steam=0;steam<2;++steam){in=initial(&f);in.countdown=1;e=emitter(&f,&in,steam);f.actors[0].active=0;srand(123);next=rand();srand(123);used=f.pool.used;e->step(e);e->refresh?e->refresh(e,NULL):(void)0;bh_script_emitter_read(e,&out);
  CHECK(out.countdown==1&&f.pool.used==used&&(unsigned long)rand()==next&&f.queue.count==9,"inactive actor freezes timer, generation, draws and random consumption");
  f.actors[0].owner=&f.actors[1];f.actors[1].active=-1;
  for(seed=1;seed<20;++seed){in.countdown=0;CHECK(bh_script_emitter_bind(&f.emitter,e,&in,steam),"reset unlimited emitter");srand(seed);gate=rand();expected_draw=gate<(steam?10000:11000);for(j=0;j<(expected_draw?(steam?6:4):0);++j)rand();next=rand();srand(seed);used=f.pool.used;e->step(e);
   CHECK(f.pool.used==used+expected_draw&&(unsigned long)rand()==next,"probability gate consumes one draw; emission consumes exact remaining constructor stream");bh_script_emitter_read(e,&out);CHECK(out.countdown==0,"unlimited timer unchanged");if(expected_draw){p=f.pool.active;CHECK(bh_script_particle_state(&f.particles,p)!=NULL,"probability hit creates real source-owned particle");bh_entity_release(&f.pool,p);}
  }
  packet.event=10;packet.actor=f.actor;count=f.pool.used;e->query(e,&packet);CHECK(e->step==bh_script_timer_delete_step()&&f.pool.used==count,"real sensor matching-actor callback defers cleanup");e->step(e);CHECK(f.pool.used==count-1,"real deferred deletion owner releases emitter");f.actors[0].owner=NULL;f.actors[0].active=1;
 }
 finish(&f);
}
static void steam_draw(void){Fixture f;BHScriptEmitterState s;BHEntity*e;BHDrawCommand*q;unsigned long frame,next;int r;float angle;
 CHECK(fixture(&f),"oriented steam refresh fixture");s=initial(&f);s.countdown=0;s.size=-7;e=emitter(&f,&s,1);srand(90);frame=(unsigned long)rand()&7;r=rand();next=rand();angle=(float)((double)r/4096.0);srand(90);e->refresh(e,NULL);q=&f.queue.commands[9];
 CHECK(f.queue.count==10&&q->kind==BH_DRAW_ORIENTED&&q->resource==456&&q->words[13]==frame&&(unsigned long)rand()==next,"actual oriented draw producer and exact two shared random calls");
 CHECK(word(q->words)==10&&word(q->words+1)==18.4f&&word(q->words+2)==-30,"draw follows actor with exact1.6 vertical subtraction");
 CHECK(word(q->words+3)==0&&word(q->words+4)==0&&word(q->words+5)==1&&word(q->words+6)==0&&word(q->words+7)==-1&&word(q->words+8)==0,"actual up/normal vectors");
 CHECK(word(q->words+9)==-7&&word(q->words+10)==-7&&word(q->words+11)==angle&&word(q->words+12)==1,"signed size, random angle and unit alpha");finish(&f);
}
int __cdecl bh_script_particles_checkpoint(unsigned long*count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;particle_cases();emitter_forced();activity_and_random();steam_draw();bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"script-particles\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_PARTICLES_STANDALONE
int main(void){return bh_script_particles_checkpoint(NULL);}
#endif
