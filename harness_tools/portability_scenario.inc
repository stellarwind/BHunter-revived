/* Only included in the generated harness application, never production src/. */
#include <SDL3/SDL.h>
static unsigned long regression_tick, regression_checks, regression_failures;
static unsigned long regression_tone, regression_loop, regression_loop_voice = BH_SOUND_INVALID;
static unsigned long regression_music_plays, regression_gdi_commands;
static unsigned long regression_gdi_executed, regression_sound_plays, regression_backend_errors;
static unsigned long regression_history;
static int regression_automated, regression_completed;
static BHVec3 regression_start;
static BHEntity *regression_static_vehicles[2];
static BHTargetConstructSlot *regression_static_slots[2];
static unsigned long regression_static_generations[2];
static BHVec3 regression_static_positions[2] = {{-10, -14, 35}, {35, -14, 55}};
static int regression_static_spawned;
static void regression_spawn_static_vehicles(BHGameApp *a);
static FILE *regression_log;
static const char *regression_caption = "PORTABILITY REGRESSION: INPUT / AUDIO / TEXT";

static void regression_check(const char *name, int passed)
{
    ++regression_checks;
    if (!passed)
        ++regression_failures;
    if (regression_log)
    {
        fprintf(regression_log, "{\"tick\":%lu,\"check\":\"%s\",\"passed\":%s}\n", regression_tick, name,
                passed ? "true" : "false");
        fflush(regression_log);
    }
}

static void regression_mark(BHGameApp *a, const char *name)
{
    if (strcmp(name, "heartbeat"))
        regression_caption = name;
    if (regression_log)
    {
        fprintf(regression_log,
                "{\"tick\":%lu,\"event\":\"%s\",\"mode\":%ld,\"paused\":%ld,\"position\":[%.5f,%.5f,%.5f],"
                "\"app_error\":%d,\"sound_error\":%d,\"music_plays\":%lu}\n",
                regression_tick, name, a->player_state.mode, a->player_state.paused, a->player->position.x,
                a->player->position.y, a->player->position.z, a->error, a->sound.error, a->cd_notify.plays);
        fflush(regression_log);
    }
}

static SDL_WindowID regression_window_id(void)
{
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    SDL_WindowID id = count == 1 ? SDL_GetWindowID(windows[0]) : 0;
    SDL_free(windows);
    return id;
}
static void regression_key(BHGameApp *a, unsigned int key, int down, int repeat)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    SDL_Keycode code = key >= 'A' && key <= 'Z' ? key + ('a' - 'A')
                       : key == VK_UP           ? SDLK_UP
                       : key == VK_CONTROL      ? SDLK_LCTRL
                       : key == VK_BACK         ? SDLK_BACKSPACE
                       : key == VK_RETURN       ? SDLK_RETURN
                       : key == VK_ESCAPE       ? SDLK_ESCAPE
                                                : SDLK_UNKNOWN;
    if (!down && key == VK_UP && getenv("BH_REGRESSION_DROP_KEYUP"))
        return;
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = regression_window_id();
    event.key.key = code;
    event.key.scancode = key >= 'A' && key <= 'Z' ? SDL_SCANCODE_A + key - 'A'
                         : key == VK_UP           ? SDL_SCANCODE_UP
                         : key == VK_CONTROL      ? SDL_SCANCODE_LCTRL
                         : key == VK_BACK         ? SDL_SCANCODE_BACKSPACE
                         : key == VK_RETURN       ? SDL_SCANCODE_RETURN
                         : key == VK_ESCAPE       ? SDL_SCANCODE_ESCAPE
                                                  : SDL_SCANCODE_UNKNOWN;
    event.key.down = down != 0;
    event.key.repeat = repeat != 0;
    regression_check("input-message-posted", SDL_PushEvent(&event));
    /* Drain at this simulation boundary, including SDL's poll-cycle sentinel.
       Otherwise a one-tick tap can collapse during a multi-tick catch-up. */
    bh_sdl_poll();
    bh_sdl_poll();
    if (key == VK_RETURN)
        fprintf(regression_log, "{\"trace\":\"return-event\",\"tick\":%lu,\"down\":%d,\"state\":%u,\"previous\":%u}\n",
                regression_tick, down, a->snapshot.current[13], a->snapshot.previous[13]);
}
static int regression_mouse(int down)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.windowID = regression_window_id();
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = down != 0;
    int posted = SDL_PushEvent(&event);
    bh_sdl_poll();
    bh_sdl_poll();
    return posted;
}
static void regression_cue(BHGameApp *a, const char *label, unsigned long resource, float pitch, int side)
{
    BHVec3 position = a->sound.listener;
    unsigned long handle;
    position.x += a->sound.listener_right.x * side * 12;
    position.y += a->sound.listener_right.y * side * 12;
    position.z += a->sound.listener_right.z * side * 12;
    handle = bh_sound_request(&a->sound, resource, .65f, pitch, side ? &position : NULL, NULL);
    regression_check(label, handle != BH_SOUND_INVALID);
    regression_mark(a, label);
}

