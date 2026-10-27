#include "bh_software_tables.h"
#include <string.h>

static unsigned long clipped(unsigned long n)
{
    return n > 255 ? 255 : n;
}
static unsigned char indexed(const BHTextureTables *t, const unsigned long rgb[3])
{
    return (unsigned char)((t->quant[1][rgb[0]] * 7 + t->quant[2][rgb[1]]) * 6 + t->quant[1][rgb[2]]);
}
static unsigned short packed(const BHTextureContext *q, const unsigned long rgb[3])
{
    unsigned long i, value = 0;
    for (i = 0; i < 3; ++i)
        value += (rgb[i] >> (q->display_right[i] & 31)) << (q->display_left[i] & 31);
    return (unsigned short)value;
}
static int display(const BHTextureContext *q, const unsigned long masks[3], BHSoftwareTables *t)
{
    unsigned long i, mask, low, bits, extra, target, used = 0;
    if (!masks)
        return 0;
    for (i = 0; i < 3; ++i)
    {
        mask = masks[i];
        if (!mask || mask > 65535 || (used & mask))
            return 0;
        used |= mask;
        low = bits = 0;
        while (!(mask & 1))
        {
            ++low;
            mask >>= 1;
        }
        while (mask & 1)
        {
            ++bits;
            mask >>= 1;
        }
        target = i == 2 ? 2 : 3;
        if (mask || bits < target || bits > 8 || low != q->display_left[i] || 8 - bits != q->display_right[i])
            return 0;
        extra = bits - target;
        t->rgb332_mask[i] = ((1UL << target) - 1) << (extra + low);
        t->rgb332_shift[i] = (long)(i == 0 ? 13 : i == 1 ? 10 : 8) - (long)extra - (long)low;
    }
    return 1;
}
void __cdecl bh_software_tables_dispose(BHSoftwareTables *t)
{
    if (!t)
        return;
    if (t->allocator.release)
    {
        if (t->shade_indexed)
            t->allocator.release(t->shade_indexed, t->allocator.user);
        if (t->blend_indexed)
            t->allocator.release(t->blend_indexed, t->allocator.user);
        if (t->shade_native)
            t->allocator.release(t->shade_native, t->allocator.user);
        if (t->blend_native)
            t->allocator.release(t->blend_native, t->allocator.user);
    }
    memset(t, 0, sizeof(*t));
}
int __cdecl bh_software_tables_initialize(BHSoftwareTables *out, BHTextureContext *q, unsigned long mode,
                                          const unsigned long masks[3])
{
    BHSoftwareTables t;
    unsigned char palette[256][3];
    unsigned long outer, inner, c, plane, weight, rgb[3], base[3], offset;
    if (!out || !q || !q->allocator.allocate || !q->allocator.release || (mode != 1 && mode != 2) ||
        out->shade_indexed || out->blend_indexed || out->shade_native || out->blend_native)
        return 0;
    memset(&t, 0, sizeof(t));
    t.allocator = q->allocator;
    t.bytes_per_pixel = mode;
    if (mode == 2 && !display(q, masks, &t))
        return 0;
    for (inner = 0; inner < 256; ++inner)
    {
        /* The last four entries wrap R to zero; original does not reserve
           them as black/white or load an external palette. */
        palette[inner][0] = (unsigned char)(((inner / 42) % 6) * 255 / 5);
        palette[inner][1] = (unsigned char)(((inner / 6) % 7) * 255 / 6);
        palette[inner][2] = (unsigned char)((inner % 6) * 255 / 5);
    }
    t.shade_indexed = (unsigned char *)t.allocator.allocate(0x20000, t.allocator.user);
    if (!t.shade_indexed)
        goto failed;
    if (!bh_texture_tables_init(q))
        goto failed;
    for (outer = 0; outer < 256; ++outer)
        for (inner = 0; inner < 256; ++inner)
        {
            for (c = 0; c < 3; ++c)
                rgb[c] = palette[inner][c] * outer / 255;
            offset = outer * 256 + inner;
            t.shade_indexed[offset] = indexed(&q->tables, rgb);
            rgb[0] = palette[inner][0] * ((outer >> 5) & 7) / 7;
            rgb[1] = palette[inner][1] * ((outer >> 2) & 7) / 7;
            rgb[2] = palette[inner][2] * (outer & 3) / 3;
            t.shade_indexed[65536 + offset] = indexed(&q->tables, rgb);
        }
    if (mode == 1)
    {
        t.blend_indexed = (unsigned char *)t.allocator.allocate(0x80000, t.allocator.user);
        if (!t.blend_indexed)
            goto failed;
    }
    else
    {
        t.shade_native = (unsigned short *)t.allocator.allocate(0x40000, t.allocator.user);
        if (!t.shade_native)
            goto failed;
        for (outer = 0; outer < 256; ++outer)
            for (inner = 0; inner < 256; ++inner)
            {
                for (c = 0; c < 3; ++c)
                    rgb[c] = palette[inner][c] * outer / 255;
                offset = outer * 256 + inner;
                t.shade_native[offset] = packed(q, rgb);
                rgb[0] = palette[inner][0] * ((outer >> 5) & 7) / 7;
                rgb[1] = palette[inner][1] * ((outer >> 2) & 7) / 7;
                rgb[2] = palette[inner][2] * (outer & 3) / 3;
                t.shade_native[65536 + offset] = packed(q, rgb);
            }
        t.blend_native = (unsigned short *)t.allocator.allocate(0x100000, t.allocator.user);
        if (!t.blend_native)
            goto failed;
    }
    for (plane = 0; plane < 8; ++plane)
        for (outer = 0; outer < 256; ++outer)
        {
            if (mode == 1)
                for (c = 0; c < 3; ++c)
                    base[c] = palette[outer][c];
            else
            {
                base[0] = outer & 0xe0;
                base[1] = (outer & 0x1c) * 8;
                base[2] = (outer & 3) * 64;
            }
            for (inner = 0; inner < 256; ++inner)
            {
                for (c = 0; c < 3; ++c)
                {
                    if (plane == 0)
                        rgb[c] = palette[inner][c] + (255 - palette[inner][c]) * base[c] / 255;
                    else if (plane == 1)
                        rgb[c] = (255 - palette[inner][c]) * base[c] / 255;
                    else if (plane == 2)
                        rgb[c] = palette[inner][c] + base[c];
                    else
                    {
                        weight = plane - 2;
                        rgb[c] = (palette[inner][c] * weight + base[c] * (6 - weight)) / 6;
                    }
                    rgb[c] = clipped(rgb[c]);
                }
                offset = plane * 65536 + outer * 256 + inner;
                if (mode == 1)
                    t.blend_indexed[offset] = indexed(&q->tables, rgb);
                else
                    t.blend_native[offset] = packed(q, rgb);
            }
        }
    *out = t;
    return 1;
failed:
    bh_software_tables_dispose(&t);
    return 0;
}
