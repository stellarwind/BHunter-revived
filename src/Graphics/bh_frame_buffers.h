#ifndef BH_FRAME_BUFFERS_H
#define BH_FRAME_BUFFERS_H
#include "../World/bh_scene.h"
typedef void (*BHFrameProduce)(void *);
typedef struct BHFrameSlot
{
    void *data;
    long busy, age;
} BHFrameSlot;
typedef enum BHFrameBufferError
{
    BH_FRAME_BUFFER_OK,
    BH_FRAME_BUFFER_ARGUMENT,
    BH_FRAME_BUFFER_MEMORY,
    BH_FRAME_BUFFER_BUSY,
    BH_FRAME_BUFFER_EMPTY
} BHFrameBufferError;
typedef struct BHFrameBuffers
{
    BHFrameSlot slots[3];
    BHSceneAllocator allocator;
    unsigned long bytes;
    long active, latest;
    int writing;
    double delay_ms;
    BHFrameProduce produce;
    void *produce_user;
    BHFrameBufferError error;
} BHFrameBuffers;
/* The SDL main loop owns production, publication, and consumption. */
int bh_frame_buffers_bind(BHFrameBuffers *, const BHSceneAllocator *);
/* Set the frame interval and allocate all three buffers. */
int bh_frame_buffers_start(BHFrameBuffers *, BHFrameProduce, void *, double fps, unsigned long bytes);
/* Select the oldest available slot. Its buffer is borrowed until publication. */
void *bh_frame_buffers_acquire(BHFrameBuffers *);
/* Age the slots and publish the active writer as the latest frame. */
int bh_frame_buffers_publish(BHFrameBuffers *);
/* Release owned buffers when there is no outstanding writer. */
int bh_frame_buffers_dispose(BHFrameBuffers *);
#endif
