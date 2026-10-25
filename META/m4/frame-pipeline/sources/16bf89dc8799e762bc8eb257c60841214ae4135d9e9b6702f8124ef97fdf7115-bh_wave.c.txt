#include "bh_wave.h"
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT (64UL*1024UL*1024UL)
typedef char wave_word_size[(sizeof(unsigned short)==2 && sizeof(unsigned long)==4)?1:-1];
static unsigned short u16(const unsigned char *p) {
    return (unsigned short)((unsigned short)p[0]|((unsigned short)p[1]<<8));
}
static unsigned long u32(const unsigned char *p) {
    return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);
}
static int scan_fail(BHWaveView *view,BHWaveError error) {
    memset(view,0,sizeof(*view));view->error=error;return 0;
}
int __cdecl bh_wave_scan(BHWaveView *view,const void *bytes,unsigned long size) {
    const unsigned char *data=(const unsigned char*)bytes;
    unsigned long end,at,count,padded;BHWaveView next;
    if(!view)return 0;
    if(!data)return scan_fail(view,BH_WAVE_ARGUMENT);
    if(size>MAX_INPUT)return scan_fail(view,BH_WAVE_LIMIT);
    if(size<12)return scan_fail(view,BH_WAVE_TRUNCATED);
    if(memcmp(data,"RIFF",4) || memcmp(data+8,"WAVE",4))return scan_fail(view,BH_WAVE_FORMAT);
    count=u32(data+4);
    if(count<4)return scan_fail(view,BH_WAVE_FORMAT);
    if(count>size-8)return scan_fail(view,BH_WAVE_TRUNCATED);
    end=count+8;
    if(end!=size)return scan_fail(view,BH_WAVE_FORMAT);
    memset(&next,0,sizeof(next));next.riff_bytes=end;
    for(at=12;at<end;) {
        if(end-at<8)return scan_fail(view,BH_WAVE_TRUNCATED);
        count=u32(data+at+4);
        if(count>end-at-8)return scan_fail(view,BH_WAVE_TRUNCATED);
        padded=count+(count&1); /* bounded by MAX_INPUT before addition */
        if(padded>end-at-8)return scan_fail(view,BH_WAVE_TRUNCATED);
        if(!memcmp(data+at,"fmt ",4) && !next.format) {
            if(count<14)return scan_fail(view,BH_WAVE_FORMAT);
            next.format=data+at+8;next.format_size=count;
        } else if(!memcmp(data+at,"data",4) && !next.samples) {
            next.samples=data+at+8;next.sample_bytes=count;
        }
        at+=8+padded;
    }
    if(!next.format || !next.samples)return scan_fail(view,BH_WAVE_FORMAT);
    *view=next;return 1;
}
static void *__cdecl allocate(unsigned long bytes,void *user) {(void)user;return malloc(bytes);}
static void __cdecl release(void *p,void *user) {(void)user;free(p);}
static int fail(BHWave *wave,BHWaveError error) {if(wave)wave->error=error;return 0;}
int __cdecl bh_wave_init(BHWave *wave,const BHSceneAllocator *allocator) {
    if(!wave)return 0;
    memset(wave,0,sizeof(*wave));
    if(allocator) {
        if(!allocator->allocate || !allocator->release)return fail(wave,BH_WAVE_ARGUMENT);
        wave->allocator=*allocator;
    } else {wave->allocator.allocate=allocate;wave->allocator.release=release;}
    return 1;
}
void __cdecl bh_wave_dispose(BHWave *wave) {
    BHSceneAllocator allocator;
    if(!wave)return;
    allocator=wave->allocator;
    if(wave->samples && allocator.release)allocator.release(wave->samples,allocator.user);
    memset(wave,0,sizeof(*wave));wave->allocator=allocator;
}
int __cdecl bh_wave_decode(BHWave *wave,const void *bytes,unsigned long size) {
    BHWaveView view;BHWave next;const unsigned char *p;unsigned long expected;
    if(!wave || !wave->allocator.allocate || !wave->allocator.release)return fail(wave,BH_WAVE_ARGUMENT);
    if(!bh_wave_scan(&view,bytes,size))return fail(wave,view.error);
    if(view.format_size<16)return fail(wave,BH_WAVE_FORMAT);
    bh_wave_init(&next,&wave->allocator);p=view.format;
    next.format_tag=u16(p);next.channels=u16(p+2);next.sample_rate=u32(p+4);
    next.byte_rate=u32(p+8);next.block_align=u16(p+12);next.bits=u16(p+14);
    if(next.format_tag!=1 || (next.channels!=1 && next.channels!=2) ||
       (next.bits!=8 && next.bits!=16))return fail(wave,BH_WAVE_UNSUPPORTED);
    if(!next.sample_rate || next.sample_rate>192000UL)return fail(wave,BH_WAVE_UNSUPPORTED);
    expected=next.channels*(next.bits/8);
    if(next.block_align!=expected || next.byte_rate!=expected*next.sample_rate ||
       view.sample_bytes%expected)return fail(wave,BH_WAVE_FORMAT);
    /* PCM may use the canonical 16-byte fmt or WAVEFORMATEX with bounded
       cbSize. Extra declared bytes are retained only in the borrowed view. */
    if(view.format_size!=16 && (view.format_size<18 || u16(p+16)!=view.format_size-18))
        return fail(wave,BH_WAVE_FORMAT);
    next.sample_bytes=view.sample_bytes;next.frames=view.sample_bytes/expected;
    if(next.sample_bytes) {
        next.samples=(unsigned char*)next.allocator.allocate(next.sample_bytes,next.allocator.user);
        if(!next.samples)return fail(wave,BH_WAVE_MEMORY);
        memcpy(next.samples,view.samples,next.sample_bytes);
    }
    bh_wave_dispose(wave);*wave=next;return 1;
}
int __cdecl bh_wave_read(BHWave *wave,BHVFile *file) {
    long start=-1,end;unsigned long size;unsigned char *data=NULL;int ok=0;
    if(!wave || !wave->allocator.allocate || !wave->allocator.release || !file)return fail(wave,BH_WAVE_ARGUMENT);
    if(bh_vfile_seek(file,0,SEEK_CUR))return fail(wave,BH_WAVE_IO);
    start=bh_vfile_tell(file);
    if(start<0 || bh_vfile_seek(file,0,SEEK_END))return fail(wave,BH_WAVE_IO);
    end=bh_vfile_tell(file);
    if(end<start || bh_vfile_seek(file,start,SEEK_SET))return fail(wave,BH_WAVE_IO);
    size=(unsigned long)(end-start);
    if(size>MAX_INPUT)fail(wave,BH_WAVE_LIMIT);
    else if(!size)fail(wave,BH_WAVE_TRUNCATED);
    else {
        data=(unsigned char*)wave->allocator.allocate(size,wave->allocator.user);
        if(!data)fail(wave,BH_WAVE_MEMORY);
        else if(bh_vfile_read(data,1,size,file)!=size || bh_vfile_error()!=BH_VFILE_OK)fail(wave,BH_WAVE_IO);
        else ok=bh_wave_decode(wave,data,size);
    }
    if(data)wave->allocator.release(data,wave->allocator.user);
    if(!ok && bh_vfile_seek(file,start,SEEK_SET))wave->error=BH_WAVE_IO;
    return ok;
}
