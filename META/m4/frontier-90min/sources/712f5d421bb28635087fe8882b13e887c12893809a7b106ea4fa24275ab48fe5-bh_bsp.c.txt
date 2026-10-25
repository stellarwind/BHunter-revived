#include "bh_bsp.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef char material_size[sizeof(BHBspMaterial)==32 ? 1:-1];
typedef char vertex_size[sizeof(BHBspVertex)==60 ? 1:-1];
typedef char edge_size[sizeof(BHBspEdge)==16 ? 1:-1];
typedef char face_size[sizeof(BHBspFace)==12 ? 1:-1];
typedef char node_size[sizeof(BHBspNode)==56 ? 1:-1];
typedef char leaf_size[sizeof(BHBspLeaf)==8 ? 1:-1];
static const unsigned long strides[]={32,60,16,12,56,8};
static void *__cdecl allocate(unsigned long n,void *user){return malloc(n);}
static void __cdecl release(void *p,void *user){free(p);}
static int fail(BHBsp *bsp,BHBspError error){if(bsp)bsp->error=error;return 0;}
static unsigned long u32(const unsigned char *p){return (unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);}
static float f32(const unsigned char *p){float v;memcpy(&v,p,4);return v;}
static int finite(const unsigned char *p){return (u32(p)&0x7f800000UL)!=0x7f800000UL;}
static int index(unsigned long v,unsigned long n,int sentinel){return v<n || (sentinel && v==0xffffffffUL);}
static float midpoint(float minimum,float maximum){float value;bh_round_float(&value,(double)minimum+maximum);bh_round_float(&value,(double)value*0.5);return value;}

int __cdecl bh_bsp_init(BHBsp *bsp,const BHSceneAllocator *allocator) {
    BHSceneAllocator memory;
    if(!bsp || (allocator && (!allocator->allocate || !allocator->release)))return 0;
    if(allocator)memory=*allocator;else{memory.allocate=allocate;memory.release=release;memory.user=NULL;}
    memset(bsp,0,sizeof(*bsp));bsp->allocator=memory;return 1;
}
void __cdecl bh_bsp_dispose(BHBsp *bsp) {
    BHSceneAllocator memory;
    if(!bsp)return;memory=bsp->allocator;
    if(bsp->storage)memory.release(bsp->storage,memory.user);
    if(bsp->bounds)memory.release(bsp->bounds,memory.user);
    memset(bsp,0,sizeof(*bsp));bsp->allocator=memory;
}

/* Functional next chains must terminate; shared tails are valid. */
static int chain(const unsigned char *data,unsigned long n,unsigned long stride,unsigned char *color) {
    unsigned long i,at,next;
    memset(color,0,n);
    for(i=0;i<n;i++) {
        at=i;
        while(at!=0xffffffffUL && color[at]==0){color[at]=1;at=u32(data+at*stride+4);}
        if(at!=0xffffffffUL && color[at]==1)return 0;
        at=i;
        while(at!=0xffffffffUL && color[at]==1){color[at]=2;next=u32(data+at*stride+4);at=next;}
    }
    return 1;
}
static int tree(const unsigned char *data,unsigned long n,unsigned char *work) {
    unsigned char *color=work,*phase=work+n;
    unsigned long *stack=(unsigned long*)(work+((2*n+3)&~3UL));
    unsigned long i,at,child;long top;
    memset(color,0,n);memset(phase,0,n);
    for(i=0;i<n;i++)if(!color[i]) {
        top=0;stack[0]=i;color[i]=1;
        while(top>=0) {
            at=stack[top];
            if(phase[at]==2){color[at]=2;--top;continue;}
            child=u32(data+at*56+48+4*phase[at]++);
            if(child==0xffffffffUL || color[child]==2)continue;
            if(color[child]==1)return 0;
            color[child]=1;stack[++top]=child;
        }
    }
    return 1;
}

