#ifndef BH_EFFECT_DRAW_H
#define BH_EFFECT_DRAW_H
#include "bh_flash_flare.h"
#include "bh_draw_submit.h"
typedef struct BHEffectDrawContext
{
    BHDrawQueue *queue;
    unsigned long directional_model, fixed_model, timed_texture, random_texture, fixed_texture;
} BHEffectDrawContext;
/* Whole00403670/00403720/00403800, explicit borrowed original resource globals.
   Entity fields remain read-only: phase+e8, roll+f0, velocity+f8, optional model
   +148, fixed angles+3c and actual world position+0c. Context/queue/resources
   outlive consumption. PC53 nearest/masked is required. Scalar diagnostics
   return1 queued,0 partial/full,-1 invalid; later producers still run after
   exhaustion as original. Variable draw always consumes exactly two CRT rand
   values, including absent model/full queue. No constructor/tick admission. */
int __cdecl bh_effect_directional_draw(BHEffectDrawContext *, const BHEntity *);
int __cdecl bh_effect_variable_draw(BHEffectDrawContext *, const BHEntity *);
int __cdecl bh_effect_fixed_draw(BHEffectDrawContext *, const BHEntity *);
#endif
