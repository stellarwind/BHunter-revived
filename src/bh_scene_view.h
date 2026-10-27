#ifndef BH_SCENE_VIEW_H
#define BH_SCENE_VIEW_H
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "bh_math.h"
#include "bh_view_camera.h"
/* Diagnostic platform adapter, no original function-recovery claim. Owns a
   private offscreen RGB Direct3D2 target and z-buffer; no game/window/input
   loop. World/script ownership remains separate restoration work. */
typedef struct BHSceneView
{
    LPDIRECTDRAW draw;
    LPDIRECT3D2 d3d;
    LPDIRECT3DDEVICE2 device;
    LPDIRECTDRAWSURFACE color, depth;
    LPDIRECT3DVIEWPORT2 viewport;
    unsigned long width, height, triangles, pixels;
    HRESULT error;
    int begun;
    BHViewConfig projection;
    /* Explicit target-clear/TL binding: zero is the analytic control profile;
       one uses reviewed original near/z depth with a zero-cleared Z target.
   Original native material/state owners must set GREATEREQUAL separately.
       This profile also retains original positive camera Y->screen Y. */
    unsigned long original_depth;
} BHSceneView;
int __cdecl bh_scene_view_init(BHSceneView *view, unsigned long width, unsigned long height);
void __cdecl bh_scene_view_dispose(BHSceneView *view);
int __cdecl bh_scene_view_begin(BHSceneView *view);
/* Camera-space convex polygon; explicit diagnostic 90-degree horizontal FOV,
   positive Z forward, +Y up, near .25, far400. Recovered00452140 performs
   depth clipping, division and screen-edge clipping; native RGB rasterizes
   and depth-tests triangles. Recovered00451b80 builds projection/frustum.
   original_depth selects the reviewed native near/z conversion; the complete
   software/native primitive dispatcher remains unfinished.
   No original BSP traversal, occlusion, lighting or material behavior claimed. */
int __cdecl bh_scene_view_polygon(BHSceneView *view, const BHVec3 *vertices, unsigned long count, unsigned long rgb);
struct BHTextureContext;
struct BHTextureImage;
/* Diagnostic polygon submission over real recovered texture residency.
   UV pairs are the raw BSP edge floats; clip/interpolate before projection. */
int __cdecl bh_scene_view_textured_polygon(BHSceneView *view, const BHVec3 *vertices, const float *uv,
                                           unsigned long count, struct BHTextureContext *textures,
                                           struct BHTextureImage *image);
int __cdecl bh_scene_view_end(BHSceneView *view, const char *bmp);
#endif
