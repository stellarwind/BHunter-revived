#ifndef BH_OVERLAY_TEXT_H
#define BH_OVERLAY_TEXT_H
#include "bh_overlay_font.h"
#include <stddef.h>
#include <string.h>

/* Original Comic Sans MS glyphs, baked at 24 pixels with quality 3.
   Runtime composition uses portable buffers and preserves TextOutA metrics. */
typedef struct BHOverlayCanvas
{
    unsigned char *pixels;
    unsigned width, height, pitch, bytes;
    unsigned long masks[3];
    const unsigned char *palette;
} BHOverlayCanvas;
typedef struct BHOverlayFont
{
    const unsigned char *mask;
    unsigned width, height;
    BHOverlayGlyph glyphs[256];
} BHOverlayFont;
static inline unsigned bh_overlay_u16(const unsigned char *p)
{
    return p[0] | (unsigned)p[1] << 8;
}
static inline unsigned long bh_overlay_u32(const unsigned char *p)
{
    return p[0] | (unsigned long)p[1] << 8 | (unsigned long)p[2] << 16 | (unsigned long)p[3] << 24;
}
static inline int bh_overlay_font_read(BHOverlayFont *font, const unsigned char *data, size_t bytes)
{
    unsigned i, width, height;
    if (!font || !data || bytes < 24 + 256 * 16 || memcmp(data, "BHCFONT1", 8) || bh_overlay_u32(data + 16) != 24 ||
        bh_overlay_u32(data + 20) != 256)
        return 0;
    width = bh_overlay_u32(data + 8);
    height = bh_overlay_u32(data + 12);
    if (!width || !height || width > 4096 || height > 4096 || bytes != 24 + 256 * 16 + (size_t)width * height)
        return 0;
    for (i = 0; i < 256; ++i)
    {
        const unsigned char *p = data + 24 + i * 16;
        BHOverlayGlyph *glyph = font->glyphs + i;
        glyph->x = bh_overlay_u16(p);
        glyph->y = bh_overlay_u16(p + 2);
        glyph->width = bh_overlay_u16(p + 4);
        glyph->height = bh_overlay_u16(p + 6);
        glyph->offset_x = (short)bh_overlay_u16(p + 8);
        glyph->offset_y = (short)bh_overlay_u16(p + 10);
        glyph->advance = (short)bh_overlay_u16(p + 12);
        glyph->reserved = (short)bh_overlay_u16(p + 14);
        if (glyph->x + (unsigned)glyph->width > width || glyph->y + (unsigned)glyph->height > height ||
            glyph->reserved || glyph->advance < 0 || glyph->advance > 128 || glyph->offset_x < -128 ||
            glyph->offset_x > 128 || glyph->offset_y < -128 || glyph->offset_y > 128)
            return 0;
    }
    font->width = width;
    font->height = height;
    font->mask = data + 24 + 256 * 16;
    return 1;
}

static inline int bh_overlay_color(const BHOverlayCanvas *canvas, unsigned long rgb, unsigned long *packed)
{
    unsigned long color = 0, used = 0;
    unsigned i;
    if (canvas->bytes == 1)
    {
        unsigned best = ~0u, index = 0;
        if (!canvas->palette)
            return 0;
        for (i = 0; i < 256; ++i)
        {
            unsigned channel, distance = 0;
            for (channel = 0; channel < 3; ++channel)
            {
                int delta = (int)((rgb >> (16 - channel * 8)) & 255) - canvas->palette[i * 3 + channel];
                distance += delta < 0 ? -delta : delta;
            }
            if (distance < best)
            {
                best = distance;
                index = i;
            }
        }
        *packed = index;
        return 1;
    }
    for (i = 0; i < 3; ++i)
    {
        unsigned long mask = canvas->masks[i], shifted = mask;
        unsigned shift = 0, bits = 0;
        if (!mask || (mask & used))
            return 0;
        used |= mask;
        while (!(shifted & 1))
        {
            shifted >>= 1;
            ++shift;
        }
        while (shifted & 1)
        {
            shifted >>= 1;
            ++bits;
        }
        if (shifted || bits > 8 || shift + bits > canvas->bytes * 8)
            return 0;
        color |= ((((rgb >> (16 - i * 8)) & 255) >> (8 - bits)) << shift) & mask;
    }
    /* An opaque black texel must differ from the transparent zero sentinel. */
    if (canvas->bytes == 4)
        color |= 0xff000000UL & ~used;
    *packed = color;
    return 1;
}

static inline int bh_overlay_text(BHOverlayCanvas *canvas, const BHOverlayFont *font, int x, int y, const char *text,
                                  unsigned long rgb)
{
    const unsigned char *at;
    unsigned long color;
    if (!canvas || !font || !text || !memchr(text, 0, 59) || !canvas->pixels || !font->mask || !canvas->width ||
        !canvas->height || canvas->width > 4096 || canvas->height > 4096 || canvas->bytes < 1 || canvas->bytes > 4 ||
        canvas->pitch < canvas->width * canvas->bytes || !bh_overlay_color(canvas, rgb, &color))
        return 0;
    for (at = (const unsigned char *)text; *at; ++at)
    {
        const BHOverlayGlyph *glyph = font->glyphs + *at;
        int gx = x + glyph->offset_x, gy = y + glyph->offset_y;
        int left = gx < 0 ? 0 : gx, top = gy < 0 ? 0 : gy;
        int right = gx + glyph->width, bottom = gy + glyph->height, px, py;
        if (right > (int)canvas->width)
            right = canvas->width;
        if (bottom > (int)canvas->height)
            bottom = canvas->height;
        for (py = top; py < bottom; ++py)
            for (px = left; px < right; ++px)
                if (font->mask[(glyph->y + py - gy) * font->width + glyph->x + px - gx])
                {
                    unsigned byte;
                    unsigned char *out = canvas->pixels + (size_t)py * canvas->pitch + px * canvas->bytes;
                    for (byte = 0; byte < canvas->bytes; ++byte)
                        out[byte] = (unsigned char)(color >> (byte * 8));
                }
        x += glyph->advance;
    }
    return 1;
}
#endif
