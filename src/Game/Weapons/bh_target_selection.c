#include "bh_target_selection.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static_assert(offsetof(BHEntity, query) == 0xd0, "BHEntity.query offset");
static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");
static_assert(sizeof(long) == 4, "long size");
static_assert(sizeof(BHEntity *) == 4, "target pointer word");
static_assert(offsetof(BHEntityPositionPacket, position) == 4, "BHEntityPositionPacket.position offset");
typedef union BHTargetQueryPacket {
    BHEntityPositionPacket position;
    unsigned long words[10];
} BHTargetQueryPacket;
static_assert(sizeof(BHTargetQueryPacket) == 40, "BHTargetQueryPacket size");

static __inline float selection_stored(double value)
{
    union {
        float value;
        volatile unsigned long bits;
    } written, read;
    written.value = (float)value;
    read.bits = written.bits;
    return read.value;
}

int bh_target_select(BHEntity *player, BHEntity *head, const BHEntity *excluded, const BHVec3 *origin,
                     unsigned long limit, BHTargetSelection *result)
{
    BHTargetQueryPacket packet;
    long eligible, kind, weight;
    double x, y, z, score;
    float stored_x, stored_y, stored_z;
    volatile float best;
    if (!origin || !result)
        return 0;
    best = 9.0e37f;
    result->target = NULL;
    result->visited = 0;
    result->score = best;
    if (player)
    {
        memcpy(&eligible, &player->player.bounty_pending, 4);
        if (eligible)
        {
            x = (double)player->position.x - origin->x;
            y = (double)player->position.y - origin->y;
            z = (double)player->position.z - origin->z;
            stored_x = selection_stored(x);
            stored_z = selection_stored(z);
            memcpy(&weight, &player->player.fine, 4);
            best = (float)(sqrt((x * stored_x + y * y) + z * stored_z) / (double)weight);
            result->target = player;
            result->score = best;
        }
    }
    while (head)
    {
        if (result->visited >= limit)
            return 0;
        ++result->visited;
        memcpy(&kind, &head->actor.kind, 4);
        if (head != excluded && kind == 2)
        {
            if (!head->query)
                return 0;
            packet.position.event = 3;
            head->query(head, &packet);
            x = (double)packet.position.position.x - origin->x;
            y = (double)packet.position.position.y - origin->y;
            z = (double)packet.position.position.z - origin->z;
            stored_x = selection_stored(x);
            stored_y = selection_stored(y);
            memcpy(&weight, &head->actor.bounty_token, 4);
            score = sqrt((x * stored_x + y * stored_y) + z * z) / (double)weight;
            /* FCOM's C0 is set for less OR unordered, not just C '<'. */
            if (!(score >= best))
            {
                best = (float)score;
                result->target = head;
                result->score = best;
            }
        }
        memcpy(&head, &head->actor.global_next, 4);
    }
    return 1;
}
