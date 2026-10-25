#include "bh_camera_queue.h"
#include <string.h>

int __cdecl bh_camera_queue_pose(BHDrawQueue *q,const BHVec3 *position,const BHVec3 *angles) {
    unsigned long tail,index;BHDrawCommand *command;
    if(!q || !q->commands || !position || !angles || q->count<9 || q->count>q->capacity) {
        if(q)q->error=BH_DRAW_ARGUMENT;return -1;
    }
    if(q->count==q->capacity){q->error=BH_DRAW_FULL;return 0;}
    tail=q->commands[0].resource;
    if(tail>=q->count || (q->commands[tail].next!=BH_DRAW_END && q->commands[tail].next>=q->count)) {
        q->error=BH_DRAW_TOPOLOGY;return -1;
    }
    index=q->count++;command=q->commands+index;
    command->next=q->commands[tail].next;
    q->commands[tail].next=index;q->commands[0].resource=index;
    command->kind=BH_DRAW_CAMERA;
    /* Original payload starts at +4, occupying the semantic resource word. */
    memcpy(&command->resource,position,12);
    memcpy(command->words+2,angles,12);
    q->error=BH_DRAW_OK;return 1;
}

int __cdecl bh_camera_queue_execute(const BHViewConfig *view,BHMainCamera *camera,
    const BHDrawQueue *q,const BHDrawCommand *command,unsigned long *next) {
    float pose[6];unsigned long following;int ok;
    if(!view || !camera || !q || !q->commands || !command || !next ||
       command->kind!=BH_DRAW_CAMERA)return 0;
    following=command->next;
    if(following!=BH_DRAW_END && following>=q->count)return 0;
    memcpy(pose,&command->resource,sizeof(pose));
    ok=bh_main_camera_pose(view,camera,(double)pose[0],(double)pose[1],(double)pose[2],
        (double)pose[3],(double)pose[4],(double)pose[5]);
    *next=command->next;return ok;
}
