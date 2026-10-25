#ifndef BH_DRAW_QUEUE_H
#define BH_DRAW_QUEUE_H
#include "bh_scene.h"
#include "bh_math.h"
/* Source semantic command selectors, not original executable function pointers.
   Primitive consumers/Direct3D submission remain separate unfinished owners. */
typedef enum BHDrawKind {BH_DRAW_NOP,BH_DRAW_STATE,BH_DRAW_BILLBOARD,BH_DRAW_SEGMENT,BH_DRAW_ORIENTED,BH_DRAW_MODEL_RESOURCE,BH_DRAW_RECT,BH_DRAW_TEXTURE_MASK,BH_DRAW_CLIP_RECT,BH_DRAW_CAMERA} BHDrawKind;
typedef struct BHDrawCommand {
    unsigned long kind,resource,words[18],next;
} BHDrawCommand;
typedef enum BHDrawError {BH_DRAW_OK,BH_DRAW_ARGUMENT,BH_DRAW_MEMORY,BH_DRAW_FULL,BH_DRAW_TOPOLOGY} BHDrawError;
typedef struct BHDrawQueue {
    BHDrawCommand *commands;
    unsigned long count,capacity;
    BHSceneAllocator allocator;
    BHDrawError error;
} BHDrawQueue;
#define BH_DRAW_END 0xffffffffUL
/* capacity includes nine original bucket headers. Fresh owner; dispose before
   reinitializing. begin resets headers/count, preserving unused payload words.
   Header next links form one global chain in render order, not nine rings. */
int __cdecl bh_draw_queue_init(BHDrawQueue *queue,unsigned long capacity,const BHSceneAllocator *allocator);
int __cdecl bh_draw_queue_dispose(BHDrawQueue *queue);
int __cdecl bh_draw_queue_begin(BHDrawQueue *queue); /* 00477390 */
/* Return 1 queued, 0 full, -1 invalid. Resource is a borrowed opaque source
   texture identifier; no original texture object or vtable is fabricated.
   Payload floats are copied verbatim, including nonfinite bit patterns. */
int __cdecl bh_draw_billboard(BHDrawQueue *queue,unsigned long bucket,unsigned long resource,
    const BHVec3 *point,float width,float height,float angle,unsigned long frame,float alpha); /* 00478280 */
int __cdecl bh_draw_segment(BHDrawQueue *queue,unsigned long bucket,unsigned long resource,
    const BHVec3 *from,const BHVec3 *to,float first_width,float second_width,
    float first_alpha,float second_alpha,unsigned long control); /* 00478390 */
int __cdecl bh_draw_oriented(BHDrawQueue *queue,unsigned long bucket,unsigned long resource,
    const BHVec3 *point,const BHVec3 *up,const BHVec3 *normal,float width,float height,
    float angle,unsigned long control,float alpha); /* 004785f0 */
#endif
