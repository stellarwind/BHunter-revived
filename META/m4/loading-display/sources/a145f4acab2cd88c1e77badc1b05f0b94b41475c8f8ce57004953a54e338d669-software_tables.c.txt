#include "../src/bh_software_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<12)printf("FAIL software-tables %d: %s\n",line,label);}}
typedef struct Memory {unsigned long calls,live,fail_at,sizes[3];} Memory;
static void *__cdecl allocate(unsigned long n,void *user) {
    Memory *m=(Memory *)user;void *p;++m->calls;if(m->calls<=3)m->sizes[m->calls-1]=n;
    if(m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;
}
static void __cdecl release(void *p,void *user){Memory *m=(Memory *)user;if(p){--m->live;free(p);}}
static int context(BHTextureContext *q,Memory *m,unsigned long mode) {
    DDPIXELFORMAT pixel;memset(q,0,sizeof(*q));memset(m,0,sizeof(*m));q->allocator.allocate=allocate;q->allocator.release=release;q->allocator.user=m;
    if(mode==1)return 1;
    memset(&pixel,0,sizeof(pixel));pixel.dwSize=sizeof(pixel);pixel.dwFlags=DDPF_RGB;pixel.dwRGBBitCount=16;
    pixel.dwRBitMask=mode==2?0xf800:0x7c00;pixel.dwGBitMask=mode==2?0x7e0:0x3e0;pixel.dwBBitMask=0x1f;
    return bh_texture_bind_display(q,&pixel);
}
static void palette(unsigned long index,unsigned long rgb[3]) {
    /* Independent mixed-radix decoding; high entries intentionally alias0..3. */
    unsigned long red=(index%252)/42,green=(index%42)/6,blue=index%6;
    rgb[0]=red*51;rgb[1]=(green*85)/2;rgb[2]=blue*51;
}
static unsigned long expected_index(const unsigned long rgb[3]) {
    /* Bucket boundaries rather than production quantization tables. */
    unsigned long c,k,digit[3],levels;
    for(c=0;c<3;++c){levels=c==1?7:6;digit[c]=0;for(k=1;k<levels;++k)if(rgb[c]>=(k*256+levels-1)/levels)digit[c]=k;}
    return digit[0]*42+digit[1]*6+digit[2];
}
static unsigned long expected_native(const unsigned long rgb[3],unsigned long mode) {
    /* Independent565/555 field arithmetic, no production display/pack helper. */
    return (rgb[0]/8)*(mode==2?2048:1024)+(rgb[1]/(mode==2?4:8))*32+rgb[2]/8;
}
static void exhaustive(unsigned long mode) {
    BHTextureContext q;BHSoftwareTables t;Memory memory;unsigned long masks[3],inner,outer,plane,c,offset,src[3],base[3],rgb[3],expected,weight;
    memset(&t,0,sizeof(t));CHECK(context(&q,&memory,mode),"real admitted display-shift binding");
    masks[0]=mode==2?0xf800:0x7c00;masks[1]=mode==2?0x7e0:0x3e0;masks[2]=0x1f;
    CHECK(bh_software_tables_initialize(&t,&q,mode==1?1:2,mode==1?NULL:masks),"complete original table initialization with real allocator/dependency");
    CHECK(q.tables.initialized && t.shade_indexed && t.bytes_per_pixel==(mode==1?1:2),"actual shared quantization initialization and always allocated indexed shades");
    if(!t.shade_indexed)return;
    CHECK(memory.sizes[0]==0x20000 && memory.calls==(mode==1?2:3) && memory.live==memory.calls,"original allocation count and first128KiB layout");
    if(mode==1)CHECK(memory.sizes[1]==0x80000 && t.blend_indexed && !t.shade_native && !t.blend_native,"indexed512KiB eight-plane layout");
    else {
        CHECK(memory.sizes[1]==0x40000 && memory.sizes[2]==0x100000 && !t.blend_indexed && t.shade_native && t.blend_native,"native256KiB shade and1MiB blend layouts");
        CHECK(t.rgb332_mask[0]==(mode==2?0xe000:0x7000) && t.rgb332_mask[1]==(mode==2?0x700:0x380) && t.rgb332_mask[2]==0x18,"original top3/3/2 runtime masks");
        CHECK(t.rgb332_shift[0]==(mode==2?0:1) && t.rgb332_shift[1]==(mode==2?2:3) && t.rgb332_shift[2]==5,"original runtime extraction shift formulas");
    }
    for(outer=0;outer<256;++outer)for(inner=0;inner<256;++inner) {
        palette(inner,src);offset=outer*256+inner;
        for(c=0;c<3;++c)rgb[c]=src[c]*outer/255;
        CHECK(t.shade_indexed[offset]==expected_index(rgb),"exhaustive indexed intensity table");
        if(mode!=1)CHECK(t.shade_native[offset]==expected_native(rgb,mode),"exhaustive native intensity table");
        rgb[0]=src[0]*(outer/32)/7;rgb[1]=src[1]*((outer/4)%8)/7;rgb[2]=src[2]*(outer%4)/3;
        CHECK(t.shade_indexed[65536+offset]==expected_index(rgb),"exhaustive indexed normalized RGB332 modulation");
        if(mode!=1)CHECK(t.shade_native[65536+offset]==expected_native(rgb,mode),"exhaustive native normalized RGB332 modulation");
    }
    for(plane=0;plane<8;++plane)for(outer=0;outer<256;++outer)for(inner=0;inner<256;++inner) {
        palette(inner,src);if(mode==1)palette(outer,base);else{base[0]=(outer/32)*32;base[1]=((outer/4)%8)*32;base[2]=(outer%4)*64;}
        for(c=0;c<3;++c) {
            /* Explicit mathematical definition independent of production helpers. */
            if(plane==0)rgb[c]=src[c]+(base[c]*(255-src[c]))/255;
            else if(plane==1)rgb[c]=(base[c]*(255-src[c]))/255;
            else if(plane==2)rgb[c]=base[c]+src[c];
            else {weight=plane-2;rgb[c]=(base[c]*(6-weight)+src[c]*weight)/6;}
            if(rgb[c]>255)rgb[c]=255;
        }
        offset=plane*65536+outer*256+inner;expected=mode==1?expected_index(rgb):expected_native(rgb,mode);
        CHECK((mode==1?t.blend_indexed[offset]:t.blend_native[offset])==expected,"exhaustive eight-plane blend definition");
    }
    CHECK(t.shade_indexed[255*256+251]==251 && t.shade_indexed[255*256+252]==0 && t.shade_indexed[255*256+255]==3,"full-intensity white and wrapped252..255 palette entries");
    if(mode!=1) {
        CHECK(t.shade_native[255*256+251]==(mode==2?0xffff:0x7fff),"full intensity white fills actual565/555 RGB fields");
        CHECK(t.blend_native[255*256]==(mode==2?0xe718:0x7398),"native RGB332 blend maximum is coarse224/224/192, not normalized white");
    }
    CHECK(!bh_software_tables_initialize(&t,&q,mode==1?1:2,masks),"live owner reinitialization guard prevents original leak");
    bh_software_tables_dispose(&t);CHECK(!memory.live && !t.shade_indexed && !t.blend_native,"real allocations released by explicit source adapter");
    bh_software_tables_dispose(&t);CHECK(!memory.live,"disposal idempotent");
}
static void guards(void) {
    BHTextureContext q;BHSoftwareTables t;Memory memory;unsigned long fail_at,masks[3]={0xf800,0x7e0,0x1f};
    for(fail_at=1;fail_at<=3;++fail_at) {
        memset(&t,0,sizeof(t));CHECK(context(&q,&memory,2),"failure fixture display");memory.fail_at=fail_at;
        CHECK(!bh_software_tables_initialize(&t,&q,2,masks) && !memory.live && !t.shade_indexed && !t.blend_native,"each allocation failure frees real partial allocations and leaves owner empty");
    }
    memset(&t,0,sizeof(t));context(&q,&memory,2);
    CHECK(!bh_software_tables_initialize(NULL,&q,2,masks) && !bh_software_tables_initialize(&t,NULL,2,masks),"null source binding guards");
    CHECK(!bh_software_tables_initialize(&t,&q,0,masks) && !bh_software_tables_initialize(&t,&q,3,masks),"unsupported pixel byte count explicitly rejected");
    masks[0]=0;CHECK(!bh_software_tables_initialize(&t,&q,2,masks)&&!memory.calls,"zero mask rejected instead of infinite original divide loop");
    masks[0]=0xf801;CHECK(!bh_software_tables_initialize(&t,&q,2,masks)&&!memory.calls,"noncontiguous mask rejected");
    masks[0]=0x7e0;CHECK(!bh_software_tables_initialize(&t,&q,2,masks)&&!memory.calls,"overlapping mask rejected");
    masks[0]=0xf800;q.display_left[0]=10;CHECK(!bh_software_tables_initialize(&t,&q,2,masks)&&!memory.calls,"mask and display shift mismatch rejected");
    q.display_left[0]=11;masks[0]=0x10000;CHECK(!bh_software_tables_initialize(&t,&q,2,masks)&&!memory.calls,"outside16bit mask rejected");
    q.allocator.allocate=NULL;CHECK(!bh_software_tables_initialize(&t,&q,1,NULL),"missing allocator rejects without guessed dependency");
}
int bh_software_tables_checkpoint(unsigned long *count) {
    checks=0;failures=0;exhaustive(1);exhaustive(2);exhaustive(3);guards();
    *count=checks;printf("{\"scenario\":\"software-tables\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SOFTWARE_TABLES_STANDALONE
int main(void){unsigned long count;return bh_software_tables_checkpoint(&count);}
#endif
