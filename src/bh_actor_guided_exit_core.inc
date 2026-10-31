#include "bh_actor_exit_image.h"
#include "bh_actor_portal_image.h"
#include "bh_math.h"

static const BHVec3 bh_actor_guided_up = {0, 1, 0};
/* VC5 may reuse an extended value even across a volatile float write/read.
   Moving its integer representation enforces the original binary32 reload
   without an external rounding call or inline assembly. */
static __inline float guided_float_spill(double value)
{
    union {
        float value;
        volatile unsigned long bits;
    } stored, reloaded;
    stored.value = (float)value;
    reloaded.bits = stored.bits;
    return reloaded.value;
}

/* Complete 00434650. The caller supplies the same borrowed portal/count/flag
   image used by the exit selector. Floating locals preserve the owner's
   retained x87 intermediates, including the vertical value times its spill. */
int __cdecl bh_actor_guided_exit_core(const BHEntity *actor, void *room_image, int incoming)
{
    const BHActorExitModelRecord *model = bh_actor_exit_counts + *(int *)((char *)room_image + 0xf8);
    char *geometry = (char *)room_image + 0xe8;
    float best = 9.0e37f;
    BHVec3 anchor, normal, side, point;
    int i, choice;
    BHPortal *portal;
    bh_actor_portal_center_core(geometry, incoming, (float *)&anchor);
    bh_actor_portal_normal_core(geometry, incoming, (float *)&normal);
    bh_vec3_cross(&bh_actor_guided_up, &normal, &side);
    choice = incoming;
    for (i = 0, portal = (BHPortal *)(geometry + 0xe4); i < model->portal_count; ++i, ++portal)
    {
        unsigned long flags;
        double vertical, lateral;
        float stored_vertical, score;
        if (i == incoming || !portal->neighbor)
            continue;
        flags = *(unsigned long *)((char *)portal->neighbor + 0x114);
        if ((flags & 0x10) && *(int *)((char *)actor + 0xfc) > 0)
            continue;
        if ((flags & 0x20) && (*(int *)((char *)actor + 0xfc) == 0 || *(int *)((char *)actor + 0xfc) == 6))
            continue;
        if (portal->direction != (((BHPortal *)(geometry + 0xe4))[incoming].direction ^ 4UL))
            continue;
        bh_actor_portal_center_core(geometry, i, (float *)&point);
        point.x -= anchor.x;
        point.y -= anchor.y;
        point.z = guided_float_spill((double)point.z - anchor.z);
        vertical = (point.x * bh_actor_guided_up.x + point.y * bh_actor_guided_up.y) + point.z * bh_actor_guided_up.z;
        lateral = (point.x * side.x + point.y * side.y) + point.z * side.z;
        stored_vertical = guided_float_spill(vertical);
        score = vertical * stored_vertical + lateral * lateral;
        if (score < best)
        {
            best = score;
            choice = i;
        }
    }
    return choice;
}
