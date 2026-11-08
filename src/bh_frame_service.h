#ifndef BH_FRAME_SERVICE_H
#define BH_FRAME_SERVICE_H
#include "bh_frame_buffers.h"
#include "bh_draw_queue.h"
#include "bh_input_collection.h"
#include "bh_sound_queue.h"
#include "bh_entities.h"
typedef enum BHFrameServiceError
{
    BH_FRAME_SERVICE_OK,
    BH_FRAME_SERVICE_ARGUMENT,
    BH_FRAME_SERVICE_INPUT,
    BH_FRAME_SERVICE_BUFFER,
    BH_FRAME_SERVICE_ENTITY,
    BH_FRAME_SERVICE_SOUND,
    BH_FRAME_SERVICE_PACKET
} BHFrameServiceError;
/* Borrowed live state for complete00479140. draw is the canonical BORROWING
   descriptor already bound to actual scene/entity producers. It must own no
   allocation; frame buffers own command storage. slot_draw descriptors are
   separate borrowed metadata read by the renderer after publication. */
typedef struct BHFrameService
{
    BHInputCollection *input;
    BHEntityPool *entities;
    BHSoundQueue *sound;
    BHFrameBuffers *frames;
    BHDrawQueue *draw, *slot_draw[3];
    BHPlayerInputState *player_input; /* destination state at original004f5518 */
    const void *packet;
    unsigned long packet_bytes; /* current list004f5500 +2010 */
    const long *paused;
    unsigned long *frame_counter;
    float master_db;
    unsigned long calls, produced, input_calls, step_calls, refresh_calls, sound_calls;
    unsigned long packet_calls, failures;
    unsigned long changed;
    float last_dt;
    BHSoundTickStats sound_stats;
    BHFrameServiceError error;
    BHEntityError step_error, refresh_error;
} BHFrameService;
/* Produce one frame: collect
   input, increment wrapping frame counter only when pause==0, acquire actual
   frame buffer, initialize3500-command queue, step/refresh entities, deferred
   sound tick with original binary32 1/30, apply current input packet and publish.
   Verified RET0047d680 needs no source stub. Returns status diagnostics while
   original subsequent effects continue after dependency-reported failures.
   Native sound API failures remain in sound_stats but do not fail the frame;
   original004791c9 overwrites the sound return without testing it.
   player_input and packet are read after entity refresh/sound; callers must
   keep packet bound to the CURRENT input list's serialized packet when those
   callbacks replace the list. The original00475870 takes (packet,state); our
   source API deliberately takes (state,packet). Original004f5518 is the
   destination; current list004f5500 +2010 supplies the packet. Explicit source
   storage does not recover original global allocation or startup construction.
   No callback body, device, entity or gameplay state is fabricated here. */
int bh_frame_service_tick(BHFrameService *);
#endif
