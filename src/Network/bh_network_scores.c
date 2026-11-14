#include "bh_network_scores.h"
#include <string.h>
static int valid(BHNetworkScores *context)
{
    if (!context || !context->scores || !context->scores->remote || !context->scores->local_name || !context->text ||
        !context->text->queue || !context->transport)
    {
        if (context)
            context->error = 1;
        return 0;
    }
    context->error = 0;
    return 1;
}
static void word(unsigned char *destination, unsigned short value)
{
    destination[0] = (unsigned char)value;
    destination[1] = (unsigned char)(value >> 8);
}
static int snapshot(BHNetworkScores *context)
{
    unsigned char payload[16];
    unsigned long player_index;
    unsigned short score;
    for (player_index = 0; player_index < 8; ++player_index)
    {
        score = 0;
        if ((long)player_index == context->scores->local_index)
            score = (unsigned short)context->scores->local_score;
        else if (context->scores->remote[player_index])
            memcpy(&score, &context->scores->remote[player_index]->remote.score, 2);
        word(payload + player_index * 2, score);
    }
    if (!bh_packet_begin(&context->writer, context->packet, sizeof(context->packet), 12, context->frame) ||
        !bh_packet_append(&context->writer, payload, 16))
    {
        context->error = 2;
        return 0;
    }
    context->send_result = bh_network_packet_send(context->transport, &context->writer, 0, 0);
    ++context->broadcasts;
    if (!bh_hud_scores_text(context->text, context->scores))
    {
        context->error = 3;
        return 0;
    }
    return 1;
}
int bh_network_scores_broadcast(BHNetworkScores *context)
{
    if (!valid(context))
        return 0;
    return snapshot(context);
}
int bh_network_scores_death(BHNetworkScores *context, long attacker, BHEntity *victim)
{
    long credited_slot = -1;
    unsigned long player_index, local_score_bits;
    unsigned short score;
    unsigned char payload[2];
    if (!valid(context))
        return 0;
    for (player_index = 0; player_index < 8; ++player_index)
        if (context->scores->remote[player_index] == victim)
            credited_slot = (long)player_index;
    if (victim == context->local_player)
        credited_slot = attacker;
    if (context->mode == 2)
    {
        if (attacker >= 0 && attacker < 8)
        {
            local_score_bits = (unsigned long)context->scores->local_score - 1;
            memcpy(&context->scores->local_score, &local_score_bits, 4);
        }
        if (credited_slot >= 0 && credited_slot < 8)
        {
            if (credited_slot == attacker)
            {
                local_score_bits = (unsigned long)context->scores->local_score + 1;
                memcpy(&context->scores->local_score, &local_score_bits, 4);
            }
            else if (context->scores->remote[credited_slot])
            {
                memcpy(&score, &context->scores->remote[credited_slot]->remote.score, 2);
                ++score;
                memcpy(&context->scores->remote[credited_slot]->remote.score, &score, 2);
            }
        }
        return snapshot(context);
    }
    payload[0] = (unsigned char)attacker;
    payload[1] = (unsigned char)credited_slot;
    if (!bh_packet_begin(&context->writer, context->packet, sizeof(context->packet), 11, context->frame) ||
        !bh_packet_append(&context->writer, payload, 2))
    {
        context->error = 2;
        return 0;
    }
    context->send_result = bh_network_packet_send(context->transport, &context->writer, BH_NETWORK_SERVER, 0);
    ++context->reports;
    return 1;
}
