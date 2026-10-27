#include "bh_loading_blit.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <string.h>
static int fail(BHLoadingBlit *c)
{
    if (c)
        ++c->failures;
    return 0;
}
static long truncate_low(double value)
{
    __int64 whole = (__int64)value;
    unsigned long low = (unsigned long)whole;
    long result;
    memcpy(&result, &low, 4);
    return result;
}
static int valid(BHLoadingBlit *c, const BHLoadingBlitTarget *t, float from, float to)
{
    if (!c || !c->images || !t || !t->base || !c->images->arena || c->width < 1 || c->width > 4096 || c->height < 1 ||
        c->height > 4096 || !_finite(from) || !_finite(to) || from < -1000000 || from > 1000000 || to < -1000000 ||
        to > 1000000 || !bh_math_environment_supported())
        return fail(c);
    if (c->images->frame_stride > c->images->arena_bytes ||
        (unsigned long)c->width * (unsigned long)c->height > c->images->arena_bytes - c->images->frame_stride)
        return fail(c);
    return 1;
}
static int pixel(BHLoadingBlit *c, const BHLoadingBlitTarget *t, long x, long y, __int64 at)
{
    unsigned long n = c->images->pixel_bytes, value, i;
    unsigned char index;
    if (n != 1 && n != 2 && n != 4)
        return 1;
    if (at < 0 || at > (__int64)t->bytes - n)
        return fail(c);
    index = c->images->arena[c->images->frame_stride + (unsigned long)y * c->width + x];
    value = n == 1 ? c->images->indexed[index] : c->images->native[index];
    for (i = 0; i < n; ++i)
        t->base[(unsigned long)at + i] = (unsigned char)(value >> (8 * i));
    ++c->writes;
    return 1;
}
static int horizontal(BHLoadingBlit *c, const BHLoadingBlitTarget *t, long y, long lo, long hi)
{
    long x;
    unsigned long n = c->images->pixel_bytes;
    if (lo < 0)
        lo = 0;
    if (hi > c->width)
        hi = c->width;
    for (x = lo; x < hi; ++x)
        if (!pixel(c, t, x, y, (__int64)t->row0 + (__int64)y * t->pitch + (__int64)x * n))
            return 0;
    return 1;
}
static int vertical(BHLoadingBlit *c, const BHLoadingBlitTarget *t, long x, long lo, long hi)
{
    long y, step;
    unsigned long n = c->images->pixel_bytes;
    __int64 at;
    if (lo < 0)
        lo = 0;
    if (hi > c->height)
        hi = c->height;
    if (n != 1 && n != 2 && n != 4)
        return 1;
    step = (t->pitch / (long)n) * (long)n;
    at = (__int64)t->row0 + (__int64)lo * t->pitch + (__int64)x * n;
    for (y = lo; y < hi; ++y, at += step)
        if (!pixel(c, t, x, y, at))
            return 0;
    return 1;
}
int __cdecl bh_loading_blit_top(BHLoadingBlit *c, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, y, p, lo, hi;
    double ray;
    float a, b;
    if (!valid(c, t, from, to))
        return 0;
    w = c->width / 2;
    h = c->height / 2;
    bh_round_float(&a, (double)from * 8.0);
    bh_round_float(&b, (double)to * 8.0);
    p = h * w;
    for (y = 0; y < h; ++y, p -= w)
    {
        ray = (double)p / h;
        lo = truncate_low(ray * a + w);
        hi = truncate_low(ray * b + w);
        if (!horizontal(c, t, y, lo, hi))
            return 0;
    }
    return 1;
}
int __cdecl bh_loading_blit_right(BHLoadingBlit *c, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, x, p, q, lo, hi;
    double pa, qb;
    float a, b;
    if (!valid(c, t, from, to))
        return 0;
    w = c->width / 2;
    h = c->height / 2;
    bh_round_float(&a, ((double)from - .125) * 4.0);
    bh_round_float(&b, ((double)to - .125) * 4.0);
    p = h * w;
    q = 0;
    for (x = 0; x < w; ++x, p -= h, q += h * 2)
    {
        pa = (double)p / w;
        qb = (double)q / w;
        lo = truncate_low(qb * a + pa);
        hi = truncate_low((qb * b + pa) + 1.0);
        if (!vertical(c, t, w + x, lo, hi))
            return 0;
    }
    return 1;
}
int __cdecl bh_loading_blit_bottom(BHLoadingBlit *c, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, y, p, q, lo, hi;
    double pa, qb, ra, rb;
    float a, b;
    if (!valid(c, t, from, to))
        return 0;
    w = c->width / 2;
    h = c->height / 2;
    bh_round_float(&a, ((double)from - .375) * 4.0);
    bh_round_float(&b, ((double)to - .375) * 4.0);
    ra = 1.0 - a;
    rb = 1.0 - b;
    p = h * w;
    q = 0;
    for (y = 0; y < h; ++y, p -= w, q += w * 2)
    {
        pa = (double)p / h;
        qb = (double)q / h;
        lo = truncate_low(qb * rb + pa);
        hi = truncate_low((qb * ra + pa) + 1.0);
        if (!horizontal(c, t, h + y, lo, hi))
            return 0;
    }
    return 1;
}
int __cdecl bh_loading_blit_left(BHLoadingBlit *c, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, x, p, q, lo, hi;
    double pa, qb, ra, rb;
    float a, b;
    if (!valid(c, t, from, to))
        return 0;
    w = c->width / 2;
    h = c->height / 2;
    bh_round_float(&a, ((double)from - .625) * 4.0);
    bh_round_float(&b, ((double)to - .625) * 4.0);
    ra = 1.0 - a;
    rb = 1.0 - b;
    p = h * w;
    q = 0;
    for (x = 0; x < w; ++x, p -= h, q += h * 2)
    {
        pa = (double)p / w;
        qb = (double)q / w;
        lo = truncate_low(qb * rb + pa);
        hi = truncate_low((qb * ra + pa) + 1.0);
        if (!vertical(c, t, w - x - 1, lo, hi))
            return 0;
    }
    return 1;
}
int __cdecl bh_loading_blit_top_last(BHLoadingBlit *c, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, y, p, q, lo, hi;
    double pa, qb;
    float a, b;
    if (!valid(c, t, from, to))
        return 0;
    w = c->width / 2;
    h = c->height / 2;
    bh_round_float(&a, ((double)from - .875) * 8.0);
    bh_round_float(&b, ((double)to - .875) * 8.0);
    p = h * w;
    q = 0;
    for (y = 0; y < h; ++y, p -= w, q += w)
    {
        pa = (double)p / h;
        qb = (double)q / h;
        lo = truncate_low(pa * a + qb);
        hi = truncate_low(pa * b + qb);
        if (!horizontal(c, t, y, lo, hi))
            return 0;
    }
    return 1;
}
