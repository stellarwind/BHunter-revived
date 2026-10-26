#include "../src/bh_physics_effects.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL physics-effects %d: %s\n",__LINE__,s);}}while(0)
typedef struct Memory {unsigned long live;int fail;} Memory;
static void *__cdecl allocate(unsigned long size,void *user){Memory*m=user;void*p;if(m->fail)return NULL;p=malloc(size);if(p)++m->live;return p;}
static void __cdecl release(void*p,void*user){Memory*m=user;--m->live;free(p);}
typedef struct Fixture {
    BHEntityPool pool;BHDrawQueue queue;BHContactEffects effects;
    BHSoundQueue sound;BHNetworkTransport transport;BHPhysicsEffects context;
    BHWorldGeometry world;BHSweepHit hit;Memory memory;
} Fixture;
static int setup(Fixture*f){BHSceneAllocator allocator;unsigned long i;BHSoundResource*r;
    memset(f,0,sizeof(*f));allocator.allocate=allocate;allocator.release=release;allocator.user=&f->memory;
    if(!bh_entities_init(&f->pool,1,&allocator)||!bh_draw_queue_init(&f->queue,32,NULL)||
       !bh_contact_effects_init(&f->effects,&f->pool,&f->queue,101,102,103,104)||
       !bh_sound_queue_init(&f->sound,NULL,NULL,NULL)||!bh_network_transport_init(&f->transport,NULL,0))return 0;
    for(i=0;i<3;++i){r=bh_sound_resource_allocate(&f->sound);if(!r)return 0;r->streaming=1;r->base_frequency=100;}
    f->context.effects=&f->effects;f->context.sound=&f->sound;f->context.transport=&f->transport;
    f->context.impact_sound=0;f->context.quad_sound=1;f->context.rays_sound=2;
    f->context.peer_count=2;f->context.sender=0xabcd;
    f->world.maximum.x=f->world.maximum.y=f->world.maximum.z=100;
    f->hit.surface_present=1;f->hit.surface.contact.point.x=25;f->hit.surface.contact.point.y=50;f->hit.surface.contact.point.z=75;
    f->hit.surface.normal.y=1;return 1;
}
static void finish(Fixture*f){f->memory.fail=0;while(f->pool.active)CHECK(bh_entity_release(&f->pool,f->pool.active),"actual generated effect release");
    CHECK(bh_contact_effects_dispose(&f->effects)&&bh_entities_dispose(&f->pool)&&!f->memory.live,"real pool and persistent effect context disposed");
    CHECK(bh_sound_queue_dispose(&f->sound)&&bh_network_transport_dispose(&f->transport)&&bh_draw_queue_dispose(&f->queue),"real source sound/network/queue owners disposed without execution");
}
static unsigned long word(const unsigned char*p){return p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);}
static void packet_cases(void){Fixture f;BHSoundVoice*v;BHEntity*e;BHSparkState*s;int expected,next,i;float life[3];
    const unsigned char position[8]={0x34,0x12,0,0x60,0,0x80,0,0xa0};
    const unsigned char loud[4]={0,128,70,3},quiet[4]={255,0,64,1};
    CHECK(setup(&f),"actual owners and three streaming resource definitions");
    srand(1);for(i=0;i<3;++i){expected=rand();life[i]=(float)((double)expected/131072.0+.5625);for(expected=0;expected<8;++expected)rand();}next=rand();srand(1);
    CHECK(bh_physics_effects_run(&f.context,&f.hit,1,&f.world,0x1234),"whole loud effects block");
    CHECK(f.context.sound_requests==1&&f.context.spark_attempts==3&&f.context.sparks_created==3&&f.pool.used==3&&rand()==next,"outer lifetime and successful constructor consume exactly27 CRT draws");
    CHECK(f.context.packets==1&&f.context.last_packet_size==20&&f.context.last_packet[0]==6&&f.context.last_packet[1]==12&&f.context.last_packet[2]==0xcd&&f.context.last_packet[3]==0xab,"actual type6 envelope and unpadded size");
    CHECK(!memcmp(f.context.last_packet+8,position,8)&&!memcmp(f.context.last_packet+16,loud,4),"independent contact position and quantized loud payload bytes");
    CHECK(word(f.context.last_packet+4)==(word(f.context.last_packet)^word(position)^word(position+4)^word(loud)),"independent XOR checksum");
    CHECK(f.context.last_send==S_FALSE&&!f.transport.native_calls,"actual NULL-peer sender makes no native network call");
    v=bh_sound_voice(&f.sound,f.context.last_voice);CHECK(v&&v->resource->index==0&&v->frequency==110&&v->positioned&&v->moving&&!memcmp(&v->position,&f.hit.surface.contact.point,12)&&v->velocity.x==0&&v->velocity.y==0&&v->velocity.z==0,"actual source sound state has contact point and zero moving vector");
    CHECK(v&&fabs(v->volume-log((double)(float).525)*(-1000)/log(.5))<.0003,"actual source sound volume retains independently derived .525 level");
    for(e=f.pool.active;e;e=e->next){s=bh_contact_spark_state(&f.effects,e);CHECK(s!=NULL,"actual installed spark callbacks");if(s){int found=0;for(i=0;i<3;++i)if(s->lifetime==life[i])found=1;CHECK(found,"retained level sets original lifetime from outer random draw");}}
    finish(&f);
    CHECK(setup(&f),"quiet threshold owners");srand(1);for(i=0;i<9;++i)rand();next=rand();srand(1);
    CHECK(bh_physics_effects_run(&f.context,&f.hit,.5f,&f.world,0x1234)&&!f.context.sound_requests&&f.context.sparks_created==1&&rand()==next,"exact threshold emits one spark without sound");
    CHECK(!memcmp(f.context.last_packet+16,quiet,4)&&f.context.last_packet_size==20,"quiet sentinel payload is ff,00,40,01");finish(&f);
    CHECK(setup(&f),"world failure owners");CHECK(bh_physics_effects_run(&f.context,&f.hit,1,NULL,0)&&!f.context.packets&&f.context.sparks_created==3&&f.context.sound_requests==1,"NULL world suppresses only packet and retains subsequent particles");finish(&f);
    CHECK(setup(&f),"single peer owners");f.context.peer_count=1;CHECK(bh_physics_effects_run(&f.context,&f.hit,0,&f.world,0)&&!f.context.packets&&!f.context.sparks_created&&!f.context.sound_requests&&!f.pool.used,"zero level/single player has no effect, playback or packet");finish(&f);
}
static void special_cases(void){Fixture f;BHImpactState*s;BHSoundVoice*v;BHEntity*held;unsigned long flags;
    for(flags=2;flags<=6;flags+=2){CHECK(setup(&f),"special material owners");f.hit.surface.flags=flags;srand(1);
        CHECK(bh_physics_effects_run(&f.context,&f.hit,0,&f.world,0)&&f.pool.used==1&&f.context.sound_requests==1&&!f.context.packets&&!f.context.spark_attempts&&rand()==41,"special allocation/sound with no random, spark or packet path");
        s=bh_contact_impact_state(&f.effects,f.pool.active);v=bh_sound_voice(&f.sound,f.context.last_voice);
        CHECK(s&&s->size==((flags&2)?16:12)&&s->age==0&&s->duration==1&&!memcmp(&s->normal,&f.hit.surface.normal,12),"real special impact state and bit2 priority");
        CHECK(v&&v->resource->index==((flags&2)?1:2)&&v->frequency==100&&v->volume==((flags&2)?0:1000),"special actual sound IDs, unit pitch and original volume1/2");finish(&f);
    }
    CHECK(setup(&f),"allocation-failure owners");held=bh_entity_acquire(&f.pool);CHECK(held!=NULL,"fill actual initial pool");f.memory.fail=1;f.hit.surface.flags=2;srand(1);
    CHECK(!bh_physics_effects_run(&f.context,&f.hit,0,&f.world,0)&&!f.context.sound_requests&&f.pool.used==1&&rand()==41,"failed special allocation suppresses sound and consumes no random state");
    f.hit.surface.flags=0;srand(1);CHECK(!bh_physics_effects_run(&f.context,&f.hit,1,&f.world,0)&&f.context.spark_attempts==3&&!f.context.sparks_created&&f.context.sound_requests==1,"failed spark allocations retain all outer draws and preceding sound/packet effects");
    CHECK(rand()==26500,"three failed spark attempts consume exactly three outer draws");finish(&f);
}
int __cdecl bh_physics_effects_checkpoint(unsigned long*count){BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;packet_cases();special_cases();bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"physics-effects\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false,\"play_calls\":0,\"native_send_calls\":0}\n",checks,failures);return failures;
}
#ifdef BH_PHYSICS_EFFECTS_STANDALONE
int main(void){unsigned long count;return bh_physics_effects_checkpoint(&count);}
#endif
