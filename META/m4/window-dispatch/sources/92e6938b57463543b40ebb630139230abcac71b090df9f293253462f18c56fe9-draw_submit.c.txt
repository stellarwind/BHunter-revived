#include "../src/bh_draw_submit.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks;
static int failures;
#define CHECK(x,s) check((x)!=0,s,__LINE__)
static void check(int ok,const char *name,int line) {
    ++checks;
    if(!ok) {++failures;if(failures<20)printf("FAIL draw submit %d: %s\n",line,name);}
}

typedef struct Fixture {
    BHDrawQueue queue;
    BHDrawSubmitContext submit;
    BHDrawResourceContext draw;
    BHDrawResource resource;
    BHMainCamera camera;
    BHViewConfig config;
    BHSpriteBillboardContext billboard;
    BHScreenClipContext clip;
    BHTextureImage image;
    unsigned long lookup_handle,lookup_calls,emit_calls,color;
    unsigned long image_lookup_handle,image_lookup_calls;
    int reject;
    BHDrawCommand *mutate_command;
    unsigned long replacement_next;
} Fixture;

static const BHDrawResource *__cdecl lookup(unsigned long handle,void *user) {
    Fixture *f=(Fixture *)user;
    ++f->lookup_calls;
    f->lookup_handle=handle;
    return handle==37 ? &f->resource : 0;
}
static BHTextureImage *__cdecl image_lookup(unsigned long handle,void *user) {
    Fixture *f=(Fixture *)user;
    ++f->image_lookup_calls;
    f->image_lookup_handle=handle;
    return handle==71 ? &f->image : 0;
}
static int __cdecl emit(BHClipVertex *ring,BHTextureImage *image,
    unsigned long color,void *user) {
    Fixture *f=(Fixture *)user;
    CHECK(ring && ring->next && image==&f->image,"real image and primitive ring");
    ++f->emit_calls;f->color=color;
    if(f->mutate_command)f->mutate_command->next=f->replacement_next;
    return !f->reject;
}
static void setup(Fixture *f,unsigned long capacity) {
    memset(f,0,sizeof(*f));
    CHECK(bh_draw_queue_init(&f->queue,capacity,0),"queue initialized");
    f->camera.projected.m[0][0]=1;
    f->camera.projected.m[1][1]=1;
    f->camera.projected.m[2][2]=1;
    f->camera.planes[0].normal.z=1;
    f->camera.focal=100;
    f->config.near_z=1;f->config.far_z=100;
    f->config.left=0;f->config.right=200;
    f->config.top=0;f->config.bottom=200;
    bh_screen_clip_init(&f->clip,1,100,0,200,0,200);
    f->billboard.camera=&f->camera;f->billboard.config=&f->config;
    f->billboard.clip=&f->clip;f->billboard.emit=emit;
    f->billboard.emit_user=f;f->billboard.base_color=0xff000000UL;
    f->draw.camera=&f->camera;f->draw.config=&f->config;
    f->draw.billboard=&f->billboard;
    f->resource.tag=BH_RESOURCE_TAG_BILLBOARD;
    f->resource.data.billboard.image=&f->image;
    f->resource.data.billboard.width=4;
    f->resource.data.billboard.height=2;
    f->submit.draw=&f->draw;f->submit.lookup=lookup;
    f->submit.lookup_user=f;
}
static void queued_resource(void) {
    Fixture f;BHVec3 position,angles;BHDrawCommand *command;
    unsigned long next;float payload[7];
    setup(&f,11);
    position.x=1000;position.y=1000;position.z=20;
    angles.x=1.25f;angles.y=-2.5f;angles.z=3.75f;
    CHECK(bh_draw_submit_resource(&f.queue,0,37,&position,&angles,.5f)==1,
        "resource producer appends one command");
    CHECK(f.queue.count==10 && f.queue.commands[1].next==9 &&
        f.queue.commands[9].next==2,"bucket insertion retains chain");
    command=&f.queue.commands[9];
    CHECK(command->kind==BH_DRAW_MODEL_RESOURCE && command->resource==37,
        "semantic callback kind and borrowed handle");
    memcpy(payload,command->words,sizeof(payload));
    CHECK(!memcmp(payload,&position,3*sizeof(float)) &&
        !memcmp(payload+3,&angles,3*sizeof(float)) && payload[6]==.5f,
        "producer copies all seven binary32 values");
    next=BH_DRAW_END;
    CHECK(bh_draw_submit_execute(&f.submit,command,&next),
        "callback resolves descriptor and invokes real resource renderer");
    CHECK(next==2 && f.lookup_calls==1 && f.lookup_handle==37 &&
        f.emit_calls==1 && f.draw.dispatched==1 && f.color==0xff7f7f7fUL,
        "callback returns following link and carries position and modulation");
    f.resource.data.billboard.image=0;
    next=BH_DRAW_END;
    CHECK(!bh_draw_submit_execute(&f.submit,command,&next) && next==2 &&
        f.draw.error==BH_DRAW_RESOURCE_ARGUMENT && f.emit_calls==1,
        "resource draw failure still publishes the original next link");
    f.resource.data.billboard.image=&f.image;
    CHECK(bh_draw_submit_resource(&f.queue,8,99,&position,&angles,1)==1,
        "second resource command fills queue");
    next=BH_DRAW_END;
    CHECK(!bh_draw_submit_execute(&f.submit,&f.queue.commands[10],&next) &&
        next==BH_DRAW_END && f.lookup_calls==3 && f.emit_calls==1,
        "missing explicit descriptor binding leaves draw untouched");
    CHECK(bh_draw_submit_resource(&f.queue,1,37,&position,&angles,1)==0 &&
        f.queue.error==BH_DRAW_FULL && f.queue.count==11,
        "full queue drops command without overflow");
    f.mutate_command=command;f.replacement_next=10;
    next=BH_DRAW_END;
    CHECK(bh_draw_submit_execute(&f.submit,command,&next) &&
        next==10 && command->next==10 && f.emit_calls==2,
        "callback observes next link changed by the synchronous draw");
    CHECK(bh_draw_queue_dispose(&f.queue),"queue disposed");
}
static void arguments(void) {
    Fixture f;BHVec3 p={0,0,10};unsigned long next=17;
    setup(&f,10);
    CHECK(bh_draw_submit_resource(&f.queue,0,37,0,&p,1)==-1 &&
        f.queue.error==BH_DRAW_ARGUMENT && f.queue.count==9,
        "missing position rejects before append");
    CHECK(!bh_draw_submit_execute(&f.submit,0,&next) && next==17,
        "missing command leaves next output intact");
    CHECK(bh_draw_queue_dispose(&f.queue),"queue disposed after reject");
}
static void billboard(void) {
    Fixture f;BHVec3 position;BHDrawCommand *command;
    BHDrawBillboardSubmitContext submit;unsigned long next;
    setup(&f,10);
    position.x=1000;position.y=1000;position.z=20;
    memset(&submit,0,sizeof(submit));
    submit.draw=&f.billboard;submit.lookup=image_lookup;
    submit.lookup_user=&f;
    CHECK(bh_draw_billboard(&f.queue,0,71,&position,4,2,0,0x101,.5f)==1,
        "real 00478280 producer appends billboard payload");
    command=&f.queue.commands[9];
    CHECK(command->kind==BH_DRAW_BILLBOARD && command->resource==71 &&
        command->words[7]==0x101 && command->next==2,
        "image handle, flags and next occupy reviewed command fields");
    next=BH_DRAW_END;
    CHECK(bh_draw_submit_billboard_execute(&submit,command,&next) &&
        next==2 && f.image_lookup_calls==1 && f.image_lookup_handle==71 &&
        f.emit_calls==1 && f.color==0xff7f7f7fUL,
        "billboard callback binds real image and carries modulation");
    command->resource=99;next=BH_DRAW_END;
    CHECK(!bh_draw_submit_billboard_execute(&submit,command,&next) &&
        next==2 && f.image_lookup_calls==2 && f.emit_calls==1,
        "missing image binding still publishes current link");
    command->resource=71;f.reject=1;next=BH_DRAW_END;
    CHECK(!bh_draw_submit_billboard_execute(&submit,command,&next) &&
        next==2 && f.billboard.error==BH_SPRITE_BILLBOARD_EMIT &&
        f.emit_calls==2,"real sprite failure retains callback link");
    f.reject=0;f.mutate_command=command;f.replacement_next=7;
    next=BH_DRAW_END;
    CHECK(bh_draw_submit_billboard_execute(&submit,command,&next) &&
        next==7 && command->next==7 && f.emit_calls==3,
        "billboard callback reads next after synchronous sprite draw");
    CHECK(bh_draw_queue_dispose(&f.queue),"billboard queue disposed");
}
int bh_draw_submit_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;
    checks=0;failures=0;
    if(!bh_math_environment_enter(&fp))return 2;
    queued_resource();arguments();billboard();
    CHECK(bh_math_environment_supported(),"PC53 environment preserved");
    bh_math_environment_leave(&fp);
    *count=checks;
    printf("{\"scenario\":\"draw-submit\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_DRAW_SUBMIT_STANDALONE
int main(void) {unsigned long count;return bh_draw_submit_checkpoint(&count);}
#endif
