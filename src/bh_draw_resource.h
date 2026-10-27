#ifndef BH_DRAW_RESOURCE_H
#define BH_DRAW_RESOURCE_H
#include "bh_model_instance.h"
#include "bh_sprite_billboard.h"
#include "bh_sprite_oriented.h"

/* Source-side descriptor for 0045adf0. The original 32-bit records are
   discriminated by word zero; pointers below are native borrowed bindings,
   not a claim that this structure is an on-disk or original x86 layout. */
typedef struct BHDrawResource BHDrawResource;
typedef enum BHDrawResourceTag
{
    BH_RESOURCE_TAG_MODEL = 0,
    BH_RESOURCE_TAG_LOD = 1,
    BH_RESOURCE_TAG_BILLBOARD = 2,
    BH_RESOURCE_TAG_RIBBON = 3
} BHDrawResourceTag;
struct BHDrawResource
{
    BHDrawResourceTag tag;
    union {
        struct
        {
            BHModelInstanceContext *instance;
        } model;
        struct
        {
            float radius;
            unsigned long count;
            float thresholds[5];
            const BHDrawResource *children[5];
        } lod;
        struct
        {
            BHTextureImage *image;
            float width, height, angle;
            unsigned long flags;
        } billboard;
        struct
        {
            BHVec3 local_end;
            float start_width, end_width;
            unsigned long flags;
            BHTextureImage *image;
        } ribbon;
    } data;
};

typedef enum BHDrawResourceError
{
    BH_DRAW_RESOURCE_OK,
    BH_DRAW_RESOURCE_ARGUMENT,
    BH_DRAW_RESOURCE_TOPOLOGY,
    BH_DRAW_RESOURCE_CAPACITY,
    BH_DRAW_RESOURCE_MODEL,
    BH_DRAW_RESOURCE_BILLBOARD,
    BH_DRAW_RESOURCE_RIBBON
} BHDrawResourceError;
/* Camera, sprite contexts and their output/storage are borrowed for each
   synchronous call. The oriented path must have room for two appended nodes. */
typedef struct BHDrawResourceContext
{
    const BHMainCamera *camera;
    const BHViewConfig *config;
    BHSpriteBillboardContext *billboard;
    BHSpriteOrientedContext *oriented;
    unsigned long dispatched, culled;
    BHDrawResourceError error;
    int failed_model_error, failed_clip_error;
} BHDrawResourceContext;

int __cdecl bh_draw_resource(BHDrawResourceContext *, const BHDrawResource *, double x, double y, double z,
                             double rotate_y, double rotate_x, double rotate_z, float modulation);
#endif
