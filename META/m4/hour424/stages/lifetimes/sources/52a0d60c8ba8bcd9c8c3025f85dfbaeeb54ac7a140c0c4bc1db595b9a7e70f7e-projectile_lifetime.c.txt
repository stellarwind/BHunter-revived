#include "../src/bh_projectile_lifetime.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL projectile-lifetime %lu: %s\n",checks,s);}}while(0)
typedef struct Fixture {BHEntityPool pool;BHSoundQueue sound;BHProjectileLifetime ctx;BHEntity *entity,*target,*secondary;unsigned long handle,other;BHProjectileQueryState state;} Fixture;
static int fixture(Fixture *f){BHSoundResource *r;memset(f,0,sizeof(*f));
 if(!bh_entities_init(&f->pool,4,NULL)||!bh_sound_queue_init(&f->sound,NULL,NULL,NULL))return 0;
 f->entity=bh_entity_acquire(&f->pool);f->target=bh_entity_acquire(&f->pool);f->secondary=bh_entity_acquire(&f->pool);if(!f->entity||!f->target||!f->secondary)return 0;
 r=bh_sound_resource_allocate(&f->sound);if(!r)return 0;r->streaming=1;r->base_frequency=8000;
 f->handle=bh_sound_request(&f->sound,r->index,1,1,NULL,NULL);f->other=bh_sound_request(&f->sound,r->index,.5f,1,NULL,NULL);if(f->handle==BH_SOUND_INVALID||f->other==BH_SOUND_INVALID)return 0;
 f->ctx.pool=&f->pool;f->ctx.sound=&f->sound;f->state.countdown=1;f->state.sound=f->handle;f->state.identity=123;f->state.target=f->target;f->state.secondary=f->secondary;
 return bh_projectile_lifetime_bind(&f->ctx,f->entity,&f->state);
}
static void finish(Fixture *f){CHECK(bh_sound_queue_dispose(&f->sound),"actual sound ownership disposed without Play");CHECK(bh_entities_dispose(&f->pool),"actual entity pool disposed");}
static void cleanup(void){Fixture f;unsigned long used,g;unsigned char payload[636];BHVec3 position;BHProjectileQueryState state;BHProjectileSyncPacket p;
 CHECK(fixture(&f),"actual pool and two streaming sound voices");position=f.entity->position;memcpy(payload,f.entity->payload,sizeof(payload));used=f.pool.used;g=f.entity->generation;
 bh_projectile_cleanup(f.entity);CHECK(f.pool.used==used-1&&f.entity->generation==g+1&&!bh_sound_voice(&f.sound,f.handle),"whole owner stops real sound handle then releases actual entity");CHECK(bh_sound_voice(&f.sound,f.other)!=NULL,"unrelated real sound voice retained");
 CHECK(!memcmp(payload,f.entity->payload,sizeof(payload))&&!memcmp(&position,&f.entity->position,12),"cleanup retains every payload byte including stale118 handle and position");bh_projectile_cleanup(f.entity);CHECK(f.pool.used==used-1&&f.ctx.error==1,"source guard rejects double cleanup before sound/pool effects");finish(&f);
 CHECK(fixture(&f),"stale-handle cleanup fixture");state=f.state;state.sound=BH_SOUND_INVALID;CHECK(bh_projectile_lifetime_bind(&f.ctx,f.entity,&state),"original invalid handle retained");used=f.pool.used;bh_projectile_cleanup(f.entity);CHECK(f.pool.used==used-1&&bh_sound_voice(&f.sound,f.handle)&&f.sound.error==BH_SOUND_STALE,"ignored Stop failure still releases entity and keeps other voices");finish(&f);
 CHECK(fixture(&f),"event20 deferred cleanup fixture");memset(&p,0,sizeof(p));p.event=20;used=f.pool.used;f.entity->query(f.entity,&p);
 CHECK(f.entity->step==bh_projectile_cleanup&&!f.entity->query&&!f.entity->refresh&&f.pool.used==used&&bh_entities_valid(&f.pool),"actual event20 unregisters query and clears refresh before deferring real cleanup");
 CHECK(bh_entities_step(&f.pool)&&f.pool.used==used-1&&!bh_sound_voice(&f.sound,f.handle),"actual source dispatcher invokes whole deferred cleanup");finish(&f);
}
static void packets(void){Fixture f;BHProjectileSyncPacket p,before;BHProjectileQueryState out;float times[4]={1,.125f,.2f,0};unsigned long i,bits=0x7fc01234UL;BHEntity saved;
 CHECK(fixture(&f),"packet fixture");memcpy(times+3,&bits,4);
 for(i=0;i<4;++i){f.state.countdown=times[i];f.state.identity=123;bh_projectile_lifetime_bind(&f.ctx,f.entity,&f.state);memset(&p,0x5a,sizeof(p));p.event=18;p.identity=123;p.kind=3;p.position.x=7;p.position.y=8;p.position.z=9;before=p;
  f.entity->query(f.entity,&p);bh_projectile_query_state(f.entity,&out);CHECK(p.identity==0xffffffffUL&&p.event==before.event&&p.kind==before.kind&&p.reserved==before.reserved&&!memcmp(&p.position,&before.position,12),"matching packet acknowledges only identity before kind3 application");
  CHECK(out.identity==1&&!memcmp(&f.entity->position,&p.position,12),"kind3 sync copies all3 position words and sets entity identity1");CHECK(i==3?_isnan(out.countdown):(out.countdown==(i==0?.2f:times[i])),"countdown clamp preserves low/equal and unordered values");
 }
 bh_projectile_lifetime_bind(&f.ctx,f.entity,&f.state);p.event=18;p.identity=999;p.kind=3;saved=*f.entity;before=p;f.entity->query(f.entity,&p);CHECK(!memcmp(&p,&before,sizeof(p))&&!memcmp(&saved,f.entity,sizeof(saved)),"mismatched identity leaves packet and entity untouched");
 p.identity=f.state.identity;p.kind=2;before=p;saved=*f.entity;f.entity->query(f.entity,&p);CHECK(p.identity==0xffffffffUL&&!memcmp(&saved,f.entity,sizeof(saved)),"matching nonkind3 still acknowledges packet but retains every entity byte");
 p.event=17;memcpy(&p.identity,&f.target,4);f.entity->query(f.entity,&p);bh_projectile_query_state(f.entity,&out);CHECK(!out.target&&out.secondary==f.secondary,"event17 clears only matching primary reference");
 f.state.target=f.state.secondary=f.target;bh_projectile_lifetime_bind(&f.ctx,f.entity,&f.state);f.entity->query(f.entity,&p);bh_projectile_query_state(f.entity,&out);CHECK(!out.target&&!out.secondary,"event17 independently clears both equal references");
 p.event=8;saved=*f.entity;before=p;f.entity->query(f.entity,&p);CHECK(!memcmp(&saved,f.entity,sizeof(saved))&&!memcmp(&before,&p,sizeof(p)),"unhandled packet is a complete no-effect branch");finish(&f);
}
int __cdecl bh_projectile_lifetime_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;cleanup();packets();bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"projectile-lifetime\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_PROJECTILE_LIFETIME_STANDALONE
int main(void){unsigned long count;return bh_projectile_lifetime_checkpoint(&count);}
#endif
