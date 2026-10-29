/* Complete 00434f40 candidate. Original room globals/layout resolve through
   these real, validated source owners; no original image or guessed globals. */
typedef struct BHActorRoutePromotionBinding
{
    BHActorRouteContext *context;
    BHPickupRoom *old_room, *room;
    BHWorldGeometry *world;
    const BHPortal *incoming, *outgoing;
} BHActorRoutePromotionBinding;

static __inline float promote_stored(double value)
{
    union { float value; volatile unsigned long bits; } written, read;
    written.value = (float)value;
    read.bits = written.bits;
    return read.value;
}
static __inline void promote_difference(BHVec3 *out, const BHVec3 *left, const BHVec3 *right)
{
    out->x = promote_stored((double)left->x - right->x);
    out->y = promote_stored((double)left->y - right->y);
    out->z = promote_stored((double)left->z - right->z);
}
static __inline void promote_tangent(BHVec3 *out, const BHVec3 *point, const BHVec3 *delta)
{
    out->x = promote_stored((double)point->x - (double)delta->x * (-1.3));
    out->y = promote_stored((double)point->y - (double)delta->y * (-1.3));
    out->z = promote_stored((double)point->z - (double)delta->z * (-1.3));
}

#define PROMOTE_FLOAT(offset) (*(float *)((char *)a + (offset)))
#define PROMOTE_VEC(offset) (*(BHVec3 *)((char *)a + (offset)))
#define PROMOTE_WORD(offset) (*(unsigned short *)((char *)a + (offset)))
#define PROMOTE_WORLD(offset) (*(BHWorldGeometry **)((char *)a + (offset)))
#define PROMOTE_LINK(actor, offset) (*(BHEntity **)((char *)(actor) + (offset)))

