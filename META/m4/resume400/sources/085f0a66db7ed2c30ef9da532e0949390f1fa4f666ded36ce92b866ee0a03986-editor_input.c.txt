#include "../src/bh_editor_input.h"
#include "../src/bh_input_evaluate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL editor-input %d: %s\n",__LINE__,s);}}while(0)
typedef struct Memory {int fail;unsigned long live;} Memory;
static void *__cdecl allocate(unsigned long n,void *u){Memory *m=(Memory*)u;void *p;if(m->fail)return NULL;p=malloc(n);if(p){memset(p,0xa5,n);++m->live;}return p;}
static void __cdecl release(void *p,void *u){Memory *m=(Memory*)u;if(p){--m->live;free(p);}}
static void bindings_and_dispatch(void){
    BHPlayerInputOwner owner;BHEntityPool pool;BHPlayerInputState state;
    BHEditorInput c;BHInputSnapshot snapshot;BHPlayerInputList *first,*other;unsigned long i;
    Memory memory={0,0};BHSceneAllocator allocator={allocate,release,&memory};
    static const unsigned short expected[29]={0x4e,0x2e,0x21,0x22,0x25,0x27,0x26,0x28,0,2,1,0x5a,0x58,0x43,0x56,0x53,0x4d,0x71,0x6d,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39};
    memset(&c,0,sizeof(c));memset(&state,0,sizeof(state));
    CHECK(bh_player_input_owner_init(&owner,&allocator)&&bh_entities_init(&pool,1,NULL),"real list/pool initialization");
    c.owner=&owner;c.pool=&pool;c.state=&state;c.name="";
    CHECK(bh_editor_input_setup(&c)&&c.list&&c.dispatcher,"whole setup links real list and dispatcher");
    if(!c.list||!c.dispatcher)return;
    first=c.list;CHECK(first==owner.first&&first->flags==2&&first->count==29,"original mode1/count29 ownership");
    CHECK(c.dispatcher->step==bh_editor_input_tick&&pool.used==1&&bh_entities_valid(&pool),"actual installed callback");
    CHECK(!bh_editor_input_setup(&c)&&pool.used==1&&owner.first==first,"source repeated-setup guard preserves lifetime");
    for(i=0;i<256;++i){BHInputBinding *b=first->bindings+i;
        if(i==8){CHECK(b->kind==4,"slot8 retains actual kind4");CHECK(b->flags==0xa5&&b->modifiers==0xa5a5&&b->key[0]==0xa5a5&&(unsigned char)b->name[0]==0xa5,"kind4 preserves inactive words");}
        else if(i<29){CHECK(b->kind==1&&b->key[0]==expected[i]&&b->flags==0,"all28 exact single-key records");CHECK(b->modifiers==((i==13||i==14)?0x11:0),"only copy/paste require control");CHECK(!b->name[0]&&(unsigned char)b->name[1]==0xa5&&b->key[1]==0xa5a5,"empty original name and untouched tails");}
        else CHECK(b->kind==0,"unused slots remain inactive");
    }
    memset(&snapshot,0,sizeof(snapshot));snapshot.current[0x4e]=1;snapshot.current[0x43]=1;snapshot.pair4[0]=0x1234;snapshot.pair4[1]=0x5678;
    state.values[200]=0xabcdef12;
    CHECK(bh_input_evaluate(first,&snapshot,1)&&bh_entities_step(&pool),"source packet evaluator through real entity dispatcher");
    CHECK(state.values[0]==1&&state.values[13]==0&&state.values[8]==0x56781234&&state.values[200]==0xabcdef12,"packet/state argument roles, modifier gate and kind4 raw payload");
    snapshot.current[0x11]=1;CHECK(bh_input_evaluate(first,&snapshot,1)&&bh_entities_step(&pool)&&state.values[13]==1,"actual modified binding reaches editor state");
    snapshot.current[0x4e]=0;CHECK(bh_input_evaluate(first,&snapshot,1)&&bh_entities_step(&pool)&&state.values[0]==0,"release packet clears editor state");
    other=bh_player_input_create(&owner,1);CHECK(other!=NULL,"second real live input list");
    if(other){bh_player_input_bind_key(other,5,"",0,0,0x41);memset(&snapshot,0,sizeof(snapshot));snapshot.current[0x41]=1;memset(&state,0,sizeof(state));c.list=other;
        CHECK(bh_input_evaluate(other,&snapshot,1)&&bh_entities_step(&pool)&&state.values[5]==1&&!state.values[0]&&!state.values[8],"tick reloads current list rather than captured original list");}
    CHECK(bh_entity_release(&pool,c.dispatcher)&&bh_entities_dispose(&pool),"real dispatcher/pool teardown before context");
    bh_player_input_owner_dispose(&owner);CHECK(memory.live==0,"real list allocator releases both lists");
}
static void failure_cases(void){
    BHPlayerInputOwner owner;BHEntityPool pool;BHPlayerInputState state;BHEditorInput c;BHEntity *held;
    Memory list_memory={0,0},pool_memory={0,0};BHSceneAllocator a={allocate,release,&list_memory},p={allocate,release,&pool_memory};
    memset(&c,0,sizeof(c));memset(&state,0,sizeof(state));
    CHECK(bh_player_input_owner_init(&owner,&a)&&bh_entities_init(&pool,1,&p),"failure-control real owners");
    c.owner=&owner;c.pool=&pool;c.state=&state;list_memory.fail=1;
    CHECK(!bh_editor_input_setup(&c)&&!c.list&&!c.dispatcher&&!pool.used&&!owner.first,"list failure creates no dispatcher or partial owner");
    list_memory.fail=0;held=bh_entity_acquire(&pool);pool_memory.fail=1;
    CHECK(held&&bh_editor_input_setup(&c)&&c.list&&c.list->count==29&&!c.dispatcher&&pool.used==1,"real entity growth failure retains already installed input list");
    CHECK(bh_entity_release(&pool,held)&&bh_entities_dispose(&pool),"failed-dispatcher pool teardown");
    bh_player_input_owner_dispose(&owner);CHECK(!list_memory.live&&!pool_memory.live,"all actual failure allocations released");
    CHECK(!bh_editor_input_setup(NULL),"source NULL guard");
}
int bh_editor_input_checkpoint(unsigned long *count){checks=0;failures=0;bindings_and_dispatch();failure_cases();*count=checks;printf("{\"scenario\":\"editor-input\",\"checks\":%lu,\"failures\":%d}\n",checks,failures);return failures;}
