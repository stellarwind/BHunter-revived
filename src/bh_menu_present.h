#ifndef BH_MENU_PRESENT_H
#define BH_MENU_PRESENT_H
#include "bh_menu_row.h"
#include "bh_quad_queue.h"
#include "bh_light_config.h"
#define BH_DRAW_LIGHT 18UL
/* Explicit borrowed original camera+160/16c and menu+1bc..200 bindings.
   Row model identity is the actual BHMenuRow identity; image handles bind its
   actual owned atlas image in the renderer lookup. No resource is fabricated. */
typedef struct BHMenuPresentRow
{
    BHMenuRow *row;
    unsigned long image_handle;
} BHMenuPresentRow;
typedef struct BHMenuPresent
{
    BHDrawQueue *queue;
    const BHVec3 *eye, *angles;
    BHMenuPresentRow *rows;
    unsigned long row_count, frame_resource, phase;
    long application_mode, selection;
    float display_selection, push, distance;
    unsigned long capture, queued, dropped;
    int error;
} BHMenuPresent;
int __cdecl bh_menu_light_queue(BHDrawQueue *, unsigned long, long, float, float, float, float, float, unsigned long,
                                unsigned long);
int __cdecl bh_menu_light_execute(BHLightConfig *, const BHDrawCommand *, unsigned long *);
/* Whole0042abd0. Missing eye or application_mode>=1 suppresses everything.
   Capacity exhaustion drops individual commands while preserving call order.
   Finite moderate PC53 inputs; original PC24 remains unvalidated. */
int __cdecl bh_menu_present(BHMenuPresent *);
#endif
