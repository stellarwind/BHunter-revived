#include "bh_network_codec.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>

typedef char
    packet_integer_widths[sizeof(unsigned long) == 4 && sizeof(unsigned short) == 2 && sizeof(__int64) == 8 ? 1 : -1];
static unsigned short read16(const unsigned char *p)
{
    return (unsigned short)(p[0] | ((unsigned short)p[1] << 8));
}
static unsigned long read32(const unsigned char *p)
{
    return p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}
static void write16(unsigned char *p, unsigned short v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}
static void write32(unsigned char *p, unsigned long v)
{
    unsigned long i;
    for (i = 0; i < 4; ++i)
        p[i] = (unsigned char)(v >> (i * 8));
}
static int finite_vector(const BHVec3 *p)
{
    return p && _finite(p->x) && _finite(p->y) && _finite(p->z);
}
static int space(BHPacketWriter *w, unsigned long count)
{
    if (!w || !w->data || w->finished || w->cursor < 8 || w->cursor > w->capacity)
    {
        if (w)
            w->error = BH_PACKET_ARGUMENT;
        return 0;
    }
    if (w->cursor - 8 > 255 || count > 255 - (w->cursor - 8) || count > w->capacity - w->cursor)
    {
        w->error = BH_PACKET_CAPACITY;
        return 0;
    }
    return 1;
}
int __cdecl bh_packet_begin(BHPacketWriter *w, void *bytes, unsigned long capacity, unsigned char type,
                            unsigned short sender)
{
    BHPacketWriter next;
    if (!w || !bytes || capacity < 8)
        return 0;
    memset(&next, 0, sizeof(next));
    next.data = (unsigned char *)bytes;
    next.capacity = capacity;
    next.cursor = 8;
    memset(bytes, 0, 8);
    next.data[0] = type;
    write16(next.data + 2, sender);
    *w = next;
    return 1;
}
int __cdecl bh_packet_append(BHPacketWriter *w, const void *bytes, unsigned long count)
{
    if (!bytes && count)
    {
        if (w)
            w->error = BH_PACKET_ARGUMENT;
        return 0;
    }
    if (!space(w, count))
        return 0;
    if (count)
        memmove(w->data + w->cursor, bytes, count);
    w->cursor += count;
    w->error = BH_PACKET_OK;
    return 1;
}
int __cdecl bh_packet_finish(BHPacketWriter *w, unsigned long *wire_size)
{
    unsigned long end, aligned, checksum, i;
    if (!wire_size || !space(w, 0))
        return 0;
    end = w->cursor;
    aligned = (end + 3) & ~3UL;
    if (aligned > w->capacity)
    {
        w->error = BH_PACKET_CAPACITY;
        return 0;
    }
    w->data[1] = (unsigned char)(end - 8);
    memset(w->data + end, 0, aligned - end);
    checksum = read32(w->data);
    for (i = 8; i < aligned; i += 4)
        checksum ^= read32(w->data + i);
    write32(w->data + 4, checksum);
    w->cursor = aligned;
    w->finished = 1;
    *wire_size = end;
    w->error = BH_PACKET_OK;
    return 1;
}
int __cdecl bh_packet_write_position(BHPacketWriter *w, const BHWorldGeometry *world, unsigned short index,
                                     const BHVec3 *point)
{
    unsigned char data[8];
    unsigned long i;
    double t;
    __int64 value;
    if (!world)
        return 0;
    if (!finite_vector(point) || !finite_vector(&world->minimum) || !finite_vector(&world->maximum) ||
        !bh_math_environment_supported())
    {
        if (w)
            w->error = BH_PACKET_NUMERICAL;
        return -1;
    }
    if (!space(w, 8))
        return -1;
    write16(data, index);
    for (i = 0; i < 3; ++i)
    {
        t = ((double)((const float *)point)[i] - ((const float *)&world->minimum)[i]) /
            ((double)((const float *)&world->maximum)[i] - ((const float *)&world->minimum)[i]);
        if (!(t >= -.5))
            t = -.5;
        else if (!(t < 1.5))
            t = (double)1.49f;
        value = (__int64)(16384.0 - t * (-32768.0));
        write16(data + 2 + i * 2, (unsigned short)value);
    }
    memcpy(w->data + w->cursor, data, 8);
    w->cursor += 8;
    w->error = BH_PACKET_OK;
    return 1;
}
int __cdecl bh_packet_write_vector(BHPacketWriter *w, const BHVec3 *v, float scale)
{
    unsigned char data[3];
    unsigned long i, low;
    long value;
    __int64 converted;
    double divisor;
    if (!finite_vector(v) || !_finite(scale) || !bh_math_environment_supported())
    {
        if (w)
            w->error = BH_PACKET_NUMERICAL;
        return 0;
    }
    if (!space(w, 3))
        return 0;
    divisor = (double)scale * .0078125;
    for (i = 0; i < 3; ++i)
    {
        converted = (__int64)(((const float *)v)[i] / divisor);
        low = (unsigned long)converted;
        memcpy(&value, &low, 4);
        if (value > 127)
            value = 127;
        else if (value < -128)
            value = -128;
        data[i] = (unsigned char)value;
    }
    memcpy(w->data + w->cursor, data, 3);
    w->cursor += 3;
    w->error = BH_PACKET_OK;
    return 1;
}
int __cdecl bh_packet_receive(BHPacketReader *r, void *bytes, unsigned long size, unsigned long capacity,
                              BHPacketStats *stats)
{
    BHPacketReader next;
    unsigned long end, aligned, checksum, i;
    unsigned char *p = (unsigned char *)bytes;
    if (!r || !p || !stats || size < 8 || size > capacity)
        return -1;
    end = 8 + p[1];
    aligned = (end + 3) & ~3UL;
    if (end > size || aligned > capacity)
        return -1;
    memset(&next, 0, sizeof(next));
    next.data = p;
    next.size = end;
    next.cursor = 8;
    next.stats = stats;
    *r = next;
    ++stats->accepted;
    memset(p + end, 0, aligned - end);
    checksum = read32(p);
    for (i = 8; i < aligned; i += 4)
        checksum ^= read32(p + i);
    if (checksum == read32(p + 4))
        return 1;
    --stats->accepted;
    ++stats->rejected;
    r->error = BH_PACKET_CHECKSUM;
    return 0;
}
static int available(BHPacketReader *r, unsigned long size)
{
    if (!r || !r->data || !r->stats || r->cursor > r->size || size > r->size - r->cursor)
    {
        if (r)
            r->error = BH_PACKET_TRUNCATED;
        return 0;
    }
    return 1;
}
static int reject_index(BHPacketReader *r)
{
    --r->stats->accepted;
    ++r->stats->rejected;
    r->error = BH_PACKET_INDEX;
    return 0;
}
int __cdecl bh_packet_read_position(BHPacketReader *r, const BHNavigation *navigation, BHWorldGeometry **world,
                                    BHVec3 *point)
{
    BHPackedPositions stream;
    BHPositionResult result;
    if (!available(r, 0))
        return -1;
    stream.bytes = r->data;
    stream.size = r->size;
    stream.cursor = r->cursor;
    stream.accepted = r->stats->accepted;
    stream.rejected = r->stats->rejected;
    result = bh_position_read(&stream, navigation, world, point);
    r->cursor = stream.cursor;
    r->stats->accepted = stream.accepted;
    r->stats->rejected = stream.rejected;
    r->error = result == BH_POSITION_OK         ? BH_PACKET_OK
               : result == BH_POSITION_REJECTED ? BH_PACKET_INDEX
                                                : BH_PACKET_ARGUMENT;
    return result;
}
int __cdecl bh_packet_read_actor(BHPacketReader *r, unsigned long local_id, BHEntity *local, BHEntity *const remote[8],
                                 BHEntity **actor)
{
    unsigned short id;
    if (!remote || !actor)
    {
        if (r)
            r->error = BH_PACKET_ARGUMENT;
        return -1;
    }
    if (!available(r, 2))
        return -1;
    id = read16(r->data + r->cursor);
    r->cursor += 2;
    if (id == local_id)
    {
        *actor = local;
        r->error = BH_PACKET_OK;
        return 1;
    }
    if (id >= 8)
        return reject_index(r);
    if (!remote[id])
        return 0;
    *actor = remote[id];
    r->error = BH_PACKET_OK;
    return 1;
}