int __cdecl bh_bsp_decode(BHBsp *bsp,const void *bytes,unsigned long size) {
    const unsigned char *data=(const unsigned char*)bytes,*p;
    unsigned long n[6],offsets[6],at=0,i,j,k,maximum=0;
    unsigned char *work;
    BHBsp next;
    double x,y,z;
    float a,b;
    float rounded_x,rounded_z,squared;
    BHBspBound *bound;
    if(!bsp || !bsp->allocator.allocate || !data)return fail(bsp,BH_BSP_ARGUMENT);
    if(size>16UL*1024*1024)return fail(bsp,BH_BSP_LIMIT);
    if(!bh_math_environment_supported())return fail(bsp,BH_BSP_NUMERICAL);
    for(i=0;i<6;i++) {
        if(size-at<4)return fail(bsp,BH_BSP_TRUNCATED);
        n[i]=u32(data+at);at+=4;offsets[i]=at;
        if(!n[i] || (n[i]&0x80000000UL))return fail(bsp,BH_BSP_FORMAT);
        if(n[i]>65535)return fail(bsp,BH_BSP_LIMIT);
        if(n[i]>(size-at)/strides[i])return fail(bsp,BH_BSP_TRUNCATED);
        at+=n[i]*strides[i];if(n[i]>maximum)maximum=n[i];
    }
    if(at!=size)return fail(bsp,BH_BSP_FORMAT);
    for(i=0;i<n[0];i++)if(!memchr(data+offsets[0]+i*32+9,0,23))return fail(bsp,BH_BSP_FORMAT);
    for(i=0;i<n[1];i++)for(j=0;j<3;j++)if(!finite(data+offsets[1]+i*60+j*4))return fail(bsp,BH_BSP_FORMAT);
    for(i=0;i<n[2];i++) {
        p=data+offsets[2]+i*16;
        if(!index(u32(p),n[1],0) || !index(u32(p+4),n[2],1) || !finite(p+8) || !finite(p+12))return fail(bsp,BH_BSP_FORMAT);
    }
    for(i=0;i<n[3];i++) {
        p=data+offsets[3]+i*12;
        if(!index(u32(p),n[0],0) || !index(u32(p+4),n[2],0))return fail(bsp,BH_BSP_FORMAT);
    }
    for(i=0;i<n[4];i++) {
        p=data+offsets[4]+i*56;
        for(j=0;j<10;j++)if(!finite(p+j*4))return fail(bsp,BH_BSP_FORMAT);
        for(j=0;j<3;j++)if(f32(p+j*4)>f32(p+12+j*4))return fail(bsp,BH_BSP_FORMAT);
        if(!index(u32(p+40),n[5],1) || !index(u32(p+44),n[5],1) ||
           !index(u32(p+48),n[4],1) || !index(u32(p+52),n[4],1))return fail(bsp,BH_BSP_FORMAT);
    }
    for(i=0;i<n[5];i++) {
        p=data+offsets[5]+i*8;
        if(!index(u32(p),n[3],0) || !index(u32(p+4),n[5],1))return fail(bsp,BH_BSP_FORMAT);
    }
    work=(unsigned char*)bsp->allocator.allocate(6*maximum+4,bsp->allocator.user);
    if(!work)return fail(bsp,BH_BSP_MEMORY);
    k=chain(data+offsets[2],n[2],16,work) && chain(data+offsets[5],n[5],8,work) && tree(data+offsets[4],n[4],work);
    bsp->allocator.release(work,bsp->allocator.user);
    if(!k)return fail(bsp,BH_BSP_CYCLE);
    bh_bsp_init(&next,&bsp->allocator);
    next.storage=(unsigned char*)next.allocator.allocate(size,next.allocator.user);
    if(!next.storage)return fail(bsp,BH_BSP_MEMORY);
    next.bounds=(BHBspBound*)next.allocator.allocate(n[4]*sizeof(BHBspBound),next.allocator.user);
    if(!next.bounds){bh_bsp_dispose(&next);return fail(bsp,BH_BSP_MEMORY);}
    memcpy(next.storage,data,size);next.bytes=size;memcpy(next.counts,n,sizeof(n));
    next.materials=(const BHBspMaterial*)(next.storage+offsets[0]);next.vertices=(const BHBspVertex*)(next.storage+offsets[1]);
    next.edges=(const BHBspEdge*)(next.storage+offsets[2]);next.faces=(const BHBspFace*)(next.storage+offsets[3]);
    next.nodes=(const BHBspNode*)(next.storage+offsets[4]);next.leaves=(const BHBspLeaf*)(next.storage+offsets[5]);
    for(i=0;i<n[4];i++) {
        bound=next.bounds+i;
        /* Keep the original collision-loader float stores before the halves. */
        bound->center.x=midpoint(next.nodes[i].minimum[0],next.nodes[i].maximum[0]);
        bound->center.y=midpoint(next.nodes[i].minimum[1],next.nodes[i].maximum[1]);
        bound->center.z=midpoint(next.nodes[i].minimum[2],next.nodes[i].maximum[2]);
        x=(double)next.nodes[i].maximum[0]-bound->center.x;y=(double)next.nodes[i].maximum[1]-bound->center.y;z=(double)next.nodes[i].maximum[2]-bound->center.z;
        /* 0044b6a4/0044b6be retain x/z in x87 while also storing a float
           operand for their multiply. y is squared at current precision.
           Separate-TU storage prevents pinned VC5 reusing excess precision. */
        bh_round_float(&rounded_x,x);bh_round_float(&rounded_z,z);
        bh_round_float(&squared,(x*rounded_x+y*y)+z*rounded_z);
        bound->radius_squared=squared;bound->radius=(float)sqrt(squared);
        if(!_finite(bound->radius_squared) || !_finite(bound->radius)){bh_bsp_dispose(&next);return fail(bsp,BH_BSP_FORMAT);}
    }
    p=data+offsets[4];a=(float)fabs(f32(p));b=(float)fabs(f32(p+12));x=a<b?b:a;
    a=(float)fabs(f32(p+4));b=(float)fabs(f32(p+16));y=a<b?b:a;
    a=(float)fabs(f32(p+8));b=(float)fabs(f32(p+20));z=a<b?b:a;
    next.root_radius=(float)sqrt((z*z+y*y)+x*x);
    if(!_finite(next.root_radius)){bh_bsp_dispose(&next);return fail(bsp,BH_BSP_FORMAT);}
    bh_bsp_dispose(bsp);*bsp=next;return 1;
}

