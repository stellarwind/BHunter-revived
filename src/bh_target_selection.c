#include "bh_target_selection.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

typedef char target_query_offset[offsetof(BHEntity, query) == 0xd0 ? 1 : -1];
typedef char target_payload_offset[offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
typedef char target_signed_word[sizeof(long) == 4 ? 1 : -1];
typedef char target_pointer_word[sizeof(BHEntity *) == 4 ? 1 : -1];
typedef char target_packet_position[offsetof(BHEntityPositionPacket, position) == 4 ? 1 : -1];
typedef union BHTargetQueryPacket {
    BHEntityPositionPacket position;
    unsigned long words[10];
} BHTargetQueryPacket;
typedef char target_packet_capacity[sizeof(BHTargetQueryPacket) == 40 ? 1 : -1];

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

int __cdecl bh_target_select(BHEntity *player, BHEntity *head, const BHEntity *excluded, const BHVec3 *origin,
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
        memcpy(&eligible, player->payload + 0x154, 4);
        if (eligible)
        {
            x = (double)player->position.x - origin->x;
            y = (double)player->position.y - origin->y;
            z = (double)player->position.z - origin->z;
            stored_x = selection_stored(x);
            stored_z = selection_stored(z);
            memcpy(&weight, player->payload + 0x150, 4);
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
        memcpy(&kind, head->payload + 24, 4);
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
            memcpy(&weight, head->payload + 28, 4);
            score = sqrt((x * stored_x + y * stored_y) + z * z) / (double)weight;
            /* FCOM's C0 is set for less OR unordered, not just C '<'. */
            if (!(score >= best))
            {
                best = (float)score;
                result->target = head;
                result->score = best;
            }
        }
        memcpy(&head, head->payload + 12, 4);
    }
    return 1;
}
