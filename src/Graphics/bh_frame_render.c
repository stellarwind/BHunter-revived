#include "bh_renderer.h"
#include "../Platform/bh_platform_clock.h"
#include "bh_frame_render.h"
#include <float.h>
#include <string.h>
#ifdef BH_SDL_BACKEND
#include "../Platform/bh_platform.h"
#endif

static int error(BHFrameDisplay *frame_display, BHFrameRenderError value)
{
    if (frame_display)
        frame_display->error = value;
    return 0;
}
static int native(BHFrameDisplay *frame_display, HRESULT hr)
{
    ++frame_display->native_calls;
    frame_display->last_hresult = hr;
    if (FAILED(hr))
    {
        ++frame_display->failures;
        frame_display->error = BH_FRAME_RENDER_NATIVE;
        return 0;
    }
    return 1;
}
static int rect(BHFrameDisplay *frame_display, RECT *result_rect, long *rows)
{
    if (!_finite(frame_display->left) || !_finite(frame_display->top) || frame_display->left < 0.0f || frame_display->top < 0.0f || frame_display->left > 640.0f || frame_display->top > 480.0f)
        return error(frame_display, BH_FRAME_RENDER_BOUNDS);
    result_rect->left = (long)frame_display->left;
    result_rect->top = (long)frame_display->top;
    result_rect->right = 640;
    result_rect->bottom = 480;
    *rows = (long)((double)480.0 - (double)frame_display->top);
    return 1;
}
static int flush_and_cycle(BHFrameDisplay *frame_display)
{
    BHTextureImage *image, *slow, *fast;
    int ok = 1;
    slow = fast = frame_display->textures->images;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return error(frame_display, BH_FRAME_RENDER_TOPOLOGY);
    }
    frame_display->textures->frame = (long)((unsigned long)frame_display->textures->frame + 1);
    for (image = frame_display->textures->images; image; image = image->next)
    {
        ++frame_display->flushes;
        if (!bh_texture_draw_flush(frame_display->textures, image, NULL))
            ok = 0;
    }
    if (!native(frame_display, bh_renderer_record_draws(frame_display->textures->device)))
        ok = 0;
    return ok;
}
static int clear_depth(BHFrameDisplay *frame_display, int game_frame)
{
    RECT r;
    DDBLTFX fx;
    long rows;
    int ok = 1;
    unsigned long width, bytes, at;
    long y;
    if (!frame_display || !frame_display->textures)
        return error(frame_display, BH_FRAME_RENDER_ARGUMENT);
    frame_display->error = BH_FRAME_RENDER_OK;
    if (!frame_display->auxiliary || (!frame_display->textures->software && !frame_display->textures->device))
        return error(frame_display, BH_FRAME_RENDER_BINDING);
    if (!rect(frame_display, &r, &rows))
        return 0;
    if (!frame_display->textures->software && !flush_and_cycle(frame_display))
        ok = 0;
#ifdef BH_SDL_BACKEND
    /* The horizon normally covers the color target, but rasterized edges
       can leave pixels from the previous frame's damage fade. */
    if (game_frame && !frame_display->textures->software)
        return native(frame_display, bh_sdl_clear_frame() ? DD_OK : E_FAIL) && ok;
#else
    (void)game_frame;
#endif
    if (frame_display->textures->software && frame_display->locked)
    {
        width = (unsigned long)(r.right - r.left);
        bytes = width * 2;
        if (!frame_display->auxiliary_pixels || frame_display->auxiliary_pitch <= 0 || bytes > (unsigned long)frame_display->auxiliary_pitch ||
            (unsigned long)r.top * (unsigned long)frame_display->auxiliary_pitch + width * 2 > frame_display->auxiliary_bytes ||
            (rows > 0 &&
             (unsigned long)(r.top + rows - 1) * (unsigned long)frame_display->auxiliary_pitch + (unsigned long)r.left * 2 + bytes >
                 frame_display->auxiliary_bytes))
            return error(frame_display, BH_FRAME_RENDER_BOUNDS);
        for (y = 0; y < rows; y++)
        {
            at = (unsigned long)(r.top + y) * (unsigned long)frame_display->auxiliary_pitch + (unsigned long)r.left * 2;
            memset(frame_display->auxiliary_pixels + at, 0, bytes);
        }
        return ok;
    }
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillDepth = 0;
    if (!native(frame_display, bh_renderer_surface_blit(frame_display->auxiliary, &r, NULL, NULL, DDBLT_WAIT | DDBLT_DEPTHFILL, &fx)))
        ok = 0;
    return ok;
}
int bh_frame_clear_depth(BHFrameDisplay *d)
{
    return clear_depth(d, 0);
}
int bh_frame_clear_game(BHFrameDisplay *d)
{
    return clear_depth(d, 1);
}
static unsigned long color_value(BHFrameDisplay *frame_display, long red, long green, long blue)
{
    BHTextureContext *texture_context = frame_display->textures;
    if (frame_display->indexed_color == 1)
        return (unsigned long)(((red * 5) / 255) * 42 + ((green * 6) / 255) * 6 + (blue * 5) / 255);
    return (unsigned long)(((red >> (texture_context->display_right[0] & 31)) << (texture_context->display_left[0] & 31)) +
                           ((green >> (texture_context->display_right[1] & 31)) << (texture_context->display_left[1] & 31)) +
                           ((blue >> (texture_context->display_right[2] & 31)) << (texture_context->display_left[2] & 31)));
}
int bh_frame_clear_color(BHFrameDisplay *frame_display, long red, long green, long blue)
{
    RECT r;
    DDBLTFX fx;
    DDSURFACEDESC desc;
    long rows;
    int ok = 1, attempt;
    HRESULT hr;
    if (!frame_display || !frame_display->textures)
        return error(frame_display, BH_FRAME_RENDER_ARGUMENT);
    frame_display->error = BH_FRAME_RENDER_OK;
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
        return error(frame_display, BH_FRAME_RENDER_ARGUMENT);
    if (!frame_display->color || !frame_display->auxiliary || (!frame_display->textures->software && !frame_display->textures->device))
        return error(frame_display, BH_FRAME_RENDER_BINDING);
    if (!rect(frame_display, &r, &rows))
        return 0;
    if (!frame_display->textures->software && !flush_and_cycle(frame_display))
        ok = 0;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = color_value(frame_display, red, green, blue);
    if (frame_display->locked)
    {
        if (!native(frame_display, bh_renderer_surface_unlock(frame_display->color)))
            ok = 0;
    }
    if (!native(frame_display, bh_renderer_surface_blit(frame_display->color, &r, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx)))
        ok = 0;
    if (frame_display->locked)
    {
        for (attempt = 0; attempt < 500; attempt++)
        {
            memset(&desc, 0, sizeof(desc));
            desc.dwSize = sizeof(desc);
            hr = bh_renderer_surface_lock(frame_display->color, &desc);
            ++frame_display->native_calls;
            frame_display->last_hresult = hr;
            if (hr == DD_OK)
            {
                frame_display->color_pixels = (unsigned char *)desc.lpSurface;
                frame_display->color_pitch = desc.lPitch;
                if (frame_display->atlas)
                    frame_display->atlas->scan_step = (unsigned long)desc.lPitch;
                break;
            }
            ++frame_display->failures;
            frame_display->error = BH_FRAME_RENDER_NATIVE;
            ok = 0;
            bh_platform_delay(1);
            ++frame_display->sleeps;
        }
        if (attempt == 500)
        {
            bh_frame_clear_depth(frame_display);
            return error(frame_display, BH_FRAME_RENDER_NATIVE);
        }
    }
    if (!bh_frame_clear_depth(frame_display))
        ok = 0;
    if (!ok)
        frame_display->error = BH_FRAME_RENDER_NATIVE;
    return ok;
}
