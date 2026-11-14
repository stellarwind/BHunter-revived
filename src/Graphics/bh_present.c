#include "bh_renderer.h"
#include "bh_present.h"
static int result(BHPresentContext *context, HRESULT hr)
{
    ++context->native_calls;
    context->last_hresult = hr;
    if (FAILED(hr))
    {
        ++context->failures;
        return 0;
    }
    return 1;
}
int bh_present_end(BHPresentContext *context)
{
    BHTextureContext *textures;
    HRESULT hr;
    int ok = 1;
    if (!context || !context->fade || !context->fade->render || !(textures = context->fade->render->textures))
        return 0;
    if (textures->software)
    {
        if (!context->auxiliary || !context->color)
            return 0;
    }
    else if (!textures->device)
        return 0;
    /* TEST AH,41 skips zero, negative and unordered; double threshold is0. */
    if ((double)context->fade->amount > 0.0)
    {
        ++context->fade_calls;
        if (!bh_fade_apply(context->fade))
            ok = 0;
    }
    if (!textures->software)
    {
        if (!result(context, bh_renderer_record_draws(textures->device)))
            ok = 0;
    }
    else
    {
        if (!result(context, bh_renderer_surface_unlock(context->auxiliary)))
            ok = 0;
        if (!result(context, bh_renderer_surface_unlock(context->color)))
            ok = 0;
        context->locked = 0;
        hr = bh_renderer_surface_status(context->color);
        ++context->native_calls;
        context->last_hresult = hr;
        if (hr == DDERR_SURFACELOST)
        {
            ++context->restores;
            if (!result(context, bh_renderer_surface_restore(context->color)))
                ok = 0;
        }
        else if (FAILED(hr))
        {
            ++context->failures;
            ok = 0;
        }
        if (context->target)
        {
            hr = bh_renderer_surface_status(context->target);
            ++context->native_calls;
            context->last_hresult = hr;
            if (hr == DDERR_SURFACELOST)
            {
                ++context->restores;
                if (!result(context, bh_renderer_surface_restore(context->target)))
                    ok = 0;
            }
            else if (FAILED(hr))
            {
                ++context->failures;
                ok = 0;
            }
            ++context->copies;
            if (!result(context, bh_renderer_surface_copy(context->target, 0, 0, context->color, NULL)))
                ok = 0;
        }
    }
    return ok;
}
