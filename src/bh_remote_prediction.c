#include "bh_remote_prediction.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
int __cdecl bh_remote_predict_position(const BHEntity *entity, unsigned short tick, BHVec3 *out)
{
    BHVec3 anchor, velocity, acceleration, position;
    unsigned short stamp, elapsed;
    long delta;
    double t, half_squared;
    unsigned long i;
    float *p = &position.x;
    if (!entity || !out || !bh_math_environment_supported())
        return 0;
    memcpy(&stamp, (const unsigned char *)entity + 0x17c, 2);
    memcpy(&anchor, (const unsigned char *)entity + 0x180, 12);
    elapsed = (unsigned short)(tick - stamp);
    delta = elapsed & 0x8000 ? (long)elapsed - 65536 : (long)elapsed;
    if (delta <= 0)
    {
        *out = anchor;
        return 1;
    }
    memcpy(&velocity, (const unsigned char *)entity + 0x48, 12);
    memcpy(&acceleration, (const unsigned char *)entity + 0x60, 12);
    for (i = 0; i < 3; ++i)
        if (!_finite((&anchor.x)[i]) || !_finite((&velocity.x)[i]) || !_finite((&acceleration.x)[i]))
            return 0;
    t = (double)delta;
    half_squared = (t * t) * 0.5;
    for (i = 0; i < 3; ++i)
        bh_round_float(p + i, (double)(&velocity.x)[i] * t + (&anchor.x)[i]);
    for (i = 0; i < 3; ++i)
        bh_round_float(p + i, (double)(&acceleration.x)[i] * half_squared + p[i]);
    *out = position;
    return 1;
}
int __cdecl bh_remote_choose_slot(const unsigned long (*rows)[32], unsigned long count, long row, long first,
                                  long second, long *selected)
{
    const unsigned long *slots;
    unsigned long product;
    long rank, ordinal = 0, i, value;
    if (!rows || !selected || row < 0 || (unsigned long)row >= count || first > 30 || second > 30)
        return 0;
    slots = rows[row];
    if (first >= 0 && second >= 0 && slots[first] && slots[second])
    {
        *selected = rand() < 0x4000 ? first : second;
        return 1;
    }
    if (first >= 0 && slots[first])
    {
        *selected = first;
        return 1;
    }
    if (second >= 0 && slots[second])
    {
        *selected = second;
        return 1;
    }
    product = (unsigned long)rand() * slots[31];
    memcpy(&value, &product, 4);
    rank = value / 32768;
    for (i = 0; i < 31; ++i)
        if (slots[i])
        {
            if (ordinal == rank)
            {
                *selected = i;
                return 1;
            }
            ++ordinal;
        }
    *selected = 0;
    return 1;
}
