#include "bh_loading_blit.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <float.h>
#include <string.h>
static int fail(BHLoadingBlit *loading_blit)
{
    if (loading_blit)
        ++loading_blit->failures;
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
static int valid(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *loading_blit_target, float from, float to)
{
    if (!loading_blit || !loading_blit->images || !loading_blit_target || !loading_blit_target->base || !loading_blit->images->arena || loading_blit->width < 1 || loading_blit->width > 4096 || loading_blit->height < 1 ||
        loading_blit->height > 4096 || !_finite(from) || !_finite(to) || from < -1000000 || from > 1000000 || to < -1000000 ||
        to > 1000000 || !bh_math_environment_supported())
        return fail(loading_blit);
    if (loading_blit->images->frame_stride > loading_blit->images->arena_bytes ||
        (unsigned long)loading_blit->width * (unsigned long)loading_blit->height > loading_blit->images->arena_bytes - loading_blit->images->frame_stride)
        return fail(loading_blit);
    return 1;
}
static int pixel(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *loading_blit_target, long x, long y, __int64 at)
{
    unsigned long n = loading_blit->images->pixel_bytes, value, i;
    unsigned char index;
    if (n != 1 && n != 2 && n != 4)
        return 1;
    if (at < 0 || at > (__int64)loading_blit_target->bytes - n)
        return fail(loading_blit);
    index = loading_blit->images->arena[loading_blit->images->frame_stride + (unsigned long)y * loading_blit->width + x];
    value = n == 1 ? loading_blit->images->indexed[index] : loading_blit->images->native[index];
    for (i = 0; i < n; ++i)
        loading_blit_target->base[(unsigned long)at + i] = (unsigned char)(value >> (8 * i));
    ++loading_blit->writes;
    return 1;
}
static int horizontal(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *loading_blit_target, long y, long lo, long hi)
{
    long x;
    unsigned long n = loading_blit->images->pixel_bytes;
    if (lo < 0)
        lo = 0;
    if (hi > loading_blit->width)
        hi = loading_blit->width;
    for (x = lo; x < hi; ++x)
        if (!pixel(loading_blit, loading_blit_target, x, y, (__int64)loading_blit_target->row0 + (__int64)y * loading_blit_target->pitch + (__int64)x * n))
            return 0;
    return 1;
}
static int vertical(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *loading_blit_target, long x, long lo, long hi)
{
    long y, step;
    unsigned long n = loading_blit->images->pixel_bytes;
    __int64 at;
    if (lo < 0)
        lo = 0;
    if (hi > loading_blit->height)
        hi = loading_blit->height;
    if (n != 1 && n != 2 && n != 4)
        return 1;
    step = (loading_blit_target->pitch / (long)n) * (long)n;
    at = (__int64)loading_blit_target->row0 + (__int64)lo * loading_blit_target->pitch + (__int64)x * n;
    for (y = lo; y < hi; ++y, at += step)
        if (!pixel(loading_blit, loading_blit_target, x, y, at))
            return 0;
    return 1;
}
int bh_loading_blit_top(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, y, p, lo, hi;
    double ray;
    float a, b;
    if (!valid(loading_blit, t, from, to))
        return 0;
    w = loading_blit->width / 2;
    h = loading_blit->height / 2;
    bh_round_float(&a, (double)from * 8.0);
    bh_round_float(&b, (double)to * 8.0);
    p = h * w;
    for (y = 0; y < h; ++y, p -= w)
    {
        ray = (double)p / h;
        lo = truncate_low(ray * a + w);
        hi = truncate_low(ray * b + w);
        if (!horizontal(loading_blit, t, y, lo, hi))
            return 0;
    }
    return 1;
}
int bh_loading_blit_right(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, x, p, q, lo, hi;
    double pa, qb;
    float a, b;
    if (!valid(loading_blit, t, from, to))
        return 0;
    w = loading_blit->width / 2;
    h = loading_blit->height / 2;
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
        if (!vertical(loading_blit, t, w + x, lo, hi))
            return 0;
    }
    return 1;
}
int bh_loading_blit_bottom(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, y, p, q, lo, hi;
    double pa, qb, ra, rb;
    float a, b;
    if (!valid(loading_blit, t, from, to))
        return 0;
    w = loading_blit->width / 2;
    h = loading_blit->height / 2;
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
        if (!horizontal(loading_blit, t, h + y, lo, hi))
            return 0;
    }
    return 1;
}
int bh_loading_blit_left(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, x, p, q, lo, hi;
    double pa, qb, ra, rb;
    float a, b;
    if (!valid(loading_blit, t, from, to))
        return 0;
    w = loading_blit->width / 2;
    h = loading_blit->height / 2;
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
        if (!vertical(loading_blit, t, w - x - 1, lo, hi))
            return 0;
    }
    return 1;
}
int bh_loading_blit_top_last(BHLoadingBlit *loading_blit, const BHLoadingBlitTarget *t, float from, float to)
{
    long w, h, y, p, q, lo, hi;
    double pa, qb;
    float a, b;
    if (!valid(loading_blit, t, from, to))
        return 0;
    w = loading_blit->width / 2;
    h = loading_blit->height / 2;
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
        if (!horizontal(loading_blit, t, y, lo, hi))
            return 0;
    }
    return 1;
}
