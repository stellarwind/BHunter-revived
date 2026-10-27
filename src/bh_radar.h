#ifndef BH_RADAR_H
#define BH_RADAR_H
#include "bh_quad_queue.h"
#include "bh_pickup_draw.h"
/* Explicit scattered global bindings produced by the still-unrecovered
   HUD setup0041b2d0. This record does not claim original struct layout or that
   setup's attachment/weapon callbacks have been restored. The vectors are
   actual world radar placement/orientation and two yaw projection rows. */
typedef struct BHRadarFrame
{
    BHVec3 origin, axis_x, axis_y, normal, yaw_x, yaw_y;
    long unit, screen_radius, screen_x, screen_y;
    float near_z; /* live original0049175c, not its initialized PE value1 */
    unsigned long flat, visibility_word;
} BHRadarFrame;
typedef enum BHRadarError
{
    BH_RADAR_OK,
    BH_RADAR_ARGUMENT,
    BH_RADAR_NUMERICAL,
    BH_RADAR_QUEUE
} BHRadarError;
typedef struct BHRadarContext
{
    BHDrawQueue *queue;
    const BHRadarFrame *frame;
    unsigned long marker_image, bar_image;
    unsigned long attempted, queued, dropped, culled;
    BHRadarError error;
} BHRadarContext;
/* New source binding adapter, borrowed frame/queue outlive calls; borrowed
   image handles outlive queue consumption. No setup/startup implementation. */
int __cdecl bh_radar_bind(BHRadarContext *, BHDrawQueue *, const BHRadarFrame *, unsigned long marker_image,
                          unsigned long bar_image);
/* Complete0041b650: fourth original argument is a raw integer Boolean, despite
   the float decompiler signature. Retains height/radius gates, bit8 visibility
   rule, exact-kind1/2 size rules, quadrant UVs and both flat/world branches.
   skip_marker suppresses only the main marker; kind1 still queues its bar.
   Return1 includes culling/full-queue drops; source invalid binding/queue or
   nonfinite numerical guard reports0. Does not consume/change target position.
   Caller owns PC53/nearest/masked profile; overlapping inputs/queue forbidden. */
int __cdecl bh_radar_marker(BHRadarContext *, const BHVec3 *player_position, const BHVec3 *target_position,
                            unsigned long kind, unsigned long extended, unsigned long skip_marker);
#endif
