#ifndef BH_FRAME_WORKER_H
#define BH_FRAME_WORKER_H
#include "bh_frame_buffers.h"
#include "bh_render_execute.h"
#include "bh_present_begin.h"
#include "bh_capture.h"
#include "bh_frame_callback.h"

typedef enum BHFrameWorkerError
{
    BH_FRAME_WORKER_OK,
    BH_FRAME_WORKER_ARGUMENT,
    BH_FRAME_WORKER_WAIT,
    BH_FRAME_WORKER_EMPTY,
    BH_FRAME_WORKER_BINDING,
    BH_FRAME_WORKER_RENDER,
    BH_FRAME_WORKER_NATIVE,
    BH_FRAME_WORKER_CAPTURE
} BHFrameWorkerError;
/* Explicit live bindings replace the original process globals. All storage is
   borrowed. Each queue's commands must be its corresponding slot's allocation;
   producers retain queue metadata/resources until the worker clears busy.
   The flags are shared Win32 LONG words; only the owning application changes
   ready/display_state/stop/request_capture while this owner executes. */
typedef struct BHFrameWorker
{
    BHFrameBuffers *frames;
    BHDrawQueue *queues[3];
    BHRenderExecute *render;
    BHFrameCallback *callback;
    BHPresentBegin *begin;
    BHCapture *capture;
    BHViewConfig *viewport;
    long width, height;
    LPDIRECTDRAWSURFACE primary, back;
    volatile LONG ready, display_state, stop, done, request_capture;
    const char *dump_prefix; /* NULL preserves c:\\DUMP%.4d.BMP */
    unsigned long iterations, rendered, skipped, waits, timeouts, sleeps;
    unsigned long native_calls, restores, flips, captures, failures;
    DWORD last_wait;
    HRESULT last_hresult;
    long selected;
    BHFrameWorkerError error;
} BHFrameWorker;
/* Complete00471e00, including optional BMP request, display-state wait,
   lost-surface recovery and Flip. Shutdown is sampled at the loop bottom,
   after the frame, then done is published and zero is returned. Native errors
   record evidence but preserve the original subsequent operations. No eligible
   slot (original undefined saved-stack fallback) is guarded as an empty frame.
   The source thread adapter owns no thread/window/device lifetime. */
DWORD WINAPI bh_frame_worker_run(void *);
#endif