int __cdecl bh_bsp_read(BHBsp *bsp,BHVFile *file) {
    long start,end;unsigned long size;unsigned char *bytes;int ok;
    if(!bsp || !bsp->allocator.allocate || !file)return fail(bsp,BH_BSP_ARGUMENT);
    if(bh_vfile_seek(file,0,SEEK_CUR) || (start=bh_vfile_tell(file))<0)return fail(bsp,BH_BSP_IO);
    if(bh_vfile_seek(file,0,SEEK_END) || (end=bh_vfile_tell(file))<start || bh_vfile_seek(file,start,SEEK_SET)) {
        bh_vfile_seek(file,start,SEEK_SET);return fail(bsp,BH_BSP_IO);
    }
    size=(unsigned long)(end-start);
    if(size>16UL*1024*1024)return fail(bsp,BH_BSP_LIMIT);
    if(!size)return fail(bsp,BH_BSP_TRUNCATED);
    bytes=(unsigned char*)bsp->allocator.allocate(size,bsp->allocator.user);
    if(!bytes)return fail(bsp,BH_BSP_MEMORY);
    ok=bh_vfile_read(bytes,1,size,file)==size && bh_vfile_error()==BH_VFILE_OK;
    if(ok)ok=bh_bsp_decode(bsp,bytes,size);else fail(bsp,BH_BSP_IO);
    bsp->allocator.release(bytes,bsp->allocator.user);
    if(!ok && bh_vfile_seek(file,start,SEEK_SET))bsp->error=BH_BSP_IO;
    return ok;
}

int __cdecl bh_bsp_load(BHBsp *bsp,const char *name,const char *prefix,BHBspSearch search) {
    char path[256];unsigned long length=0,prefix_length=0;BHVFile *file=NULL;int ok;
    BHBsp next;BHBspError error;
    if(!bsp || !name || !prefix || search<BH_BSP_BARE_ONLY || search>BH_BSP_PREFIX_ONLY)return fail(bsp,BH_BSP_ARGUMENT);
    while(length<256 && name[length])++length;while(prefix_length<256 && prefix[prefix_length])++prefix_length;
    if(!length || length==256 || prefix_length==256 || length+prefix_length>=256)return fail(bsp,BH_BSP_LIMIT);
    memcpy(path,prefix,prefix_length);memcpy(path+prefix_length,name,length+1);
    if(search==BH_BSP_PREFIX_FIRST || search==BH_BSP_PREFIX_ONLY)file=bh_vfile_open(path,"rb");
    else file=bh_vfile_open(name,"rb");
    if(!file && search==BH_BSP_PREFIX_FIRST)file=bh_vfile_open(name,"rb");
    else if(!file && search==BH_BSP_BARE_FIRST)file=bh_vfile_open(path,"rb");
    if(!file)return fail(bsp,BH_BSP_IO);
    if(!bh_bsp_init(&next,&bsp->allocator)){bh_vfile_close(file);return fail(bsp,BH_BSP_ARGUMENT);}
    ok=bh_bsp_read(&next,file);if(bh_vfile_close(file) && ok){next.error=BH_BSP_IO;ok=0;}
    if(ok){bh_bsp_dispose(bsp);*bsp=next;return 1;}
    error=next.error;bh_bsp_dispose(&next);return fail(bsp,error);
}
