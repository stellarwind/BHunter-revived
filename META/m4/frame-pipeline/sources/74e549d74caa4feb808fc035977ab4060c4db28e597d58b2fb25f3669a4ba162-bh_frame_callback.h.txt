#ifndef BH_FRAME_CALLBACK_H
#define BH_FRAME_CALLBACK_H
#include "bh_render_execute.h"
typedef int (__cdecl *BHFrameRenderCallback)(BHRenderExecute *);
/* Source-context equivalent of original00491880. Register before worker start;
   callback changes during worker execution require external synchronization. */
typedef struct BHFrameCallback {BHFrameRenderCallback render;} BHFrameCallback;
/* Complete00471df0: store caller-provided function identity, including NULL.
   Source signature carries the explicit renderer context used by recovered
   owners instead of the original (0,0,raw-buffer) global ABI. */
void __cdecl bh_frame_callback_set(BHFrameCallback *,BHFrameRenderCallback);
/* Complete00479120: install the actual recovered00479090 owner through the
   setter and return1. NULL source context rejects rather than writing globals. */
int __cdecl bh_frame_callback_start(BHFrameCallback *);
#endif
