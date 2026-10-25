#ifndef BH_MATERIAL_QUEUE_H
#define BH_MATERIAL_QUEUE_H
#include "bh_draw_submit.h"
#include "bh_texture_policy.h"
/* Complete 00478060: enqueue seven raw image-policy words. Resource identities
   are borrowed source handles and must stay live until consumer execution.
   Return 1 queued, 0 full, -1 invalid source binding/topology. */
int __cdecl bh_material_queue_words(BHDrawQueue *,unsigned long bucket,
    unsigned long image,unsigned long word0,unsigned long word1,
    unsigned long word2,unsigned long word3,unsigned long word4,
    unsigned long word5,unsigned long word6);
/* Complete 00477fa0, invoking real 0044f510 source owner through lookup.
   Unknown image/missing binding is a source adapter guard, not original behavior.
   The next link is published after the setter, including failed image lookup.
   Lookup must not mutate command; next must not alias command storage. */
int __cdecl bh_material_queue_execute(BHDrawImageLookup,void *,
    const BHDrawCommand *,unsigned long *next);
#endif
