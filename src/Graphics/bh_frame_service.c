#include "bh_frame_service.h"
#include <string.h>
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif

static void failure(BHFrameService *frame_service, BHFrameServiceError error)
{
    ++frame_service->failures;
    if (frame_service->error == BH_FRAME_SERVICE_OK)
        frame_service->error = error;
}
int bh_frame_service_tick(BHFrameService *frame_service)
{
    void *buffer;
    long active;
    unsigned long i;
    if (!frame_service)
        return 0;
    ++frame_service->calls;
    frame_service->error = BH_FRAME_SERVICE_OK;
    if (!frame_service->input || !frame_service->entities || !frame_service->sound || !frame_service->frames || !frame_service->draw || !frame_service->paused || !frame_service->frame_counter ||
        !frame_service->player_input || !frame_service->packet || frame_service->packet_bytes < 2 || frame_service->frames->bytes < 3500UL * sizeof(BHDrawCommand) ||
        frame_service->draw->allocator.allocate || frame_service->draw->allocator.release)
    {
        failure(frame_service, BH_FRAME_SERVICE_ARGUMENT);
        return 0;
    }
    for (i = 0; i < 3; ++i)
        if (!frame_service->slot_draw[i] || frame_service->slot_draw[i] == frame_service->draw || (i && frame_service->slot_draw[i] == frame_service->slot_draw[0]) ||
            (i == 2 && frame_service->slot_draw[2] == frame_service->slot_draw[1]))
        {
            failure(frame_service, BH_FRAME_SERVICE_ARGUMENT);
            return 0;
        }
    ++frame_service->input_calls;
    if (!bh_input_collect(frame_service->input, &frame_service->changed))
        failure(frame_service, BH_FRAME_SERVICE_INPUT);
    if (!*frame_service->paused)
        *frame_service->frame_counter = *frame_service->frame_counter + 1UL;
    buffer = bh_frame_buffers_acquire(frame_service->frames);
    if (!buffer)
    {
        failure(frame_service, BH_FRAME_SERVICE_BUFFER);
        return 0;
    }
    active = frame_service->frames->active;
    frame_service->draw->commands = (BHDrawCommand *)buffer;
    frame_service->draw->capacity = 3500;
#ifdef BH_SDL_BACKEND
    bh_interpolation_reset();
#endif
    if (!bh_draw_queue_begin(frame_service->draw))
        failure(frame_service, BH_FRAME_SERVICE_BUFFER);
    frame_service->step_error = BH_ENTITY_OK;
    frame_service->refresh_error = BH_ENTITY_OK;
    ++frame_service->step_calls;
    if (!bh_entities_step(frame_service->entities))
    {
        frame_service->step_error = frame_service->entities->error;
        failure(frame_service, BH_FRAME_SERVICE_ENTITY);
    }
    ++frame_service->refresh_calls;
    if (!bh_entities_refresh(frame_service->entities))
    {
        frame_service->refresh_error = frame_service->entities->error;
        failure(frame_service, BH_FRAME_SERVICE_ENTITY);
    }
    frame_service->last_dt = 0.033333335071802139f;
    /* 004791c4 calls sound then overwrites EAX at004791c9. Native sound
       rejection is diagnostic, not an original reason to stop gameplay.
       Invalid source ownership/numerics remain explicit binding failures. */
    ++frame_service->sound_calls;
    if (!bh_sound_tick(frame_service->sound, frame_service->last_dt, frame_service->master_db, &frame_service->sound_stats) && frame_service->sound->error != BH_SOUND_BACKEND)
        failure(frame_service, BH_FRAME_SERVICE_SOUND);
    ++frame_service->packet_calls;
    if (!bh_player_input_apply_packet_checked(frame_service->player_input, frame_service->packet, frame_service->packet_bytes))
        failure(frame_service, BH_FRAME_SERVICE_PACKET);
    /* The worker sees metadata for THIS allocation; the canonical descriptor
       can move to another slot as soon as this one has been published. */
    *frame_service->slot_draw[active] = *frame_service->draw;
    if (!bh_frame_buffers_publish(frame_service->frames))
        failure(frame_service, BH_FRAME_SERVICE_BUFFER);
    else
        ++frame_service->produced;
    return frame_service->error == BH_FRAME_SERVICE_OK;
}
