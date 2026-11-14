#ifndef BH_RENDER_EXECUTE_H
#define BH_RENDER_EXECUTE_H
#include "bh_frame_render.h"
#include "../Game/UI/bh_message_panel.h"
#include "../Core/bh_queue_reserve.h"
#include "../Camera/bh_camera_queue.h"
#include "bh_horizon_queue.h"
#include "bh_material_queue.h"
#include "bh_draw_controls.h"
#include "bh_draw_submit.h"
#include "Geometry/bh_screen_quad.h"
#include "../Game/Items/bh_pickup_draw.h"
#include "Geometry/bh_plane_quad.h"
#include "Geometry/bh_quad_queue.h"
#include "../World/Routes/bh_path_queue.h"
#include "../World/bh_segment_queue.h"
#include "bh_fade.h"
#include "../Game/Effects/bh_flash_flare.h"
#include "../Game/UI/bh_menu_present.h"
#include "Textures/bh_gdi_text.h"

typedef enum BHRenderExecuteError
{
    BH_RENDER_EXECUTE_OK,
    BH_RENDER_EXECUTE_ARGUMENT,
    BH_RENDER_EXECUTE_BINDING,
    BH_RENDER_EXECUTE_SELECTOR,
    BH_RENDER_EXECUTE_TOPOLOGY,
    BH_RENDER_EXECUTE_CALLBACK,
    BH_RENDER_EXECUTE_NATIVE
} BHRenderExecuteError;
/* Borrowed source identities replace the original 84-byte records' function
   pointers. A producer must retain every bound owner until this synchronous
   drain returns. PATH_POINT is contiguous payload of PATH_HEADER, never a
   callable list entry. */
typedef struct BHRenderExecute
{
    BHFrameDisplay *display;
    BHTextQueue *text;
    BHAtlasContext *atlas;
    BHDrawQueue *draw;
    const BHViewConfig *view;
    BHMainCamera *camera;
    BHRenderState *state;
    BHHorizonQueueContext *horizon;
    BHDrawImageLookup image_lookup;
    void *image_user;
    BHDrawSubmitContext *model;
    BHDrawBillboardSubmitContext *billboard;
    BHDrawControlContext *controls;
    BHScreenQuadSubmit *screen;
    BHPickupDrawContext *rect;
    BHQuadQueueContext *plane;
    BHPathQueueContext *path;
    BHSegmentQueueContext *segment;
    BHFadeContext *fade;
    BHFlareSubmit *flare;
    BHLightConfig *light;
    BHGdiText *gdi;
    unsigned long display_mode; /* original 004df250; 1 clears white */
    unsigned long text_calls, draw_calls, flushes, native_calls, failures;
    unsigned long kinds[20];
    HRESULT last_hresult;
    BHRenderExecuteError error;
} BHRenderExecute;
/* Complete 00479090 plus its unique 00450930 tail for valid source queues.
   Callback failures are recorded but do not suppress later records or the
   native tail or fail the frame, mirroring original ignored return values.
   error/failures remain diagnostic when the return value is1. Invalid selectors,
   cycles and missing bindings stop at an explicit source adapter boundary.
   Native execution requires an already begun real Direct3D2 scene. */
int bh_render_execute(BHRenderExecute *);
#endif
