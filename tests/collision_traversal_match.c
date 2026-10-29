/* Isolated original traversal/gate arithmetic oracle. Candidate is uninstalled.
   Destination callback deliberately probes entry to the gate; it is not a
   recovered collision owner or a game-target definition. */
#include "../src/bh_collision_core_model.h"
#include "../src/bh_collision_query.h"
#include "../src/bh_round.h"
#include <windows.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BHCollisionCoreModel *bh_traversal_a, *bh_traversal_b;
BHMat34 bh_traversal_relative;
static void (__cdecl *original_transform)(const BHMat34 *,const BHVec3 *,BHVec3 *);
static int probe_node, probes;
int __cdecl bh_collision_traversal_core(int,int);
int __cdecl bh_collision_query_gate_check(BHCollisionQuery *,long,long,const BHMat34 *);
int __cdecl bh_collision_baseline_gate_check(BHCollisionQuery *,long,long,const BHMat34 *);
int __cdecl bh_traversal_node(long an,long bn)
{
    ++probes;
    return an==probe_node;
}
/* Explicit original-backed math adapter, isolated to this executable. */
void __cdecl bh_mat34_transform_point(const BHMat34 *m,const BHVec3 *p,BHVec3 *out)
{
    original_transform(m,p,out);
}
#include "../build/collision-traversal-match/original-traversal.h"

static void *__cdecl allocate(unsigned long size,void *user)
{
    return malloc(size);
}
static void __cdecl release(void *p,void *user)
{
    free(p);
}

