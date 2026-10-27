#include "bh_remote_query.h"
#include "bh_round.h"
#include <string.h>
#include <float.h>
static BHEntity *link(const BHEntity *e, unsigned long off)
{
    BHEntity *p;
    memcpy(&p, (const char *)e + off, 4);
    return p;
}
static void set_link(BHEntity *e, unsigned long off, BHEntity *p)
{
    memcpy((char *)e + off, &p, 4);
}
static int fail(BHRemoteQuery *c)
{
    if (c)
        c->error = 1;
    return 0;
}
static int retire(BHRemoteQuery *c, BHEntity *e)
{
    BHWorldGeometry *world;
    BHPickupRoom *room = NULL;
    BHEntity *rp, *rn, *gp, *gn, *at;
    unsigned long i, n = 0;
    short slot;
    if (!c->pool || !c->head || !c->count || !c->remote || !*c->count)
        return fail(c);
    for (at = c->pool->active; at && at != e && n++ < c->pool->capacity; at = at->next)
    {
    }
    if (at != e)
        return fail(c);
    memcpy(&slot, (char *)e + 0x114, 2);
    if (slot < 0 || slot >= 8)
        return fail(c);
    memcpy(&world, (char *)e + 0x198, 4);
    rp = link(e, 0xf8);
    rn = link(e, 0xf4);
    gp = link(e, 0xec);
    gn = link(e, 0xf0);
    if (world)
    {
        for (i = 0; i < c->room_count; i++)
            if (c->rooms[i].world == world)
            {
                room = c->rooms + i;
                break;
            }
        if (!room || (rp ? link(rp, 0xf4) : room->actors) != e || (rn && link(rn, 0xf8) != e))
            return fail(c);
    }
    if ((gp ? link(gp, 0xf0) : *c->head) != e || (gn && link(gn, 0xec) != e))
        return fail(c);
    if (world)
    {
        if (rp)
            set_link(rp, 0xf4, rn);
        else
            room->actors = rn;
        if (rn)
            set_link(rn, 0xf8, rp);
    }
    if (gp)
        set_link(gp, 0xf0, gn);
    else
        *c->head = gn;
    if (gn)
        set_link(gn, 0xec, gp);
    --*c->count;
    c->remote[slot] = NULL;
    return bh_entity_release(c->pool, e);
}
int __cdecl bh_remote_query(BHRemoteQuery *c, BHEntity *e, BHRemoteQueryPacket *p)
{
    BHVec3 position;
    BHWorldGeometry *world;
    BHEntity *target;
    BHPacketWriter writer;
    unsigned long i, j, recipient;
    unsigned short id;
    unsigned char severity;
    float value;
    if (!c || !e || !p)
        return fail(c);
    c->error = 0;
    switch (p->words[0])
    {
    case 0:
        if (!c->transport || !c->remote)
            return fail(c);
        if (!bh_remote_predict_position(e, c->tick, &position))
            return fail(c);
        memcpy(&world, (char *)e + 0x198, 4);
        if (!world)
            return 1;
        for (i = 0; i < c->world_count; i++)
            if (c->worlds[i].world == world)
                break;
        if (i == c->world_count)
            return fail(c);
        memcpy(&recipient, (char *)e + 0x138, 4);
        target = p->damage.attacker;
        value = p->damage.severity;
        if (!_finite(value) || value < -1000000)
            return fail(c);
        if (value > 1.99)
            value = 1.99f;
        if (!bh_packet_begin(&writer, c->packet, sizeof(c->packet), 7, c->tick) ||
            !bh_packet_write_position(&writer, world, c->worlds[i].index, &position))
            return fail(c);
        if (target == c->local)
        {
            id = (unsigned short)c->local_id;
            if (!bh_packet_append(&writer, &id, 2))
                return fail(c);
        }
        else
            for (j = 0; j < 8; j++)
                if (c->remote[j] == target)
                {
                    /* An absent target never dereferences NULL. Valid original callers
               supply live local/remote actors; NULL is an unknown identity. */
                    if (target)
                    {
                        memcpy(&id, (char *)target + 0x114, 2);
                        if (!bh_packet_append(&writer, &id, 2))
                            return fail(c);
                    }
                    break;
                }
        severity = (unsigned char)(long)((double)value * 128.0);
        if (!bh_packet_append(&writer, &severity, 1) || !bh_packet_finish(&writer, &c->wire_size))
            return fail(c);
        c->send_result = bh_network_send(c->transport, recipient, c->packet, c->wire_size, 0);
        ++c->sends;
        return 1;
    case 3:
    case 16:
        if (!bh_remote_predict_position(e, c->tick, &position))
            return fail(c);
        memcpy(p->floats + 1, &position, 12);
        if (p->words[0] == 16)
            for (j = 0; j < 2; j++)
                for (i = 0; i < 3; i++)
                {
                    memcpy(&value, (char *)e + 0x48 + j * 0x18 + i * 4, 4);
                    bh_round_float(p->floats + 4 + j * 3 + i, (double)value * (j ? 900.0 : 30.0));
                }
        return 1;
    case 17:
        for (i = 0; i < 9; i++)
            if (link(e, 0x1a4 + i * 12) == p->removed.actor)
                set_link(e, 0x1a4 + i * 12, NULL);
        return 1;
    case 20:
        return retire(c, e);
    default:
        return 1;
    }
}
