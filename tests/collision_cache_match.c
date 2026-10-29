#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/bh_collision_cache.h"
#include "bh_collision_cache_core.h"
#include "original-cache.h"

typedef void (__cdecl *CacheOwner)(BHCollisionCacheCoreView *);
static unsigned long state = 0x44b130UL;
static unsigned long word(void)
{
    state = state * 1664525UL + 1013904223UL;
    return state;
}
static void *__cdecl allocate(unsigned long bytes, void *user)
{
    (void)user;
    return malloc(bytes);
}
static void __cdecl release(void *pointer, void *user)
{
    (void)user;
    free(pointer);
}
static void release_original(BHCollisionCacheCoreView *model)
{
    free(model->flags);
    free(model->centers);
    free(model->radii);
    free(model->squared_radii);
    free(model->faces);
}
static int cache_differs(const BHCollisionCacheCoreView *a, const BHCollisionCacheCoreView *b)
{
    return memcmp(a->flags, b->flags, a->face_count * 4) ||
           memcmp(a->faces, b->faces, a->face_count * 12) ||
           memcmp(a->centers, b->centers, a->node_count * 12) ||
           memcmp(a->radii, b->radii, a->node_count * 4) ||
           memcmp(a->squared_radii, b->squared_radii, a->node_count * 4);
}
int main(void)
{
    unsigned char *code = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    unsigned int saved = _control87(0, 0), precisions[3] = {_PC_24, _PC_53, _PC_64};
    unsigned long pc, i, j, k, candidate_differences[3] = {0,0,0}, production_differences[3] = {0,0,0}, lazy_differences = 0;
    BHBspFace faces[8];
    BHBspNode nodes[9];
    BHCollisionCacheCoreView expected, actual, lazy_before, production_view;
    BHCollisionCache production;
    BHSceneAllocator allocator;
    BHBsp bsp;
    if (!code)
        return 2;
    install_original_cache(code);
    FlushInstructionCache(GetCurrentProcess(), code, 4096);
    for (pc = 0; pc < 3; ++pc)
    {
        _control87(precisions[pc] | _RC_NEAR | _MCW_EM, _MCW_PC | _MCW_RC | _MCW_EM);
        for (i = 0; i < 4608; ++i)
        {
            memset(&expected, 0, sizeof(expected));
            expected.face_count = 1 + word() % 8;
            expected.node_count = 1 + word() % 9;
            expected.source_faces = faces;
            expected.nodes = nodes;
            for (j = 0; j < 8; ++j)
            {
                faces[j].material = word();
                faces[j].first_edge = word();
                faces[j].runtime = word();
            }
            memset(nodes, 0, sizeof(nodes));
            for (j = 0; j < 9; ++j)
                for (k = 0; k < 3; ++k)
                {
                    nodes[j].minimum[k] = (float)((long)(word() >> 8) - 0x800000L) / 1000.0f;
                    nodes[j].maximum[k] = nodes[j].minimum[k] + (float)(word() & 65535) / 997.0f;
                    if(i>=4096)
                    {
                        static const unsigned long inputs[]={0,0x80000000UL,1,0x80000001UL,
                            0x007fffffUL,0x00800000UL,0x3eaaaaabUL,0xbeaaaaabUL,
                            0x3f7fffffUL,0x3f800001UL,0x4b000001UL,0xcb000001UL,
                            0x5a000001UL,0xda000001UL};
                        unsigned long bits=inputs[(i+j+k)%14];
                        memcpy(&nodes[j].minimum[k],&bits,4);
                        nodes[j].maximum[k]=nodes[j].minimum[k];
                        if(i&1)nodes[j].maximum[k]+=.125f;
                    }
                }
            actual = expected;
            ((CacheOwner)code)(&expected);
            bh_collision_cache_prepare_core(&actual);
            if (cache_differs(&expected, &actual))
            {
                if (!candidate_differences[pc])
                    for (j = 0; j < (unsigned long)expected.node_count; ++j)
                        if (memcmp(expected.squared_radii+j, actual.squared_radii+j, 4))
                        {
                            printf("candidate first pc=%u case=%lu node=%lu squared expected=%08lx actual=%08lx radius expected=%08lx actual=%08lx\n",
                                   pc == 0 ? 24 : pc == 1 ? 53 : 64, i, j,
                                   ((unsigned long *)expected.squared_radii)[j],((unsigned long *)actual.squared_radii)[j],
                                   ((unsigned long *)expected.radii)[j],((unsigned long *)actual.radii)[j]);
                            break;
                        }
                ++candidate_differences[pc];
            }
            memset(&production, 0, sizeof(production));
            memset(&bsp, 0, sizeof(bsp));
            allocator.allocate = allocate;
            allocator.release = release;
            allocator.user = NULL;
            bsp.faces = faces;
            bsp.nodes = nodes;
            bsp.counts[3] = expected.face_count;
            bsp.counts[4] = expected.node_count;
            if (!bh_collision_cache_prepare(&production, &bsp, &allocator))
                return 3;
            production_view = expected;
            production_view.flags = production.face_flags;
            production_view.faces = production.faces;
            production_view.centers = production.centers;
            production_view.radii = production.radii;
            production_view.squared_radii = production.squared_radii;
            if (cache_differs(&expected, &production_view))
                ++production_differences[pc];
            /* Both original and production lazy guards preserve mutable data. */
            production.face_flags[0]=expected.flags[0]=0x87654321UL;
            production.faces[0].runtime=expected.faces[0].runtime=7;
            production.centers[0].x=expected.centers[0].x=-3.25f;
            ((CacheOwner)code)(&expected);
            {
                BHCollisionCache before=production;
                if(!bh_collision_cache_prepare(&production,&bsp,&allocator)||
                   memcmp(&before,&production,sizeof(before))||cache_differs(&expected,&production_view))
                    ++lazy_differences;
            }
            bh_collision_cache_dispose(&production);
            lazy_before = actual;
            bh_collision_cache_prepare_core(&actual);
            if (memcmp(&actual, &lazy_before, sizeof(actual)))
                ++lazy_differences;
            release_original(&actual);
            release_original(&expected);
        }
    }
    _control87(saved, _MCW_PC | _MCW_RC | _MCW_EM);
    VirtualFree(code, 0, MEM_RELEASE);
    printf("{\"cases\":13824,\"candidate_differences_pc24\":%lu,\"candidate_differences_pc53\":%lu,\"candidate_differences_pc64\":%lu,\"production_differences_pc24\":%lu,\"production_differences_pc53\":%lu,\"production_differences_pc64\":%lu,\"lazy_differences\":%lu}\n",
           candidate_differences[0],candidate_differences[1],candidate_differences[2],production_differences[0],production_differences[1],production_differences[2],lazy_differences);
    return lazy_differences || candidate_differences[0] || candidate_differences[1] ||
           candidate_differences[2] || production_differences[0] ||
           production_differences[1] || production_differences[2] ? 1 : 0;
}
