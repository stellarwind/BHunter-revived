#include "../bh_renderer.h"
#include "../../Platform/bh_platform_clock.h"
#include "bh_loading_display.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <float.h>
static void restore(BHLoadingDisplay *loading_display, BHRenderSurface *surface)
{
    if (surface && bh_renderer_surface_status(surface) == DDERR_SURFACELOST)
    {
        ++loading_display->restores;
        bh_renderer_surface_restore(surface);
    }
}
static int wedge(BHLoadingDisplay *loading_display, const BHLoadingBlitTarget *t, int n, float a, float b)
{
    ++loading_display->wedge_calls;
    switch (n)
    {
    case 0:
        return bh_loading_blit_top(&loading_display->blit, t, a, b);
    case 1:
        return bh_loading_blit_right(&loading_display->blit, t, a, b);
    case 2:
        return bh_loading_blit_bottom(&loading_display->blit, t, a, b);
    case 3:
        return bh_loading_blit_left(&loading_display->blit, t, a, b);
    default:
        return bh_loading_blit_top_last(&loading_display->blit, t, a, b);
    }
}
int bh_loading_display(BHLoadingDisplay *loading_display, float progress)
{
    BHLoadingImages *im;
    DDSURFACEDESC desc;
    BHLoadingBlitTarget target;
    const unsigned char *indices, *source;
    const unsigned long *native;
    unsigned char *row, *out;
    unsigned long offset, now, x, y, value, depth, pitch_bytes, row_offset;
    unsigned __int64 span, needed;
    long old_redraw = 0;
    float previous;
    int ok = 1;
    if (!loading_display)
        return 0;
    ++loading_display->calls;
    im = loading_display->images;
    if (!im || !im->arena || !loading_display->color)
        return 1;
    if (im->stage == 0 && im->redraw_count == 2)
    {
        ++im->tick_calls;
        im->deadline = bh_platform_ticks() + 5000UL;
    }
    if (im->stage < 2)
    {
        ++im->tick_calls;
        now = bh_platform_ticks();
        old_redraw = im->redraw_count;
        if (im->redraw_count)
            im->redraw_count = (long)((unsigned long)im->redraw_count - 1UL);
        if ((long)(now - im->deadline) >= 0)
        {
            im->stage = (long)((unsigned long)im->stage + 1UL);
            if (im->stage == 1)
            {
                ++im->tick_calls;
                im->deadline = bh_platform_ticks() + 5000UL;
                im->redraw_count = 2;
            }
            else if (im->stage == 2)
            {
                loading_display->previous[0] = loading_display->previous[1] = -1.0f;
                loading_display->baseline = progress;
                bh_round_float(&loading_display->scale, 1.0 / (1.0 - (double)progress));
            }
        }
        if (!old_redraw)
            return 1;
    }
    if (im->stage < 0 || im->stage > 2 || !loading_display->primary || loading_display->width <= 0 || loading_display->height <= 0 || loading_display->width > 640 ||
        loading_display->height > 480)
    {
        ++loading_display->failures;
        return 0;
    }
    offset = im->stage == 0 ? 0x96000UL : im->stage == 1 ? 0xe1000UL : 0;
    if (offset > im->arena_bytes || (unsigned long)loading_display->width * (unsigned long)loading_display->height > im->arena_bytes - offset)
    {
        ++loading_display->failures;
        return 0;
    }
    indices = im->stage == 0 ? im->midas_indexed : im->stage == 1 ? im->iel_indexed : im->indexed;
    native = im->stage == 0 ? im->midas_native : im->stage == 1 ? im->iel_native : im->native;
    bh_round_float(&loading_display->normalized, ((double)progress - loading_display->baseline) * loading_display->scale);
    restore(loading_display, loading_display->color);
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    loading_display->lock_result = bh_renderer_surface_lock(loading_display->color, &desc);
    if (loading_display->lock_result != DD_OK)
    {
        ++loading_display->failures;
        return 0;
    }
    ++loading_display->locks;
    depth = im->pixel_bytes;
    pitch_bytes = desc.lPitch < 0 ? 0UL - (unsigned long)desc.lPitch : (unsigned long)desc.lPitch;
    span = (unsigned __int64)pitch_bytes * (unsigned long)loading_display->height;
    needed = (unsigned __int64)(unsigned long)loading_display->width * depth;
    /* Lock returns logical row0, including for bottom-up native storage. The
       wedge span uses the lower allocation address and a signed row0 offset. */
    row_offset = desc.lPitch < 0 ? (unsigned long)((unsigned __int64)pitch_bytes * (loading_display->height - 1)) : 0;
    if (!pitch_bytes || pitch_bytes < needed || span > 0xffffffffUL ||
        (desc.lPitch < 0 && ((unsigned __int64)pitch_bytes * (loading_display->height - 1) > 0x7fffffffUL ||
                             (unsigned long)desc.lpSurface < row_offset)) ||
        (unsigned __int64)((unsigned long)desc.lpSurface - row_offset) + span > ((unsigned __int64)1 << 32) ||
        desc.dwWidth < (unsigned long)loading_display->width || desc.dwHeight < (unsigned long)loading_display->height)
    {
        bh_renderer_surface_unlock(loading_display->color);
        ++loading_display->unlocks;
        ++loading_display->failures;
        return 0;
    }
    if (loading_display->alternate)
    {
        previous = loading_display->previous[0];
        loading_display->previous[0] = loading_display->normalized;
        loading_display->alternate = 0;
    }
    else
    {
        previous = loading_display->previous[1];
        loading_display->previous[1] = loading_display->normalized;
        loading_display->alternate = 1;
    }
    source = im->arena + offset;
    if (_isnan(previous) || previous < 0.0f || old_redraw)
    {
        ++loading_display->full_copies;
        row = (unsigned char *)desc.lpSurface;
        for (y = 0; y < (unsigned long)loading_display->height; ++y)
        {
            out = row;
            for (x = 0; x < (unsigned long)loading_display->width; ++x)
            {
                value = *source++;
                if (depth == 1)
                    *out++ = indices[value];
                else if (depth == 2)
                {
                    value = native[value];
                    *out++ = (unsigned char)value;
                    *out++ = (unsigned char)(value >> 8);
                }
                else if (depth == 3)
                {
                    value = native[value];
                    *out++ = (unsigned char)(value >> 16);
                    *out++ = (unsigned char)(value >> 8);
                    *out++ = (unsigned char)value;
                }
                else if (depth == 4)
                {
                    value = native[value];
                    memcpy(out, &value, 4);
                    out += 4;
                }
            }
            if (y + 1 < (unsigned long)loading_display->height)
                row += desc.lPitch;
        }
        previous = 0.0f;
    }
    loading_display->blit.images = im;
    loading_display->blit.width = loading_display->width;
    loading_display->blit.height = loading_display->height;
    target.base = (unsigned char *)desc.lpSurface - row_offset;
    target.bytes = (unsigned long)span;
    target.row0 = (long)row_offset;
    target.pitch = desc.lPitch;
    if (!old_redraw && !_isnan(loading_display->normalized) && !_isnan(previous) && loading_display->normalized != previous)
    {
        if (!(loading_display->normalized > 0.125f))
            ok = wedge(loading_display, &target, 0, previous, loading_display->normalized) && ok;
        else
        {
            ok = wedge(loading_display, &target, 0, previous, 0.125f) && ok;
            if (!(loading_display->normalized > 0.375f))
                ok = wedge(loading_display, &target, 1, 0.125f, loading_display->normalized) && ok;
            else
            {
                ok = wedge(loading_display, &target, 1, 0.125f, 0.375f) && ok;
                if (!(loading_display->normalized > 0.625f))
                    ok = wedge(loading_display, &target, 2, 0.375f, loading_display->normalized) && ok;
                else
                {
                    ok = wedge(loading_display, &target, 2, 0.375f, 0.625f) && ok;
                    if (!(loading_display->normalized > 0.875f))
                        ok = wedge(loading_display, &target, 3, 0.625f, loading_display->normalized) && ok;
                    else
                    {
                        ok = wedge(loading_display, &target, 3, 0.625f, 0.875f) && ok;
                        ok = wedge(loading_display, &target, 4, 0.875f, loading_display->normalized > 1.0f ? 1.0f : loading_display->normalized) && ok;
                    }
                }
            }
        }
    }
    loading_display->unlock_result = bh_renderer_surface_unlock(loading_display->color);
    ++loading_display->unlocks;
    restore(loading_display, loading_display->color);
    if (loading_display->software && loading_display->software_target)
    {
        restore(loading_display, loading_display->software_target);
        loading_display->copy_result = bh_renderer_surface_copy(loading_display->software_target, 0, 0, loading_display->color, NULL);
    }
    restore(loading_display, loading_display->primary);
    restore(loading_display, loading_display->back);
    if (loading_display->present)
    {
        ++loading_display->presentations;
        loading_display->flip_result = loading_display->present(loading_display->color, loading_display->present_user);
    }
    else
    {
        ++loading_display->flips;
        loading_display->flip_result = bh_renderer_surface_present(loading_display->primary);
    }
    if (!ok || FAILED(loading_display->flip_result))
    {
        ++loading_display->failures;
        return 0;
    }
    return 1;
}
