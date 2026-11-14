#include "bh_renderer.h"
#include "bh_present_begin.h"
#include <string.h>
static int result(BHPresentBegin *present_begin, HRESULT hr)
{
    ++present_begin->native_calls;
    present_begin->last_hresult = hr;
    if (FAILED(hr))
    {
        ++present_begin->failures;
        return 0;
    }
    return 1;
}
static int restore(BHPresentBegin *present_begin, BHRenderSurface *s, int *lost)
{
    HRESULT hr;
    *lost = 0;
    if (!s)
        return 1;
    hr = bh_renderer_surface_status(s);
    ++present_begin->native_calls;
    present_begin->last_hresult = hr;
    if (hr == DDERR_SURFACELOST)
    {
        *lost = 1;
        ++present_begin->restores;
        return result(present_begin, bh_renderer_surface_restore(s));
    }
    if (FAILED(hr))
    {
        ++present_begin->failures;
        return 0;
    }
    return 1;
}
int bh_present_begin(BHPresentBegin *present_begin)
{
    BHPresentContext *present_context;
    BHFadeContext *fade;
    BHTextureContext *textures;
    DDSURFACEDESC desc;
    BHTextureSlot *slot;
    unsigned long i;
    int lost, ok = 1;
    if (!present_begin || !(present_context = present_begin->present) || !(fade = present_context->fade) || !fade->render || !(textures = fade->render->textures))
        return 0;
    if (textures->software)
    {
        if (!present_context->color || !present_context->auxiliary || present_context->locked)
            return 0;
    }
    else if (!textures->device)
        return 0;
    if (!restore(present_begin, present_context->color, &lost))
        ok = 0;
    if (!restore(present_begin, present_context->auxiliary, &lost))
        ok = 0;
    if (textures->software)
    {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        /* Original Lock flags are zero, not DDLOCK_WAIT. */
        if (!result(present_begin, bh_renderer_surface_lock(present_context->color, &desc)))
            return 0;
        fade->pixels = (unsigned char *)desc.lpSurface;
        fade->pitch = desc.lPitch;
        if (!result(present_begin, bh_renderer_surface_lock(present_context->auxiliary, &desc)))
        {
            result(present_begin, bh_renderer_surface_unlock(present_context->color));
            return 0;
        }
        present_begin->auxiliary_pixels = (unsigned char *)desc.lpSurface;
        present_begin->auxiliary_pitch = desc.lPitch;
        present_context->locked = 1;
    }
    else
    {
        for (i = 0; i < 9; ++i)
            for (slot = textures->formats[i].slots; slot; slot = slot->next)
            {
                if (!slot->surface)
                    return 0;
                if (!restore(present_begin, slot->surface, &lost))
                    ok = 0;
                if (lost)
                {
                    if (slot->image)
                        slot->image->resident = NULL;
                    slot->image = NULL;
                    ++present_begin->invalidations;
                }
            }
    }
    return ok;
}
