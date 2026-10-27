#ifndef BH_MODEL_SCALE_H
#define BH_MODEL_SCALE_H
#include "bh_model_draw.h"
#include <limits.h>
/* Complete00470d50: reverse vertex/UV payloads along count edge slots while
   retaining every next link; return the following edge. Count0 returns first.
   LONG_MIN reports guarded invalid count/index, but can also be a legitimate
   negative following-edge value. Validate inputs independently when status
   matters (the scale owner preflights and ignores this return). Model storage
   is borrowed; any negative chain terminator is permitted. */
long __cdecl bh_model_reverse_edges(BHModel *, long first, long count);
/* Complete00470e00. Nonzero model.frame is a complete successful no-op.
   Every associated source draw cache MUST appear in caches; no global cache
   registry exists. Cold zero caches are permitted; duplicate/foreign caches
   are rejected before mutation. All real cache allocations are disposed via
   their owner; reinitialize each cache before drawing again. Opaque original
   runtime words are zeroed, never cast/released as pointers. Model arrays,
   allocator, texture ownership and transform retain their existing lifetime.
   Caller owns PC53/nearest/masked FP profile. Zero/infinity/NaN scales retain
   original IEEE behavior and unordered reflection branches; NaN payload/sign
   and PC24 are not promised. Source guards require bounded valid arrays and
   a root node; reflected face chains must terminate within edge_count slots.
   Returns0 on invalid source binding, with no model/cache mutation. */
int __cdecl bh_model_scale(BHModel *, float sx, float sy, float sz, BHModelDrawCache *const *caches,
                           unsigned long cache_count);
#endif
