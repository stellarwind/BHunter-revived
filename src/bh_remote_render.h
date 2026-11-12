#ifndef BH_REMOTE_RENDER_H
#define BH_REMOTE_RENDER_H
#include "bh_remote_prediction.h"
#include "bh_player_propulsion.h"
#include "bh_draw_submit.h"
#include "bh_sound_queue.h"
/* Remote entities store the quantized nozzle channels in their own payload. */
typedef struct BHRemoteRender
{
    BHDrawQueue *queue;
    BHSoundQueue *sound;
    unsigned long models[8], spark_texture, wreck_model, thrust_texture;
    unsigned short tick;
    unsigned long attempted, dropped;
    int error;
} BHRemoteRender;
/* Whole00423a80. Signed16 tick difference clamped to zero; position and angle
   anchors at+180/+18c; velocity pairs+48/+54, acceleration pairs+60/+6c.
   Outputs must not overlap entity or each other. PC53 finite contract. */
int bh_remote_predict_pose(const BHEntity *, unsigned short tick, BHVec3 *, BHVec3 *);
/* Model, propulsion and shield spark queue producers and
   actual deferred sound request/update. No sound Play occurs in this owner.
   Resource slots, tick, queue and sound are caller-owned live bindings. */
int bh_remote_render(BHRemoteRender *, BHEntity *);
#endif