int main(void)
{
    BHCollisionCoreModel a,b;
    BHBsp source_bsp,target_bsp;
    BHBspBound source_bounds[7],target_bounds[1];
    BHGeometry source_geometry,target_geometry;
    BHCollisionQuery query;
    BHCollisionHit untouched,output;
    BHBspNode saved_node;
    unsigned long source_flags[7]={0,0,0,0,0,0,0},target_flags[1]={0};
    BHBspNode nodes[7];
    BHBspLeaf leaves[7];
    BHBspFace faces[7];
    BHVec3 ac[7],bc[1];
    float ar[7],br[1];
    unsigned char *code;
    int (__cdecl *original)(int,int);
    unsigned int saved=_controlfp(0,0);
    unsigned int modes[3]={_PC_24,_PC_53,_PC_64};
    unsigned long mode,i,j,cases=0,differences=0,production_differences=0,query_checks=0,query_failures=0,baseline_differences[3]={0,0,0};
    int expected,actual,expected_probes;
    code=(unsigned char *)VirtualAlloc(0,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!code)return 2;
    original_transform=(void (__cdecl *)(const BHMat34 *,const BHVec3 *,BHVec3 *))(code+1024);
    install_original_traversal(code);
    FlushInstructionCache(GetCurrentProcess(),code,4096);
    original=(int (__cdecl *)(int,int))code;
    memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));
    memset(&bh_traversal_relative,0,sizeof(bh_traversal_relative));
    bh_traversal_relative.m[0][0]=bh_traversal_relative.m[1][1]=bh_traversal_relative.m[2][2]=1;
    memset(nodes,0,sizeof(nodes));memset(leaves,0,sizeof(leaves));memset(faces,0,sizeof(faces));
    memset(ac,0,sizeof(ac));memset(bc,0,sizeof(bc));
    a.nodes=nodes;a.leaves=leaves;a.faces=faces;a.centers=ac;a.radii=ar;
    b.centers=bc;b.radii=br;
    bh_traversal_a=&a;bh_traversal_b=&b;
    memset(&source_bsp,0,sizeof(source_bsp));memset(&target_bsp,0,sizeof(target_bsp));
    memset(&source_geometry,0,sizeof(source_geometry));memset(&target_geometry,0,sizeof(target_geometry));
    memset(&query,0,sizeof(query));
    source_bsp.nodes=target_bsp.nodes=nodes;
    source_bsp.bounds=source_bounds;target_bsp.bounds=target_bounds;
    source_bsp.counts[4]=7;target_bsp.counts[4]=1;
    source_bsp.counts[3]=7;target_bsp.counts[3]=1;
    source_geometry.bsp=&source_bsp;target_geometry.bsp=&target_bsp;
    source_geometry.face_flags=source_flags;target_geometry.face_flags=target_flags;
    source_geometry.allocator.allocate=allocate;source_geometry.allocator.release=release;
    if(!bh_collision_query_init(&query,&source_geometry,&target_geometry))return 2;
    memset(&untouched,0xa5,sizeof(untouched));
    for(j=0;j<7;++j)
    {
        nodes[j].front_leaf=j;nodes[j].back_leaf=-1;
        nodes[j].front_node=j<3?j*2+1:-1;nodes[j].back_node=j<3?j*2+2:-1;
        leaves[j].face=j;leaves[j].next=-1;
    }
    for(mode=0;mode<3;++mode)
    {
        _controlfp(modes[mode]|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
        for(i=0;i<4096;++i)
        {
            /* Boundary cancellation: centers are binary32, their difference
               needs more than24bits. Original retained gate rejects atPC53/64. */
            memset(ac,0,sizeof(ac));memset(bc,0,sizeof(bc));
            ac[0].x=1.0f;bc[0].x=-(float)(1+(i%31))*0.0000000037252902984619140625f;
            for(j=0;j<7;++j)ar[j]=0;
            br[0]=1.0f;
            probe_node=0;probes=0;expected=original(0,0);expected_probes=probes;
            probes=0;actual=bh_collision_traversal_core(0,0);
            if(expected!=actual || probes!=expected_probes)++differences;
            source_bounds[0].center=ac[0];source_bounds[0].radius=ar[0];
            target_bounds[0].center=bc[0];target_bounds[0].radius=br[0];
            if(expected!=bh_collision_baseline_gate_check(&query,0,0,&bh_traversal_relative))++baseline_differences[mode];
            if(expected!=bh_collision_query_gate_check(&query,0,0,&bh_traversal_relative))++production_differences;
            if(mode==1)
            {
                saved_node=nodes[0];nodes[0].front_leaf=nodes[0].back_leaf=-1;
                nodes[0].front_node=nodes[0].back_node=-1;output=untouched;
                if(bh_collision_query_run(&query,0,0,&bh_traversal_relative,&bh_traversal_relative,&output)!=0 ||
                    query.work!=(expected?4:1) || query.error!=BH_BSP_OK || query.busy ||
                    memcmp(&output,&untouched,sizeof(output)))++query_failures;
                for(j=0;j<7;++j)if(query.suppressed[j])++query_failures;
                ++query_checks;
                /* Exact tangency is an independent positive control: both
                   original gate and public source query must enter node work. */
                bc[0].x=0;target_bounds[0].center=bc[0];probes=0;
                if(original(0,0)!=1 || probes!=1)++query_failures;
                output=untouched;
                if(bh_collision_query_run(&query,0,0,&bh_traversal_relative,&bh_traversal_relative,&output)!=0 ||
                    query.work!=4 || query.error!=BH_BSP_OK || query.busy ||
                    memcmp(&output,&untouched,sizeof(output)))++query_failures;
                for(j=0;j<7;++j)if(query.suppressed[j])++query_failures;
                nodes[0]=saved_node;++query_checks;
            }
            ++cases;
            /* Seven-node recursion and balanced real face runtime words. */
            memset(ac,0,sizeof(ac));memset(bc,0,sizeof(bc));br[0]=100;
            probe_node=(int)(i%8)-1;probes=0;expected=original(0,0);expected_probes=probes;
            for(j=0;j<7;++j)if(faces[j].runtime)++differences;
            probes=0;actual=bh_collision_traversal_core(0,0);
            if(expected!=actual || probes!=expected_probes)++differences;
            for(j=0;j<7;++j)if(faces[j].runtime)++differences;
            ++cases;
        }
    }
    _controlfp(saved,_MCW_PC|_MCW_RC|_MCW_EM);
    bh_collision_query_dispose(&query);
    printf("{\"cases\":%lu,\"candidate_differences\":%lu,\"production_gate_differences\":%lu,\"public_query_checks\":%lu,\"public_query_failures\":%lu,\"baseline_tu_gate_differences_pc24\":%lu,\"baseline_tu_gate_differences_pc53\":%lu,\"baseline_tu_gate_differences_pc64\":%lu}\n",cases,differences,production_differences,query_checks,query_failures,baseline_differences[0],baseline_differences[1],baseline_differences[2]);
    VirtualFree(code,0,MEM_RELEASE);
    return differences || production_differences || query_failures || !baseline_differences[1] || !baseline_differences[2];
}
