#include "bh_renderer.h"
#include "bh_render_execute.h"
#include "Textures/bh_texture_draw.h"
#include "../Math/bh_round.h"
#include <string.h>

static void failed(BHRenderExecute *render_execute, BHRenderExecuteError error)
{
    ++render_execute->failures;
    if (render_execute->error == BH_RENDER_EXECUTE_OK)
        render_execute->error = error;
}
static int native(BHRenderExecute *render_execute, HRESULT result)
{
    ++render_execute->native_calls;
    render_execute->last_hresult = result;
    if (FAILED(result))
    {
        failed(render_execute, BH_RENDER_EXECUTE_NATIVE);
        return 0;
    }
    return 1;
}
static int draw_one(BHRenderExecute *render_execute, const BHDrawCommand *command, unsigned long *next)
{
    switch (command->kind)
    {
    case BH_DRAW_NOP:
        *next = command->next;
        return 1;
    case BH_DRAW_STATE:
        return render_execute->state ? bh_queue_mode_execute(render_execute->state, command, next) : -3;
    case BH_DRAW_BILLBOARD:
        return render_execute->billboard ? bh_draw_submit_billboard_execute(render_execute->billboard, command, next) : -3;
    case BH_DRAW_SEGMENT:
        return render_execute->segment ? bh_segment_queue_execute(render_execute->segment, command, next) : -3;
    case BH_DRAW_ORIENTED:
    case BH_DRAW_QUAD:
        if (!render_execute->plane)
            return -3;
        return command->kind == BH_DRAW_QUAD ? bh_quad_queue_execute(render_execute->plane, command, next)
                                       : bh_plane_quad_execute(render_execute->plane, command, next);
    case BH_DRAW_MODEL_RESOURCE:
        return render_execute->model ? bh_draw_submit_execute(render_execute->model, command, next) : -3;
    case BH_DRAW_RECT:
        return render_execute->rect && render_execute->image_lookup ? bh_pickup_rect_execute(render_execute->rect, render_execute->image_lookup, render_execute->image_user, command, next)
                                          : -3;
    case BH_DRAW_TEXTURE_MASK:
    case BH_DRAW_CLIP_RECT:
        return render_execute->controls ? bh_draw_controls_execute(render_execute->controls, render_execute->draw, command, next) : -3;
    case BH_DRAW_CAMERA:
        if (render_execute->controls)
            bh_portal_clip_reset(render_execute->controls->portal_cache);
        return render_execute->view && render_execute->camera ? bh_camera_queue_execute(render_execute->view, render_execute->camera, render_execute->draw, command, next) : -3;
    case BH_DRAW_HORIZON:
        return render_execute->horizon ? bh_horizon_queue_execute(render_execute->horizon, render_execute->draw, command, next) : -3;
    case BH_DRAW_MATERIAL_WORDS:
        return render_execute->image_lookup ? bh_material_queue_execute(render_execute->image_lookup, render_execute->image_user, command, next) : -3;
    case BH_DRAW_SCREEN_QUAD:
        return render_execute->screen ? bh_screen_quad_execute(render_execute->screen, command, next) : -3;
    case BH_DRAW_PATH_HEADER:
        return render_execute->path ? bh_path_queue_execute(render_execute->path, render_execute->draw, command, next) : -3;
    case BH_DRAW_FADE:
        return render_execute->fade ? bh_fade_execute(render_execute->fade, command, next) : -3;
    case BH_DRAW_FLARE:
        return render_execute->flare ? bh_flare_execute(render_execute->flare, (BHDrawCommand *)command, next) : -3;
    case BH_DRAW_LIGHT:
        return render_execute->light ? bh_menu_light_execute(render_execute->light, command, next) : -3;
    case BH_DRAW_GDI_TEXT:
        return render_execute->gdi ? bh_gdi_text_execute(render_execute->gdi, command, next) : -3;
    default:
        return -2;
    }
}
static int barrier(BHRenderExecute *render_execute)
{
    BHTextureContext *textures = render_execute->display->textures;
    BHTextureImage *image;
    unsigned long guard = 0;
    int ok = 1;
    if (textures->software)
        return 1;
    if (!textures->device)
    {
        failed(render_execute, BH_RENDER_EXECUTE_BINDING);
        return 0;
    }
    textures->frame = (long)((unsigned long)textures->frame + 1UL);
    for (image = textures->images; image; image = image->next)
    {
        if (++guard > 1048576UL)
        {
            failed(render_execute, BH_RENDER_EXECUTE_TOPOLOGY);
            return 0;
        }
        if (!bh_texture_draw_flush(textures, image, NULL))
        {
            failed(render_execute, BH_RENDER_EXECUTE_CALLBACK);
            ok = 0;
        }
        ++render_execute->flushes;
    }
    if (!native(render_execute, bh_renderer_record_draws(textures->device)))
        ok = 0;
    return ok;
}
static int draw_anchored(BHRenderExecute *render_execute, const BHDrawCommand *command, unsigned long *next)
{
#ifdef BH_SDL_BACKEND
    unsigned long flags = command->kind == BH_DRAW_RECT          ? command->words[11]
                          : command->kind == BH_DRAW_SCREEN_QUAD ? command->words[6]
                                                                 : 0;
    float shift = render_execute->view ? render_execute->view->margin_x : 0;
    if (flags & BH_HUD_ANCHOR_LEFT)
        shift = -shift;
    else if (!(flags & BH_HUD_ANCHOR_RIGHT))
        shift = 0;
    if (shift != 0)
    {
        if (command->kind == BH_DRAW_RECT || command->kind == BH_DRAW_SCREEN_QUAD)
        {
            BHDrawCommand shifted = *command;
            float x;
            memcpy(&x, shifted.words, sizeof(x));
            bh_round_float(&x, (double)x + shift);
            memcpy(shifted.words, &x, sizeof(x));
            return draw_one(render_execute, &shifted, next);
        }
    }
#endif
    return draw_one(render_execute, command, next);
}
int bh_render_execute(BHRenderExecute *render_execute)
{
    unsigned long at, steps = 0, processed = 0, next;
    int ok = 1, status;
    if (!render_execute)
        return 0;
    render_execute->error = BH_RENDER_EXECUTE_OK;
    render_execute->failures = 0;
    render_execute->text_calls = render_execute->draw_calls = render_execute->flushes = render_execute->native_calls = 0;
    memset(render_execute->kinds, 0, sizeof(render_execute->kinds));
    if (!render_execute->display || !render_execute->display->textures || !render_execute->text || !render_execute->draw || !render_execute->draw->commands || !render_execute->draw->count)
    {
        failed(render_execute, BH_RENDER_EXECUTE_ARGUMENT);
        return 0;
    }
    if (render_execute->controls)
        bh_portal_clip_reset(render_execute->controls->portal_cache);
    status = render_execute->display_mode == 1 ? bh_frame_clear_color(render_execute->display, 255, 255, 255) : bh_frame_clear_game(render_execute->display);
    if (!status)
    {
        failed(render_execute, BH_RENDER_EXECUTE_CALLBACK);
        ok = 0;
    }
    status = bh_message_text_drain(render_execute->text, render_execute->atlas, &processed);
    render_execute->text_calls = processed;
    if (status < 0)
    {
        failed(render_execute, BH_RENDER_EXECUTE_SELECTOR);
        return 0;
    }
    at = 0;
    while (at != BH_DRAW_END)
    {
        const BHDrawCommand *command;
        if (at >= render_execute->draw->count || ++steps > render_execute->draw->count)
        {
            failed(render_execute, BH_RENDER_EXECUTE_TOPOLOGY);
            return 0;
        }
        command = render_execute->draw->commands + at;
        if (command->kind >= 20 || command->kind == BH_DRAW_PATH_POINT)
        {
            failed(render_execute, BH_RENDER_EXECUTE_SELECTOR);
            return 0;
        }
        next = command->next;
        status = draw_anchored(render_execute, command, &next);
        if (status == -3)
        {
            failed(render_execute, BH_RENDER_EXECUTE_BINDING);
            return 0;
        }
        if (status == -2)
        {
            failed(render_execute, BH_RENDER_EXECUTE_SELECTOR);
            return 0;
        }
        /* Original00479090 consumes the callback's next-command pointer;
           callback draw results are not a frame/game success condition.
           Keep failures observable, without promoting them to game exit. */
        if (status <= 0)
            failed(render_execute, BH_RENDER_EXECUTE_CALLBACK);
        ++render_execute->draw_calls;
        ++render_execute->kinds[command->kind];
        at = next;
    }
    if (!barrier(render_execute))
        ok = 0;
    return ok;
}
