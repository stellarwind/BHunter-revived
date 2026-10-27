#ifndef BH_SCRIPT_DEBRIS_H
#define BH_SCRIPT_DEBRIS_H
#include "bh_entities.h"
#include "bh_draw_queue.h"
typedef enum BHScriptDebrisError
{
    BH_DEBRIS_OK,
    BH_DEBRIS_ARGUMENT,
    BH_DEBRIS_POOL,
    BH_DEBRIS_QUEUE
} BHScriptDebrisError;
typedef struct BHScriptDebrisContext
{
    BHEntityPool *pool;
    BHDrawQueue *queue;
    unsigned long image, origin_tag;
    BHScriptDebrisError error;
} BHScriptDebrisContext;
typedef struct BHScriptDebrisState
{
    float lifetime, angle, spin, width, height;
    unsigned long frame, origin_tag;
} BHScriptDebrisState;
/* Whole00403120 constructor family. Context replaces actual pool, bitmap
   identity004df23c and borrowed raw attribution word004dfb34. Success or
   count0 resets attribution0; pool allocation failure retains it and all prior
   created entities. Each success consumes exactly9 shared CRT random draws.
   Return created count is source ownership observability, not original EAX.
   Source rejects negative/count>65535; PC53/masked arithmetic retained. */
unsigned long __cdecl bh_script_debris_create(BHScriptDebrisContext *, long count, const BHVec3 *, float lifetime,
                                              float speed, float size);
int __cdecl bh_script_debris_read(const BHEntity *, BHScriptDebrisState *);
int __cdecl bh_script_debris_write(BHEntity *, const BHScriptDebrisState *);
void __cdecl bh_script_debris_tick(BHEntity *);            /*00402d20*/
void __cdecl bh_script_debris_refresh(BHEntity *, void *); /*004027f0*/
#endif
