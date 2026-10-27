#include "bh_input_enum.h"

/* Exact original0048a410 and SDK IID_IDirectInputDevice2A; the closely named
   DeviceA IID ends in680 rather than this required682. Immutable IID data is
   local so source integration does not require a process-global GUID owner. */
static const GUID device2a_iid = {0x5944e682, 0xc92e, 0x11cf, {0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0, 0}};
BOOL CALLBACK bh_input_enum_callback(LPCDIDEVICEINSTANCEA instance, LPVOID userdata)
{
    BHForceFeedback *owner = (BHForceFeedback *)userdata;
    LPDIRECTINPUTDEVICEA temporary = NULL;
    LPDIRECTINPUTDEVICE2A queried = NULL;
    HRESULT result;
    if (!instance || !owner || !owner->input || owner->device)
        return DIENUM_CONTINUE;
    result = IDirectInput_CreateDevice(owner->input, &instance->guidInstance, &temporary, NULL);
    if (FAILED(result))
        return DIENUM_CONTINUE;
    /* A successful COM creation supplies a valid nonNULL interface. Retain a
       source guard for a violated native contract without dereferencing NULL. */
    if (!temporary)
        return DIENUM_CONTINUE;
    result = IDirectInputDevice_QueryInterface(temporary, &device2a_iid, (void **)&queried);
    IDirectInputDevice_Release(temporary);
    if (FAILED(result) || !queried)
        return DIENUM_CONTINUE;
    owner->device = queried;
    return DIENUM_STOP;
}
