#include "bh_render_state.h"
#include "bh_texture_draw.h"
#include <string.h>
static int valid(BHRenderState *s)
{
    return s && s->textures && (s->textures->software || s->textures->device);
}
static int native(BHRenderState *s, HRESULT h)
{
    ++s->native_calls;
    s->last_hresult = h;
    if (FAILED(h))
    {
        ++s->failures;
        return 0;
    }
    return 1;
}
int __cdecl bh_render_depth(BHRenderState *s, unsigned long mode)
{
    int ok = 1;
    if (!valid(s))
        return 0;
    if (s->depth == mode)
        return 1;
    s->depth = mode;
    if (s->textures->software || mode > 2)
        return 1;
    if (!native(s, IDirect3DDevice2_SetRenderState(s->textures->device, D3DRENDERSTATE_ZWRITEENABLE, mode != 1)))
        ok = 0;
    if (!native(s, IDirect3DDevice2_SetRenderState(s->textures->device, D3DRENDERSTATE_ZFUNC,
                                                   mode == 2 ? D3DCMP_ALWAYS : D3DCMP_GREATEREQUAL)))
        ok = 0;
    return ok;
}
int __cdecl bh_render_blend(BHRenderState *s, unsigned long enable, unsigned long source, unsigned long destination)
{
    int ok = 1;
    if (!valid(s))
        return 0;
    if (s->blend == enable && s->source == source && s->destination == destination)
        return 1;
    s->blend = enable;
    s->source = source;
    s->destination = destination;
    if (s->textures->software)
        return 1;
    if (!native(s, IDirect3DDevice2_SetRenderState(s->textures->device, D3DRENDERSTATE_ALPHABLENDENABLE, enable)))
        ok = 0;
    if (!native(s, IDirect3DDevice2_SetRenderState(s->textures->device, D3DRENDERSTATE_SRCBLEND, source)))
        ok = 0;
    if (!native(s, IDirect3DDevice2_SetRenderState(s->textures->device, D3DRENDERSTATE_DESTBLEND, destination)))
        ok = 0;
    return ok;
}
int __cdecl bh_render_state_init(BHRenderState *s, BHTextureContext *textures)
{
    int ok = 1;
    if (!s || !textures || (!textures->software && !textures->device))
        return 0;
    memset(s, 0, sizeof(*s));
    s->textures = textures;
    s->depth = s->blend = s->source = s->destination = 0xffffffffUL;
    if (!bh_render_depth(s, 0))
        ok = 0;
    if (!bh_render_blend(s, 0, D3DBLEND_ONE, D3DBLEND_ZERO))
        ok = 0;
    return ok;
}
int __cdecl bh_render_mode(BHRenderState *s, unsigned long mode)
{
    BHTextureImage *image, *slow, *fast;
    int ok = 1;
    if (!valid(s))
        return 0;
    if (!s->textures->software)
    {
        /* Source preflight guards malformed borrowed lists before publishing
           an epoch or flushing any image. Real cache owner supplies nodes. */
        slow = fast = s->textures->images;
        while (fast && fast->next)
        {
            slow = slow->next;
            fast = fast->next->next;
            if (slow == fast)
                return 0;
        }
        s->textures->frame = (long)((unsigned long)s->textures->frame + 1);
        ++s->barriers;
        for (image = s->textures->images; image; image = image->next)
            if (!bh_texture_draw_flush(s->textures, image, NULL))
                ok = 0;
        if (!native(s, IDirect3DDevice2_EndScene(s->textures->device)))
            ok = 0;
        if (!native(s, IDirect3DDevice2_BeginScene(s->textures->device)))
            ok = 0;
    }
    s->mode = mode;
    if (mode <= 4)
    {
        if (!bh_render_depth(s, mode ? 1 : 0))
            ok = 0;
        switch (mode)
        {
        case 0:
            if (!bh_render_blend(s, 0, D3DBLEND_ONE, D3DBLEND_ZERO))
                ok = 0;
            break;
        case 1:
            if (!bh_render_blend(s, 1, D3DBLEND_ONE, D3DBLEND_INVSRCCOLOR))
                ok = 0;
            break;
        case 2:
            if (!bh_render_blend(s, 1, D3DBLEND_ZERO, D3DBLEND_INVSRCCOLOR))
                ok = 0;
            break;
        case 3:
            if (!bh_render_blend(s, 1, D3DBLEND_ONE, D3DBLEND_ONE))
                ok = 0;
            break;
        case 4:
            if (!bh_render_blend(s, 1, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA))
                ok = 0;
            break;
        }
    }
    return ok;
}
