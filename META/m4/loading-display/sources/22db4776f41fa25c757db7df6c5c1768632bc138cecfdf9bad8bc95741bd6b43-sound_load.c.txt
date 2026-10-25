#include "../src/bh_sound_load.h"
#include "../src/bh_vfile.h"
#include "../src/bh_riff.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<14)printf("FAIL sound-load: %s\n",s);}}while(0)
typedef struct Memory {unsigned long calls,live,deny;} Memory;
static void *__cdecl allocate(unsigned long n,void *user){Memory *m=(Memory*)user;void *p;++m->calls;if(m->calls==m->deny)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory*)user;if(p){--m->live;free(p);}}
static void put16(unsigned char *p,unsigned long x){p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);}
static void put32(unsigned char *p,unsigned long x){put16(p,x);put16(p+2,x>>16);}
static int write_fixture(const char *name,int bad,unsigned long length){
    unsigned char data[108];FILE *file;unsigned long i;
    memset(data,0,sizeof(data));memcpy(data,"RIFF",4);put32(data+4,100);memcpy(data+8,"WAVEfmt ",8);
    put32(data+16,16);put16(data+20,1);put16(data+22,1);put32(data+24,bad?0:8000);put32(data+28,bad?0:16000);
    put16(data+32,2);put16(data+34,16);memcpy(data+36,"data",4);put32(data+40,64);
    for(i=0;i<64;++i)data[44+i]=(unsigned char)(i*7+3);
    file=fopen(name,"wb");if(!file)return 0;i=fwrite(data,1,length,file);return fclose(file)==0&&i==length;
}
static unsigned long crc(const unsigned char *p,unsigned long n,unsigned long c){unsigned long i;int b;
    for(i=0;i<n;++i){c^=p[i];for(b=0;b<8;++b)c=(c>>1)^(0xedb88320UL&(0UL-(c&1)));}return c;}
