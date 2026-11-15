#include "bh_network_codec.h"
#include "../Math/bh_fp.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <string.h>

static_assert(sizeof(unsigned long) == 4 && sizeof(unsigned short) == 2 && sizeof(__int64) == 8,
              "packet integer widths");
uint16_t bh_packet_read_u16(const unsigned char *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static unsigned long read32(const unsigned char *p)
{
    return p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}
void bh_packet_write_u16(unsigned char *p, uint16_t v)
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
static int finite_vector(const BHVec3 *vector)
{
    return vector && _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static int space(BHPacketWriter *packet_writer, unsigned long count)
{
    if (!packet_writer || !packet_writer->data || packet_writer->finished || packet_writer->cursor < 8 || packet_writer->cursor > packet_writer->capacity)
    {
        if (packet_writer)
            packet_writer->error = BH_PACKET_ARGUMENT;
        return 0;
    }
    if (packet_writer->cursor - 8 > 255 || count > 255 - (packet_writer->cursor - 8) || count > packet_writer->capacity - packet_writer->cursor)
    {
        packet_writer->error = BH_PACKET_CAPACITY;
        return 0;
    }
    return 1;
}
int bh_packet_begin(BHPacketWriter *w, void *bytes, unsigned long capacity, unsigned char type, unsigned short sender)
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
    bh_packet_write_u16(next.data + 2, sender);
    *w = next;
    return 1;
}
int bh_packet_append(BHPacketWriter *packet_writer, const void *bytes, unsigned long count)
{
    if (!bytes && count)
    {
        if (packet_writer)
            packet_writer->error = BH_PACKET_ARGUMENT;
        return 0;
    }
    if (!space(packet_writer, count))
        return 0;
    if (count)
        memmove(packet_writer->data + packet_writer->cursor, bytes, count);
    packet_writer->cursor += count;
    packet_writer->error = BH_PACKET_OK;
    return 1;
}
int bh_packet_finish(BHPacketWriter *packet_writer, unsigned long *wire_size)
{
    unsigned long end, aligned, checksum, i;
    if (!wire_size || !space(packet_writer, 0))
        return 0;
    end = packet_writer->cursor;
    aligned = (end + 3) & ~3UL;
    if (aligned > packet_writer->capacity)
    {
        packet_writer->error = BH_PACKET_CAPACITY;
        return 0;
    }
    packet_writer->data[1] = (unsigned char)(end - 8);
    memset(packet_writer->data + end, 0, aligned - end);
    checksum = read32(packet_writer->data);
    for (i = 8; i < aligned; i += 4)
        checksum ^= read32(packet_writer->data + i);
    write32(packet_writer->data + 4, checksum);
    packet_writer->cursor = aligned;
    packet_writer->finished = 1;
    *wire_size = end;
    packet_writer->error = BH_PACKET_OK;
    return 1;
}
int bh_packet_write_position(BHPacketWriter *packet_writer, const BHWorldGeometry *world, unsigned short index, const BHVec3 *point)
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
        if (packet_writer)
            packet_writer->error = BH_PACKET_NUMERICAL;
        return -1;
    }
    if (!space(packet_writer, 8))
        return -1;
    bh_packet_write_u16(data, index);
    for (i = 0; i < 3; ++i)
    {
        t = ((double)((const float *)point)[i] - ((const float *)&world->minimum)[i]) /
            ((double)((const float *)&world->maximum)[i] - ((const float *)&world->minimum)[i]);
        if (!(t >= -.5))
            t = -.5;
        else if (!(t < 1.5))
            t = (double)1.49f;
        value = (__int64)(16384.0 - t * (-32768.0));
        bh_packet_write_u16(data + 2 + i * 2, (uint16_t)value);
    }
    memcpy(packet_writer->data + packet_writer->cursor, data, 8);
    packet_writer->cursor += 8;
    packet_writer->error = BH_PACKET_OK;
    return 1;
}
int bh_packet_write_vector(BHPacketWriter *packet_writer, const BHVec3 *v, float scale)
{
    unsigned char data[3];
    unsigned long i, low;
    long value;
    __int64 converted;
    double divisor;
    if (!finite_vector(v) || !_finite(scale) || !bh_math_environment_supported())
    {
        if (packet_writer)
            packet_writer->error = BH_PACKET_NUMERICAL;
        return 0;
    }
    if (!space(packet_writer, 3))
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
    memcpy(packet_writer->data + packet_writer->cursor, data, 3);
    packet_writer->cursor += 3;
    packet_writer->error = BH_PACKET_OK;
    return 1;
}
int bh_packet_receive(BHPacketReader *packet_reader, void *bytes, unsigned long size, unsigned long capacity, BHPacketStats *stats)
{
    BHPacketReader next;
    unsigned long end, aligned, checksum, i;
    unsigned char *p = (unsigned char *)bytes;
    if (!packet_reader || !p || !stats || size < 8 || size > capacity)
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
    *packet_reader = next;
    ++stats->accepted;
    memset(p + end, 0, aligned - end);
    checksum = read32(p);
    for (i = 8; i < aligned; i += 4)
        checksum ^= read32(p + i);
    if (checksum == read32(p + 4))
        return 1;
    --stats->accepted;
    ++stats->rejected;
    packet_reader->error = BH_PACKET_CHECKSUM;
    return 0;
}
static int available(BHPacketReader *packet_reader, unsigned long size)
{
    if (!packet_reader || !packet_reader->data || !packet_reader->stats || packet_reader->cursor > packet_reader->size || size > packet_reader->size - packet_reader->cursor)
    {
        if (packet_reader)
            packet_reader->error = BH_PACKET_TRUNCATED;
        return 0;
    }
    return 1;
}
static int reject_index(BHPacketReader *packet_reader)
{
    --packet_reader->stats->accepted;
    ++packet_reader->stats->rejected;
    packet_reader->error = BH_PACKET_INDEX;
    return 0;
}
int bh_packet_read_position(BHPacketReader *packet_reader, const BHNavigation *navigation, BHWorldGeometry **world, BHVec3 *point)
{
    BHPackedPositions stream;
    BHPositionResult result;
    if (!available(packet_reader, 0))
        return -1;
    stream.bytes = packet_reader->data;
    stream.size = packet_reader->size;
    stream.cursor = packet_reader->cursor;
    stream.accepted = packet_reader->stats->accepted;
    stream.rejected = packet_reader->stats->rejected;
    result = bh_position_read(&stream, navigation, world, point);
    packet_reader->cursor = stream.cursor;
    packet_reader->stats->accepted = stream.accepted;
    packet_reader->stats->rejected = stream.rejected;
    packet_reader->error = result == BH_POSITION_OK         ? BH_PACKET_OK
               : result == BH_POSITION_REJECTED ? BH_PACKET_INDEX
                                                : BH_PACKET_ARGUMENT;
    return result;
}
int bh_packet_read_actor(BHPacketReader *packet_reader, unsigned long local_id, BHEntity *local, BHEntity *const remote[8],
                         BHEntity **actor)
{
    unsigned short id;
    if (!remote || !actor)
    {
        if (packet_reader)
            packet_reader->error = BH_PACKET_ARGUMENT;
        return -1;
    }
    if (!available(packet_reader, 2))
        return -1;
    id = bh_packet_read_u16(packet_reader->data + packet_reader->cursor);
    packet_reader->cursor += 2;
    if (id == local_id)
    {
        *actor = local;
        packet_reader->error = BH_PACKET_OK;
        return 1;
    }
    if (id >= 8)
        return reject_index(packet_reader);
    if (!remote[id])
        return 0;
    *actor = remote[id];
    packet_reader->error = BH_PACKET_OK;
    return 1;
}
