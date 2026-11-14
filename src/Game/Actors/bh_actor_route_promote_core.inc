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
    union {
        float value;
        volatile unsigned long bits;
    } written, read;
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

int bh_actor_route_promote_core(BHEntity *actor, const BHActorRoutePromotionBinding *binding, long incoming, long outgoing)
{
    const BHPortal *in = binding->incoming, *out = binding->outgoing;
    BHWorldGeometry *world = binding->world;
    BHEntity *previous, *next;
    BHVec3 center, delta;
    float u, v, cu, cv, plane, first_length, width, reverse_z;
    double length, duration, test_width;
    unsigned short ticks;

    previous = actor->actor.room_previous;
    if (previous)
        previous->actor.room_next = actor->actor.room_next;
    else
        binding->old_room->actors = actor->actor.room_next;
    next = actor->actor.room_next;
    if (next)
        next->actor.room_previous = actor->actor.room_previous;
    actor->actor.current = actor->actor.next;
    actor->world = actor->actor.current;
    actor->actor.route_start = actor->actor.route_end;
    actor->actor.room_next = binding->room->actors;
    binding->room->actors = actor;
    actor->actor.room_previous = NULL;
    next = actor->actor.room_next;
    if (next)
        next->actor.room_previous = actor;

    u = promote_stored((1.0 - (double)actor->actor.u) * out->scratch[1] + (double)actor->actor.u * out->scratch[0]);
    v = promote_stored((1.0 - (double)actor->actor.v) * out->scratch[3] + (double)actor->actor.v * out->scratch[2]);
    cu = promote_stored(((double)out->scratch[1] + out->scratch[0]) * .5);
    cv = promote_stored(((double)out->scratch[3] + out->scratch[2]) * .5);
    plane = promote_stored((double)out->plane);
    actor->actor.next = (BHWorldGeometry *)out->neighbor;
    switch (out->direction & 3UL)
    {
    case 0:
        actor->actor.portal_center.y = cu;
        actor->actor.portal_center.z = cv;
        actor->actor.route_end.y = u;
        actor->actor.portal_center.x = plane;
        actor->actor.route_end.x = plane;
        actor->actor.route_end.z = v;
        break;
    case 1:
        actor->actor.portal_center.x = cu;
        actor->actor.portal_center.z = cv;
        actor->actor.route_end.x = u;
        actor->actor.portal_center.y = plane;
        actor->actor.route_end.y = plane;
        actor->actor.route_end.z = v;
        break;
    default:
        actor->actor.portal_center.x = cu;
        actor->actor.portal_center.y = cv;
        actor->actor.route_end.x = u;
        actor->actor.portal_center.z = plane;
        actor->actor.route_end.y = v;
        actor->actor.route_end.z = plane;
        break;
    }
    if (out->direction == (in->direction ^ 4UL) && (world->query_flags & 8))
    {
        actor->actor.route_end = actor->actor.route_start;
        ((float *)&actor->actor.route_end)[(out->direction & 3UL)] = plane;
        promote_difference(&delta, &actor->actor.route_end, &actor->actor.route_start);
        actor->actor.route_first = actor->actor.route_end;
        actor->actor.route_second = actor->actor.route_start;
        length = bh_vec3_snap_axis(&delta);
        length += length;
    }
    else
    {
        center.x = promote_stored(((double)world->minimum.x + world->maximum.x) * .5);
        center.z = promote_stored(((double)world->minimum.z + world->maximum.z) * .5);
        center.y = promote_stored(((double)actor->actor.route_end.y + actor->actor.route_start.y) * .5);
        promote_difference(&delta, &center, &actor->actor.route_start);
        first_length = promote_stored(bh_vec3_snap_axis(&delta));
        promote_tangent(&actor->actor.route_first, &actor->actor.route_start, &delta);
        promote_difference(&delta, &center, &actor->actor.route_end);
        length = bh_vec3_snap_axis(&delta) + first_length;
        promote_tangent(&actor->actor.route_second, &actor->actor.route_end, &delta);
        if (incoming == outgoing)
        {
            reverse_z = -delta.z;
            actor->actor.route_first.x = promote_stored((double)reverse_z + actor->actor.route_first.x);
            actor->actor.route_first.y = promote_stored((double)actor->actor.route_first.y - 2);
            actor->actor.route_first.z = promote_stored((double)actor->actor.route_first.z + delta.x);
            actor->actor.route_second.x = promote_stored((double)actor->actor.route_second.x - reverse_z);
            actor->actor.route_second.y = promote_stored((double)actor->actor.route_second.y - (-2));
            actor->actor.route_second.z = promote_stored((double)actor->actor.route_second.z - delta.x);
        }
    }
    actor->actor.phase = 0;
    duration = promote_stored(length / ((double)actor->actor.speed * actor->actor.speed_scale));
    actor->actor.duration = (float)duration;
    if (!duration_ticks(duration, &ticks))
        return 0;
    actor->actor.duration_ticks = ticks;
    actor->actor.tick = bh_actor_route_now(binding->context);
    test_width = (double)out->scratch[1] - out->scratch[0];
    width = promote_stored(test_width);
    if (test_width < 30)
    {
        test_width = (double)out->scratch[3] - out->scratch[2];
        if (test_width > width)
            width = promote_stored(test_width);
        if (width < 30)
        {
            test_width = (double)in->scratch[1] - in->scratch[0];
            if (test_width > width)
                width = promote_stored(test_width);
            if (width < 30)
            {
                test_width = (double)in->scratch[3] - in->scratch[2];
                if (test_width > width)
                    width = promote_stored(test_width);
                if (width < 30)
                {
                    duration = (double)actor->actor.duration / (((double)width - (-10.0)) * .025);
                    actor->actor.duration = promote_stored(duration);
                    if (!duration_ticks(duration, &ticks))
                        return 0;
                    actor->actor.duration_ticks = ticks;
                }
            }
        }
    }
    return _finite(actor->actor.duration) != 0;
}
