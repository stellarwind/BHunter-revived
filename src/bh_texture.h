#ifndef BH_TEXTURE_H
#define BH_TEXTURE_H
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "bh_pcx.h"

typedef struct BHTextureImage BHTextureImage;
typedef struct BHTextureSlot BHTextureSlot;
typedef struct BHTextureFormat {
    unsigned long width,height,requested_depth,kind;
    unsigned long minimum,references,count;
    BHTextureSlot *slots;
    DDPIXELFORMAT pixel;
} BHTextureFormat;
struct BHTextureSlot {
    LPDIRECTDRAWSURFACE surface;
    LPDIRECTDRAWPALETTE palette;
    D3DTEXTUREHANDLE handle;
    BHTextureImage *image;
    long frame;
    unsigned short pixels_dirty,palette_dirty;
    BHTextureSlot *next;
};
/* Recovered x86 0x70 image/cache object, with defined zero initial values for
   otherwise unspecified padding. Native interface pointers remain real. */
struct BHTextureImage {
    BHTextureSlot *resident;
    LPDIRECTDRAWSURFACE source;
    LPDIRECTDRAWPALETTE palette;
    BHTextureFormat *format;
    BHTextureImage *next;
    unsigned char *dither;
    unsigned short *screen;
    char name[28];
    unsigned long vertex_count,vertex_capacity,run_count,run_serial;
    unsigned long *runs;
    D3DTLVERTEX *vertices;
    unsigned long flags,unused54,padding[6];
};
typedef struct BHTextureTables {
    unsigned long initialized,quant[4][256],right[3][256],left[3][256],levels[4][8];
} BHTextureTables;
typedef enum BHTextureError {BH_TEXTURE_OK,BH_TEXTURE_ARGUMENT,BH_TEXTURE_MEMORY,
    BH_TEXTURE_FORMAT,BH_TEXTURE_BACKEND,BH_TEXTURE_TOPOLOGY,BH_TEXTURE_IO} BHTextureError;
typedef struct BHTextureContext {
    BHTextureFormat formats[9];
    LPDIRECTDRAW draw;LPDIRECT3DDEVICE2 device;
    BHTextureImage *images;
    BHSceneAllocator allocator;
    unsigned long software,slot_caps_override;
    long frame;
    BHTextureError error;HRESULT last_hresult;
    unsigned long evictions,flushes,pixel_uploads,palette_uploads;
    char prefix[256],errors[8192];long search;
    int palette_mode;unsigned long palette_color;
    unsigned long display_right[3],display_left[3];
    BHTextureTables tables;
} BHTextureContext;
/* New explicit lifetime binding, AddRefs real native interfaces. Original nine
   class dimensions/minima are pinned data; format enumeration is explicit. */
int __cdecl bh_texture_context_init(BHTextureContext *context,LPDIRECTDRAW draw,
    LPDIRECT3DDEVICE2 device,const BHSceneAllocator *allocator);
/* Explicit original software branch: owns a draw reference and format/arena
   state without a fabricated native device. */
int __cdecl bh_texture_context_init_software(BHTextureContext *context,
    LPDIRECTDRAW draw,const BHSceneAllocator *allocator);
void __cdecl bh_texture_context_dispose(BHTextureContext *context);
HRESULT CALLBACK bh_texture_format_callback(LPDDSURFACEDESC desc,LPVOID format); /* 0044e490 */
int __cdecl bh_texture_formats(BHTextureContext *context); /* 0044e510 */
BHTextureFormat *__cdecl bh_texture_request_format(BHTextureContext *context,
    unsigned long width,unsigned long height,unsigned long depth,unsigned long kind); /* 0044e580 */
int __cdecl bh_texture_slot_create(BHTextureContext *context,BHTextureFormat *format); /* 0044e5f0 */
/* Original mandatory/best-effort two passes; inspect context.error separately
   for retained allocation/native failures on its best-effort pass. */
int __cdecl bh_texture_preallocate(BHTextureContext *context); /* 0044e780 */
D3DTEXTUREHANDLE __cdecl bh_texture_resident(BHTextureContext *context,BHTextureImage *image); /* 004505f0 */
/* Explicit diagnostic staging adapters. Source decoded pixels/palette -> real
   owning native image; no full 0044f550 cache/queue/error-string claim. Returns
   borrowed cache identity until context disposal; first name match wins.
   Palette mode/color use recovered 0044f1c0. Unsupported native formats fail. */
BHTextureImage *__cdecl bh_texture_stage(BHTextureContext *context,const char *name,
    const BHPcx *image,int mode,unsigned long color);
BHTextureImage *__cdecl bh_texture_load_pcx(BHTextureContext *context,const char *name,
    const char *prefix,int mode,unsigned long color);
/* Complete 0044e880 over explicit context. A nonempty path gains trailing '\\'.
   Empty prefix or out-of-range policy chooses bare-only (-1). */
int __cdecl bh_texture_set_path(BHTextureContext *context,const char *prefix,long policy);
/* Complete 0044e9b0; explicit display shifts replace configured globals. Like
   the original, initialization latches once and does not follow later changes. */
int __cdecl bh_texture_tables_init(BHTextureContext *context);
/* Complete 0044eb60. palette_bytes <=768 selects RGB triples, >768 RGBA; input
   must contain 256 entries at that stride. Native mode/nonindexed images do
   nothing, as original. Software outputs are owned by the image. */
int __cdecl bh_texture_palette_tables(BHTextureContext *context,BHTextureImage *image,
    const unsigned char *palette,unsigned long palette_bytes);
/* Complete 0044f550. Cache aliases are independent of current palette/path.
   Real PCX streaming reader dialect, native surfaces and source draw arenas.
   Source guards reject malformed/overlong input and clean failed allocations.
   Returned cache identity remains borrowed until context disposal. */
BHTextureImage *__cdecl bh_texture_load(BHTextureContext *context,const char *name);
/* Platform binding from an actual display format to original bit shifts.
   Must be set before initializing software tables. */
int __cdecl bh_texture_bind_display(BHTextureContext *context,const DDPIXELFORMAT *pixel);
#endif