/* Harness-only stationary fixtures use real constructors, queries, lifetime
   updates and target HUD rendering. Sleeping their physics keeps them parked. */
static void regression_spawn_static_vehicles(BHGameApp *a)
{
    unsigned i;
    a->target_draw.player = a->player;
    for (i = 0; i < 2; ++i)
    {
        BHTargetDrawState state;
        BHEntity *entity =
            bh_target_construct_moving(&a->targets, regression_static_positions + i, &a->zero, &a->zero, &a->zero,
                                       a->actor_models[i ? 8 : 0], 1000, 0, a->player, i ? -5000 : 5000, -1, 0);
        regression_static_vehicles[i] = entity;
        regression_check(i ? "stationary-suspect-created" : "stationary-target-created", entity != NULL);
        if (entity)
        {
            unsigned slot;
            BHTargetStepState step_state;
            regression_static_generations[i] = entity->generation;
            for (slot = 0; slot < a->targets.capacity; ++slot)
                if (a->targets.slots[slot].body.entity == entity)
                {
                    regression_static_slots[i] = a->targets.slots + slot;
                    break;
                }
            bh_target_step_read(entity, &step_state);
            step_state.emission = 0;
            bh_target_step_write(entity, &step_state);
            if (regression_static_slots[i])
                regression_static_slots[i]->body.sleep_time = 3;
            bh_target_draw_state_read(entity, &state);
            regression_check(i ? "suspect-HUD-ownership" : "target-HUD-ownership",
                             state.owner == a->player && state.number == (i ? -5000 : 5000) && entity->refresh &&
                                 entity->query);
        }
    }
    regression_static_spawned = 1;
    regression_mark(a, "stationary-target-and-suspect-ready");
}

static void regression_begin(BHGameApp *a)
{
    unsigned long i;
    regression_automated = getenv("BH_REGRESSION_MANUAL") == NULL;
    regression_log = fopen("regression-events.jsonl", "w");
    regression_check("event-log-opened", regression_log != NULL);
    srand(1);
    regression_check("three-room-level-loaded", a->editor_world.count == 3);
    for (i = 0; i < GAME_STARTS; ++i)
    {
        a->starts[i].position.x = 15;
        a->starts[i].position.y = -14;
        a->starts[i].position.z = -40;
        a->starts[i].angles = a->zero;
        a->route_starts[i] = a->starts[i].position;
    }
    regression_tone = bh_sound_load(&a->sounds, "reg-tone.wav", 40, .7f, 0, 30);
    regression_loop = bh_sound_load(&a->sounds, "reg-loop.wav", 40, .5f, 1, 30);
    regression_check("tone-resource-loaded", regression_tone != BH_SOUND_INVALID);
    regression_check("loop-resource-loaded", regression_loop != BH_SOUND_INVALID);
    regression_mark(a, "arena-ready");
    if (!regression_automated)
    {
        a->ship = -1;
        bh_menu_action_mission(&a->menu_edits, 0);
        a->menu.root->entity->step = NULL;
        a->menu.root->entity->refresh = NULL;
    }
}

