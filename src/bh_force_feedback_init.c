#include "bh_force_feedback_init.h"
#include "bh_input_enum.h"
#include <string.h>
#include <stddef.h>
typedef char original_effect_layout[sizeof(void*)==4&&sizeof(DIEFFECT)==52&&offsetof(DIEFFECT,lpvTypeSpecificParams)==48?1:-1];
typedef char original_effect_flags[(DIEFF_OBJECTOFFSETS|DIEFF_CARTESIAN)==0x12&&DIJOFS_X==0&&DIJOFS_Y==4?1:-1];
int __cdecl bh_force_feedback_init(BHForceFeedbackInit *context,HINSTANCE instance,HWND window){
    static const GUID constant_force={0x13541c20,0x8e33,0x11d0,{0x9a,0xd0,0,0xa0,0xc9,0xa0,0x6e,0x35}};
    BHForceFeedback *force;DWORD axes[2]={DIJOFS_X,DIJOFS_Y};LONG directions[2]={0,0};DICONSTANTFORCE magnitude;DIEFFECT effect;
    if(!context||!context->start||!(force=context->start->force)||force->input||force->device||force->effect||force->enabled)return 0;
    ++context->creates;context->create_result=DirectInputCreateA(instance,0x500,&force->input,NULL);
    if(FAILED(context->create_result))return 0;
    ++context->enumerations;context->enumeration_result=IDirectInput_EnumDevices(force->input,DIDEVTYPE_JOYSTICK,bh_input_enum_callback,force,DIEDFL_ATTACHEDONLY|DIEDFL_FORCEFEEDBACK);
    if(!force->device)return 0;
    ++context->data_formats;context->data_result=IDirectInputDevice2_SetDataFormat(force->device,&c_dfDIJoystick);
    if(FAILED(context->data_result))return 0;
    ++context->cooperative_levels;context->cooperative_result=IDirectInputDevice2_SetCooperativeLevel(force->device,window,DISCL_EXCLUSIVE|DISCL_FOREGROUND);
    if(FAILED(context->cooperative_result))return 0;
    memset(&effect,0,sizeof(effect));magnitude.lMagnitude=0;
    effect.dwSize=52;effect.dwFlags=DIEFF_OBJECTOFFSETS|DIEFF_CARTESIAN;
    effect.dwDuration=INFINITE;effect.dwGain=10000;effect.dwTriggerButton=INFINITE;
    effect.cAxes=2;effect.rgdwAxes=axes;effect.rglDirection=directions;
    effect.cbTypeSpecificParams=4;effect.lpvTypeSpecificParams=&magnitude;
    ++context->effects;context->effect_result=IDirectInputDevice2_CreateEffect(force->device,&constant_force,&effect,&force->effect,NULL);
    if(FAILED(context->effect_result))return 0;
    force->enabled=1;bh_force_feedback_start(context->start);return 1;
}
