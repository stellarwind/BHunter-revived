#include "bh_menu_runtime.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <float.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static int fail(BHMenuRuntimeContext *c)
{
    if (c)
        c->error = 1;
    return 0;
}
static BHMenuSettingsMenu *menu(BHMenuRuntime *m)
{
    return m->actions->menu;
}
static int valid(BHMenuRuntime *m)
{
    BHMenuSettingsMenu *s;
    if (!m || !m->context || !m->entity || !m->actions || !(s = m->actions->menu) || !s->rows || !s->row_count ||
        s->row_count > 4096 || !m->destinations || !m->input || !m->actions->row_actions ||
        m->actions->action_count < s->row_count || s->selection < 0 || (unsigned long)s->selection >= s->row_count)
        return 0;
    return 1;
}
static int actionable(BHMenuRuntime *m, long i)
{
    return m->destinations[i] || m->actions->row_actions[i] || (m->edit_actions && m->edit_actions[i]);
}
static int blocked(BHMenuRuntime *m)
{
    if (m->input_blocked)
        return 1;
    if (m->action_context && m->action_context->text && m->action_context->text->entity &&
        m->action_context->text->menu == menu(m))
        return 1;
    return m->edit_context && m->edit_context->capture && m->edit_context->capture->entity &&
           m->edit_context->capture->menu == menu(m);
}
static int choose(BHMenuRuntime *m, int direction, int advance)
{
    BHMenuSettingsMenu *s = menu(m);
    unsigned long i;
    for (i = 0; i < s->row_count; ++i)
    {
        if (advance)
        {
            s->selection += direction;
            if (s->selection < 0)
                s->selection = (long)s->row_count - 1;
            if ((unsigned long)s->selection >= s->row_count)
                s->selection = 0;
        }
        if (actionable(m, s->selection))
            return 1;
        advance = 1;
    }
    return fail(m->context);
}
static void refresh(BHMenuRuntime *m)
{
    unsigned long i;
    BHMenuSettingsMenu *s = menu(m);
    for (i = 0; i < s->row_count; ++i)
        if (s->rows[i] && s->rows[i]->refresh)
        {
            ++m->context->refreshes;
            s->rows[i]->refresh(s, (long)i);
        }
}
static unsigned long sound(BHMenuRuntimeContext *c, unsigned long resource, float level)
{
    ++c->sounds;
    return bh_sound_request(c->sound, resource, level, 1, NULL, NULL);
}
static void stop_slide(BHMenuRuntime *m)
{
    if (m->slide_voice != BH_SOUND_INVALID)
    {
        bh_sound_stop(m->context->sound, m->slide_voice);
        m->slide_voice = BH_SOUND_INVALID;
    }
}
static void drain(BHMenuRuntime *m, unsigned long first, unsigned long last)
{
    unsigned long i;
    for (i = first; i <= last; ++i)
        bh_player_input_transition(&m->input_state, i);
}
static int activate(BHMenuRuntime *m)
{
    if (!valid(m))
        return fail(m ? m->context : NULL);
    m->entity->step = bh_menu_runtime_step_entity;
    m->entity->refresh = bh_menu_runtime_refresh_entity;
    if (m->waypoint >= 0)
        *m->context->waypoint = m->waypoint;
    m->distance = 100;
    m->exiting = 0;
    if (!bh_input_set_enabled(m->input, m->context->snapshot, 1))
        m->context->error = 1;
    refresh(m);
    if (!valid(m))
        return fail(m->context);
    return choose(m, 1, 0);
}
int __cdecl bh_menu_runtime_bind(BHMenuRuntime *m)
{
    BHEntity *p;
    unsigned long n = 0;
    if (!valid(m) || !m->context->pool || !m->context->snapshot || !m->context->sound || !m->context->waypoint ||
        !m->context->stay || !m->context->input_changed || !m->context->mode || !m->context->paused)
        return 0;
    for (p = m->context->pool->active; p && n++ < m->context->pool->capacity; p = p->next)
        if (p == m->entity)
            break;
    if (!p)
        return 0;
    memcpy(m->entity->payload + 608, &m, 4);
    m->entity->step = bh_menu_runtime_step_entity;
    m->entity->refresh = bh_menu_runtime_refresh_entity;
    return 1;
}
static BHMenuRuntime *binding(BHEntity *e)
{
    BHMenuRuntime *m;
    memcpy(&m, e->payload + 608, 4);
    return m;
}
void __cdecl bh_menu_runtime_step_entity(BHEntity *e)
{
    BHMenuRuntime *m = binding(e);
    if (m && m->entity == e)
        bh_menu_runtime_step(m);
}
void __cdecl bh_menu_runtime_refresh_entity(BHEntity *e, void *unused)
{
    BHMenuRuntime *m = binding(e);
    BHMenuPresent *p;
    (void)unused;
    if (!m || m->entity != e || !(p = m->present))
        return;
    p->phase = m->phase;
    p->selection = menu(m)->selection;
    p->display_selection = menu(m)->display_selection;
    p->push = m->push;
    p->distance = m->distance;
    p->capture = (unsigned long)m->capture;
    p->application_mode = *m->context->mode;
    bh_menu_present(p);
}
static int press(BHMenuRuntime *m, int back)
{
    BHMenuRuntimeContext *c = m->context;
    BHMenuSettingsMenu *s = menu(m);
    BHMenuRuntime *destination;
    BHMenuAction action;
    long selected;
    if (!back)
        *c->input_changed = 1;
    sound(c, c->move_sound, .6f);
    if (back)
        s->selection = m->back_selection;
    selected = s->selection;
    if (selected < 0 || (unsigned long)selected >= s->row_count)
        return fail(c);
    action = m->actions->row_actions[selected];
    if (action)
    {
        if (!m->action_context)
            return fail(c);
        m->action_context->menu = s;
        action(m->action_context, selected);
    }
    else if (m->edit_actions && m->edit_actions[selected])
    {
        if (!m->edit_context)
            return fail(c);
        m->edit_context->menu = s;
        m->edit_actions[selected](m->edit_context, selected);
    }
    if (!valid(m))
        return fail(c);
    selected = s->selection;
    destination = m->destinations[selected];
    if (*c->stay || destination == m)
    {
        *c->stay = 0;
        m->capture = 1;
        return choose(m, 1, 0);
    }
    sound(c, c->open_sound, 1);
    drain(m, 4, 10);
    bh_input_set_enabled(m->input, c->snapshot, 0);
    m->exiting = 1;
    if (destination && !activate(destination))
        return 0;
    return 1;
}
static void sessions(BHMenuRuntime *m)
{
    BHMenuRuntimeContext *c = m->context;
    BHMenuSettingsMenu *s = menu(m);
    const BHProviderRecord *p;
    GUID selected;
    int retained = 0;
    long i, index;
    double next;
    if (!(m->actions->timeout > 0))
        return;
    next = (double)m->actions->timeout - (1.0 / 30.0);
    m->actions->timeout = rf(next);
    if (next > 0)
        return;
    if (!c->provider || !(p = *c->provider) || !c->sessions)
    {
        c->error = 1;
        return;
    }
    if (p->kind == 2 && !p->data[96])
        return;
    m->actions->timeout = rf((double)m->actions->timeout + 1);
    index = s->selection - 2;
    if (index >= 0 && index < c->sessions->count)
    {
        selected = c->sessions->records[index].instance;
        retained = 1;
    }
    bh_network_sessions_refresh(c->sessions, p);
    if (retained)
        for (i = 0; i < c->sessions->count; ++i)
            if (!memcmp(&selected, &c->sessions->records[i].instance, sizeof(GUID)))
                s->selection = i + 2;
    refresh(m);
}
int __cdecl bh_menu_runtime_step(BHMenuRuntime *m)
{
    BHMenuRuntimeContext *c;
    BHMenuSettingsMenu *s;
    double v, next;
    float f;
    short mouse;
    unsigned long raw;
    int pressed;
    unsigned char a, b, d, e;
    if (!valid(m))
        return fail(m ? m->context : NULL);
    c = m->context;
    s = menu(m);
    if (!bh_math_environment_supported() || !_finite(s->display_selection) || !_finite(m->push) ||
        !_finite(m->distance) || !_finite(m->scroll) || !_finite(m->actions->timeout))
        return fail(c);
    if (*c->paused == 1)
        return 1;
    ++m->phase;
    if (m->pending_selection >= 0)
    {
        if ((unsigned long)m->pending_selection >= s->row_count)
            return fail(c);
        s->selection = m->pending_selection;
    }
    m->pending_selection = -1;
    sessions(m);
    if (!valid(m))
        return fail(c);
    if ((m == c->ship_menus[0] || m == c->ship_menus[1]) && s->selection < 3)
        *c->waypoint = s->selection + 14;
    if (!m->exiting && !blocked(m))
    {
        bh_player_input_apply_packet(&m->input_state, m->input->state);
        if (m->distance >= 10)
        {
            drain(m, 0, 1);
            drain(m, 4, 10);
        }
        else
        {
            raw = m->input_state.values[2];
            mouse = (short)(raw >> 16);
            m->scroll = rf((double)m->scroll - (double)mouse * .0025);
            memcpy(&f, m->input_state.values + 3, 4);
            if (!_finite(f))
                return fail(c);
            if (f < -.35)
                m->scroll = rf((double)m->scroll - (-.1));
            else if (f > .35)
                m->scroll = rf((double)m->scroll - .1);
            a = bh_player_input_transition(&m->input_state, 0);
            if (a == 0 || m->scroll > 1)
            {
                if (m->scroll > 1)
                    m->scroll = rf((double)m->scroll - 1);
                *c->input_changed = 1;
                sound(c, c->move_sound, .6f);
                if (!choose(m, -1, 1))
                    return 0;
            }
            a = bh_player_input_transition(&m->input_state, 1);
            if (a == 0 || m->scroll < -1)
            {
                if (m->scroll < -1)
                    m->scroll = rf((double)m->scroll + 1);
                *c->input_changed = 1;
                sound(c, c->move_sound, .6f);
                if (!choose(m, 1, 1))
                    return 0;
            }
            a = bh_player_input_transition(&m->input_state, 4);
            b = bh_player_input_transition(&m->input_state, 5);
            d = bh_player_input_transition(&m->input_state, 6);
            e = bh_player_input_transition(&m->input_state, 7);
            pressed = a == 0 || b == 0 || d == 0 || e == 0;
            if (pressed && !press(m, 0))
                return 0;
            if (m->back_selection >= 0)
            {
                a = bh_player_input_transition(&m->input_state, 8);
                b = bh_player_input_transition(&m->input_state, 9);
                d = bh_player_input_transition(&m->input_state, 10);
                if (a == 0 || b == 0 || d == 0)
                {
                    if (!press(m, 1))
                        return 0;
                    return !c->error;
                }
            }
        }
    }
    if ((float)s->selection != s->display_selection || (m->capture && m->push > 0))
    {
        if (m->push <= 0)
        {
            if (m->slide_voice == BH_SOUND_INVALID)
                m->slide_voice = sound(c, c->slide_sound, 1);
            if (s->display_selection > (float)s->selection)
            {
                next = (double)s->display_selection - .2;
                s->display_selection = rf(next);
                if (next < (float)s->selection)
                    s->display_selection = (float)s->selection;
            }
            else if (s->display_selection < (float)s->selection)
            {
                next = (double)s->display_selection - (-.2);
                s->display_selection = rf(next);
                if (next > (float)s->selection)
                    s->display_selection = (float)s->selection;
            }
        }
        else
        {
            if (m->push == 1)
                sound(c, c->open_sound, 1);
            next = (double)m->push - .2;
            m->push = rf(next);
            if (next < 0)
                m->push = 0;
            if (m->capture && m->push == 0)
                m->capture = 0;
        }
    }
    else
    {
        stop_slide(m);
        if (!blocked(m))
        {
            if (m->push == 0)
                sound(c, c->press_sound, 1);
            if (m->push < 1)
            {
                next = (double)m->push - (-.2);
                m->push = rf(next);
                if (next > 1)
                    m->push = 1;
            }
        }
    }
    if (m->distance > 4.5)
    {
        v = (double)m->distance - ((double)m->distance - 4.5) * .4;
        m->distance = rf(v);
        if (v < 4.5)
            m->distance = 4.5f;
    }
    if (m->exiting)
    {
        next = (double)m->distance - (2.0 / 3.0);
        m->distance = rf(next);
        if (next < -5)
        {
            stop_slide(m);
            m->entity->refresh = NULL;
            m->entity->step = NULL;
        }
    }
    return !c->error;
}
static void __cdecl fade_refresh(BHEntity *e, void *unused)
{
    (void)unused;
    bh_menu_music_fade(e);
}
static void put(BHEntity *e, unsigned long o, const void *v, unsigned long n)
{
    memcpy((char *)e + o, v, n);
}
int __cdecl bh_menu_runtime_return(BHMenuRuntimeContext *c)
{
    BHMenuRuntime *root;
    BHFlybyScene *f;
    BHFlybyCameraState *s;
    BHEntity *camera, *e;
    BHVec3 zero = {0, 0, 0}, axis, angles = {3.14159265358979323846f, 0, 0};
    long l;
    unsigned long i;
    if (!c || !(root = c->root) || !valid(root) || root->context != c || !(f = c->flyby) || !(s = f->flyby_state) ||
        !f->flyby_camera || !f->flyby_camera->route || !f->starts || f->start_count <= 10 ||
        !(camera = f->camera_entity) || !c->transition || c->transition->pool != c->pool || f->entities != c->pool ||
        f->selected_waypoint != c->waypoint || f->mode_count != c->mode || !c->sessions || !c->mode)
        return fail(c);
    if (f->bound_entity != camera || f->bound_generation != camera->generation)
        return fail(c);
    if (*c->mode > 1)
    {
        bh_network_connection_dispose(c->sessions);
        c->sessions->count = 0;
    }
    *c->mode = 0;
    if (!activate(root))
        return 0;
    e = bh_entity_acquire(c->pool);
    if (e)
    {
        bh_menu_transition_bind(c->transition, e, 2, 0);
        e->step = bh_menu_fade_expiry;
        e->refresh = fade_refresh;
    }
    *c->waypoint = 10;
    l = 0;
    put(camera, 0xc4, &l, 4);
    camera->position = f->starts[10].position;
    put(camera, 0x23c, &camera->position, 12);
    put(camera, 0x248, &camera->position, 12);
    l = 10;
    put(camera, 0x284, &l, 4);
    axis.x = 1;
    axis.y = axis.z = 0;
    put(camera, 0x18, &axis, 12);
    axis.x = 0;
    axis.y = 1;
    put(camera, 0x24, &axis, 12);
    axis.y = 0;
    axis.z = 1;
    put(camera, 0x30, &axis, 12);
    for (i = 0; i < 4; ++i)
        put(camera, 0x48 + i * 12, &zero, 12);
    put(camera, 0x3c, &angles, 12);
    if (!bh_camera_state_init(&s->camera, 10))
        return fail(c);
    s->position = s->previous_position = s->older_position = camera->position;
    s->angles = angles;
    s->velocity = zero;
    s->basis_x.x = 1;
    s->basis_x.y = s->basis_x.z = 0;
    s->basis_y.y = 1;
    s->basis_y.x = s->basis_y.z = 0;
    s->basis_z.z = 1;
    s->basis_z.x = s->basis_z.y = 0;
    s->orbit_time = 0;
    s->route.phase = 0;
    s->route.waypoint_index = 10;
    s->route.current =
        bh_world_locate(f->flyby_camera->route->first, f->flyby_camera->route->scene_count, NULL, &camera->position, 0);
    put(camera, 0xc4, &s->route.current, 4);
    put(camera, 0x160, &camera->position, 12);
    put(camera, 0x16c, &angles, 12);
    {
        double time = 0;
        put(camera, 0x1f8, &time, 8);
    }
    l = 0;
    put(camera, 0x1dc, &l, 4);
    camera->step = bh_flyby_scene_camera_step_entity;
    camera->refresh = bh_flyby_scene_refresh_entity;
    for (i = 0; i < 50; ++i)
    {
        camera->step(camera);
        ++c->camera_steps;
        if (f->error != BH_FLYBY_SCENE_OK)
            return fail(c);
    }
    if (!c->transition->music)
    {
        e = bh_entity_acquire(c->pool);
        if (!e)
            return fail(c);
        c->transition->range_ready = 0;
        c->transition->music = e;
        if (!bh_menu_transition_bind(c->transition, e, .2, 0))
            return fail(c);
        e->step = bh_menu_music_tick;
    }
    return !c->error;
}
