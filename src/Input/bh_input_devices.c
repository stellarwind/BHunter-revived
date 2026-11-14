#include "bh_input_devices.h"
#include "../Platform/bh_platform.h"
#include "bh_input_key_names.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
static void *allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void release(void *memory, void *user)
{
    free(memory);
}
int bh_input_devices_bind(BHInputDevices *input_devices, BHInputCollection *input_collection, const BHSceneAllocator *scene_allocator)
{
    unsigned long i;
    if (!input_devices || !input_collection || !input_collection->snapshot || !input_collection->lists || (scene_allocator && (!scene_allocator->allocate || !scene_allocator->release)))
        return 0;
    memset(input_devices, 0, sizeof(*input_devices));
    input_devices->collection = input_collection;
    for (i = 0; i < 255; ++i)
        input_devices->names[i] = bh_input_key_defaults[i];
    if (scene_allocator)
        input_devices->allocator = *scene_allocator;
    else
    {
        input_devices->allocator.allocate = allocate;
        input_devices->allocator.release = release;
    }
    return 1;
}
int bh_input_devices_start(BHInputDevices *input_devices)
{
    BHInputCollection *input_collection;
    BHInputSnapshot *input_snapshot;
    unsigned long i, character, bytes;
    char *name;
    int replace;
    if (!input_devices || !(input_collection = input_devices->collection) || !(input_snapshot = input_collection->snapshot) || !input_collection->lists || input_collection->lists->first ||
        !input_devices->allocator.allocate || !input_devices->allocator.release)
        return 0;
    input_devices->allocation_failures = 0;
    for (i = 0; i < 255; ++i)
    {
        character = bh_sdl_key_character((unsigned)i) & 255;
        replace = character != i && i >= 65 && i <= 90 && character >= 65 && character <= 90;
        if (replace || !input_devices->names[i])
        {
            if (input_devices->allocated[i])
                input_devices->allocator.release((void *)input_devices->names[i], input_devices->allocator.user);
            input_devices->allocated[i] = 0;
            bytes = replace || character >= 32 ? 8 : 10;
            name = (char *)input_devices->allocator.allocate(bytes, input_devices->allocator.user);
            input_devices->names[i] = name;
            if (!name)
            {
                ++input_devices->allocation_failures;
                continue;
            }
            input_devices->allocated[i] = 1;
            if (replace || character >= 32)
                sprintf(name, "Key '%c'", (int)character);
            else
                sprintf(name, "Key #%d", (int)i);
        }
    }
    input_snapshot->history_head = 0;
    memset(input_snapshot->history, 0, 32);
    memset(input_snapshot->current, 0, 255);
    memset(input_snapshot->previous, 0, 255);
    memset(input_snapshot->axes, 0, sizeof(input_snapshot->axes));
    memset(&input_snapshot->gamepad, 0, sizeof(input_snapshot->gamepad));
    input_snapshot->previous_gamepad_buttons = 0;
    input_snapshot->pair4[0] = input_snapshot->pair4[1] = 0;
    input_snapshot->pair3[0] = input_snapshot->pair3[1] = 512;
    input_snapshot->mouse_axes[0] = input_snapshot->mouse_axes[1] = 0;
    return !input_devices->allocation_failures;
}
void bh_input_devices_dispose(BHInputDevices *input_devices)
{
    unsigned long i;
    if (!input_devices)
        return;
    if (input_devices->allocator.release)
        for (i = 0; i < 255; ++i)
            if (input_devices->allocated[i])
                input_devices->allocator.release((void *)input_devices->names[i], input_devices->allocator.user);
    for (i = 0; i < 255; ++i)
    {
        input_devices->allocated[i] = 0;
        input_devices->names[i] = bh_input_key_defaults[i];
    }
}
