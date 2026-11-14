#include "bh_input_mode.h"
#include <string.h>
static unsigned long phase(BHEntity *entity)
{
    unsigned long phase_value;
    memcpy(&phase_value, &entity->menu.input_phase, 4);
    return phase_value;
}
static void set_phase(BHEntity *entity, unsigned long phase_value)
{
    memcpy(&entity->menu.input_phase, &phase_value, 4);
}
static void prime(BHPlayerInputState *input_state)
{
    unsigned long i;
    for (i = 0; i < 255; ++i)
        bh_player_input_transition(input_state, i);
}
static int real_commit(BHMenuKeyCommit commit_callback)
{
    return commit_callback == bh_menu_keys_motion || commit_callback == bh_menu_keys_weapons ||
           commit_callback == bh_menu_keys_camera || commit_callback == bh_menu_keys_extra;
}
static void capture_tick(BHEntity *entity)
{
    BHInputKeyCapture *capture;
    memcpy(&capture, &entity->bindings.key_capture, sizeof(capture));
    if (capture && capture->entity == entity)
        bh_input_key_capture_step(capture);
}
int bh_input_key_capture_begin(BHInputKeyCapture *capture)
{
    BHEntity *entity;
    if (!capture || capture->entity || !capture->pool || !capture->input || !capture->snapshot || !capture->state ||
        !capture->settings || !capture->settings->input_profile || !capture->menu || !capture->sound ||
        !capture->result || !real_commit(capture->commit))
        return 0;
    entity = bh_entity_acquire(capture->pool);
    if (!entity)
        return 0;
    set_phase(entity, 0);
    memcpy(&entity->bindings.key_capture, &capture, sizeof(capture));
    entity->step = capture_tick;
    capture->entity = entity;
    capture->error = 0;
    if (!bh_input_set_enabled(capture->input, capture->snapshot, 1))
    {
        bh_entity_release(capture->pool, entity);
        capture->entity = NULL;
        return 0;
    }
    return 1;
}
int bh_input_key_capture_step(BHInputKeyCapture *capture)
{
    unsigned long capture_phase, key_code;
    BHEntity *entity;
    BHMenuKeyCommit commit;
    if (!capture || !capture->entity || !real_commit(capture->commit))
        return 0;
    entity = capture->entity;
    capture->error = 0;
    if (!bh_player_input_apply_packet_checked(capture->state, capture->input->state, sizeof(capture->input->state)))
    {
        capture->error = 1;
        return 0;
    }
    capture_phase = phase(entity);
    if (capture_phase == 0)
    {
        prime(capture->state);
        set_phase(entity, 1);
        return 1;
    }
    if (capture_phase == 1)
    {
        for (key_code = 0; key_code < 255; ++key_code)
        {
            if (key_code == 0x1b)
                continue;
            if (bh_player_input_transition(capture->state, key_code) == 0)
            {
                capture->result[0] = (unsigned char)key_code;
                capture->result[1] = 0;
                ++capture->sounds;
                capture->last_voice = bh_sound_request(capture->sound, capture->sound_resource, .6f, 1, NULL, NULL);
                if (capture->last_voice == BH_SOUND_INVALID)
                    capture->error = 1;
                set_phase(entity, phase(entity) + 1);
            }
        }
        return !capture->error;
    }
    prime(capture->state);
    if (!bh_input_set_enabled(capture->input, capture->snapshot, 0))
        capture->error = 1;
    commit = capture->commit;
    commit(capture->settings, capture->menu, capture->result, capture->menu->selection);
    capture->commit = NULL;
    if (!bh_entity_release(capture->pool, entity))
        capture->error = 1;
    else
        capture->entity = NULL;
    return !capture->error;
}
int bh_input_key_capture_cancel(BHInputKeyCapture *capture)
{
    int okay;
    if (!capture || !capture->entity || capture->pool->dispatch_depth)
        return 0;
    okay = bh_input_set_enabled(capture->input, capture->snapshot, 0);
    if (!bh_entity_release(capture->pool, capture->entity))
        return 0;
    capture->entity = NULL;
    capture->commit = NULL;
    if (!okay)
        capture->error = 1;
    return okay;
}
