#include "bh_renderer.h"
#include "bh_present.h"
static int result(BHPresentContext *c, HRESULT hr)
{
    ++c->native_calls;
    c->last_hresult = hr;
    if (FAILED(hr))
    {
        ++c->failures;
        return 0;
    }
    return 1;
}
int bh_present_end(BHPresentContext *c)
{
    BHTextureContext *textures;
    HRESULT hr;
    int ok = 1;
    if (!c || !c->fade || !c->fade->render || !(textures = c->fade->render->textures))
        return 0;
    if (textures->software)
    {
        if (!c->auxiliary || !c->color)
            return 0;
    }
    else if (!textures->device)
        return 0;
    /* TEST AH,41 skips zero, negative and unordered; double threshold is0. */
    if ((double)c->fade->amount > 0.0)
    {
        ++c->fade_calls;
        if (!bh_fade_apply(c->fade))
            ok = 0;
    }
    if (!textures->software)
    {
        if (!result(c, bh_renderer_record_draws(textures->device)))
            ok = 0;
    }
    else
    {
        if (!result(c, bh_renderer_surface_unlock(c->auxiliary)))
            ok = 0;
        if (!result(c, bh_renderer_surface_unlock(c->color)))
            ok = 0;
        c->locked = 0;
        hr = bh_renderer_surface_status(c->color);
        ++c->native_calls;
        c->last_hresult = hr;
        if (hr == DDERR_SURFACELOST)
        {
            ++c->restores;
            if (!result(c, bh_renderer_surface_restore(c->color)))
                ok = 0;
        }
        else if (FAILED(hr))
        {
            ++c->failures;
            ok = 0;
        }
        if (c->target)
        {
            hr = bh_renderer_surface_status(c->target);
            ++c->native_calls;
            c->last_hresult = hr;
            if (hr == DDERR_SURFACELOST)
            {
                ++c->restores;
                if (!result(c, bh_renderer_surface_restore(c->target)))
                    ok = 0;
            }
            else if (FAILED(hr))
            {
                ++c->failures;
                ok = 0;
            }
            ++c->copies;
            if (!result(c, bh_renderer_surface_copy(c->target, 0, 0, c->color, NULL)))
                ok = 0;
        }
    }
    return ok;
}
