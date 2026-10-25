#include "../src/bh_player_input_events.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures,calls,order[8];static BHEntity *stop;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL player-input-events: %s\n",s);}} while(0)
static void set_next(BHEntity *e,BHEntity *n) {memcpy(e->payload+12,&n,4);}
static void set_class(BHEntity *e,long c) {memcpy(e->payload+24,&c,4);}
/* Isolated observable fixture methods; no production callback is invented. */
static void __cdecl event(BHEntity *e,void *raw) {
    unsigned long *p=(unsigned long*)raw;
    if(calls<8)order[calls]=e->generation;
    ++calls;if(p[0]!=15)++failures;p[0]=99;
    if(e==stop)set_next(e,NULL);
}
static void writers(void) {
    BHPlayerInputOwner owner;BHPlayerInputList *list,expected;unsigned long i;
    CHECK(bh_player_input_owner_init(&owner,NULL),"real input allocator owner");
    list=bh_player_input_create(&owner,1);CHECK(list!=NULL,"real selective input list constructor");if(!list)return;
    memset(list->bindings,0xa5,sizeof(list->bindings));list->count=0;
    expected=*list;expected.bindings[255].kind=4;expected.count=256;
    bh_player_input_bind_kind4(list,255);
    CHECK(!memcmp(list,&expected,sizeof(expected)),"high-index kind4 changes only tag and count");
    expected.bindings[0].kind=4;bh_player_input_bind_kind4(list,0);
    CHECK(!memcmp(list,&expected,sizeof(expected)),"low-index kind4 preserves existing high count and all key/name words");
    for(i=0;i<256;++i)bh_player_input_bind_kind4(list,i);
    CHECK(list->count==256,"full kind4 table retains256 count");
    for(i=0;i<256;++i)CHECK(list->bindings[i].kind==4,"all valid binding indices activated");
    bh_player_input_owner_dispose(&owner);
}
static void broadcast(void) {
    BHEntity nodes[3];unsigned long i,visited;
    memset(nodes,0,sizeof(nodes));for(i=0;i<3;++i) {
        nodes[i].generation=i+1;nodes[i].query=event;set_class(nodes+i,i==1?2:1);
        set_next(nodes+i,i<2?nodes+i+1:NULL);
    }
    calls=0;stop=NULL;
    CHECK(bh_player_input_broadcast(nodes,3,&visited) && visited==3 && calls==2 && order[0]==1 && order[1]==3,
        "class1 filter, original list order, and event15 reset after previous callback mutation");
    calls=0;stop=nodes;
    CHECK(bh_player_input_broadcast(nodes,3,&visited) && visited==1 && calls==1,
        "successor loaded after real installed query mutation");stop=NULL;set_next(nodes,nodes+1);
    nodes[2].query=NULL;calls=0;
    CHECK(!bh_player_input_broadcast(nodes,3,&visited) && visited==3 && calls==1,
        "missing real selected callback diagnosed after prior observable callbacks");nodes[2].query=event;
    set_next(nodes+2,nodes);calls=0;
    CHECK(!bh_player_input_broadcast(nodes,4,&visited) && visited==4 && calls==3,"cyclic game list bounded by source contract");
    calls=0;
    CHECK(bh_player_input_broadcast(NULL,0,&visited) && visited==0 && calls==0,"original empty-list no-op");
    CHECK(!bh_player_input_broadcast(nodes,0,&visited) && visited==0 && calls==0,"source zero-budget guard before callback");
}
int bh_player_input_events_checkpoint(unsigned long *count) {
    checks=failures=0;writers();broadcast();if(count)*count=checks;
    printf("{\"scenario\":\"player-input-events\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_PLAYER_INPUT_EVENTS_STANDALONE
int main(void) {return bh_player_input_events_checkpoint(NULL);}
#endif
