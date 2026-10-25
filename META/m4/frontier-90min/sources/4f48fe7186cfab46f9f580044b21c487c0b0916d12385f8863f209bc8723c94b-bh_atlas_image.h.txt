#ifndef BH_ATLAS_IMAGE_H
#define BH_ATLAS_IMAGE_H
#include "bh_texture.h"
typedef struct BHAtlasRectangle {unsigned short x,y,width,height;} BHAtlasRectangle;
/* Original x86 0x13ac cached atlas record; no guessed vtable/extra fields. */
typedef struct BHAtlasImage BHAtlasImage;
struct BHAtlasImage {
    BHAtlasImage *next;
    char name[128];
    unsigned char palette[768];
    DDPIXELFORMAT pixel;
    unsigned long rectangle_count;
    BHAtlasRectangle rectangles[512];
    LPDIRECTDRAWSURFACE surface;
};
typedef struct BHAtlasContext {
    BHTextureContext *textures; /* borrowed real native draw/allocator/palette policy */
    BHAtlasImage *images;
    unsigned long scan_step; /* original004e9750, independent of decoded row pitch */
    BHTextureError error;HRESULT last_hresult;
} BHAtlasContext;
/* New binding adapter. Zero initialize context before first use. */
int __cdecl bh_atlas_context_init(BHAtlasContext *,BHTextureContext *,unsigned long scan_step);
/* Complete0046f490. Literal case-insensitive full-name cache: no extension
   rewrite, prefix or texture search policy. Original first-pixel index is
   transparent for rectangle extraction after current palette transformation.
   Existing cache identity wins over requested format/current palette policy.
   Reads real VFILE/PCX reader and creates/locks actual native offscreen surfaces
   in8/16/24/32bit formats. Source guards reject malformed input, unsafe vertical
   scan reads, >512rectangles, failed allocations/native calls transactionally.
   RGB masks containing bit31 explicitly reject: the original signed mask loop
   differs there and this real native backend rejects that surface profile. */
BHAtlasImage *__cdecl bh_atlas_image_load(BHAtlasContext *,const char *,const DDPIXELFORMAT *);
/* Source owner release adapter, no original destructor claim. Dispose before
   releasing the borrowed texture context and its real native draw. */
void __cdecl bh_atlas_context_dispose(BHAtlasContext *);
#endif
