#ifndef BH_FORCE_FEEDBACK_INIT_H
#define BH_FORCE_FEEDBACK_INIT_H
#include "bh_force_feedback_start.h"
typedef struct BHForceFeedbackInit
{
    BHForceFeedbackStart *start;
    unsigned long creates, enumerations, data_formats, cooperative_levels, effects;
    HRESULT create_result, enumeration_result, data_result, cooperative_result, effect_result;
} BHForceFeedbackInit;
/* Whole00471260. Original ABI has HINSTANCE and HWND, despite the stale one-arg
   decompiler signature. Explicit source owner adds context. Fresh native owner
   required; partial failure retains native references for actual stop/disposal.
   Successful construction enables feedback and invokes actual recovered start,
   including device Acquire/effect Start when !started. Errors in that final
   start retain original return1. Supplied HWND must be application-owned for
   foreground/exclusive cooperative level. Hardware-positive execution requires
   a legitimate device/window; source checks deliberately refuse physical I/O. */
int __cdecl bh_force_feedback_init(BHForceFeedbackInit *, HINSTANCE, HWND);
#endif
