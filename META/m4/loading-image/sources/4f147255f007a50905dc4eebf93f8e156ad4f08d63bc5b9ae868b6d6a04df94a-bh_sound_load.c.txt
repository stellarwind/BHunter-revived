#include "bh_sound_load.h"
#include "bh_vfile.h"
#include "bh_riff.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static unsigned long fail(BHSoundLoad *c,BHSoundLoadError error){
    if(c){c->error=error;++c->failures;}return BH_SOUND_INVALID;
}
static unsigned long u32(const unsigned char *p){return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);}
static unsigned short u16(const unsigned char *p){return (unsigned short)(p[0]|((unsigned short)p[1]<<8));}
static void *allocate(BHSoundLoad *c,unsigned long bytes){++c->allocations;
    return c->file_allocator.allocate?c->file_allocator.allocate(bytes,c->file_allocator.user):malloc(bytes);}
static void release(BHSoundLoad *c,void *p){if(!p)return;
    if(c->file_allocator.release)c->file_allocator.release(p,c->file_allocator.user);else free(p);}
unsigned long __cdecl bh_sound_load(BHSoundLoad *c,const char *name,float range,float level,unsigned short streaming,float denominator){
    BHSoundQueue *q;BHVFile *file;unsigned char *bytes;long length;
    const unsigned char *format,*samples;unsigned long sample_bytes,format_bytes,i,frequency;
    DSBUFFERDESC description;WAVEFORMATEX pcm;LPWAVEFORMATEX native_format;
    LPDIRECTSOUNDBUFFER buffer=NULL;void *first=NULL,*second=NULL;DWORD first_size=0,second_size=0;
    BHSoundResource *resource;double volume;int random;
    if(!c)return BH_SOUND_INVALID;c->error=BH_SOUND_LOAD_OK;
    if(!(q=c->queue)||!name||strlen(name)>=64||!q->device||q->voice_limit_policy<1||q->voice_limit_policy>3||
       (!!c->file_allocator.allocate!=!!c->file_allocator.release))return fail(c,BH_SOUND_LOAD_ARGUMENT);
    if(!bh_math_environment_supported())return fail(c,BH_SOUND_LOAD_NUMERICAL);
    file=bh_vfile_open(name,"rb");if(!file)return fail(c,BH_SOUND_LOAD_IO);++c->files;
    if(bh_vfile_seek(file,0,SEEK_END)||(length=bh_vfile_tell(file))<=0||length>64L*1024L*1024L||bh_vfile_seek(file,0,SEEK_SET)){
        bh_vfile_close(file);return fail(c,BH_SOUND_LOAD_IO);}
    c->file_bytes=(unsigned long)length;bytes=(unsigned char*)allocate(c,(unsigned long)length);
    if(!bytes){bh_vfile_close(file);return fail(c,BH_SOUND_LOAD_MEMORY);}
    if(bh_vfile_read(bytes,(unsigned long)length,1,file)!=1){bh_vfile_close(file);release(c,bytes);return fail(c,BH_SOUND_LOAD_IO);}
    bh_vfile_close(file);
    if(!bh_riff_scan(bytes,(unsigned long)length,&format,&samples,&sample_bytes)||!format||!samples||!sample_bytes){
        release(c,bytes);return fail(c,BH_SOUND_LOAD_FORMAT);}
    format_bytes=u32(format-4);native_format=(LPWAVEFORMATEX)format;
    if(format_bytes<16){release(c,bytes);return fail(c,BH_SOUND_LOAD_FORMAT);}
    if(u16(format)==WAVE_FORMAT_PCM){memset(&pcm,0,sizeof(pcm));memcpy(&pcm,format,16);native_format=&pcm;}
    else if(format_bytes<18||u16(format+16)>format_bytes-18){release(c,bytes);return fail(c,BH_SOUND_LOAD_FORMAT);}
    memset(&description,0,sizeof(description));description.dwSize=20;description.dwFlags=0xe0;
    description.dwBufferBytes=sample_bytes;description.lpwfxFormat=native_format;
    ++c->native_calls;c->create_result=IDirectSound_CreateSoundBuffer(q->device,&description,&buffer,NULL);
    if(c->create_result!=DS_OK||!buffer){if(buffer){IDirectSoundBuffer_Release(buffer);++c->releases;}release(c,bytes);return fail(c,BH_SOUND_LOAD_NATIVE);}
    ++c->native_calls;c->lock_result=IDirectSoundBuffer_Lock(buffer,0,sample_bytes,&first,&first_size,&second,&second_size,0);
    if(c->lock_result!=DS_OK){IDirectSoundBuffer_Release(buffer);++c->releases;release(c,bytes);return fail(c,BH_SOUND_LOAD_NATIVE);}
    c->first_bytes=first_size;c->second_bytes=second_size;
    if(first_size>sample_bytes||second_size>sample_bytes-first_size||(first_size&&!first)||(second_size&&!second)){
        IDirectSoundBuffer_Unlock(buffer,first,first_size,second,second_size);IDirectSoundBuffer_Release(buffer);++c->releases;
        release(c,bytes);return fail(c,BH_SOUND_LOAD_NATIVE);}
    if(first_size)memcpy(first,samples,first_size);if(second_size)memcpy(second,samples+first_size,second_size);
    ++c->native_calls;c->unlock_result=IDirectSoundBuffer_Unlock(buffer,first,first_size,second,second_size);
    if(c->unlock_result!=DS_OK){++c->failures;c->error=BH_SOUND_LOAD_NATIVE;}
    release(c,bytes);
    resource=bh_sound_resource_allocate(q);
    if(!resource){IDirectSoundBuffer_Release(buffer);++c->releases;return fail(c,BH_SOUND_LOAD_MEMORY);}
    strcpy(resource->name,name);
    ++c->native_calls;c->frequency_result=IDirectSoundBuffer_GetFrequency(buffer,&frequency);
    if(c->frequency_result==DS_OK)bh_round_float(&resource->base_frequency,(double)frequency);
    else resource->base_frequency=22050.0f;
    resource->no_more=resource->voice_limit==1;
    resource->buffers[0]=buffer;resource->streaming=streaming;resource->buffer_count=1;
    resource->sample_bytes=first_size;resource->voice_limit=q->voice_limit_policy;resource->stream_head=NULL;
    volume=level>0.0f?(log((double)level)*-1000.0)/log(0.5):-10000.0;
    if(!(volume> -10000.0))volume=-10000.0;
    bh_round_float(&resource->time_scale,1.0/(double)denominator);
    bh_round_float(&resource->base_volume,volume);
    random=rand();resource->audible_range=range;
    bh_round_float(&resource->phase,((double)random*resource->time_scale)*(1.0/32768.0));
    for(i=0;i<resource->voice_limit;++i){resource->playing[i]=0;
        resource->cached_volume[i]=resource->cached_frequency[i]=resource->cached_pan[i]=-100000;resource->voices[i]=NULL;}
    ++c->loads;return resource->index;
}
