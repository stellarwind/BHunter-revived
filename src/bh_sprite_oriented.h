#ifndef BH_SPRITE_ORIENTED_H
#define BH_SPRITE_ORIENTED_H
#include "bh_attribute_clipper.h"
#include "bh_model_draw.h"

/* Native form of the 0x54-byte path entry at 004e38e0. Pointers are borrowed;
   the original uses previous and next as two independent chain links. */
typedef struct BHSpriteOrientedVertex
{
    BHVec3 point;
    BHVec3 corners[4];
    struct BHSpriteOrientedVertex *previous, *next;
    unsigned long flags;
    float texture_start, width, texture_end;
} BHSpriteOrientedVertex;

typedef enum BHSpriteOrientedError
{
    BH_SPRITE_ORIENTED_OK,
    BH_SPRITE_ORIENTED_ARGUMENT,
    BH_SPRITE_ORIENTED_TOPOLOGY,
    BH_SPRITE_ORIENTED_CAPACITY,
    BH_SPRITE_ORIENTED_NUMERICAL,
    BH_SPRITE_ORIENTED_CLIP,
    BH_SPRITE_ORIENTED_EMIT
} BHSpriteOrientedError;

/* Explicit binding for 004569b0 and its owning geometry producer 00455f70.
   path[0] is the first node, used is the original mutable pool count, and
   capacity includes room for every near-plane intersection. All bindings
   are borrowed through the call. Emission consumes each temporary polygon
   synchronously; neither ring nor context scratch may be retained. */
typedef struct BHSpriteOrientedContext
{
    BHSpriteOrientedVertex *path;
    unsigned long used, capacity;
    float near_z, projection_scale;
    BHAttributeClipContext *clip;
    BHTextureImage *image;
    unsigned long color;
    BHModelPolygonEmit emit;
    void *emit_user;
    unsigned long paths_emitted, polygons_emitted;
    BHSpriteOrientedError error;
} BHSpriteOrientedContext;

/* Source owner of original 00455f70. Head is a valid forward chain in path;
   its nodes and all temporary polygons are mutated during this call.
   Zero projected edges retain zero direction. A zero miter denominator still
   performs the masked division, then unordered comparison selects original
   float -0.8 clamp, keeping finite corners and original invalid sticky flag.
   The caller's required FP profile masks traps; sticky status is not cleared. */
int __cdecl bh_sprite_oriented_ribbon(BHSpriteOrientedContext *, BHSpriteOrientedVertex *head);
/* Complete original 004569b0 near-plane clipping and chain dispatch. */
int __cdecl bh_sprite_oriented_draw(BHSpriteOrientedContext *);

#endif
