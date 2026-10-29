/* Probe original0044c200's plane decision through its real prologue/math.
   Only the two recursive child calls are rebound to a counting leaf. */
#define BH_COLLISION_ORIGINAL_HEADER "../build/collision-plane-original/original-full.h"
#define main unused_full_collision_main
#include "collision_full_original.c"
#undef main
static unsigned long child_calls;
static long child_ids[2];
static int __cdecl child_probe(int an,int bn)
{
    if(child_calls<2)child_ids[child_calls]=bn;
    ++child_calls;return 0;
}
int main(void)
{
    unsigned char *code=(unsigned char *)VirtualAlloc(0,8192,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    Mesh a,b;
    BHCollisionQuery q;
    BHCollisionHit hit;
    BHMat34 relative,world;
    unsigned char old_hit[48];
    unsigned int saved=_controlfp(0,0);
    unsigned long i,probe,cases=0,differences=0;
    if(!code)return 2;
    _controlfp(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
    install_original_collision(code);
    for(i=0;i<2;++i)
    {
        unsigned long at=512+(i?0x87d:0x860);
        long displacement=(long)child_probe-(long)(code+at+5);
        if(code[at]!=0xe8)return 3;
        memcpy(code+at+1,&displacement,4);
    }
    FlushInstructionCache(GetCurrentProcess(),code,8192);
    for(probe=0;probe<4;++probe)
    {
        if(!prepare(&a,0,0,0)||!prepare(&b,1,0,0))return 4;
        a.nodes[0].normal[0]=a.nodes[0].normal[1]=a.nodes[0].normal[2]=0.57735026f;
        a.nodes[0].front_leaf=a.nodes[0].back_leaf=-1;
        a.cache.radii[0]=a.bounds[0].radius=36028797018963968.0f;
        for(i=0;i<3;++i)
        {
            b.cache.centers[i].x=b.bounds[i].center.x=18014398509481984.0f;
            b.cache.centers[i].y=b.bounds[i].center.y=-18014398509481984.0f;
            b.cache.centers[i].z=b.bounds[i].center.z=(float)probe;
            b.cache.radii[i]=b.bounds[i].radius=.25f;
            if(i)b.nodes[i].front_node=b.nodes[i].back_node=-1;
        }
        identity(&relative);identity(&world);memset(original_state,0,sizeof(original_state));memset(old_hit,0xa5,sizeof(old_hit));
        bind_state(8,&a.view);bind_state(12,&b.view);bind_state(0x30,&world);bind_state(0x88,old_hit);
        memcpy(original_state+0x4c,&relative,48);
        if(!bh_collision_query_init(&q,&a.geometry,&b.geometry))return 5;
        child_calls=0;
        if(((OriginalQuery)(code+512))(0,0)!=0)return 6;
        if(bh_collision_query_node(&q,0,0,&relative,&world,&hit)!=0)return 7;
        ++cases;
        if((child_calls!=0)!=(q.work!=1))++differences;
        printf("plane probe=%lu original_children=%lu source_work=%lu\n",probe,child_calls,q.work);
        bh_collision_query_dispose(&q);bh_collision_cache_dispose(&a.cache);bh_collision_cache_dispose(&b.cache);
    }
    _controlfp(saved,_MCW_PC|_MCW_RC|_MCW_EM);VirtualFree(code,0,MEM_RELEASE);
    printf("{\"plane_cases\":%lu,\"plane_differences\":%lu,\"pc\":53}\n",cases,differences);
    return differences?1:0;
}
