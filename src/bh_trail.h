#ifndef BH_TRAIL_H
#define BH_TRAIL_H
#include "bh_entities.h"
#include "bh_queue_reserve.h"
typedef struct BHTrailContext
{
    BHEntityPool *pool;
    BHDrawQueue *queue;
    unsigned long emitted, failures;
} BHTrailContext;
typedef struct BHTrailParameters
{
    unsigned long bucket, image, point_word; /* exact original generator+f0 */
    BHEntity *attached;
    float lifetime, decay, interval, node_lifetime, width, width_step, texture_scale;
    BHVec3 drift;
} BHTrailParameters;
/* Zero-count source bindings for the whole callback family. Caller acquires
   generator in the real pool. Context/pool/queue and attachment survive all
   callbacks. Fresh generator only; state occupies original+e8..12b and the
   source-private context bridge at payload+600. A copied owner is invalid.
   Source callbacks use actual query event3, generation, pool/command owners.
   Parameters must be finite; node chains must remain live and acyclic. */
int __cdecl bh_trail_bind(BHTrailContext *, BHEntity *, const BHTrailParameters *);
/* Complete004071b0/004072d0 constructors. node_value/fade_seconds derives
   the per-tick decrement; frequency derives interval1/frequency. Original
   width/drift rates scale by double1/30 before binary32 stores. These source
   guards require finite inputs and positive fade_seconds/frequency. Paired
   construction preserves both independent allocation attempts and returns
   number constructed (0..2); optional outputs borrow live pool entities. */
BHEntity *__cdecl bh_trail_create(BHTrailContext *, unsigned long image, long bucket, unsigned long point_word,
                                  const BHVec3 *position, BHEntity *attached, float lifetime, float fade_seconds,
                                  float frequency, float node_value, float width, float width_rate,
                                  const BHVec3 *velocity, float texture_scale);
unsigned long __cdecl bh_trail_create_pair(BHTrailContext *, unsigned long image, long bucket, unsigned long point_word,
                                           const BHVec3 *position, BHEntity *attached, float lifetime,
                                           float fade_seconds, float frequency, float node_value, float width,
                                           float width_rate, const BHVec3 *velocity, float texture_scale,
                                           BHEntity **first, BHEntity **second);
/* Whole00406eb0 generator,00406e20 node tick,00406d50 path producer. */
void __cdecl bh_trail_generate(BHEntity *);
void __cdecl bh_trail_node_tick(BHEntity *);
void __cdecl bh_trail_refresh(BHEntity *, void *);
#endif
