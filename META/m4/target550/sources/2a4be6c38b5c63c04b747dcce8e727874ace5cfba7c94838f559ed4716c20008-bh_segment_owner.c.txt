#include "bh_segment_owner.h"
#include <string.h>
int __cdecl bh_segment_owner_dispose(BHSegmentOwner *owner) {
    if(!owner || owner->query.busy)return 0;
    if(!bh_segment_query_dispose(&owner->query))return 0;
    bh_collision_cache_dispose(&owner->cache);
    memset(&owner->geometry,0,sizeof(owner->geometry));return 1;
}
int __cdecl bh_segment_owner_query(BHSegmentOwner *owner,const BHBsp *bsp,
    const BHVec3 *start,const BHVec3 *end,const BHMat34 *world,
    const BHMat34 *local,BHSegmentHit *result) {
    if(!bsp)return 0;
    if(!owner || !start || !end || !world || !local || !result || owner->query.busy)return -1;
    if(!bh_collision_cache_prepare(&owner->cache,bsp,NULL))return -1;
    if(!owner->query.frames) {
        owner->geometry.bsp=bsp;owner->geometry.face_flags=owner->cache.face_flags;
        owner->geometry.allocator=owner->cache.allocator;
        if(!bh_segment_query_init(&owner->query,&owner->geometry))return -1;
    }
    return bh_segment_query_run(&owner->query,start,end,world,local,result);
}
