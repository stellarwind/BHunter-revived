#include "bh_message_panel.h"
#include "bh_menu_actions.h"
#include "../../Platform/bh_platform.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static int bounded(const char *s, unsigned long n)
{
    return s && memchr(s, 0, n) != NULL;
}
static int refresh(BHMenuActions *menu_actions, long row)
{
    BHMenuSettingsRefresh f;
    if (!menu_actions->menu || row < 0 || (unsigned long)row >= menu_actions->menu->row_count || !menu_actions->menu->rows || !menu_actions->menu->rows[row])
    {
        menu_actions->error = 1;
        return 0;
    }
    f = menu_actions->menu->rows[row]->refresh;
    if (f)
    {
        ++menu_actions->refreshes;
        f(menu_actions->menu, row);
    }
    return 1;
}
static void paint(BHMenuActions *menu_actions, long row, const char *text)
{
    unsigned char body[128];
    unsigned long n;
    if (!menu_actions->rows || !bounded(text, 128))
    {
        menu_actions->error = 1;
        return;
    }
    n = (unsigned long)strlen(text);
    memcpy(body, text, n + 1);
    ++menu_actions->paints;
    if (!bh_menu_row_refresh(menu_actions->rows, (unsigned long)row, NULL, body))
        menu_actions->error = 2;
}
static void save(BHMenuActions *menu_actions)
{
    ++menu_actions->saves;
    if (!bh_config_save(menu_actions->config, menu_actions->save_path))
        menu_actions->error = 3;
}
static void begin_text(BHMenuActions *menu_actions, const char *initial, long limit, BHInputTextCommit commit)
{
    BHInputTextCapture *input_text_capture = menu_actions->text, saved;
    unsigned long n, old_size;
    char previous[160];
    if (!input_text_capture || input_text_capture->entity || !input_text_capture->buffer || !bounded(initial, input_text_capture->capacity) ||
        !bounded(input_text_capture->buffer, input_text_capture->capacity < sizeof(previous) ? input_text_capture->capacity : sizeof(previous)))
    {
        menu_actions->error = 1;
        return;
    }
    n = (unsigned long)strlen(initial);
    if (n > (unsigned long)limit)
    {
        menu_actions->error = 1;
        return;
    }
    saved = *input_text_capture;
    old_size = (unsigned long)strlen(input_text_capture->buffer) + 1;
    memcpy(previous, input_text_capture->buffer, old_size);
    memcpy(input_text_capture->buffer, initial, n + 1);
    input_text_capture->length = (long)n;
    input_text_capture->limit = limit;
    input_text_capture->commit = commit;
    input_text_capture->menu = menu_actions->menu;
    input_text_capture->rows = menu_actions->rows;
    input_text_capture->profile = menu_actions->profiles;
    if (!bh_input_text_begin_painted(input_text_capture))
    {
        menu_actions->error = 4;
        if (!input_text_capture->entity)
        {
            memcpy(saved.buffer, previous, old_size);
            *input_text_capture = saved;
        }
    }
}
void bh_menu_action_network_host(BHMenuActions *menu_actions, long row)
{
    (void)row;
    if (menu_actions->network_start)
        menu_actions->network_start(menu_actions->network_user, NULL);
}
void bh_menu_action_network_join(BHMenuActions *menu_actions, long row)
{
    (void)row;
    if (menu_actions->network_address)
        begin_text(menu_actions, menu_actions->network_address, 15, BH_TEXT_COMMIT_ADDRESS);
}
void bh_menu_network_address(BHMenuActions *menu_actions, long row)
{
    paint(menu_actions, row, menu_actions->network_address && *menu_actions->network_address ? menu_actions->network_address : "Enter host IPv4");
}
void bh_menu_action_video(BHMenuActions *menu_actions, long row)
{
#ifdef BH_SDL_BACKEND
    if (!menu_actions || !menu_actions->config)
        return;
    menu_actions->error = 0;
    bh_sdl_video_cycle((int)row);
    refresh(menu_actions, 0);
    refresh(menu_actions, 1);
    refresh(menu_actions, 2);
#else
    long i, index = -1, value;
    if (!menu_actions || !menu_actions->config)
        return;
    menu_actions->error = 0;
    if (row == 0 || row == 2)
    {
        long *setting = row == 0 ? &menu_actions->config->values.hardware_acceleration : &menu_actions->config->values.resolution;
        value = *setting;
        *setting = 1 - (value & 1);
    }
    else if (row == 1)
    {
        if (!menu_actions->devices || !menu_actions->device_count || menu_actions->device_count > 4096)
        {
            menu_actions->error = 1;
            return;
        }
        for (i = 0; i < (long)menu_actions->device_count; i++)
        {
            if (!bounded(menu_actions->devices[i].name, 128))
            {
                menu_actions->error = 1;
                return;
            }
            if (!_stricmp(menu_actions->devices[i].name, menu_actions->config->values.device_name))
                index = i;
        }
        if (++index >= (long)menu_actions->device_count)
            index = 0;
        strcpy(menu_actions->config->values.device_name, menu_actions->devices[index].name);
    }
    refresh(menu_actions, row);
#endif
}

