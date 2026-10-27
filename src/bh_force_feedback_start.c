#include "bh_force_feedback_start.h"
int __cdecl bh_force_feedback_start(BHForceFeedbackStart *c)
{
    int ok = 1;
    BHForceFeedback *f;
    if (!c || !(f = c->force))
        return 0;
    if (!f->enabled || c->started)
        return 1;
    if (f->device)
    {
        ++c->acquires;
        c->acquire_result = IDirectInputDevice2_Acquire(f->device);
        if (FAILED(c->acquire_result))
        {
            ++c->failures;
            ok = 0;
        }
    }
    if (f->effect)
    {
        ++c->starts;
        c->start_result = IDirectInputEffect_Start(f->effect, 1, 0);
        if (FAILED(c->start_result))
        {
            ++c->failures;
            ok = 0;
        }
    }
    c->started = 1;
    return ok;
}