static void readback(BHSoundResource *r,unsigned long expected_crc,unsigned long size,unsigned long rate){
    void *a=NULL,*b=NULL;DWORD an=0,bn=0,status=DSBSTATUS_PLAYING,frequency=0;unsigned long actual;
    CHECK(r&&r->buffers[0]&&r->sample_bytes==size&&r->buffer_count==1,"actual published native buffer and first-span sample count");if(!r||!r->buffers[0])return;
    CHECK(IDirectSoundBuffer_GetStatus(r->buffers[0],&status)==DS_OK&&!(status&DSBSTATUS_PLAYING),"native buffer never enters playback");
    CHECK(IDirectSoundBuffer_GetFrequency(r->buffers[0],&frequency)==DS_OK&&frequency==rate&&r->base_frequency==(float)rate,"actual native frequency reaches original unsigned-to-float metadata");
    if(IDirectSoundBuffer_Lock(r->buffers[0],0,size,&a,&an,&b,&bn,0)==DS_OK){
        actual=crc((const unsigned char*)a,an,0xffffffffUL);if(bn)actual=crc((const unsigned char*)b,bn,actual);
        CHECK(an+bn==size&&(actual^0xffffffffUL)==expected_crc,"actual native Lock readback equals independently fingerprinted WAV samples");
        CHECK(IDirectSoundBuffer_Unlock(r->buffers[0],a,an,b,bn)==DS_OK,"readback unlocks both actual spans");
    }else CHECK(0,"actual native readback Lock");
}
typedef struct RealWave {const char *name;unsigned long offset,size,samples,crc,rate,byte_rate,channels,align,bits;} RealWave;
static const RealWave real_waves[]={
#include "wave_manifest.h"
};
int bh_sound_load_checkpoint(const char *archive,unsigned long *count){
    const char *name="sound-load-fixture.wav",*bad="sound-load-bad.wav",*short_name="sound-load-short.wav";
    BHSoundLoad load;BHSoundQueue queue;BHSoundResource *r;LPDIRECTSOUND device=NULL;BHMathEnvironment fp;
    Memory files={0,0,0},resources={0,0,0};BHSceneAllocator allocator={allocate,release,&resources};
    unsigned long id,i,j,expected_crc,old_count,previous;int first_rand,next_rand,mounted=0;HRESULT hr;
    unsigned char samples[64];float levels[5]={1,.5f,2,0,-1};float volumes[5]={0,-1000,1000,-10000,-10000};
    checks=failures=0;memset(&load,0,sizeof(load));memset(&queue,0,sizeof(queue));
    if(!bh_math_environment_enter(&fp))return 2;
    CHECK(write_fixture(name,0,108)&&write_fixture(bad,1,108)&&write_fixture(short_name,0,40),"owned valid, native-rejected and truncated external WAV fixtures");
    hr=DirectSoundCreate(NULL,&device,NULL);CHECK(hr==DS_OK&&device,"real DirectSound owner without Play");if(hr!=DS_OK||!device)goto done;
    CHECK(IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK,"normal native cooperative level without display/input capture");
    CHECK(bh_sound_queue_init(&queue,device,NULL,&allocator),"actual resource allocator/native device binding");
    load.queue=&queue;load.file_allocator.allocate=allocate;load.file_allocator.release=release;load.file_allocator.user=&files;
    srand(31);first_rand=rand();srand(31);
    CHECK(bh_sound_load(&load,"missing-sound-load.wav",4,1,1,8)==BH_SOUND_INVALID&&!files.live&&rand()==first_rand,"missing file retains RNG and allocation ownership");
    files.deny=files.calls+1;srand(31);
    CHECK(bh_sound_load(&load,name,4,1,1,8)==BH_SOUND_INVALID&&load.error==BH_SOUND_LOAD_MEMORY&&!files.live&&rand()==first_rand,"file allocation failure closes actual VFILE without RNG consumption");files.deny=0;
    CHECK(bh_sound_load(&load,short_name,4,1,1,8)==BH_SOUND_INVALID&&load.error==BH_SOUND_LOAD_FORMAT&&!files.live,"bounded original RIFF request semantics reject truncated file safely");
    CHECK(bh_sound_load(&load,bad,4,1,1,8)==BH_SOUND_INVALID&&load.create_result!=DS_OK&&!files.live,"actual native CreateSoundBuffer rejects invalid sample rate, releasing temporary ownership");
    resources.deny=resources.calls+1;previous=load.releases;srand(31);
    CHECK(bh_sound_load(&load,name,4,1,1,8)==BH_SOUND_INVALID&&load.releases==previous+1&&!files.live&&rand()==first_rand,"resource allocation failure releases actual uploaded native buffer before any RNG draw");resources.deny=0;
    for(i=0;i<64;++i)samples[i]=(unsigned char)(i*7+3);expected_crc=crc(samples,64,0xffffffffUL)^0xffffffffUL;
    /* Real freelist resource, not a fabricated object; unused recycled payload
       retains precisely the original selective initialization contract. */
    r=bh_sound_resource_allocate(&queue);CHECK(r!=NULL,"actual recycled resource ownership for old-voice-limit contract");if(!r)goto done;
    r->voice_limit=1;r->cached_volume[2]=12345;r->cached_frequency[2]=23456;r->cached_pan[2]=34567;
    r->free_next=NULL;queue.free_resources=r;queue.voice_limit_policy=2;
    srand(71);first_rand=rand();next_rand=rand();srand(71);
    id=bh_sound_load(&load,name,4,1,0x1234,8);
    CHECK(id==r->index&&rand()==next_rand&&load.loads==1&&!files.live,"whole loader reuses real resource and consumes exactly one shared CRT random draw");
    CHECK(r->no_more==1&&r->voice_limit==2&&r->streaming==0x1234&&r->audible_range==4&&r->time_scale==.125f&&
          r->phase==(float)((double)first_rand/262144.0)&&!strcmp(r->name,name),"raw five-argument metadata, stored-float phase and old-limit no_more quirk");
    CHECK(r->cached_volume[2]==12345&&r->cached_frequency[2]==23456&&r->cached_pan[2]==34567,"unused recycled cache slots remain unchanged");
    for(j=0;j<2;++j)CHECK(!r->playing[j]&&r->cached_volume[j]==-100000&&r->cached_frequency[j]==-100000&&r->cached_pan[j]==-100000&&!r->voices[j],"current-policy cache and voice initialization");
    readback(r,expected_crc,64,8000);
    for(i=0;i<5;++i){id=bh_sound_load(&load,name,25,levels[i],(unsigned short)(i&1),4);
        CHECK(id!=BH_SOUND_INVALID,"complete loader finite positive/zero/negative level fixture");if(id==BH_SOUND_INVALID)continue;
        r=queue.resources[id];CHECK(r->base_volume==volumes[i]&&r->time_scale==.25f&&r->audible_range==25&&r->no_more==0,"exact dyadic level attenuation and lower clamp metadata");}
    old_count=queue.resource_count;
    if(archive){mounted=bh_vfile_mount(archive);CHECK(mounted,"actual immutable archive mounted as data only");
        for(i=0;i<sizeof(real_waves)/sizeof(real_waves[0]);++i){
            id=bh_sound_load(&load,real_waves[i].name,30,1,(unsigned short)(i&1),8);
            CHECK(id!=BH_SOUND_INVALID,"whole original loader reads actual archived WAV via restored VFILE/RIFF/native owners");
            if(id!=BH_SOUND_INVALID)readback(queue.resources[id],real_waves[i].crc,real_waves[i].samples,real_waves[i].rate);
        }
        CHECK(queue.resource_count==old_count+sizeof(real_waves)/sizeof(real_waves[0]),"all actual WAVs publish owning source resources without playback");}
done:
    if(mounted)CHECK(bh_vfile_detach_archive(),"fixture-owned archive mount detaches after all actual VFILE handles closed");
    bh_vfile_shutdown();
    if(queue.allocator.release)CHECK(bh_sound_queue_dispose(&queue)&&!resources.live,"actual native buffers/resource arenas and source allocations disposed once");
    if(device)IDirectSound_Release(device);remove(name);remove(bad);remove(short_name);
    CHECK(!files.live&&bh_math_environment_supported(),"temporary ownership and explicit callerPC53 retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"sound-load\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0,\"archive\":%s}\n",checks,failures,archive?"true":"false");return failures!=0;
}
#ifdef BH_SOUND_LOAD_STANDALONE
int main(int argc,char **argv){return bh_sound_load_checkpoint(argc>1?argv[1]:NULL,NULL);}
#endif
