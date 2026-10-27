#ifndef BH_CAPTURE_H
#define BH_CAPTURE_H
#include "bh_present.h"
/* Explicit bindings for0045b810 software BMP writer and0045bc40 native owner.
   Coordinates truncate independently toward zero. Original packed BMP output
   deliberately retains its fixed640x480 size fields and omits row padding.
   pixels/pitch come from software frame begin; native uses actual color Lock.
   Format arrays are B,G,R masks, extraction shifts and expansion shifts. */
typedef struct BHCapture
{
    BHPresentContext *present;
    float left, top, right, bottom;
    unsigned long pixel_bytes, masks[3], extract[3], expand[3];
    unsigned long writes, rows, native_calls, failures;
    HRESULT last_hresult;
} BHCapture;
int __cdecl bh_capture_software(BHCapture *, const char *);
int __cdecl bh_capture_write(BHCapture *, const char *);
#endif
