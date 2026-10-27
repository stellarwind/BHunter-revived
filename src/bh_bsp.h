#ifndef BH_BSP_H
#define BH_BSP_H
#include "bh_scene.h"
#include "bh_math.h"

typedef struct BHBspMaterial
{
    unsigned char attributes[9];
    char texture[23];
} BHBspMaterial;
typedef struct BHBspVertex
{
    float x, y, z;
    unsigned char opaque[48];
} BHBspVertex;
typedef struct BHBspEdge
{
    long vertex, next;
    float u, v;
} BHBspEdge;
typedef struct BHBspFace
{
    long material, first_edge, runtime;
} BHBspFace;
typedef struct BHBspNode
{
    float minimum[3], maximum[3], normal[3], plane;
    long front_leaf, back_leaf, front_node, back_node;
} BHBspNode;
typedef struct BHBspLeaf
{
    long face, next;
} BHBspLeaf;
typedef struct BHBspBound
{
    BHVec3 center;
    float radius_squared, radius;
} BHBspBound;
typedef enum BHBspError
{
    BH_BSP_OK,
    BH_BSP_ARGUMENT,
    BH_BSP_TRUNCATED,
    BH_BSP_FORMAT,
    BH_BSP_LIMIT,
    BH_BSP_MEMORY,
    BH_BSP_IO,
    BH_BSP_NUMERICAL,
    BH_BSP_CYCLE
} BHBspError;
typedef enum BHBspSearch
{
    BH_BSP_BARE_ONLY = -1,
    BH_BSP_PREFIX_FIRST = 0,
    BH_BSP_BARE_FIRST = 1,
    BH_BSP_PREFIX_ONLY = 2
} BHBspSearch;
typedef struct BHBsp
{
    const BHBspMaterial *materials;
    const BHBspVertex *vertices;
    const BHBspEdge *edges;
    const BHBspFace *faces;
    const BHBspNode *nodes;
    const BHBspLeaf *leaves;
    unsigned long counts[6];
    BHBspBound *bounds;
    float root_radius;
    unsigned char *storage;
    unsigned long bytes;
    BHBspError error;
    BHSceneAllocator allocator;
} BHBsp;
/* Explicit data adapters; not the original renderer/collision object ABI.
   Initialize once; dispose before reinitializing. Input may not overlap owned
   storage. Decode/read/load failure retains previous output. Numeric profile
   is the checked PC53 math contract. No texture object is fabricated.
   Limits: 65535 records per nonempty section, 16 MiB input, 256-byte paths. */
int __cdecl bh_bsp_init(BHBsp *bsp, const BHSceneAllocator *allocator);
void __cdecl bh_bsp_dispose(BHBsp *bsp);
int __cdecl bh_bsp_decode(BHBsp *bsp, const void *bytes, unsigned long size);
int __cdecl bh_bsp_read(BHBsp *bsp, BHVFile *file);
int __cdecl bh_bsp_load(BHBsp *bsp, const char *name, const char *prefix, BHBspSearch search);
#endif
