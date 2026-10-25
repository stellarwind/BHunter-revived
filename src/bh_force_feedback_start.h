#ifndef BH_FORCE_FEEDBACK_START_H
#define BH_FORCE_FEEDBACK_START_H
#include "bh_force_feedback.h"
typedef struct BHForceFeedbackStart {
 BHForceFeedback *force; /* borrowed original owner */
 unsigned long started; /* original004edef0, retained even on native failure */
 unsigned long acquires,starts,failures;
 HRESULT acquire_result,start_result;
} BHForceFeedbackStart;
/* Complete00471220: enabled && !started; optional device Acquire, optional
   effect Start(1,0), then started=1 regardless of native HRESULT failures. */
int __cdecl bh_force_feedback_start(BHForceFeedbackStart *);
#endif
