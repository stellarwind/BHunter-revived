#include "bh_camera_queue.h"
#include <string.h>

int bh_camera_queue_pose(BHDrawQueue *queue, const BHVec3 *position, const BHVec3 *angles)
{
    unsigned long tail, index;
    BHDrawCommand *command;
    if (!queue || !queue->commands || !position || !angles || queue->count < 9 || queue->count > queue->capacity)
    {
        if (queue)
            queue->error = BH_DRAW_ARGUMENT;
        return -1;
    }
    if (queue->count == queue->capacity)
    {
        queue->error = BH_DRAW_FULL;
        return 0;
    }
    tail = queue->commands[0].resource;
    if (tail >= queue->count || (queue->commands[tail].next != BH_DRAW_END && queue->commands[tail].next >= queue->count))
    {
        queue->error = BH_DRAW_TOPOLOGY;
        return -1;
    }
    index = queue->count++;
    command = queue->commands + index;
    command->next = queue->commands[tail].next;
    queue->commands[tail].next = index;
    queue->commands[0].resource = index;
    command->kind = BH_DRAW_CAMERA;
    /* Original payload starts at +4, occupying the semantic resource word. */
    memcpy(&command->resource, position, 12);
    memcpy(command->words + 2, angles, 12);
    queue->error = BH_DRAW_OK;
    return 1;
}

int bh_camera_queue_execute(const BHViewConfig *view, BHMainCamera *camera, const BHDrawQueue *queue,
                            const BHDrawCommand *command, unsigned long *next)
{
    float pose[6];
    unsigned long following;
    int ok;
    if (!view || !camera || !queue || !queue->commands || !command || !next || command->kind != BH_DRAW_CAMERA)
        return 0;
    following = command->next;
    if (following != BH_DRAW_END && following >= queue->count)
        return 0;
    memcpy(pose, &command->resource, sizeof(pose));
    ok = bh_main_camera_pose(view, camera, (double)pose[0], (double)pose[1], (double)pose[2], (double)pose[3],
                             (double)pose[4], (double)pose[5]);
    *next = command->next;
    return ok;
}
