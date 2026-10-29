#include "../src/bh_script_lifetime.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL script-lifetime %lu: %s\n",checks,s);}}while(0)
typedef struct Memory {unsigned long calls,live,fail_at;} Memory;
static void *__cdecl alloc(unsigned long n,void *u){Memory *m=(Memory*)u;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl free_memory(void *p,void *u){--((Memory*)u)->live;free(p);}
typedef struct Fixture {BHEntityPool pool;BHSoundQueue sounds;BHScriptLifetimeContext ctx;Memory mem;BHEntity *actor,*other;} Fixture;
static int init(Fixture *f){BHSceneAllocator a;BHSoundResource *r;unsigned long i;memset(f,0,sizeof(*f));
 a.allocate=alloc;a.release=free_memory;a.user=&f->mem;
 if(!bh_entities_init(&f->pool,4,&a)||!bh_sound_queue_init(&f->sounds,NULL,NULL,&a))return 0;
 f->actor=bh_entity_acquire(&f->pool);f->other=bh_entity_acquire(&f->pool);
 if(!f->actor||!f->other)return 0;
 if(!bh_script_timers_init(&f->ctx.timers,&f->pool,NULL,0))return 0;
 f->ctx.sounds=&f->sounds;
 for(i=0;i<2;++i){r=bh_sound_resource_allocate(&f->sounds);if(!r)return 0;
  r->streaming=1;r->base_frequency=8000;r->base_volume=0;f->ctx.mission_sound[i]=r->index;}
 return 1;
}
static void finish(Fixture *f){CHECK(bh_sound_queue_dispose(&f->sounds),"real sound ownership disposed");CHECK(bh_entities_dispose(&f->pool),"real entity pool disposed");CHECK(!f->mem.live,"all owned allocations freed");}
static unsigned long voice(Fixture *f){return bh_sound_request(&f->sounds,0,.5f,1,NULL,NULL);}
static int present(Fixture *f,BHEntity *e){BHEntity *p;for(p=f->pool.active;p;p=p->next)if(p==e)return 1;return 0;}
static BHScriptLifetimeState state(Fixture *f){BHScriptLifetimeState s;memset(&s,0,sizeof(s));s.actor=f->actor;s.script_loaded=1;s.sound=voice(f);s.countdown=1;s.mission_countdown=4;s.delta.x=1;s.delta.y=-2;s.delta.z=4;return s;}
static void query_cases(void){Fixture f;BHEntityCallback callbacks[5];long events[7]={0,4,8,9,10,20,14};unsigned long c,i,l,m,used;BHEntity *e;BHScriptTimerPacket p,before;BHScriptLifetimeState s;int immediate,deferred,stopped;
 callbacks[0]=bh_script_unloaded_query;callbacks[1]=bh_script_motion_query;callbacks[2]=bh_script_sensor_query;callbacks[3]=bh_script_sound_query;callbacks[4]=bh_script_sound_actor_query;
 CHECK(init(&f),"real pool/source streaming resource context");
 for(c=0;c<5;++c)for(i=0;i<7;++i)for(l=0;l<2;++l)for(m=0;m<2;++m){
  e=bh_entity_acquire(&f.pool);s=state(&f);s.script_loaded=l;f.actor->origin.x=10;f.actor->origin.y=20;f.actor->origin.z=30;
  CHECK(e&&s.sound!=BH_SOUND_INVALID&&bh_script_lifetime_bind(&f.ctx,e,&s,callbacks[c]),"bind actual query and real streaming voice");
  p.event=events[i];p.actor=m?f.actor:f.other;before=p;used=f.pool.used;
  immediate=c==0?(p.event==20&&!l):(c==4?(p.event==4||p.event==20):(p.event==4||(p.event==20&&!l)));
  deferred=!immediate&&((c==1||c==2)?(p.event==10&&m):(c==4&&p.event==9&&m));
  stopped=(c==3&&immediate)||(c==4&&(immediate||deferred));
  e->query(e,&p);
  CHECK(!memcmp(&p,&before,sizeof(p)),"query packet preserved");
  CHECK(f.pool.used==used-(immediate!=0)&&present(&f,e)==!immediate,"event-specific immediate ownership");
  CHECK((bh_sound_voice(&f.sounds,s.sound)==NULL)==stopped,"only specified sound callbacks stop the actual queued voice");
  CHECK(f.actor->origin.x==((c==1&&deferred)?9:10)&&f.actor->origin.y==((c==1&&deferred)?22:20)&&f.actor->origin.z==((c==1&&deferred)?26:30),"only matching motion cleanup reverses actor-origin delta");
  if(!immediate){CHECK((e->step==bh_script_timer_delete_step())==deferred,"actual deferred delete identity");if(deferred){e->step(e);CHECK(!present(&f,e),"real admitted delete owner releases using persistent timer binding");}else bh_entity_release(&f.pool,e);}
  if(!stopped)bh_sound_stop(&f.sounds,s.sound);
 }
 finish(&f);
}
static void expiry_cases(void){Fixture f;BHScriptLifetimeState s,read;BHEntity *e;unsigned long i,nan_bits=0x7fc01234UL;float cases[6]={1,0,-1,.03125f,.0625f,0};int expires;
 memcpy(cases+5,&nan_bits,4);CHECK(init(&f),"expiry fixture");
 for(i=0;i<6;++i){e=bh_entity_acquire(&f.pool);s=state(&f);s.countdown=cases[i];CHECK(bh_script_lifetime_bind(&f.ctx,e,&s,NULL),"bind timer state");
  expires=!(cases[i]>1.0/30.0);bh_script_sound_expiry(e);
  CHECK(present(&f,e)==!expires,"pre-store ordered/unordered expiration branch");CHECK((bh_sound_voice(&f.sounds,s.sound)==NULL)==expires,"expiry uses real sound-stop owner");
  if(!expires){CHECK(bh_script_lifetime_read(e,&read)&&read.countdown<cases[i],"retained rounded countdown");bh_sound_stop(&f.sounds,s.sound);bh_entity_release(&f.pool,e);}
 }
 finish(&f);
}
static void mission_cases(void){Fixture f;BHEntity *e,*first,*second;BHScriptLifetimeState s;BHSoundVoice *v;unsigned long i,count;float before;
 CHECK(init(&f),"mission fixture");
 e=bh_entity_acquire(&f.pool);before=.03125f;i=1;memcpy((char*)e+0x180,&before,4);memcpy((char*)e+0x104,&i,4);e->step=bh_mission_sound_tick;
 CHECK(bh_script_lifetime_bind_context(&f.ctx,e),"bind private cue context without legacy-state snapshot");
 CHECK(bh_script_lifetime_read(e,&s)&&s.mission_countdown==before&&s.mission_resource==1&&e->step==bh_mission_sound_tick&&!e->query,"private binding preserves cue publication");
 count=f.pool.used;e->step(e);v=f.sounds.resources[1]->stream_head;
 CHECK(!present(&f,e)&&f.pool.used==count-1&&v&&v->resource==f.sounds.resources[1],"cue with private-only binding expires through actual sound and pool owners");
 if(v)bh_sound_stop(&f.sounds,(unsigned long)v->index+((unsigned long)v->generation<<16));
 CHECK(!bh_script_lifetime_bind_context(&f.ctx,e),"inactive cue rejects private binding");
 first=bh_mission_sound_first(&f.ctx);second=bh_mission_sound_second(&f.ctx);
 CHECK(first&&second&&first->step==bh_mission_sound_tick&&second->step==bh_mission_sound_tick,"distinct factories install whole tick owner");
 for(i=0;i<2;++i){e=i?second:first;CHECK(bh_script_lifetime_read(e,&s)&&s.mission_countdown==4&&s.mission_resource==i,"four-second countdown and distinct actual resource binding");
  before=s.mission_countdown;e->step(e);bh_script_lifetime_read(e,&s);CHECK(s.mission_countdown<before&&present(&f,e)&&!f.sounds.resources[i]->stream_head,"positive timer retains entity without requesting sound");
  s.mission_countdown=.03125f;CHECK(bh_script_lifetime_bind(&f.ctx,e,&s,NULL),"bind near-expiry value retaining real step");count=f.pool.used;e->step(e);
  v=f.sounds.resources[i]->stream_head;CHECK(f.pool.used==count-1&&!present(&f,e)&&v&&v->resource==f.sounds.resources[i],"expiry queues correct real resource and releases entity");
  CHECK(v&&v->frequency==8000&&v->volume==0&&!v->positioned&&!v->moving,"request retains unit gain/pitch and null spatial arguments");
 }
 /* Fill current pool chunk so the next factory must call the real allocator. */
 while(f.pool.free_slots)CHECK(bh_entity_acquire(&f.pool)!=NULL,"fill actual pool");
 f.mem.fail_at=f.mem.calls+1;count=f.pool.used;CHECK(!bh_mission_sound_first(&f.ctx)&&f.pool.used==count,"first factory real growth allocation failure");
 f.mem.fail_at=f.mem.calls+1;CHECK(!bh_mission_sound_second(&f.ctx)&&f.pool.used==count,"second factory real growth allocation failure");f.mem.fail_at=0;finish(&f);
}
int __cdecl bh_script_lifetime_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
 query_cases();expiry_cases();mission_cases();bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"script-lifetime\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_LIFETIME_STANDALONE
int main(void){unsigned long n;return bh_script_lifetime_checkpoint(&n);}
#endif
