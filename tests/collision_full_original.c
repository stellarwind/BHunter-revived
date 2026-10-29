/* Complete original traversal bodies, isolated from all game/platform execution. */
#include "../src/bh_collision_core_model.h"
#include "../src/bh_collision_cache.h"
#include "../src/bh_collision_query.h"
#include <windows.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char original_state[0x8c];
#ifndef BH_COLLISION_ORIGINAL_HEADER
#define BH_COLLISION_ORIGINAL_HEADER "../build/collision-full-original/original-full.h"
#endif
#include BH_COLLISION_ORIGINAL_HEADER
typedef int (__cdecl *OriginalQuery)(int, int);
typedef int (__cdecl *OriginalFace)(void);
typedef struct Mesh
{
    BHBsp bsp;
    BHBspVertex vertices[28];
    BHBspEdge edges[28];
    BHBspFace faces[7];
    BHBspNode nodes[7];
    BHBspLeaf leaves[14];
    BHBspBound bounds[7];
    BHCollisionCache cache;
    BHGeometry geometry;
    BHCollisionCoreModel view;
} Mesh;
static void *__cdecl allocate(unsigned long size, void *user) { return malloc(size); }
static void __cdecl release(void *p, void *user) { free(p); }
static void identity(BHMat34 *m)
{
    memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;
}
static void bind_state(unsigned long offset, const void *pointer)
{
    memcpy(original_state+offset,&pointer,4);
}
static long read_word(const unsigned char *p) { long v;memcpy(&v,p,4);return v; }
static int prepare(Mesh *m,int destination,unsigned long variant,int stress)
{
    unsigned long i,j;
    memset(m,0,sizeof(*m));
    m->bsp.vertices=m->vertices;m->bsp.edges=m->edges;m->bsp.faces=m->faces;
    m->bsp.nodes=m->nodes;m->bsp.leaves=m->leaves;m->bsp.bounds=m->bounds;
    m->bsp.counts[1]=m->bsp.counts[2]=28;m->bsp.counts[3]=m->bsp.counts[4]=7;
    m->bsp.counts[5]=14;m->bsp.allocator.allocate=allocate;m->bsp.allocator.release=release;
    for(i=0;i<7;++i)
    {
        BHBspNode *n=m->nodes+i;
        float extent=1.0f+(float)((variant+i)%5)*.125f;
        for(j=0;j<4;++j)
        {
            BHBspVertex *v=m->vertices+4*i+j;
            v->y=(j==0||j==3)?-extent:extent;
            if(destination)v->z=j<2?-extent:extent;
            else v->x=j<2?-extent:extent;
            if(stress)
            {
                static const float across[4]={-1.123457f,-1.315679f,1.234567f,1.456789f};
                static const float along[4]={-1.712345f,1.956789f,1.801237f,-1.834567f};
                float factor=extent*.625f;
                v->y=along[j]*factor;
                if(destination)v->z=across[j]*factor;
                else v->x=across[j]*factor;
            }
            if(stress>=2&&destination)
            {
                /* Convex quad in a tilted plane. All normal components enter
                   the original/source plane sums; no gameplay is executed. */
                float across=v->z,along=v->y;
                v->x=across*.8f+along*.48f;
                v->y=across*-.6f+along*.64f;
                v->z=along*-.6f;
            }
            m->edges[4*i+j].vertex=4*i+j;
            m->edges[4*i+j].next=j==3?-1:4*i+j+1;
        }
        m->faces[i].first_edge=4*i;
        n->minimum[0]=n->minimum[1]=n->minimum[2]=-2;
        n->maximum[0]=n->maximum[1]=n->maximum[2]=2;
        n->normal[destination?0:2]=1;
        if(stress>=2&&destination)
        {
            n->normal[0]=.36f;n->normal[1]=.48f;n->normal[2]=.8f;
        }
        n->plane=(variant&16)&&i==0?8:0;
        n->front_node=i<3?2*i+1:-1;n->back_node=i<3?2*i+2:-1;
        n->front_leaf=(variant&(1UL<<(i%4)))?-1:2*i;
        n->back_leaf=(variant&32)?2*i+1:-1;
        m->leaves[2*i].face=(variant&64)&&i?0:i;
        m->leaves[2*i+1].face=i;
        m->leaves[2*i].next=(variant&128)?2*i+1:-1;
        m->leaves[2*i+1].next=-1;
    }
    if(!bh_collision_cache_prepare(&m->cache,&m->bsp,0))return 0;
    for(i=0;i<7;++i)
    {
        m->bounds[i].center=m->cache.centers[i];m->bounds[i].radius=m->cache.radii[i];
        m->bounds[i].radius_squared=m->cache.squared_radii[i];
        m->cache.face_flags[i]=(((variant>>(i+8))&1)?1:0)|((variant&256)?0x87650000UL:0);
    }
    m->geometry.bsp=&m->bsp;m->geometry.face_flags=m->cache.face_flags;
    m->geometry.allocator=m->bsp.allocator;
    m->view.vertices=m->vertices;m->view.edges=m->edges;m->view.nodes=m->nodes;
    m->view.leaves=m->leaves;m->view.faces=m->cache.faces;m->view.flags=m->cache.face_flags;
    m->view.centers=m->cache.centers;m->view.radii=m->cache.radii;
    return 1;
}
int main(void)
{
    unsigned char *code=(unsigned char *)VirtualAlloc(0,8192,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    unsigned int saved=_controlfp(0,0);
    Mesh a,b;
    BHCollisionQuery query;
    BHCollisionHit hit,untouched;
    BHMat34 relative,world;
    unsigned char old_hit[48];
    unsigned long variant,pose,mode,i,cases=0,differences=0,state_differences=0,hits=0,cached_cases=0,cached_differences=0,cached_hits=0;
    int expected,actual;
    if(!code)return 2;
    _controlfp(_PC_53|_RC_NEAR|_MCW_EM,_MCW_PC|_MCW_RC|_MCW_EM);
    install_original_collision(code);FlushInstructionCache(GetCurrentProcess(),code,8192);
    for(variant=0;variant<6144;++variant)
    {
        int stress=variant>=4096?2:(variant>=2048?1:0);
        if(!prepare(&a,0,variant,stress)||!prepare(&b,1,(variant*13)^0x246,stress))return 3;
        if(!bh_collision_query_init(&query,&a.geometry,&b.geometry))return 4;
        for(pose=0;pose<8;++pose)
        {
            identity(&relative);identity(&world);
            relative.m[0][3]=(float)((long)(pose%4)-1)*.375f;
            relative.m[2][3]=(float)((long)(pose/4)-1)*.5f;
            if(pose==7)relative.m[0][3]=16;
            world.m[0][3]=3;world.m[1][3]=-7;
            if(pose==5){relative.m[0][0]=0;relative.m[0][1]=-1;relative.m[1][0]=1;relative.m[1][1]=0;}
            for(mode=0;mode<2;++mode)
            {
                memset(original_state,0,sizeof(original_state));memset(old_hit,0xa5,sizeof(old_hit));
                bind_state(8,&a.view);bind_state(12,&b.view);bind_state(0x30,&world);bind_state(0x88,old_hit);
                memcpy(original_state+0x4c,&relative,48);
                memset(&hit,0xa5,sizeof(hit));untouched=hit;
                expected=((OriginalQuery)(code+(mode?512:0)))(0,0);
                actual=mode?bh_collision_query_node(&query,0,0,&relative,&world,&hit):
                            bh_collision_query_run(&query,0,0,&relative,&world,&hit);
                ++cases;
                if(expected)++hits;
                if(expected!=actual || (!expected&&memcmp(&hit,&untouched,sizeof(hit))) || (expected &&
                   (memcmp(old_hit+4,&hit.contact.point,12)||memcmp(old_hit+16,&hit.normal,12)||
                    memcmp(original_state+0x3c,&hit.contact.raw_point,12)||
                    read_word(original_state)!=hit.a_node||read_word(original_state+4)!=hit.b_node||
                    read_word(old_hit+40)!=hit.b_face||read_word(old_hit+44)!=(long)hit.flags||
                    read_word(original_state+0x7c)!=(long)(a.cache.faces+hit.a_face))))
                {
                    if(differences<4){
                        printf("mismatch variant=%lu pose=%lu mode=%lu old=%d source=%d nodes=%ld/%ld vs %ld/%ld face=%ld/%ld flags=%ld/%ld\n",variant,pose,mode,expected,actual,read_word(original_state),read_word(original_state+4),hit.a_node,hit.b_node,read_word(old_hit+40),hit.b_face,read_word(old_hit+44),hit.flags);
                        for(i=0;i<3;++i)printf(" axis%lu point=%08lx/%08lx raw=%08lx/%08lx normal=%08lx/%08lx\n",i,read_word(old_hit+4+4*i),read_word((unsigned char *)&hit.contact.point+4*i),read_word(original_state+0x3c+4*i),read_word((unsigned char *)&hit.contact.raw_point+4*i),read_word(old_hit+16+4*i),read_word((unsigned char *)&hit.normal+4*i));
                    }
                    ++differences;
                }
                for(i=0;i<7;++i)
                    if(a.cache.faces[i].runtime||query.suppressed[i])++state_differences;
                if(query.busy||query.error!=BH_BSP_OK)++state_differences;
                if(expected)
                {
                    BHFaceContact cached,old_cached;
                    BHVec3 normal;
                    long af=(read_word(original_state+0x7c)-(long)a.cache.faces)/sizeof(BHBspFace);
                    long bf=read_word(old_hit+40),bnode=read_word(original_state+4);
                    int cached_expected,cached_actual,trial_index;
                    memcpy(&normal,old_hit+16,12);
                    for(trial_index=0;trial_index<2;++trial_index)
                    {
                    BHMat34 trial=relative;
                    if(trial_index)trial.m[0][3]=16;
                    memcpy(original_state+0x4c,&trial,48);
                    memset(&cached,0xa5,sizeof(cached));old_cached=cached;
                    cached_expected=((OriginalFace)(code+5120))();
                    cached_actual=bh_collision_face_contact(&a.bsp,af,&trial,&b.bsp,bf,bnode,&world,&normal,&cached);
                    ++cached_cases;
                    if(cached_expected)++cached_hits;
                    if(cached_expected!=cached_actual||(cached_expected&&
                       (memcmp(old_hit+4,&cached.point,12)||memcmp(original_state+0x3c,&cached.raw_point,12)))||
                       (!cached_expected&&memcmp(&cached,&old_cached,sizeof(cached))))
                    {
                        if(cached_differences<2)printf("cached mismatch variant=%lu pose=%lu mode=%lu old=%d source=%d\n",variant,pose,mode,cached_expected,cached_actual);
                        ++cached_differences;
                    }
                    }
                }
            }
        }
        bh_collision_query_dispose(&query);bh_collision_cache_dispose(&a.cache);bh_collision_cache_dispose(&b.cache);
    }
    _controlfp(saved,_MCW_PC|_MCW_RC|_MCW_EM);VirtualFree(code,0,MEM_RELEASE);
    printf("{\"cases\":%lu,\"hits\":%lu,\"differences\":%lu,\"state_differences\":%lu,\"cached_cases\":%lu,\"cached_hits\":%lu,\"cached_differences\":%lu,\"pc\":53}\n",cases,hits,differences,state_differences,cached_cases,cached_hits,cached_differences);
    return differences||state_differences||cached_differences?1:0;
}
