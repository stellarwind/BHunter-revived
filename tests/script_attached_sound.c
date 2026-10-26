#include "../src/bh_script_attached_sound.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL attached-sound %lu: %s\n",checks,s);}}while(0)
typedef struct Memory{unsigned long calls,live,fail_at;}Memory;
static void *__cdecl alloc(unsigned long n,void *u){Memory*m=(Memory*)u;void*p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void*p,void*u){--((Memory*)u)->live;free(p);}
typedef struct Fixture{BHEntityPool pool;BHSoundQueue sounds;BHScriptLifetimeContext ctx;BHScriptTimerActor actors[2];BHEntity *entity,*actor,*owner;Memory memory;unsigned long resource;}Fixture;
static int fixture(Fixture*f){BHSceneAllocator a;BHSoundResource*r;memset(f,0,sizeof(*f));a.allocate=alloc;a.release=release;a.user=&f->memory;
 if(!bh_entities_init(&f->pool,4,&a)||!bh_sound_queue_init(&f->sounds,NULL,NULL,&a))return 0;
 f->actor=bh_entity_acquire(&f->pool);f->owner=bh_entity_acquire(&f->pool);f->entity=bh_entity_acquire(&f->pool);if(!f->actor||!f->owner||!f->entity)return 0;
 f->actors[0].entity=f->actor;f->actors[1].entity=f->owner;f->actors[0].active=1;
 if(!bh_script_timers_init(&f->ctx.timers,&f->pool,f->actors,2))return 0;f->ctx.sounds=&f->sounds;
 r=bh_sound_resource_allocate(&f->sounds);if(!r)return 0;r->streaming=1;r->base_frequency=11025;r->base_volume=-125;f->resource=r->index;
 f->actor->position.x=10;f->actor->position.y=-20;f->actor->position.z=30;
 return 1;
}
static void finish(Fixture*f){CHECK(bh_sound_queue_dispose(&f->sounds),"real sound queue disposes without playback");CHECK(bh_entities_dispose(&f->pool),"real entity pool disposal");CHECK(!f->memory.live,"all actual owned allocations released");}
static BHScriptAttachedSoundState initial(Fixture*f){BHScriptAttachedSoundState s;s.actor=f->actor;s.script_loaded=1;s.handle=BH_SOUND_INVALID;s.resource=f->resource;s.level=s.target=0;return s;}
static BHScriptAttachedSoundState read(Fixture*f){BHScriptAttachedSoundState s;memset(&s,0,sizeof(s));CHECK(bh_script_attached_sound_read(f->entity,&s),"read actual verified state");return s;}
static void verify_voice(Fixture*f,unsigned long handle,float gain){BHSoundVoice*v=bh_sound_voice(&f->sounds,handle);double db=-125+log((double)gain)*(-1000.0)/log(.5);
 CHECK(v&&v->resource==f->sounds.resources[f->resource],"actual resource and generated sound handle");
 if(v){CHECK(v->positioned&&v->moving&&!memcmp(&v->position,&f->actor->position,12),"actual actor spatial position and non-NULL zero velocity");CHECK(v->velocity.x==0&&v->velocity.y==0&&v->velocity.z==0&&v->frequency==11025,"unit pitch and exact zero velocity");CHECK(fabs((double)v->volume-db)<.001,"actual native-independent queued gain converted by real sound owner");}
}
static void ramp(void){Fixture f;BHScriptAttachedSoundState s;BHSoundVoice*v;unsigned long handle,used,i;BHVec3 retained;BHScriptTimerPacket p;
 CHECK(fixture(&f),"persistent actor table, actual entity pool and real streaming sound resource");s=initial(&f);used=f.pool.used;
 CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"bind real attached step and actual cleanup query");
 CHECK(f.entity->step==bh_script_attached_sound_tick&&f.entity->query==bh_script_sound_actor_query,"both installed methods are real complete owners");
 f.entity->step(f.entity);s=read(&f);CHECK(s.level==0&&s.target==15&&s.handle==BH_SOUND_INVALID&&!f.sounds.resources[0]->stream_head,"first tick selects activity target after counter comparison");
 f.entity->step(f.entity);s=read(&f);handle=s.handle;CHECK(s.level==1&&s.target==15,"rising1 requests instead of updating");verify_voice(&f,handle,0.06666667014360428f);
 for(i=2;i<=15;++i){float gain=(float)((double)i/15.0);f.actor->position.x=(float)i;f.entity->step(f.entity);s=read(&f);CHECK(s.level==(long)i&&s.handle==handle,"ramp retains one generated voice");verify_voice(&f,handle,gain);}
 v=bh_sound_voice(&f.sounds,handle);retained=v->position;f.actor->position.x=77;f.entity->step(f.entity);CHECK(!memcmp(&v->position,&retained,12),"equal counter performs no spatial update");
 f.actors[0].active=0;f.entity->step(f.entity);s=read(&f);CHECK(s.level==15&&s.target==0,"activity-off changes target after equal-counter no-op");
 for(i=14;i>0;--i){float gain=(float)((double)i/15.0);f.entity->step(f.entity);s=read(&f);CHECK(s.level==(long)i&&s.target==0&&s.handle==handle,"falling nonzero updates existing voice");verify_voice(&f,handle,gain);}
 f.entity->step(f.entity);s=read(&f);CHECK(!s.level&&!s.target&&s.handle==BH_SOUND_INVALID&&!bh_sound_voice(&f.sounds,handle),"falling0 actually stops voice then invalidates field");
 CHECK(f.pool.used==used,"attached tick never releases its entity");
 f.actors[0].owner=&f.actors[1];f.actors[0].active=0;f.actors[1].active=-3;f.entity->step(f.entity);s=read(&f);CHECK(s.target==15,"one-level owner overrides actor activity and preserves any nonzero signed activity");
 f.entity->step(f.entity);s=read(&f);handle=s.handle;f.actors[0].active=1;f.actors[1].active=0;f.entity->step(f.entity);s=read(&f);CHECK(s.level==2&&s.target==0,"owner-off sampled after one upward step");
 p.event=9;p.actor=f.owner;f.entity->query(f.entity,&p);CHECK(bh_sound_voice(&f.sounds,handle)!=NULL,"real cleanup query compares original actor identity, not activity owner");
 p.actor=f.actor;f.entity->query(f.entity,&p);CHECK(!bh_sound_voice(&f.sounds,handle)&&f.entity->step==bh_script_timer_delete_step(),"actual event9 query stops and defers release");f.entity->step(f.entity);CHECK(f.pool.used==used-1,"real timer delete releases after query");finish(&f);
}
static void edge_cases(void){Fixture f;BHScriptAttachedSoundState s;unsigned long handle,bytes_before[4],bytes_after[4];BHScriptTimerPacket p;
 CHECK(fixture(&f),"edge-case fixture");s=initial(&f);s.level=0;s.target=1;s.resource=BH_SOUND_INVALID;
 CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"invalid resource remains original request result, no fake voice");f.entity->step(f.entity);s=read(&f);CHECK(s.level==1&&s.handle==BH_SOUND_INVALID&&s.target==15,"failed request increments counter then selects target15");
 s.level=s.target=15;s.handle=0x12345678UL;f.actors[0].active=0;CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"bind stale non--1 handle");f.entity->step(f.entity);s=read(&f);CHECK(s.target==0&&s.handle==0x12345678UL,"stale handle remains distinct from -1");
 s.level=1;s.target=0;CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"bind stale handle reaching zero");f.entity->step(f.entity);s=read(&f);CHECK(s.level==0&&s.handle==BH_SOUND_INVALID,"stale Stop failure still invalidates original handle field");
 s=initial(&f);s.target=1;CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"valid resource after request failure");f.entity->step(f.entity);s=read(&f);handle=s.handle;CHECK(handle!=BH_SOUND_INVALID,"source sound request succeeds without playback");
 s.level=LONG_MAX;s.target=LONG_MAX;CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"signed maximum equality");f.entity->step(f.entity);s=read(&f);CHECK(s.level==LONG_MAX&&s.target==0,"signed maximum does not increment at equality");
 s.level=LONG_MIN;s.target=LONG_MIN;CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"signed minimum equality");f.entity->step(f.entity);s=read(&f);CHECK(s.level==LONG_MIN,"signed minimum equal counters do not mutate");
 memcpy(bytes_before,(unsigned char*)f.entity+0x128,4);memcpy(bytes_before+1,(unsigned char*)f.entity+0x130,12);
 s.level=s.target=0;CHECK(bh_script_attached_sound_bind(&f.ctx,f.entity,&s),"binding retains unrelated verified payload state");memcpy(bytes_after,(unsigned char*)f.entity+0x128,4);memcpy(bytes_after+1,(unsigned char*)f.entity+0x130,12);CHECK(!memcmp(bytes_before,bytes_after,16),"binding does not erase unrelated countdown/motion state");
 p.event=20;p.actor=f.actor;f.entity->query(f.entity,&p);CHECK(!bh_sound_voice(&f.sounds,handle),"actual unconditional event20 cleanup stops source queued voice");finish(&f);
}
int __cdecl bh_script_attached_sound_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;ramp();edge_cases();bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"script-attached-sound\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_ATTACHED_SOUND_STANDALONE
int main(void){return bh_script_attached_sound_checkpoint(NULL);}
#endif
