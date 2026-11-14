#include "bh_input_text.h"
#include "../Platform/bh_platform.h"
#include <stdio.h>
#include <string.h>
static unsigned long phase(BHEntity *menu_entity)
{
    unsigned long p;
    memcpy(&p, &menu_entity->menu.input_phase, 4);
    return p;
}
static void set_phase(BHEntity *menu_entity, unsigned long p)
{
    memcpy(&menu_entity->menu.input_phase, &p, 4);
}
static void prime(BHPlayerInputState *s)
{
    unsigned long i;
    for (i = 0; i < 255; ++i)
        bh_player_input_transition(s, i);
}
static int row_valid(BHInputTextCapture *input_text_capture)
{
    long i = input_text_capture->menu->selection;
    return i >= 0 && (unsigned long)i < input_text_capture->menu->row_count && input_text_capture->menu->rows && input_text_capture->menu->rows[i];
}
static int valid(BHInputTextCapture *input_text_capture)
{
    const char *end;
    if (!input_text_capture || !input_text_capture->pool || !input_text_capture->input || !input_text_capture->snapshot || !input_text_capture->state || !input_text_capture->menu || !input_text_capture->rows || !input_text_capture->sound || !input_text_capture->buffer ||
        !input_text_capture->capacity || input_text_capture->limit < 0 || (unsigned long)input_text_capture->limit >= input_text_capture->capacity)
        return 0;
    if (input_text_capture->limit > 15 ||
        (input_text_capture->commit == BH_TEXT_COMMIT_PROFILE ? !input_text_capture->profile : input_text_capture->commit != BH_TEXT_COMMIT_ADDRESS || !input_text_capture->address_commit))
        return 0;
    end = (const char *)memchr(input_text_capture->buffer, 0, input_text_capture->capacity);
    return end && input_text_capture->length >= 0 && input_text_capture->length <= input_text_capture->limit && end - input_text_capture->buffer == input_text_capture->length && row_valid(input_text_capture) &&
           (unsigned long)input_text_capture->menu->selection < input_text_capture->rows->count;
}
static void sound(BHInputTextCapture *input_text_capture, float pitch)
{
    ++input_text_capture->sounds;
    input_text_capture->last_voice = bh_sound_request(input_text_capture->sound, input_text_capture->sound_resource, .6f, pitch, NULL, NULL);
    if (input_text_capture->last_voice == BH_SOUND_INVALID)
        input_text_capture->error = 1;
}
static void paint(BHInputTextCapture *input_text_capture)
{
    unsigned long i, n = (unsigned long)strlen(input_text_capture->buffer), first = n > 17 ? n - 17 : 0;
    for (i = 0; i < n; ++i)
        if (input_text_capture->buffer[i] >= 'a' && input_text_capture->buffer[i] <= 'z')
            input_text_capture->buffer[i] -= 32;
    sprintf(input_text_capture->display, "%s_%s", input_text_capture->buffer + first, input_text_capture->commit == BH_TEXT_COMMIT_ADDRESS ? "\nA Join  B Back" : "");
    ++input_text_capture->repaints;
    if (!bh_menu_row_refresh(input_text_capture->rows, (unsigned long)input_text_capture->menu->selection, NULL, input_text_capture->display))
        input_text_capture->error = 1;
}
static void text_tick(BHEntity *entity)
{
    BHInputTextCapture *input_text_capture;
    memcpy(&input_text_capture, &entity->bindings.text_capture, 4);
    if (input_text_capture && input_text_capture->entity == entity)
        bh_input_text_step(input_text_capture);
}
static void address_input(BHPlatformTextEvent event, const char *text, void *user)
{
    BHInputTextCapture *input_text_capture = user;
    BHMenuSettingsRefresh refresh;
    long length = input_text_capture->length;
    if (event == BH_PLATFORM_TEXT_CANCEL || event == BH_PLATFORM_TEXT_SUBMIT)
    {
        if (!bh_input_text_cancel(input_text_capture))
            return;
        sound(input_text_capture, 1);
        if (event == BH_PLATFORM_TEXT_SUBMIT)
            input_text_capture->address_commit(input_text_capture->address_user, input_text_capture->buffer);
        else if ((refresh = input_text_capture->menu->rows[input_text_capture->menu->selection]->refresh) != NULL)
            refresh(input_text_capture->menu, input_text_capture->menu->selection);
        return;
    }
    if (event == BH_PLATFORM_TEXT_BACKSPACE)
    {
        if (input_text_capture->length > 0)
            --input_text_capture->length;
    }
    else if (event == BH_PLATFORM_TEXT_INSERT)
    {
        for (; *text && input_text_capture->length < input_text_capture->limit; ++text)
            if ((*text >= '0' && *text <= '9') || *text == '.')
                input_text_capture->buffer[input_text_capture->length++] = *text;
    }
    if (input_text_capture->length != length)
    {
        input_text_capture->buffer[input_text_capture->length] = 0;
        sound(input_text_capture, 1);
        paint(input_text_capture);
    }
}
static int begin(BHInputTextCapture *input_text_capture, int initial_paint)
{
    BHEntity *entity;
    if (!input_text_capture || input_text_capture->entity || !valid(input_text_capture))
        return 0;
    entity = bh_entity_acquire(input_text_capture->pool);
    if (!entity)
        return 0;
    set_phase(entity, 0);
    memcpy(&entity->bindings.text_capture, &input_text_capture, 4);
    entity->step = text_tick;
    input_text_capture->entity = entity;
    input_text_capture->error = 0;
    if (input_text_capture->commit == BH_TEXT_COMMIT_ADDRESS)
    {
        entity->step = NULL;
        if (!bh_sdl_text_begin(address_input, input_text_capture))
        {
            bh_entity_release(input_text_capture->pool, entity);
            input_text_capture->entity = NULL;
            return 0;
        }
        if (initial_paint)
            paint(input_text_capture);
        return !input_text_capture->error;
    }
    if (initial_paint)
        paint(input_text_capture);
    if (!bh_input_set_enabled(input_text_capture->input, input_text_capture->snapshot, 1))
    {
        bh_entity_release(input_text_capture->pool, entity);
        input_text_capture->entity = NULL;
        return 0;
    }
    return !input_text_capture->error;
}
int bh_input_text_begin(BHInputTextCapture *c)
{
    return begin(c, 0);
}
int bh_input_text_begin_painted(BHInputTextCapture *c)
{
    return begin(c, 1);
}
int bh_input_text_step(BHInputTextCapture *input_text_capture)
{
    BHEntity *e;
    unsigned long p;
    long key;
    BHMenuSettingsRefresh refresh;
    if (!input_text_capture || !input_text_capture->entity || input_text_capture->commit != BH_TEXT_COMMIT_PROFILE || !valid(input_text_capture))
        return 0;
    e = input_text_capture->entity;
    input_text_capture->error = 0;
    if (!bh_player_input_apply_packet_checked(input_text_capture->state, input_text_capture->input->state, sizeof(input_text_capture->input->state)))
    {
        input_text_capture->error = 1;
        return 0;
    }
    p = phase(e);
    if (!p)
    {
        prime(input_text_capture->state);
        set_phase(e, 1);
        return 1;
    }
    if (p == 1)
    {
        for (key = 0; key < 255; ++key)
        {
            if (bh_player_input_transition(input_text_capture->state, (unsigned long)key) != 0)
                continue;
            sound(input_text_capture, 1);
            if (key == 27)
                set_phase(e, 3);
            else if (key == 13)
                set_phase(e, 2);
            else if (key == 8)
            {
                sound(input_text_capture, .7f);
                if (input_text_capture->length > 0)
                    input_text_capture->buffer[--input_text_capture->length] = 0;
                else
                {
                    sound(input_text_capture, .5f);
                    input_text_capture->buffer[input_text_capture->length] = 0;
                }
            }
            else
            {
                key = (long)bh_sdl_key_character((unsigned)key);
                if (key < 0)
                {
                    input_text_capture->error = 1;
                    return 0;
                }
                if (key >= 32 && (key != 32 || input_text_capture->length > 0))
                {
                    if (input_text_capture->length < input_text_capture->limit)
                    {
                        sound(input_text_capture, 1);
                        input_text_capture->buffer[input_text_capture->length++] = (char)key;
                        input_text_capture->buffer[input_text_capture->length] = 0;
                    }
                    else
                    {
                        sound(input_text_capture, .7f);
                        sound(input_text_capture, .5f);
                    }
                }
            }
            paint(input_text_capture);
        }
        return !input_text_capture->error;
    }
    if (p != 2 && p != 3)
        return 1;
    if (p == 2)
        while (input_text_capture->length > 0 && input_text_capture->buffer[input_text_capture->length - 1] == ' ')
            input_text_capture->buffer[--input_text_capture->length] = 0;
    prime(input_text_capture->state);
    if (!bh_input_set_enabled(input_text_capture->input, input_text_capture->snapshot, 0))
        input_text_capture->error = 1;
    input_text_capture->commit = BH_TEXT_COMMIT_NONE;
    if (p == 2)
    {
        if (!bh_menu_profile_commit(input_text_capture->profile, input_text_capture->menu, input_text_capture->buffer, input_text_capture->menu->selection))
            input_text_capture->error = 1;
    }
    else
    {
        refresh = input_text_capture->menu->rows[input_text_capture->menu->selection]->refresh;
        if (refresh)
            refresh(input_text_capture->menu, input_text_capture->menu->selection);
    }
    if (!bh_entity_release(input_text_capture->pool, e))
        input_text_capture->error = 1;
    else
        input_text_capture->entity = NULL;
    return !input_text_capture->error;
}
int bh_input_text_cancel(BHInputTextCapture *input_text_capture)
{
    int okay = 1;
    if (!input_text_capture || !input_text_capture->entity || input_text_capture->pool->dispatch_depth)
        return 0;
    if (input_text_capture->commit == BH_TEXT_COMMIT_ADDRESS)
        bh_sdl_text_end();
    else
        okay = bh_input_set_enabled(input_text_capture->input, input_text_capture->snapshot, 0);
    if (!bh_entity_release(input_text_capture->pool, input_text_capture->entity))
        return 0;
    input_text_capture->entity = NULL;
    input_text_capture->commit = BH_TEXT_COMMIT_NONE;
    return okay;
}
