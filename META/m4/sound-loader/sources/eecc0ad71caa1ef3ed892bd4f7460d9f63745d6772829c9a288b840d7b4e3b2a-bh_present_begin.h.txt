#ifndef BH_PRESENT_BEGIN_H
#define BH_PRESENT_BEGIN_H
#include "bh_present.h"
/* Complete0045cdf0. All surfaces, texture slots and renderer are borrowed.
   Software color mapping is published through present->fade; auxiliary is
   retained here. A failed second lock unlocks color and does not set locked. */
typedef struct BHPresentBegin {
    BHPresentContext *present;
    unsigned char *auxiliary_pixels;
    long auxiliary_pitch;
    unsigned long restores,invalidations,native_calls,failures;
    HRESULT last_hresult;
} BHPresentBegin;
int __cdecl bh_present_begin(BHPresentBegin *);
#endif
