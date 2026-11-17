#include "bh_menu_runtime.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <string.h>
#include <float.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static int fail(BHMenuRuntimeContext *context)
{
    if (context)
        context->error = 1;
    return 0;
}
static BHMenuSettingsMenu *menu(BHMenuRuntime *runtime)
{
    return runtime->actions->menu;
}
static int valid(BHMenuRuntime *runtime)
{
    BHMenuSettingsMenu *menu_settings_menu;
    if (!runtime || !runtime->context || !runtime->entity || !runtime->actions || !(menu_settings_menu = runtime->actions->menu) || !menu_settings_menu->rows || !menu_settings_menu->row_count ||
        menu_settings_menu->row_count > 4096 || !runtime->destinations || !runtime->input || !runtime->actions->row_actions ||
        runtime->actions->action_count < menu_settings_menu->row_count || menu_settings_menu->selection < 0 || (unsigned long)menu_settings_menu->selection >= menu_settings_menu->row_count)
        return 0;
    return 1;
}
static int actionable(BHMenuRuntime *runtime, long i)
{
    return runtime->destinations[i] || runtime->actions->row_actions[i] || (runtime->edit_actions && runtime->edit_actions[i]);
}
static int blocked(BHMenuRuntime *runtime)
{
    if (runtime->input_blocked)
        return 1;
    if (runtime->action_context && runtime->action_context->text && runtime->action_context->text->entity &&
        runtime->action_context->text->menu == menu(runtime))
        return 1;
    return runtime->edit_context && runtime->edit_context->capture && runtime->edit_context->capture->entity &&
           runtime->edit_context->capture->menu == menu(runtime);
}
static int choose(BHMenuRuntime *runtime, int direction, int advance)
{
    BHMenuSettingsMenu *menu_settings_menu = menu(runtime);
    unsigned long i;
    for (i = 0; i < menu_settings_menu->row_count; ++i)
    {
        if (advance)
        {
            menu_settings_menu->selection += direction;
            if (menu_settings_menu->selection < 0)
                menu_settings_menu->selection = (long)menu_settings_menu->row_count - 1;
            if ((unsigned long)menu_settings_menu->selection >= menu_settings_menu->row_count)
                menu_settings_menu->selection = 0;
        }
        if (actionable(runtime, menu_settings_menu->selection))
            return 1;
        advance = 1;
    }
    return fail(runtime->context);
}
static void refresh(BHMenuRuntime *runtime)
{
    unsigned long i;
    BHMenuSettingsMenu *menu_settings_menu = menu(runtime);
    for (i = 0; i < menu_settings_menu->row_count; ++i)
        if (menu_settings_menu->rows[i] && menu_settings_menu->rows[i]->refresh)
        {
            ++runtime->context->refreshes;
            menu_settings_menu->rows[i]->refresh(menu_settings_menu, (long)i);
        }
}
static unsigned long sound(BHMenuRuntimeContext *context, unsigned long resource, float level)
{
    ++context->sounds;
    return bh_sound_request(context->sound, resource, level, 1, NULL, NULL);
}
static void stop_slide(BHMenuRuntime *runtime)
{
    if (runtime->slide_voice != BH_SOUND_INVALID)
    {
        bh_sound_stop(runtime->context->sound, runtime->slide_voice);
        runtime->slide_voice = BH_SOUND_INVALID;
    }
}
static void drain(BHMenuRuntime *runtime, unsigned long first, unsigned long last)
{
    unsigned long i;
    for (i = first; i <= last; ++i)
        bh_player_input_transition(&runtime->input_state, i);
}
static int activate(BHMenuRuntime *runtime)
{
    if (!valid(runtime))
        return fail(runtime ? runtime->context : NULL);
    runtime->entity->step = bh_menu_runtime_step_entity;
    runtime->entity->refresh = bh_menu_runtime_refresh_entity;
    if (runtime->waypoint >= 0)
        *runtime->context->waypoint = runtime->waypoint;
    runtime->distance = 100;
    runtime->exiting = 0;
    if (!bh_input_set_enabled(runtime->input, runtime->context->snapshot, 1))
        runtime->context->error = 1;
    refresh(runtime);
    if (!valid(runtime))
        return fail(runtime->context);
    return choose(runtime, 1, 0);
}
int bh_menu_runtime_bind(BHMenuRuntime *runtime)
{
    BHEntity *active_entity;
    unsigned long n = 0;
    if (!valid(runtime) || !runtime->context->pool || !runtime->context->snapshot || !runtime->context->sound || !runtime->context->waypoint ||
        !runtime->context->stay || !runtime->context->input_changed || !runtime->context->mode || !runtime->context->paused)
        return 0;
    for (active_entity = runtime->context->pool->active; active_entity && n++ < runtime->context->pool->capacity; active_entity = active_entity->next)
        if (active_entity == runtime->entity)
            break;
    if (!active_entity)
        return 0;
    memcpy(&runtime->entity->bindings.menu_runtime, &runtime, 4);
    runtime->entity->step = bh_menu_runtime_step_entity;
    runtime->entity->refresh = bh_menu_runtime_refresh_entity;
    return 1;
}
static BHMenuRuntime *binding(BHEntity *entity)
{
    BHMenuRuntime *m;
    memcpy(&m, &entity->bindings.menu_runtime, 4);
    return m;
}
void bh_menu_runtime_step_entity(BHEntity *e)
{
    BHMenuRuntime *runtime = binding(e);
    if (runtime && runtime->entity == e)
        bh_menu_runtime_step(runtime);
}
void bh_menu_runtime_refresh_entity(BHEntity *e, void *unused)
{
    BHMenuRuntime *runtime = binding(e);
    BHMenuPresent *menu_present;
    (void)unused;
    if (!runtime || runtime->entity != e || !(menu_present = runtime->present))
        return;
    menu_present->phase = runtime->phase;
    menu_present->selection = menu(runtime)->selection;
    menu_present->display_selection = menu(runtime)->display_selection;
    menu_present->push = runtime->push;
    menu_present->distance = runtime->distance;
    menu_present->capture = (unsigned long)runtime->capture;
    menu_present->application_mode = *runtime->context->mode;
    bh_menu_present(menu_present);
}
static int press(BHMenuRuntime *runtime, int back)
{
    BHMenuRuntimeContext *context = runtime->context;
    BHMenuSettingsMenu *menu_settings_menu = menu(runtime);
    BHMenuRuntime *destination;
    BHMenuAction action;
    long selected;
    if (!back)
        *context->input_changed = 1;
    sound(context, context->move_sound, .6f);
    if (back)
        menu_settings_menu->selection = runtime->back_selection;
    selected = menu_settings_menu->selection;
    if (selected < 0 || (unsigned long)selected >= menu_settings_menu->row_count)
        return fail(context);
    action = runtime->actions->row_actions[selected];
    if (action)
    {
        if (!runtime->action_context)
            return fail(context);
        runtime->action_context->menu = menu_settings_menu;
        action(runtime->action_context, selected);
    }
    else if (runtime->edit_actions && runtime->edit_actions[selected])
    {
        if (!runtime->edit_context)
            return fail(context);
        runtime->edit_context->menu = menu_settings_menu;
        runtime->edit_actions[selected](runtime->edit_context, selected);
    }
    if (!valid(runtime))
        return fail(context);
    selected = menu_settings_menu->selection;
    destination = runtime->destinations[selected];
    if (*context->stay || destination == runtime)
    {
        *context->stay = 0;
        runtime->capture = 1;
        return choose(runtime, 1, 0);
    }
    sound(context, context->open_sound, 1);
    drain(runtime, 4, 10);
    bh_input_set_enabled(runtime->input, context->snapshot, 0);
    runtime->exiting = 1;
    if (destination && !activate(destination))
        return 0;
    return 1;
}
int bh_menu_runtime_step(BHMenuRuntime *runtime)
{
    BHMenuRuntimeContext *context;
    BHMenuSettingsMenu *menu_settings_menu;
    double v, next;
    float f;
    short mouse;
    unsigned long raw;
    int pressed;
    unsigned char a, b, d, e;
    if (!valid(runtime))
        return fail(runtime ? runtime->context : NULL);
    context = runtime->context;
    menu_settings_menu = menu(runtime);
    if (!bh_math_environment_supported() || !_finite(menu_settings_menu->display_selection) || !_finite(runtime->push) ||
        !_finite(runtime->distance) || !_finite(runtime->scroll))
        return fail(context);
    if (*context->paused == 1)
        return 1;
    ++runtime->phase;
    if (runtime->pending_selection >= 0)
    {
        if ((unsigned long)runtime->pending_selection >= menu_settings_menu->row_count)
            return fail(context);
        menu_settings_menu->selection = runtime->pending_selection;
    }
    runtime->pending_selection = -1;
    if (runtime == context->ship_menu || runtime == context->network_ship_menu)
    {
        if (menu_settings_menu->selection < BH_GARAGE_VANOS_ROW)
            *context->waypoint = BH_GARAGE_FIRST_WAYPOINT + menu_settings_menu->selection;
        else if (menu_settings_menu->selection == BH_GARAGE_VANOS_ROW && context->flyby && context->flyby->vanos_display)
            *context->waypoint = BH_GARAGE_VANOS_WAYPOINT;
    }
    if (!runtime->exiting && !blocked(runtime))
    {
        bh_player_input_apply_packet(&runtime->input_state, runtime->input->state);
        if (runtime->distance >= 10)
        {
            drain(runtime, 0, 1);
            drain(runtime, 4, 10);
        }
        else
        {
            raw = runtime->input_state.values[2];
            mouse = (short)(raw >> 16);
            runtime->scroll = rf((double)runtime->scroll - (double)mouse * .0025);
            memcpy(&f, runtime->input_state.values + 3, 4);
            if (!_finite(f))
                return fail(context);
            if (f < -.35)
                runtime->scroll = rf((double)runtime->scroll - (-.1));
            else if (f > .35)
                runtime->scroll = rf((double)runtime->scroll - .1);
            a = bh_player_input_transition(&runtime->input_state, 0);
            if (a == 0 || runtime->scroll > 1)
            {
                if (runtime->scroll > 1)
                    runtime->scroll = rf((double)runtime->scroll - 1);
                *context->input_changed = 1;
                sound(context, context->move_sound, .6f);
                if (!choose(runtime, -1, 1))
                    return 0;
            }
            a = bh_player_input_transition(&runtime->input_state, 1);
            if (a == 0 || runtime->scroll < -1)
            {
                if (runtime->scroll < -1)
                    runtime->scroll = rf((double)runtime->scroll + 1);
                *context->input_changed = 1;
                sound(context, context->move_sound, .6f);
                if (!choose(runtime, 1, 1))
                    return 0;
            }
            a = bh_player_input_transition(&runtime->input_state, 4);
            b = bh_player_input_transition(&runtime->input_state, 5);
            d = bh_player_input_transition(&runtime->input_state, 6);
            e = bh_player_input_transition(&runtime->input_state, 7);
            pressed = a == 0 || b == 0 || d == 0 || e == 0;
            if (pressed && !press(runtime, 0))
                return 0;
            if (runtime->back_selection >= 0)
            {
                a = bh_player_input_transition(&runtime->input_state, 8);
                b = bh_player_input_transition(&runtime->input_state, 9);
                d = bh_player_input_transition(&runtime->input_state, 10);
                if (a == 0 || b == 0 || d == 0)
                {
                    if (!press(runtime, 1))
                        return 0;
                    return !context->error;
                }
            }
        }
    }
    if ((float)menu_settings_menu->selection != menu_settings_menu->display_selection || (runtime->capture && runtime->push > 0))
    {
        if (runtime->push <= 0)
        {
            if (runtime->slide_voice == BH_SOUND_INVALID)
                runtime->slide_voice = sound(context, context->slide_sound, 1);
            if (menu_settings_menu->display_selection > (float)menu_settings_menu->selection)
            {
                next = (double)menu_settings_menu->display_selection - .2;
                menu_settings_menu->display_selection = rf(next);
                if (next < (float)menu_settings_menu->selection)
                    menu_settings_menu->display_selection = (float)menu_settings_menu->selection;
            }
            else if (menu_settings_menu->display_selection < (float)menu_settings_menu->selection)
            {
                next = (double)menu_settings_menu->display_selection - (-.2);
                menu_settings_menu->display_selection = rf(next);
                if (next > (float)menu_settings_menu->selection)
                    menu_settings_menu->display_selection = (float)menu_settings_menu->selection;
            }
        }
        else
        {
            if (runtime->push == 1)
                sound(context, context->open_sound, 1);
            next = (double)runtime->push - .2;
            runtime->push = rf(next);
            if (next < 0)
                runtime->push = 0;
            if (runtime->capture && runtime->push == 0)
                runtime->capture = 0;
        }
    }
    else
    {
        stop_slide(runtime);
        if (!blocked(runtime))
        {
            if (runtime->push == 0)
                sound(context, context->press_sound, 1);
            if (runtime->push < 1)
            {
                next = (double)runtime->push - (-.2);
                runtime->push = rf(next);
                if (next > 1)
                    runtime->push = 1;
            }
        }
    }
    if (runtime->distance > 4.5)
    {
        v = (double)runtime->distance - ((double)runtime->distance - 4.5) * .4;
        runtime->distance = rf(v);
        if (v < 4.5)
            runtime->distance = 4.5f;
    }
    if (runtime->exiting)
    {
        next = (double)runtime->distance - (2.0 / 3.0);
        runtime->distance = rf(next);
        if (next < -5)
        {
            stop_slide(runtime);
            runtime->entity->refresh = NULL;
            runtime->entity->step = NULL;
        }
    }
    return !context->error;
}
static void fade_refresh(BHEntity *e, void *unused)
{
    (void)unused;
    bh_menu_music_fade(e);
}
int bh_menu_runtime_return(BHMenuRuntimeContext *context)
{
    BHMenuRuntime *root;
    BHFlybyScene *flyby_scene;
    BHFlybyCameraState *flyby_camera_state;
    BHEntity *camera, *entity;
    BHVec3 zero = {0, 0, 0}, axis, angles = {3.14159265358979323846f, 0, 0};
    long l;
    unsigned long i;
    if (!context || !(root = context->root) || !valid(root) || root->context != context || !(flyby_scene = context->flyby) || !(flyby_camera_state = flyby_scene->flyby_state) ||
        !flyby_scene->flyby_camera || !flyby_scene->flyby_camera->route || !flyby_scene->starts || flyby_scene->start_count <= 10 ||
        !(camera = flyby_scene->camera_entity) || !context->transition || context->transition->pool != context->pool || flyby_scene->entities != context->pool ||
        flyby_scene->selected_waypoint != context->waypoint || flyby_scene->mode_count != context->mode || !context->mode)
        return fail(context);
    if (flyby_scene->bound_entity != camera || flyby_scene->bound_generation != camera->generation)
        return fail(context);
    *context->mode = 0;
    if (!activate(root))
        return 0;
    entity = bh_entity_acquire(context->pool);
    if (entity)
    {
        bh_menu_transition_bind(context->transition, entity, 2, 0);
        entity->step = bh_menu_fade_expiry;
        entity->refresh = fade_refresh;
    }
    *context->waypoint = 10;
    l = 0;
    memcpy(&camera->world, &l, 4);
    camera->position = flyby_scene->starts[10].position;
    memcpy(&camera->flyby.previous_position, &camera->position, 12);
    memcpy(&camera->flyby.older_position, &camera->position, 12);
    l = 10;
    memcpy(&camera->flyby.waypoint_index, &l, 4);
    axis.x = 1;
    axis.y = axis.z = 0;
    memcpy(&camera->basis[0], &axis, 12);
    axis.x = 0;
    axis.y = 1;
    memcpy(&camera->basis[1], &axis, 12);
    axis.y = 0;
    axis.z = 1;
    memcpy(&camera->basis[2], &axis, 12);
    camera->velocity = zero;
    camera->angular_velocity = zero;
    camera->acceleration = zero;
    camera->angular_acceleration = zero;
    memcpy(&camera->euler, &angles, 12);
    if (!bh_camera_state_init(&flyby_camera_state->camera, 10))
        return fail(context);
    flyby_camera_state->position = flyby_camera_state->previous_position = flyby_camera_state->older_position = camera->position;
    flyby_camera_state->angles = angles;
    flyby_camera_state->velocity = zero;
    flyby_camera_state->basis_x.x = 1;
    flyby_camera_state->basis_x.y = flyby_camera_state->basis_x.z = 0;
    flyby_camera_state->basis_y.y = 1;
    flyby_camera_state->basis_y.x = flyby_camera_state->basis_y.z = 0;
    flyby_camera_state->basis_z.z = 1;
    flyby_camera_state->basis_z.x = flyby_camera_state->basis_z.y = 0;
    flyby_camera_state->orbit_time = 0;
    flyby_camera_state->route.phase = 0;
    flyby_camera_state->route.waypoint_index = 10;
    flyby_camera_state->route.current =
        bh_world_locate(flyby_scene->flyby_camera->route->first, flyby_scene->flyby_camera->route->scene_count, NULL, &camera->position, 0);
    memcpy(&camera->world, &flyby_camera_state->route.current, 4);
    memcpy(&camera->flyby.camera.position, &camera->position, 12);
    memcpy(&camera->flyby.camera.angles, &angles, 12);
    {
        double time = 0;
        memcpy(&camera->flyby.orbit_time, &time, 8);
    }
    l = 0;
    memcpy(&camera->flyby.route_phase, &l, 4);
    camera->step = bh_flyby_scene_camera_step_entity;
    camera->refresh = bh_flyby_scene_refresh_entity;
    for (i = 0; i < 50; ++i)
    {
        camera->step(camera);
        ++context->camera_steps;
        if (flyby_scene->error != BH_FLYBY_SCENE_OK)
            return fail(context);
    }
    if (!context->transition->music)
    {
        entity = bh_entity_acquire(context->pool);
        if (!entity)
            return fail(context);
        context->transition->range_ready = 0;
        context->transition->music = entity;
        if (!bh_menu_transition_bind(context->transition, entity, .2, 0))
            return fail(context);
        entity->step = bh_menu_music_tick;
    }
    return !context->error;
}
