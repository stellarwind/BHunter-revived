#ifndef BH_WEAPON_HUD_ARC_H
#define BH_WEAPON_HUD_ARC_H
#include "bh_weapon_hud_state.h"
#include "bh_queue_reserve.h"

typedef enum BHArcError {BH_ARC_OK,BH_ARC_ARGUMENT,BH_ARC_NUMERICAL,BH_ARC_CALLBACK,BH_ARC_QUEUE} BHArcError;
typedef struct BHArcContext {
    BHDrawQueue *queue;
    unsigned long image;
    unsigned long attempted,queued,dropped,path_points,random_calls;
    BHArcError error;
} BHArcContext;
/* Observed player fields +0c position,+30 forward,+280 aim direction. These
   explicit source bindings do not assert a compact original player layout. */
typedef struct BHWeaponHudArcPlayer {BHVec3 position,forward,aim;} BHWeaponHudArcPlayer;

/* Complete00432050: cubic smoothstep blend of two interpolated control lines,
   initial point plus16progress steps, original random jitter/call ordering,
   queue exhaustion and borrowed image header publication. Strength>1 clamps1;
   source finite-input guard rejects negative strength before sqrt/publication.
   Reuses linked VC5 CRT rand (RAND_MAX32767), including global stream state;
   no replacement random controller. Caller owns PC53/nearest/masked profile.
   Context/vectors/queue may not overlap, and image outlives queued consumption.
   Header.kind PATH_HEADER/resource=image/words0=successful pointcount. Points:
   resource=floatXbits; words0=Y,1=Z,2=width,3=texture start,4=texture end,
   5=modulation,6=random tile. Unused payload remains unchanged, as original.
   Full queues drop individual attempted points; failure cannot undo a header.
   The separately recovered path queue consumer owns drawing these commands. */
int __cdecl bh_arc_queue(BHArcContext *,const BHVec3 *start,const BHVec3 *control,
    const BHVec3 *target_control,const BHVec3 *target,float strength);
/* Complete375B primary weapon-index4 HUD00432ff0. Capture player control points
   before target's installed event3 query; preserve query effects and read charge
   afterward. Three CRT random draws jitter the target control even if queuefull,
   then charge/48 is supplied to actual00432050 producer. NULL target is a no-op.
   No weapon constructor/target query callback is fabricated or counted. Target,
   state and player survive synchronous query; mutation is allowed but freeing
   them during the callback is outside the original lifetime contract. */
int __cdecl bh_weapon_hud_arc(BHArcContext *,BHWeaponHudState *,const BHWeaponHudArcPlayer *);
#endif
