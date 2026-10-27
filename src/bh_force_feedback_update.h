#ifndef BH_FORCE_FEEDBACK_UPDATE_H
#define BH_FORCE_FEEDBACK_UPDATE_H
#include "bh_force_feedback.h"
typedef struct BHForceFeedbackUpdate
{
    BHForceFeedback *force;
    unsigned long last_tick, calls;
    float x, y;
    HRESULT result;
} BHForceFeedbackUpdate;
typedef struct BHForceFeedbackParameters
{
    DIEFFECT effect;
    LONG direction[2];
    DICONSTANTFORCE magnitude;
} BHForceFeedbackParameters;
/* Count-zero arithmetic/descriptor adapter used by the complete owner below.
   1 ready and history updated; 0 throttled/disabled; -1 invalid binding/input.
   Parameters own the pointers within their effect and must not be copied. */
int __cdecl bh_force_feedback_prepare(BHForceFeedbackUpdate *, unsigned long tick, float x, float y,
                                      BHForceFeedbackParameters *);
/* Whole00471420. Uses actual SetParameters with original0x20000140 flags;
   requires a live effect whenever enabled. History commits BEFORE the native
   call, including native failure. No Acquire, Start or device polling. */
int __cdecl bh_force_feedback_update(BHForceFeedbackUpdate *, unsigned long tick, float x, float y);
#endif
