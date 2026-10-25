#ifndef BH_FRAME_RENDER_H
#define BH_FRAME_RENDER_H
#include "bh_texture_draw.h"
#include "bh_atlas_image.h"

typedef enum BHFrameRenderError {
    BH_FRAME_RENDER_OK,BH_FRAME_RENDER_ARGUMENT,BH_FRAME_RENDER_BINDING,
    BH_FRAME_RENDER_TOPOLOGY,BH_FRAME_RENDER_BOUNDS,BH_FRAME_RENDER_NATIVE
} BHFrameRenderError;
typedef struct BHFrameDisplay {
    BHTextureContext *textures;       /* original 004edc70 / 004ede80 / device */
    BHAtlasContext *atlas;            /* optional shared 004e9750 scan step */
    LPDIRECTDRAWSURFACE auxiliary,color; /* original 004e9c48 / 004e9c3c */
    unsigned char *auxiliary_pixels,*color_pixels; /* original locked pointers */
    unsigned long auxiliary_bytes;    /* source guard for locked depth clear */
    long auxiliary_pitch,color_pitch;
    float left,top;                   /* original 004ede9c / 004edea0 */
    unsigned long locked,indexed_color; /* original 004e9c50 / 004e38c4 */
    unsigned long native_calls,flushes,failures,sleeps;
    HRESULT last_hresult;
    BHFrameRenderError error;
} BHFrameDisplay;
/* Complete 0045ca10 and 0045cbc0 respectively. Both use the original fixed
   640x480 right/bottom and caller's x87 rounding profile. Native mode flushes
   each live texture image, EndScene/BeginScene, then depth-fills auxiliary.
   Software mode color-fills actual color surface; when locked it Unlocks,
   retries Lock at most500 times with Sleep(1), updates live color pointer/pitch
   and atlas scan step, then clears the 16-bit auxiliary pixels. Original native
   errors do not suppress subsequent calls; return0 records them after effects.
   Explicit borrowed surfaces/pixels replace globals. Bounds/list validity and
   RGB values 0..255 are source adapter guards; this callback only supplies 255.
   No arbitrary invalid pointer or signed-overflow behavior is recovered. */
int __cdecl bh_frame_clear_depth(BHFrameDisplay *);
int __cdecl bh_frame_clear_color(BHFrameDisplay *,long red,long green,long blue);
#endif
