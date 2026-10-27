#include "bh_callback_helpers.h"
#include <string.h>
static void removed(BHEntity *entity, void *packet, unsigned long offset, int extra)
{
    unsigned long event, target, current, zero = 0;
    if (!entity || !packet)
        return;
    memcpy(&event, packet, 4);
    if (event != 17)
        return;
    memcpy(&target, (unsigned char *)packet + 4, 4);
    memcpy(&current, (unsigned char *)entity + offset, 4);
    if (target != current)
        return;
    memcpy((unsigned char *)entity + offset, &zero, 4);
    if (extra)
        memcpy((unsigned char *)entity + 0xfc, &zero, 4);
}
void __cdecl bh_actor_target_removed(BHEntity *entity, void *packet)
{
    removed(entity, packet, 0x1b4, 0);
}
void __cdecl bh_effect_target_removed(BHEntity *entity, void *packet)
{
    removed(entity, packet, 0xf4, 1);
}
void __cdecl bh_pickup_target_removed(BHEntity *entity, void *packet)
{
    removed(entity, packet, 0x104, 0);
}
void __cdecl bh_marker_query(BHEntity *entity, void *packet)
{
    unsigned long event, zero = 0;
    if (!entity || !packet)
        return;
    memcpy(&event, packet, 4);
    if (event == 20)
        memcpy((unsigned char *)entity + 0x108, &zero, 4);
    else if (event == 3)
        memcpy((unsigned char *)packet + 4, &entity->position, 12);
}
void __cdecl bh_vec3_interpolate(float factor, BHVec3 *out, const BHVec3 *a, const BHVec3 *b)
{
    double x, y, z;
    if (!out || !a || !b)
        return;
    x = ((double)b->x - a->x) * factor + a->x;
    y = ((double)b->y - a->y) * factor;
    z = ((double)b->z - a->z) * factor;
    out->x = (float)x;
    out->y = (float)(y + a->y);
    out->z = (float)(z + a->z);
}
int __cdecl bh_packet_read_vector(BHPacketReader *reader, BHVec3 *out, float scale)
{
    double factor;
    unsigned long i;
    float *component;
    if (!reader || !out || !reader->data)
        return 0;
    if (reader->cursor > reader->size || reader->size - reader->cursor < 3)
    {
        reader->error = BH_PACKET_TRUNCATED;
        return 0;
    }
    factor = (double)scale * (1.0 / 128.0);
    component = &out->x;
    for (i = 0; i < 3; ++i)
    {
        int value = (int)(signed char)reader->data[reader->cursor++];
        component[i] = (float)(value * factor);
    }
    return 1;
}
