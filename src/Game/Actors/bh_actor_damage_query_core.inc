/* Complete 00435e80 source core. The owning TU supplies real lifecycle
   callbacks/owners; source validation and clock binding stay in its wrapper. */
static __inline float damage_query_stored(double value)
{
    union {
        float value;
        volatile unsigned long bits;
    } written, read;
    written.value = (float)value;
    read.bits = written.bits;
    return read.value;
}
#define DAMAGE_QUERY_CURVE(phase, destination)                                                                         \
    bh_camera_curve((phase), (destination), &e->actor.route_start, &e->actor.route_first, &e->actor.route_second,      \
                    &e->actor.route_end)

static BHPickupRoom *damage_query_room(BHActorDamageContext *c, BHEntity *e)
{
    BHWorldGeometry *current = *(BHWorldGeometry **)&e->actor.current;
    unsigned long i;
    return bh_actor_route_room(c->route, current);
}

void bh_actor_damage_query_core(BHEntity *e, void *packet, BHActorDamageContext *c, unsigned short tick)
{
    unsigned long *p = (unsigned long *)packet;
    float *f = (float *)packet;
    BHVec3 before, at, after, delta;
    float t, length;
    double ratio, retained, dx, dy, dz, root, factor;
    double next_x, next_y, next_z, previous_x, previous_y, previous_z;
    BHPickupRoom *room;
    BHEntity *previous, *next;
    switch (p[0])
    {
    case 20:
        e->step = release_tick;
        e->refresh = 0;
        bh_entity_set_query(c->pool, e, 0);
        return;
    case 4:
        room = damage_query_room(c, e);
        if (!room)
        {
            c->error = BH_ACTOR_ROUTE_TOPOLOGY;
            return;
        }
        previous = e->actor.room_previous;
        if (previous)
            previous->actor.room_next = e->actor.room_next;
        else
            room->actors = e->actor.room_next;
        next = e->actor.room_next;
        if (next)
            next->actor.room_previous = e->actor.room_previous;
        bh_sound_stop(c->route->sound, e->actor.sound);
        bh_entity_release(c->pool, e);
        return;
    case 17:
        if (p[1] == e->actor.target_identity)
            e->actor.target_identity = 0;
        if (p[1] == e->actor.damage_source_identity)
            e->actor.damage_source_identity = 0;
        return;
    case 13:
        p[5] = 1;
        p[6] = p[3];
        return;
    case 0:
        e->step = bh_actor_damage_tick;
        retained = (double)f[1] + e->actor.damage;
        e->actor.damage = damage_query_stored(retained);
        if (retained >= 1)
            e->actor.damage_source_identity = p[2];
        return;
    case 2:
        e->step = bh_actor_damage_tick;
        e->actor.offset_velocity.x = damage_query_stored((double)f[4] * f[7] + e->actor.offset_velocity.x);
        e->actor.offset_velocity.y = damage_query_stored((double)f[5] * f[7] + e->actor.offset_velocity.y);
        e->actor.offset_velocity.z = damage_query_stored((double)f[6] * f[7] + e->actor.offset_velocity.z);
        return;
    case 11:
        retained = (double)e->actor.fade - .1;
        e->actor.fade = damage_query_stored(retained);
        if (retained < .15)
            e->actor.fade = .15f;
        e->step = bh_actor_damage_tick;
        return;
    case 1:
    case 3:
    case 16:
        break;
    default:
        return;
    }
    ratio = (double)(short)(tick - e->actor.tick) / (short)e->actor.duration_ticks;
    t = damage_query_stored(ratio);
    if (p[0] == 3)
    {
        DAMAGE_QUERY_CURVE(t, (BHVec3 *)(f + 1));
        f[1] = damage_query_stored((double)e->actor.offset.x + f[1]);
        f[2] = damage_query_stored((double)e->actor.offset.y + f[2]);
        f[3] = damage_query_stored((double)e->actor.offset.z + f[3]);
        return;
    }
    if (p[0] == 16)
        DAMAGE_QUERY_CURVE(damage_query_stored(ratio - 1.0 / 30.0), &before);
    DAMAGE_QUERY_CURVE(t, &at);
    if (p[0] == 1)
    {
        dx = ((double)e->actor.offset.x + at.x) - f[1];
        dy = ((double)e->actor.offset.y + at.y) - f[2];
        dz = ((double)e->actor.offset.z + at.z) - f[3];
        delta.x = damage_query_stored(dx);
        delta.y = damage_query_stored(dy);
        delta.z = damage_query_stored(dz);
        root = sqrt((dx * delta.x + dy * delta.y) + dz * delta.z);
        length = damage_query_stored(root);
        if (root > 0 && length < f[5])
        {
            factor = (((1.0 - (double)length / f[5]) * f[4]) * .003) / length;
            /* 004362ad..004362c9 publishes all three velocity stores before
               scheduling the damage tick; keep VC5 from hoisting that store. */
            *(volatile float *)&e->actor.offset_velocity =
                damage_query_stored(factor * delta.x + e->actor.offset_velocity.x);
            *(volatile float *)(&e->actor.offset_velocity.y) =
                damage_query_stored(factor * delta.y + e->actor.offset_velocity.y);
            *(volatile float *)(&e->actor.offset_velocity.z) =
                damage_query_stored(factor * delta.z + e->actor.offset_velocity.z);
            *(BHEntityStep volatile *)&e->step = bh_actor_damage_tick;
        }
        return;
    }
    DAMAGE_QUERY_CURVE(damage_query_stored((double)t - (-1.0 / 30.0)), &after);
    next_x = ((double)after.x - at.x) * 30;
    next_y = ((double)after.y - at.y) * 30;
    next_z = ((double)after.z - at.z) * 30;
    previous_x = ((double)at.x - before.x) * 30;
    previous_y = ((double)at.y - before.y) * 30;
    previous_z = ((double)at.z - before.z) * 30;
    f[4] = damage_query_stored(next_x);
    f[5] = damage_query_stored(next_y);
    f[6] = damage_query_stored(next_z);
    f[7] = damage_query_stored((previous_x - (double)f[4]) * 30);
    f[8] = damage_query_stored((previous_y - next_y) * 30);
    f[9] = damage_query_stored((previous_z - (double)f[6]) * 30);
    f[1] = damage_query_stored((double)e->actor.offset.x + at.x);
    f[2] = damage_query_stored((double)e->actor.offset.y + at.y);
    f[3] = damage_query_stored((double)e->actor.offset.z + at.z);
}
#undef DAMAGE_QUERY_CURVE
