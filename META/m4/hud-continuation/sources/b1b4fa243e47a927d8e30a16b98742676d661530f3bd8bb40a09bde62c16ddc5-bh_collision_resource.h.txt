#ifndef BH_COLLISION_RESOURCE_H
#define BH_COLLISION_RESOURCE_H
#include "bh_collision_model.h"
/* Explicit owner for collision-loader globals004e1050/0049132c. This is
   independent of the rendering-model and texture search policy owners. */
typedef struct BHCollisionResourcePath {char prefix[256];BHBspSearch search;} BHCollisionResourcePath;
/* Complete0044b0a0: copy literal prefix, append missing backslash, choose
   bare-only for empty prefixes or policies outside0..2. No slash/case rewrite.
   Source bounded-input/null guards preserve the entire context on rejection;
   otherwise bytes beyond the new terminator retain their previous values. */
int __cdecl bh_collision_resource_path(BHCollisionResourcePath *,const char *,long policy);
/* Source composition adapter: use this actual configured path/search with
   the complete recovered collision model loader. Model must be initialized
   and disposed through its real existing owner; no resource handle fabricated. */
int __cdecl bh_collision_resource_load(const BHCollisionResourcePath *,
    BHCollisionModel *,const char *name);
#endif
