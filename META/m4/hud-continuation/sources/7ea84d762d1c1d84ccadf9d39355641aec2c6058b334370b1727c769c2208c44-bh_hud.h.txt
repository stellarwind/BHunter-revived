#ifndef BH_HUD_H
#define BH_HUD_H
#include "bh_attachment.h"

/* Verified player prefix used by 0041b2d0. This is separate from BHEntity:
   +e8 here is tested as a raw Boolean, not called as a draw callback.
   Opaque bytes are preserved. The root is a borrowed world/portal graph. */
typedef struct BHHudPlayer {
    unsigned char opaque0[12];
    BHVec3 position,right,up,forward;
    float yaw;
    unsigned char opaque1[132];
    BHAttachmentNode *root;
    unsigned char opaque2[32];
    long flat_raw;
} BHHudPlayer;

typedef enum BHHudError {
    BH_HUD_OK,BH_HUD_ARGUMENT,BH_HUD_NUMERICAL,BH_HUD_QUEUE,BH_HUD_ATTACHMENT
} BHHudError;
/* Explicit owning storage for the setup's scattered globals. frame and the
   attachment's frame must designate the same live BHRadarFrame. The attachment
   context supplies the live signed viewport width, queue and image bindings.
   Setup owns neither player, graph, queue nor images. Existing frame near_z /
   visibility_word are live inputs owned elsewhere and are preserved. */
typedef struct BHHudContext {
    BHAttachmentContext *attachment;
    BHRadarFrame *frame;
    unsigned long background_image;
    BHVec3 world_center; /* 004df1c8..004df1d0, before radar offsets */
    long screen_diameter; /* 004d1898, updated only on flat path */
    unsigned long attempted,queued,dropped;
    BHHudError error;
} BHHudContext;

/* Complete 0041b2d0: all world/yaw globals, optional flat background, graph
   render and guarded recursive reset. A null root still performs all setup.
   Screen metrics are retained on the world path. Full-queue drops are success.
   On a source queue/render error, graph reset still executes after rendering.
   Caller provides moderate finite values, nonoverlapping inputs/state and the
   PC53/nearest/masked FP profile. No original code or callback substitute. */
int __cdecl bh_hud_setup(BHHudContext *,const BHHudPlayer *);
#endif
