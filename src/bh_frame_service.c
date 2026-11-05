#include "bh_frame_service.h"
#include <string.h>
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif

static void failure(BHFrameService *s, BHFrameServiceError error)
{
    ++s->failures;
    if (s->error == BH_FRAME_SERVICE_OK)
        s->error = error;
}
int __cdecl bh_frame_service_tick(BHFrameService *s)
{
    void *buffer;
    long active;
    unsigned long i;
    if (!s)
        return 0;
    ++s->calls;
    s->error = BH_FRAME_SERVICE_OK;
    if (!s->input || !s->entities || !s->sound || !s->frames || !s->draw || !s->paused || !s->frame_counter ||
        !s->player_input || !s->packet || s->packet_bytes < 2 || s->frames->bytes < 3500UL * sizeof(BHDrawCommand) ||
        s->draw->allocator.allocate || s->draw->allocator.release)
    {
        failure(s, BH_FRAME_SERVICE_ARGUMENT);
        return 0;
    }
    for (i = 0; i < 3; ++i)
        if (!s->slot_draw[i] || s->slot_draw[i] == s->draw || (i && s->slot_draw[i] == s->slot_draw[0]) ||
            (i == 2 && s->slot_draw[2] == s->slot_draw[1]))
        {
            failure(s, BH_FRAME_SERVICE_ARGUMENT);
            return 0;
        }
    ++s->input_calls;
    if (!bh_input_collect(s->input, &s->changed))
        failure(s, BH_FRAME_SERVICE_INPUT);
    if (!*s->paused)
        *s->frame_counter = *s->frame_counter + 1UL;
    buffer = bh_frame_buffers_acquire(s->frames);
    if (!buffer)
    {
        failure(s, BH_FRAME_SERVICE_BUFFER);
        return 0;
    }
    active = s->frames->active;
    s->draw->commands = (BHDrawCommand *)buffer;
    s->draw->capacity = 3500;
#ifdef BH_SDL_BACKEND
    bh_interpolation_reset();
#endif
    if (!bh_draw_queue_begin(s->draw))
        failure(s, BH_FRAME_SERVICE_BUFFER);
    s->step_error = BH_ENTITY_OK;
    s->refresh_error = BH_ENTITY_OK;
    ++s->step_calls;
    if (!bh_entities_step(s->entities))
    {
        s->step_error = s->entities->error;
        failure(s, BH_FRAME_SERVICE_ENTITY);
    }
    ++s->refresh_calls;
    if (!bh_entities_refresh(s->entities))
    {
        s->refresh_error = s->entities->error;
        failure(s, BH_FRAME_SERVICE_ENTITY);
    }
    s->last_dt = 0.033333335071802139f;
    /* 004791c4 calls sound then overwrites EAX at004791c9. Native sound
       rejection is diagnostic, not an original reason to stop gameplay.
       Invalid source ownership/numerics remain explicit binding failures. */
    ++s->sound_calls;
    if (!bh_sound_tick(s->sound, s->last_dt, s->master_db, &s->sound_stats) && s->sound->error != BH_SOUND_BACKEND)
        failure(s, BH_FRAME_SERVICE_SOUND);
    ++s->packet_calls;
    if (!bh_player_input_apply_packet_checked(s->player_input, s->packet, s->packet_bytes))
        failure(s, BH_FRAME_SERVICE_PACKET);
    /* The worker sees metadata for THIS allocation; the canonical descriptor
       can move to another slot as soon as this one has been published. */
    *s->slot_draw[active] = *s->draw;
    if (!bh_frame_buffers_publish(s->frames))
        failure(s, BH_FRAME_SERVICE_BUFFER);
    else
        ++s->produced;
    return s->error == BH_FRAME_SERVICE_OK;
}
