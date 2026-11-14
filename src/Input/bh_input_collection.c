#include "bh_input_collection.h"
#include "../Platform/bh_platform.h"
#include <string.h>
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
    c->mouse_sensitivity = 1.0f;
    s->pair3[0] = s->pair3[1] = 512;
    return 1;
}
int bh_input_collect(BHInputCollection *c, unsigned long *changed)
{
    BHInputSnapshot *s;
    BHPlayerInputList *list, *slow, *fast;
    float motion[2] = {0, 0};
    unsigned long i, count = 0;
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
    if (!c->disable_mouse_motion)
        bh_sdl_relative_mouse(motion, motion + 1);
    for (i = 0; i < 2; ++i)
    {
        /* Consume only this tick's movement. The evaluator clamps the combined
           mouse and gamepad axis and applies any keyboard override. */
        float axis = motion[i] * .01f * c->mouse_sensitivity;
        /* SDL Y grows downward. Bindings receive up-positive mouse motion by default. */
        if (i == 1 && !c->mouse_invert_y)
            axis = -axis;
        s->mouse_axes[i] = axis < -1 ? -1 : axis > 1 ? 1 : axis;
        s->pair4[i] = (unsigned short)(long)(motion[i] < -32768 ? -32768 : motion[i] > 32767 ? 32767 : motion[i]);
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
