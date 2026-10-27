#ifndef BH_TARGET_DRAW_H
#define BH_TARGET_DRAW_H
#include "bh_pickup_draw.h"
#include "bh_project_scale.h"
typedef struct BHTargetDrawState
{
    BHVec3 local_anchor;
    long number;
    unsigned long flags, resource;
    BHEntity *owner;
} BHTargetDrawState;
typedef struct BHTargetDrawContext
{
    BHDrawQueue *queue;
    const BHMainCamera *camera;
    const BHViewConfig *view;
    const BHEntity *player;
    unsigned long atlas_image, marker_image;
    long screen_width; /* explicit original signed004e2998, scale=width/640 */
} BHTargetDrawContext;
/* Count-zero field views of this target class only. The actual local basis
   occupies entity+18..38, its model angles+3c..44. Other entity classes can
   assign unrelated payload meanings; these are not generic entity fields. */
int __cdecl bh_target_draw_state_read(const BHEntity *, BHTargetDrawState *);
int __cdecl bh_target_draw_state_write(BHEntity *, const BHTargetDrawState *);
/* Whole0043c3f0. Always queue model at world+basis*anchor. Owner==player and
   nonzero signed number gate full atlas background/digits and optional marker.
   All handles resolve through actual existing resource/image consumers.
   Return1 all queued,0 any queue exhaustion,-1 source binding/producer error.
   Preserve successful appends and continue the whole owner after exhaustion.
   Camera/view/queue/player/images/target lifetimes are borrowed; PC53 required.
   Entity remains unchanged. No constructor, callback stub or visual substitute. */
int __cdecl bh_target_draw(BHTargetDrawContext *, const BHEntity *);
#endif
