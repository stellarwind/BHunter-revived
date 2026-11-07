#include "bh_input_text.h"
#include "bh_platform.h"
#include <stdio.h>
#include <string.h>
static unsigned long phase(BHEntity *e)
{
    unsigned long p;
    memcpy(&p, (unsigned char *)e + 0x1bc, 4);
    return p;
}
static void set_phase(BHEntity *e, unsigned long p)
{
    memcpy((unsigned char *)e + 0x1bc, &p, 4);
}
static void prime(BHPlayerInputState *s)
{
    unsigned long i;
    for (i = 0; i < 255; ++i)
        bh_player_input_transition(s, i);
}
static int row_valid(BHInputTextCapture *c)
{
    long i = c->menu->selection;
    return i >= 0 && (unsigned long)i < c->menu->row_count && c->menu->rows && c->menu->rows[i];
}
static int valid(BHInputTextCapture *c)
{
    const char *end;
    if (!c || !c->pool || !c->input || !c->snapshot || !c->state || !c->menu || !c->rows || !c->sound || !c->buffer ||
        !c->capacity || c->limit < 0 || (unsigned long)c->limit >= c->capacity)
        return 0;
    if (c->limit > 15 || (c->commit == BH_TEXT_COMMIT_PROFILE ? !c->profile
                           : c->commit != BH_TEXT_COMMIT_ADDRESS || !c->address_commit))
        return 0;
    end = (const char *)memchr(c->buffer, 0, c->capacity);
    return end && c->length >= 0 && c->length <= c->limit &&
           end - c->buffer == c->length && row_valid(c) && (unsigned long)c->menu->selection < c->rows->count;
}
static void sound(BHInputTextCapture *c, float pitch)
{
    ++c->sounds;
    c->last_voice = bh_sound_request(c->sound, c->sound_resource, .6f, pitch, NULL, NULL);
    if (c->last_voice == BH_SOUND_INVALID)
        c->error = 1;
}
static void paint(BHInputTextCapture *c)
{
    unsigned long i, n = (unsigned long)strlen(c->buffer), first = n > 17 ? n - 17 : 0;
    for (i = 0; i < n; ++i)
        if (c->buffer[i] >= 'a' && c->buffer[i] <= 'z')
            c->buffer[i] -= 32;
    sprintf(c->display, "%s_%s", c->buffer + first,
            c->commit == BH_TEXT_COMMIT_ADDRESS ? "\nA Join  B Back" : "");
    ++c->repaints;
    if (!bh_menu_row_refresh(c->rows, (unsigned long)c->menu->selection, NULL, c->display))
        c->error = 1;
}
static void __cdecl text_tick(BHEntity *e)
{
    BHInputTextCapture *c;
    memcpy(&c, e->payload, 4);
    if (c && c->entity == e)
        bh_input_text_step(c);
}
static void address_input(BHPlatformTextEvent event, const char *text, void *user)
{
    BHInputTextCapture *c = user;
    BHMenuSettingsRefresh refresh;
    long length = c->length;
    if (event == BH_PLATFORM_TEXT_CANCEL || event == BH_PLATFORM_TEXT_SUBMIT)
    {
        if (!bh_input_text_cancel(c))
            return;
        sound(c, 1);
        if (event == BH_PLATFORM_TEXT_SUBMIT)
            c->address_commit(c->address_user, c->buffer);
        else if ((refresh = c->menu->rows[c->menu->selection]->refresh) != NULL)
            refresh(c->menu, c->menu->selection);
        return;
    }
    if (event == BH_PLATFORM_TEXT_BACKSPACE)
    {
        if (c->length > 0)
            --c->length;
    }
    else if (event == BH_PLATFORM_TEXT_INSERT)
    {
        for (; *text && c->length < c->limit; ++text)
            if ((*text >= '0' && *text <= '9') || *text == '.')
                c->buffer[c->length++] = *text;
    }
    if (c->length != length)
    {
        c->buffer[c->length] = 0;
        sound(c, 1);
        paint(c);
    }
}
static int begin(BHInputTextCapture *c, int initial_paint)
{
    BHEntity *e;
    if (!c || c->entity || !valid(c))
        return 0;
    e = bh_entity_acquire(c->pool);
    if (!e)
        return 0;
    set_phase(e, 0);
    memcpy(e->payload, &c, 4);
    e->step = text_tick;
    c->entity = e;
    c->error = 0;
    if (c->commit == BH_TEXT_COMMIT_ADDRESS)
    {
        e->step = NULL;
        if (!bh_sdl_text_begin(address_input, c))
        {
            bh_entity_release(c->pool, e);
            c->entity = NULL;
            return 0;
        }
        if (initial_paint)
            paint(c);
        return !c->error;
    }
    if (initial_paint)
        paint(c);
    if (!bh_input_set_enabled(c->input, c->snapshot, 1))
    {
        bh_entity_release(c->pool, e);
        c->entity = NULL;
        return 0;
    }
    return !c->error;
}
int __cdecl bh_input_text_begin(BHInputTextCapture *c)
{
    return begin(c, 0);
}
int __cdecl bh_input_text_begin_painted(BHInputTextCapture *c)
{
    return begin(c, 1);
}
int __cdecl bh_input_text_step(BHInputTextCapture *c)
{
    BHEntity *e;
    unsigned long p;
    long key;
    BHMenuSettingsRefresh refresh;
    if (!c || !c->entity || c->commit != BH_TEXT_COMMIT_PROFILE || !valid(c))
        return 0;
    e = c->entity;
    c->error = 0;
    if (!bh_player_input_apply_packet_checked(c->state, c->input->state, sizeof(c->input->state)))
    {
        c->error = 1;
        return 0;
    }
    p = phase(e);
    if (!p)
    {
        prime(c->state);
        set_phase(e, 1);
        return 1;
    }
    if (p == 1)
    {
        for (key = 0; key < 255; ++key)
        {
            if (bh_player_input_transition(c->state, (unsigned long)key) != 0)
                continue;
            sound(c, 1);
            if (key == 27)
                set_phase(e, 3);
            else if (key == 13)
                set_phase(e, 2);
            else if (key == 8)
            {
                sound(c, .7f);
                if (c->length > 0)
                    c->buffer[--c->length] = 0;
                else
                {
                    sound(c, .5f);
                    c->buffer[c->length] = 0;
                }
            }
            else
            {
                key = (long)bh_sdl_key_character((unsigned)key);
                if (key < 0)
                {
                    c->error = 1;
                    return 0;
                }
                if (key >= 32 && (key != 32 || c->length > 0))
                {
                    if (c->length < c->limit)
                    {
                        sound(c, 1);
                        c->buffer[c->length++] = (char)key;
                        c->buffer[c->length] = 0;
                    }
                    else
                    {
                        sound(c, .7f);
                        sound(c, .5f);
                    }
                }
            }
            paint(c);
        }
        return !c->error;
    }
    if (p != 2 && p != 3)
        return 1;
    if (p == 2)
        while (c->length > 0 && c->buffer[c->length - 1] == ' ')
            c->buffer[--c->length] = 0;
    prime(c->state);
    if (!bh_input_set_enabled(c->input, c->snapshot, 0))
        c->error = 1;
    c->commit = BH_TEXT_COMMIT_NONE;
    if (p == 2)
    {
        if (!bh_menu_profile_commit(c->profile, c->menu, c->buffer, c->menu->selection))
            c->error = 1;
    }
    else
    {
        refresh = c->menu->rows[c->menu->selection]->refresh;
        if (refresh)
            refresh(c->menu, c->menu->selection);
    }
    if (!bh_entity_release(c->pool, e))
        c->error = 1;
    else
        c->entity = NULL;
    return !c->error;
}
int __cdecl bh_input_text_cancel(BHInputTextCapture *c)
{
    int okay = 1;
    if (!c || !c->entity || c->pool->dispatch_depth)
        return 0;
    if (c->commit == BH_TEXT_COMMIT_ADDRESS)
        bh_sdl_text_end();
    else
        okay = bh_input_set_enabled(c->input, c->snapshot, 0);
    if (!bh_entity_release(c->pool, c->entity))
        return 0;
    c->entity = NULL;
    c->commit = BH_TEXT_COMMIT_NONE;
    return okay;
}