int __cdecl bh_actor_route_promote_core(BHEntity *a, const BHActorRoutePromotionBinding *binding,
                                       long incoming, long outgoing)
{
    const BHPortal *in = binding->incoming, *out = binding->outgoing;
    BHWorldGeometry *world = binding->world;
    BHEntity *previous, *next;
    BHVec3 center, delta;
    float u, v, cu, cv, plane, first_length, width, reverse_z;
    double length, duration, test_width;
    unsigned short ticks;

    previous = PROMOTE_LINK(a, 0xf8);
    if (previous)
        PROMOTE_LINK(previous, 0xf4) = PROMOTE_LINK(a, 0xf4);
    else
        binding->old_room->actors = PROMOTE_LINK(a, 0xf4);
    next = PROMOTE_LINK(a, 0xf4);
    if (next)
        PROMOTE_LINK(next, 0xf8) = PROMOTE_LINK(a, 0xf8);
    PROMOTE_WORLD(0x1a4) = PROMOTE_WORLD(0x1a8);
    PROMOTE_WORLD(0xc4) = PROMOTE_WORLD(0x1a4);
    PROMOTE_VEC(0x120) = PROMOTE_VEC(0x12c);
    PROMOTE_LINK(a, 0xf4) = binding->room->actors;
    binding->room->actors = a;
    PROMOTE_LINK(a, 0xf8) = NULL;
    next = PROMOTE_LINK(a, 0xf4);
    if (next)
        PROMOTE_LINK(next, 0xf8) = a;

    u = promote_stored((1.0 - (double)PROMOTE_FLOAT(0x18c)) * out->scratch[1] +
               (double)PROMOTE_FLOAT(0x18c) * out->scratch[0]);
    v = promote_stored((1.0 - (double)PROMOTE_FLOAT(0x190)) * out->scratch[3] +
               (double)PROMOTE_FLOAT(0x190) * out->scratch[2]);
    cu = promote_stored(((double)out->scratch[1] + out->scratch[0]) * .5);
    cv = promote_stored(((double)out->scratch[3] + out->scratch[2]) * .5);
    plane = promote_stored((double)out->plane);
    PROMOTE_WORLD(0x1a8) = (BHWorldGeometry *)out->neighbor;
    switch (out->direction & 3UL)
    {
    case 0:
        PROMOTE_FLOAT(0x13c) = cu; PROMOTE_FLOAT(0x140) = cv;
        PROMOTE_FLOAT(0x130) = u; PROMOTE_FLOAT(0x138) = plane;
        PROMOTE_FLOAT(0x12c) = plane; PROMOTE_FLOAT(0x134) = v;
        break;
    case 1:
        PROMOTE_FLOAT(0x138) = cu; PROMOTE_FLOAT(0x140) = cv;
        PROMOTE_FLOAT(0x12c) = u; PROMOTE_FLOAT(0x13c) = plane;
        PROMOTE_FLOAT(0x130) = plane; PROMOTE_FLOAT(0x134) = v;
        break;
    default:
        PROMOTE_FLOAT(0x138) = cu; PROMOTE_FLOAT(0x13c) = cv;
        PROMOTE_FLOAT(0x12c) = u; PROMOTE_FLOAT(0x140) = plane;
        PROMOTE_FLOAT(0x130) = v; PROMOTE_FLOAT(0x134) = plane;
        break;
    }
    if (out->direction == (in->direction ^ 4UL) && (world->query_flags & 8))
    {
        PROMOTE_VEC(0x12c) = PROMOTE_VEC(0x120);
        PROMOTE_FLOAT(0x12c + 4 * (out->direction & 3UL)) = plane;
        promote_difference(&delta, &PROMOTE_VEC(0x12c), &PROMOTE_VEC(0x120));
        PROMOTE_VEC(0x144) = PROMOTE_VEC(0x12c);
        PROMOTE_VEC(0x150) = PROMOTE_VEC(0x120);
        length = bh_vec3_snap_axis(&delta);
        length += length;
    }
    else
    {
        center.x = promote_stored(((double)world->minimum.x + world->maximum.x) * .5);
        center.z = promote_stored(((double)world->minimum.z + world->maximum.z) * .5);
        center.y = promote_stored(((double)PROMOTE_FLOAT(0x130) + PROMOTE_FLOAT(0x124)) * .5);
        promote_difference(&delta, &center, &PROMOTE_VEC(0x120));
        first_length = promote_stored(bh_vec3_snap_axis(&delta));
        promote_tangent(&PROMOTE_VEC(0x144), &PROMOTE_VEC(0x120), &delta);
        promote_difference(&delta, &center, &PROMOTE_VEC(0x12c));
        length = bh_vec3_snap_axis(&delta) + first_length;
        promote_tangent(&PROMOTE_VEC(0x150), &PROMOTE_VEC(0x12c), &delta);
        if (incoming == outgoing)
        {
            reverse_z = -delta.z;
            PROMOTE_FLOAT(0x144) = promote_stored((double)reverse_z + PROMOTE_FLOAT(0x144));
            PROMOTE_FLOAT(0x148) = promote_stored((double)PROMOTE_FLOAT(0x148) - 2);
            PROMOTE_FLOAT(0x14c) = promote_stored((double)PROMOTE_FLOAT(0x14c) + delta.x);
            PROMOTE_FLOAT(0x150) = promote_stored((double)PROMOTE_FLOAT(0x150) - reverse_z);
            PROMOTE_FLOAT(0x154) = promote_stored((double)PROMOTE_FLOAT(0x154) - (-2));
            PROMOTE_FLOAT(0x158) = promote_stored((double)PROMOTE_FLOAT(0x158) - delta.x);
        }
    }
    PROMOTE_FLOAT(0x180) = 0;
    duration = promote_stored(length / ((double)PROMOTE_FLOAT(0x188) * PROMOTE_FLOAT(0x198)));
    PROMOTE_FLOAT(0x184) = (float)duration;
    if (!duration_ticks(duration, &ticks))
        return 0;
    PROMOTE_WORD(0x1e6) = ticks;
    PROMOTE_WORD(0x1e4) = bh_actor_route_now(binding->context);
    test_width = (double)out->scratch[1] - out->scratch[0];
    width = promote_stored(test_width);
    if (test_width < 30)
    {
        test_width = (double)out->scratch[3] - out->scratch[2];
        if (test_width > width) width = promote_stored(test_width);
        if (width < 30)
        {
            test_width = (double)in->scratch[1] - in->scratch[0];
            if (test_width > width) width = promote_stored(test_width);
            if (width < 30)
            {
                test_width = (double)in->scratch[3] - in->scratch[2];
                if (test_width > width) width = promote_stored(test_width);
                if (width < 30)
                {
                    duration = (double)PROMOTE_FLOAT(0x184) / (((double)width - (-10.0)) * .025);
                    PROMOTE_FLOAT(0x184) = promote_stored(duration);
                    if (!duration_ticks(duration, &ticks)) return 0;
                    PROMOTE_WORD(0x1e6) = ticks;
                }
            }
        }
    }
    return _finite(PROMOTE_FLOAT(0x184)) != 0;
}
#undef PROMOTE_FLOAT
#undef PROMOTE_VEC
#undef PROMOTE_WORD
#undef PROMOTE_WORLD

#undef PROMOTE_LINK
