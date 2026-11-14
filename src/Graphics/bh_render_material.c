#include "bh_renderer.h"
#include "bh_render_material.h"
#include <string.h>

static void append(BHTextureContext *q, const char *message)
{
    unsigned long n = (unsigned long)strlen(q->errors), left = (unsigned long)sizeof(q->errors) - n - 1;
    if (left)
    {
        strncpy(q->errors + n, message, left);
        q->errors[n + left] = 0;
    }
}
static void native(BHRenderMaterial *owner, HRESULT hr)
{
    ++owner->native_calls;
    owner->last_hresult = hr;
    if (hr != DD_OK)
    {
        ++owner->failures;
    }
}
const char *bh_render_material_initialize(BHRenderMaterial *owner, BHTextureContext *q, BHRenderer *renderer)
{
    unsigned long i;
    static const D3DRENDERSTATETYPE states[13] = {D3DRENDERSTATE_ZENABLE,
                                                  D3DRENDERSTATE_ZFUNC,
                                                  D3DRENDERSTATE_ZWRITEENABLE,
                                                  D3DRENDERSTATE_SPECULARENABLE,
                                                  D3DRENDERSTATE_SHADEMODE,
                                                  D3DRENDERSTATE_DITHERENABLE,
                                                  D3DRENDERSTATE_TEXTUREPERSPECTIVE,
                                                  D3DRENDERSTATE_TEXTUREMAPBLEND,
                                                  D3DRENDERSTATE_TEXTUREHANDLE,
                                                  D3DRENDERSTATE_TEXTUREMAG,
                                                  D3DRENDERSTATE_TEXTUREMIN,
                                                  D3DRENDERSTATE_CULLMODE,
                                                  D3DRENDERSTATE_COLORKEYENABLE};
    static const DWORD values[13] = {1, 7, 1, 1, 2, 1, 1, 2, 0, 2, 2, 2, 1};
    if (!q)
        return "Invalid render material context.";
    if (q->software)
        return NULL;
    if (!owner || owner->initialized || !renderer || !q->device || !q->draw || !q->allocator.allocate ||
        !q->allocator.release)
        return "Invalid render material binding.";
    for (i = 0; i < sizeof(q->errors) && q->errors[i]; ++i)
    {
    }
    if (i == sizeof(q->errors))
        return "Unterminated render error string.";
    owner->last_hresult = DD_OK;
    owner->native_calls = owner->failures = 0;
    if (!bh_texture_preallocate(q))
        append(q, " VMEMERR: Alloc texture slots failed. ");
    owner->initialized = 1;
    for (i = 0; i < 13; ++i)
    {
        native(owner, bh_renderer_set_state(q->device, states[i], values[i]));
    }
    return q->errors[0] ? q->errors : NULL;
}
void bh_render_material_dispose(BHRenderMaterial *owner)
{
    if (!owner)
        return;
    memset(owner, 0, sizeof(*owner));
}
