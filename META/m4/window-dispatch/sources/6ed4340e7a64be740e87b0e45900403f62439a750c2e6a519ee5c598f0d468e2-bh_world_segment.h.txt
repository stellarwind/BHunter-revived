#ifndef BH_WORLD_SEGMENT_H
#define BH_WORLD_SEGMENT_H
#include "bh_world_query.h"
#include "bh_segment_query.h"

/* Borrowed bindings replace original model registry, room +128 lists, and
   ordered NULL-terminated actor array004df278. Query ownership/lifetime stays
   with caller. NULL query is the original absent collision-model case. Actors
   have common kind0, as initialized by the recovered entity pool/acquire owner;
   nonzero common kinds designate world objects, supplied through rooms. */
typedef struct BHWorldSegmentRoom {
    BHWorldGeometry *world;
    BHSegmentQuery *query;
    BHEntity *actors;
    unsigned long kind; /* original common object+8, for camera blocker owner */
} BHWorldSegmentRoom;
typedef struct BHWorldSegmentActor {BHEntity *entity;BHSegmentQuery *query;} BHWorldSegmentActor;
typedef enum BHWorldSegmentError {
    BH_WORLD_SEGMENT_OK,BH_WORLD_SEGMENT_ARGUMENT,BH_WORLD_SEGMENT_TOPOLOGY,
    BH_WORLD_SEGMENT_DEPENDENCY,BH_WORLD_SEGMENT_BUSY
} BHWorldSegmentError;
typedef struct BHWorldSegmentContext {
    BHWorldGeometry *first;
    unsigned long world_count;
    BHWorldSegmentRoom *rooms;
    unsigned long room_count;
    const BHWorldSegmentActor *actors;
    unsigned long actor_count,max_room_actors;
    BHWorldSegmentError error;
    int busy;
} BHWorldSegmentContext;
typedef struct BHWorldSegmentHit {
    BHWorldHit object;
    float fraction;
    BHVec3 point,normal;
    long material_flags; /* original+2c: geometry flags via signed32->float->ftol */
} BHWorldSegmentHit;

/* Source binding adapter; all worlds must occur exactly once in rooms and the
   source scene ring. Does not create callbacks/models/entities. */
int __cdecl bh_world_segment_init(BHWorldSegmentContext *,BHWorldGeometry *first,
    unsigned long world_count,BHWorldSegmentRoom *rooms,unsigned long room_count,
    const BHWorldSegmentActor *actors,unsigned long actor_count,unsigned long max_room_actors);
/* Complete0040c4a0, owning0040ba40 and0040b020. Returns1 hit,0 miss,-1 invalid.
   Prior room cache is retained when lookup fails, as in original. A NULL cache
   with no containing room returns before global actors. Cache is updated once
   before portal traversal. On miss only material_flags is cleared. On error
   caller output/cache remain unchanged; real callbacks may already have run.
   Room actor query callbacks must handle event3 and populate its position;
   successor at actor+f4 is read after callback. No callback is fabricated.
   Geometry bindings, entity lifetimes and topology remain valid during query.
   All inputs finite; actor bases satisfy bh_mat34_inverse_rigid's orthonormal
   contract. Caller uses masked PC53/nearest. No allocation occurs. */
int __cdecl bh_world_segment_run(BHWorldSegmentContext *,const BHWorldHit *self,
    BHWorldGeometry **cached,const BHVec3 *start,const BHVec3 *end,float radius,
    BHWorldSegmentHit *out);

typedef struct BHCameraCorrectionOwner {
    BHWorldSegmentContext *query;
    BHWorldGeometry *cached,*selected;
    BHWorldSegmentError error;
} BHCameraCorrectionOwner;
/* Complete0041e6b0. Owner fields replace globals004dfaa0/004df274. Returns1
   for a valid correction/miss,0 for dependency failure. Eye and target do not
   overlap. A source dependency error restores eye/cache/selected; successful
   writes retain original first-hit publication order. No global state used. */
int __cdecl bh_camera_correct(BHCameraCorrectionOwner *,BHVec3 *eye,
                              const BHVec3 *target,BHEntity *self);
/* Real contextual camera callback bridge. actor_token is actual x86 actor
   pointer supplied by camera update; user is BHCameraCorrectionOwner*. */
void __cdecl bh_camera_correct_callback(BHVec3 *,const BHVec3 *,unsigned long actor_token,void *user);
#endif
