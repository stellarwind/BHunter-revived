#include "bh_atlas_glyph.h"
#include <string.h>
static unsigned long packed(unsigned long color, const DDPIXELFORMAT *pixel)
{
    unsigned long masks[3], rgb[3], i, result = 0;
    long mask;
    int shift;
    masks[0] = pixel->dwRBitMask;
    masks[1] = pixel->dwGBitMask;
    masks[2] = pixel->dwBBitMask;
    rgb[0] = (color >> 16) & 255;
    rgb[1] = (color >> 8) & 255;
    rgb[2] = color & 255;
    for (i = 0; i < 3; ++i)
    {
        shift = -8;
        for (mask = (long)masks[i]; mask; mask /= 2)
            ++shift;
        result += (shift < 0 ? rgb[i] >> (-shift) : rgb[i] << shift) & masks[i];
    }
    return result;
}
static unsigned long nearest(unsigned long color, const unsigned char *palette)
{
    unsigned long i, result = 0;
    long r, g, b, distance, best = 99999;
    for (i = 0; i < 256; ++i)
    {
        r = (long)palette[i * 3] - (long)((color >> 16) & 255);
        if (r < 0)
            r = -r;
        g = (long)palette[i * 3 + 1] - (long)((color >> 8) & 255);
        if (g < 0)
            g = -g;
        b = (long)palette[i * 3 + 2] - (long)(color & 255);
        if (b < 0)
            b = -b;
        distance = r + g + b;
        if (distance < best)
        {
            best = distance;
            result = i;
        }
    }
    return result;
}
int __cdecl bh_atlas_page_clear(BHAtlasContext *context, BHAtlasPage *page, unsigned long color)
{
    BHTextureContext *q;
    DDSURFACEDESC lock;
    HRESULT hr;
    unsigned long bits, bytes, x, y, i;
    int ok;
    if (!page)
        return 1;
    if (!context || !(q = context->textures) || !page->atlas || !page->image || !page->surface || !page->size ||
        page->size > 4096)
        return 0;
    bits = page->atlas->pixel.dwRGBBitCount;
    bytes = (bits + 7) / 8;
    if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
    {
        q->error = BH_TEXTURE_FORMAT;
        return 0;
    }
    q->error = BH_TEXTURE_OK;
    if (page->last_color != color)
    {
        page->last_color = color;
        page->packed_color = bits == 8 ? nearest(color, page->atlas->palette) : packed(color, &page->atlas->pixel);
    }
    memset(&lock, 0, sizeof(lock));
    lock.dwSize = sizeof(lock);
    hr = IDirectDrawSurface_Lock(page->surface, NULL, &lock, 0, NULL);
    q->last_hresult = hr;
    ok = hr == DD_OK;
    if (ok)
    {
        if (!lock.lpSurface || lock.dwWidth < page->size || lock.dwHeight < page->size ||
            lock.lPitch < (long)(page->size * bytes))
        {
            q->error = BH_TEXTURE_TOPOLOGY;
            ok = 0;
        }
        if (ok)
            for (y = 0; y < page->size; ++y)
                for (x = 0; x < page->size; ++x)
                {
                    unsigned char *out = (unsigned char *)lock.lpSurface + y * lock.lPitch + x * bytes;
                    for (i = 0; i < bytes; ++i)
                        out[i] = (unsigned char)(page->packed_color >> (8 * i));
                }
        hr = IDirectDrawSurface_Unlock(page->surface, NULL);
        q->last_hresult = hr;
        if (hr != DD_OK)
        {
            q->error = BH_TEXTURE_BACKEND;
            ok = 0;
        }
    }
    else
        q->error = BH_TEXTURE_BACKEND;
    page->left = page->cursor_x = page->cursor_y = page->line_height = 0;
    if (page->image->resident)
        page->image->resident->pixels_dirty = 1;
    return ok;
}
void __cdecl bh_atlas_page_cursor(BHAtlasPage *page, long x, long y)
{
    if (page)
    {
        page->line_height = 0;
        page->left = page->cursor_x = x;
        page->cursor_y = y;
    }
}
