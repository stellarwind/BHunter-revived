/* Internal SDL GPU adapter. Legacy structs are retained only as data ABI. */
#ifndef BH_SDL_ADAPTER_H
#define BH_SDL_ADAPTER_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d.h>
#include <ddraw.h>
#include <windows.h>
#ifdef __cplusplus
extern "C"
{
#endif
    HRESULT __cdecl bh_IDirect3D2_CreateDevice(void *self, const GUID *rclsid, struct IDirectDrawSurface *surface,
                                               struct IDirect3DDevice2 **device);
#undef IDirect3D2_CreateDevice
#define IDirect3D2_CreateDevice bh_IDirect3D2_CreateDevice
    HRESULT __cdecl bh_IDirect3D2_CreateMaterial(void *self, struct IDirect3DMaterial2 **material,
                                                 IUnknown *outer);
#undef IDirect3D2_CreateMaterial
#define IDirect3D2_CreateMaterial bh_IDirect3D2_CreateMaterial
    HRESULT __cdecl bh_IDirect3D2_CreateViewport(void *self, struct IDirect3DViewport2 **viewport,
                                                 IUnknown *outer);
#undef IDirect3D2_CreateViewport
#define IDirect3D2_CreateViewport bh_IDirect3D2_CreateViewport
    HRESULT __cdecl bh_IDirect3D2_EnumDevices(void *self, LPD3DENUMDEVICESCALLBACK cb, void *ctx);
#undef IDirect3D2_EnumDevices
#define IDirect3D2_EnumDevices bh_IDirect3D2_EnumDevices
    ULONG __cdecl bh_IDirect3D2_Release(void *self);
#undef IDirect3D2_Release
#define IDirect3D2_Release bh_IDirect3D2_Release
    ULONG __cdecl bh_IDirect3DDevice2_AddRef(void *self);
#undef IDirect3DDevice2_AddRef
#define IDirect3DDevice2_AddRef bh_IDirect3DDevice2_AddRef
    HRESULT __cdecl bh_IDirect3DDevice2_AddViewport(void *self, struct IDirect3DViewport2 *viewport);
#undef IDirect3DDevice2_AddViewport
#define IDirect3DDevice2_AddViewport bh_IDirect3DDevice2_AddViewport
    HRESULT __cdecl bh_IDirect3DDevice2_BeginScene(void *self);
#undef IDirect3DDevice2_BeginScene
#define IDirect3DDevice2_BeginScene bh_IDirect3DDevice2_BeginScene
    HRESULT __cdecl bh_IDirect3DDevice2_DeleteViewport(void *self, struct IDirect3DViewport2 *viewport);
#undef IDirect3DDevice2_DeleteViewport
#define IDirect3DDevice2_DeleteViewport bh_IDirect3DDevice2_DeleteViewport
    HRESULT __cdecl bh_IDirect3DDevice2_DrawPrimitive(void *self, D3DPRIMITIVETYPE primitive_type,
                                                      D3DVERTEXTYPE vertex_type, void *vertices,
                                                      DWORD vertex_count, DWORD flags);
#undef IDirect3DDevice2_DrawPrimitive
#define IDirect3DDevice2_DrawPrimitive bh_IDirect3DDevice2_DrawPrimitive
    HRESULT __cdecl bh_IDirect3DDevice2_EndScene(void *self);
#undef IDirect3DDevice2_EndScene
#define IDirect3DDevice2_EndScene bh_IDirect3DDevice2_EndScene
    HRESULT __cdecl bh_IDirect3DDevice2_EnumTextureFormats(void *self, LPD3DENUMTEXTUREFORMATSCALLBACK cb,
                                                           void *ctx);
#undef IDirect3DDevice2_EnumTextureFormats
#define IDirect3DDevice2_EnumTextureFormats bh_IDirect3DDevice2_EnumTextureFormats
    ULONG __cdecl bh_IDirect3DDevice2_Release(void *self);
#undef IDirect3DDevice2_Release
#define IDirect3DDevice2_Release bh_IDirect3DDevice2_Release
    HRESULT __cdecl bh_IDirect3DDevice2_SetCurrentViewport(void *self, struct IDirect3DViewport2 *viewport);
#undef IDirect3DDevice2_SetCurrentViewport
#define IDirect3DDevice2_SetCurrentViewport bh_IDirect3DDevice2_SetCurrentViewport
    HRESULT __cdecl bh_IDirect3DDevice2_SetLightState(void *self, D3DLIGHTSTATETYPE dwLightStateType,
                                                      DWORD dwLightState);
#undef IDirect3DDevice2_SetLightState
#define IDirect3DDevice2_SetLightState bh_IDirect3DDevice2_SetLightState
    HRESULT __cdecl bh_IDirect3DDevice2_SetRenderState(void *self, D3DRENDERSTATETYPE dwRenderStateType,
                                                       DWORD dwRenderState);
#undef IDirect3DDevice2_SetRenderState
#define IDirect3DDevice2_SetRenderState bh_IDirect3DDevice2_SetRenderState
    HRESULT __cdecl bh_IDirect3DMaterial2_GetHandle(void *self, struct IDirect3DDevice2 *device,
                                                    D3DMATERIALHANDLE *handle);
#undef IDirect3DMaterial2_GetHandle
#define IDirect3DMaterial2_GetHandle bh_IDirect3DMaterial2_GetHandle
    ULONG __cdecl bh_IDirect3DMaterial2_Release(void *self);
#undef IDirect3DMaterial2_Release
#define IDirect3DMaterial2_Release bh_IDirect3DMaterial2_Release
    HRESULT __cdecl bh_IDirect3DMaterial2_SetMaterial(void *self, D3DMATERIAL *data);
#undef IDirect3DMaterial2_SetMaterial
#define IDirect3DMaterial2_SetMaterial bh_IDirect3DMaterial2_SetMaterial
    HRESULT __cdecl bh_IDirect3DTexture2_GetHandle(void *self, struct IDirect3DDevice2 *device,
                                                   D3DTEXTUREHANDLE *handle);
#undef IDirect3DTexture2_GetHandle
#define IDirect3DTexture2_GetHandle bh_IDirect3DTexture2_GetHandle
    HRESULT __cdecl bh_IDirect3DTexture2_PaletteChanged(void *self, DWORD dwStart, DWORD dwCount);
#undef IDirect3DTexture2_PaletteChanged
#define IDirect3DTexture2_PaletteChanged bh_IDirect3DTexture2_PaletteChanged
    ULONG __cdecl bh_IDirect3DTexture2_Release(void *self);
#undef IDirect3DTexture2_Release
#define IDirect3DTexture2_Release bh_IDirect3DTexture2_Release
    HRESULT __cdecl bh_IDirect3DViewport2_Clear(void *self, DWORD count, D3DRECT *rects, DWORD flags);
#undef IDirect3DViewport2_Clear
#define IDirect3DViewport2_Clear bh_IDirect3DViewport2_Clear
    ULONG __cdecl bh_IDirect3DViewport2_Release(void *self);
#undef IDirect3DViewport2_Release
#define IDirect3DViewport2_Release bh_IDirect3DViewport2_Release
    HRESULT __cdecl bh_IDirect3DViewport2_SetViewport2(void *self, D3DVIEWPORT2 *data);
#undef IDirect3DViewport2_SetViewport2
#define IDirect3DViewport2_SetViewport2 bh_IDirect3DViewport2_SetViewport2
    ULONG __cdecl bh_IDirectDraw2_Release(void *self);
#undef IDirectDraw2_Release
#define IDirectDraw2_Release bh_IDirectDraw2_Release
    HRESULT __cdecl bh_IDirectDraw2_RestoreDisplayMode(void *self);
#undef IDirectDraw2_RestoreDisplayMode
#define IDirectDraw2_RestoreDisplayMode bh_IDirectDraw2_RestoreDisplayMode
    HRESULT __cdecl bh_IDirectDraw2_SetCooperativeLevel(void *self, HWND hWnd, DWORD dwFlags);
#undef IDirectDraw2_SetCooperativeLevel
#define IDirectDraw2_SetCooperativeLevel bh_IDirectDraw2_SetCooperativeLevel
    HRESULT __cdecl bh_IDirectDraw2_SetDisplayMode(void *self, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP,
                                                   DWORD dwRefreshRate, DWORD dwFlags);
#undef IDirectDraw2_SetDisplayMode
#define IDirectDraw2_SetDisplayMode bh_IDirectDraw2_SetDisplayMode
    ULONG __cdecl bh_IDirectDrawClipper_Release(void *self);
#undef IDirectDrawClipper_Release
#define IDirectDrawClipper_Release bh_IDirectDrawClipper_Release
    HRESULT __cdecl bh_IDirectDrawClipper_SetHWnd(void *self, DWORD dwFlags, HWND hWnd);
#undef IDirectDrawClipper_SetHWnd
#define IDirectDrawClipper_SetHWnd bh_IDirectDrawClipper_SetHWnd
    HRESULT __cdecl bh_IDirectDrawPalette_GetEntries(void *self, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries,
                                                     LPPALETTEENTRY lpEntries);
#undef IDirectDrawPalette_GetEntries
#define IDirectDrawPalette_GetEntries bh_IDirectDrawPalette_GetEntries
    ULONG __cdecl bh_IDirectDrawPalette_Release(void *self);
#undef IDirectDrawPalette_Release
#define IDirectDrawPalette_Release bh_IDirectDrawPalette_Release
    HRESULT __cdecl bh_IDirectDrawPalette_SetEntries(void *self, DWORD dwFlags, DWORD dwStartingEntry,
                                                     DWORD dwCount, LPPALETTEENTRY lpEntries);
#undef IDirectDrawPalette_SetEntries
#define IDirectDrawPalette_SetEntries bh_IDirectDrawPalette_SetEntries
    HRESULT __cdecl bh_IDirectDrawSurface_AddAttachedSurface(void *self, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
#undef IDirectDrawSurface_AddAttachedSurface
#define IDirectDrawSurface_AddAttachedSurface bh_IDirectDrawSurface_AddAttachedSurface
    ULONG __cdecl bh_IDirectDrawSurface_AddRef(void *self);
#undef IDirectDrawSurface_AddRef
#define IDirectDrawSurface_AddRef bh_IDirectDrawSurface_AddRef
    HRESULT __cdecl bh_IDirectDrawSurface_Blt(void *self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface,
                                              LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
#undef IDirectDrawSurface_Blt
#define IDirectDrawSurface_Blt bh_IDirectDrawSurface_Blt
    HRESULT __cdecl bh_IDirectDrawSurface_BltFast(void *self, DWORD dwX, DWORD dwY,
                                                  LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect,
                                                  DWORD dwTrans);
#undef IDirectDrawSurface_BltFast
#define IDirectDrawSurface_BltFast bh_IDirectDrawSurface_BltFast
    HRESULT __cdecl bh_IDirectDrawSurface_DeleteAttachedSurface(void *self, DWORD dwFlags,
                                                                LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
#undef IDirectDrawSurface_DeleteAttachedSurface
#define IDirectDrawSurface_DeleteAttachedSurface bh_IDirectDrawSurface_DeleteAttachedSurface
    HRESULT __cdecl bh_IDirectDrawSurface_Flip(void *self, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride,
                                               DWORD dwFlags);
#undef IDirectDrawSurface_Flip
#define IDirectDrawSurface_Flip bh_IDirectDrawSurface_Flip
    HRESULT __cdecl bh_IDirectDrawSurface_GetAttachedSurface(void *self, LPDDSCAPS lpDDSCaps,
                                                             LPDIRECTDRAWSURFACE *lplpDDAttachedSurface);
#undef IDirectDrawSurface_GetAttachedSurface
#define IDirectDrawSurface_GetAttachedSurface bh_IDirectDrawSurface_GetAttachedSurface
    HRESULT __cdecl bh_IDirectDrawSurface_GetDC(void *self, HDC *lphDC);
#undef IDirectDrawSurface_GetDC
#define IDirectDrawSurface_GetDC bh_IDirectDrawSurface_GetDC
    HRESULT __cdecl bh_IDirectDrawSurface_GetPixelFormat(void *self, LPDDPIXELFORMAT lpDDPixelFormat);
#undef IDirectDrawSurface_GetPixelFormat
#define IDirectDrawSurface_GetPixelFormat bh_IDirectDrawSurface_GetPixelFormat
    HRESULT __cdecl bh_IDirectDrawSurface_IsLost(void *self);
#undef IDirectDrawSurface_IsLost
#define IDirectDrawSurface_IsLost bh_IDirectDrawSurface_IsLost
    HRESULT __cdecl bh_IDirectDrawSurface_Lock(void *self, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc,
                                               DWORD dwFlags, HANDLE hEvent);
#undef IDirectDrawSurface_Lock
#define IDirectDrawSurface_Lock bh_IDirectDrawSurface_Lock
    HRESULT __cdecl bh_IDirectDrawSurface_QueryInterface(void *self, const IID *riid, void **ppvObject);
#undef IDirectDrawSurface_QueryInterface
#define IDirectDrawSurface_QueryInterface bh_IDirectDrawSurface_QueryInterface
    ULONG __cdecl bh_IDirectDrawSurface_Release(void *self);
#undef IDirectDrawSurface_Release
#define IDirectDrawSurface_Release bh_IDirectDrawSurface_Release
    HRESULT __cdecl bh_IDirectDrawSurface_ReleaseDC(void *self, HDC hDC);
#undef IDirectDrawSurface_ReleaseDC
#define IDirectDrawSurface_ReleaseDC bh_IDirectDrawSurface_ReleaseDC
    HRESULT __cdecl bh_IDirectDrawSurface_Restore(void *self);
#undef IDirectDrawSurface_Restore
#define IDirectDrawSurface_Restore bh_IDirectDrawSurface_Restore
    HRESULT __cdecl bh_IDirectDrawSurface_SetClipper(void *self, LPDIRECTDRAWCLIPPER lpDDClipper);
#undef IDirectDrawSurface_SetClipper
#define IDirectDrawSurface_SetClipper bh_IDirectDrawSurface_SetClipper
    HRESULT __cdecl bh_IDirectDrawSurface_SetPalette(void *self, LPDIRECTDRAWPALETTE lpDDPalette);
#undef IDirectDrawSurface_SetPalette
#define IDirectDrawSurface_SetPalette bh_IDirectDrawSurface_SetPalette
    HRESULT __cdecl bh_IDirectDrawSurface_Unlock(void *self, LPVOID lpSurfaceData);
#undef IDirectDrawSurface_Unlock
#define IDirectDrawSurface_Unlock bh_IDirectDrawSurface_Unlock
    ULONG __cdecl bh_IDirectDraw_AddRef(void *self);
#undef IDirectDraw_AddRef
#define IDirectDraw_AddRef bh_IDirectDraw_AddRef
    HRESULT __cdecl bh_IDirectDraw_CreateClipper(void *self, DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper,
                                                 IUnknown *pUnkOuter);
#undef IDirectDraw_CreateClipper
#define IDirectDraw_CreateClipper bh_IDirectDraw_CreateClipper
    HRESULT __cdecl bh_IDirectDraw_CreatePalette(void *self, DWORD dwFlags, LPPALETTEENTRY lpColorTable,
                                                 LPDIRECTDRAWPALETTE *lplpDDPalette, IUnknown *pUnkOuter);
#undef IDirectDraw_CreatePalette
#define IDirectDraw_CreatePalette bh_IDirectDraw_CreatePalette
    HRESULT __cdecl bh_IDirectDraw_CreateSurface(void *self, LPDDSURFACEDESC lpDDSurfaceDesc,
                                                 LPDIRECTDRAWSURFACE *lplpDDSurface, IUnknown *pUnkOuter);
#undef IDirectDraw_CreateSurface
#define IDirectDraw_CreateSurface bh_IDirectDraw_CreateSurface
    HRESULT __cdecl bh_IDirectDraw_QueryInterface(void *self, const IID *riid, void **ppvObject);
#undef IDirectDraw_QueryInterface
#define IDirectDraw_QueryInterface bh_IDirectDraw_QueryInterface
    ULONG __cdecl bh_IDirectDraw_Release(void *self);
#undef IDirectDraw_Release
#define IDirectDraw_Release bh_IDirectDraw_Release
    HRESULT __cdecl bh_IDirectDraw_RestoreDisplayMode(void *self);
#undef IDirectDraw_RestoreDisplayMode
#define IDirectDraw_RestoreDisplayMode bh_IDirectDraw_RestoreDisplayMode
    HRESULT __cdecl bh_IDirectDraw_SetCooperativeLevel(void *self, HWND hWnd, DWORD dwFlags);
#undef IDirectDraw_SetCooperativeLevel
#define IDirectDraw_SetCooperativeLevel bh_IDirectDraw_SetCooperativeLevel
    HRESULT __cdecl bh_gpu_create(GUID *, LPDIRECTDRAW *, IUnknown *);
    HRESULT __cdecl bh_gpu_enumerate(LPDDENUMCALLBACKA, void *);
#define DirectDrawCreate bh_gpu_create
#define DirectDrawEnumerateA bh_gpu_enumerate
    typedef void(__cdecl *BHSDLMessage)(HWND, UINT, WPARAM, LPARAM, void *);
    HWND __cdecl bh_sdl_open(int hidden, BHSDLMessage, void *);
    int __cdecl bh_sdl_poll(void);
    double __cdecl bh_sdl_seconds(void);
    void __cdecl bh_sdl_pace(void);
    void __cdecl bh_sdl_close(void);
    const char *__cdecl bh_sdl_error(void);
    void __cdecl bh_sdl_video_cycle(int row);
    const char *__cdecl bh_sdl_video_label(int row);
    int __cdecl bh_sdl_capture(const char *path);
#ifdef __cplusplus
}
#endif
#endif
