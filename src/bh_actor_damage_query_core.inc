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
#define DAMAGE_QUERY_FLOAT(offset) (*(float *)((char *)e + (offset)))
#define DAMAGE_QUERY_WORD(offset) (*(unsigned long *)((char *)e + (offset)))
#define DAMAGE_QUERY_SHORT(offset) (*(unsigned short *)((char *)e + (offset)))
#define DAMAGE_QUERY_VEC(offset) ((BHVec3 *)((char *)e + (offset)))
#define DAMAGE_QUERY_LINK(actor, offset) (*(BHEntity **)((char *)(actor) + (offset)))
#define DAMAGE_QUERY_CURVE(phase, destination)                                                                         \
    bh_camera_curve((phase), (destination), DAMAGE_QUERY_VEC(0x120), DAMAGE_QUERY_VEC(0x144), DAMAGE_QUERY_VEC(0x150), \
                    DAMAGE_QUERY_VEC(0x12c))

static BHPickupRoom *damage_query_room(BHActorDamageContext *c, BHEntity *e)
{
    BHWorldGeometry *current = *(BHWorldGeometry **)((char *)e + 0x1a4);
    unsigned long i;
    for (i = 0; i < c->route->room_count; ++i)
        if (c->route->rooms[i].world == current)
            return c->route->rooms + i;
    return NULL;
}

void __cdecl bh_actor_damage_query_core(BHEntity *e, void *packet, BHActorDamageContext *c, unsigned short tick)
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
        previous = DAMAGE_QUERY_LINK(e, 0xf8);
        if (previous)
            DAMAGE_QUERY_LINK(previous, 0xf4) = DAMAGE_QUERY_LINK(e, 0xf4);
        else
            room->actors = DAMAGE_QUERY_LINK(e, 0xf4);
        next = DAMAGE_QUERY_LINK(e, 0xf4);
        if (next)
            DAMAGE_QUERY_LINK(next, 0xf8) = DAMAGE_QUERY_LINK(e, 0xf8);
        bh_sound_stop(c->route->sound, DAMAGE_QUERY_WORD(0x104));
        bh_entity_release(c->pool, e);
        return;
    case 17:
        if (p[1] == DAMAGE_QUERY_WORD(0x1b8))
            DAMAGE_QUERY_WORD(0x1b8) = 0;
        if (p[1] == DAMAGE_QUERY_WORD(0x1ac))
            DAMAGE_QUERY_WORD(0x1ac) = 0;
        return;
    case 13:
        p[5] = 1;
        p[6] = p[3];
        return;
    case 0:
        e->step = bh_actor_damage_tick;
        retained = (double)f[1] + DAMAGE_QUERY_FLOAT(0x194);
        DAMAGE_QUERY_FLOAT(0x194) = damage_query_stored(retained);
        if (retained >= 1)
            DAMAGE_QUERY_WORD(0x1ac) = p[2];
        return;
    case 2:
        e->step = bh_actor_damage_tick;
        DAMAGE_QUERY_FLOAT(0x168) = damage_query_stored((double)f[4] * f[7] + DAMAGE_QUERY_FLOAT(0x168));
        DAMAGE_QUERY_FLOAT(0x16c) = damage_query_stored((double)f[5] * f[7] + DAMAGE_QUERY_FLOAT(0x16c));
        DAMAGE_QUERY_FLOAT(0x170) = damage_query_stored((double)f[6] * f[7] + DAMAGE_QUERY_FLOAT(0x170));
        return;
    case 11:
        retained = (double)DAMAGE_QUERY_FLOAT(0x19c) - .1;
        DAMAGE_QUERY_FLOAT(0x19c) = damage_query_stored(retained);
        if (retained < .15)
            DAMAGE_QUERY_FLOAT(0x19c) = .15f;
        e->step = bh_actor_damage_tick;
        return;
    case 1:
    case 3:
    case 16:
        break;
    default:
        return;
    }
    ratio = (double)(short)(tick - DAMAGE_QUERY_SHORT(0x1e4)) / (short)DAMAGE_QUERY_SHORT(0x1e6);
    t = damage_query_stored(ratio);
    if (p[0] == 3)
    {
        DAMAGE_QUERY_CURVE(t, (BHVec3 *)(f + 1));
        f[1] = damage_query_stored((double)DAMAGE_QUERY_FLOAT(0x15c) + f[1]);
        f[2] = damage_query_stored((double)DAMAGE_QUERY_FLOAT(0x160) + f[2]);
        f[3] = damage_query_stored((double)DAMAGE_QUERY_FLOAT(0x164) + f[3]);
        return;
    }
    if (p[0] == 16)
        DAMAGE_QUERY_CURVE(damage_query_stored(ratio - 1.0 / 30.0), &before);
    DAMAGE_QUERY_CURVE(t, &at);
    if (p[0] == 1)
    {
        dx = ((double)DAMAGE_QUERY_FLOAT(0x15c) + at.x) - f[1];
        dy = ((double)DAMAGE_QUERY_FLOAT(0x160) + at.y) - f[2];
        dz = ((double)DAMAGE_QUERY_FLOAT(0x164) + at.z) - f[3];
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
            *(volatile float *)((char *)e + 0x168) = damage_query_stored(factor * delta.x + DAMAGE_QUERY_FLOAT(0x168));
            *(volatile float *)((char *)e + 0x16c) = damage_query_stored(factor * delta.y + DAMAGE_QUERY_FLOAT(0x16c));
            *(volatile float *)((char *)e + 0x170) = damage_query_stored(factor * delta.z + DAMAGE_QUERY_FLOAT(0x170));
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
    f[1] = damage_query_stored((double)DAMAGE_QUERY_FLOAT(0x15c) + at.x);
    f[2] = damage_query_stored((double)DAMAGE_QUERY_FLOAT(0x160) + at.y);
    f[3] = damage_query_stored((double)DAMAGE_QUERY_FLOAT(0x164) + at.z);
}
#undef DAMAGE_QUERY_CURVE
#undef DAMAGE_QUERY_FLOAT
#undef DAMAGE_QUERY_WORD
#undef DAMAGE_QUERY_SHORT
#undef DAMAGE_QUERY_VEC
#undef DAMAGE_QUERY_LINK
