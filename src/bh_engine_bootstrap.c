#include "bh_engine_bootstrap.h"
#include "bh_model_path.h"
#include "bh_texture_policy.h"
#include "bh_main_camera.h"
#include "bh_vfile.h"
#include <stdio.h>
static int fail(BHEngineBootstrap *c, int n)
{
    c->error = n;
    if (c->progress)
        bh_loading_progress_remove(c->progress);
    return 0;
}
void __cdecl bh_engine_frame_produce(void *user)
{
    BHEngineBootstrap *c = (BHEngineBootstrap *)user;
    if (c && c->service)
    {
        if (c->global_input)
        {
            c->service->packet = c->global_input->state;
            c->service->packet_bytes = sizeof(c->global_input->state);
        }
        bh_frame_service_tick(c->service);
    }
}
LRESULT __cdecl bh_engine_input_message(void *user, HWND w, UINT m, WPARAM a, LPARAM b)
{
    BHEngineBootstrap *c = (BHEngineBootstrap *)user;
    return bh_input_message(c ? c->snapshot : 0, w, m, a, b);
}
int __cdecl bh_engine_bootstrap(BHEngineBootstrap *c, HINSTANCE instance, HWND window)
{
    const char *error;
    unsigned long i;
    BHPlayerTick *player;
    if (!c || !c->archive || !c->images || !c->progress || !c->devices || !c->providers || !c->force || !c->sound ||
        !c->inputs || !c->game || !c->game->pool || !c->game->textures || !c->game->resources ||
        !c->game->resources->models || !c->view || !c->material || !c->frames || !c->service || !c->snapshot)
        return c ? fail(c, 1) : 0;
    c->error = 0;
    if (!bh_vfile_mount(c->archive))
        return fail(c, 2);
    c->mounted = 1;
    ++c->stages;
    if (!bh_loading_images_init(c->images) || !bh_loading_progress_install(c->progress))
        return fail(c, 3);
    ++c->stages;
    if (!bh_input_devices_start(c->devices, window))
        return fail(c, 4);
    c->global_input = bh_player_input_create(c->inputs, 1);
    if (!c->global_input)
        return fail(c, 5);
    if (c->debug)
    {
        bh_player_input_bind_string(c->global_input, 0, "EDIT");
        bh_player_input_bind_string(c->global_input, 1, "NET");
        bh_player_input_bind_string(c->global_input, 2, "REORDER");
        bh_player_input_bind_key(c->global_input, 3, "", 0, 0, 0x77);
        bh_player_input_bind_key(c->global_input, 4, "", 0, 0, 0x78);
    }
    ++c->stages;
    bh_sound_startup(c->sound, window, &c->sound_stats);
    bh_provider_startup(c->providers);
    bh_force_feedback_init(c->force, instance, window);
    if (!bh_bootstrap_resources(c->game, 5))
        return fail(c, 6);
    ++c->stages;
    if (!bh_entities_init(c->game->pool, 0x2580, 0) || !bh_main_camera_settings(c->view, 60, .2, 25000) ||
        !bh_texture_set_path(c->game->textures, "textures", 0) ||
        !bh_model_path_set(c->game->resources->models, "bsp", 0) || !bh_texture_policy_set(c->game->textures, 2, 0) ||
        !bh_bootstrap_resources(c->game, 6))
        return fail(c, 7);
    ++c->stages;
    if (!c->game->player || !c->game->player->tick || !c->game->player->tick->weapons ||
        !c->game->player->tick->weapons->factory)
        return fail(c, 8);
    if (!bh_weapon_factory_open(c->game->player->tick->weapons->factory, c->game->pool, c->sound))
        return fail(c, 8);
    if (!bh_game_bootstrap(c->game))
        return fail(c, 8);
    player = c->game->player->tick;
    if (!player->physics || !c->physics_bodies || !c->physics_body_count || !c->physics_models ||
        !c->physics_model_count || !c->physics_effects)
        return fail(c, 8);
    for (i = 0; i < c->physics_body_count; ++i)
        if (c->physics_bodies + i == player->body)
            break;
    if (i == c->physics_body_count)
        return fail(c, 8);
    if (!player->physics->pool &&
        !bh_physics_step_init(player->physics, c->game->pool, c->physics_bodies, c->physics_body_count,
                              c->physics_worlds, c->physics_world_count, c->physics_models, c->physics_model_count,
                              c->physics_effects, c->physics_fallback, 0))
        return fail(c, 8);
    bh_loading_display(c->progress->display, 1);
    error = bh_render_material_initialize(c->material, c->game->textures, c->d3d);
#ifdef DEBUGGING
    if (c->debug && error)
        printf("%s\n", error);
#endif
    bh_loading_progress_remove(c->progress);
    ++c->stages;
    if (!bh_frame_buffers_start(c->frames, bh_engine_frame_produce, c, 30, 0x47c70, bh_engine_input_message, c))
        return fail(c, 9);
    bh_loading_display(c->progress->display, 1);
    bh_loading_display(c->progress->display, 1);
    ++c->stages;
    return 1;
}
