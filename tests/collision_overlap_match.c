/* Focused geometry-owner comparisons. No game, renderer or resource playback. */
#include "../src/bh_geometry_core.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../build/collision-overlap-match/original-geometry.h"

typedef void (__cdecl *SetFlags)(BHGeometryCoreView *, long, unsigned long);
typedef unsigned long (__cdecl *GetFlags)(const BHGeometryCoreView *, long);
typedef void (__cdecl *Vertex)(const BHGeometryCoreView *, long, long, BHVec3 *);

static unsigned long next_word(unsigned long *state)
{
    *state = *state * 1664525UL + 1013904223UL;
    return *state;
}

int main(void)
{
    unsigned char *code;
    BHGeometryCoreView original_view, source_view;
    BHGeometry source;
    BHBsp bsp;
    BHBspVertex vertices[16];
    BHBspEdge edges[16];
    BHBspFace faces[16];
    unsigned long first[16], second[16], seed = 0x44e210UL;
    unsigned long i, j, face, value, got, differences = 0, guards = 0;
    BHVec3 expected, actual, production, untouched;
    SetFlags original_set;
    GetFlags original_get;
    Vertex original_vertex;
    code = (unsigned char *)VirtualAlloc(0,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if (!code) return 2;
    install_original_geometry(code);
    FlushInstructionCache(GetCurrentProcess(),code,4096);
    original_set = (SetFlags)code;
    original_get = (GetFlags)(code + 64);
    original_vertex = (Vertex)(code + 128);
    memset(&original_view,0xa5,sizeof(original_view));
    memset(&source_view,0x5a,sizeof(source_view));
    memset(&source,0,sizeof(source));
    memset(&bsp,0,sizeof(bsp));
    original_view.flags=first;source_view.flags=second;
    source.bsp=&bsp;source.face_flags=second;
    bsp.counts[3]=bsp.counts[2]=bsp.counts[1]=16;
    bsp.vertices=vertices;bsp.edges=edges;bsp.faces=faces;
    original_view.vertices=source_view.vertices=vertices;
    original_view.edges=source_view.edges=edges;
    original_view.faces=source_view.faces=faces;
    memset(first,0,sizeof(first));memset(second,0,sizeof(second));
    for (i=0;i<16384;++i)
    {
        face=next_word(&seed)%16;value=next_word(&seed);
        original_set(&original_view,face,value);
        if (!bh_geometry_set_flags(&source,face,value)) ++differences;
        if (memcmp(first,second,sizeof(first))) ++differences;
        got=0xa5a5a5a5UL;
        if (!bh_geometry_get_flags(&source,face,&got) || got!=original_get(&original_view,face)) ++differences;
        /* The complete cores also operate directly over real mutable arrays. */
        value=next_word(&seed);
        original_set(&original_view,face,value);bh_geometry_set_flags_core(&source_view,face,value);
        if (bh_geometry_get_flags_core(&source_view,face)!=original_get(&original_view,face) ||
            memcmp(first,second,sizeof(first))) ++differences;
    }
    for (i=0;i<2048;++i)
    {
        for (j=0;j<16;++j)
        {
            ((unsigned long *)&vertices[j])[0]=next_word(&seed);
            ((unsigned long *)&vertices[j])[1]=next_word(&seed);
            ((unsigned long *)&vertices[j])[2]=next_word(&seed);
            edges[j].vertex=(long)(next_word(&seed)%16);
            /* Permuted 16-edge cycle supports every bounded valid ordinal. */
            edges[j].next=(long)((j+5)%16);
            faces[j].first_edge=(long)(next_word(&seed)%16);
        }
        face=next_word(&seed)%16;j=next_word(&seed)%16;
        original_vertex(&original_view,face,j,&expected);
        bh_geometry_vertex_core(&source_view,face,j,&actual);
        if (!bh_geometry_vertex(&source,face,j,&production) ||
            memcmp(&expected,&actual,12) || memcmp(&expected,&production,12)) ++differences;
    }
    got=0x12345678UL;
    if (!bh_geometry_get_flags(&source,-1,&got) && got==0x12345678UL) ++guards;
    if (!bh_geometry_get_flags(&source,16,&got) && got==0x12345678UL) ++guards;
    if (!bh_geometry_get_flags(&source,0,0)) ++guards;
    if (!bh_geometry_get_flags(0,0,&got) && got==0x12345678UL) ++guards;
    if (!bh_geometry_set_flags(&source,-1,1) && !memcmp(first,second,sizeof(first))) ++guards;
    if (!bh_geometry_set_flags(&source,16,1) && !memcmp(first,second,sizeof(first))) ++guards;
    if (!bh_geometry_set_flags(0,0,1)) ++guards;
    memset(&untouched,0xa5,sizeof(untouched));actual=untouched;
    if (!bh_geometry_vertex(&source,-1,0,&actual) && !memcmp(&actual,&untouched,12)) ++guards;
    if (!bh_geometry_vertex(&source,16,0,&actual) && !memcmp(&actual,&untouched,12)) ++guards;
    if (!bh_geometry_vertex(&source,0,-1,&actual) && !memcmp(&actual,&untouched,12)) ++guards;
    if (!bh_geometry_vertex(&source,0,0,0)) ++guards;
    if (!bh_geometry_vertex(&source,0,17,&actual) && !memcmp(&actual,&untouched,12)) ++guards;
    j=faces[0].first_edge;faces[0].first_edge=-1;
    if (!bh_geometry_vertex(&source,0,0,&actual) && !memcmp(&actual,&untouched,12)) ++guards;
    faces[0].first_edge=j;value=edges[j].vertex;edges[j].vertex=16;
    if (!bh_geometry_vertex(&source,0,0,&actual) && !memcmp(&actual,&untouched,12)) ++guards;
    edges[j].vertex=value;
    printf("{\"flag_cases\":16384,\"installed_vertex_cases\":2048,\"differences\":%lu,\"guard_checks\":%lu}\n",differences,guards);
    VirtualFree(code,0,MEM_RELEASE);
    return differences || guards!=14;
}
