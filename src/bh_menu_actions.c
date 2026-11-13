#include "bh_message_panel.h"
#include "bh_menu_actions.h"
#include "bh_platform.h"
#include "bh_round.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static int bounded(const char *s, unsigned long n)
{
    return s && memchr(s, 0, n) != NULL;
}
static int refresh(BHMenuActions *c, long row)
{
    BHMenuSettingsRefresh f;
    if (!c->menu || row < 0 || (unsigned long)row >= c->menu->row_count || !c->menu->rows || !c->menu->rows[row])
    {
        c->error = 1;
        return 0;
    }
    f = c->menu->rows[row]->refresh;
    if (f)
    {
        ++c->refreshes;
        f(c->menu, row);
    }
    return 1;
}
static void paint(BHMenuActions *c, long row, const char *text)
{
    unsigned char body[128];
    unsigned long n;
    if (!c->rows || !bounded(text, 128))
    {
        c->error = 1;
        return;
    }
    n = (unsigned long)strlen(text);
    memcpy(body, text, n + 1);
    ++c->paints;
    if (!bh_menu_row_refresh(c->rows, (unsigned long)row, NULL, body))
        c->error = 2;
}
static void save(BHMenuActions *c)
{
    ++c->saves;
    if (!bh_config_save(c->config, c->save_path))
        c->error = 3;
}
static void begin_text(BHMenuActions *c, const char *initial, long limit, BHInputTextCommit commit)
{
    BHInputTextCapture *t = c->text, saved;
    unsigned long n, old_size;
    char previous[160];
    if (!t || t->entity || !t->buffer || !bounded(initial, t->capacity) ||
        !bounded(t->buffer, t->capacity < sizeof(previous) ? t->capacity : sizeof(previous)))
    {
        c->error = 1;
        return;
    }
    n = (unsigned long)strlen(initial);
    if (n > (unsigned long)limit)
    {
        c->error = 1;
        return;
    }
    saved = *t;
    old_size = (unsigned long)strlen(t->buffer) + 1;
    memcpy(previous, t->buffer, old_size);
    memcpy(t->buffer, initial, n + 1);
    t->length = (long)n;
    t->limit = limit;
    t->commit = commit;
    t->menu = c->menu;
    t->rows = c->rows;
    t->profile = c->profiles;
    if (!bh_input_text_begin_painted(t))
    {
        c->error = 4;
        if (!t->entity)
        {
            memcpy(saved.buffer, previous, old_size);
            *t = saved;
        }
    }
}
void bh_menu_action_network_host(BHMenuActions *c, long row)
{
    (void)row;
    if (c->network_start)
        c->network_start(c->network_user, NULL);
}
void bh_menu_action_network_join(BHMenuActions *c, long row)
{
    (void)row;
    if (c->network_address)
        begin_text(c, c->network_address, 15, BH_TEXT_COMMIT_ADDRESS);
}
void bh_menu_network_address(BHMenuActions *c, long row)
{
    paint(c, row, c->network_address && *c->network_address ? c->network_address : "Enter host IPv4");
}
void bh_menu_action_video(BHMenuActions *c, long row)
{
#ifdef BH_SDL_BACKEND
    if (!c || !c->config)
        return;
    c->error = 0;
    bh_sdl_video_cycle((int)row);
    refresh(c, 0);
    refresh(c, 1);
    refresh(c, 2);
#else
    long i, index = -1, value;
    if (!c || !c->config)
        return;
    c->error = 0;
    if (row == 0 || row == 2)
    {
        long *setting = row == 0 ? &c->config->values.hardware_acceleration : &c->config->values.resolution;
        value = *setting;
        *setting = 1 - (value & 1);
    }
    else if (row == 1)
    {
        if (!c->devices || !c->device_count || c->device_count > 4096)
        {
            c->error = 1;
            return;
        }
        for (i = 0; i < (long)c->device_count; i++)
        {
            if (!bounded(c->devices[i].name, 128))
            {
                c->error = 1;
                return;
            }
            if (!_stricmp(c->devices[i].name, c->config->values.device_name))
                index = i;
        }
        if (++index >= (long)c->device_count)
            index = 0;
        strcpy(c->config->values.device_name, c->devices[index].name);
    }
    refresh(c, row);
#endif
}

