#include "bh_remote_query.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <float.h>
static BHEntity *link(const BHEntity *entity, unsigned long off)
{
    BHEntity *p;
    memcpy(&p, (const char *)entity + off, 4);
    return p;
}
static void set_link(BHEntity *entity, unsigned long off, BHEntity *p)
{
    memcpy((char *)entity + off, &p, 4);
}
static int fail(BHRemoteQuery *query)
{
    if (query)
        query->error = 1;
    return 0;
}
static int retire(BHRemoteQuery *query, BHEntity *entity)
{
    BHWorldGeometry *world;
    BHPickupRoom *room = NULL;
    BHEntity *rp, *rn, *gp, *gn, *at;
    unsigned long i, n = 0, voice;
    short slot;
    if (!query->pool || !query->head || !query->count || !query->remote || !*query->count)
        return fail(query);
    for (at = query->pool->active; at && at != entity && n++ < query->pool->capacity; at = at->next)
    {
    }
    if (at != entity)
        return fail(query);
    memcpy(&slot, (char *)&entity->remote.slot, 2);
    if (slot < 0 || slot >= 8)
        return fail(query);
    world = entity->remote.newest.world;
    rp = entity->actor.room_previous;
    rn = entity->actor.room_next;
    gp = entity->actor.global_previous;
    gn = entity->actor.global_next;
    if (world)
    {
        for (i = 0; i < query->room_count; i++)
            if (query->rooms[i].world == world)
            {
                room = query->rooms + i;
                break;
            }
        if (!room || (rp ? rp->actor.room_next : room->actors) != entity || (rn && rn->actor.room_previous != entity))
            return fail(query);
    }
    if ((gp ? gp->actor.global_next : *query->head) != entity || (gn && gn->actor.global_previous != entity))
        return fail(query);
    memcpy(&voice, (char *)&entity->remote.engine_sound, 4);
    if (voice != BH_SOUND_INVALID && !query->sound)
        return fail(query);
    if (voice != BH_SOUND_INVALID)
    {
        bh_sound_stop(query->sound, voice);
        voice = BH_SOUND_INVALID;
        memcpy((char *)&entity->remote.engine_sound, &voice, 4);
    }
    if (world)
    {
        if (rp)
            (rp->actor.room_next = rn);
        else
            room->actors = rn;
        if (rn)
            (rn->actor.room_previous = rp);
    }
    if (gp)
        (gp->actor.global_next = gn);
    else
        *query->head = gn;
    if (gn)
        (gn->actor.global_previous = gp);
    --*query->count;
    query->remote[slot] = NULL;
    return bh_entity_release(query->pool, entity);
}
int bh_remote_query(BHRemoteQuery *query, BHEntity *entity, BHRemoteQueryPacket *p)
{
    BHVec3 position;
    BHWorldGeometry *world;
    BHEntity *target;
    BHPacketWriter writer;
    unsigned long i, j, recipient;
    unsigned short id;
    unsigned char severity;
    float value;
    if (!query || !entity || !p)
        return fail(query);
    query->error = 0;
    switch (p->words[0])
    {
    case 0:
        if (!query->transport || !query->remote)
            return fail(query);
        if (!bh_remote_predict_position(entity, query->tick, &position))
            return fail(query);
        world = entity->remote.newest.world;
        if (!world)
            return 1;
        for (i = 0; i < query->world_count; i++)
            if (query->worlds[i].world == world)
                break;
        if (i == query->world_count)
            return fail(query);
        memcpy(&recipient, (char *)&entity->remote.player_id, 4);
        target = p->damage.attacker;
        value = p->damage.severity;
        if (!_finite(value) || value < -1000000)
            return fail(query);
        if (value > 1.99)
            value = 1.99f;
        if (!bh_packet_begin(&writer, query->packet, sizeof(query->packet), 7, query->tick) ||
            !bh_packet_write_position(&writer, world, query->worlds[i].index, &position))
            return fail(query);
        if (target == query->local)
        {
            id = (unsigned short)query->local_id;
            if (!bh_packet_append(&writer, &id, 2))
                return fail(query);
        }
        else
            for (j = 0; j < 8; j++)
                if (query->remote[j] == target)
                {
                    /* An absent target never dereferences NULL. Valid original callers
               supply live local/remote actors; NULL is an unknown identity. */
                    if (target)
                    {
                        memcpy(&id, (char *)&target->remote.slot, 2);
                        if (!bh_packet_append(&writer, &id, 2))
                            return fail(query);
                    }
                    break;
                }
        severity = (unsigned char)(long)((double)value * 128.0);
        if (!bh_packet_append(&writer, &severity, 1) || !bh_packet_finish(&writer, &query->wire_size))
            return fail(query);
        query->send_result = bh_network_send(query->transport, recipient, query->packet, query->wire_size, 0);
        ++query->sends;
        return 1;
    case 3:
    case 16:
        if (!bh_remote_predict_position(entity, query->tick, &position))
            return fail(query);
        memcpy(p->floats + 1, &position, 12);
        if (p->words[0] == 16)
            for (j = 0; j < 2; j++)
                for (i = 0; i < 3; i++)
                {
                    memcpy(&value, (char *)&entity->velocity + j * 0x18 + i * 4, 4);
                    bh_round_float(p->floats + 4 + j * 3 + i, (double)value * (j ? 900.0 : 30.0));
                }
        return 1;
    case 17:
        for (i = 0; i < 9; i++)
            if (link(entity, 0x1a4 + i * 12) == p->removed.actor)
                set_link(entity, 0x1a4 + i * 12, NULL);
        return 1;
    case 20:
        return retire(query, entity);
    default:
        return 1;
    }
}
