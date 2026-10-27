#ifndef BH_PROJECT_SCALE_H
#define BH_PROJECT_SCALE_H
#include "bh_main_camera.h"
/* Whole00455840: transform world inputs through the actual projected camera;
   reject depth below near (including unordered) without changing inputs.
   Otherwise store screenX,screenY,focal/depth in that order, overwriting the
   input Z with scale. Equal pointers retain the observed sequential stores.
   Camera/config are borrowed actual source owners, replacing scattered globals.
   PC53/nearest/masked is required; output pointers must not alias the camera
   or config, though any or all three scalar pointers may equal each other. */
int __cdecl bh_project_point_scale(const BHMainCamera *, const BHViewConfig *, float *x, float *y, float *z);
#endif
