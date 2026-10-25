#include "bh_queue_reserve.h"
#include <stddef.h>
typedef char reserve_stride[sizeof(BHDrawCommand)==84?1:-1];
typedef char reserve_link[offsetof(BHDrawCommand,next)==0x50?1:-1];
BHDrawCommand *__cdecl bh_queue_reserve(BHDrawQueue *q,long bucket) {
    static const unsigned long headers[9]={1,2,3,4,5,0,6,7,8};
    unsigned long header,tail,index;BHDrawCommand *command;
    if(!q || !q->commands || q->count<9 || q->count>q->capacity){if(q)q->error=BH_DRAW_ARGUMENT;return NULL;}
    if(q->count==q->capacity){q->error=BH_DRAW_FULL;return NULL;}
    header=(unsigned long)bucket<9?headers[(unsigned long)bucket]:0;
    tail=q->commands[header].resource;
    if(tail>=q->count || (q->commands[tail].next!=BH_DRAW_END && q->commands[tail].next>=q->count)){q->error=BH_DRAW_TOPOLOGY;return NULL;}
    index=q->count++;command=q->commands+index;command->next=q->commands[tail].next;
    q->commands[tail].next=index;q->commands[header].resource=index;
    q->error=BH_DRAW_OK;return command;
}
int __cdecl bh_queue_mode(BHDrawQueue *q,long bucket,unsigned long mode) {
    BHDrawCommand *command=bh_queue_reserve(q,bucket);
    if(!command)return q && q->error==BH_DRAW_FULL?0:-1;
    command->kind=BH_DRAW_STATE;command->words[0]=mode;return 1;
}
int __cdecl bh_queue_mode_execute(BHRenderState *state,const BHDrawCommand *command,unsigned long *next) {
    int ok;if(!state || !command || !next)return 0;
    ok=bh_render_mode(state,command->words[0]);*next=command->next;return ok;
}
