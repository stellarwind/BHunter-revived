#include "../src/bh_segment_queue.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL segment-queue: %s\n",s);}}while(0)
typedef struct Observation {
    BHSegmentQueueContext *context;BHDrawCommand *command;BHTextureImage *image;
    BHSpriteOrientedVertex before[8];
    unsigned long lookup_calls,emit_calls,handle,used,vertices;
    float scalar[8],u[8];int missing,reject,change_next;
} Observation;
/* Isolated observation boundaries around the real transform/clip/ribbon owners.
   These do not replace any recovered geometry or production image owner. */
static BHTextureImage *__cdecl lookup(unsigned long handle,void *raw) {
    Observation *o=(Observation *)raw;++o->lookup_calls;o->handle=handle;
    o->used=o->context->draw->used;
    memcpy(o->before,o->context->draw->path,o->used*sizeof(o->before[0]));
    return o->missing?NULL:o->image;
}
static int __cdecl observe(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *raw) {
    Observation *o=(Observation *)raw;BHClipVertex *p=ring;unsigned long n=0;
    ++o->emit_calls;
    do{
        if(n<8){memcpy(o->scalar+n,&p->unknown28,4);o->u[n]=p->u;}
        ++n;p=p->next;
    }while(p!=ring && n<16);
    o->vertices=n;
    if(o->change_next)o->command->next=73;
    return !o->reject;
}
static void setup(BHSegmentQueueContext *c,BHMainCamera *camera,
    BHSpriteOrientedContext *draw,BHSpriteOrientedVertex *pool,
    BHAttributeClipContext *clip,BHTextureImage *image,Observation *o) {
    memset(c,0,sizeof(*c));memset(camera,0,sizeof(*camera));memset(draw,0,sizeof(*draw));
    memset(pool,0xa5,8*sizeof(*pool));memset(image,0,sizeof(*image));memset(o,0,sizeof(*o));
    camera->projected.m[0][0]=camera->projected.m[1][1]=camera->projected.m[2][2]=1;
    bh_attribute_clip_init(clip,1,100,-10,10,-10,10);
    draw->path=pool;draw->capacity=8;draw->near_z=1;draw->projection_scale=2;
    draw->clip=clip;draw->emit=observe;draw->emit_user=o;
    c->camera=camera;c->draw=draw;c->lookup=lookup;c->lookup_user=o;
    o->context=c;o->image=image;
}
int bh_segment_queue_checkpoint(unsigned long *count) {
    BHSegmentQueueContext c;BHMainCamera camera;BHSpriteOrientedContext draw;
    BHSpriteOrientedVertex pool[8],copy;BHAttributeClipContext clip;BHTextureImage image;
    Observation o;BHDrawQueue queue;BHDrawCommand *command;BHMathEnvironment fp;
    BHVec3 a={0,0,2},b={2,0,2};unsigned long next,i,flags;
    checks=failures=0;memset(&queue,0,sizeof(queue));
    if(!bh_math_environment_enter(&fp))return 2;
    setup(&c,&camera,&draw,pool,&clip,&image,&o);copy=pool[0];
    camera.projected.m[0][0]=2;camera.projected.m[0][3]=4;
    camera.projected.m[1][1]=3;camera.projected.m[1][3]=-2;
    camera.projected.m[2][2]=4;camera.projected.m[2][3]=1;
    CHECK(bh_segment_queue_append(&c,&b,-3,0xfedcba98UL,-.25f),"complete append accepts signed width/modulation and raw flags");
    CHECK(draw.used==1 && c.accepted==1 && !pool[0].previous && !pool[0].next,"append publishes first pool count and cleared links");
    CHECK(pool[0].point.x==8 && pool[0].point.y==-2 && pool[0].point.z==9,"actual camera matrix transforms copied endpoint");
    CHECK(pool[0].width==-3 && pool[0].flags==0xfedcba98UL && pool[0].texture_start==-.25f && pool[0].texture_end==0,"exact width flags modulation and implicit first UV");
    CHECK(!memcmp(pool[0].corners,copy.corners,sizeof(copy.corners)),"original corner scratch remains untouched");
    for(i=1;i<8;++i){
        CHECK(bh_segment_queue_append(&c,&a,1,i,.5f),"append to existing real pool");
        CHECK(pool[i].previous==pool+i-1 && pool[i-1].next==pool+i && !pool[i].next && pool[i].texture_end==(float)i,
            "implicit signed used-minus-one UV and adjacent bidirectional links");
    }
    copy=pool[7];CHECK(!bh_segment_queue_append(&c,&a,1,0,1) && c.error==BH_SEGMENT_QUEUE_CAPACITY && draw.used==8 &&
        !memcmp(&copy,pool+7,sizeof(copy)),"capacity guard leaves prior complete pool state");
    CHECK(bh_draw_queue_init(&queue,20,NULL),"actual segment producer queue owner");if(!queue.commands)goto done;
    for(flags=0;flags<8;++flags){
        setup(&c,&camera,&draw,pool,&clip,&image,&o);bh_draw_queue_begin(&queue);
        CHECK(bh_draw_segment(&queue,3,19,&a,&b,1,1,.2f,.8f,flags)==1,"real00478390 producer creates segment payload");
        command=queue.commands+9;o.command=command;next=99;
        CHECK(bh_segment_queue_execute(&c,command,&next) && next==command->next && c.error==BH_SEGMENT_QUEUE_OK,
            "complete consumer dispatches actual append and corrected ribbon owner");
        CHECK(c.accepted==2 && !draw.used && o.lookup_calls==1 && o.used==2 && o.handle==19 && draw.image==&image,
            "two appends precede lookup and successful ribbon owns count reset");
        CHECK(o.before[0].point.x==0 && o.before[1].point.x==2 && o.before[0].point.z==2 &&
            o.before[0].width==1 && o.before[1].width==1 && o.before[0].flags==flags && o.before[1].flags==flags &&
            o.before[0].texture_start==.2f && o.before[1].texture_start==.8f &&
            o.before[0].texture_end==0 && o.before[1].texture_end==1,"whole endpoint payload and implicit longitudinal UV reach owning draw");
        CHECK(o.emit_calls==1 && o.vertices==4 && draw.paths_emitted==1,"all UV flip/exchange flags reach real polygon construction");
    }
    setup(&c,&camera,&draw,pool,&clip,&image,&o);o.command=command;o.change_next=1;
    CHECK(bh_segment_queue_execute(&c,command,&next) && next==73,"next link is loaded after synchronous primitive emission");
    setup(&c,&camera,&draw,pool,&clip,&image,&o);o.command=command;o.change_next=1;o.reject=1;command->next=61;
    CHECK(!bh_segment_queue_execute(&c,command,&next) && c.error==BH_SEGMENT_QUEUE_DRAW && next==73 && draw.used==2,
        "source emitter rejection retains real pool and reads changed next");
    setup(&c,&camera,&draw,pool,&clip,&image,&o);o.missing=1;command->next=62;
    CHECK(!bh_segment_queue_execute(&c,command,&next) && c.error==BH_SEGMENT_QUEUE_IMAGE && next==62 && draw.used==2 &&
        c.accepted==2 && !o.emit_calls,"unbound image fails after both source appends without fabricating output");
    setup(&c,&camera,&draw,pool,&clip,&image,&o);draw.capacity=1;
    CHECK(!bh_segment_queue_execute(&c,command,&next) && c.error==BH_SEGMENT_QUEUE_CAPACITY && draw.used==1 &&
        c.accepted==1 && !o.lookup_calls && next==62,"second append capacity error preserves first side effect");
    setup(&c,&camera,&draw,pool,&clip,&image,&o);bh_draw_queue_begin(&queue);
    a.z=b.z=.5f;bh_draw_segment(&queue,3,19,&a,&b,1,1,.2f,.8f,0);command=queue.commands+9;
    CHECK(bh_segment_queue_execute(&c,command,&next) && !draw.used && !o.emit_calls,"fully near-clipped segment resets without polygons");
    for(i=0;i<2;++i){
        setup(&c,&camera,&draw,pool,&clip,&image,&o);bh_draw_queue_begin(&queue);
        a.z=i?2:.5f;b.z=i?.5f:2;
        bh_draw_segment(&queue,3,19,&a,&b,1,1,.2f,.8f,0);command=queue.commands+9;
        CHECK(bh_segment_queue_execute(&c,command,&next) && !draw.used && o.emit_calls && c.accepted==2,
            "both near-plane crossing directions use real intersection and ribbon owners");
    }
    setup(&c,&camera,&draw,pool,&clip,&image,&o);a.x=0;a.y=0;a.z=2;
    CHECK(bh_segment_queue_append(&c,&a,1,0,1),"preexisting pool entry belongs to earlier producer");
    a.x=2;b.x=4;a.z=b.z=2;bh_draw_queue_begin(&queue);bh_draw_segment(&queue,3,19,&a,&b,1,1,.2f,.8f,0);
    CHECK(bh_segment_queue_execute(&c,queue.commands+9,&next) && o.used==3 && o.before[1].texture_end==1 &&
        o.before[2].texture_end==2 && !draw.used,"consumer appends to shared current pool rather than resetting it first");
    setup(&c,&camera,&draw,pool,&clip,&image,&o);c.camera=NULL;
    CHECK(!bh_segment_queue_append(&c,&a,1,0,1) && !draw.used && c.error==BH_SEGMENT_QUEUE_ARGUMENT,"missing camera rejected without a guessed transform");
done:
    bh_draw_queue_dispose(&queue);CHECK(bh_math_environment_supported(),"consumer retains caller PC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"segment-queue\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_SEGMENT_QUEUE_STANDALONE
int main(void){unsigned long count;return bh_segment_queue_checkpoint(&count);}
#endif
