#include "../src/bh_sound_queue.h"
#include "../src/bh_fp.h"
#include "../src/bh_frame_service.h"
#include "../src/bh_input_messages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned long checks,plays;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failures<12)printf("FAIL sound tick %d: %s\n",__LINE__,label);}} while(0)
static int tick(BHSoundQueue *q,float dt,float master,BHSoundTickStats *s) {
    int ok=bh_sound_tick(q,dt,master,s);plays+=s->play_calls;return ok;
}
static BHSoundResource *resource(BHSoundQueue *q,BHWave *w,int streaming,int limit) {
    unsigned long id=bh_sound_register_wave(q,w,(unsigned short)streaming,(unsigned short)limit,0);
    BHSoundResource *r;
    CHECK(id!=BH_SOUND_INVALID,"source PCM -> native secondary registration");
    if(id==BH_SOUND_INVALID)return NULL;
    r=q->resources[id];r->time_scale=.125f;r->audible_range=10;return r;
}
static void __cdecl produce_sound_frame(void *p){bh_frame_service_tick((BHFrameService*)p);}
static LRESULT __cdecl sound_frame_message(void *p,HWND w,UINT m,WPARAM a,LPARAM b){return bh_input_message((BHInputSnapshot*)p,w,m,a,b);}
static void service_continuation(BHSoundQueue *q,BHSoundResource *r,BHSoundVoice *voice){
 BHFrameService service;BHFrameBuffers frames;BHDrawQueue draw,slots[3];BHInputCollection input;BHInputSnapshot snapshot;BHPlayerInputOwner lists;BHPlayerInputList *list;BHPlayerInputState player;BHEntityPool entities;
 volatile LONG stop=0,done=0,paused=0;volatile unsigned long counter=0;unsigned long i;
 memset(&service,0,sizeof(service));memset(&draw,0,sizeof(draw));memset(slots,0,sizeof(slots));memset(&player,0,sizeof(player));
 CHECK(bh_player_input_owner_init(&lists,NULL)&&bh_input_collection_init(&input,&snapshot,&lists),"service source input owners");input.disable_mouse_motion=1;
 list=bh_player_input_create(&lists,1);CHECK(list!=NULL,"service real input packet");if(!list)return;
 CHECK(bh_entities_init(&entities,2,NULL)&&bh_frame_buffers_bind(&frames,NULL),"service entity/frame owners");
 CHECK(bh_frame_buffers_start(&frames,produce_sound_frame,&service,30,3500UL*sizeof(BHDrawCommand),sound_frame_message,&snapshot),"service buffers without renderer");
 service.input=&input;service.entities=&entities;service.sound=q;service.frames=&frames;service.draw=&draw;for(i=0;i<3;++i)service.slot_draw[i]=slots+i;
 service.player_input=&player;service.packet=list->state;service.packet_bytes=sizeof(list->state);service.paused=&paused;service.frame_counter=&counter;service.stop=&stop;service.worker_done=&done;
 voice->frequency=-7000;voice->pending=1;r->phase=0;
 CHECK(bh_frame_service_tick(&service)&&service.error==BH_FRAME_SERVICE_OK&&service.produced==1&&service.packet_calls==1,"original service continues and publishes after real native sound rejection");
 CHECK(q->error==BH_SOUND_BACKEND&&service.sound_stats.last_error!=DS_OK&&service.sound_stats.frequency_failures==1&&service.sound_stats.failed_frequency==-7000&&service.sound_stats.failed_resource==r->index&&r->cached_frequency[0]==-7000,"rejected frequency remains unchanged and diagnostic is retained");
 r->buffer_count=4;
 CHECK(!bh_frame_service_tick(&service)&&service.error==BH_FRAME_SERVICE_SOUND&&q->error==BH_SOUND_TOPOLOGY&&service.produced==2,"invalid source topology is still a frame error");r->buffer_count=1;
 CHECK(bh_frame_buffers_dispose(&frames)&&bh_entities_dispose(&entities),"service source owners released");bh_player_input_owner_dispose(&lists);
}
static void one_shot(BHSoundQueue *q,BHWave *w) {
    BHSoundResource *r=resource(q,w,0,1);BHSoundVoice *v;BHSoundTickStats s;
    unsigned long h,next;DWORD status,frequency;long volume,pan;unsigned short generation;
    BHVec3 p={20,0,0},velocity={33,0,0};
    if(!r)return;
    h=bh_sound_request(q,r->index,1,1,&p,&velocity);v=bh_sound_voice(q,h);
    CHECK(v!=NULL,"real queued positioned moving voice");if(!v)return;
    q->listener_active=0;
    CHECK(tick(q,.25f,0,&s) && s.clock_calls==1 && !s.play_calls && v->pending && r->phase==0,"disabled listener freezes phase and pending start");
    q->listener_active=-2;q->listener_right.x=1;
    CHECK(tick(q,0,0,&s) && s.clock_calls==2 && !s.play_calls && v->pending,"phase exactly zero does not process");
    CHECK(tick(q,.125f,-100,&s) && s.play_calls==1 && s.position_calls==1 && !v->pending && r->playing[0],"deferred one-shot starts through actual native Play");
    CHECK(v->distance_squared==400 && v->cached.x==20 && v->listener_index==0,"real listener distance and displacement storage");
    CHECK(r->cached_volume[0]==-1100 && r->cached_pan[0]==10000 && r->cached_frequency[0]==7199,"analytic attenuation, signed-square pan, float-spilled Doppler before truncation");
    CHECK(IDirectSoundBuffer_GetVolume(r->buffers[0],&volume)==DS_OK && volume==-1100,"native volume agrees with cache");
    CHECK(IDirectSoundBuffer_GetPan(r->buffers[0],&pan)==DS_OK && pan==10000,"native pan agrees with cache");
    CHECK(IDirectSoundBuffer_GetFrequency(r->buffers[0],&frequency)==DS_OK && frequency==7199,"native frequency agrees with cache");
    CHECK(IDirectSoundBuffer_GetStatus(r->buffers[0],&status)==DS_OK && (status&DSBSTATUS_PLAYING) && !(status&DSBSTATUS_LOOPING),"native one-shot flag, no loop");
    CHECK(tick(q,.001f,-100,&s) && !s.play_calls && !s.volume_calls && !s.pan_calls && !s.frequency_calls,"unchanged parameters do not repeat native setters or start");
    /* Deterministic completion observation: Stop the real secondary, then let
       the owner query its real GetStatus. No wall-clock wait or fake vtable. */
    generation=v->generation;CHECK(IDirectSoundBuffer_Stop(r->buffers[0])==DS_OK,"real native completion control");
    CHECK(tick(q,.25f,-100,&s) && s.status_calls==1 && s.recycled==1 && !r->voices[0] && !r->playing[0],"stopped nonpending one-shot retires in status pass");
    CHECK(!bh_sound_voice(q,h) && v->generation==(unsigned short)(generation+1) && q->free_voices==v,"retirement invalidates generation and owns free list");
    next=bh_sound_request(q,r->index,1,1,NULL,NULL);
    CHECK(bh_sound_voice(q,next)==v && v->generation==(unsigned short)(generation+2),"next allocation supplies second generation increment");
    CHECK(tick(q,.25f,-3501,&s) && !s.play_calls && !v->pending && !r->playing[0] && r->voices[0]==v,"muted one-shot keeps original attached but nonpending lifetime");
    CHECK(tick(q,.25f,0,&s) && !s.play_calls && r->voices[0]==v,"muted one-shot does not restart when master recovers");
    CHECK(bh_sound_stop(q,next),"explicit stop releases muted attached voice");
    h=bh_sound_request(q,r->index,1,1,NULL,NULL);v=bh_sound_voice(q,h);
    CHECK(tick(q,.25f,-3500,&s) && s.play_calls==1 && r->cached_volume[0]==-3500,"strict mute threshold admits -3500");
    v->pending=1;
    CHECK(tick(q,.25f,-3500,&s) && s.stop_calls==1 && s.play_calls==1 && !s.status_calls,"pending restart stops before reposition and Play");
    CHECK(bh_sound_stop(q,h),"one-shot cleanup");
}
static void persistent(BHSoundQueue *q,BHWave *w) {
    BHSoundResource *r=resource(q,w,1,3);BHSoundTickStats s;BHSoundVoice *v;
    BHVec3 p={30,0,0};unsigned long h[4],i;DWORD status;
    if(!r)return;
    for(i=0;i<4;++i) {p.x=(float)(40-i*10);h[i]=bh_sound_request(q,r->index,1,1,&p,NULL);}
    CHECK(tick(q,.25f,0,&s) && s.duplicate_calls==2 && s.play_calls==3 && r->buffer_count==3 && r->no_more,"persistent list duplicates native buffers to selected voice limit");
    CHECK(r->cached_volume[0]==0 && r->cached_volume[1]==-1000 && r->cached_volume[2]==-2000,"nearest three selected in distance order, farthest excluded");
    CHECK(IDirectSoundBuffer_GetStatus(r->buffers[0],&status)==DS_OK && (status&DSBSTATUS_LOOPING),"actual secondary uses looping Play");
    CHECK(tick(q,.25f,-4000,&s) && s.stop_calls==3 && !s.play_calls && bh_sound_voice(q,h[3]),"muted loops stop buffers while retaining source requests");
    CHECK(tick(q,.25f,0,&s) && s.play_calls==3 && !s.duplicate_calls,"unmuted persistent requests restart existing native slots");
    for(i=0;i<3;++i)CHECK(bh_sound_stop(q,h[i]),"remove persistent request without immediate buffer assignment");
    CHECK(tick(q,.25f,0,&s) && s.stop_calls==2 && !s.play_calls && r->playing[0] && !r->playing[1] && !r->playing[2],"surplus native slots stop after list shrinks");
    v=bh_sound_voice(q,h[3]);v->position.x=-20;
    CHECK(tick(q,.25f,0,&s) && r->cached_pan[0]==-10000 && s.pan_calls==1,"negative pan and persistent reassignment update existing slot");
    CHECK(bh_sound_stop(q,h[3]) && tick(q,.25f,0,&s) && s.stop_calls==1 && !r->playing[0],"empty list stops final native slot");
    /* Equal distances preserve list traversal order: most recent request first. */
    p.x=10;h[0]=bh_sound_request(q,r->index,1,1,&p,NULL);p.x=-10;h[1]=bh_sound_request(q,r->index,1,1,&p,NULL);
    CHECK(tick(q,.25f,0,&s) && r->cached_pan[0]==-10000 && r->cached_pan[1]==10000,"stable nearest ranking preserves ties in original list order");
    CHECK(bh_sound_stop(q,h[0]) && bh_sound_stop(q,h[1]),"tie voices cleanup");
}
static void arithmetic(BHSoundQueue *q,BHWave *w) {
    BHSoundResource *r=resource(q,w,0,1);BHSoundTickStats s;BHSoundVoice *v;
    BHVec3 p={3,4,0};unsigned long h;float saved;
    if(!r)return;
    h=bh_sound_request(q,r->index,1,1,&p,NULL);v=bh_sound_voice(q,h);r->audible_range=2.5f;
    q->listener_right.x=1;
    CHECK(tick(q,.25f,0,&s) && v->distance_squared==25 && r->cached_volume[0]==-1000 && r->cached_pan[0]==3600,"non-dyadic signed square pan retains PC53 order before conversion");
    r->phase=1;CHECK(tick(q,.25f,0,&s) && r->phase==.75f && !s.status_calls,"positive phase skips native update");
    r->phase=0;r->time_scale=.125f;
    CHECK(tick(q,10,0,&s) && r->phase==0,"large delta clamps negative carried phase to zero without catch-up loop");
    v->volume=1;v->positioned=0;v->frequency=4294975296.0f;
    CHECK(tick(q,.25f,0,&s) && r->cached_volume[0]==0 && r->cached_frequency[0]==8192,"64-bit CRT conversion preserves low EAX before native setter");
    v->frequency=-8000;
    CHECK(!tick(q,.25f,0,&s) && q->error==BH_SOUND_BACKEND && s.last_error!=DS_OK && s.frequency_calls==1 && r->cached_frequency[0]==-8000,
        "actual invalid native frequency reports failure but original cache mutation continues");
    CHECK(tick(q,.25f,0,&s) && !s.frequency_calls,"failed setter cache remains cached on following update, as original");
    service_continuation(q,r,v);
    saved=r->phase;r->buffer_count=4;
    CHECK(!tick(q,.25f,0,&s) && q->error==BH_SOUND_TOPOLOGY && r->phase==saved,"malformed slot count rejects before indexing");r->buffer_count=1;
    CHECK(bh_sound_stop(q,h),"arithmetic voice cleanup");
}
static void duplicate_failure(BHSoundQueue *q,BHWave *w) {
    BHSoundResource *r=resource(q,w,1,2);BHSoundTickStats s;
    DSBUFFERDESC description;LPDIRECTSOUNDBUFFER primary=NULL,secondary;unsigned long a,b;
    if(!r)return;
    memset(&description,0,sizeof(description));description.dwSize=sizeof(description);description.dwFlags=DSBCAPS_PRIMARYBUFFER;
    CHECK(IDirectSound_CreateSoundBuffer(q->device,&description,&primary,NULL)==DS_OK && primary,"real primary interface for native duplication rejection");
    if(!primary)return;
    secondary=r->buffers[0];r->buffers[0]=primary;
    a=bh_sound_request(q,r->index,1,1,NULL,NULL);b=bh_sound_request(q,r->index,1,1,NULL,NULL);
    /* Muted throughout: no setter, cursor, Play or volume write to primary. */
    CHECK(tick(q,.25f,-10000,&s) && s.duplicate_calls==1 && !s.play_calls && !s.volume_calls &&
        r->no_more && r->buffer_count==1 && !r->buffers[1],"native duplication failure retains count and sets sticky fallback");
    CHECK(tick(q,.25f,-10000,&s) && !s.duplicate_calls && !s.play_calls,"sticky failure prevents repeated native duplicate attempts");
    CHECK(bh_sound_stop(q,a) && bh_sound_stop(q,b),"failed duplication leaves real source requests stoppable");
    r->buffers[0]=secondary;IDirectSoundBuffer_Release(primary);
}
int bh_sound_tick_checkpoint(unsigned long *count) {
    LPDIRECTSOUND device=NULL;BHSoundQueue q;BHWave w;BHSoundTickStats s;
    unsigned char *silence;HRESULT hr;
    checks=plays=0;failures=0;
    hr=DirectSoundCreate(NULL,&device,NULL);CHECK(hr==DS_OK && device,"native DirectSound device");
    if(hr==DS_OK && device) {
        CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK,"native normal cooperative level");
        memset(&w,0,sizeof(w));silence=(unsigned char *)calloc(16000,1);
        CHECK(silence!=NULL,"one second silent PCM allocation");
        if(silence) {
            w.samples=silence;w.sample_bytes=16000;w.format_tag=1;w.channels=1;w.sample_rate=8000;w.byte_rate=16000;w.block_align=2;w.bits=16;
            CHECK(bh_sound_queue_init(&q,device,NULL,NULL),"source owner borrows real device");
            CHECK(tick(&q,.125f,0,&s) && s.clock_calls==1,"empty inactive owner");
            one_shot(&q,&w);persistent(&q,&w);arithmetic(&q,&w);duplicate_failure(&q,&w);
            CHECK(bh_sound_queue_dispose(&q),"all native buffers and source requests disposed");free(silence);
        }
        CHECK(IDirectSound_Release(device)==0,"native references returned exactly once");
    }
    *count=checks;
    printf("{\"scenario\":\"sound-tick\",\"checks\":%lu,\"failures\":%d,\"play_calls\":%lu,\"silent_pcm\":true,\"original_code_required\":false}\n",checks,failures,plays);
    return failures!=0;
}
