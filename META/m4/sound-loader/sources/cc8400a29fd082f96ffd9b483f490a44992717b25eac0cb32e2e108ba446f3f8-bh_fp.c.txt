#include "bh_fp.h"
#include <float.h>

static unsigned int environment_mask(void) { return _MCW_PC|_MCW_RC|_MCW_EM; }
static unsigned int environment_required(void) { return _PC_53|_RC_NEAR|_MCW_EM; }

int __cdecl bh_math_environment_supported(void) {
    return (_control87(0,0)&environment_mask())==environment_required();
}

int __cdecl bh_math_environment_enter(BHMathEnvironment *environment) {
    if (!environment) return 0;
    environment->control=_control87(0,0);
    _control87(environment_required(),environment_mask());
    return bh_math_environment_supported();
}

void __cdecl bh_math_environment_leave(const BHMathEnvironment *environment) {
    if (environment) _control87(environment->control,environment_mask());
}
