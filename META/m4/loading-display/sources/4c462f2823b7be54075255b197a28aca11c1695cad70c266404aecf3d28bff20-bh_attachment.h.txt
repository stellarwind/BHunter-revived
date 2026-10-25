#ifndef BH_ATTACHMENT_H
#define BH_ATTACHMENT_H
#include "bh_world_geometry.h"
#include "bh_radar.h"
#include "bh_screen_quad.h"

/* Historical frontier name: these attachments are the world's six portal
   neighbors, not separate allocation/model instances. The actual constructed
   world component owns bounds, +114 query flags, portals and traversal mark. */
typedef BHWorldGeometry BHAttachmentNode;
typedef enum BHAttachmentError {
    BH_ATTACHMENT_OK,BH_ATTACHMENT_ARGUMENT,BH_ATTACHMENT_NUMERICAL,
    BH_ATTACHMENT_PORTAL,BH_ATTACHMENT_QUEUE
} BHAttachmentError;
typedef struct BHAttachmentContext {
    BHDrawQueue *queue;
    const BHRadarFrame *frame;
    BHVec3 player_position;
    float yaw;
    long viewport_width;
    unsigned long default_image,flag200_image,flag400_image,flag800_image;
    unsigned long visited,attempted,queued,dropped,culled;
    BHAttachmentError error;
} BHAttachmentContext;

/* Complete00417f30. An already-clear node suppresses descendant traversal.
   Clear before following all six links in order: shared nodes and cycles are
   valid. No allocation, resource release or link mutation. NULL is permitted
   only as a source convenience at the public boundary. */
void __cdecl bh_attachment_reset(BHAttachmentNode *);
/* Complete00417f70 recursive room/portal radar connectors. Exactly two live
   neighbors produce one portal-to-portal segment; other degrees produce six
   slot-order center-to-portal segments. Radial400 clipping mutates the center
   used by later slots, and post-clip distances determine recursive visits.
   Height fade zero suppresses the second endpoint's visit, but the first
   visit in the two-neighbor branch survives. Visited is set before drawing.
   Uses real004785f0 /00478e50 queue producers, buckets3/8 and borrowed image
   handles. Full queues silently drop while traversal continues. Returns1 for
   culls/drops,0 for invalid source bindings/numerics/topology. Earlier marks
   and queue writes survive failures; caller may reset afterward. Context,
   frame, all reachable world members and images outlive queued consumption;
   queue storage must not overlap them. PC53 nearest/masked finite profile;
   PC24/exceptional original behavior and original software rasterizer remain
   unvalidated. No original full-entity layout or model lifetime is invented. */
int __cdecl bh_attachment_render(BHAttachmentContext *,BHAttachmentNode *);
#endif
