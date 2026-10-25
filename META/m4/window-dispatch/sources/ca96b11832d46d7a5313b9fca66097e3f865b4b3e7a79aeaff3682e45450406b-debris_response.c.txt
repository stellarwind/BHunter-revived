#include "../src/bh_debris_response.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL debris %d: %s\n",__LINE__,label);}} while(0)
static unsigned long word(float value) {unsigned long bits;memcpy(&bits,&value,4);return bits;}
static void identity(BHPhysicsState *s) {
    memset(s,0,sizeof(*s));s->pose.m[0][0]=s->pose.m[1][1]=s->pose.m[2][2]=1;
}
static unsigned long wave_fixture(unsigned char *bytes) {
    unsigned long riff=100,format_size=16,sample_rate=8000,byte_rate=16000,samples=64;
    unsigned short pcm=1,mono=1,align=2,bits=16;
    memset(bytes,0,108);memcpy(bytes,"RIFF",4);memcpy(bytes+4,&riff,4);memcpy(bytes+8,"WAVEfmt ",8);
    memcpy(bytes+16,&format_size,4);memcpy(bytes+20,&pcm,2);memcpy(bytes+22,&mono,2);
    memcpy(bytes+24,&sample_rate,4);memcpy(bytes+28,&byte_rate,4);memcpy(bytes+32,&align,2);memcpy(bytes+34,&bits,2);
    memcpy(bytes+36,"data",4);memcpy(bytes+40,&samples,4);return 108;
}
static void cases(void) {
    BHEntityPool pool;BHEntity *a,*b,*c,*registry[4];
    BHDebrisContext ctx;BHDebrisBody body,other;BHPhysicsState state,other_state,saved;
    BHSoundQueue sound;BHWave wave;unsigned char bytes[108];LPDIRECTSOUND device=NULL;
    BHSoundVoice *voice;HRESULT hr;unsigned long sound_id,handle,old_count,generation;
    BHVec3 inertia={2,3,4},acceleration={0,0,0};
    BHDebrisBlastPacket blast;BHDebrisOwnerPacket detach;
    BHPhysicsContactPacket contact;BHSweepHit hit;BHPhysicsResponse response;
    int expected_random,actual_random;long event;
    memset(&body,0,sizeof(body));memset(&other,0,sizeof(other));
    CHECK(bh_entities_init(&pool,4,NULL),"source entity pool");
    a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);c=bh_entity_acquire(&pool);
    CHECK(a && b && c,"source entity lifetimes");
    if(!a || !b || !c) {bh_entities_dispose(&pool);return;}
    registry[0]=b;registry[1]=a;registry[2]=c;registry[3]=NULL;
    hr=DirectSoundCreate(NULL,&device,NULL);CHECK(hr==DS_OK && device,"real native sound dependency");
    if(hr!=DS_OK || !device) {bh_entities_dispose(&pool);return;}
    CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK,"native normal sound ownership");
    CHECK(bh_sound_queue_init(&sound,device,NULL,NULL),"source sound queue");
    CHECK(bh_wave_init(&wave,NULL) && bh_wave_decode(&wave,bytes,wave_fixture(bytes)),"actual PCM decode");
    sound_id=bh_sound_register_wave(&sound,&wave,0,3,0);
    CHECK(sound_id!=BH_SOUND_INVALID,"native uploaded buffer; no playback");
    if(sound_id==BH_SOUND_INVALID) {bh_wave_dispose(&wave);bh_sound_queue_dispose(&sound);IDirectSound_Release(device);bh_entities_dispose(&pool);return;}
    CHECK(!bh_debris_context_init(&ctx,&pool,&sound,sound_id,registry,3),"bounded registry rejects missing sentinel");
    CHECK(bh_debris_context_init(&ctx,&pool,&sound,sound_id,registry,4),"explicit real owners");
    identity(&state);identity(&other_state);a->position.x=a->position.y=a->position.z=0;
    CHECK(bh_debris_bind(&ctx,&body,a,&state,2,.5f,&inertia),"real query callback installed");
    CHECK(!bh_debris_bind(&ctx,&body,a,&state,2,.5f,&inertia),"binding cannot be overwritten");
    CHECK(bh_debris_bind(&ctx,&other,b,&other_state,2,.5f,&inertia),"second real body");
    CHECK(!bh_debris_context_dispose(&ctx),"context retained while live callbacks borrow it");ctx.error=0;
    memset(&contact,0,sizeof(contact));contact.event=2;contact.point.y=2;contact.normal.x=1;contact.impulse=2;contact.severity=.5f;
    state.angular_velocity.x=2;state.angular_velocity.y=4;state.angular_velocity.z=6;body.sleep_time=3;
    srand(73);expected_random=rand();srand(73);
    a->query(a,&contact);actual_random=rand();
    CHECK(state.acceleration.x==-900 && state.acceleration.y==0 && state.acceleration.z==0,"closed-form contact linear response");
    CHECK(state.angular_acceleration.x==0 && state.angular_acceleration.y==0 && state.angular_acceleration.z==270,"off-center contact torque divided by body inertia");
    CHECK(fabs(state.angular_velocity.x-1.7)<1e-6 && fabs(state.angular_velocity.y-3.4)<1e-6 && fabs(state.angular_velocity.z-5.1)<1e-6,"spin damped by original double .85");
    CHECK(body.sleep_time==0 && expected_random==actual_random,"response wakes only after completion and consumes no RNG");
    voice=bh_sound_voice(&sound,body.last_sound);
    CHECK(voice && voice->frequency==8000 && voice->position.y==2 && voice->moving && voice->velocity.x==0 && voice->pending,"receiver queues real positioned zero-velocity sound");
    CHECK(voice && fabs(voice->volume+1000)<.001,"severity passes to real logarithmic level conversion");
    CHECK(bh_sound_stop(&sound,body.last_sound)==1,"real queued sound cleanup");
    /* Rotate body +90 degrees around Z: world X normal becomes local -Y;
       contact along world Y becomes local X, yielding the same Z torque. */
    identity(&state);state.pose.m[0][0]=state.pose.m[1][1]=0;state.pose.m[0][1]=-1;state.pose.m[1][0]=1;
    contact.severity=.03125f;handle=body.last_sound;old_count=sound.voice_count;
    a->query(a,&contact);
    CHECK(state.angular_acceleration.z==270 && state.acceleration.x==-900,"body inverse rotation affects torque but world force keeps direction");
    CHECK(body.last_sound==handle && sound.voice_count==old_count,"below-threshold severity does not request audio");
    identity(&state);contact.point.y=0;contact.severity=.05f;
    a->query(a,&contact);
    CHECK(state.angular_acceleration.x==0 && state.angular_acceleration.y==0 && state.angular_acceleration.z==0,"zero arm leaves normalizer output zero and produces no torque");
    CHECK(bh_sound_voice(&sound,body.last_sound)!=NULL,"float .05 exceeds original double .05 threshold");
    CHECK(bh_sound_stop(&sound,body.last_sound)==1,"threshold request cleanup");
    identity(&state);body.friction=.13f;contact.impulse=.23f;
    contact.normal.x=contact.normal.y=.17f;contact.severity=0;
    state.acceleration.x=state.acceleration.y=4.574699878692627f;
    a->query(a,&contact);
    CHECK(word(state.acceleration.x)==0xb3b5b573UL && state.acceleration.y==0,
        "PC53 keeps X force product while Y reloads its original float spill");
    body.friction=.5f;contact.impulse=2;contact.normal.x=1;contact.normal.y=0;
    ctx.contact_sound=BH_SOUND_INVALID;body.sleep_time=3;identity(&state);contact.severity=.5f;
    a->query(a,&contact);
    CHECK(state.acceleration.x==-900 && body.sleep_time==0 && body.last_sound==BH_SOUND_INVALID,"sound rejection does not roll back physical response or wake");ctx.contact_sound=sound_id;
    identity(&state);a->position.x=3;a->position.y=4;
    memset(&blast,0,sizeof(blast));blast.event=1;blast.strength=2;blast.radius=10;body.sleep_time=3;
    a->query(a,&blast);
    CHECK(state.acceleration.x==9 && state.acceleration.y==12 && state.acceleration.z==0 && body.sleep_time==0,"3-4-5 radial blast analytic falloff");
    saved=state;blast.radius=5;body.sleep_time=3;a->query(a,&blast);
    CHECK(!memcmp(&state,&saved,sizeof(state)) && body.sleep_time==3,"blast at exact radius returns before wake");
    blast.radius=-1;a->query(a,&blast);CHECK(body.sleep_time==3,"negative radius also misses");
    blast.radius=10;body.sleep_time=2;a->query(a,&blast);CHECK(body.sleep_time==2,"wake comparison is strictly greater than two");
    a->position.x=a->position.y=0;identity(&state);a->query(a,&blast);
    CHECK(_isnan(state.acceleration.x) && _isnan(state.acceleration.y) && _isnan(state.acceleration.z),"masked zero-distance original division retained");
    identity(&state);body.mass=0;a->position.x=1;a->query(a,&blast);
    CHECK(!_finite(state.acceleration.x),"zero-mass blast division retained");body.mass=2;a->position.x=0;
    identity(&state);body.inertia.x=0;contact.severity=0;a->query(a,&contact);
    CHECK(_isnan(state.angular_acceleration.x),"zero inertia retains original masked 0/0");body.inertia=inertia;
    identity(&state);body.sleep_time=3;body.owner=c;detach.event=17;detach.owner=b;
    a->query(a,&detach);CHECK(body.owner==c && body.sleep_time==3,"foreign detach ignored without wake");
    detach.owner=c;a->query(a,&detach);CHECK(!body.owner && body.sleep_time==3,"matching detach clears only owner");
    event=6;a->query(a,&event);CHECK(!a->step && body.sleep_time==3,"stop clears tick and returns without wake");
    event=3;saved=state;a->query(a,&event);CHECK(!memcmp(&saved,&state,sizeof(state)) && body.sleep_time==3,"unsupported position event ignored");
    /* Consume an actual source-produced packet, rather than hand-building all
       tests. The complete physics owner remains WIP; this checks its interface. */
    memset(&hit,0,sizeof(hit));hit.surface_present=1;hit.motion.x=-2;hit.surface.normal.x=1;hit.surface.contact.point.y=2;
    CHECK(bh_physics_contact_response(&hit,1,.5f,2,-1,&acceleration,&response) && response.emit_first,"source physics packet producer");
    identity(&state);a->query(a,&response.first);
    CHECK(state.acceleration.x==900 && state.angular_acceleration.z==-270,"real event-2 packet consumed by recovered receiver");
    if(bh_sound_voice(&sound,body.last_sound))CHECK(bh_sound_stop(&sound,body.last_sound)==1,"producer packet sound cleanup");
    event=20;a->refresh=bh_debris_receive;generation=a->generation;a->query(a,&event);
    CHECK(a->step==bh_debris_remove && !a->query && !a->refresh && body.entity==a && pool.used==3,"retirement removes query/refresh and defers release");
    registry[3]=c;bh_debris_remove(a);
    CHECK(ctx.error && body.entity==a && pool.used==3,"mutated registry without sentinel rejects before removal");
    registry[3]=NULL;ctx.error=0;
    CHECK(bh_entities_valid(&pool) && bh_entities_step(&pool),"source pool executes deferred removal");
    CHECK(!body.entity && pool.used==2 && a->generation==generation+1 && registry[0]==b && registry[1]==c && !registry[2],"first registry occurrence shifts through sentinel then real release");
    registry[0]=c;registry[1]=NULL;generation=b->generation;bh_debris_remove(b);
    CHECK(!other.entity && pool.used==1 && b->generation==generation+1 && registry[0]==c && !registry[1],"absent registry entry still releases body");
    b=bh_entity_acquire(&pool);identity(&state);
    CHECK(b && bh_debris_bind(&ctx,&body,b,&state,2,.5f,&inertia),"recycled source slot takes a fresh generation binding");
    CHECK(bh_debris_unbind(&body) && !body.entity && !b->query && pool.used==2,"explicit unbind removes callback without releasing caller-owned entity");
    CHECK(bh_entities_valid(&pool) && bh_debris_context_dispose(&ctx),"released callbacks no longer borrow context");
    CHECK(bh_sound_queue_dispose(&sound),"native/source sound owners released");bh_wave_dispose(&wave);
    CHECK(IDirectSound_Release(device)==0,"final native device reference released");
    CHECK(bh_entities_dispose(&pool),"source pool disposal");
}
int bh_debris_response_checkpoint(unsigned long *count) {
    checks=0;failed=0;cases();*count=checks;
    printf("{\"scenario\":\"debris-response\",\"checks\":%lu,\"failures\":%d,\"play_calls\":0,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
