#include "bh_frame_buffers.h"
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef char frame_slot_size[sizeof(BHFrameSlot) == 12 ? 1 : -1];
typedef char frame_slot_busy[offsetof(BHFrameSlot, busy) == 4 ? 1 : -1];
typedef char frame_slot_age[offsetof(BHFrameSlot, age) == 8 ? 1 : -1];
static void *__cdecl allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void release(void *memory, void *user)
{
    free(memory);
}
static int fail(BHFrameBuffers *c, BHFrameBufferError error)
{
    if (c)
        c->error = error;
    return 0;
}
int __cdecl bh_frame_buffers_bind(BHFrameBuffers *c, const BHSceneAllocator *a)
{
    if (!c || (a && (!a->allocate || !a->release)))
        return 0;
    memset(c, 0, sizeof(*c));
    c->active = c->latest = -1;
    if (a)
        c->allocator = *a;
    else
    {
        c->allocator.allocate = allocate;
        c->allocator.release = release;
    }
    return 1;
}
int __cdecl bh_frame_buffers_start(BHFrameBuffers *c, BHFrameProduce produce, void *produce_user, double fps,
                                   unsigned long bytes)
{
    void *data[3];
    unsigned long i, j;
    double delay;
    if (!c || !c->allocator.allocate || !c->allocator.release || !produce || !_finite(fps) || fps <= 0 ||
        !bytes || c->writing)
        return fail(c, BH_FRAME_BUFFER_ARGUMENT);
    for (i = 0; i < 3; ++i)
        if (c->slots[i].data)
            return fail(c, BH_FRAME_BUFFER_BUSY);
    delay = 1000.0 / fps;
    if (delay < 1.0)
        delay = 1.0;
    if (!_finite(delay))
        return fail(c, BH_FRAME_BUFFER_ARGUMENT);
    for (i = 0; i < 3; ++i)
    {
        data[i] = c->allocator.allocate(bytes, c->allocator.user);
        if (!data[i])
        {
            for (j = 0; j < i; ++j)
                c->allocator.release(data[j], c->allocator.user);
            return fail(c, BH_FRAME_BUFFER_MEMORY);
        }
    }
    c->produce = produce;
    c->produce_user = produce_user;
    c->delay_ms = delay;
    c->bytes = bytes;
    c->active = c->latest = -1;
    for (i = 0; i < 3; ++i)
    {
        c->slots[i].data = data[i];
        c->slots[i].busy = 0;
        c->slots[i].age = 3;
    }
    c->error = BH_FRAME_BUFFER_OK;
    return 1;
}
void *__cdecl bh_frame_buffers_acquire(BHFrameBuffers *c)
{
    unsigned long i;
    long best = -1, age = -1;
    if (!c || !c->bytes)
    {
        fail(c, BH_FRAME_BUFFER_ARGUMENT);
        return NULL;
    }
    if (c->writing)
    {
        fail(c, BH_FRAME_BUFFER_BUSY);
        return NULL;
    }
    for (i = 0; i < 3; ++i)
        if (!c->slots[i].busy && c->slots[i].age > age)
        {
            best = (long)i;
            age = c->slots[i].age;
        }
    if (best < 0)
    {
        fail(c, BH_FRAME_BUFFER_EMPTY);
        return NULL;
    }
    c->active = best;
    c->slots[best].busy = 1;
    c->slots[best].age = -1;
    c->writing = 1;
    c->error = BH_FRAME_BUFFER_OK;
    return c->slots[best].data;
}
int __cdecl bh_frame_buffers_publish(BHFrameBuffers *c)
{
    unsigned long i, bits;
    if (!c || !c->writing || c->active < 0 || c->active > 2)
        return fail(c, BH_FRAME_BUFFER_ARGUMENT);
    for (i = 0; i < 3; ++i)
    {
        bits = (unsigned long)c->slots[i].age + 1;
        memcpy(&c->slots[i].age, &bits, 4);
    }
    c->slots[c->active].busy = 0;
    c->latest = c->active;
    c->writing = 0;
    c->error = BH_FRAME_BUFFER_OK;
    return 1;
}
int __cdecl bh_frame_buffers_dispose(BHFrameBuffers *c)
{
    unsigned long i;
    if (!c || c->writing)
        return fail(c, BH_FRAME_BUFFER_BUSY);
    if (c->allocator.release)
        for (i = 0; i < 3; ++i)
            if (c->slots[i].data)
            {
                c->allocator.release(c->slots[i].data, c->allocator.user);
                c->slots[i].data = NULL;
            }
    c->bytes = 0;
    c->active = c->latest = -1;
    c->error = BH_FRAME_BUFFER_OK;
    return 1;
}
