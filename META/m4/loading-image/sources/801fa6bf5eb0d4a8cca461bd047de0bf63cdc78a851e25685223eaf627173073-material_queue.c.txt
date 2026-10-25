#include "../src/bh_material_queue.h"
#include "../src/bh_clipper.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef char image_words_layout[offsetof(BHTextureImage,unused54)==0x54?1:-1];
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL material-queue: %s\n",s);}}while(0)
typedef struct Images {BHTextureImage first,second;} Images;
static BHTextureImage *__cdecl lookup(unsigned long handle,void *user){
    Images *images=(Images *)user;
    return handle==17?&images->first:handle==29?&images->second:NULL;
}
static int enqueue(BHDrawQueue *q,unsigned long bucket,unsigned long handle){
    return bh_material_queue_words(q,bucket,handle,1,0x3d4ccccdUL,0,
        0x80000000UL,0x7fc01234UL,0x3f800000UL,0xff800000UL);
}
static unsigned long word(float value){unsigned long bits;memcpy(&bits,&value,4);return bits;}
static void projected_uv(void){
    BHDrawQueue q;BHClipContext clip;BHClipVertex vertex[4],*ring;Images images;
    unsigned long i,next;float expected_u[4]={8,12,12,8},expected_v[4]={19,27,27,19};
    memset(&images,0,sizeof(images));memset(vertex,0,sizeof(vertex));
    CHECK(bh_draw_queue_init(&q,10,NULL) && bh_clip_init(&clip,1,100,-10,10,-10,10),
        "real clipper and queue composition initialized");
    for(i=0;i<4;++i){
        vertex[i].point.x=(i==1 || i==2)?3:1;
        vertex[i].point.y=i<2?-1:1;vertex[i].point.z=2;
        vertex[i].u=-99;vertex[i].v=-99;
        vertex[i].next=vertex+(i+1)%4;vertex[i].previous=vertex+(i+3)%4;
    }
    CHECK(bh_material_queue_words(&q,5,17,1,word(2),word(3),word(0),word(4),word(5),word(5))==1 &&
        bh_material_queue_execute(lookup,&images,q.commands+9,&next),"queued policy installed on selected image");
    ring=bh_clip_polygon(&clip,vertex,&images.first);
    CHECK(ring && clip.error==BH_CLIP_OK && clip.inverse_ready,"real clipping consumes image projected-UV policy");
    for(i=0;i<4;++i)CHECK(vertex[i].u==expected_u[i] && vertex[i].v==expected_v[i],
        "world x/z affine UV terms applied to each actual vertex");
    CHECK(bh_draw_queue_begin(&q) && bh_material_queue_words(&q,5,17,0,0,0,0,0,0,0)==1 &&
        bh_material_queue_execute(lookup,&images,q.commands+9,&next),"second queued policy disables projected UV");
    for(i=0;i<4;++i){vertex[i].u=7;vertex[i].v=9;}
    CHECK(bh_clip_polygon(&clip,vertex,&images.first)!=NULL && vertex[0].u==7 && vertex[3].v==9,
        "disabled policy preserves authored UV coordinates");
    CHECK(bh_draw_queue_dispose(&q),"composed queue disposed");
}
static void functional(void){
    BHDrawQueue q;Images images,before;unsigned long i,next,header;
    static const unsigned long mapping[11]={1,2,3,4,5,0,6,7,8,0,0};
    static const unsigned long buckets[11]={0,1,2,3,4,5,6,7,8,9,0xffffffffUL};
    unsigned long expected[7]={1,0x3d4ccccdUL,0,0x80000000UL,0x7fc01234UL,0x3f800000UL,0xff800000UL};
    memset(&images,0x5a,sizeof(images));before=images;
    CHECK(bh_draw_queue_init(&q,12,NULL),"real queue owner initialized");
    for(i=0;i<11;++i){
        CHECK(bh_draw_queue_begin(&q),"reset source queue");header=mapping[i];
        CHECK(enqueue(&q,buckets[i],17)==1 && q.count==10,"enqueue raw policy");
        CHECK(q.commands[header].next==9 && q.commands[header].resource==9 &&
            q.commands[9].next==(header==8?BH_DRAW_END:header+1),"exact bucket/default mapping and chain");
        CHECK(q.commands[9].kind==BH_DRAW_MATERIAL_WORDS && q.commands[9].resource==17 &&
            !memcmp(q.commands[9].words,expected,28),"all raw payload bits retained");
        next=123;
        CHECK(bh_material_queue_execute(lookup,&images,q.commands+9,&next) &&
            next==q.commands[9].next,"consumer executes setter and publishes next");
        CHECK(!memcmp(&images.first.unused54,expected,28),"real image owner receives seven words");
        CHECK(!memcmp(&images.second,&before.second,sizeof(images.second)) &&
            !memcmp(&images.first,&before.first,0x54),"unselected image and preceding fields unchanged");
    }
    CHECK(bh_draw_queue_begin(&q) && enqueue(&q,5,17)==1 && enqueue(&q,5,29)==1,
        "two deferred updates queued");
    CHECK(q.commands[0].next==9 && q.commands[9].next==10 && q.commands[10].next==1,
        "FIFO before numbered drawing buckets");
    CHECK(bh_material_queue_execute(lookup,&images,q.commands+10,&next) &&
        !memcmp(&images.second.unused54,expected,28),"second source resource lookup resolves independently");
    CHECK(enqueue(&q,5,17)==1 && enqueue(&q,5,17)==0 && q.count==12 && q.error==BH_DRAW_FULL,
        "full queue drops update without allocating");
    CHECK(bh_draw_queue_begin(&q) && enqueue(&q,8,999)==1,"unknown source handle queues without guessing pointer");
    next=123;
    CHECK(!bh_material_queue_execute(lookup,&images,q.commands+9,&next) && next==BH_DRAW_END,
        "missing resource guard still publishes following link");
    next=123;
    CHECK(!bh_material_queue_execute(NULL,&images,q.commands+9,&next) && next==123,
        "missing lookup rejected");
    CHECK(!bh_material_queue_execute(lookup,&images,q.commands+9,NULL) &&
        !bh_material_queue_execute(lookup,&images,q.commands+1,&next),"wrong kind and missing output rejected");
    CHECK(bh_draw_queue_begin(&q),"topology fixture reset");q.commands[0].resource=99;
    CHECK(enqueue(&q,5,17)==-1 && q.error==BH_DRAW_TOPOLOGY && q.count==9,"invalid tail rejected");
    CHECK(enqueue(NULL,5,17)==-1,"missing queue rejected");
    CHECK(bh_draw_queue_dispose(&q),"real queue disposed");
}
int bh_material_queue_checkpoint(unsigned long *count){
    checks=failures=0;functional();projected_uv();if(count)*count=checks;
    printf("{\"scenario\":\"material-queue\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_MATERIAL_QUEUE_STANDALONE
int main(void){unsigned long count=0;return bh_material_queue_checkpoint(&count);}
#endif