void bh_menu_action_audio(BHMenuActions *menu_actions, long row)
{
    double v;
    long n, percent;
    float volume;
    char body[128];
    int cd;
    if (!menu_actions || !menu_actions->media || !menu_actions->media->audio)
        return;
    menu_actions->error = 0;
    if (row < 0 || row > 3)
        return;
    cd = row >= 2;
    v = cd ? bh_cd_audio_volume(menu_actions->media->audio) : bh_wave_audio_volume(menu_actions->media->audio);
    if (v == ((row & 1) ? 0 : 1))
        return;
    n = (long)(__int64)(((row & 1) ? -.5 : 1.5) - v * (-10.0));
    if (row & 1)
    {
        if (n < 0)
            n = 0;
    }
    else if (n > 10)
        n = 10;
    bh_round_float(&volume, (double)n * .1);
    if (cd)
        bh_cd_set_volume(menu_actions->media, volume);
    else
        bh_wave_set_volume(menu_actions->media, volume);
    v = cd ? bh_cd_audio_volume(menu_actions->media->audio) : bh_wave_audio_volume(menu_actions->media->audio);
    percent = (long)(__int64)(.5 - v * (-10.0)) * 10;
    sprintf(body, "%d%% %s", percent, "Volume");
    paint(menu_actions, cd ? 2 : 0, body);
    paint(menu_actions, cd ? 3 : 1, body);
}
void bh_menu_action_profile(BHMenuActions *menu_actions, long row)
{
    BHInputProfile *input_profile, *previous;
    if (!menu_actions || !menu_actions->profiles || !menu_actions->profiles->current || !(input_profile = *menu_actions->profiles->current))
        return;
    menu_actions->error = 0;
    if (row == 0)
    {
        *menu_actions->profiles->current = input_profile->next;
        if (menu_actions->settings)
            menu_actions->settings->input_profile = input_profile->next;
        refresh(menu_actions, 0);
    }
    else if (row == 1)
        begin_text(menu_actions, "", 15, BH_TEXT_COMMIT_PROFILE);
    else if (row == 3)
    {
        if (input_profile->flags && input_profile->next != input_profile)
        {
            previous = bh_config_profile_remove(menu_actions->config, input_profile);
            if (!previous)
            {
                menu_actions->error = 1;
                return;
            }
            *menu_actions->profiles->current = previous;
            if (menu_actions->settings)
                menu_actions->settings->input_profile = previous;
            refresh(menu_actions, 0);
            if (menu_actions->profiles->pending_selection)
                *menu_actions->profiles->pending_selection = 0;
        }
        else
        {
            bh_sound_request(menu_actions->sound, menu_actions->denial_sound, .6f, .7f, NULL, NULL);
            bh_sound_request(menu_actions->sound, menu_actions->denial_sound, .6f, .5f, NULL, NULL);
        }
    }
}
static int named(BHInputProfile *start, BHInputProfile *exclude, const char *name)
{
    BHInputProfile *input_profile = start;
    do
    {
        if (input_profile != exclude && !_stricmp(input_profile->name, name))
            return 1;
        input_profile = input_profile->next;
    } while (input_profile != start);
    return 0;
}
void bh_menu_action_profile_promote(BHMenuActions *menu_actions, long unused)
{
    BHInputProfile *input_profile, *clone, *next, *previous;
    const BHInputProfile *base;
    BHInputProfile normalized;
    char prefix[13];
    int suffix = 1;
    (void)unused;
    if (!menu_actions || !menu_actions->profiles || !menu_actions->profiles->current || !(input_profile = *menu_actions->profiles->current) || !menu_actions->profiles->pending_selection)
        return;
    menu_actions->error = 0;
    *menu_actions->profiles->pending_selection = 0;
    if (input_profile->flags)
        return;
    if (!bh_config_valid(menu_actions->config) || !(base = input_profile->baseline) || !bounded(menu_actions->player_name, 32))
    {
        menu_actions->error = 1;
        return;
    }
    normalized = *input_profile;
    normalized.next = normalized.previous = NULL;
    normalized.baseline = NULL;
    if (!memcmp(&normalized, base, 76))
        return;
    clone = bh_config_profile_clone_after(menu_actions->config, input_profile);
    if (!clone)
    {
        menu_actions->error = 5;
        return;
    }
    next = input_profile->next;
    previous = input_profile->previous;
    *input_profile = *base;
    input_profile->next = next;
    input_profile->previous = previous;
    input_profile->baseline = base;
    strncpy(prefix, menu_actions->player_name, 12);
    prefix[12] = 0;
    sprintf(clone->name, "%s #%d", prefix, suffix);
    while (suffix < 9 && named(input_profile, clone, clone->name))
    {
        ++suffix;
        sprintf(clone->name, "%s #%d", prefix, suffix);
    }
    clone->flags = 1;
    *menu_actions->profiles->current = clone;
    if (menu_actions->settings)
        menu_actions->settings->input_profile = clone;
}
void bh_menu_action_graphics_notice(BHMenuActions *menu_actions, long unused)
{
#ifdef BH_SDL_BACKEND
    (void)unused;
    if (menu_actions)
        menu_actions->error = 0;
    return;
#else
    static const char text[] = "4Please exit and\nrestart the game\nfor any graphic\nchanges to take\neffect!\n";
    BHMessageState *message_state;
    float time;
    (void)unused;
    if (!menu_actions || !(message_state = menu_actions->messages))
        return;
    menu_actions->error = 0;
    if (message_state->arena_used > 8192)
    {
        menu_actions->error = 1;
        return;
    }
    if (sizeof(text) > 8192 - message_state->arena_used)
        return;
    memcpy(message_state->arena + message_state->arena_used, text, sizeof(text));
    message_state->arena_used += sizeof(text);
    if (message_state->panel_entity && message_state->panel_step_identity && *message_state->panel_step_identity == 0x442950UL)
    {
        memcpy(&time, &bh_message_panel_fields(message_state->panel_entity)->timer, 4);
        if (time > .5f)
        {
            time = .5f;
            memcpy(&bh_message_panel_fields(message_state->panel_entity)->timer, &time, 4);
        }
    }
#endif
}
