/* Complete304B original selector, isolated native oracle; real source pool/query owners. */
#include "../src/bh_target_selection.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char *code;
#include "../build/ai-selection-match/original.h"
static BHEntity *actors[4];
static unsigned long calls,order[4],mutate,cut,seed=0x421610;
static unsigned long next_value(void){seed=seed*1664525UL+1013904223UL;return seed;}
static unsigned long word(BHEntity *e,unsigned long at){unsigned long v;memcpy(&v,(char*)e+at,4);return v;}
static void put(BHEntity *e,unsigned long at,unsigned long v){memcpy((char*)e+at,&v,4);}
static float from_bits(unsigned long v){float f;memcpy(&f,&v,4);return f;}
static float number(void){return (float)(long)(next_value()%200001UL-100000UL)*(1.0f/1024);}
static void __cdecl position(BHEntity *e,const void *data)
{
    BHEntityPositionPacket *p=(BHEntityPositionPacket*)data;unsigned long i;
    if(p->event!=3||calls>=3){puts("Unexpected selector query");exit(3);}
    for(i=1;i<4&&actors[i]!=e;++i){}
    if(i==4){puts("Unknown query actor");exit(3);}order[calls++]=i;
    p->position=e->origin;
    if(mutate)put(e,0x100,word(e,0x100)^0x80000000UL);
    if(cut)put(e,0xf0,0);
}
int main(void)
{
    BHMathEnvironment env;BHEntityPool pool;BHEntity saved[4];BHTargetSelection result;
    BHEntity *player,*head,*excluded,*expected;BHVec3 origin;unsigned long i,j,k,source_calls,source_order[4],source_words[3][2],differences=0,errors=0;
    const long weights[]={-2147483647L-1,-16777217,-3,-1,0,1,3,16777217,2147483647L};
    if(!bh_math_environment_enter(&env)||!bh_entities_init(&pool,4,NULL))return 3;
    for(j=0;j<4;++j){actors[j]=bh_entity_acquire(&pool);if(!actors[j])return 3;}
    for(j=1;j<4;++j)if(!bh_entity_set_query(&pool,actors[j],position))return 3;
    code=(unsigned char*)VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!code)return 3;install_selection_original(code);
    for(i=0;i<8192;++i)
    {
        origin.x=number();origin.y=number();origin.z=number();
        for(j=0;j<4;++j)
        {
            actors[j]->position.x=number();actors[j]->position.y=number();actors[j]->position.z=number();
            actors[j]->origin.x=number();actors[j]->origin.y=number();actors[j]->origin.z=number();
            if(j){put(actors[j],0xfc,(i>>(j+1))&1?2:1);put(actors[j],0x100,(unsigned long)weights[(i+j)%9]);put(actors[j],0xf0,j<3?(unsigned long)actors[j+1]:0);}
        }
        put(actors[0],0x238,(i>>2)&1);put(actors[0],0x234,(unsigned long)weights[i%9]);
        if(i%17==0){origin.x=origin.y=origin.z=0;for(j=0;j<4;++j)actors[j]->position=actors[j]->origin=origin;}
        if(i%31==0){actors[1]->origin.x=from_bits(0x7fc00000UL);put(actors[1],0xfc,2);}
        if(i%43==0){actors[0]->position.x=from_bits(0x7f800000UL);put(actors[0],0x238,1);}
        if(i%47==0){origin.x=from_bits(0x7fc00000UL);}
        player=i&1?actors[0]:NULL;head=i%5?actors[1]:NULL;excluded=i%6<4?actors[i%6]:NULL;mutate=i&8;cut=i&16;
        for(j=0;j<4;++j)saved[j]=*actors[j];
        calls=0;_clearfp();if(!bh_target_select(player,head,excluded,&origin,3,&result))++errors;
        source_calls=calls;memcpy(source_order,order,sizeof(order));
        for(j=1;j<4;++j){source_words[j-1][0]=word(actors[j],0x100);source_words[j-1][1]=word(actors[j],0xf0);}
        for(j=0;j<4;++j)*actors[j]=saved[j];
        memcpy(code+512,&player,4);memcpy(code+516,&head,4);calls=0;_clearfp();
        expected=((BHEntity*(__cdecl*)(const BHEntity*,const BHVec3*))code)(excluded,&origin);
        if(result.target!=expected||calls!=source_calls)++differences;
        else for(k=0;k<calls;++k)if(order[k]!=source_order[k]){++differences;break;}
        for(j=1;j<4;++j)if(word(actors[j],0x100)!=source_words[j-1][0]||word(actors[j],0xf0)!=source_words[j-1][1])++differences;
    }
    _clearfp();if(!bh_entities_valid(&pool)||!bh_entities_dispose(&pool))++errors;
    VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&env);
    printf("{\"selection_original_cases\":8192,\"target_query_order_callback_state_differences\":%lu,\"errors\":%lu,\"original_bytes\":304,\"precision\":\"PC53\",\"strict_all_profile_match\":false}\n",differences,errors);
    return differences||errors;
}
