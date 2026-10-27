#ifndef BH_CAMERA_QUEUE_H
#define BH_CAMERA_QUEUE_H
#include "bh_draw_queue.h"
#include "bh_main_camera.h"
/* Complete 00477fe0. Camera pose commands always join header zero, preceding
   every numbered draw bucket. Inputs are radians and borrowed only until the
   producer returns; the queue retains all six binary32 payloads verbatim.
   Return 1 queued, 0 full, -1 invalid source binding/topology. */
int __cdecl bh_camera_queue_pose(BHDrawQueue *, const BHVec3 *position, const BHVec3 *angles);
/* Complete 00477830 with explicit bindings for the original global projection
   and camera owner. Applies the recovered 00452070 pose owner, then reads next.
   Queue storage/config/camera must be live and not overlap one another. */
int __cdecl bh_camera_queue_execute(const BHViewConfig *, BHMainCamera *, const BHDrawQueue *, const BHDrawCommand *,
                                    unsigned long *next);
#endif
