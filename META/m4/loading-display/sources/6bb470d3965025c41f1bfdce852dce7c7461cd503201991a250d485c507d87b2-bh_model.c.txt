#include "bh_model.h"
#include "bh_texture.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef char model_size[sizeof(BHModel)==0x80?1:-1];
typedef char model_transform[offsetof(BHModel,transform)==8?1:-1];
typedef char model_materials[offsetof(BHModel,materials)==0x3c?1:-1];
typedef char model_images[offsetof(BHModel,images)==0x68?1:-1];
typedef char model_runtime[offsetof(BHModel,runtime)==0x6c?1:-1];
static void *__cdecl allocate(unsigned long n,void *user){return malloc(n);}
static void __cdecl release(void *p,void *user){free(p);}
static int fail(BHModelContext *c,BHBspError error){if(c)c->error=error;return 0;}
int __cdecl bh_model_context_init(BHModelContext *c,BHTextureContext *textures,const BHSceneAllocator *a) {
    if(!c || !textures || (a && (!a->allocate || !a->release)))return 0;
    memset(c,0,sizeof(*c));c->textures=textures;c->search=BH_BSP_BARE_ONLY;
    if(a)c->allocator=*a;else{c->allocator.allocate=allocate;c->allocator.release=release;}
    return 1;
}
int __cdecl bh_model_set_path(BHModelContext *c,const char *prefix,BHBspSearch search) {
    unsigned long n=0;
    if(!c || !prefix || search<BH_BSP_BARE_ONLY || search>BH_BSP_PREFIX_ONLY)return fail(c,BH_BSP_ARGUMENT);
    while(n<256 && prefix[n])++n;if(n==256)return fail(c,BH_BSP_LIMIT);
    memcpy(c->prefix,prefix,n+1);c->search=search;c->error=BH_BSP_OK;return 1;
}
void __cdecl bh_model_dispose(BHModelContext *c,BHModel *m) {
    if(!c || !m)return;
    if(m->materials)c->allocator.release(m->materials,c->allocator.user);
    if(m->vertices)c->allocator.release(m->vertices,c->allocator.user);
    if(m->edges)c->allocator.release(m->edges,c->allocator.user);
    if(m->faces)c->allocator.release(m->faces,c->allocator.user);
    if(m->nodes)c->allocator.release(m->nodes,c->allocator.user);
    if(m->leaves)c->allocator.release(m->leaves,c->allocator.user);
    if(m->images)c->allocator.release(m->images,c->allocator.user);
    memset(m,0,sizeof(*m));
}
void __cdecl bh_model_delete(BHModelContext *c,BHModel *m) {
    if(!c || !m)return;bh_model_dispose(c,m);c->allocator.release(m,c->allocator.user);
}
static void *copy(BHModelContext *c,const void *p,unsigned long bytes) {
    void *out=c->allocator.allocate(bytes,c->allocator.user);if(out)memcpy(out,p,bytes);return out;
}
int __cdecl bh_model_read(BHModelContext *c,BHVFile *file,BHModel *model) {
    BHModel next;BHBsp data;unsigned long i,j;BHBspError error;
    if(!c || !c->textures || !c->allocator.allocate || !file || !model)return fail(c,BH_BSP_ARGUMENT);
    memset(&next,0,sizeof(next));bh_bsp_init(&data,&c->allocator);
    /* Explicit validation dependency rejects corrupt record graphs before any
       texture cache side effects. Original record arrays are copied verbatim. */
    if(!bh_bsp_read(&data,file)){error=data.error;bh_bsp_dispose(&data);return fail(c,error);}
    next.transform.m[0][0]=next.transform.m[1][1]=next.transform.m[2][2]=1;
    next.radius=data.root_radius;
    next.material_count=data.counts[0];next.vertex_count=data.counts[1];next.edge_count=data.counts[2];
    next.face_count=data.counts[3];next.node_count=data.counts[4];next.leaf_count=data.counts[5];
    next.materials=(BHBspMaterial *)copy(c,data.materials,next.material_count*32);
    next.vertices=(BHBspVertex *)copy(c,data.vertices,next.vertex_count*60);
    next.edges=(BHBspEdge *)copy(c,data.edges,next.edge_count*16);
    next.faces=(BHBspFace *)copy(c,data.faces,next.face_count*12);
    next.nodes=(BHBspNode *)copy(c,data.nodes,next.node_count*56);
    next.leaves=(BHBspLeaf *)copy(c,data.leaves,next.leaf_count*8);
    next.images=(BHTextureImage **)c->allocator.allocate(next.material_count*sizeof(*next.images),c->allocator.user);
    bh_bsp_dispose(&data);
    if(!next.materials || !next.vertices || !next.edges || !next.faces || !next.nodes || !next.leaves || !next.images) {
        bh_model_dispose(c,&next);return fail(c,BH_BSP_MEMORY);
    }
    for(i=0;i<next.material_count;++i) {
        next.images[i]=NULL;
        for(j=0;j<next.face_count;++j)if(next.faces[j].material==(long)i)break;
        if(j<next.face_count && next.materials[i].texture[0])
            next.images[i]=bh_texture_load(c->textures,next.materials[i].texture);
    }
    bh_model_dispose(c,model);*model=next;c->error=BH_BSP_OK;return 1;
}
BHModel *__cdecl bh_model_load(BHModelContext *c,const char *name) {
    char path[256];unsigned long n=0,p;BHVFile *file=NULL;BHModel *model;
    if(!c || !name || !c->textures || !c->allocator.allocate){fail(c,BH_BSP_ARGUMENT);return NULL;}
    while(n<256 && name[n])++n;p=(unsigned long)strlen(c->prefix);
    if(!n || n==256 || p+n>=256){fail(c,BH_BSP_LIMIT);return NULL;}
    memcpy(path,c->prefix,p);memcpy(path+p,name,n+1);
    switch(c->search) {
    case BH_BSP_PREFIX_FIRST:file=bh_vfile_open(path,"rb");if(!file)file=bh_vfile_open(name,"rb");break;
    case BH_BSP_BARE_FIRST:file=bh_vfile_open(name,"rb");if(!file)file=bh_vfile_open(path,"rb");break;
    case BH_BSP_PREFIX_ONLY:file=bh_vfile_open(path,"rb");break;
    case BH_BSP_BARE_ONLY:file=bh_vfile_open(name,"rb");break;
    }
    if(!file){fail(c,BH_BSP_IO);return NULL;}
    model=(BHModel *)c->allocator.allocate(sizeof(*model),c->allocator.user);
    if(!model){bh_vfile_close(file);fail(c,BH_BSP_MEMORY);return NULL;}
    memset(model,0,sizeof(*model));
    if(!bh_model_read(c,file,model)){bh_vfile_close(file);bh_model_delete(c,model);return NULL;}
    if(bh_vfile_close(file)){bh_model_delete(c,model);fail(c,BH_BSP_IO);return NULL;}
    return model;
}
