#include "bh_input_messages.h"
#include <string.h>
void bh_input_event(BHInputSnapshot *snapshot, const BHPlatformEvent *event)
{
    unsigned key;
    if (!snapshot || !event)
        return;
    if (event->type == BH_PLATFORM_FOCUS)
    {
        if (!event->down)
        {
            memset(snapshot->current, 0, sizeof(snapshot->current));
            memset(snapshot->previous, 0, sizeof(snapshot->previous));
            memset(snapshot->history, 0, sizeof(snapshot->history));
            snapshot->history_head = 0;
            memset(&snapshot->gamepad, 0, sizeof(snapshot->gamepad));
            snapshot->previous_gamepad_buttons = 0;
        }
        return;
    }
    key = event->key;
    if (event->type != BH_PLATFORM_KEY || !key || key >= 255)
        return;
    if (event->down && !event->repeat && !snapshot->current[key])
    {
        if (key > 6)
        {
            snapshot->history[snapshot->history_head] = (unsigned char)key;
            snapshot->history_head = (snapshot->history_head + 1) & 31;
        }
        snapshot->current[key] = 1;
    }
    else if (!event->down)
        snapshot->current[key] = 0;
}