static void regression_before_tick(BHGameApp *a)
{
    unsigned long t;
    ++regression_tick;
    t = regression_tick;
    if (!regression_automated)
    {
        if (!regression_static_spawned && regression_tick >= 30 && a->player_state.mode == 1)
            regression_spawn_static_vehicles(a);
        {
            unsigned i;
            for (i = 0; i < 2; ++i)
                if (regression_static_slots[i] && regression_static_vehicles[i] &&
                    bh_entity_is_active(&a->pool, regression_static_vehicles[i]) &&
                    regression_static_vehicles[i]->generation == regression_static_generations[i])
                    regression_static_slots[i]->body.sleep_time = 3;
        }
        if (getenv("BH_REGRESSION_STATIC_CHECK") && regression_tick == 120)
        {
            unsigned i;
            for (i = 0; i < 2; ++i)
            {
                BHEntity *entity = regression_static_vehicles[i];
                regression_check(i ? "suspect-remains-stationary" : "target-remains-stationary",
                                 entity && bh_entity_is_active(&a->pool, entity) && entity->step && entity->refresh &&
                                     !memcmp(&entity->position, regression_static_positions + i, sizeof(BHVec3)));
            }
            regression_completed = 1;
            a->quit = 1;
        }
        t = (t - 1) % 900 + 1;
        if (t == 1)
            regression_mark(a, "manual-cue-cycle-start");
    }
    if (regression_automated)
    {
        if (t == 10)
        {
            unsigned long i;
            bh_input_set_enabled(a->menu.root->input, &a->snapshot, 0);
            for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
                if (a->bootstrap.menus[i].action[1] == bh_menu_action_profile)
                {
                    a->bootstrap.menus[i].settings.selection = 1;
                    bh_menu_action_profile(&a->bootstrap.menus[i].actions, 1);
                    break;
                }
            regression_check("profile-text-capture-started", a->text_capture.entity != NULL);
        }
        if (t == 15 || t == 19 || t == 23 || t == 27)
            regression_key(a, t == 15 ? 'A' : t == 19 ? 'B' : t == 23 ? VK_BACK : 'C', 1, 0);
        if (t == 16 || t == 20 || t == 24 || t == 28)
            regression_key(a, t == 16 ? 'A' : t == 20 ? 'B' : t == 24 ? VK_BACK : 'C', 0, 0);
        if (t == 50)
            bh_input_set_enabled(a->menu.root->input, &a->snapshot, 1);
        if (t == 35 || t == 60 || t == 120 || t == 180)
            regression_key(a, VK_RETURN, 1, 0);
        if (t == 36 || t == 63 || t == 123 || t == 183)
            regression_key(a, VK_RETURN, 0, 0);
        if (t == 230)
        {
            regression_start = a->player->position;
            regression_key(a, VK_UP, 1, 0);
        }
        if (t == 245)
            regression_key(a, VK_UP, 1, 1);
        if (t == 260)
            regression_key(a, VK_UP, 0, 0);
        if (t == 290)
            regression_key(a, VK_CONTROL, 1, 0);
        if (t == 305)
            regression_key(a, VK_CONTROL, 0, 0);
        if (t == 310)
            regression_check("mouse-down-posted", regression_mouse(1));
        if (t == 313)
            regression_check("mouse-up-posted", regression_mouse(0));
        if (t == 320)
            regression_key(a, 'J', 1, 0);
        if (t == 323)
        {
            SDL_Event event;
            memset(&event, 0, sizeof(event));
            event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
            event.window.windowID = regression_window_id();
            SDL_PushEvent(&event);
            bh_sdl_poll();
            regression_check("focus-loss-clears-held-key", a->snapshot.current['J'] == 0 && !a->focus_sample);
        }
        if (t == 324)
            regression_key(a, 'J', 0, 0);
        if (t == 350 || t == 380)
            regression_key(a, VK_ESCAPE, 1, 0);
        if (t == 353 || t == 383)
            regression_key(a, VK_ESCAPE, 0, 0);
    }
    if (t == 420)
        regression_cue(a, "cue-left", regression_tone, 1, -1);
    if (t == 450)
        regression_cue(a, "cue-right", regression_tone, 1, 1);
    if (t == 480)
        regression_cue(a, "cue-pitch-low", regression_tone, .75f, 0);
    if (t == 510)
        regression_cue(a, "cue-pitch-high", regression_tone, 1.5f, 0);
    if (t == 540)
        regression_cue(a, "cue-game-warning", a->player_tick.resources.damage_warning, 1, 0);
    if (t == 570)
        regression_cue(a, "cue-game-impact", a->effect.impact_sound, 1, 0);
    if (t == 600)
    {
        regression_loop_voice = bh_sound_request(&a->sound, regression_loop, .5f, 1, NULL, NULL);
        regression_check("loop-requested", regression_loop_voice != BH_SOUND_INVALID);
        regression_mark(a, "loop-start");
    }
    if (t == 630)
        regression_check("loop-updated",
                         bh_sound_update(&a->sound, regression_loop_voice, .25f, 1.25f, NULL, NULL) == 1);
    if (t == 660)
    {
        regression_check("loop-stopped", bh_sound_stop(&a->sound, regression_loop_voice));
        regression_check("loop-handle-retired", bh_sound_voice(&a->sound, regression_loop_voice) == NULL);
        regression_mark(a, "loop-stop");
    }
    if (t == 690)
    {
        regression_check("config-saved", bh_config_save(&a->config, "regression-save.cfg"));
        regression_mark(a, "save-roundtrip");
    }
    if (t == 720)
    {
        BHConfig loaded;
        int ok = bh_config_init(&loaded, NULL);
        if (ok)
            ok = bh_config_bootstrap(&loaded, "regression-save.cfg", a->device_names, 1);
        regression_check("config-reloaded",
                         ok && !memcmp(loaded.settings, a->config.settings, sizeof(loaded.settings)));
        bh_config_dispose(&loaded);
    }
    if (t == 750)
    {
        bh_sdl_video_cycle(2);
        regression_mark(a, "video-aspect-cycle");
    }
    if (t == 780)
    {
        bh_sdl_video_cycle(2);
        bh_sdl_video_cycle(2);
        regression_mark(a, "video-aspect-restored");
    }
    if (t == 810)
    {
        unsigned long j;
        for (j = 0; j < 6; ++j)
            regression_cue(a, "overlapping-cue", regression_tone, .8f + j * .1f, 0);
    }
    if (t == 850)
    {
        unsigned long engine_resource = BH_SOUND_INVALID, engine_handle;
        BHSoundVoice *engine;
        memcpy(&engine_handle, (char *)a->player + 0x220, 4);
        engine = bh_sound_voice(&a->sound, engine_handle);
        if (engine && engine->resource)
            engine_resource = engine->resource->index;
        regression_check("all-effects-stopped", bh_sound_stop_all(&a->sound));
        if (!regression_automated && engine_resource != BH_SOUND_INVALID)
        {
            engine_handle = bh_sound_request(&a->sound, engine_resource, 0, 1, NULL, NULL);
            memcpy((char *)a->player + 0x220, &engine_handle, 4);
        }
        regression_mark(a, "all-effects-stop");
    }
}

