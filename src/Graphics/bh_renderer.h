/* Native BH renderer API. SDL owns GPU implementation; the platform owns
   window/events. Original descriptors and vertex/state values are data ABI,
   not COM interfaces. All resource parameters are typed opaque handles. */
#ifndef BH_RENDERER_H
#define BH_RENDERER_H
#include "bh_render_types.h"
#ifdef __cplusplus
extern "C"
{
#endif
    int bh_renderer_surface_text(BHRenderSurface *surface, int x, int y, const char *text, unsigned long rgb);
    HRESULT __cdecl bh_renderer_create(BHRenderer **out);
    HRESULT __cdecl bh_renderer_device_create(BHRenderer *self, BHRenderSurface *surface, BHRenderDevice **device);
    ULONG __cdecl bh_renderer_device_retain(BHRenderDevice *self);
    /* Input vertices are copied in order; portal-clipped fans remain unchanged. */
    HRESULT __cdecl bh_renderer_draw_fan(BHRenderDevice *self, const D3DTLVERTEX *vertices, DWORD vertex_count);
    /* Record the current batch without presenting or synchronizing the GPU. */
    HRESULT __cdecl bh_renderer_record_draws(BHRenderDevice *self);
    HRESULT __cdecl bh_renderer_texture_formats(BHRenderDevice *self, LPD3DENUMTEXTUREFORMATSCALLBACK cb, void *ctx);
    ULONG __cdecl bh_renderer_device_release(BHRenderDevice *self);
    HRESULT __cdecl bh_renderer_set_state(BHRenderDevice *self, D3DRENDERSTATETYPE dwRenderStateType,
                                          DWORD dwRenderState);
    ULONG __cdecl bh_renderer_release(BHRenderer *self);
    ULONG __cdecl bh_renderer_surface_release(BHRenderSurface *self);
    HRESULT __cdecl bh_renderer_surface_texture_handle(BHRenderSurface *self, D3DTEXTUREHANDLE *handle);
    HRESULT __cdecl bh_renderer_palette_get(BHRenderPalette *self, DWORD dwBase, DWORD dwNumEntries,
                                            LPPALETTEENTRY lpEntries);
    ULONG __cdecl bh_renderer_palette_release(BHRenderPalette *self);
    /* Record previous draws before changing the palette and invalidate its textures. */
    HRESULT __cdecl bh_renderer_palette_set(BHRenderPalette *self, DWORD dwStartingEntry, DWORD dwCount,
                                            LPPALETTEENTRY lpEntries);
    HRESULT __cdecl bh_renderer_surface_attach_depth(BHRenderSurface *self, BHRenderSurface *lpDDSAttachedSurface);
    ULONG __cdecl bh_renderer_surface_retain(BHRenderSurface *self);
    HRESULT __cdecl bh_renderer_surface_blit(BHRenderSurface *self, LPRECT lpDestRect, BHRenderSurface *lpDDSrcSurface,
                                             LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
    HRESULT __cdecl bh_renderer_surface_copy(BHRenderSurface *self, DWORD dwX, DWORD dwY,
                                             BHRenderSurface *lpDDSrcSurface, LPRECT lpSrcRect);
    HRESULT __cdecl bh_renderer_surface_detach_depth(BHRenderSurface *self, BHRenderSurface *lpDDSAttachedSurface);
    HRESULT __cdecl bh_renderer_surface_present(BHRenderSurface *self);
    HRESULT __cdecl bh_renderer_surface_backbuffer(BHRenderSurface *self, BHRenderSurface **lplpDDAttachedSurface);
    HRESULT __cdecl bh_renderer_surface_format(BHRenderSurface *self, LPDDPIXELFORMAT lpDDPixelFormat);
    HRESULT __cdecl bh_renderer_surface_status(BHRenderSurface *self);
    /* Target locks read back rendered pixels; unlock uploads only modified targets. */
    HRESULT __cdecl bh_renderer_surface_lock(BHRenderSurface *self, LPDDSURFACEDESC lpDDSurfaceDesc);
    HRESULT __cdecl bh_renderer_surface_restore(BHRenderSurface *self);
    HRESULT __cdecl bh_renderer_surface_set_palette(BHRenderSurface *self, BHRenderPalette *lpDDPalette);
    HRESULT __cdecl bh_renderer_surface_unlock(BHRenderSurface *self);
    ULONG __cdecl bh_renderer_retain(BHRenderer *self);
    HRESULT __cdecl bh_renderer_palette_create(BHRenderer *self, LPPALETTEENTRY lpColorTable,
                                               BHRenderPalette **lplpDDPalette);
    HRESULT __cdecl bh_renderer_surface_create(BHRenderer *self, LPDDSURFACEDESC lpDDSurfaceDesc,
                                               BHRenderSurface **lplpDDSurface);
#ifdef __cplusplus
}
#endif
#endif
