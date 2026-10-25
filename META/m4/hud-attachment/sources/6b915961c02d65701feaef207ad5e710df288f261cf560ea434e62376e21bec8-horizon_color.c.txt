#include "../src/bh_horizon_color.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;if(failures<15)printf("FAIL horizon-color: %s\n",s);}} while(0)
typedef struct Memory {unsigned long live,calls;int fail;} Memory;
static void *__cdecl allocate(unsigned long size,void *user){Memory *m=(Memory *)user;void *p;++m->calls;if(m->fail)return NULL;p=malloc(size);if(p)++m->live;return p;}
static void __cdecl release(void *p,void *user){Memory *m=(Memory *)user;if(p){--m->live;free(p);}}
static void setup(BHTextureContext *q,Memory *m) {
    memset(q,0,sizeof(*q));memset(m,0,sizeof(*m));q->allocator.allocate=allocate;q->allocator.release=release;q->allocator.user=m;
    q->display_right[0]=q->display_right[2]=3;q->display_right[1]=2;
    q->display_left[0]=11;q->display_left[1]=5;q->display_left[2]=0;
}
static unsigned long channel(unsigned long index,unsigned long row,unsigned long component,unsigned long color) {
    static const unsigned long stride[3]={42,6,1},steps[3]={6,7,6},shift[3]={16,8,0};
    unsigned long level=(index/stride[component])%steps[component];
    unsigned long cube=(level*255)/(steps[component]-1),constant=(color>>shift[component])&255;
    return (row*cube+(255-row)*constant)/255;
}
static void complete_tables(void) {
    BHTextureContext q;BHHorizonColor c;Memory m;unsigned long row,i,r,g,b,expected;
    setup(&q,&m);CHECK(bh_horizon_color_init(&c,&q) && bh_horizon_color_prepare(&c,1,0x123456),"indexed real initializer and allocation");
    for(row=0;row<256;++row)for(i=0;i<256;++i){r=channel(i,row,0,0x123456);g=channel(i,row,1,0x123456);b=channel(i,row,2,0x123456);
        expected=(((r*6)/256)*7+(g*7)/256)*6+(b*6)/256;
        CHECK(c.indexed[row*256+i]==expected,"all indexed blend/quantization entries match independent integer oracle");}
    CHECK(c.indexed[255*256]==c.indexed[255*256+252] && c.indexed[255*256+3]==c.indexed[255*256+255],"last four cube entries wrap exactly");
    CHECK(bh_horizon_color_prepare(&c,2,0x123456),"packed allocation despite equal shared key");
    for(row=0;row<256;++row)for(i=0;i<256;++i){r=channel(i,row,0,0x123456);g=channel(i,row,1,0x123456);b=channel(i,row,2,0x123456);
        expected=((r/8)*2048)+((g/4)*32)+(b/8);
        CHECK(c.packed[row*256+i]==expected,"all packed565 blend entries match independent division oracle");}
    CHECK(c.fills==2 && m.live==2 && m.calls==2,"two actual tables owned under shared key");
    bh_horizon_color_dispose(&c);CHECK(!m.live && !c.textures && !c.indexed && !c.packed,"actual allocator releases both tables");
}
static void cache(void) {
    BHTextureContext q;BHHorizonColor c;Memory m;unsigned char old;unsigned long fills;unsigned short packed;
    setup(&q,&m);CHECK(bh_horizon_color_init(&c,&q) && bh_horizon_color_prepare(&c,1,0xff0000),"cache initial indexed red");old=c.indexed[0];
    CHECK(bh_horizon_color_prepare(&c,0,0x0000ff) && c.fills==2,"mode zero selects packed branch");
    CHECK(bh_horizon_color_prepare(&c,1,0x0000ff) && c.fills==2 && c.indexed[0]==old,
        "shared key intentionally returns stale indexed red after packed blue");
    fills=c.fills;CHECK(bh_horizon_color_prepare(&c,1,0xab0000ff) && c.fills==fills+1 && c.indexed[0]!=old,
        "high color byte participates in key despite identical visible RGB");
    CHECK(bh_horizon_color_prepare(&c,99,0xab0000ff) && c.fills==fills+1,"all non-one modes reuse packed pointer and shared key");
    packed=c.packed[0];q.display_left[0]=q.display_left[1]=q.display_left[2]=0;
    CHECK(bh_horizon_color_prepare(&c,2,0xab0000ff) && c.packed[0]==packed && c.fills==fills+1,
        "format shift change alone does not invalidate cache");
    q.tables.initialized=0;CHECK(bh_horizon_color_prepare(&c,2,0xab0000ff) && q.tables.initialized && c.fills==fills+1,
        "real texture initializer runs even on cache hit");
    q.display_right[0]=35;q.display_right[1]=34;q.display_right[2]=35;
    q.display_left[0]=43;q.display_left[1]=37;q.display_left[2]=32;
    CHECK(bh_horizon_color_prepare(&c,7,0x010000ff) && c.packed[0]==31,
        "x86 shifts mask low five bits on actual miss");bh_horizon_color_dispose(&c);CHECK(!m.live,"cache buffers released");
}
static void failures_and_wrapping(void) {
    BHTextureContext q;BHHorizonColor c;Memory m;unsigned long i,expected,r,g,b;
    setup(&q,&m);CHECK(bh_horizon_color_init(&c,&q),"failure owner initialized");m.fail=1;
    CHECK(!bh_horizon_color_prepare(&c,1,0xabcdef) && c.error==BH_HORIZON_COLOR_MEMORY && !c.indexed && !c.fills && !c.color && q.tables.initialized,
        "indexed allocation guard preserves unpublished key while real tables initialize");
    CHECK(!bh_horizon_color_prepare(&c,0,0xabcdef) && !c.packed && !c.color && !c.fills,"packed allocation guard preserves shared key");
    m.fail=0;q.display_right[0]=q.display_right[1]=q.display_right[2]=0;q.display_left[0]=q.display_left[1]=q.display_left[2]=0;
    CHECK(bh_horizon_color_prepare(&c,2,0xffffff),"overlapping channel fields supported as original addition");
    CHECK(c.packed[0]==765,"packed fields add instead of bitwise OR");
    q.display_left[0]=31;q.display_left[1]=16;q.display_left[2]=8;
    CHECK(bh_horizon_color_prepare(&c,2,0x00feabcd),"large shift and low16 packed store");
    for(i=0;i<256;++i){r=channel(i,173,0,0xfeabcd);g=channel(i,173,1,0xfeabcd);b=channel(i,173,2,0xfeabcd);
        expected=(r<<31)+(g<<16)+(b<<8);CHECK(c.packed[173*256+i]==(unsigned short)expected,"unsigned32 shift/add wrap before low16 output");}
    bh_horizon_color_dispose(&c);CHECK(!m.live,"failure/wrap owner released");
    CHECK(!bh_horizon_color_prepare(NULL,1,0) && !bh_horizon_color_init(NULL,&q),"missing owners reject");
}
int bh_horizon_color_tests(void) {
    checks=failures=0;complete_tables();cache();failures_and_wrapping();
    printf("horizon-color: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_horizon_color_checkpoint(unsigned long *count) {
    bh_horizon_color_tests();if(count)*count=checks;
    printf("{\"scenario\":\"horizon-color\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_HORIZON_COLOR_STANDALONE
int main(void){unsigned long count=0;return bh_horizon_color_checkpoint(&count);}
#endif
