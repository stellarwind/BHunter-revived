#include "../src/bh_contact_effects.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL contact effect %d: %s\n",__LINE__,label);}} while(0)
static unsigned long bits(float value) {unsigned long word;memcpy(&word,&value,4);return word;}
static float value(unsigned long word) {float result;memcpy(&result,&word,4);return result;}
typedef struct Memory {unsigned long live;int fail_next;} Memory;
static void *__cdecl allocate(unsigned long size,void *user) {Memory *m=(Memory *)user;void *p;if(m->fail_next){m->fail_next=0;return NULL;}p=malloc(size);if(p)++m->live;return p;}
static void __cdecl release(void *pointer,void *user) {Memory *m=(Memory *)user;--m->live;free(pointer);}
static void cases(void) {
    BHEntityPool pool;BHDrawQueue queue,small;BHContactEffects ctx;
    BHEntity *entity,*foreign;BHSparkState *spark;BHImpactState *impact;
    BHVec3 point={10,20,30},normal={1,0,0},before;
    BHDrawCommand *command;
    Memory memory={0,0};BHSceneAllocator allocator;
    const unsigned long initial[8]={0x3f800000UL,0x404f0800UL,0x3e2e1000UL,0x3ee68b33UL,0x3f7ef59aUL,0xc0ae5349UL,0x3f31bfd4UL,0xc0566666UL};
    const unsigned long tick[10]={0x3f7bbbbcUL,0x404f64d5UL,0x3ef79c44UL,0x3f83bf11UL,0x411d1836UL,0x41a02f66UL,0x41ef1b4fUL,0xc0a75a32UL,0x3f5489bcUL,0xc04dd2f1UL};
    allocator.allocate=allocate;allocator.release=release;allocator.user=&memory;
    CHECK(bh_entities_init(&pool,1,&allocator) && bh_draw_queue_init(&queue,32,&allocator),"real source pool/queue owners");
    CHECK(bh_contact_effects_init(&ctx,&pool,&queue,101,102,103,104),"explicit resource and callback context");
    srand(1);entity=bh_contact_spark_create(&ctx,&point,1,2);spark=bh_contact_spark_state(&ctx,entity);
    CHECK(entity && spark && pool.used==1 && entity->position.x==10 &&
        entity->step && entity->refresh && spark->frame==4,"real pool allocation installs source spark callbacks");
    CHECK(bits(spark->lifetime)==initial[0] && bits(spark->angle)==initial[1] && bits(spark->spin)==initial[2] &&
        bits(spark->width)==initial[3] && bits(spark->height)==initial[4] &&
        bits(spark->velocity.x)==initial[5] && bits(spark->velocity.y)==initial[6] && bits(spark->velocity.z)==initial[7],
        "independent binary32 fixtures from verified CRT seed and original constants");
    CHECK(rand()==26962,"spark creator consumes exactly eight native shared CRT draws");
    CHECK(!bh_contact_effects_dispose(&ctx),"context lifetime cannot end with active callbacks");ctx.error=BH_EFFECT_OK;
    CHECK(bh_entities_refresh(&pool) && queue.count==10,"real source refresh dispatch submits command");
    command=queue.commands+9;
    CHECK(command->kind==BH_DRAW_BILLBOARD && command->resource==101 &&
        command->words[3]==initial[3] && command->words[4]==initial[4] && command->words[5]==initial[1] &&
        command->words[6]==initial[0] && command->words[7]==4,"spark callback submits actual lifetime/angle/frame/size fields");
    memory.fail_next=1;srand(1);
    CHECK(!bh_contact_spark_create(&ctx,&point,1,2) && pool.used==1 && ctx.error==BH_EFFECT_POOL && rand()==41,
        "failed real pool growth consumes no random state or partial entity");ctx.error=BH_EFFECT_OK;
    CHECK(bh_entities_step(&pool),"real source step dispatch advances particle");
    CHECK(bits(spark->lifetime)==tick[0] && bits(spark->angle)==tick[1] && bits(spark->width)==tick[2] &&
        bits(spark->height)==tick[3] && bits(entity->position.x)==tick[4] &&
        bits(entity->position.y)==tick[5] && bits(entity->position.z)==tick[6] &&
        bits(spark->velocity.x)==tick[7] && bits(spark->velocity.y)==tick[8] && bits(spark->velocity.z)==tick[9],
        "original fixed tick, drag/gravity and growth binary32 stores");
    spark->lifetime=1.0f/120;before=entity->position;
    CHECK(bh_entities_step(&pool) && !pool.used && !bh_contact_spark_state(&ctx,entity) &&
        !memcmp(&entity->position,&before,sizeof(before)),"expiry releases before motion through real pool owner");
    foreign=bh_entity_acquire(&pool);
    CHECK(foreign && !bh_contact_spark_state(&ctx,foreign),"recycled foreign callback state is not mistaken for an effect");
    CHECK(bh_entity_release(&pool,foreign),"release isolated foreign slot");
    entity=bh_contact_impact_create(&ctx,&point,&normal,6);impact=bh_contact_impact_state(&ctx,entity);
    CHECK(entity && impact && impact->size==16 && impact->duration==1 && impact->age==0,
        "physics special-surface flags 2/4 preserve bit-2 priority and constructor fields");
    impact->age=.25f;bh_draw_queue_begin(&queue);srand(1);
    CHECK(bh_entities_refresh(&pool) && queue.count==10 && rand()==18467,"quad refresh consumes one random draw");
    command=queue.commands+9;
    CHECK(command->kind==BH_DRAW_ORIENTED && command->resource==102 &&
        command->words[9]==bits(4) && command->words[10]==bits(4) &&
        command->words[11]==bits(41.0f/4096) && command->words[12]==bits(.75f) &&
        command->words[3]==bits(0) && command->words[5]==bits(1),"impact quad size, opacity, random angle and fixed orientation");
    while(queue.count<queue.capacity) bh_draw_billboard(&queue,0,1,&point,1,1,0,0,1);
    srand(1);CHECK(bh_entities_refresh(&pool) && queue.count==queue.capacity && rand()==18467 &&
        ctx.error==BH_EFFECT_OK,"full command queue still consumes original refresh random draw and drops silently");
    CHECK(bh_entity_release(&pool,entity),"complete quad lifetime cleanup");
    entity=bh_contact_impact_create(&ctx,&point,&normal,4);impact=bh_contact_impact_state(&ctx,entity);
    CHECK(impact && impact->size==12,"second contact family source allocation");
    impact->age=.25f;ctx.has_camera=1;ctx.camera_yaw=1.5707963267948966f;
    bh_draw_queue_begin(&queue);srand(1);
    CHECK(bh_entities_refresh(&pool) && queue.count==12 && rand()==41,"rays refresh emits three commands without random draws");
    command=queue.commands+9;
    CHECK(command->resource==103 && command->words[9]==bits(3) && fabs(value(command->words[12])-.525)<1e-7 &&
        fabs(value(command->words[3])-1)<1e-6 && fabs(value(command->words[5]))<1e-6,
        "explicit camera yaw supplies original oriented-quad basis");
    command=queue.commands+10;
    CHECK(command->kind==BH_DRAW_SEGMENT && command->resource==104 &&
        command->words[0]==bits(10) && command->words[3]==bits(14.5f) && command->words[6]==bits(2.25f),
        "first ray segment uses full envelope extent and original width");
    command=queue.commands+11;
    CHECK(command->words[0]==bits(12.25f) && command->words[3]==bits(10),"second ray reverses endpoints at half envelope extent");
    CHECK(bh_draw_queue_init(&small,11,&allocator),"bounded two-command frame");ctx.queue=&small;
    CHECK(bh_entities_refresh(&pool) && small.count==11 && small.error==BH_DRAW_FULL &&
        small.commands[9].kind==BH_DRAW_ORIENTED && small.commands[10].kind==BH_DRAW_SEGMENT,
        "limited frame retains first two ray commands, drops only the third");
    ctx.queue=&queue;CHECK(bh_draw_queue_dispose(&small),"release separate bounded frame owner");
    /* The first envelope remains PC53; the second reloads its stored float.
       Independent original-operation fixture deliberately distinguishes them. */
    impact->age=.01f;impact->duration=.7f;impact->size=13;impact->normal.x=1.25f;
    entity->position.x=0;ctx.has_camera=0;bh_draw_queue_begin(&queue);
    CHECK(bh_entities_refresh(&pool) && queue.commands[10].words[3]==0x3eea5180UL &&
        queue.commands[11].words[0]==0x3e6a5181UL && queue.commands[10].words[6]==0x404d0750UL,
        "original first-envelope PC53 carry and second-envelope float reload remain distinct");
    impact->age=0;impact->duration=1.0f/30;
    CHECK(bh_entities_step(&pool) && !pool.used && !bh_contact_impact_state(&ctx,entity),"age uses fixed +1/30 and >= expiration boundary");
    CHECK(bh_entities_valid(&pool) && bh_contact_effects_dispose(&ctx) && bh_draw_queue_dispose(&queue) &&
        bh_entities_dispose(&pool) && memory.live==0,"all actual callbacks, arenas and pool allocations cleaned up");
}
int bh_contact_effects_checkpoint(unsigned long *count) {
    checks=0;failed=0;cases();*count=checks;
    printf("{\"scenario\":\"contact-effects\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
