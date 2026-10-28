#include "bh_messages.h"
#include "bh_mission_profile.h"
#include <string.h>
#include <stddef.h>

typedef char message_ring_slot_size[sizeof(((BHMessageState *)0)->ring[0]) == 512 ? 1 : -1];
typedef char message_arena_size[sizeof(((BHMessageState *)0)->arena) == 8192 ? 1 : -1];
void __cdecl bh_message_state_init(BHMessageState *messages)
{
    if (messages)
    {
        memset(messages, 0, sizeof(*messages));
        messages->last_index = -1;
    }
}
static unsigned long folded_eight(long value)
{
    unsigned long sign = (unsigned long)value >> 31;
    unsigned long magnitude = sign ? 0UL - (unsigned long)value : (unsigned long)value;
    unsigned long folded = magnitude & 7UL;
    return sign ? 0UL - folded : folded;
}
int __cdecl bh_message_enqueue(BHMessageState *messages, const char *text)
{
    size_t length;
    unsigned long slot;
    long next;
    float panel_time;
    if (!messages || !text)
    {
        if (messages)
            messages->error = BH_MESSAGE_ARGUMENT;
        return 0;
    }
    if (messages->ring_count < 0 || messages->ring_count > 8 || messages->ring_start < 0 || messages->ring_start > 7 ||
        messages->arena_used > 8192)
    {
        messages->error = BH_MESSAGE_ARGUMENT;
        return 0;
    }
    length = strlen(text) + 1;
    if (length > 512)
    {
        messages->error = BH_MESSAGE_RING_SIZE;
        return 0;
    }
    if (messages->ring_count == 8)
    {
        next = (long)((unsigned long)messages->ring_start + 1UL);
        messages->ring_start = (long)folded_eight(next);
    }
    else
        ++messages->ring_count;
    slot = folded_eight((long)((unsigned long)messages->ring_count + (unsigned long)messages->ring_start - 1UL));
    memcpy(messages->ring[slot], text, length);
    messages->last_index = (long)slot;
    messages->display_ticks = 64;
    messages->display_phase = 0;
    {
        BH_MP_BEGIN(marker);
        BH_MP_END(marker, "message_arrival", "portrait", (unsigned char)text[0]);
    }
    if (length > 8192 - messages->arena_used)
    {
        messages->error = BH_MESSAGE_ARENA_FULL;
        return 0;
    }
    memcpy(messages->arena + messages->arena_used, text, length);
    messages->arena_used += (unsigned long)length;
    if (!messages->panel_entity || !messages->panel_step_identity)
    {
        messages->error = BH_MESSAGE_PANEL_BINDING;
        return 0;
    }
    if (*messages->panel_step_identity == 0x00442950UL)
    {
        memcpy(&panel_time, messages->panel_entity->payload + 4, 4); /* +e8 */
        if (panel_time > 3.0f)
        {
            panel_time = 3.0f;
            memcpy(messages->panel_entity->payload + 4, &panel_time, 4);
        }
    }
    messages->error = BH_MESSAGE_OK;
    return 1;
}
