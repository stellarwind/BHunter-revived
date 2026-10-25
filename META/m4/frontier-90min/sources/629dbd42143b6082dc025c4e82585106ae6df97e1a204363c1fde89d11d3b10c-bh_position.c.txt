#include "bh_position.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>

static unsigned long word(const unsigned char *p) {
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8);
}

BHPositionResult __cdecl bh_position_read(BHPackedPositions *stream, const BHNavigation *navigation,
                                          BHWorldGeometry **world, BHVec3 *point) {
    const unsigned char *record;
    BHWorldGeometry *selected;
    BHVec3 next;
    const float *minimum, *maximum;
    float *out = (float *)&next;
    unsigned long index, axis;
    double t;
    if (!stream || !navigation || !world || !point || !stream->bytes ||
        stream->cursor > stream->size || stream->size - stream->cursor < 2 ||
        navigation->node_count > 65535 || (navigation->node_count && !navigation->nodes) ||
        !bh_math_environment_supported())
        return BH_POSITION_ERROR;
    record = stream->bytes + stream->cursor;
    index = word(record);
    if (index >= navigation->node_count) {
        stream->cursor += 2;
        ++stream->rejected;
        --stream->accepted;
        return BH_POSITION_REJECTED;
    }
    if (stream->size - stream->cursor < 8)
        return BH_POSITION_ERROR;
    selected = navigation->nodes[index].placement;
    if (!selected)
        return BH_POSITION_ERROR;
    minimum = (const float *)&selected->minimum;
    maximum = (const float *)&selected->maximum;
    for (axis = 0; axis < 3; ++axis)
        if (!_finite(minimum[axis]) || !_finite(maximum[axis]))
            return BH_POSITION_ERROR;
    for (axis = 0; axis < 3; ++axis) {
        t = ((double)word(record + 2 + axis * 2) - 16384.0) * (1.0 / 32768.0);
        bh_round_float(out + axis, (double)maximum[axis] * t + (double)minimum[axis] * (1.0 - t));
    }
    *world = selected;
    *point = next;
    stream->cursor += 8;
    return BH_POSITION_OK;
}
