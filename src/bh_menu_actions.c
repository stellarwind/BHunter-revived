#include "bh_menu_actions.h"
#include "bh_round.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
typedef char provider_size[sizeof(BHProviderRecord) == 188 && sizeof(BHMenuNetworkRecord) == 188 ? 1 : -1];
static long get(const void *p, unsigned long o)
{
    long v;
    memcpy(&v, (const unsigned char *)p + o, 4);
    return v;
}
static void put(void *p, unsigned long o, long v)
{
    memcpy((unsigned char *)p + o, &v, 4);
}
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
static BHProviderRecord *selected(BHMenuActions *c)
{
    return c && c->settings ? (BHProviderRecord *)c->settings->selected_network : NULL;
}
static int record(BHMenuActions *c)
{
    BHProviderRecord *r = selected(c);
    unsigned long i;
    if (!r || !c->providers || c->providers->count < 0 || c->providers->count > 8)
        return 0;
    for (i = 0; i < (unsigned long)c->providers->count; i++)
        if (c->providers->records + i == r)
            return 1;
    return 0;
}
static int connection_start(BHMenuActions *c)
{
    if (!record(c) || !c->sessions)
    {
        c->error = 1;
        return 0;
    }
    return bh_network_connection(c->sessions, selected(c));
}
static void launch(BHMenuActions *c, long page, int host, long row, float display)
{
    BHMenuActionsRoot *root = *c->root;
    if (!root || !root->menu || !root->row_actions || root->action_count < 2)
    {
        c->error = 1;
        return;
    }
    root->timeout = 2;
    root->page = page;
    root->row_actions[1] = host ? bh_menu_action_host : NULL;
    root = *c->root;
    if (!root || !root->menu)
    {
        c->error = 1;
        return;
    }
    root->menu->selection = row;
    root->menu->display_selection = display;
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
    t->settings = c->settings;
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
void __cdecl bh_menu_action_video(BHMenuActions *c, long row)
{
    long i, index = -1, value;
    if (!c || !c->config)
        return;
    c->error = 0;
    if (row == 0 || row == 2)
    {
        unsigned long offset = row == 0 ? 4 : 8;
        value = get(c->config->settings, offset);
        put(c->config->settings, offset, 1 - (value & 1));
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
            if (!_stricmp(c->devices[i].name, (char *)c->config->settings + 12))
                index = i;
        }
        if (++index >= (long)c->device_count)
            index = 0;
        strcpy((char *)c->config->settings + 12, c->devices[index].name);
    }
    refresh(c, row);
}
void __cdecl bh_menu_action_audio(BHMenuActions *c, long row)
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
void __cdecl bh_menu_action_profile(BHMenuActions *c, long row)
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
        if (p != exclude && !_stricmp((const char *)p + 12, name))
            return 1;
        p = p->next;
    } while (p != start);
    return 0;
}
void __cdecl bh_menu_action_profile_promote(BHMenuActions *c, long unused)
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
    sprintf((char *)clone + 12, "%s #%d", prefix, suffix);
    while (suffix < 9 && named(p, clone, (char *)clone + 12))
    {
        ++suffix;
        sprintf((char *)clone + 12, "%s #%d", prefix, suffix);
    }
    clone->flags = 1;
    *c->profiles->current = clone;
    if (c->settings)
        c->settings->input_profile = clone;
}
void __cdecl bh_menu_action_connect(BHMenuActions *c, long index)
{
    if (!c || !c->settings || !c->providers || !c->root || !c->network_failed)
        return;
    c->error = 0;
    if (index < 0 || index >= c->providers->count)
    {
        c->error = 1;
        return;
    }
    if (index > 0)
        c->settings->last_nonzero = index;
    c->settings->selected_network = (BHMenuNetworkRecord *)(c->providers->records + index);
    if (!connection_start(c))
    {
        *c->network_failed = 1;
        return;
    }
    launch(c, 10, 1, 1, 1);
}
static int persist_text(BHMenuActions *c, int modem)
{
    BHProviderRecord *r = selected(c);
    if (!record(c) || !bounded((char *)r->data, modem ? 96 : 128))
    {
        c->error = 1;
        return 0;
    }
    if (modem)
    {
        if (!bounded((char *)r->data + 96, 32))
        {
            c->error = 1;
            return 0;
        }
        strcpy((char *)c->config->settings + 268, (char *)r->data);
        strcpy((char *)c->config->settings + 364, (char *)r->data + 96);
    }
    else
        strcpy((char *)c->config->settings + 140, (char *)r->data);
    save(c);
    return 1;
}
void __cdecl bh_menu_action_modem(BHMenuActions *c, long row)
{
    BHProviderRecord *r;
    long i;
    if (!c || !c->config || !(r = selected(c)))
        return;
    c->error = 0;
    switch (row)
    {
    case 0:
        persist_text(c, 1);
        break;
    case 1:
        if (!c->modems || c->modems->count < 1 || c->modems->count > 8)
        {
            c->error = 1;
            return;
        }
        i = bh_modem_name_find(c->modems, (char *)r->data) + 1;
        if (i >= c->modems->count)
            i = 0;
        if (!bounded(c->modems->names[i], 96))
        {
            c->error = 1;
            return;
        }
        strcpy((char *)r->data, c->modems->names[i]);
        refresh(c, 1);
        break;
    case 2:
        begin_text(c, (char *)r->data + 96, 31, BH_TEXT_COMMIT_ADDRESS);
        break;
    case 3:
        if (!persist_text(c, 1))
            return;
        if (!connection_start(c))
        {
            *c->network_failed = 1;
            return;
        }
        if (!r->data[96])
            launch(c, 2, 1, 1, 1);
        else
            launch(c, 10, 0, 2, 2);
        break;
    default:
        break;
    }
}
void __cdecl bh_menu_action_tcpip(BHMenuActions *c, long row)
{
    BHProviderRecord *r;
    if (!c || !c->config || !(r = selected(c)))
        return;
    c->error = 0;
    if (row == 0)
        persist_text(c, 0);
    else if (row == 1)
        begin_text(c, (char *)r->data, 95, BH_TEXT_COMMIT_PHONE);
    else if (row == 2)
    {
        if (!persist_text(c, 0))
            return;
        if (!connection_start(c))
        {
            *c->network_failed = 1;
            return;
        }
        launch(c, 10, 1, 1, 1);
    }
}
void __cdecl bh_menu_action_serial(BHMenuActions *c, long row)
{
    BHProviderRecord *r;
    long value;
    if (!c || !c->config || !(r = selected(c)))
        return;
    c->error = 0;
    switch (row)
    {
    case 0:
        memcpy(c->config->settings + 396, r->data, 20);
        save(c);
        break;
    case 1:
        value = get(r->data, 0);
        put(r->data, 0, (value & 3) + 1);
        break;
    case 2:
        put(r->data, 4, bh_serial_baud_next(get(r->data, 4)));
        break;
    case 3:
        put(r->data, 8, bh_serial_stop_next(get(r->data, 8)));
        break;
    case 4:
        put(r->data, 12, bh_serial_parity_next(get(r->data, 12)));
        break;
    case 5:
        put(r->data, 16, bh_serial_flow_next(get(r->data, 16)));
        break;
    case 6:
        memcpy(c->config->settings + 396, r->data, 20);
        save(c);
        if (!connection_start(c))
            *c->network_failed = 1;
        else
            launch(c, 10, 1, 1, 1);
        break;
    default:
        break;
    }
    refresh(c, row);
}
void __cdecl bh_menu_action_disconnect(BHMenuActions *c, long unused)
{
    BHMenuActionsRoot *r;
    (void)unused;
    if (!c || !c->root || !(r = *c->root) || !c->sessions)
        return;
    c->error = 0;
    r->timeout = 0;
    bh_network_connection_dispose(c->sessions);
    c->sessions->count = 0;
}
void __cdecl bh_menu_action_graphics_notice(BHMenuActions *c, long unused)
{
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
        memcpy(&time, m->panel_entity->payload + 4, 4);
        if (time > .5f)
        {
            time = .5f;
            memcpy(m->panel_entity->payload + 4, &time, 4);
        }
    }
}
void __cdecl bh_menu_action_host(BHMenuActions *c, long unused)
{
    BHMenuActionsRoot *r;
    (void)unused;
    if (!c || !c->root || !(r = *c->root) || !c->frame_count || !c->mode)
        return;
    c->error = 0;
    r->timeout = 0;
    *c->frame_count = 0;
    bh_network_host(c->game, c->player_name);
    *c->mode = 2;
}
void __cdecl bh_menu_action_join(BHMenuActions *c, long row)
{
    BHMenuActionsRoot *r;
    long index = (long)((unsigned long)row - 2UL);
    if (!c || !c->root || !(r = *c->root) || !c->frame_count || !c->mode || !c->network_failed || !c->sessions)
        return;
    c->error = 0;
    r->timeout = 0;
    *c->frame_count = 0;
    if (index < 0 || index >= c->sessions->count)
    {
        *c->network_failed = 1;
        return;
    }
    if (!bh_network_join(c->game, &c->sessions->records[index].instance, c->player_name))
    {
        *c->network_failed = 1;
        return;
    }
    *c->mode = 3;
}
