#ifndef BH_INPUT_ENUM_H
#define BH_INPUT_ENUM_H
#include "bh_force_feedback.h"
/* Complete004711b0, native stdcall enumeration ABI (all original RET8 paths).
   Userdata is a borrowed actual BHForceFeedback owner with a live input root
   and empty device field. On success it owns the queried Device2A reference;
   bh_force_feedback_stop releases it. The temporary base interface is always
   released after QI. No device is acquired, polled, configured or played.
   Invalid instance/owner/root or occupied device are guarded CONTINUE paths,
   preserving existing references without issuing native calls. Owner and root
   must outlive enumeration; callback does not retain the instance/userdata. */
BOOL CALLBACK bh_input_enum_callback(LPCDIDEVICEINSTANCEA instance, LPVOID userdata);
#endif
