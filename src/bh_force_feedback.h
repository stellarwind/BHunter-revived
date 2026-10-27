#ifndef BH_FORCE_FEEDBACK_H
#define BH_FORCE_FEEDBACK_H
#include <windows.h>
#include <dinput.h>
/* Source lifetime binding owns exactly the references handed to these fields.
   No constructor, acquisition or force-feedback effect is fabricated. */
typedef struct BHForceFeedback
{
    LPDIRECTINPUTA input;         /* original004edee0 */
    LPDIRECTINPUTEFFECT effect;   /* original004edee4 */
    LPDIRECTINPUTDEVICE2A device; /* original004edee8 */
    unsigned long enabled;        /* original004edeec */
    unsigned long effect_releases, unacquires, device_releases, input_releases;
    HRESULT last_unacquire;
} BHForceFeedback;
/* Complete004713c0: clear enabled, release/null effect, unacquire then reload
   and release/null device, release/null input. Original ignores Unacquire's
   HRESULT, so later releases occur after failure too. NULL context is a source
   guard. Repeated calls are harmless once all owned pointers are cleared. */
void __cdecl bh_force_feedback_stop(BHForceFeedback *);
#endif
