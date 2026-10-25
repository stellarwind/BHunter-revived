#ifndef BH_PLAYER_CAMERA_H
#define BH_PLAYER_CAMERA_H
#include "bh_entities.h"
#include "bh_camera_state.h"

/* Exact32-byte start-pose table entry at original004df6c0. The final two
   words belong to other level/spawn consumers and reset preserves them. */
typedef struct BHPlayerCameraStart {
    BHVec3 position,angles;
    unsigned long metadata[2];
} BHPlayerCameraStart;
/* Explicit borrowed equivalents of global focus, start table, player count,
   local player index and shared camera callback state. The player must use
   the original x86 BHEntity layout and own its embedded camera at+e8.
   starts and camera must not overlap player. */
typedef struct BHPlayerCameraContext {
    BHEntity *player;
    const BHPlayerCameraStart *starts;
    unsigned long start_count;
    long player_count,local_player_index;
    BHCameraStateContext *camera;
} BHPlayerCameraContext;

/* Source field binding adapter; no original-function recovery claim. */
BHCameraState *__cdecl bh_player_camera_state(BHEntity *player);
/* Complete004221e0. Relocates actor and three camera eye histories, resets
   four motion vectors, restores start angles/basis/matrix, switches mode1,
   then performs50 synchronous source camera updates at1/30 second.
   These are pure source state-settling calls, not execution of gameplay or
   a frame/controller loop. Finally copies position to+298 and zeros+2a4.
   Caller supplies initialized camera/mode storage and finite start poses.
   Source bounds/nonfinite/binding guards reject before actor mutation;
   callback side effects retain their original synchronous semantics. */
int __cdecl bh_player_camera_reset(BHPlayerCameraContext *,unsigned long index);
#endif
