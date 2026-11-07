#include "bh_remote_position.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static unsigned long word(BHEntity *entity, unsigned long o)
{
    unsigned long n;
    memcpy(&n, (char *)entity + o, 4);
    return n;
}
static void setword(BHEntity *entity, unsigned long o, unsigned long n)
{
    memcpy((char *)entity + o, &n, 4);
}
static unsigned short half(BHEntity *entity, unsigned long o)
{
    unsigned short n;
    memcpy(&n, (char *)entity + o, 2);
    return n;
}
static void sethalf(BHEntity *entity, unsigned long o, unsigned short n)
{
    memcpy((char *)entity + o, &n, 2);
}
static short delta(unsigned short a, unsigned short b)
{
    unsigned short u = (unsigned short)(a - b);
    short s;
    memcpy(&s, &u, 2);
    return s;
}
static float number(BHEntity *entity, unsigned long o)
{
    float f;
    memcpy(&f, (char *)entity + o, 4);
    return f;
}
static void real(BHEntity *entity, unsigned long o, float f)
{
    memcpy((char *)entity + o, &f, 4);
}
static double wrap(float n)
{
    const double pi = 3.141592653589793;
    if (n >= pi)
        return fmod((double)n + pi, 6.283185307179586) - pi;
    if (n < -pi)
        return fmod((double)n - pi, 6.283185307179586) + pi;
    return n;
}
static BHPickupRoom *room(BHRemotePackets *packets, BHWorldGeometry *world)
{
    unsigned long i;
    for (i = 0; i < packets->room_count; ++i)
        if (packets->rooms[i].world == world)
            return packets->rooms + i;
    return 0;
}
static int move(BHRemotePackets *packets, BHEntity *entity, BHWorldGeometry *old, BHWorldGeometry *next)
{
    BHPickupRoom *a = old ? room(packets, old) : 0, *b = next ? room(packets, next) : 0;
    BHEntity *previous = (BHEntity *)word(entity, 0xf8), *following = (BHEntity *)word(entity, 0xf4), *head;
    if (old == next)
        return 1;
    if ((old && !a) || (next && !b))
        return 0;
    if (a && ((previous ? (BHEntity *)word(previous, 0xf4) : a->actors) != entity ||
              (following && (BHEntity *)word(following, 0xf8) != entity)))
        return 0;
    if (a)
    {
        if (previous)
            setword(previous, 0xf4, (unsigned long)following);
        else
            a->actors = following;
        if (following)
            setword(following, 0xf8, (unsigned long)previous);
    }
    if (b)
    {
        head = b->actors;
        setword(entity, 0xf4, (unsigned long)head);
        b->actors = entity;
        setword(entity, 0xf8, 0);
        if (head)
            setword(head, 0xf8, (unsigned long)entity);
    }
    return 1;
}
static void clamp(BHEntity *entity, unsigned long offset, double squared, double limit)
{
    float x = number(entity, offset), y = number(entity, offset + 4), z = number(entity, offset + 8);
    double length = ((double)z * z + (double)y * y) + (double)x * x;
    float factor;
    if (length > squared)
    {
        factor = rf(limit / sqrt(length));
        real(entity, offset, rf((double)number(entity, offset) * factor));
        real(entity, offset + 4, rf((double)number(entity, offset + 4) * factor));
        real(entity, offset + 8, rf((double)number(entity, offset + 8) * factor));
    }
}
int __cdecl bh_remote_position_packet(BHRemotePackets *packets, void *p, unsigned long size, unsigned long capacity)
{
    BHEntity *entity;
    unsigned short slot, sender, newest, middle, oldest;
    unsigned long bank, i, o;
    BHWorldGeometry *before, *world;
    BHVec3 position;
    unsigned char *bytes, flags;
    short dt, previous_dt, age;
    float velocity[3], new_ang[3], old_ang[3], oldp, midp, latestp, angle, decay_a, decay_v;
    double accel, decay;
    int decoded, first_pose;
    if (!packets || !p)
    {
        if (packets)
            packets->error = 1;
        return 0;
    }
    packets->error = 0;
    ++packets->pending;
    if (!bh_packet_receive(&packets->reader, p, size, capacity, &packets->framing))
    {
        --packets->pending;
        ++packets->bad;
        packets->error = 2;
        return 0;
    }
    if (!packets->remote || !packets->local_slot || !packets->simulation_tick || !packets->navigation ||
        (packets->room_count && !packets->rooms) || !bh_math_environment_supported())
    {
        packets->error = 1;
        return 0;
    }
    if (packets->reader.size - packets->reader.cursor < 2)
    {
        packets->error = 3;
        return 0;
    }
    memcpy(&slot, packets->reader.data + packets->reader.cursor, 2);
    packets->reader.cursor += 2;
    entity = packets->local;
    if ((long)slot != *packets->local_slot)
    {
        if (slot > 7)
        {
            --packets->pending;
            ++packets->bad;
            packets->error = 4;
            return 0;
        }
        entity = packets->remote[slot];
        if (!entity)
            return 1;
    }
    if (!entity)
    {
        packets->error = 1;
        return 0;
    }
    memcpy(&sender, packets->reader.data + 2, 2);
    before = (BHWorldGeometry *)word(entity, 0x198);
    first_pose = before == NULL;
    oldest = half(entity, 0x13c);
    middle = half(entity, 0x15c);
    newest = half(entity, 0x17c);
    if (!first_pose && delta(sender, oldest) < 0)
    {
        --packets->pending;
        return 1;
    }
    if (first_pose)
        bank = 0x17c;
    else if (delta(sender, middle) < 0)
        bank = 0x13c;
    else if (sender != middle && delta(sender, newest) >= 0)
    {
        if (sender != newest)
        {
            memcpy((char *)entity + 0x13c, (char *)entity + 0x15c, 32);
            memcpy((char *)entity + 0x15c, (char *)entity + 0x17c, 32);
        }
        bank = 0x17c;
    }
    else
    {
        if (sender != middle)
            memcpy((char *)entity + 0x13c, (char *)entity + 0x15c, 32);
        bank = 0x15c;
    }
    sethalf(entity, bank, sender);
    world = before;
    decoded = bh_packet_read_position(&packets->reader, packets->navigation, &world, &position);
    if (decoded != 1)
    {
        if (!decoded)
        {
            --packets->pending;
            ++packets->bad;
        }
        packets->error = decoded < 0 ? 3 : 4;
        return 0;
    }
    setword(entity, bank + 28, (unsigned long)world);
    memcpy((char *)entity + bank + 4, &position, 12);
    if (bank == 0x17c && !move(packets, entity, before, world))
    {
        packets->error = 5;
        return 0;
    }
    if (packets->reader.cursor > packets->reader.size || packets->reader.size - packets->reader.cursor < 4)
    {
        packets->error = 3;
        return 0;
    }
    bytes = packets->reader.data + packets->reader.cursor;
    for (i = 0; i < 3; ++i)
    {
        signed char v;
        memcpy(&v, bytes + i, 1);
        real(entity, bank + 16 + 4 * i, rf((double)v * .02454369328916073));
    }
    flags = bytes[3];
    packets->reader.cursor += 4;
    if (bank == 0x17c)
        sethalf(entity, 0x11a, flags);
    if (first_pose)
    {
        memcpy((char *)entity + 0x13c, (char *)entity + 0x17c, 32);
        memcpy((char *)entity + 0x15c, (char *)entity + 0x17c, 32);
        sethalf(entity, 0x13c, (unsigned short)(sender - 2));
        sethalf(entity, 0x15c, (unsigned short)(sender - 1));
    }
    dt = delta(half(entity, 0x17c), half(entity, 0x15c));
    previous_dt = delta(half(entity, 0x15c), half(entity, 0x13c));
    if (!dt)
        dt = 1;
    if (!previous_dt)
        previous_dt = 1;
    for (i = 0; i < 3; ++i)
    {
        oldp = number(entity, 0x140 + 4 * i);
        midp = number(entity, 0x160 + 4 * i);
        latestp = number(entity, 0x180 + 4 * i);
        velocity[i] = rf(((double)latestp - midp) / dt);
        accel = ((double)velocity[i] - ((double)midp - oldp) / previous_dt) / dt;
        real(entity, 0x48 + 4 * i, rf(((double)velocity[i] + number(entity, 0x48 + 4 * i)) * .5));
        real(entity, 0x60 + 4 * i, rf((accel + number(entity, 0x60 + 4 * i)) * .5));
        angle = rf((double)number(entity, 0x18c + 4 * i) - number(entity, 0x16c + 4 * i));
        new_ang[i] = rf(wrap(angle) / dt);
        angle = rf((double)number(entity, 0x16c + 4 * i) - number(entity, 0x14c + 4 * i));
        old_ang[i] = rf(wrap(angle) / previous_dt);
    }
    for (i = 0; i < 3; ++i)
    {
        real(entity, 0x54 + 4 * i, rf(((double)new_ang[i] + number(entity, 0x54 + 4 * i)) * .5));
        real(entity, 0x6c + 4 * i, rf((((double)new_ang[i] - old_ang[i]) / dt + number(entity, 0x6c + 4 * i)) * .5));
    }
    clamp(entity, 0x48, 100, 10);
    clamp(entity, 0x60, 4, 2);
    age = delta((unsigned short)*packets->simulation_tick, half(entity, 0x17c));
    if (age < 0)
        age = delta(0, (unsigned short)age);
    if (age)
    {
        decay_a = rf(pow(.8, (double)age));
        decay = pow(.965, (double)age);
        decay_v = rf(decay);
        if (!_finite(decay_a) || !_finite(decay_v))
        {
            packets->error = 6;
            return 0;
        }
        for (i = 0; i < 6; ++i)
            real(entity, 0x60 + 4 * i, rf((double)decay_a * number(entity, 0x60 + 4 * i)));
        for (i = 0; i < 6; ++i)
        {
            o = 0x48 + 4 * i;
            real(entity, o, rf((i ? decay_v : decay) * number(entity, o)));
        }
    }
    --packets->pending;
    ++packets->processed;
    return 1;
}
