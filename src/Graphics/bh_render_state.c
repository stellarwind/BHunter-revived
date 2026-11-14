#include "bh_renderer.h"
#include "bh_render_state.h"
#include "Textures/bh_texture_draw.h"
#include <string.h>
static int valid(BHRenderState *state)
{
    return state && state->textures && (state->textures->software || state->textures->device);
}
static int native(BHRenderState *state, HRESULT status)
{
    ++state->native_calls;
    state->last_hresult = status;
    if (FAILED(status))
    {
        ++state->failures;
        return 0;
    }
    return 1;
}
int bh_render_depth(BHRenderState *state, unsigned long mode)
{
    int ok = 1;
    if (!valid(state))
        return 0;
    if (state->depth == mode)
        return 1;
    state->depth = mode;
    if (state->textures->software || mode > 2)
        return 1;
    if (!native(state, bh_renderer_set_state(state->textures->device, D3DRENDERSTATE_ZWRITEENABLE, mode != 1)))
        ok = 0;
    if (!native(state, bh_renderer_set_state(state->textures->device, D3DRENDERSTATE_ZFUNC,
                                             mode == 2 ? D3DCMP_ALWAYS : D3DCMP_GREATEREQUAL)))
        ok = 0;
    return ok;
}
int bh_render_blend(BHRenderState *state, unsigned long enable, unsigned long source, unsigned long destination)
{
    int ok = 1;
    if (!valid(state))
        return 0;
    if (state->blend == enable && state->source == source && state->destination == destination)
        return 1;
    state->blend = enable;
    state->source = source;
    state->destination = destination;
    if (state->textures->software)
        return 1;
    if (!native(state, bh_renderer_set_state(state->textures->device, D3DRENDERSTATE_ALPHABLENDENABLE, enable)))
        ok = 0;
    if (!native(state, bh_renderer_set_state(state->textures->device, D3DRENDERSTATE_SRCBLEND, source)))
        ok = 0;
    if (!native(state, bh_renderer_set_state(state->textures->device, D3DRENDERSTATE_DESTBLEND, destination)))
        ok = 0;
    return ok;
}
int bh_render_state_init(BHRenderState *state, BHTextureContext *textures)
{
    int ok = 1;
    if (!state || !textures || (!textures->software && !textures->device))
        return 0;
    memset(state, 0, sizeof(*state));
    state->textures = textures;
    state->depth = state->blend = state->source = state->destination = 0xffffffffUL;
    if (!bh_render_depth(state, 0))
        ok = 0;
    if (!bh_render_blend(state, 0, D3DBLEND_ONE, D3DBLEND_ZERO))
        ok = 0;
    return ok;
}
int bh_render_mode(BHRenderState *state, unsigned long mode)
{
    BHTextureImage *image, *slow, *fast;
    int ok = 1;
    if (!valid(state))
        return 0;
    if (!state->textures->software)
    {
        /* Source preflight guards malformed borrowed lists before publishing
           an epoch or flushing any image. Real cache owner supplies nodes. */
        slow = fast = state->textures->images;
        while (fast && fast->next)
        {
            slow = slow->next;
            fast = fast->next->next;
            if (slow == fast)
                return 0;
        }
        state->textures->frame = (long)((unsigned long)state->textures->frame + 1);
        ++state->barriers;
        for (image = state->textures->images; image; image = image->next)
            if (!bh_texture_draw_flush(state->textures, image, NULL))
                ok = 0;
        if (!native(state, bh_renderer_record_draws(state->textures->device)))
            ok = 0;
    }
    state->mode = mode;
    if (mode <= 4)
    {
        if (!bh_render_depth(state, mode ? 1 : 0))
            ok = 0;
        switch (mode)
        {
        case 0:
            if (!bh_render_blend(state, 0, D3DBLEND_ONE, D3DBLEND_ZERO))
                ok = 0;
            break;
        case 1:
            if (!bh_render_blend(state, 1, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR))
                ok = 0;
            break;
        case 2:
            if (!bh_render_blend(state, 1, D3DBLEND_ZERO, D3DBLEND_INVSRCCOLOR))
                ok = 0;
            break;
        case 3:
            if (!bh_render_blend(state, 1, D3DBLEND_ONE, D3DBLEND_ONE))
                ok = 0;
            break;
        case 4:
            if (!bh_render_blend(state, 1, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA))
                ok = 0;
            break;
        }
    }
    return ok;
}
