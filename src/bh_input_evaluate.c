#include "bh_input_evaluate.h"
#include <stddef.h>
#include <string.h>
typedef char history_offset[offsetof(BHInputSnapshot, history) == 510 ? 1 : -1];
typedef char history_head_offset[offsetof(BHInputSnapshot, history_head) == 542 ? 1 : -1];
static int key(const BHInputSnapshot *s, const unsigned char *history, unsigned short index, int previous)
{
    if (previous && index == 255)
        return history[0] != 0;
    return ((const unsigned char *)s)[index + (previous ? 255 : 0)] != 0;
}
int __cdecl bh_input_evaluate(BHPlayerInputList *list, BHInputSnapshot *snapshot, int full)
{
    unsigned char output[0x120], down[256], up[256], history[32], head;
    unsigned long n_down = 0, n_up = 0, at = 2, records = 0, i, j, n, pad;
    BHInputBinding *b;
    float value, mouse;
    const char *end;
    int match;
    if (!list || !snapshot || list->count < 0 || list->count > 256 || snapshot->history_head > 31)
        return 0;
    memcpy(history, snapshot->history, 32);
    head = snapshot->history_head;
    for (i = 0; i < (unsigned long)list->count; ++i)
    {
        b = list->bindings + i;
        if ((b->kind == 1 && (b->modifiers > 255 || b->key[0] > 255)) ||
            (b->kind == 2 && (b->modifiers > 255 || b->key[0] > 255 || b->key[1] > 255)))
            return 0;
        switch (b->kind)
        {
        case 1:
            if (full)
            {
                if (key(snapshot, history, b->key[0], 0) && (!b->modifiers || key(snapshot, history, b->modifiers, 0)))
                    down[n_down++] = (unsigned char)i;
                else
                    up[n_up++] = (unsigned char)i;
            }
            else if (key(snapshot, history, b->key[0], 0))
            {
                if (!key(snapshot, history, b->key[0], 1) && (!b->modifiers || key(snapshot, history, b->modifiers, 0)))
                    down[n_down++] = (unsigned char)i;
            }
            else if (key(snapshot, history, b->key[0], 1))
                up[n_up++] = (unsigned char)i;
            break;
        case 2:
            if (at + 6 > sizeof(output))
                return 0;
            j = b->key[2];
            value = (j >= 1 && j <= 16) ? snapshot->axes[(j - 1) / 2] : 0;
            if (j >= 1 && j <= 16 && !(j & 1))
                value = -value;
            j = b->key[3];
            mouse = (j >= 1 && j <= 4) ? snapshot->mouse_axes[(j - 1) / 2] : 0;
            if (j >= 1 && j <= 4 && !(j & 1))
                mouse = -mouse;
            value = value + mouse;
            if (!(value >= -1))
                value = -1;
            else if (value > 1)
                value = 1;
            if (!b->modifiers || key(snapshot, history, b->modifiers, 0))
            {
                if (b->key[0] && key(snapshot, history, b->key[0], 0))
                    value = 1;
                if (b->key[1] && key(snapshot, history, b->key[1], 0))
                    value = -1;
            }
            output[at++] = 2;
            output[at++] = (unsigned char)i;
            memcpy(output + at, &value, 4);
            at += 4;
            ++records;
            break;
        case 3:
        case 4:
            if (at + 6 > sizeof(output))
                return 0;
            output[at++] = b->kind;
            output[at++] = (unsigned char)i;
            memcpy(output + at, b->kind == 3 ? snapshot->pair3 : snapshot->pair4, 4);
            at += 4;
            ++records;
            break;
        case 5:
            end = (const char *)memchr(b->name, 0, 20);
            if (!end)
                return 0;
            n = (unsigned long)(end - b->name);
            match = 1;
            for (j = 0; j < n; ++j)
                if ((int)(signed char)b->name[n - j - 1] != (int)history[(head - j - 1) & 31])
                {
                    match = 0;
                    break;
                }
            if (match)
            {
                history[head] = 0;
                head = (unsigned char)((head + 1) & 31);
                down[n_down++] = (unsigned char)i;
            }
            break;
        }
    }
    pad = (4 - ((n_down + 2) & 3)) & 3;
    if (at + 2 + n_down + pad > sizeof(output))
        return 0;
    output[at++] = 0;
    output[at++] = (unsigned char)n_down;
    memcpy(output + at, down, n_down);
    at += n_down;
    memset(output + at, 0, pad);
    at += pad;
    pad = (4 - ((n_up + 2) & 3)) & 3;
    if (at + 2 + n_up + pad > sizeof(output))
        return 0;
    output[at++] = 1;
    output[at++] = (unsigned char)n_up;
    memcpy(output + at, up, n_up);
    at += n_up;
    memset(output + at, 0, pad);
    at += pad;
    output[0] = (unsigned char)at;
    output[1] = (unsigned char)(records + 2);
    memcpy(list->state, output, at);
    memcpy(snapshot->history, history, 32);
    snapshot->history_head = head;
    return 1;
}
int __cdecl bh_input_set_enabled(BHPlayerInputList *list, BHInputSnapshot *snapshot, int mode)
{
    if (!list)
        return 0;
    if (mode)
    {
        list->flags = 2;
        return 1;
    }
    if (!bh_input_evaluate(list, snapshot, 1))
        return 0;
    list->flags = 0;
    return 1;
}
