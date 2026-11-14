#include "bh_remote_position.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <math.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static short delta(unsigned short a, unsigned short b)
{
    unsigned short u = (unsigned short)(a - b);
    short s;
    memcpy(&s, &u, 2);
    return s;
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
    BHEntity *previous = entity->actor.room_previous, *following = entity->actor.room_next, *head;
    if (old == next)
        return 1;
    if ((old && !a) || (next && !b))
        return 0;
    if (a && ((previous ? previous->actor.room_next : a->actors) != entity ||
              (following && following->actor.room_previous != entity)))
        return 0;
    if (a)
    {
        if (previous)
            previous->actor.room_next = following;
        else
            a->actors = following;
        if (following)
            following->actor.room_previous = previous;
    }
    if (b)
    {
        head = b->actors;
        entity->actor.room_next = head;
        b->actors = entity;
        entity->actor.room_previous = 0;
        if (head)
            head->actor.room_previous = entity;
    }
    return 1;
}
static void clamp(BHVec3 *vector, double squared, double limit)
{
    float x = vector->x, y = vector->y, z = vector->z;
    double length = ((double)z * z + (double)y * y) + (double)x * x;
    float factor;
    if (length > squared)
    {
        factor = rf(limit / sqrt(length));
        vector->x = rf((double)vector->x * factor);
        vector->y = rf((double)vector->y * factor);
        vector->z = rf((double)vector->z * factor);
    }
}
int bh_remote_position_packet(BHRemotePackets *packets, void *p, unsigned long size, unsigned long capacity)
{
    BHEntity *entity;
    unsigned short slot, sender, newest, middle, oldest;
    BHRemotePose *pose;
    unsigned long i;
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
    if (bh_packet_receive(&packets->reader, p, size, capacity, &packets->framing) != 1)
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
    if (packets->reader.size != BH_NETWORK_POSE_BYTES &&
        packets->reader.size != BH_NETWORK_POSE_BYTES + BH_PROPULSION_CHANNELS)
    {
        --packets->pending;
        ++packets->bad;
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
    before = entity->remote.newest.world;
    first_pose = before == NULL;
    oldest = entity->remote.oldest.tick;
    middle = entity->remote.middle.tick;
    newest = entity->remote.newest.tick;
    if (!first_pose && delta(sender, oldest) < 0)
    {
        --packets->pending;
        return 1;
    }
    if (first_pose)
        pose = &entity->remote.newest;
    else if (delta(sender, middle) < 0)
        pose = &entity->remote.oldest;
    else if (sender != middle && delta(sender, newest) >= 0)
    {
        if (sender != newest)
        {
            entity->remote.oldest = entity->remote.middle;
            entity->remote.middle = entity->remote.newest;
        }
        pose = &entity->remote.newest;
    }
    else
    {
        if (sender != middle)
            entity->remote.oldest = entity->remote.middle;
        pose = &entity->remote.middle;
    }
    pose->tick = sender;
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
    pose->world = world;
    pose->position = position;
    if (pose == &entity->remote.newest && !move(packets, entity, before, world))
    {
        packets->error = 5;
        return 0;
    }
    bytes = packets->reader.data + packets->reader.cursor;
    for (i = 0; i < 3; ++i)
    {
        signed char v;
        memcpy(&v, bytes + i, 1);
        bh_vec3_set_component(&pose->angles, i, rf((double)v * .02454369328916073));
    }
    flags = bytes[3];
    if (pose == &entity->remote.newest)
    {
        entity->remote.pose_flags = flags;
        if (entity != packets->local)
        {
            unsigned char *propulsion = entity->remote.propulsion;
            if (packets->reader.size > BH_NETWORK_POSE_BYTES)
                memcpy(propulsion, bytes + 4, BH_PROPULSION_CHANNELS);
            else
                memset(propulsion, 0, BH_PROPULSION_CHANNELS);
        }
    }
    packets->reader.cursor = packets->reader.size;
    if (first_pose)
    {
        entity->remote.oldest = entity->remote.newest;
        entity->remote.middle = entity->remote.newest;
        entity->remote.oldest.tick = (unsigned short)(sender - 2);
        entity->remote.middle.tick = (unsigned short)(sender - 1);
    }
    dt = delta(entity->remote.newest.tick, entity->remote.middle.tick);
    previous_dt = delta(entity->remote.middle.tick, entity->remote.oldest.tick);
    if (!dt)
        dt = 1;
    if (!previous_dt)
        previous_dt = 1;
    for (i = 0; i < 3; ++i)
    {
        oldp = bh_vec3_component(&entity->remote.oldest.position, i);
        midp = bh_vec3_component(&entity->remote.middle.position, i);
        latestp = bh_vec3_component(&entity->remote.newest.position, i);
        velocity[i] = rf(((double)latestp - midp) / dt);
        accel = ((double)velocity[i] - ((double)midp - oldp) / previous_dt) / dt;
        bh_vec3_set_component(&entity->velocity, i,
                              rf(((double)velocity[i] + bh_vec3_component(&entity->velocity, i)) * .5));
        bh_vec3_set_component(&entity->acceleration, i,
                              rf((accel + bh_vec3_component(&entity->acceleration, i)) * .5));
        angle = rf((double)bh_vec3_component(&entity->remote.newest.angles, i) -
                   bh_vec3_component(&entity->remote.middle.angles, i));
        new_ang[i] = rf(wrap(angle) / dt);
        angle = rf((double)bh_vec3_component(&entity->remote.middle.angles, i) -
                   bh_vec3_component(&entity->remote.oldest.angles, i));
        old_ang[i] = rf(wrap(angle) / previous_dt);
    }
    for (i = 0; i < 3; ++i)
    {
        bh_vec3_set_component(&entity->angular_velocity, i,
                              rf(((double)new_ang[i] + bh_vec3_component(&entity->angular_velocity, i)) * .5));
        bh_vec3_set_component(&entity->angular_acceleration, i,
                              rf((((double)new_ang[i] - old_ang[i]) / dt +
                                  bh_vec3_component(&entity->angular_acceleration, i)) * .5));
    }
    clamp(&entity->velocity, 100, 10);
    clamp(&entity->acceleration, 4, 2);
    age = delta((unsigned short)*packets->simulation_tick, entity->remote.newest.tick);
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
        for (i = 0; i < 3; ++i)
        {
            bh_vec3_set_component(&entity->acceleration, i,
                                  rf((double)decay_a * bh_vec3_component(&entity->acceleration, i)));
            bh_vec3_set_component(&entity->angular_acceleration, i,
                                  rf((double)decay_a * bh_vec3_component(&entity->angular_acceleration, i)));
        }
        for (i = 0; i < 3; ++i)
        {
            bh_vec3_set_component(&entity->velocity, i,
                                  rf((i ? decay_v : decay) * bh_vec3_component(&entity->velocity, i)));
            bh_vec3_set_component(&entity->angular_velocity, i,
                                  rf((double)decay_v * bh_vec3_component(&entity->angular_velocity, i)));
        }
    }
    --packets->pending;
    ++packets->processed;
    return 1;
}
