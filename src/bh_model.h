#ifndef BH_MODEL_H
#define BH_MODEL_H
#include "bh_bsp.h"
#include "bh_texture.h"
/* Original 0x80 x86 rendering-model layout. Raw vertex/face cache bytes are
   retained, never interpreted as host pointers by the loader. */
typedef struct BHModel
{
    unsigned long frame;
    float radius;
    BHMat34 transform;
    unsigned long material_count;
    BHBspMaterial *materials;
    unsigned long vertex_count;
    BHBspVertex *vertices;
    unsigned long edge_count;
    BHBspEdge *edges;
    unsigned long face_count;
    BHBspFace *faces;
    unsigned long node_count;
    BHBspNode *nodes;
    unsigned long leaf_count;
    BHBspLeaf *leaves;
    BHTextureImage **images;
    unsigned long runtime[5];
} BHModel;
typedef struct BHModelContext
{
    BHSceneAllocator allocator;
    BHTextureContext *textures;
    char prefix[256];
    BHBspSearch search;
    BHBspError error;
} BHModelContext;
/* Explicit context/lifetime adapters. Texture context outlives models; image
   cache entries are borrowed. Initialize model to zero before first read.
   Dispose every model with the same context/allocator that created it. */
int __cdecl bh_model_context_init(BHModelContext *, BHTextureContext *, const BHSceneAllocator *);
int __cdecl bh_model_set_path(BHModelContext *, const char *prefix, BHBspSearch);
void __cdecl bh_model_dispose(BHModelContext *, BHModel *);
void __cdecl bh_model_delete(BHModelContext *, BHModel *);
/* 00457e20; bounded validation before publication is a source guard. Failure
   retains previous model and reports context.error. File remains caller-owned. */
int __cdecl bh_model_read(BHModelContext *, BHVFile *, BHModel *);
/* 00458140; original four search policies, fallback only on open failure. */
BHModel *__cdecl bh_model_load(BHModelContext *, const char *name);
#endif