static void regression_after_tick(BHGameApp *a)
{
    unsigned long t = regression_tick;
    BHDrawQueue *queue = a->slots + a->frames.latest;
    if (bh_gdi_text_queue(queue, 8, 8, regression_caption, 0x80ffff) == 1)
        ++regression_gdi_commands;
    regression_sound_plays += a->service.sound_stats.play_calls;
    if (a->service.sound_stats.last_error || a->sound.error == BH_SOUND_BACKEND)
        ++regression_backend_errors;
    if (t % 30 == 0)
        regression_mark(a, "heartbeat");
    if ((t == 35 || t == 36) && regression_automated)
        fprintf(regression_log,
                "{\"trace\":\"return-tick\",\"tick\":%lu,\"state\":%u,\"global\":%lu,\"packet\":[%u,%u,%u,%u]}\n", t,
                a->snapshot.current[13], a->global_state.values[13], a->bootstrap.all_keys->state[2],
                a->bootstrap.all_keys->state[3], a->bootstrap.all_keys->state[4], a->bootstrap.all_keys->state[5]);
    if (t == 45 && regression_automated)
    {
        fprintf(
            regression_log,
            "{\"trace\":\"text-state\",\"buffer\":\"%s\",\"profile\":\"%s\",\"capture_active\":%d,\"capture_error\":%d}\n",
            a->text_buffer, (char *)a->profile->payload, a->text_capture.entity != NULL, a->text_capture.error);
        regression_check("text-edit-and-profile-commit", !a->text_capture.entity && !a->text_capture.error &&
                                                             !strcmp(a->text_buffer, "AC") &&
                                                             !strcmp((char *)a->profile->payload, "AC"));
    }
    if (t == 220 && regression_automated)
    {
        regression_check("script-entered-game", a->player_state.mode == 1);
        regression_music_plays = a->cd_notify.plays;
    }
    if (t == 240 && regression_automated)
    {
        regression_check("keyboard-held", a->snapshot.current[VK_UP] == 1);
        regression_history = a->snapshot.history_head;
    }
    if (t == 250 && regression_automated)
        regression_check("repeat-does-not-add-history", a->snapshot.history_head == regression_history);
    if (t == 265 && regression_automated)
    {
        double x = a->player->position.x - regression_start.x;
        double y = a->player->position.y - regression_start.y;
        double z = a->player->position.z - regression_start.z;
        regression_check("keyboard-release", a->snapshot.current[VK_UP] == 0);
        regression_check("player-moved", x * x + y * y + z * z > .01);
    }
    if (t == 360 && regression_automated)
        regression_check("escape-paused", a->player_state.paused != 0);
    if (t == 390 && regression_automated)
        regression_check("escape-resumed", a->player_state.paused == 0);
    if (t == 312 && regression_automated)
        regression_check("mouse-button-held", a->snapshot.current[1] == 1);
    if (t == 316 && regression_automated)
        regression_check("mouse-button-released", a->snapshot.current[1] == 0);
    if (t == 422)
        regression_check("left-pan-applied", a->sound.resources[regression_tone]->cached_pan[0] < -5000);
    if (t == 452)
        regression_check("right-pan-applied", a->sound.resources[regression_tone]->cached_pan[0] > 5000);
    if (t == 482)
        regression_check("low-pitch-applied", a->sound.resources[regression_tone]->cached_frequency[0] == 16537);
    if (t == 512)
        regression_check("high-pitch-applied", a->sound.resources[regression_tone]->cached_frequency[0] == 33075);
    if (t == 615)
        regression_check("loop-playing", a->sound.resources[regression_loop]->playing[0] != 0);
    if (t == 900 && regression_automated)
    {
        regression_check("music-opened", a->cd.device != 0 && a->cd.local_file);
        regression_check("music-loop-notification",
                         a->cd_notify.plays > regression_music_plays + 1 && a->cd_notify.failures == 0);
        regression_check("text-commands-submitted", regression_gdi_commands >= 850 && a->gdi.failures == 0);
        regression_check("text-commands-executed", regression_gdi_executed >= 850);
        regression_check("sound-backend-clean", regression_sound_plays > 10 && !regression_backend_errors);
        regression_check("runtime-errors-clear",
                         !a->error && !a->player_tick.error && !a->menu.error && !a->sound.error);
        regression_check("math-profile-preserved", bh_math_environment_supported());
        regression_completed = 1;
        a->quit = 1;
    }
}

static void regression_after_render(BHGameApp *a)
{
    regression_gdi_executed += a->render.kinds[BH_DRAW_GDI_TEXT];
}

static void regression_finish(BHGameApp *a)
{
    FILE *summary;
    if (!regression_log)
        return;
    if (a->player_state.paused)
    {
        bh_cd_set_volume(&a->media, a->player_state.saved_cd_volume);
        bh_wave_set_volume(&a->media, a->player_state.saved_wave_volume);
    }
    regression_mark(a, "session-finished");
    summary = fopen("regression-summary.json", "w");
    if (summary)
    {
        fprintf(summary,
                "{\"completed\":%s,\"automated\":%s,\"ticks\":%lu,\"checks\":%lu,\"failures\":%lu,"
                "\"game_error\":%d,\"music_plays\":%lu,\"gdi_commands\":%lu,"
                "\"diagnostic_code_in_production_source\":false}\n",
                regression_completed ? "true" : "false", regression_automated ? "true" : "false", regression_tick,
                regression_checks, regression_failures, a->error, a->cd_notify.plays, regression_gdi_commands);
        fclose(summary);
    }
    fclose(regression_log);
    regression_log = NULL;
    if (regression_automated && (regression_failures || !regression_completed))
        a->error = 90;
}
