#ifndef BH_RENDER_STARTUP_H
#define BH_RENDER_STARTUP_H
#include "bh_software_tables.h"
/* Source-owned renderer lifetime, corresponding to the original global
   renderer/device/depth/render bindings. Zero initialize first.
   Startup does not install the still missing software raster kernels. */
typedef struct BHRenderStartup
{
    BHRenderer *renderer;
    BHRenderDevice *device;
    BHRenderSurface *depth, *render, *screen;
    BHRenderPalette *palette;
    BHTextureContext textures;
    BHSoftwareTables tables;
    DDPIXELFORMAT pixel;
    unsigned long width, height, bits, bytes_per_pixel, owns_render, software;
    float extent_x, extent_y, clip_x, clip_y, scale_x, scale_y, offset_x, offset_y;
    HRESULT last_hresult;
    /* Count-zero source lifetime bookkeeping. Each successful depth attach
       owns a target reference even before hardware startup commits. */
    BHRenderSurface *depth_attachments[2];
    unsigned long depth_attachment_count, detach_failures;
    HRESULT last_detach_hresult;
} BHRenderStartup;
/* Complete0045bff0/0045c540. draw/surfaces are native renderer resources.
   Source owner retains them until dispose. software render may be NULL;
   fallback depth16 and offscreen surfaces preserve the original caps.
   No live owner reuse; dispose before retry. Native errors retain HRESULT.
   Depth attachment bookkeeping and palette setup are best effort,
   as in the original; format/table failures are surfaced as source guards. */
int bh_render_startup_hardware(BHRenderStartup *, BHRenderer *, BHRenderSurface *render, BHRenderSurface *screen,
                               unsigned long width, unsigned long height, unsigned long bits);
int bh_render_startup_software(BHRenderStartup *, BHRenderer *, BHRenderSurface *format_surface,
                               BHRenderSurface *render, BHRenderSurface *screen, unsigned long width,
                               unsigned long height, unsigned long bits);
/* Count-zero attachment adapter, also used by original hardware startup.
   Records successful depth bindings for ordered resource teardown.
   Does not alter startup's last_hresult or best-effort call ordering. */
HRESULT bh_render_startup_attach_depth(BHRenderStartup *, BHRenderSurface *);
/* Detaches source-added depth attachments before releasing depth/target ownership. If a
   native detach fails, retains remaining ownership for caller retry; inspect
   depth_attachment_count/last_detach_hresult/detach_failures. Caller must not
   externally replace attachments while this owner is live. */
void bh_render_startup_dispose(BHRenderStartup *);
/* Create logical render surfaces. SDL platform code owns the physical
   window/display mode. Successful resources belong to this owner. */
typedef struct BHRenderMode
{
    BHRenderSurface *screen, *render;
    BHRenderStartup engine;
    char errors[8192];
    HRESULT last_hresult;
} BHRenderMode;
int bh_render_mode_startup(BHRenderMode *, BHRenderer *, unsigned long width, unsigned long height, unsigned long bits,
                           int software);
void bh_render_mode_dispose(BHRenderMode *);
#endif
