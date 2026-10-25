#include "bh_geometry.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef char face_info_layout[sizeof(BHFaceInfo)==44 && offsetof(BHFaceInfo,normal)==28 ? 1:-1];
static void *__cdecl allocate(unsigned long n,void *user){return malloc(n);}
static void __cdecl release(void *p,void *user){free(p);}
static int fail(BHGeometry *g,BHBspError e){if(g)g->error=e;return 0;}
static int face_index(BHGeometry *g,long face){return g && g->bsp && g->face_flags && face>=0 && (unsigned long)face<g->bsp->counts[3];}

int __cdecl bh_geometry_init(BHGeometry *g,const BHBsp *bsp,const BHSceneAllocator *allocator) {
    BHGeometry next;
    if(!g || !bsp || !bsp->storage || !bsp->faces || !bsp->edges || !bsp->vertices ||
       !bsp->nodes || !bsp->leaves || !bsp->materials || !bsp->counts[3] || bsp->counts[3]>65535 ||
       (allocator && (!allocator->allocate || !allocator->release)))return 0;
    memset(&next,0,sizeof(next));
    if(allocator)next.allocator=*allocator;
    else{next.allocator.allocate=allocate;next.allocator.release=release;}
    next.face_flags=(unsigned long*)next.allocator.allocate(bsp->counts[3]*4,next.allocator.user);
    if(!next.face_flags)return 0;
    /* The original collision loader initializes its separate +78 array to 0;
       these are runtime face flags, not the serialized face's third word. */
    memset(next.face_flags,0,bsp->counts[3]*4);next.bsp=bsp;*g=next;return 1;
}
int __cdecl bh_geometry_dispose(BHGeometry *g) {
    if(!g || g->enumerating)return fail(g,BH_BSP_ARGUMENT);
    if(g->face_flags)g->allocator.release(g->face_flags,g->allocator.user);
    memset(g,0,sizeof(*g));return 1;
}
int __cdecl bh_geometry_set_flags(BHGeometry *g,long face,unsigned long flags) {
    if(!face_index(g,face))return fail(g,BH_BSP_ARGUMENT);
    g->face_flags[face]=flags;g->error=BH_BSP_OK;return 1;
}
int __cdecl bh_geometry_get_flags(BHGeometry *g,long face,unsigned long *flags) {
    if(!face_index(g,face) || !flags)return fail(g,BH_BSP_ARGUMENT);
    *flags=g->face_flags[face];g->error=BH_BSP_OK;return 1;
}
int __cdecl bh_geometry_vertex(BHGeometry *g,long face,long ordinal,BHVec3 *out) {
    const BHBsp *b;long edge;unsigned long steps=0;
    if(!face_index(g,face) || ordinal<0 || !out)return fail(g,BH_BSP_ARGUMENT);
    b=g->bsp;edge=b->faces[face].first_edge;
    while(ordinal && edge>=0 && (unsigned long)edge<b->counts[2] && steps++<b->counts[2]) {
        edge=b->edges[edge].next;--ordinal;
    }
    if(ordinal || edge<0 || (unsigned long)edge>=b->counts[2] || b->edges[edge].vertex<0 ||
       (unsigned long)b->edges[edge].vertex>=b->counts[1])return fail(g,BH_BSP_FORMAT);
    memcpy(out,&b->vertices[b->edges[edge].vertex],sizeof(*out));g->error=BH_BSP_OK;return 1;
}
int __cdecl bh_geometry_face(BHGeometry *g,long face,BHFaceInfo *out) {
    const BHBsp *b;BHFaceInfo next;long edge,root;unsigned long i,j,steps;int found;
    if(!face_index(g,face) || !out)return fail(g,BH_BSP_ARGUMENT);
    b=g->bsp;next=*out;next.face=face;next.vertices=0;next.geometry=g;
    next.flags=g->face_flags[face];
    if(b->faces[face].material<0 || (unsigned long)b->faces[face].material>=b->counts[0])return fail(g,BH_BSP_FORMAT);
    next.texture=b->materials[b->faces[face].material].texture;
    edge=b->faces[face].first_edge;
    while(edge>=0) {
        if((unsigned long)edge>=b->counts[2] || next.vertices++>=b->counts[2])return fail(g,BH_BSP_CYCLE);
        edge=b->edges[edge].next;
    }
    if(!bh_geometry_vertex(g,face,0,&next.point))return 0;
    /* Preserve exact scan/overwrite order, including the first predecessor
       when next chains share a tail. The decoded BSP owns acyclic chains. */
    for(i=0;i<b->counts[5];++i)if(b->leaves[i].face==face) {
        root=(long)i;steps=0;
        do {
            found=0;
            for(j=0;j<b->counts[5];++j)if(b->leaves[j].next==root) {root=(long)j;found=1;break;}
            if(found && ++steps>b->counts[5])return fail(g,BH_BSP_CYCLE);
        }while(found);
        for(j=0;j<b->counts[4];++j) {
            const BHBspNode *n=b->nodes+j;
            if(n->front_leaf==root)memcpy(&next.normal,n->normal,12);
            else if(n->back_leaf==root) {
                next.normal.x=-n->normal[0];next.normal.y=-n->normal[1];next.normal.z=-n->normal[2];
            }
        }
    }
    *out=next;g->error=BH_BSP_OK;return 1;
}
int __cdecl bh_geometry_each_face(BHGeometry *g,BHFaceCallback callback,void *user) {
    unsigned long i;
    if(!g || !g->bsp || !callback || g->enumerating)return fail(g,BH_BSP_ARGUMENT);
    g->enumerating=1;
    for(i=0;i<g->bsp->counts[3];++i) {
        if(!bh_geometry_face(g,(long)i,&g->scratch)){g->enumerating=0;return 0;}
        callback(&g->scratch,user);
    }
    g->enumerating=0;g->error=BH_BSP_OK;return 1;
}
