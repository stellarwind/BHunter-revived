#include "bh_input_collection.h"
#include "bh_platform.h"
#include "bh_round.h"
#include <string.h>
#include <float.h>
static int fail(BHInputCollection *c, BHInputCollectionError error)
{
    if (c)
        c->error = error;
    return 0;
}
int bh_input_collection_init(BHInputCollection *c, BHInputSnapshot *s, BHPlayerInputOwner *lists)
{
    if (!c || !s || !lists)
        return 0;
    memset(c, 0, sizeof(*c));
    memset(s, 0, sizeof(*s));
    c->snapshot = s;
    c->lists = lists;
    s->pair3[0] = s->pair3[1] = 512;
    return 1;
}
int bh_input_collect(BHInputCollection *c, unsigned long *changed)
{
    BHInputSnapshot *s;
    BHPlayerInputList *list, *slow, *fast;
    int x, y;
    unsigned long i, count = 0, bits;
    double filtered;
    if (!c || !changed || !c->snapshot || !c->lists || c->snapshot->history_head > 31)
        return fail(c, BH_INPUT_COLLECTION_ARGUMENT);
    s = c->snapshot;
    slow = fast = c->lists->first;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return fail(c, BH_INPUT_COLLECTION_TOPOLOGY);
    }
    c->error = BH_INPUT_COLLECTION_OK;
    bh_sdl_gamepad(&s->gamepad);
    if (c->disable_mouse_motion)
    {
        /* Clear retained filter state before binding evaluation. Polling or
           recentering is unnecessary when every mouse motion channel is off. */
        for (i = 0; i < 2; ++i)
        {
            c->delta[i] = 0;
            s->mouse_axes[i] = 0;
            s->pair3[i] = 512;
            s->pair4[i] = 0;
        }
    }
    else
    {
        c->last_cursor = bh_sdl_mouse(&x, &y);
        if (!c->last_cursor)
            return fail(c, BH_INPUT_COLLECTION_DEVICE);
        c->last_recenter = bh_sdl_warp(160, 100);
        bits = (unsigned long)x - 160UL;
        memcpy(c->delta, &bits, 4);
        bits = (unsigned long)y - 100UL;
        memcpy(c->delta + 1, &bits, 4);
        for (i = 0; i < 2; ++i)
        {
            unsigned short accumulated = (unsigned short)(s->pair3[i] + (unsigned long)c->delta[i]);
            s->pair3[i] = accumulated > 1023 ? 1023 : accumulated;
            s->pair4[i] = (unsigned short)c->delta[i];
            filtered = ((double)s->mouse_axes[i] - (double)c->delta[i] * -0.002) * .99;
            bh_round_float(s->mouse_axes + i, filtered);
            if (!(filtered >= -1))
                s->mouse_axes[i] = -1;
            else if (s->mouse_axes[i] > 1)
                s->mouse_axes[i] = 1;
        }
    }
    for (list = c->lists->first; list; list = list->next)
    {
        if (list->flags == 2)
        {
            list->flags = 1;
            if (!bh_input_evaluate(list, s, 1))
                return fail(c, BH_INPUT_COLLECTION_EVALUATE);
        }
        else if (list->flags && !bh_input_evaluate(list, s, 0))
            return fail(c, BH_INPUT_COLLECTION_EVALUATE);
    }
    for (i = 0; i < 255; ++i)
    {
        if (s->previous[i] != s->current[i])
            ++count;
        s->previous[i] = s->current[i];
    }
    *changed = count;
    s->previous_gamepad_buttons = s->gamepad.buttons;
    return 1;
}
