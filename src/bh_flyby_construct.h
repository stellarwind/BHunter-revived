#ifndef BH_FLYBY_CONSTRUCT_H
#define BH_FLYBY_CONSTRUCT_H
#include "bh_flyby_scene.h"
typedef struct BHFlybyConstruct
{
    BHFlybyScene *scene;
    BHEntity **current_camera;
    unsigned long settled;
    int error;
} BHFlybyConstruct;
/* Whole0042a9e0. Acquire a real camera entity, initialize actual borrowed
   flyby state, bind the existing real scene/camera callbacks, locate the world,
   run exactly50 pure source camera settling updates and reset the real player
   to start23. This initializes state; it polls no input and runs no menu/game
   loop. Source context/state/starts/world registry/camera/player must remain
   live. Recycled/unwritten source state fields must already be initialized.
   No other scene may bind this context; dispose/unbind previous owner first.
   Allocation failure preserves caller camera. Source dependency failure retires
   newly acquired entity and registry binding; earlier side effects may remain. */
BHEntity *__cdecl bh_flyby_construct(BHFlybyConstruct *);
#endif
