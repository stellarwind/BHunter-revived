#ifndef BH_RENDER_STARTUP_H
#define BH_RENDER_STARTUP_H
#include "bh_software_tables.h"
/* Source-owned renderer lifetime, corresponding to the original global
   DirectDraw/D3D2/depth/viewport/render bindings. Zero initialize first.
   Startup does not install the still missing software raster kernels. */
typedef struct BHRenderStartup {
    LPDIRECT3D2 d3d;
    LPDIRECT3DDEVICE2 device;
    LPDIRECT3DVIEWPORT2 viewport;
    LPDIRECTDRAWSURFACE depth,render,screen;
    LPDIRECTDRAWPALETTE palette;
    BHTextureContext textures;
    BHSoftwareTables tables;
    DDPIXELFORMAT pixel;
    unsigned long width,height,bits,bytes_per_pixel,owns_render,software;
    float extent_x,extent_y,clip_x,clip_y,scale_x,scale_y,offset_x,offset_y;
    HRESULT last_hresult;
} BHRenderStartup;
/* Complete0045bfe0: enumeration always continues, all six inputs ignored. */
HRESULT CALLBACK bh_render_device_policy(LPGUID,LPSTR,LPSTR,
    LPD3DDEVICEDESC,LPD3DDEVICEDESC,LPVOID);
/* Complete0045bff0/0045c540. draw/surfaces are actual native interfaces.
   Source owner retains them until dispose. software render may be NULL;
   fallback depth16 and offscreen surfaces preserve the original caps.
   No live owner reuse; dispose before retry. Native errors retain HRESULT.
   Native AddAttachedSurface/AddViewport and palette setup are best effort,
   as in the original; format/table failures are surfaced as source guards. */
int __cdecl bh_render_startup_hardware(BHRenderStartup *,LPDIRECTDRAW,
    LPDIRECTDRAWSURFACE render,LPDIRECTDRAWSURFACE screen,
    unsigned long width,unsigned long height,unsigned long bits);
int __cdecl bh_render_startup_software(BHRenderStartup *,LPDIRECTDRAW,
    LPDIRECTDRAWSURFACE format_surface,LPDIRECTDRAWSURFACE render,
    LPDIRECTDRAWSURFACE screen,unsigned long width,unsigned long height,
    unsigned long bits);
void __cdecl bh_render_startup_dispose(BHRenderStartup *);
/* Complete004715e0 mode/screen/backbuffer family. draw2 supplies actual
   SetDisplayMode(width,height,bits,0,0). Exclusive cooperative mode must be
   established by the real application first. Never calls SetCooperativeLevel.
   Error text appends original category + newline + mode + newline.
   Successful resources belong to this owner until explicit dispose. */
typedef struct BHRenderMode {
    LPDIRECTDRAWSURFACE screen,render;
    BHRenderStartup engine;
    char errors[8192];
    HRESULT last_hresult;
} BHRenderMode;
int __cdecl bh_render_mode_startup(BHRenderMode *,LPDIRECTDRAW2,
    LPDIRECTDRAW,unsigned long width,unsigned long height,unsigned long bits,
    int software);
void __cdecl bh_render_mode_dispose(BHRenderMode *);
#endif
