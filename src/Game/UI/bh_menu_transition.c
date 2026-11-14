#include "bh_message_panel.h"
#include "bh_menu_transition.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <float.h>
static BHMenuTransition *context(BHEntity *entity)
{
    BHMenuTransition *c;
    memcpy(&c, &entity->bindings.menu_transition, 4);
    return c;
}
static void put_time(BHEntity *menu_entity, double t)
{
    memcpy((char *)&menu_entity->menu.transition_time, &t, 8);
}
double bh_menu_transition_time(const BHEntity *menu_entity)
{
    double t;
    memcpy(&t, (char *)&menu_entity->menu.transition_time, 8);
    return t;
}
static int live(BHMenuTransition *menu_transition, BHEntity *entity)
{
    unsigned long g;
    memcpy(&g, &entity->bindings.transition_generation, 4);
    return bh_entity_is_active(menu_transition->pool, entity) && entity->generation == g;
}
int bh_menu_transition_bind(BHMenuTransition *menu_transition, BHEntity *menu_entity, double t, long attempts)
{
    BHEntity *active_entity;
    unsigned long i = 0;
    if (!menu_transition || !menu_transition->pool || !menu_entity)
        return 0;
    for (active_entity = menu_transition->pool->active; active_entity && i++ < menu_transition->pool->capacity; active_entity = active_entity->next)
        if (active_entity == menu_entity)
            break;
    if (!active_entity)
        return 0;
    memcpy(&menu_entity->bindings.menu_transition, &menu_transition, 4);
    memcpy(&menu_entity->bindings.transition_generation, &menu_entity->generation, 4);
    put_time(menu_entity, t);
    memcpy((char *)&menu_entity->menu.attempts, &attempts, 4);
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
static void fade(BHMenuTransition *menu_transition, float alpha, unsigned long color)
{
    ++menu_transition->fades;
    if (bh_fade_queue(menu_transition->draw, alpha, color) < 0)
        menu_transition->error = 1;
}
void bh_menu_music_fade(BHEntity *e)
{
    BHMenuTransition *c = context(e);
    if (c && live(c, e))
        fade(c, clamp(bh_menu_transition_time(e) * .5), 0);
}
void bh_menu_fade_expiry(BHEntity *e)
{
    BHMenuTransition *menu_transition = context(e);
    double t;
    if (!menu_transition || !live(menu_transition, e))
        return;
    t = bh_menu_transition_time(e);
    if (_isnan(t) || t < -4)
    {
        ++menu_transition->releases;
        bh_entity_release(menu_transition->pool, e);
        return;
    }
    put_time(e, t - 1.0 / 30.0);
}
void bh_menu_quit_fade(BHEntity *e)
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
static void warning(BHMenuTransition *menu_transition)
{
    BHMessageState *message_state = menu_transition->messages;
    static const char text[] =
        "4Hey Buddy!\n\nThere's a problem\nwith your CD!\n\nInsert your BHunter CD\nand make sure you\nain't runnin' no other\nCD player programs!\n";
    float timer;
    if (!message_state)
    {
        menu_transition->error = 1;
        return;
    }
    if (message_state->arena_used > sizeof(message_state->arena) || sizeof(text) > sizeof(message_state->arena) - message_state->arena_used)
        return;
    memcpy(message_state->arena + message_state->arena_used, text, sizeof(text));
    message_state->arena_used += sizeof(text);
    ++menu_transition->warnings;
    if (message_state->panel_entity && message_state->panel_step_identity && *message_state->panel_step_identity == 0x442950)
    {
        memcpy(&timer, &bh_message_panel_fields(message_state->panel_entity)->timer, 4);
        if (timer > .5f)
        {
            timer = .5f;
            memcpy(&bh_message_panel_fields(message_state->panel_entity)->timer, &timer, 4);
        }
    }
}
static long attempts(BHEntity *menu_entity)
{
    long n;
    memcpy(&n, (char *)&menu_entity->menu.attempts, 4);
    return n;
}
static void retry(BHMenuTransition *c, BHEntity *menu_entity)
{
    unsigned long n = (unsigned long)attempts(menu_entity);
    if (!n)
        warning(c);
    put_time(menu_entity, 5);
    ++n;
    memcpy((char *)&menu_entity->menu.attempts, &n, 4);
}
void bh_menu_music_tick(BHEntity *e)
{
    BHMenuTransition *menu_transition = context(e);
    BHCdAudio *a;
    double t, v;
    if (!menu_transition || !live(menu_transition, e) || !menu_transition->media || !(a = menu_transition->media->audio))
        return;
    t = bh_menu_transition_time(e) - 1.0 / 30.0;
    put_time(e, t);
    if (!_isnan(t) && t > 0)
        return;
    if (!menu_transition->cd_ready)
    {
        menu_transition->cd_ready = bh_cd_audio_startup(a);
        if (!menu_transition->cd_ready)
        {
#ifdef BH_SKIP_CD_AUDIO_CHECK
            menu_transition->music = NULL;
            ++menu_transition->releases;
            bh_entity_release(menu_transition->pool, e);
            return;
#endif
            retry(menu_transition, e);
        }
        else
        {
            v = bh_cd_audio_volume(a);
            if (_isnan(v) || v < .2)
                bh_cd_set_volume(menu_transition->media, .2f);
            v = bh_wave_audio_volume(a);
            if (_isnan(v) || v < .2)
                bh_wave_set_volume(menu_transition->media, .2f);
        }
        if (!menu_transition->cd_ready)
            goto exhausted;
    }
    if (!menu_transition->range_ready)
    {
        bh_cd_stop(menu_transition->media);
        menu_transition->range_ready = bh_cd_play_range(menu_transition->media, 2, 2, 0x15, 0x32, 1);
        if (!menu_transition->range_ready)
        {
#ifdef BH_SKIP_CD_AUDIO_CHECK
            menu_transition->music = NULL;
            ++menu_transition->releases;
            bh_entity_release(menu_transition->pool, e);
            return;
#endif
            retry(menu_transition, e);
        }
        else
        {
            menu_transition->music = NULL;
            ++menu_transition->releases;
            bh_entity_release(menu_transition->pool, e);
            return;
        }
    }
exhausted:
    if (attempts(e) == 10)
    {
        warning(menu_transition);
        menu_transition->music = NULL;
        ++menu_transition->releases;
        bh_entity_release(menu_transition->pool, e);
    }
}
void bh_menu_quit_tick(BHEntity *entity)
{
    BHMenuTransition *menu_transition = context(entity);
    double old, t, volume;
    float next;
    if (!menu_transition || !live(menu_transition, entity) || !menu_transition->media || !menu_transition->media->audio)
        return;
    old = bh_menu_transition_time(entity);
    t = old - (-1.0 / 30.0);
    put_time(entity, t);
    /* Finite source contract avoids undefined CRT __ftol overflow/NaN inputs. */
    if (_isnan(old) || old < -214748364.0 || old > 214748364.0)
    {
        menu_transition->error = 1;
        return;
    }
    if ((long)(old * 10) != (long)(t * 10))
    {
        volume = bh_cd_audio_volume(menu_transition->media->audio);
        if (!_isnan(volume) && volume > 0)
        {
            bh_round_float(&next, volume - .05);
            if (volume - .05 < 0)
                bh_cd_stop(menu_transition->media);
            else
                bh_cd_set_volume(menu_transition->media, next);
        }
    }
    if (!_isnan(t) && t > 2.5)
    {
        if (menu_transition->quit)
            *menu_transition->quit = 1;
        entity->step = NULL;
        bh_cd_set_volume(menu_transition->media, menu_transition->saved_volume);
    }
}
static void quit_refresh(BHEntity *e, void *unused)
{
    (void)unused;
    bh_menu_quit_fade(e);
}
BHEntity *bh_menu_quit_begin(BHMenuTransition *menu_transition)
{
    BHEntity *entity;
    if (!menu_transition || !menu_transition->pool || !menu_transition->media || !menu_transition->media->audio || !menu_transition->config || !menu_transition->config_path)
        return NULL;
    memcpy(menu_transition->config->settings, &menu_transition->hardware_flags, 2);
    ++menu_transition->saves;
    if (!bh_config_save(menu_transition->config, menu_transition->config_path))
        menu_transition->error = 1;
    bh_round_float(&menu_transition->saved_volume, bh_cd_audio_volume(menu_transition->media->audio));
    if (menu_transition->camera_index)
        *menu_transition->camera_index = 17;
    entity = bh_entity_acquire(menu_transition->pool);
    if (entity)
    {
        bh_menu_transition_bind(menu_transition, entity, 0, 0);
        entity->step = bh_menu_quit_tick;
        entity->refresh = quit_refresh;
    }
    return entity;
}
