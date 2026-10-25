#include "../src/bh_entity_resource_draw.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(value,label) do {++checks;if(!(value)){++failures;printf("FAIL %s\n",label);}} while(0)
static unsigned long bits(float value) {unsigned long word;memcpy(&word,&value,4);return word;}
static float stored(const unsigned long *word) {float value;memcpy(&value,word,4);return value;}
static void phase(BHEntity *e,float value) {memcpy(e->payload+0x18,&value,4);}
static void handle(BHEntity *e,unsigned long value) {memcpy(e->payload+0x30,&value,4);}
static BHVec3 angles(const BHEntity *e) {BHVec3 v;memcpy(&v,e->opaque0+0x24,sizeof(v));return v;}
static int close(float a,float b,float tolerance) {return fabs((double)a-b)<tolerance;}

static void full(void) {
    BHDrawQueue queue;
    BHEntityResourceDrawContext ctx;
    BHEntity entity;
    BHDrawCommand *first,*second,*glow;
    BHVec3 a;
    float p=1.25f;
    memset(&entity,0,sizeof(entity));
    CHECK(bh_draw_queue_init(&queue,12,0),"queue initialized");
    ctx.queue=&queue;ctx.glow_image=0x2233;
    entity.position.x=1.0f;entity.position.y=-2.0f;entity.position.z=3.0f;
    phase(&entity,p);handle(&entity,0x1122);
    CHECK(bh_entity_resource_draw_special(&ctx,&entity)==1,"all three records queued");
    CHECK(queue.count==12,"three records appended");
    first=&queue.commands[9];second=&queue.commands[10];glow=&queue.commands[11];
    CHECK(queue.commands[4].next==9 && first->next==10 && second->next==11,
          "bucket three preserves submission order");
    CHECK(first->kind==BH_DRAW_MODEL_RESOURCE && second->kind==BH_DRAW_MODEL_RESOURCE &&
          glow->kind==BH_DRAW_BILLBOARD,"actual producers selected");
    CHECK(first->resource==0x1122 && second->resource==0x1122 && glow->resource==0x2233,
          "resource and glow bindings retained");
    CHECK(close(stored(first->words),0.95f,0.000001f) &&
          close(stored(second->words),1.05f,0.000001f),"symmetric stored x offsets");
    CHECK(stored(first->words+1)==-2.0f && stored(second->words+2)==3.0f,
          "model positions preserve y and z");
    CHECK(stored(first->words+3)==2.5f &&
          close(stored(first->words+4),0.265640378f,0.000002f) &&
          stored(first->words+5)==0.0f,"first phase pose");
    CHECK(close(stored(second->words+3),3.425f,0.000002f) &&
          close(stored(second->words+4),0.299018294f,0.000002f) &&
          stored(second->words+5)==0.0f,"second phase pose");
    CHECK(bits(stored(first->words+6))==0x3eb33333UL &&
          bits(stored(second->words+6))==0x3eb33333UL,"literal model modulation");
    CHECK(glow->words[0]==bits(1.0f) && glow->words[1]==bits(-2.0f) &&
          glow->words[2]==bits(3.0f),"billboard uses original position");
    CHECK(close(stored(glow->words+3),6.178355217f,0.000002f) &&
          glow->words[3]==glow->words[4] &&
          close(stored(glow->words+6),0.544588804f,0.000002f),
          "glow size and alpha from retained sine");
    a=angles(&entity);
    CHECK(a.x==stored(second->words+3) && a.y==stored(second->words+4) && a.z==0.0f,
          "second pose remains in entity angle fields");
    CHECK(entity.position.x==1.0f && entity.position.y==-2.0f && entity.position.z==3.0f,
          "entity position unchanged");
    CHECK(bh_draw_queue_dispose(&queue),"queue disposed");
}

static void saturation(void) {
    BHDrawQueue queue;
    BHEntityResourceDrawContext ctx;
    BHEntity entity;
    BHVec3 a;
    memset(&entity,0,sizeof(entity));phase(&entity,1.25f);handle(&entity,42);
    ctx.queue=&queue;ctx.glow_image=77;
    CHECK(bh_draw_queue_init(&queue,10,0),"one-slot queue initialized");
    CHECK(bh_entity_resource_draw_special(&ctx,&entity)==0,"partial enqueue reported");
    CHECK(queue.count==10 && queue.commands[9].kind==BH_DRAW_MODEL_RESOURCE &&
          queue.commands[9].resource==42 && queue.error==BH_DRAW_FULL,
          "first record retained after saturation");
    a=angles(&entity);CHECK(close(a.x,3.425f,0.000002f) &&
                            close(a.y,0.299018294f,0.000002f) && a.z==0.0f,
                            "angle mutation completes after saturation");
    CHECK(bh_draw_queue_dispose(&queue),"one-slot queue disposed");
    CHECK(bh_draw_queue_init(&queue,9,0),"zero-slot queue initialized");
    CHECK(bh_entity_resource_draw_special(&ctx,&entity)==0 && queue.count==9 &&
          queue.error==BH_DRAW_FULL,"all appends attempted on full queue");
    CHECK(bh_draw_queue_dispose(&queue),"zero-slot queue disposed");
}

static void precision_and_guards(void) {
    BHDrawQueue queue;
    BHEntityResourceDrawContext ctx;
    BHEntity entity;
    float p=7.710294723510742f;
    double s;
    volatile float premature;
    memset(&entity,0,sizeof(entity));phase(&entity,p);handle(&entity,0x4567);
    ctx.queue=&queue;ctx.glow_image=0x789a;
    CHECK(bh_draw_queue_init(&queue,12,0),"precision queue initialized");
    CHECK(bh_entity_resource_draw_special(&ctx,&entity)==1,"precision phase queued");
    s=sin((double)p*5.17);
    premature=(float)s;
    CHECK(bits((float)(s+6.0))!=bits((float)(premature+6.0)),
          "fixture distinguishes premature sine rounding");
    CHECK(bits(stored(queue.commands[11].words+3))==bits((float)(s+6.0)),
          "size retains sine precision until binary32 store");
    CHECK(bits(stored(queue.commands[11].words+6))==bits((float)(0.5+s*0.25)),
          "alpha retains sine precision independently");
    CHECK(bh_entity_resource_draw_special(0,&entity)==-1 &&
          bh_entity_resource_draw_special(&ctx,0)==-1 && queue.count==12,
          "invalid source bindings do not append");
    CHECK(bh_draw_queue_dispose(&queue),"precision queue disposed");
}

int bh_entity_resource_draw_checkpoint(unsigned long *out_checks) {
    full();saturation();precision_and_guards();
    if(out_checks) *out_checks=checks;
    printf("{\"scenario\":\"entity-resource-draw\",\"checks\":%lu,\"failures\":%lu}\n",
           checks,failures);
    return failures ? 1:0;
}
#ifdef BH_ENTITY_RESOURCE_DRAW_STANDALONE
int main(void) {return bh_entity_resource_draw_checkpoint(0);}
#endif
