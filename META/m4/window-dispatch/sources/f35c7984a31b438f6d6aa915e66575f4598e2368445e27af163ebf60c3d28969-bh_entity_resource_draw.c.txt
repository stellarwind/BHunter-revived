#include "bh_entity_resource_draw.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

typedef char payload_phase_offset[offsetof(BHEntity,payload)+0x18==0xfc ? 1:-1];
typedef char payload_resource_offset[offsetof(BHEntity,payload)+0x30==0x114 ? 1:-1];
typedef char position_offset[offsetof(BHEntity,position)==0x0c ? 1:-1];

static void read_payload(const BHEntity *entity,unsigned long offset,void *out,
    unsigned long length) {
    memcpy(out,entity->payload+offset,length);
}
int __cdecl bh_entity_resource_draw_special(BHEntityResourceDrawContext *context,
    BHEntity *entity) {
    BHVec3 angles,first,second;
    unsigned long handle;
    float phase,glow_size,glow_alpha;
    double oscillation;
    int first_result,second_result,billboard_result;
    if(!context || !context->queue || !entity) return -1;
    read_payload(entity,0x18,&phase,sizeof(phase));
    read_payload(entity,0x30,&handle,sizeof(handle));
    /* The original stores each pose component to binary32 before enqueueing.
       Both calls copy their pose, then the second overwrites entity angles. */
    angles.x=phase+phase;
    angles.z=0.0f;
    angles.y=(float)(sin((double)phase*0.87)*0.3);
    memcpy(entity->opaque0+0x24,&angles,sizeof(angles));
    first=entity->position;
    first.x=(float)((double)entity->position.x-0.05);
    first_result=bh_draw_submit_resource(context->queue,3,handle,&first,&angles,0.35f);

    second=first;
    second.x=(float)((double)first.x-(-0.1));
    angles.x=(float)((double)phase*2.74);
    angles.y=(float)(sin((double)phase*1.1919000000000002)*0.3);
    memcpy(entity->opaque0+0x24,&angles,sizeof(angles));
    second_result=bh_draw_submit_resource(context->queue,3,handle,&second,&angles,0.35f);

    oscillation=sin((double)phase*5.17);
    glow_size=(float)(oscillation-(-6.0));
    glow_alpha=(float)(0.5-oscillation*(-0.25));
    billboard_result=bh_draw_billboard(context->queue,3,context->glow_image,
        &entity->position,glow_size,glow_size,0.0f,0,glow_alpha);
    if(first_result<0 || second_result<0 || billboard_result<0) return -1;
    return first_result && second_result && billboard_result;
}
