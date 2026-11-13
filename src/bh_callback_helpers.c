#include "bh_callback_helpers.h"
#include <string.h>
static void removed(BHEntity *entity, void *packet, void *target_field, float *lifetime)
{
    unsigned long event, target, current, zero = 0;
    if (!entity || !packet)
        return;
    memcpy(&event, packet, 4);
    if (event != 17)
        return;
    memcpy(&target, (unsigned char *)packet + 4, 4);
    memcpy(&current, target_field, 4);
    if (target != current)
        return;
    memcpy(target_field, &zero, 4);
    if (lifetime)
        memcpy(lifetime, &zero, 4);
}
void bh_actor_target_removed(BHEntity *entity, void *packet)
{
    removed(entity, packet, entity ? &entity->explosion.target_identity : NULL, NULL);
}
void bh_effect_target_removed(BHEntity *entity, void *packet)
{
    removed(entity, packet, entity ? &entity->trail.attached : NULL, entity ? &entity->trail.lifetime : NULL);
}
void bh_pickup_target_removed(BHEntity *entity, void *packet)
{
    removed(entity, packet, entity ? &entity->area_projectile.source : NULL, NULL);
}
void bh_marker_query(BHEntity *entity, void *packet)
{
    unsigned long event, zero = 0;
    if (!entity || !packet)
        return;
    memcpy(&event, packet, 4);
    if (event == 20)
        memcpy(&entity->marker.size, &zero, 4);
    else if (event == 3)
        memcpy((unsigned char *)packet + 4, &entity->position, 12);
}
void bh_vec3_interpolate(float factor, BHVec3 *out, const BHVec3 *a, const BHVec3 *b)
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
