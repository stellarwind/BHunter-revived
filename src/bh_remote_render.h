#ifndef BH_REMOTE_RENDER_H
#define BH_REMOTE_RENDER_H
#include "bh_remote_prediction.h"
#include "bh_draw_submit.h"
#include "bh_sound_queue.h"
typedef struct BHRemoteRender
{
    BHDrawQueue *queue;
    BHSoundQueue *sound;
    unsigned long models[8], spark_texture;
    unsigned short tick;
    unsigned long attempted, dropped;
    int error;
} BHRemoteRender;
/* Whole00423a80. Signed16 tick difference clamped to zero; position and angle
   anchors at+180/+18c; velocity pairs+48/+54, acceleration pairs+60/+6c.
   Outputs must not overlap entity or each other. PC53 finite contract. */
int __cdecl bh_remote_predict_pose(const BHEntity *, unsigned short tick, BHVec3 *, BHVec3 *);
/* Whole00423c80. Actual model/24 random oriented spark queue producers and
   actual deferred sound request/update. No sound Play occurs in this owner.
   Resource slots, tick, queue and sound are caller-owned live bindings. */
int __cdecl bh_remote_render(BHRemoteRender *, BHEntity *);
#endif
