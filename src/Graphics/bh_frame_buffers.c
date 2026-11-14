#include "bh_frame_buffers.h"
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
static_assert(sizeof(BHFrameSlot) == 12, "BHFrameSlot size");
static_assert(offsetof(BHFrameSlot, busy) == 4, "BHFrameSlot.busy offset");
static_assert(offsetof(BHFrameSlot, age) == 8, "BHFrameSlot.age offset");
static void *allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void release(void *memory, void *user)
{
    free(memory);
}
static int fail(BHFrameBuffers *frame_buffers, BHFrameBufferError error)
{
    if (frame_buffers)
        frame_buffers->error = error;
    return 0;
}
int bh_frame_buffers_bind(BHFrameBuffers *frame_buffers, const BHSceneAllocator *scene_allocator)
{
    if (!frame_buffers || (scene_allocator && (!scene_allocator->allocate || !scene_allocator->release)))
        return 0;
    memset(frame_buffers, 0, sizeof(*frame_buffers));
    frame_buffers->active = frame_buffers->latest = -1;
    if (scene_allocator)
        frame_buffers->allocator = *scene_allocator;
    else
    {
        frame_buffers->allocator.allocate = allocate;
        frame_buffers->allocator.release = release;
    }
    return 1;
}
int bh_frame_buffers_start(BHFrameBuffers *frame_buffers, BHFrameProduce produce, void *produce_user, double fps,
                           unsigned long bytes)
{
    void *data[3];
    unsigned long i, j;
    double delay;
    if (!frame_buffers || !frame_buffers->allocator.allocate || !frame_buffers->allocator.release || !produce || !_finite(fps) || fps <= 0 || !bytes ||
        frame_buffers->writing)
        return fail(frame_buffers, BH_FRAME_BUFFER_ARGUMENT);
    for (i = 0; i < 3; ++i)
        if (frame_buffers->slots[i].data)
            return fail(frame_buffers, BH_FRAME_BUFFER_BUSY);
    delay = 1000.0 / fps;
    if (delay < 1.0)
        delay = 1.0;
    if (!_finite(delay))
        return fail(frame_buffers, BH_FRAME_BUFFER_ARGUMENT);
    for (i = 0; i < 3; ++i)
    {
        data[i] = frame_buffers->allocator.allocate(bytes, frame_buffers->allocator.user);
        if (!data[i])
        {
            for (j = 0; j < i; ++j)
                frame_buffers->allocator.release(data[j], frame_buffers->allocator.user);
            return fail(frame_buffers, BH_FRAME_BUFFER_MEMORY);
        }
    }
    frame_buffers->produce = produce;
    frame_buffers->produce_user = produce_user;
    frame_buffers->delay_ms = delay;
    frame_buffers->bytes = bytes;
    frame_buffers->active = frame_buffers->latest = -1;
    for (i = 0; i < 3; ++i)
    {
        frame_buffers->slots[i].data = data[i];
        frame_buffers->slots[i].busy = 0;
        frame_buffers->slots[i].age = 3;
    }
    frame_buffers->error = BH_FRAME_BUFFER_OK;
    return 1;
}
void *bh_frame_buffers_acquire(BHFrameBuffers *frame_buffers)
{
    unsigned long i;
    long best = -1, age = -1;
    if (!frame_buffers || !frame_buffers->bytes)
    {
        fail(frame_buffers, BH_FRAME_BUFFER_ARGUMENT);
        return NULL;
    }
    if (frame_buffers->writing)
    {
        fail(frame_buffers, BH_FRAME_BUFFER_BUSY);
        return NULL;
    }
    for (i = 0; i < 3; ++i)
        if (!frame_buffers->slots[i].busy && frame_buffers->slots[i].age > age)
        {
            best = (long)i;
            age = frame_buffers->slots[i].age;
        }
    if (best < 0)
    {
        fail(frame_buffers, BH_FRAME_BUFFER_EMPTY);
        return NULL;
    }
    frame_buffers->active = best;
    frame_buffers->slots[best].busy = 1;
    frame_buffers->slots[best].age = -1;
    frame_buffers->writing = 1;
    frame_buffers->error = BH_FRAME_BUFFER_OK;
    return frame_buffers->slots[best].data;
}
int bh_frame_buffers_publish(BHFrameBuffers *frame_buffers)
{
    unsigned long i, bits;
    if (!frame_buffers || !frame_buffers->writing || frame_buffers->active < 0 || frame_buffers->active > 2)
        return fail(frame_buffers, BH_FRAME_BUFFER_ARGUMENT);
    for (i = 0; i < 3; ++i)
    {
        bits = (unsigned long)frame_buffers->slots[i].age + 1;
        memcpy(&frame_buffers->slots[i].age, &bits, 4);
    }
    frame_buffers->slots[frame_buffers->active].busy = 0;
    frame_buffers->latest = frame_buffers->active;
    frame_buffers->writing = 0;
    frame_buffers->error = BH_FRAME_BUFFER_OK;
    return 1;
}
int bh_frame_buffers_dispose(BHFrameBuffers *frame_buffers)
{
    unsigned long i;
    if (!frame_buffers || frame_buffers->writing)
        return fail(frame_buffers, BH_FRAME_BUFFER_BUSY);
    if (frame_buffers->allocator.release)
        for (i = 0; i < 3; ++i)
            if (frame_buffers->slots[i].data)
            {
                frame_buffers->allocator.release(frame_buffers->slots[i].data, frame_buffers->allocator.user);
                frame_buffers->slots[i].data = NULL;
            }
    frame_buffers->bytes = 0;
    frame_buffers->active = frame_buffers->latest = -1;
    frame_buffers->error = BH_FRAME_BUFFER_OK;
    return 1;
}
