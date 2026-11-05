#include "bh_input_devices.h"
#include "bh_platform.h"
#include "bh_input_key_names.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
static void *__cdecl allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void __cdecl release(void *memory, void *user)
{
    free(memory);
}
int __cdecl bh_input_devices_bind(BHInputDevices *d, BHInputCollection *c, const BHSceneAllocator *a)
{
    unsigned long i;
    if (!d || !c || !c->snapshot || !c->lists || (a && (!a->allocate || !a->release)))
        return 0;
    memset(d, 0, sizeof(*d));
    d->collection = c;
    for (i = 0; i < 255; ++i)
        d->names[i] = bh_input_key_defaults[i];
    if (a)
        d->allocator = *a;
    else
    {
        d->allocator.allocate = allocate;
        d->allocator.release = release;
    }
    return 1;
}
int __cdecl bh_input_devices_start(BHInputDevices *d)
{
    BHInputCollection *c;
    BHInputSnapshot *s;
    unsigned long i, character, bytes;
    char *name;
    int replace;
    if (!d || !(c = d->collection) || !(s = c->snapshot) || !c->lists || c->lists->first ||
        !d->allocator.allocate || !d->allocator.release)
        return 0;
    d->allocation_failures = 0;
    for (i = 0; i < 255; ++i)
    {
        character = bh_sdl_key_character((unsigned)i) & 255;
        replace = character != i && i >= 65 && i <= 90 && character >= 65 && character <= 90;
        if (replace || !d->names[i])
        {
            if (d->allocated[i])
                d->allocator.release((void *)d->names[i], d->allocator.user);
            d->allocated[i] = 0;
            bytes = replace || character >= 32 ? 8 : 10;
            name = (char *)d->allocator.allocate(bytes, d->allocator.user);
            d->names[i] = name;
            if (!name)
            {
                ++d->allocation_failures;
                continue;
            }
            d->allocated[i] = 1;
            if (replace || character >= 32)
                sprintf(name, "Key '%c'", (int)character);
            else
                sprintf(name, "Key #%d", (int)i);
        }
    }
    s->history_head = 0;
    memset(s->history, 0, 32);
    memset(s->current, 0, 255);
    memset(s->previous, 0, 255);
    memset(s->axes, 0, sizeof(s->axes));
    memset(&s->gamepad, 0, sizeof(s->gamepad));
    s->previous_gamepad_buttons = 0;
    c->last_recenter = bh_sdl_warp(160, 100);
    c->delta[0] = c->delta[1] = 0;
    s->pair4[0] = s->pair4[1] = 0;
    s->pair3[0] = s->pair3[1] = 512;
    s->mouse_axes[0] = s->mouse_axes[1] = 0;
    return !d->allocation_failures;
}
void __cdecl bh_input_devices_dispose(BHInputDevices *d)
{
    unsigned long i;
    if (!d)
        return;
    if (d->allocator.release)
        for (i = 0; i < 255; ++i)
            if (d->allocated[i])
                d->allocator.release((void *)d->names[i], d->allocator.user);
    for (i = 0; i < 255; ++i)
    {
        d->allocated[i] = 0;
        d->names[i] = bh_input_key_defaults[i];
    }
}
