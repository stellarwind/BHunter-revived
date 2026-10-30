#include "bh_menu_transition.h"
#include "bh_round.h"
#include <string.h>
#include <float.h>
static BHMenuTransition *context(BHEntity *e)
{
    BHMenuTransition *c;
    memcpy(&c, e->payload + 600, 4);
    return c;
}
static void put_time(BHEntity *e, double t)
{
    memcpy((char *)e + 0x1f8, &t, 8);
}
double __cdecl bh_menu_transition_time(const BHEntity *e)
{
    double t;
    memcpy(&t, (const char *)e + 0x1f8, 8);
    return t;
}
static int live(BHMenuTransition *c, BHEntity *e)
{
    unsigned long g;
    memcpy(&g, e->payload + 604, 4);
    return bh_entity_is_active(c->pool, e) && e->generation == g;
}
int __cdecl bh_menu_transition_bind(BHMenuTransition *c, BHEntity *e, double t, long attempts)
{
    BHEntity *p;
    unsigned long i = 0;
    if (!c || !c->pool || !e)
        return 0;
    for (p = c->pool->active; p && i++ < c->pool->capacity; p = p->next)
        if (p == e)
            break;
    if (!p)
        return 0;
    memcpy(e->payload + 600, &c, 4);
    memcpy(e->payload + 604, &e->generation, 4);
    put_time(e, t);
    memcpy((char *)e + 0x1c0, &attempts, 4);
    return 1;
}
static float clamp(double v)
{
    float f;
    bh_round_float(&f, v);
    if (_isnan(v) || v < 0)
        return 0;
    if (f > 1)
        return 1;
    return f;
}
static void fade(BHMenuTransition *c, float alpha, unsigned long color)
{
    ++c->fades;
    if (bh_fade_queue(c->draw, alpha, color) < 0)
        c->error = 1;
}
void __cdecl bh_menu_music_fade(BHEntity *e)
{
    BHMenuTransition *c = context(e);
    if (c && live(c, e))
        fade(c, clamp(bh_menu_transition_time(e) * .5), 0);
}
void __cdecl bh_menu_fade_expiry(BHEntity *e)
{
    BHMenuTransition *c = context(e);
    double t;
    if (!c || !live(c, e))
        return;
    t = bh_menu_transition_time(e);
    if (_isnan(t) || t < -4)
    {
        ++c->releases;
        bh_entity_release(c->pool, e);
        return;
    }
    put_time(e, t - 1.0 / 30.0);
}
void __cdecl bh_menu_quit_fade(BHEntity *e)
{
    BHMenuTransition *c = context(e);
    double t, v;
    unsigned long color;
    float alpha;
    if (!c || !live(c, e))
        return;
    t = bh_menu_transition_time(e);
    alpha = clamp(t - 1.0);
    v = (2.5 - t) * 2;
    if (_isnan(v) || v < 0)
        v = 0;
    else if (v > 1)
        v = 1;
    color = (unsigned long)(v * 255);
    fade(c, alpha, color * 0x10101UL);
}
static void warning(BHMenuTransition *c)
{
    BHMessageState *m = c->messages;
    static const char text[] =
        "4Hey Buddy!\n\nThere's a problem\nwith your CD!\n\nInsert your BHunter CD\nand make sure you\nain't runnin' no other\nCD player programs!\n";
    float timer;
    if (!m)
    {
        c->error = 1;
        return;
    }
    if (m->arena_used > sizeof(m->arena) || sizeof(text) > sizeof(m->arena) - m->arena_used)
        return;
    memcpy(m->arena + m->arena_used, text, sizeof(text));
    m->arena_used += sizeof(text);
    ++c->warnings;
    if (m->panel_entity && m->panel_step_identity && *m->panel_step_identity == 0x442950)
    {
        memcpy(&timer, (char *)m->panel_entity + 0xe8, 4);
        if (timer > .5f)
        {
            timer = .5f;
            memcpy((char *)m->panel_entity + 0xe8, &timer, 4);
        }
    }
}
static long attempts(BHEntity *e)
{
    long n;
    memcpy(&n, (char *)e + 0x1c0, 4);
    return n;
}
static void retry(BHMenuTransition *c, BHEntity *e)
{
    unsigned long n = (unsigned long)attempts(e);
    if (!n)
        warning(c);
    put_time(e, 5);
    ++n;
    memcpy((char *)e + 0x1c0, &n, 4);
}
void __cdecl bh_menu_music_tick(BHEntity *e)
{
    BHMenuTransition *c = context(e);
    BHCdAudio *a;
    double t, v;
    if (!c || !live(c, e) || !c->media || !(a = c->media->audio))
        return;
#ifdef BH_SKIP_CD_AUDIO_CHECK
    /* Optional no-CD build: retire the music retry task without opening or
       playing a CD, posting the warning, or claiming a CD is ready. */
    c->music = NULL;
    ++c->releases;
    bh_entity_release(c->pool, e);
    return;
#endif
    t = bh_menu_transition_time(e) - 1.0 / 30.0;
    put_time(e, t);
    if (!_isnan(t) && t > 0)
        return;
    if (!c->cd_ready)
    {
        c->cd_ready = bh_cd_audio_startup(a, c->window);
        if (!c->cd_ready)
            retry(c, e);
        else
        {
            v = bh_cd_audio_volume(a);
            if (_isnan(v) || v < .2)
                bh_cd_set_volume(c->media, .2f);
            v = bh_wave_audio_volume(a);
            if (_isnan(v) || v < .2)
                bh_wave_set_volume(c->media, .2f);
        }
        if (!c->cd_ready)
            goto exhausted;
    }
    if (!c->range_ready)
    {
        bh_cd_stop(c->media);
        c->range_ready = bh_cd_play_range(c->media, 2, 2, 0x15, 0x32, 1);
        if (!c->range_ready)
            retry(c, e);
        else
        {
            c->music = NULL;
            ++c->releases;
            bh_entity_release(c->pool, e);
            return;
        }
    }
exhausted:
    if (attempts(e) == 10)
    {
        warning(c);
        c->music = NULL;
        ++c->releases;
        bh_entity_release(c->pool, e);
    }
}
void __cdecl bh_menu_quit_tick(BHEntity *e)
{
    BHMenuTransition *c = context(e);
    double old, t, volume;
    float next;
    if (!c || !live(c, e) || !c->media || !c->media->audio)
        return;
    old = bh_menu_transition_time(e);
    t = old - (-1.0 / 30.0);
    put_time(e, t);
    /* Finite source contract avoids undefined CRT __ftol overflow/NaN inputs. */
    if (_isnan(old) || old < -214748364.0 || old > 214748364.0)
    {
        c->error = 1;
        return;
    }
    if ((long)(old * 10) != (long)(t * 10))
    {
        volume = bh_cd_audio_volume(c->media->audio);
        if (!_isnan(volume) && volume > 0)
        {
            bh_round_float(&next, volume - .05);
            if (volume - .05 < 0)
                bh_cd_stop(c->media);
            else
                bh_cd_set_volume(c->media, next);
        }
    }
    if (!_isnan(t) && t > 2.5)
    {
        if (c->quit)
            *c->quit = 1;
        e->step = NULL;
        bh_cd_set_volume(c->media, c->saved_volume);
    }
}
static void __cdecl quit_refresh(BHEntity *e, void *unused)
{
    (void)unused;
    bh_menu_quit_fade(e);
}
BHEntity *__cdecl bh_menu_quit_begin(BHMenuTransition *c)
{
    BHEntity *e;
    if (!c || !c->pool || !c->media || !c->media->audio || !c->config || !c->config_path)
        return NULL;
    memcpy(c->config->settings, &c->hardware_flags, 2);
    ++c->saves;
    if (!bh_config_save(c->config, c->config_path))
        c->error = 1;
    bh_round_float(&c->saved_volume, bh_cd_audio_volume(c->media->audio));
    if (c->camera_index)
        *c->camera_index = 17;
    e = bh_entity_acquire(c->pool);
    if (e)
    {
        bh_menu_transition_bind(c, e, 0, 0);
        e->step = bh_menu_quit_tick;
        e->refresh = quit_refresh;
    }
    return e;
}
