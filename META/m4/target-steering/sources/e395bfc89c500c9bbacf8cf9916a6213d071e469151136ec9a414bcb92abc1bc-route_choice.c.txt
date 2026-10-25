#include "../src/bh_route_choice.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL route-choice: %s\n",s);}} while(0)
static void word(BHEntity *a,unsigned long offset,unsigned long value) {memcpy(a->payload+offset,&value,4);}
static unsigned long state(const BHEntity *a) {unsigned long v;memcpy(&v,a->payload+0x104,4);return v;}
/* Independent multiply then16-bit rotate formulation of the original pair
   of wrapping multiplies and XOR. No CRT RNG, seed global or callback fake. */
static unsigned long next_state(unsigned long v) {unsigned long product=v*58199UL;return ((product<<16)|(product>>16))^v;}
static void branches(BHEntity *actor) {
    BHWorldGeometry rooms[7];BHGeometryModel model;unsigned long i,kind_index,flags,mask,seed,next,n,rank;
    long kinds[]={-1,0,1,6,7},excluded,expected,actual;BHEntity before;
    memset(rooms,0,sizeof(rooms));memset(&model,0,sizeof(model));rooms[0].model=&model;
    model.portal_count=6;rooms[0].navigation.count=6;
    for(i=0;i<6;++i)rooms[0].navigation.portals[i].neighbor=&rooms[i+1].navigation;
    for(kind_index=0;kind_index<5;++kind_index)for(flags=0;flags<4;++flags)for(mask=0;mask<64;++mask) {
        long kind=kinds[kind_index];unsigned char allowed[6];
        word(actor,24,(unsigned long)kind);
        for(i=0;i<6;++i) {
            rooms[i+1].query_flags=(flags<<4)|0xffcfUL; /* unrelated flags ignored */
            rooms[0].navigation.portals[i].neighbor=(mask&(1UL<<i))?&rooms[i+1].navigation:NULL;
            allowed[i]=(unsigned char)((mask&(1UL<<i)) && !((flags&1) && kind>0) && !((flags&2) && (kind==0 || kind==6)));
        }
        for(excluded=-1;excluded<6;++excluded)for(seed=0;seed<16;++seed) {
            word(actor,0x104,seed*0x1234567UL);before=*actor;next=next_state(seed*0x1234567UL);
            n=0;for(i=0;i<6;++i)if(allowed[i] && (long)i!=excluded)++n;
            expected=excluded;rank=((next&32767UL)*n)/32768UL;
            if(n)for(i=0;i<6;++i)if(allowed[i] && (long)i!=excluded){if(!rank){expected=(long)i;break;}--rank;}
            CHECK(bh_route_choose_exit(actor,rooms,NULL,excluded,&actual) && actual==expected &&
                state(actor)==(n?next:seed*0x1234567UL),"exhaustive flags/class/null/excluded/rank and RNG consumption");
            word(&before,0x104,state(actor));CHECK(!memcmp(&before,actor,sizeof(before)),"only original actor RNG word changes");
        }
        for(i=0;i<6;++i)if(allowed[i]) {
            word(actor,0x104,0xdeadbeefUL);
            CHECK(bh_route_choose_exit(actor,rooms,rooms+i+1,(long)i,&actual) && actual==(long)i && state(actor)==0xdeadbeefUL,
                "preferred eligible neighbor wins before excluded filter without RNG");
        }
    }
    /* Zero/invalid descriptor cases are checked separately from valid topology. */
    word(actor,0x104,123);model.portal_count=0;rooms[0].navigation.count=0;
    CHECK(bh_route_choose_exit(actor,rooms,NULL,-27,&actual) && actual==-27 && state(actor)==123,"no portals preserves arbitrary signed fallback");
    actual=55;model.portal_count=7;rooms[0].navigation.count=7;
    CHECK(!bh_route_choose_exit(actor,rooms,NULL,-1,&actual) && actual==55 && state(actor)==123,"oversized source descriptor rejects before mutation");
    model.portal_count=2;rooms[0].navigation.count=1;
    CHECK(!bh_route_choose_exit(actor,rooms,NULL,-1,&actual) && actual==55,"mismatched resolved portal count rejects");
    CHECK(!bh_route_choose_exit(NULL,rooms,NULL,-1,&actual),"missing actor guard");
    CHECK(!bh_route_choose_exit(actor,NULL,NULL,-1,&actual),"missing room guard");
    CHECK(!bh_route_choose_exit(actor,rooms,NULL,-1,NULL),"missing output guard");
    rooms[0].model=NULL;
    CHECK(!bh_route_choose_exit(actor,rooms,NULL,-1,&actual),"missing real model descriptor guard");
}
static void matched_topology(BHEntity *actor) {
    BHWorldGeometry rooms[2];BHGeometryModel model;BHPortal *a,*b;long selected;
    memset(rooms,0,sizeof(rooms));memset(&model,0,sizeof(model));model.portal_count=1;
    rooms[0].model=rooms[1].model=&model;rooms[0].navigation.count=rooms[1].navigation.count=1;
    rooms[0].navigation.next=&rooms[1].navigation;rooms[1].navigation.next=&rooms[0].navigation;
    a=rooms[0].navigation.portals;b=rooms[1].navigation.portals;a->direction=0;b->direction=4;
    a->aperture[1]=a->overlap[1]=a->scratch[1]=b->aperture[1]=b->overlap[1]=b->scratch[1]=10;
    a->aperture[3]=a->overlap[3]=a->scratch[3]=b->aperture[3]=b->overlap[3]=b->scratch[3]=10;
    CHECK(bh_portals_match(&rooms[0].navigation,2) && bh_portals_match(&rooms[1].navigation,2),"real source portal matcher creates reciprocal topology");
    CHECK(a->neighbor==&rooms[1].navigation && b->neighbor==&rooms[0].navigation,"real matched neighbors retained");
    word(actor,24,2);word(actor,0x104,0xffffffffUL);
    CHECK(bh_route_choose_exit(actor,rooms,rooms+1,0,&selected) && selected==0 && state(actor)==0xffffffffUL,
        "real matched preferred room bypasses excluded backtrack index");
    CHECK(bh_route_choose_exit(actor,rooms,NULL,-1,&selected) && selected==0 && state(actor)==next_state(0xffffffffUL),
        "real matched room selected with one exact actor RNG transition");
    rooms[1].query_flags=0x10;
    CHECK(bh_route_choose_exit(actor,rooms,NULL,0,&selected) && selected==0,
        "ineligible actual neighbor retains original fallback even when fallback is ineligible");
}
int bh_route_choice_checkpoint(unsigned long *count) {
    BHEntityPool pool;BHEntity *actor;BHMathEnvironment env;
    checks=failures=0;CHECK(bh_math_environment_enter(&env),"real FP environment for portal matcher");
    CHECK(bh_entities_init(&pool,1,NULL),"real entity owner");actor=bh_entity_acquire(&pool);
    CHECK(actor!=NULL,"real actor acquired");if(actor){branches(actor);matched_topology(actor);}
    CHECK(bh_entities_dispose(&pool),"real actor owner disposed");bh_math_environment_leave(&env);
    if(count)*count=checks;
    printf("{\"scenario\":\"route-choice\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_ROUTE_CHOICE_STANDALONE
int main(void) {return bh_route_choice_checkpoint(NULL);}
#endif
