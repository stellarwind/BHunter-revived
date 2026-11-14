#include "bh_input_collection.h"
#include "../Platform/bh_platform.h"
#include <string.h>
static int fail(BHInputCollection *input_collection, BHInputCollectionError error)
{
    if (input_collection)
        input_collection->error = error;
    return 0;
}
int bh_input_collection_init(BHInputCollection *input_collection, BHInputSnapshot *input_snapshot, BHPlayerInputOwner *lists)
{
    if (!input_collection || !input_snapshot || !lists)
        return 0;
    memset(input_collection, 0, sizeof(*input_collection));
    memset(input_snapshot, 0, sizeof(*input_snapshot));
    input_collection->snapshot = input_snapshot;
    input_collection->lists = lists;
    input_collection->mouse_sensitivity = 1.0f;
    input_snapshot->pair3[0] = input_snapshot->pair3[1] = 512;
    return 1;
}
int bh_input_collect(BHInputCollection *input_collection, unsigned long *changed)
{
    BHInputSnapshot *input_snapshot;
    BHPlayerInputList *list, *slow, *fast;
    float motion[2] = {0, 0};
    unsigned long i, count = 0;
    if (!input_collection || !changed || !input_collection->snapshot || !input_collection->lists || input_collection->snapshot->history_head > 31)
        return fail(input_collection, BH_INPUT_COLLECTION_ARGUMENT);
    input_snapshot = input_collection->snapshot;
    slow = fast = input_collection->lists->first;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return fail(input_collection, BH_INPUT_COLLECTION_TOPOLOGY);
    }
    input_collection->error = BH_INPUT_COLLECTION_OK;
    bh_sdl_gamepad(&input_snapshot->gamepad);
    if (!input_collection->disable_mouse_motion)
        bh_sdl_relative_mouse(motion, motion + 1);
    for (i = 0; i < 2; ++i)
    {
        /* Consume only this tick's movement. The evaluator clamps the combined
           mouse and gamepad axis and applies any keyboard override. */
        float axis = motion[i] * .01f * input_collection->mouse_sensitivity;
        /* SDL Y grows downward. Bindings receive up-positive mouse motion by default. */
        if (i == 1 && !input_collection->mouse_invert_y)
            axis = -axis;
        input_snapshot->mouse_axes[i] = axis < -1 ? -1 : axis > 1 ? 1 : axis;
        input_snapshot->pair4[i] = (unsigned short)(long)(motion[i] < -32768 ? -32768 : motion[i] > 32767 ? 32767 : motion[i]);
    }
    for (list = input_collection->lists->first; list; list = list->next)
    {
        if (list->flags == 2)
        {
            list->flags = 1;
            if (!bh_input_evaluate(list, input_snapshot, 1))
                return fail(input_collection, BH_INPUT_COLLECTION_EVALUATE);
        }
        else if (list->flags && !bh_input_evaluate(list, input_snapshot, 0))
            return fail(input_collection, BH_INPUT_COLLECTION_EVALUATE);
    }
    for (i = 0; i < 255; ++i)
    {
        if (input_snapshot->previous[i] != input_snapshot->current[i])
            ++count;
        input_snapshot->previous[i] = input_snapshot->current[i];
    }
    *changed = count;
    input_snapshot->previous_gamepad_buttons = input_snapshot->gamepad.buttons;
    return 1;
}
