#include "../src/bh_sound_queue.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL sound queue %d: %s\n",__LINE__,label);}} while(0)
typedef struct Memory {unsigned long calls,live,fail_at;} Memory;
static void *__cdecl allocate(unsigned long size,void *user) {Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls==m->fail_at)return NULL;p=malloc(size);if(p)++m->live;return p;}
static void __cdecl release(void *pointer,void *user) {Memory *m=(Memory *)user;--m->live;free(pointer);}
static void pool_cases(void) {
    Memory memory={0,0,0};BHSceneAllocator allocator;
    BHSoundQueue q;BHSoundResource *r,*first;BHSoundVoice *v;
    unsigned long i;
    allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
    CHECK(bh_sound_queue_init(&q,NULL,NULL,&allocator),"real allocator context without synthetic backend");
    memory.fail_at=1;CHECK(!bh_sound_resource_allocate(&q) && q.resource_count==0 && memory.live==0,"table allocation failure retains empty ownership");
    memory.fail_at=memory.calls+2;
    CHECK(!bh_sound_resource_allocate(&q) && q.resource_count==0 && memory.live==1 && q.resource_capacity==16,"record failure retains valid expanded pointer arena");
    memory.fail_at=0;first=NULL;
    for(i=0;i<21;++i) {
        r=bh_sound_resource_allocate(&q);if(!i)first=r;
        CHECK(r && r->index==i,"resource indices bind actual owned definitions");
    }
    CHECK(q.resource_capacity==25,"original 16 -> 20 -> 25 growth uses 1.25, not a guessed 1.5");
    first->name[0]='Q';first->free_next=NULL;q.free_resources=first;
    CHECK(bh_sound_resource_allocate(&q)==first && first->index==0 && first->name[0]=='Q' && q.resource_count==21,
        "resource freelist reuses index and preserves existing payload");
    for(i=0;i<26;++i) {v=bh_sound_voice_allocate(&q);CHECK(v && v->index==i && v->generation==0,"new voice slot ID/generation fields");}
    CHECK(q.voice_capacity==31,"voice arena follows original truncated growth sequence");
    v=q.voices[0];v->generation=65535;v->position.x=19;v->next=NULL;q.free_voices=v;
    CHECK(bh_sound_voice_allocate(&q)==v && v->generation==0 && v->position.x==19 && q.voice_count==26,
        "voice recycle wraps ushort generation and retains payload");
    CHECK(bh_sound_queue_dispose(&q) && memory.live==0,"all pool-only arrays and records released once");
}
static unsigned long wave_fixture(unsigned char *bytes) {
    unsigned long riff=100,format_size=16,sample_rate=8000,byte_rate=16000,samples=64;
    unsigned short pcm=1,mono=1,align=2,bits=16;
    memset(bytes,0,108);memcpy(bytes,"RIFF",4);memcpy(bytes+4,&riff,4);memcpy(bytes+8,"WAVEfmt ",8);
    memcpy(bytes+16,&format_size,4);memcpy(bytes+20,&pcm,2);memcpy(bytes+22,&mono,2);
    memcpy(bytes+24,&sample_rate,4);memcpy(bytes+28,&byte_rate,4);memcpy(bytes+32,&align,2);memcpy(bytes+34,&bits,2);
    memcpy(bytes+36,"data",4);memcpy(bytes+40,&samples,4);return 108;
}
static void native_cases(void) {
    LPDIRECTSOUND device=NULL;
    HRESULT hr;
    BHWave wave;unsigned char bytes[108];
    BHSoundQueue q;BHSoundResource *resource,*stream,*tie;BHSoundVoice *voice,*saved_voice;
    BHVec3 position={1,0,0},velocity={2,3,4};float priority;
    unsigned long id,stream_id,tie_id,handle[3],replacement,stream_handles[24],i,old_count;
    DWORD play,write;void *first,*second;DWORD first_size,second_size;
    DSBUFFERDESC primary_description;LPDIRECTSOUNDBUFFER primary=NULL,secondary;
    unsigned long failure_id,failure_handle,failure_replacement;unsigned short saved_generation;
    Memory memory={0,0,0};BHSceneAllocator allocator;
    allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
    hr=DirectSoundCreate(NULL,&device,NULL);CHECK(hr==DS_OK && device,"real native DirectSoundCreate");
    printf("{\"native_directsound_create_hresult\":%ld,\"play_calls\":0}\n",(long)hr);
    if(hr!=DS_OK || !device)return;
    hr=IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL);
    CHECK(hr==DS_OK,"native normal cooperative level without exclusive mode or volume writes");
    CHECK(bh_sound_queue_init(&q,device,NULL,&allocator),"native interface source ownership AddRef");
    CHECK(bh_wave_init(&wave,NULL) && bh_wave_decode(&wave,bytes,wave_fixture(bytes)),"real source PCM decode");
    memory.fail_at=memory.calls+1;
    CHECK(bh_sound_register_wave(&q,&wave,0,3,-100)==BH_SOUND_INVALID && q.resource_count==0 && memory.live==0,
        "failed source allocation releases real uploaded native buffer");memory.fail_at=0;
    id=bh_sound_register_wave(&q,&wave,0,3,-100);CHECK(id!=BH_SOUND_INVALID,"decoded PCM uploaded to real native buffer");
    if(id==BH_SOUND_INVALID){bh_wave_dispose(&wave);bh_sound_queue_dispose(&q);IDirectSound_Release(device);return;}
    resource=q.resources[id];
    hr=IDirectSoundBuffer_Lock(resource->buffers[0],0,64,&first,&first_size,&second,&second_size,0);
    CHECK(hr==DS_OK && first_size+second_size==64 && !memcmp(first,wave.samples,first_size) &&
        (!second_size || !memcmp(second,wave.samples+first_size,second_size)),"native buffer contains the actual source-decoded PCM bytes");
    if(hr==DS_OK)IDirectSoundBuffer_Unlock(resource->buffers[0],first,first_size,second,second_size);
    CHECK(resource->base_frequency==8000 && resource->buffer_count==1 && !resource->no_more,"native frequency and original duplication limit contract");
    q.listener_active=0;
    CHECK(bh_sound_priority(&q,1,&position,resource,-1,&priority) && priority==9e37f,"inactive spatial listener retains huge original baseline");
    CHECK(bh_sound_priority(&q,0,NULL,resource,-1,&priority) && priority==.01f,"non-positioned .01 baseline");
    q.listener_active=1;
    for(i=0;i<3;++i) {
        position.x=(float)i+1;handle[i]=bh_sound_request(&q,id,.5f,1,&position,&velocity);
        CHECK(handle[i]!=BH_SOUND_INVALID && bh_sound_voice(&q,handle[i]) && resource->buffer_count==i+1,
            "real native duplicate buffers and first-free queued voice selection");
    }
    CHECK(resource->no_more && q.voice_count==3,"native duplication stops exactly at three slots");
    hr=IDirectSoundBuffer_SetCurrentPosition(resource->buffers[2],32);
    CHECK(hr==DS_OK && IDirectSoundBuffer_GetCurrentPosition(resource->buffers[2],&play,&write)==DS_OK && play==32,
        "actual stopped buffer cursor controls progress without playing");
    CHECK(bh_sound_priority(&q,1,&position,resource,2,&priority) && priority==18,
        "native play cursor weights squared distance by bytes remaining");
    position.x=4;replacement=bh_sound_request(&q,id,.25f,.5f,&position,NULL);
    voice=bh_sound_voice(&q,replacement);
    CHECK(replacement!=BH_SOUND_INVALID && voice==resource->voices[2] && voice->index==2 && voice->generation==2 &&
        !bh_sound_voice(&q,handle[2]),"progress-weighted replacement increments generation on release and reuse");
    CHECK(voice->volume==-2100 && voice->frequency==4000 && voice->pending==1 && !voice->moving,
        "queued volume/frequency/flags derive from actual resource fields");
    old_count=q.voice_count;position.x=10;saved_voice=resource->voices[2];
    CHECK(bh_sound_request(&q,id,1,1,&position,NULL)==BH_SOUND_INVALID && q.error==BH_SOUND_PRIORITY &&
        resource->voices[2]==saved_voice && q.voice_count==old_count,"farther request is rejected without stealing a closer existing voice");
    CHECK(bh_sound_update(&q,replacement,0,2,NULL,NULL)==1 && voice->volume==-10100 && voice->frequency==16000 &&
        !voice->positioned && voice->position.x==4,"update clears presence flags, preserves coordinates, and floors dB before adding base volume");
    CHECK(!bh_sound_update(&q,handle[2],1,1,NULL,NULL) && q.error==BH_SOUND_STALE,"old generation cannot update replacement");
    CHECK(bh_sound_stop(&q,replacement) && resource->voices[2]==NULL && !bh_sound_voice(&q,replacement),
        "native Stop detaches buffered voice and invalidates its handle");
    position.x=2;tie_id=bh_sound_register_wave(&q,&wave,0,1,0);tie=q.resources[tie_id];
    handle[2]=bh_sound_request(&q,tie_id,1,1,&position,NULL);
    replacement=bh_sound_request(&q,tie_id,1,1,&position,NULL);
    CHECK(replacement!=BH_SOUND_INVALID && !bh_sound_voice(&q,handle[2]) && (replacement>>16)==(handle[2]>>16)+2,
        "equal priority is admitted; recycling generation reflects previous stopped slot");
    /* Real native failure control: primary buffers cannot be duplicated. No
       fabricated COM object, vtable or unconditional error shim is involved. */
    memset(&primary_description,0,sizeof(primary_description));
    primary_description.dwSize=sizeof(primary_description);primary_description.dwFlags=DSBCAPS_PRIMARYBUFFER;
    hr=IDirectSound_CreateSoundBuffer(device,&primary_description,&primary,NULL);
    CHECK(hr==DS_OK && primary,"native primary buffer for real DuplicateSoundBuffer failure control");
    if(hr==DS_OK && primary) {
        failure_id=bh_sound_register_wave(&q,&wave,0,2,0);
        secondary=q.resources[failure_id]->buffers[0];q.resources[failure_id]->buffers[0]=primary;
        failure_handle=bh_sound_request(&q,failure_id,1,1,NULL,NULL);
        failure_replacement=bh_sound_request(&q,failure_id,1,1,NULL,NULL);
        CHECK(failure_handle!=BH_SOUND_INVALID && q.resources[failure_id]->no_more &&
            q.resources[failure_id]->buffer_count==1 && !q.resources[failure_id]->buffers[1],
            "native duplication failure becomes sticky and retains original owned buffer count");
        CHECK(failure_replacement==BH_SOUND_INVALID ? bh_sound_stop(&q,failure_handle) : bh_sound_stop(&q,failure_replacement),
            "native failure fallback leaves a valid stoppable queued voice");
        q.resources[failure_id]->buffers[0]=secondary;IDirectSoundBuffer_Release(primary);
    }
    stream_id=bh_sound_register_wave(&q,&wave,1,3,0);stream=q.resources[stream_id];
    for(i=0;i<24;++i) {stream_handles[i]=bh_sound_request(&q,stream_id,1,1,NULL,NULL);CHECK(stream_handles[i]!=BH_SOUND_INVALID,"real streaming-list queue nodes");}
    CHECK(stream->buffer_count==1 && q.voice_capacity==31 && stream->stream_head==bh_sound_voice(&q,stream_handles[23]),
        "streaming requests use source list and grow voice arena without native duplicate buffers");
    CHECK(bh_sound_stop(&q,stream_handles[12]) && !bh_sound_voice(&q,stream_handles[12]) &&
        bh_sound_voice(&q,stream_handles[13])->next==bh_sound_voice(&q,stream_handles[11]) &&
        bh_sound_voice(&q,stream_handles[11])->previous==bh_sound_voice(&q,stream_handles[13]),"middle streaming node unlinks in both directions");
    CHECK(bh_sound_stop(&q,stream_handles[23]) && stream->stream_head==bh_sound_voice(&q,stream_handles[22]),"newest streaming head removal");
    voice=bh_sound_voice(&q,stream_handles[0]);saved_generation=voice->generation;voice->generation=0x8000;
    CHECK(!bh_sound_voice(&q,((unsigned long)voice->generation<<16)|voice->index) &&
        !bh_sound_stop(&q,((unsigned long)voice->generation<<16)|voice->index),
        "original signed-division handle check rejects negative generation-packed values");
    voice->generation=saved_generation;
    CHECK(bh_sound_request(&q,BH_SOUND_INVALID,1,1,NULL,NULL)==BH_SOUND_INVALID &&
        bh_sound_update(&q,BH_SOUND_INVALID,1,1,NULL,NULL)==0,"integer -1 handle/resource sentinel");
    bh_wave_dispose(&wave);
    CHECK(bh_sound_queue_dispose(&q) && memory.live==0,"real source Stop/Release and all owner allocations cleaned up");
    CHECK(IDirectSound_Release(device)==0,"native device reference returned to caller and released");
}
int bh_sound_queue_checkpoint(unsigned long *count) {
    checks=0;failed=0;pool_cases();native_cases();*count=checks;
    printf("{\"scenario\":\"sound-queue\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false,\"play_calls\":0}\n",checks,failed);
    return failed!=0;
}
