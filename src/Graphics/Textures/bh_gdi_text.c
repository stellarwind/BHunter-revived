#include "bh_gdi_text.h"
#include "../bh_renderer.h"
#include <string.h>
unsigned long bh_draw_command_next(const BHDrawCommand *command)
{
    return command->next;
}
int bh_gdi_text_queue(BHDrawQueue *queue, short x, short y, const char *text, unsigned long rgb)
{
    BHDrawCommand *command;
    unsigned long at, tail;
    const char *end;
    if (!queue || !queue->commands || !text || queue->count < 9 || queue->count > queue->capacity)
        return -1;
    end = (const char *)memchr(text, 0, 59);
    if (!end)
        return -1;
    if (queue->count == queue->capacity)
    {
        queue->error = BH_DRAW_FULL;
        return 0;
    }
    tail = queue->commands[8].resource;
    if (tail >= queue->count)
        return -1;
    at = queue->count++;
    command = queue->commands + at;
    command->next = queue->commands[tail].next;
    queue->commands[tail].next = at;
    queue->commands[8].resource = at;
    command->kind = BH_DRAW_GDI_TEXT;
    command->resource = (unsigned short)x | ((unsigned long)(unsigned short)y << 16);
    command->words[0] = ((rgb >> 16) & 255UL) + ((rgb & 255UL) << 16) + (rgb & 0xff00UL);
    strncpy((char *)(command->words + 1), text, 59);
    return 1;
}
int bh_editor_message_queue(BHDrawQueue *q)
{
    return bh_gdi_text_queue(q, 0, 0, "No objects, press 'N' to create new", 0xff0000UL);
}
static void native(BHGdiText *text_context, HRESULT hr)
{
    ++text_context->native_calls;
    text_context->last_hresult = hr;
    if (FAILED(hr))
        ++text_context->failures;
}
int bh_gdi_text_execute(BHGdiText *text_context, const BHDrawCommand *command, unsigned long *next)
{
    BHFrameDisplay *frame_display;
    DDSURFACEDESC desc;
    const char *text;
    unsigned long rgb;
    int ok;
    if (!text_context || !(frame_display = text_context->display) || !frame_display->color || !command || command->kind != BH_DRAW_GDI_TEXT || !next)
        return 0;
    text = (const char *)(command->words + 1);
    if (!memchr(text, 0, 59))
        return 0;
    *next = bh_draw_command_next(command);
    if (text_context->relock)
    {
        HRESULT hr = bh_renderer_surface_unlock(frame_display->color);
        native(text_context, hr);
        if (FAILED(hr))
            return 0;
        frame_display->locked = 0;
    }
    /* Commands keep the original COLORREF byte order; the portable boundary is RGB. */
    rgb = ((command->words[0] & 255UL) << 16) | (command->words[0] & 0xff00UL) | ((command->words[0] >> 16) & 255UL);
    ok = bh_renderer_surface_text(frame_display->color, (short)command->resource, (short)(command->resource >> 16), text, rgb);
    native(text_context, ok ? DD_OK : E_FAIL);
    if (text_context->relock)
    {
        HRESULT hr;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = bh_renderer_surface_lock(frame_display->color, &desc);
        native(text_context, hr);
        if (SUCCEEDED(hr))
        {
            frame_display->locked = 1;
            frame_display->color_pixels = (unsigned char *)desc.lpSurface;
            frame_display->color_pitch = desc.lPitch;
            if (frame_display->atlas)
                frame_display->atlas->scan_step = desc.lPitch;
        }
        else
            ok = 0;
    }
    return ok;
}
void bh_gdi_text_dispose(BHGdiText *text_context)
{
    if (text_context)
        text_context->display = NULL;
}
