#include "bh_force_feedback.h"
void __cdecl bh_force_feedback_stop(BHForceFeedback *c){
    if(!c)return;
    c->enabled=0;
    if(c->effect){++c->effect_releases;IDirectInputEffect_Release(c->effect);c->effect=NULL;}
    if(c->device){
        ++c->unacquires;c->last_unacquire=IDirectInputDevice2_Unacquire(c->device);
        ++c->device_releases;IDirectInputDevice2_Release(c->device);c->device=NULL;
    }
    if(c->input){++c->input_releases;IDirectInput_Release(c->input);c->input=NULL;}
}
