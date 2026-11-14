#include "bh_network_effect_receive.h"
#include "../Math/bh_round.h"
#include <string.h>
int bh_network_effect_receive(BHNetworkEffectReceive *receiver, void *bytes, unsigned long size, unsigned long capacity)
{
    BHVec3 position;
    unsigned char *payload;
    unsigned long id;
    int result;
    unsigned long i;
    if (!receiver || !receiver->navigation || !receiver->pool || !receiver->stats || !receiver->good)
    {
        if (receiver)
            receiver->error = 1;
        return -1;
    }
    receiver->error = 0;
    result = bh_packet_receive(&receiver->reader, bytes, size, capacity, receiver->stats);
    if (result != 1)
        return result;
    result = bh_packet_read_position(&receiver->reader, receiver->navigation, &receiver->world, &position);
    if (result != 1)
        return result;
    if (receiver->reader.cursor >= receiver->reader.size)
    {
        receiver->error = 2;
        return -1;
    }
    payload = receiver->reader.data + receiver->reader.cursor++;
    if (*payload >= 8)
        return 1;
    if (receiver->reader.size - receiver->reader.cursor < 4)
    {
        receiver->error = 2;
        return -1;
    }
    memset(&receiver->packet, 0, sizeof(receiver->packet));
    id = receiver->reader.data[2] | ((unsigned long)receiver->reader.data[3] << 8) | ((unsigned long)*payload << 16) | 0x40000000UL;
    receiver->packet.words[0] = 18;
    receiver->packet.words[1] = (long)id;
    receiver->packet.words[2] = payload[1];
    memcpy(receiver->packet.floats + 4, &position, 12);
    for (i = 0; i < 3; ++i)
        bh_round_float(receiver->packet.floats + 7 + i, (double)(signed char)payload[2 + i] * .01171875f);
    receiver->reader.cursor += 4;
    ++receiver->dispatched;
    if (!bh_entities_query(receiver->pool, 0, 0, NULL, 0, &receiver->packet))
    {
        receiver->error = 3;
        return -1;
    }
    if (receiver->packet.words[1] == -1)
    {
        --receiver->stats->accepted;
        ++*receiver->good;
    }
    return 1;
}
