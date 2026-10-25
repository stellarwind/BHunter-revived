#include "../src/bh_target_selection.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <float.h>

static unsigned long checks,failures,calls;
static BHEntity *cut,*change_weight;
static long replacement_weight;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL target-selection: %s\n",s);}} while(0)
static void word(BHEntity *e,unsigned long offset,long value) {memcpy(e->payload+offset,&value,4);}
static void link(BHEntity *e,BHEntity *next) {memcpy(e->payload+12,&next,4);}
static unsigned long bits(float value) {unsigned long b;memcpy(&b,&value,4);return b;}
static int nan_score(float value) {unsigned long b=bits(value);return (b&0x7f800000)==0x7f800000 && (b&0x007fffff)!=0;}
static float from_bits(unsigned long b) {float value;memcpy(&value,&b,4);return value;}
/* Isolated test method, never installed by production source. The deliberate
   difference between origin and position catches bypass of event-3 dispatch. */
static void __cdecl position(BHEntity *e,void *raw) {
    BHEntityPositionPacket *p=(BHEntityPositionPacket *)raw;
    CHECK(p->event==3,"event is reset before every callback");
    ++calls;p->position=e->origin;p->event=99;
    ((unsigned long *)raw)[9]=0x12345678; /* original 40-byte packet capacity */
    if(e==cut)link(e,NULL);
    if(e==change_weight)word(e,28,replacement_weight);
}
static void actor(BHEntity *e,long kind,long weight,float x,float y,float z) {
    word(e,24,kind);word(e,28,weight);link(e,NULL);
    e->origin.x=x;e->origin.y=y;e->origin.z=z;
    e->position.x=99;e->position.y=99;e->position.z=99;
}
typedef struct NumericCase {unsigned long point[3],origin[3],score[2];} NumericCase;
static const NumericCase numeric_cases[]={
#include "target_selection_cases.h"
};
static void numerical(BHEntity *player,BHEntity *a) {
    unsigned long i;BHVec3 origin;BHTargetSelection r;
    word(player,0x154,1);word(player,0x150,1);
    for(i=0;i<sizeof(numeric_cases)/sizeof(numeric_cases[0]);++i) {
        const NumericCase *n=numeric_cases+i;
        origin.x=from_bits(n->origin[0]);origin.y=from_bits(n->origin[1]);origin.z=from_bits(n->origin[2]);
        player->position.x=from_bits(n->point[0]);player->position.y=from_bits(n->point[1]);player->position.z=from_bits(n->point[2]);
        CHECK(bh_target_select(player,NULL,NULL,&origin,0,&r) && r.target==player && bits(r.score)==n->score[0],
            "player mixed retained x/z products match independent PC53 fixture");
        actor(a,2,1,player->position.x,player->position.y,player->position.z);
        CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && r.target==a && bits(r.score)==n->score[1],
            "actor mixed retained x/y products match independent PC53 fixture");
        if(bits(r.score)!=n->score[1])printf("numeric %lu actual %08lx expected %08lx\n",i,bits(r.score),n->score[1]);
    }
}
static void scenarios(BHEntity *p,BHEntity *a,BHEntity *b) {
    BHVec3 origin={0,0,0};BHTargetSelection r;
    calls=0;cut=change_weight=NULL;
    CHECK(bh_target_select(NULL,NULL,NULL,&origin,0,&r) && !r.target && !r.visited && bits(r.score)==0x7e876abd,
        "empty list returns original finite sentinel");
    CHECK(!bh_target_select(NULL,NULL,NULL,NULL,0,&r),"null origin guard");
    CHECK(!bh_target_select(NULL,NULL,NULL,&origin,0,NULL),"null result guard");
    word(p,0x154,0);word(p,0x150,1);p->position.x=3;p->position.y=4;p->position.z=0;
    CHECK(bh_target_select(p,NULL,NULL,&origin,0,&r) && !r.target,"ineligible global player ignored");
    word(p,0x154,-1);
    CHECK(bh_target_select(p,NULL,p,&origin,0,&r) && r.target==p && r.score==5,
        "eligible player selected even when excluded, uses direct position");
    actor(a,2,1,0,0,5);actor(b,2,1,3,4,0);link(a,b);
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==a && r.score==5 && calls==2,
        "exact tie retains first member and queries positions");
    CHECK(bh_target_select(p,a,NULL,&origin,2,&r) && r.target==p,"exact tie retains initial player");
    CHECK(bh_target_select(NULL,a,a,&origin,2,&r) && r.target==b && r.visited==2,"excluded list actor skipped");
    word(a,24,1);word(b,24,3);calls=0;
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && !r.target && calls==0,"only class two queried");
    actor(a,2,2,0,0,6);actor(b,2,1,0,0,4);link(a,b);
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==a && r.score==3,"distance divided by signed weight");
    word(b,28,-1);
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==b && r.score==-4,"negative divisor preserved");
    word(a,28,-1);a->origin.z=6;word(b,28,-1);
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==a && r.score==-6,"negative scores favor larger distance");
    word(p,0x150,-2);
    CHECK(bh_target_select(p,NULL,NULL,&origin,0,&r) && r.target==p && r.score==-2.5f,"player weight is signed");
    word(p,0x150,0);
    CHECK(bh_target_select(p,NULL,NULL,&origin,0,&r) && r.target==p && bits(r.score)==0x7f800000,
        "player zero divisor keeps unconditional positive infinity incumbent");
    actor(a,2,1,0,0,5);
    CHECK(bh_target_select(p,a,NULL,&origin,1,&r) && r.target==a && r.score==5,"finite actor replaces infinite player");
    word(a,28,0);
    CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && !r.target,"infinite actor does not beat finite sentinel");
    a->origin.z=0;
    CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && r.target==a && nan_score(r.score),
        "zero over zero unordered score selects through original C0");
    actor(b,2,1,0,0,5);link(a,b);
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==b && r.score==5,"finite actor replaces NaN incumbent");
    p->position.x=p->position.y=0;
    CHECK(bh_target_select(p,b,NULL,&origin,1,&r) && r.target==b,"NaN player score is replaced");
    actor(a,2,1,1,1.0f/1024,0);actor(b,2,1,1,1.0f/1024,0);link(a,b);
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==b && bits(r.score)==0x3f800004,
        "compare retained score BEFORE float store; identical positions may replace rounded-up incumbent");
    actor(a,2,1,0,0,1);actor(b,2,1,0,0,0);link(a,b);cut=a;calls=0;
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==a && r.visited==1 && calls==1,
        "list successor read after callback mutation");cut=NULL;link(a,b);
    change_weight=a;replacement_weight=-2;
    CHECK(bh_target_select(NULL,a,NULL,&origin,2,&r) && r.target==a && r.score==-0.5f,
        "weight read after callback mutation");change_weight=NULL;
    a->query=NULL;calls=0;
    CHECK(!bh_target_select(NULL,a,NULL,&origin,2,&r) && r.visited==1 && calls==0,"missing selected callback explicit error");
    a->query=position;link(b,a);calls=0;
    CHECK(!bh_target_select(NULL,a,NULL,&origin,3,&r) && r.visited==3 && calls==3,"cyclic list stops at explicit budget");
    calls=0;
    CHECK(!bh_target_select(NULL,a,NULL,&origin,0,&r) && !r.visited && !calls,"zero budget stops before dispatch");
    link(a,NULL);link(b,NULL);
    actor(a,2,1,from_bits(0x7fc00000),0,0);
    CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && r.target==a && nan_score(r.score),"quiet NaN position selects unordered");
    actor(a,2,-1,0,0,0);
    CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && r.target==a && bits(r.score)==0x80000000,
        "negative divisor preserves negative zero score");
    actor(a,2,(-2147483647L-1),0,0,1);
    CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && r.target==a && bits(r.score)==0xb0000000,
        "minimum signed weight converted without unsigned reinterpretation");
    word(a,28,2147483647L);
    CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && r.target==a && bits(r.score)==0x30000000,
        "maximum signed weight retains division precision");
    actor(a,2,16777217L,16777216.0f,0,0);
    CHECK(bh_target_select(NULL,a,NULL,&origin,1,&r) && r.target==a && bits(r.score)==0x3f7fffff,
        "FILD divisor remains exact beyond float integer precision");
    p->position=a->origin;word(p,0x150,16777217L);
    CHECK(bh_target_select(p,NULL,NULL,&origin,0,&r) && r.target==p && bits(r.score)==0x3f7fffff,
        "player FILD divisor remains exact beyond float integer precision");
    _clearfp();
}
int bh_target_selection_checkpoint(unsigned long *count) {
    BHMathEnvironment env;BHEntityPool pool;BHEntity *p,*a,*b;
    checks=failures=0;
    CHECK(bh_math_environment_enter(&env),"establish real numerical environment");
    CHECK(bh_entities_init(&pool,3,NULL),"real entity allocation owner");
    p=bh_entity_acquire(&pool);a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);
    CHECK(p && a && b,"three real acquired actor slots");
    if(p && a && b) {
        CHECK(bh_entity_set_query(&pool,a,position) && bh_entity_set_query(&pool,b,position),"real query callback registration");
        scenarios(p,a,b);
        numerical(p,a);
        CHECK(bh_entities_valid(&pool),"pool links remain valid after selection");
    }
    CHECK(bh_entities_dispose(&pool),"real entity owner disposal");
    bh_math_environment_leave(&env);if(count)*count=checks;
    printf("{\"scenario\":\"target-selection\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_TARGET_SELECTION_STANDALONE
int main(void) {return bh_target_selection_checkpoint(NULL);}
#endif
