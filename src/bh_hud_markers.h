#ifndef BH_HUD_MARKERS_H
#define BH_HUD_MARKERS_H
#include "bh_radar.h"

typedef enum BHHudMarkersError
{
    BH_HUD_MARKERS_OK,
    BH_HUD_MARKERS_ARGUMENT,
    BH_HUD_MARKERS_CALLBACK,
    BH_HUD_MARKERS_KIND,
    BH_HUD_MARKERS_LIMIT,
    BH_HUD_MARKERS_QUEUE,
    BH_HUD_MARKERS_NUMERICAL
} BHHudMarkersError;

/* Borrowed bindings for the actual 004dfb28 actor list, renderer and atlases.
   This owner creates no actors or callbacks. All bindings, query outputs and
   the current actor's lifetime must survive synchronous queries and draws. */
typedef struct BHHudMarkers
{
    BHPickupDrawContext *projection;
    BHRadarContext *radar;
    unsigned long brackets_image, status_image;
    long display_width;
    unsigned long limit, visited, attempted, queued, dropped, projected;
    BHHudMarkersError error;
} BHHudMarkers;

/* Complete 0041bbf0 over the source actor representation. Reads +fc kind after
   the installed event-3 query, +114 signed status for kind8, and +f0 successor
   after all draws. The 40-byte packet's event is initialized ONCE, as original;
   query methods must fill xyz and may observe preceding packet mutations.
   Kinds1/2/6/8 are the reviewed list domain; unknown kinds diagnose undefined
   original atlas-local behavior rather than inventing atlas defaults. The
   source additionally bounds traversal and rejects absent callbacks/nonfinite
   projection. Queue exhaustion drops individual draws without stopping later
   actors. A later error retains earlier callback and queue effects.
   player_flags borrows the original player byte+2b8, not the radar visibility
   word. It is read after each query so callback mutations remain observable.
   PC53/nearest/masked finite input profile; PC24 remains unvalidated. */
int __cdecl bh_hud_markers_draw(BHHudMarkers *, BHEntity *head, const BHVec3 *player_position,
                                const unsigned char *player_flags);
#endif
