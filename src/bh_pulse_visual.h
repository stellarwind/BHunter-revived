#ifndef BH_PULSE_VISUAL_H
#define BH_PULSE_VISUAL_H
#include "bh_draw_submit.h"
#include "bh_entities.h"
#include "bh_flash_flare.h"
typedef struct BHPulseVisualContext
{
    BHDrawQueue *queue;
    unsigned long model, image, attempted, queued;
    int error;
} BHPulseVisualContext;
/*Whole0043e840. Borrowed draw queue and registered resource/image handles.
  No object execution or backend calls. PC53 nearest, finite positive lifetime.*/
int __cdecl bh_pulse_visual_draw(BHPulseVisualContext *, const BHEntity *);
#endif
