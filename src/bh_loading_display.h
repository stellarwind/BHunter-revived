#ifndef BH_LOADING_DISPLAY_H
#define BH_LOADING_DISPLAY_H
#include <ddraw.h>
#include "bh_loading_blit.h"
typedef struct BHLoadingDisplay
{
    BHLoadingImages *images;
    LPDIRECTDRAWSURFACE color, software_target, primary, back;
    long width, height;
    int software;
    unsigned long alternate;
    float previous[2], baseline, scale;
    BHLoadingBlit blit;
    unsigned long calls, locks, full_copies, wedge_calls, unlocks, flips, restores, failures;
    float normalized;
    HRESULT lock_result, unlock_result, copy_result, flip_result;
    /* Optional windowed presentation adapter. NULL retains original Flip. */
    HRESULT(__cdecl *present)(LPDIRECTDRAWSURFACE, void *);
    void *present_user;
    unsigned long presentations;
} BHLoadingDisplay;
/* Whole0044aac0. Borrowed real native surfaces; caller owns their lifetime.
   Numeric caller profile is x87 PC53/nearest with masked traps, as required
   by the admitted wedge owners. This function does not change thread control.
   Explicit live normalization/history bindings replace original globals.
   Returns native/guard success, while preserving original mutations preceding
   native failures. Offscreen primary Flip legitimately returns NOTFLIPPABLE.
   Signed native pitch is retained: logical row0 is rebased to an allocation
   span for checked wedges, while full copies advance by the actual signed pitch.
   Source guards reject stage outside0..2, invalid dimensions/overflowing spans,
   a missing primary before native rendering. No original bootstrap is claimed. */
int __cdecl bh_loading_display(BHLoadingDisplay *, float progress);
#endif
