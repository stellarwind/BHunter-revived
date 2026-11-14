#include "bh_gdi_text.h"
#include "../bh_renderer.h"
#include <string.h>
unsigned long bh_draw_command_next(const BHDrawCommand *c)
{
    return c->next;
}
int bh_gdi_text_queue(BHDrawQueue *q, short x, short y, const char *text, unsigned long rgb)
{
    BHDrawCommand *c;
    unsigned long at, tail;
    const char *end;
    if (!q || !q->commands || !text || q->count < 9 || q->count > q->capacity)
        return -1;
    end = (const char *)memchr(text, 0, 59);
    if (!end)
        return -1;
    if (q->count == q->capacity)
    {
        q->error = BH_DRAW_FULL;
        return 0;
    }
    tail = q->commands[8].resource;
    if (tail >= q->count)
        return -1;
    at = q->count++;
    c = q->commands + at;
    c->next = q->commands[tail].next;
    q->commands[tail].next = at;
    q->commands[8].resource = at;
    c->kind = BH_DRAW_GDI_TEXT;
    c->resource = (unsigned short)x | ((unsigned long)(unsigned short)y << 16);
    c->words[0] = ((rgb >> 16) & 255UL) + ((rgb & 255UL) << 16) + (rgb & 0xff00UL);
    strncpy((char *)(c->words + 1), text, 59);
    return 1;
}
int bh_editor_message_queue(BHDrawQueue *q)
{
    return bh_gdi_text_queue(q, 0, 0, "No objects, press 'N' to create new", 0xff0000UL);
}
static void native(BHGdiText *c, HRESULT hr)
{
    ++c->native_calls;
    c->last_hresult = hr;
    if (FAILED(hr))
        ++c->failures;
}
int bh_gdi_text_execute(BHGdiText *c, const BHDrawCommand *command, unsigned long *next)
{
    BHFrameDisplay *d;
    DDSURFACEDESC desc;
    const char *text;
    unsigned long rgb;
    int ok;
    if (!c || !(d = c->display) || !d->color || !command || command->kind != BH_DRAW_GDI_TEXT || !next)
        return 0;
    text = (const char *)(command->words + 1);
    if (!memchr(text, 0, 59))
        return 0;
    *next = bh_draw_command_next(command);
    if (c->relock)
    {
        HRESULT hr = bh_renderer_surface_unlock(d->color);
        native(c, hr);
        if (FAILED(hr))
            return 0;
        d->locked = 0;
    }
    /* Commands keep the original COLORREF byte order; the portable boundary is RGB. */
    rgb = ((command->words[0] & 255UL) << 16) | (command->words[0] & 0xff00UL) | ((command->words[0] >> 16) & 255UL);
    ok = bh_renderer_surface_text(d->color, (short)command->resource, (short)(command->resource >> 16), text, rgb);
    native(c, ok ? DD_OK : E_FAIL);
    if (c->relock)
    {
        HRESULT hr;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = bh_renderer_surface_lock(d->color, &desc);
        native(c, hr);
        if (SUCCEEDED(hr))
        {
            d->locked = 1;
            d->color_pixels = (unsigned char *)desc.lpSurface;
            d->color_pitch = desc.lPitch;
            if (d->atlas)
                d->atlas->scan_step = desc.lPitch;
        }
        else
            ok = 0;
    }
    return ok;
}
void bh_gdi_text_dispose(BHGdiText *c)
{
    if (c)
        c->display = NULL;
}
