#include "bh_actor_launch.h"
#include <math.h>
#include <stdlib.h>
static __inline float rf(double v)
{
    union {
        float value;
        volatile unsigned long bits;
    } written, read;
    written.value = (float)v;
    read.bits = written.bits;
    return read.value;
}
static unsigned short current_tick(BHActorLaunchContext *ctx)
{
    BHActorRouteContext *route = (ctx->actors && ctx->actors->response && ctx->actors->response->damage)
                                     ? ctx->actors->response->damage->route
                                     : NULL;
    return route && route->live_tick ? bh_actor_route_now(route) : ctx->tick;
}

BHEntity *__cdecl bh_actor_launch(BHActorLaunchContext *ctx, BHWorldGeometry *world, unsigned long type, long token,
                                  float speed, long tag)
{
    unsigned long r, wire, i;
    unsigned char payload[9];
    BHVec3 center;
    long quantized;
    int have_index = 0;
    unsigned short index = 0;
    if (!ctx || !ctx->actors)
    {
        if (ctx)
            ctx->error = 1;
        return 0;
    }
    ctx->error = 0;
    r = (unsigned long)rand();
    r += (unsigned long)rand() * 65536UL;
    if (type == 0 || type == 3)
    {
        if (ctx->mode == 3)
            return 0;
        if (ctx->mode == 2)
        {
            if (!world)
                return 0;
            if (!ctx->transport || !ctx->navigation)
            {
                ctx->error = 1;
                return 0;
            }
            center.x = rf(((double)world->maximum.x + world->minimum.x) * .5);
            center.y = rf(((double)world->maximum.y + world->minimum.y) * .5);
            center.z = rf(((double)world->maximum.z + world->minimum.z) * .5);
            token = (token / 5000) * 5000;
            /* 0043bd61..0043bd8a: original __ftol converts to 64 bits,
               then only EAX is stored and reloaded with FILD dword. Preserve
               that low-word conversion, including masked invalid inputs. */
            quantized = (long)(__int64)speed;
            speed = (float)quantized;
            for (i = 0; i < ctx->navigation->node_count; ++i)
                if (ctx->navigation->nodes[i].placement == world)
                {
                    index = (unsigned short)i;
                    have_index = 1;
                    break;
                }
            if (!have_index)
            {
                ctx->error = 1;
                return 0;
            }
            payload[0] = (unsigned char)r;
            payload[1] = (unsigned char)(r >> 8);
            payload[2] = (unsigned char)(r >> 16);
            payload[3] = (unsigned char)(r >> 24);
            payload[4] = (unsigned char)tag;
            payload[5] = (unsigned char)((unsigned long)tag >> 8);
            payload[6] = (unsigned char)type;
            payload[7] = (unsigned char)(token / 5000);
            payload[8] = (unsigned char)quantized;
            if (!bh_packet_begin(&ctx->writer, ctx->packet, sizeof(ctx->packet), 10, current_tick(ctx)) ||
                !bh_packet_write_position(&ctx->writer, world, index, &center) ||
                !bh_packet_append(&ctx->writer, payload, 9) || !bh_packet_finish(&ctx->writer, &wire))
            {
                ctx->error = 2;
                return 0;
            }
            ctx->send_result = bh_network_send(ctx->transport, 0, ctx->packet, wire, 0);
            ++ctx->sent;
        }
    }
    return bh_actor_construct_room(ctx->actors, world, type, token, speed, (unsigned long)tag, r);
}
int __cdecl bh_actor_launch_receive(BHActorLaunchContext *ctx, void *bytes, unsigned long size, unsigned long capacity)
{
    BHWorldGeometry *world = 0;
    BHVec3 position;
    unsigned char *p;
    unsigned long seed;
    short tag;
    unsigned short saved, tick;
    BHEntity *entity;
    if (!ctx || !ctx->actors || !ctx->actors->response || !ctx->actors->response->damage ||
        !ctx->actors->response->damage->route || !ctx->actors->response->damage->route->live_tick || !ctx->navigation ||
        !ctx->stats || !ctx->good)
    {
        if (ctx)
            ctx->error = 1;
        return 0;
    }
    ctx->error = 0;
    /* These bounded source bridges return -1 for malformed input. Only 1
       represents the original owner's successful envelope/decode path. */
    if (bh_packet_receive(&ctx->reader, bytes, size, capacity, ctx->stats) != 1 ||
        bh_packet_read_position(&ctx->reader, ctx->navigation, &world, &position) != 1)
    {
        ctx->error = 2;
        return 0;
    }
    if (ctx->reader.cursor > ctx->reader.size || ctx->reader.size - ctx->reader.cursor < 9)
    {
        ctx->reader.error = BH_PACKET_TRUNCATED;
        ctx->error = 2;
        return 0;
    }
    p = ctx->reader.data + ctx->reader.cursor;
    ctx->reader.cursor += 9;
    seed = (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
    tag = (short)((unsigned short)p[4] | ((unsigned short)p[5] << 8));
    --ctx->stats->accepted;
    ++*ctx->good;
    tick = (unsigned short)ctx->reader.data[2] | ((unsigned short)ctx->reader.data[3] << 8);
    saved = bh_actor_route_now(ctx->actors->response->damage->route);
    bh_actor_route_set_tick(ctx->actors->response->damage->route, tick);
    entity = bh_actor_construct_position(ctx->actors, &position, (unsigned long)(long)(signed char)p[6],
                                         (long)(signed char)p[7] * 5000, (float)(signed char)p[8],
                                         (unsigned long)(long)tag, seed);
    bh_actor_route_set_tick(ctx->actors->response->damage->route, saved);
    ++ctx->received;
    if (!entity)
    {
        ctx->error = 3;
        return 0;
    }
    return 1;
}
