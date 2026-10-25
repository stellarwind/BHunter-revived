#include "bh_pcx.h"
#include <stdlib.h>
#include <string.h>
#define MAX_INPUT (64UL*1024UL*1024UL)
#define MAX_PIXELS (16UL*1024UL*1024UL)
typedef char pcx_integer_sizes[(sizeof(unsigned long)==4 && sizeof(unsigned short)==2)?1:-1];
static unsigned long u16(const unsigned char *p) {return (unsigned long)p[0]|((unsigned long)p[1]<<8);}
static void *__cdecl allocate(unsigned long bytes,void *user) {(void)user;return malloc(bytes);}
static void __cdecl release(void *p,void *user) {(void)user;free(p);}
static int fail(BHPcx *image,BHPcxError error) {if(image)image->error=error;return 0;}
int __cdecl bh_pcx_init(BHPcx *image,const BHSceneAllocator *allocator) {
    if(!image)return 0;memset(image,0,sizeof(*image));
    if(allocator) {
        if(!allocator->allocate || !allocator->release)return fail(image,BH_PCX_ARGUMENT);
        image->allocator=*allocator;
    } else {image->allocator.allocate=allocate;image->allocator.release=release;}
    return 1;
}
void __cdecl bh_pcx_dispose(BHPcx *image) {
    BHSceneAllocator allocator;if(!image)return;allocator=image->allocator;
    if(image->pixels && allocator.release)allocator.release(image->pixels,allocator.user);
    memset(image,0,sizeof(*image));image->allocator=allocator;
}
int __cdecl bh_pcx_decode(BHPcx *image,const void *bytes,unsigned long size) {
    const unsigned char *p=(const unsigned char*)bytes;
    unsigned long width,height,pixels,end,at,out,count;unsigned char value;BHPcx next;
    if(!image || !image->allocator.allocate || !image->allocator.release || !p)return fail(image,BH_PCX_ARGUMENT);
    if(size>MAX_INPUT)return fail(image,BH_PCX_LIMIT);
    if(size<128)return fail(image,BH_PCX_TRUNCATED);
    if(p[0]!=10)return fail(image,BH_PCX_FORMAT);
    if(p[1]!=5 || p[2]!=1 || p[3]!=8 || p[65]!=1 || u16(p+4) || u16(p+6))return fail(image,BH_PCX_UNSUPPORTED);
    width=u16(p+8)+1;height=u16(p+10)+1;
    if(width>4096 || height>4096)return fail(image,BH_PCX_LIMIT);
    if(u16(p+66)!=width)return fail(image,BH_PCX_UNSUPPORTED);
    pixels=width*height;
    if(pixels>MAX_PIXELS)return fail(image,BH_PCX_LIMIT);
    if(size<128+769)return fail(image,BH_PCX_TRUNCATED);
    end=size-769;
    if(p[end]!=12)return fail(image,BH_PCX_FORMAT);
    bh_pcx_init(&next,&image->allocator);next.width=width;next.height=height;next.pixel_bytes=pixels;
    next.pixels=(unsigned char*)next.allocator.allocate(pixels,next.allocator.user);
    if(!next.pixels)return fail(image,BH_PCX_MEMORY);
    out=0;at=128;
    while(out<pixels) {
        if(at==end) {bh_pcx_dispose(&next);return fail(image,BH_PCX_TRUNCATED);}
        value=p[at++];
        if(value==0xc0)continue; /* observed zero-run marker consumes no value */
        count=1;
        if(value>0xc0) {
            count=value&63;
            if(at==end) {bh_pcx_dispose(&next);return fail(image,BH_PCX_TRUNCATED);}
            value=p[at++];
        }
        if(count>pixels-out) {bh_pcx_dispose(&next);return fail(image,BH_PCX_FORMAT);}
        memset(next.pixels+out,value,count);out+=count;
    }
    if(at!=end) {bh_pcx_dispose(&next);return fail(image,BH_PCX_FORMAT);}
    memcpy(next.palette,p+end+1,768);
    bh_pcx_dispose(image);*image=next;return 1;
}
int __cdecl bh_pcx_read(BHPcx *image,BHVFile *file) {
    long start=-1,end;unsigned long size;unsigned char *data=NULL;int ok=0;
    if(!image || !image->allocator.allocate || !image->allocator.release || !file)return fail(image,BH_PCX_ARGUMENT);
    if(bh_vfile_seek(file,0,SEEK_CUR))return fail(image,BH_PCX_IO);
    start=bh_vfile_tell(file);
    if(start<0 || bh_vfile_seek(file,0,SEEK_END))return fail(image,BH_PCX_IO);
    end=bh_vfile_tell(file);
    if(end<start || bh_vfile_seek(file,start,SEEK_SET))return fail(image,BH_PCX_IO);
    size=(unsigned long)(end-start);
    if(size>MAX_INPUT)fail(image,BH_PCX_LIMIT);
    else if(!size)fail(image,BH_PCX_TRUNCATED);
    else {
        data=(unsigned char*)image->allocator.allocate(size,image->allocator.user);
        if(!data)fail(image,BH_PCX_MEMORY);
        else if(bh_vfile_read(data,1,size,file)!=size || bh_vfile_error()!=BH_VFILE_OK)fail(image,BH_PCX_IO);
        else ok=bh_pcx_decode(image,data,size);
    }
    if(data)image->allocator.release(data,image->allocator.user);
    if(!ok && bh_vfile_seek(file,start,SEEK_SET))image->error=BH_PCX_IO;
    return ok;
}
static unsigned long packed(const unsigned char *p) {
    return ((unsigned long)p[0]<<16)|((unsigned long)p[1]<<8)|p[2];
}
static int distance(const unsigned char *p,const unsigned char *q) {
    return abs((int)p[0]-q[0])+abs((int)p[1]-q[1])+abs((int)p[2]-q[2]);
}
static unsigned char nearest(const unsigned char *rgb) {
    int best=distance(rgb,rgb+3),d;unsigned long i;unsigned char index=1;
    for(i=2;i<256;i++){d=distance(rgb,rgb+i*3);if(d<best){best=d;index=(unsigned char)i;}}
    return index;
}
int __cdecl bh_pcx_palette(BHPcxPalette *output,const unsigned char *rgb,int mode,unsigned long color) {
    BHPcxPalette next;unsigned long i;int match=-1,delta,d;unsigned char *p;
    if(!output || !rgb)return 0;memcpy(next.rgb,rgb,768);
    for(i=0;i<256;i++)next.indices[i]=(unsigned char)i;
    if(mode==2) {
        for(i=0;i<256;i++) {p=next.rgb+i*3;if(p[0]<5 && p[1]<5 && p[2]<5)memset(p,0,3);}
        if(packed(next.rgb))next.indices[0]=nearest(next.rgb);
        for(i=1;i<256;i++)if(!packed(next.rgb+i*3))next.indices[i]=0;
    } else {
        if(mode)for(i=1;i<256;i++)if(packed(next.rgb+i*3)==color){next.indices[i]=0;match=(int)i;}
        if(packed(next.rgb)!=color || !mode) {
            if(match<0)next.indices[0]=nearest(next.rgb);
            else {next.indices[0]=(unsigned char)match;memcpy(next.rgb+match*3,next.rgb,3);}
        }
        for(i=1;i<256;i++) {
            p=next.rgb+i*3;delta=8-p[0];if(delta<0)delta=0;
            d=8-p[1];if(d<0)d=0;if(d<delta)delta=d;
            d=8-p[2];if(d<0)d=0;if(d<delta)delta=d;
            p[0]=(unsigned char)(p[0]+delta);p[1]=(unsigned char)(p[1]+delta);p[2]=(unsigned char)(p[2]+delta);
        }
    }
    memset(next.rgb,0,3);*output=next;return 1;
}
