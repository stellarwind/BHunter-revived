#ifndef BH_COLLISION_CACHE_H
#define BH_COLLISION_CACHE_H
#include "bh_bsp.h"
typedef struct BHCollisionCache {
 const BHBsp *bsp; BHSceneAllocator allocator;
 unsigned long face_count,node_count;
 unsigned long *face_flags; BHVec3 *centers;
 float *radii,*squared_radii; BHBspFace *faces;
 BHBspError error;
} BHCollisionCache;
/* Complete successful-path 0044b130 over explicit owners. Zero initialize once;
   dispose before borrowed BSP replacement. A prepared cache is left untouched,
   including mutable face words/flags. Original centers-nonnull lazy guard is
   retained. Finite decoded BSP, nonempty sections, PC53/nearest contract.
   Bounded argument and transactional allocation-failure handling are adapters.
   This owner does not replace existing geometry/query metadata implicitly. */
int __cdecl bh_collision_cache_prepare(BHCollisionCache *cache,const BHBsp *bsp,const BHSceneAllocator *allocator);
void __cdecl bh_collision_cache_dispose(BHCollisionCache *cache);
#endif
