#include "bh_effect_network.h"
#include <string.h>
int bh_effect_network_emit(BHEffectNetworkContext *ctx, const BHEntity *entity, unsigned long world_offset,
                           unsigned long sync_offset, unsigned char kind, const BHVec3 *direction)
{
    unsigned long sync, i;
    BHWorldGeometry *world;
    BHPacketWriter writer;
    unsigned char payload[2];
    if (!ctx || !entity || !direction || !ctx->transport)
        return 0;
    ctx->error = 0;
    memcpy(&sync, (const unsigned char *)entity + sync_offset, 4);
    if (ctx->peers <= 1 || !sync)
        return 1;
    memcpy(&world, (const unsigned char *)entity + world_offset, 4);
    if (!world)
        return 1;
    for (i = 0; i < ctx->world_count; i++)
        if (ctx->world_indices[i].world == world)
            break;
    if (i == ctx->world_count)
    {
        ctx->error = 1;
        return 0;
    }
    if (!bh_packet_begin(&writer, ctx->packet, sizeof(ctx->packet), 9, (unsigned short)sync) ||
        bh_packet_write_position(&writer, world, ctx->world_indices[i].index, &entity->position) != 1)
    {
        ctx->error = 1;
        return 0;
    }
    payload[0] = (unsigned char)(sync >> 16);
    payload[1] = kind;
    if (!bh_packet_append(&writer, payload, 2) || !bh_packet_write_vector(&writer, direction, 1) ||
        !bh_packet_finish(&writer, &ctx->bytes))
    {
        ctx->error = 1;
        return 0;
    }
    bh_network_send(ctx->transport, 0, ctx->packet, ctx->bytes, 0);
    ++ctx->packets;
    return 1;
}