void bh_menu_action_audio(BHMenuActions *c, long row)
{
    double v;
    long n, percent;
    float volume;
    char body[128];
    int cd;
    if (!c || !c->media || !c->media->audio)
        return;
    c->error = 0;
    if (row < 0 || row > 3)
        return;
    cd = row >= 2;
    v = cd ? bh_cd_audio_volume(c->media->audio) : bh_wave_audio_volume(c->media->audio);
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
        bh_cd_set_volume(c->media, volume);
    else
        bh_wave_set_volume(c->media, volume);
    v = cd ? bh_cd_audio_volume(c->media->audio) : bh_wave_audio_volume(c->media->audio);
    percent = (long)(__int64)(.5 - v * (-10.0)) * 10;
    sprintf(body, "%d%% %s", percent, "Volume");
    paint(c, cd ? 2 : 0, body);
    paint(c, cd ? 3 : 1, body);
}
void bh_menu_action_profile(BHMenuActions *c, long row)
{
    BHInputProfile *p, *previous;
    if (!c || !c->profiles || !c->profiles->current || !(p = *c->profiles->current))
        return;
    c->error = 0;
    if (row == 0)
    {
        *c->profiles->current = p->next;
        if (c->settings)
            c->settings->input_profile = p->next;
        refresh(c, 0);
    }
    else if (row == 1)
        begin_text(c, "", 15, BH_TEXT_COMMIT_PROFILE);
    else if (row == 3)
    {
        if (p->flags && p->next != p)
        {
            previous = bh_config_profile_remove(c->config, p);
            if (!previous)
            {
                c->error = 1;
                return;
            }
            *c->profiles->current = previous;
            if (c->settings)
                c->settings->input_profile = previous;
            refresh(c, 0);
            if (c->profiles->pending_selection)
                *c->profiles->pending_selection = 0;
        }
        else
        {
            bh_sound_request(c->sound, c->denial_sound, .6f, .7f, NULL, NULL);
            bh_sound_request(c->sound, c->denial_sound, .6f, .5f, NULL, NULL);
        }
    }
}
static int named(BHInputProfile *start, BHInputProfile *exclude, const char *name)
{
    BHInputProfile *p = start;
    do
    {
        if (p != exclude && !_stricmp(p->name, name))
            return 1;
        p = p->next;
    } while (p != start);
    return 0;
}
void bh_menu_action_profile_promote(BHMenuActions *c, long unused)
{
    BHInputProfile *p, *clone, *next, *previous;
    const BHInputProfile *base;
    BHInputProfile normalized;
    char prefix[13];
    int suffix = 1;
    (void)unused;
    if (!c || !c->profiles || !c->profiles->current || !(p = *c->profiles->current) || !c->profiles->pending_selection)
        return;
    c->error = 0;
    *c->profiles->pending_selection = 0;
    if (p->flags)
        return;
    if (!bh_config_valid(c->config) || !(base = p->baseline) || !bounded(c->player_name, 32))
    {
        c->error = 1;
        return;
    }
    normalized = *p;
    normalized.next = normalized.previous = NULL;
    normalized.baseline = NULL;
    if (!memcmp(&normalized, base, 76))
        return;
    clone = bh_config_profile_clone_after(c->config, p);
    if (!clone)
    {
        c->error = 5;
        return;
    }
    next = p->next;
    previous = p->previous;
    *p = *base;
    p->next = next;
    p->previous = previous;
    p->baseline = base;
    strncpy(prefix, c->player_name, 12);
    prefix[12] = 0;
    sprintf(clone->name, "%s #%d", prefix, suffix);
    while (suffix < 9 && named(p, clone, clone->name))
    {
        ++suffix;
        sprintf(clone->name, "%s #%d", prefix, suffix);
    }
    clone->flags = 1;
    *c->profiles->current = clone;
    if (c->settings)
        c->settings->input_profile = clone;
}
void bh_menu_action_graphics_notice(BHMenuActions *c, long unused)
{
#ifdef BH_SDL_BACKEND
    (void)unused;
    if (c)
        c->error = 0;
    return;
#else
    static const char text[] = "4Please exit and\nrestart the game\nfor any graphic\nchanges to take\neffect!\n";
    BHMessageState *m;
    float time;
    (void)unused;
    if (!c || !(m = c->messages))
        return;
    c->error = 0;
    if (m->arena_used > 8192)
    {
        c->error = 1;
        return;
    }
    if (sizeof(text) > 8192 - m->arena_used)
        return;
    memcpy(m->arena + m->arena_used, text, sizeof(text));
    m->arena_used += sizeof(text);
    if (m->panel_entity && m->panel_step_identity && *m->panel_step_identity == 0x442950UL)
    {
        memcpy(&time, &bh_message_panel_fields(m->panel_entity)->timer, 4);
        if (time > .5f)
        {
            time = .5f;
            memcpy(&bh_message_panel_fields(m->panel_entity)->timer, &time, 4);
        }
    }
#endif
}
