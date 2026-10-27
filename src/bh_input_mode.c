#include "bh_input_mode.h"
#include <string.h>
static unsigned long phase(BHEntity *e)
{
    unsigned long n;
    memcpy(&n, (unsigned char *)e + 0x1bc, 4);
    return n;
}
static void set_phase(BHEntity *e, unsigned long n)
{
    memcpy((unsigned char *)e + 0x1bc, &n, 4);
}
static void prime(BHPlayerInputState *s)
{
    unsigned long i;
    for (i = 0; i < 255; ++i)
        bh_player_input_transition(s, i);
}
static int real_commit(BHMenuKeyCommit f)
{
    return f == bh_menu_keys_motion || f == bh_menu_keys_weapons || f == bh_menu_keys_camera || f == bh_menu_keys_extra;
}
static void __cdecl capture_tick(BHEntity *e)
{
    BHInputKeyCapture *c;
    memcpy(&c, e->payload, sizeof(c));
    if (c && c->entity == e)
        bh_input_key_capture_step(c);
}
int __cdecl bh_input_key_capture_begin(BHInputKeyCapture *c)
{
    BHEntity *e;
    if (!c || c->entity || !c->pool || !c->input || !c->snapshot || !c->state || !c->settings ||
        !c->settings->input_profile || !c->menu || !c->sound || !c->result || !real_commit(c->commit))
        return 0;
    e = bh_entity_acquire(c->pool);
    if (!e)
        return 0;
    set_phase(e, 0);
    memcpy(e->payload, &c, sizeof(c));
    e->step = capture_tick;
    c->entity = e;
    c->error = 0;
    if (!bh_input_set_enabled(c->input, c->snapshot, 1))
    {
        bh_entity_release(c->pool, e);
        c->entity = NULL;
        return 0;
    }
    return 1;
}
int __cdecl bh_input_key_capture_step(BHInputKeyCapture *c)
{
    unsigned long n, i;
    BHEntity *e;
    BHMenuKeyCommit commit;
    if (!c || !c->entity || !real_commit(c->commit))
        return 0;
    e = c->entity;
    c->error = 0;
    if (!bh_player_input_apply_packet_checked(c->state, c->input->state, sizeof(c->input->state)))
    {
        c->error = 1;
        return 0;
    }
    n = phase(e);
    if (n == 0)
    {
        prime(c->state);
        set_phase(e, 1);
        return 1;
    }
    if (n == 1)
    {
        for (i = 0; i < 255; ++i)
        {
            if (i == 0x1b)
                continue;
            if (bh_player_input_transition(c->state, i) == 0)
            {
                c->result[0] = (unsigned char)i;
                c->result[1] = 0;
                ++c->sounds;
                c->last_voice = bh_sound_request(c->sound, c->sound_resource, .6f, 1, NULL, NULL);
                if (c->last_voice == BH_SOUND_INVALID)
                    c->error = 1;
                set_phase(e, phase(e) + 1);
            }
        }
        return !c->error;
    }
    prime(c->state);
    if (!bh_input_set_enabled(c->input, c->snapshot, 0))
        c->error = 1;
    commit = c->commit;
    commit(c->settings, c->menu, c->result, c->menu->selection);
    c->commit = NULL;
    if (!bh_entity_release(c->pool, e))
        c->error = 1;
    else
        c->entity = NULL;
    return !c->error;
}
int __cdecl bh_input_key_capture_cancel(BHInputKeyCapture *c)
{
    int okay;
    if (!c || !c->entity || c->pool->dispatch_depth)
        return 0;
    okay = bh_input_set_enabled(c->input, c->snapshot, 0);
    if (!bh_entity_release(c->pool, c->entity))
        return 0;
    c->entity = NULL;
    c->commit = NULL;
    if (!okay)
        c->error = 1;
    return okay;
}
