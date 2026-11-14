#include "bh_editor_numeric.h"
#include <string.h>
static short wrap(long value)
{
    unsigned short bits = (unsigned short)value;
    short result;
    memcpy(&result, &bits, 2);
    return result;
}
int bh_editor_numeric_key(BHEditorNumericContext *context, BHEntity *entity, long key)
{
    short value, digit;
    unsigned char *field;
    if (!context)
        return 0;
    if (context->selected < 0)
        return 1;
    if (!entity || context->selected >= 8)
        return 0;
    field = (unsigned char *)&entity->placement.script_values[context->selected];
    digit = wrap(key);
    memcpy(&value, field, 2);
    switch (context->entry_state)
    {
    case 0:
        if (key >= 0)
        {
            context->entry_state = 1;
            value = digit;
        }
        else
        {
            context->entry_state = 2;
            value = 0;
        }
        break;
    case 1:
        if (key >= 0)
        {
            while (value > 999)
                value = wrap((long)value - 1000);
            value = wrap((long)value * 10);
            value = wrap((long)value + digit);
            context->entry_state = 1;
        }
        else
        {
            context->entry_state = 2;
            value = wrap(-(long)value);
        }
        break;
    case 2:
        if (key < 0)
        {
            context->entry_state = 1;
            value = wrap(-(long)value);
        }
        else
        {
            while (value < -999)
                value = wrap((long)value + 1000);
            value = wrap((long)value * 10);
            value = wrap((long)value - digit);
        }
        break;
    default:
        return 1;
    }
    memcpy(field, &value, 2);
    return 1;
}
