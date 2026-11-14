#include "UI/bh_message_panel.h"
#include "bh_messages.h"
#include <string.h>
#include <stddef.h>

static_assert(sizeof(((BHMessageState *)0)->ring[0]) == 512, "message ring slot size");
static_assert(sizeof(((BHMessageState *)0)->arena) == 8192, "message arena size");
void bh_message_state_init(BHMessageState *messages)
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
int bh_message_enqueue(BHMessageState *messages, const char *text)
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
        memcpy(&panel_time, &bh_message_panel_fields(messages->panel_entity)->timer, 4); /* +e8 */
        if (panel_time > 3.0f)
        {
            panel_time = 3.0f;
            memcpy(&bh_message_panel_fields(messages->panel_entity)->timer, &panel_time, 4);
        }
    }
    messages->error = BH_MESSAGE_OK;
    return 1;
}
