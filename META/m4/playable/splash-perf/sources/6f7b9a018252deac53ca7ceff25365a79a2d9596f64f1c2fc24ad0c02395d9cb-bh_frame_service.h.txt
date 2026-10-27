#ifndef BH_FRAME_SERVICE_H
#define BH_FRAME_SERVICE_H
#include "bh_frame_buffers.h"
#include "bh_draw_queue.h"
#include "bh_input_collection.h"
#include "bh_sound_queue.h"
#include "bh_entities.h"
struct BHInputDevices;
struct BHCdClose;
struct BHNetworkShutdown;
struct BHForceFeedback;
struct BHDisplayShutdown;
typedef enum BHFrameServiceError {
    BH_FRAME_SERVICE_OK,BH_FRAME_SERVICE_ARGUMENT,BH_FRAME_SERVICE_INPUT,
    BH_FRAME_SERVICE_BUFFER,BH_FRAME_SERVICE_ENTITY,BH_FRAME_SERVICE_SOUND,
    BH_FRAME_SERVICE_PACKET,BH_FRAME_SERVICE_SHUTDOWN
} BHFrameServiceError;
/* Borrowed live state for complete00479140. draw is the canonical BORROWING
   descriptor already bound to actual scene/entity producers. It must own no
   allocation; frame buffers own command storage. slot_draw descriptors are
   separate borrowed metadata read by the worker only after publication. */
typedef struct BHFrameService {
    BHInputCollection *input;
    BHEntityPool *entities;
    BHSoundQueue *sound;
    BHFrameBuffers *frames;
    BHDrawQueue *draw,*slot_draw[3];
    BHPlayerInputState *player_input; /* destination state at original004f5518 */
    const void *packet;unsigned long packet_bytes; /* current list004f5500 +2010 */
    const volatile LONG *paused;
    volatile unsigned long *frame_counter;
    volatile LONG *stop,*worker_done;
    float master_db;
    struct BHInputDevices *devices;struct BHCdClose *cd;struct BHNetworkShutdown *network;
    struct BHForceFeedback *force_feedback;struct BHDisplayShutdown *display;
    unsigned long calls,produced,input_calls,step_calls,refresh_calls,sound_calls;
    unsigned long packet_calls,shutdown_calls,cleanup_order[6],cleanup_count,failures;
    unsigned long changed;float last_dt;
    BHSoundTickStats sound_stats;BHFrameServiceError error;
    /* Optional source-adapter diagnostics; zero leaves timing calls disabled. */
    int profile;
    DWORD input_ms,step_ms,refresh_ms,sound_ms,packet_ms;
} BHFrameService;
/* Whole original producer/acknowledged-shutdown callback. Normal: collect
   input, increment wrapping frame counter only when pause==0, acquire actual
   frame buffer, initialize3500-command queue, step/refresh entities, deferred
   sound tick with original binary32 1/30, apply current input packet and publish.
   Shutdown: nonzero signed stop requires worker_done!=0 and stop<=1; increment
   stop then input/CD/sound/network/force-feedback/display cleanup in that order.
   Verified RET0047d680 needs no source stub. Returns status diagnostics while
   original subsequent effects continue after dependency-reported failures.
   player_input and packet are read after entity refresh/sound; callers must
   keep packet bound to the CURRENT input list's serialized packet when those
   callbacks replace the list. The original00475870 takes (packet,state); our
   source API deliberately takes (state,packet). Original004f5518 is the
   destination; current list004f5500 +2010 supplies the packet. Explicit source
   storage does not recover original global allocation or startup construction.
   No callback body, device, entity or gameplay state is fabricated here. */
int __cdecl bh_frame_service_tick(BHFrameService *);
#endif
