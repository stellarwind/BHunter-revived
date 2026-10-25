#ifndef BH_PLAYER_SCENE_H
#define BH_PLAYER_SCENE_H
#include "bh_player_camera.h"
#include "bh_hud_markers.h"
#include "bh_hud_world.h"
#include "bh_hud_flat.h"
#include "bh_hud_scores.h"
#include "bh_scene_background.h"
#include "bh_scene_owner.h"
#include "bh_weapon_hud_arc.h"
#include "bh_weapon_hud_target.h"
#include "bh_weapon_factory.h"
#include "bh_sound_queue.h"
#include "bh_fade.h"

typedef enum BHPlayerSceneError {
    BH_PLAYER_SCENE_OK,BH_PLAYER_SCENE_ARGUMENT,BH_PLAYER_SCENE_BINDING,
    BH_PLAYER_SCENE_WEAPON,BH_PLAYER_SCENE_OWNER,BH_PLAYER_SCENE_QUEUE
} BHPlayerSceneError;

/* Every field is borrowed from an existing source owner. The scene callback
   never constructs player, weapon, image, scene or renderer storage. The
   pointers to mutable globals remain live across synchronous callbacks. */
typedef struct BHPlayerScene {
    BHEntity *player;
    BHDrawQueue *queue;
    BHSoundQueue *sound;
    BHHudContext *hud;
    BHHudMarkers *markers;
    BHRadarContext *radar;
    BHHudWorldContext *world_status;
    BHHudFlatContext *flat_status;
    BHHudScoreDraw *score_draw;
    BHHudScores *scores;
    BHSceneBackground *background;
    BHSceneOwner *scene;
    BHWeaponRegistry *weapons;
    BHArcContext *arc;
    BHWeaponHudTargetContext *target;
    BHEntity **actor_head;
    BHEntity **pickup_head;
    const long *mode_count,*frame_count,*display_width,*display_height;
    const long *confirmation_gate;
    unsigned long material_image,horizon_image,aim_image,aim_overlay_image;
    unsigned long particle_image,confirmation_image,trail_image,overlay_image;
    unsigned long cockpit_resource;
    unsigned long pickup_limit;
    /* random_calls counts this owner's direct draws plus arc draws. The
       background owner also consumes the shared CRT stream internally. */
    unsigned long attempted,queued,dropped,random_calls;
    BHPlayerSceneError error;
} BHPlayerScene;

/* Complete 0041c860 source callback, including the independently gated tail.
   Returns 1 for a completed callback (queue-full drops included), 0 for an
   invalid/missing live binding or source dependency error. Earlier mutations
   and queue publications are retained on later failures. The caller owns the
   VC5 CRT random stream and PC53 nearest/masked floating-point profile.
   Nonfinite camera pose is outside the source renderer's supported domain and
   rejects before queue mutation; original behavior there remains unverified. */
int __cdecl bh_player_scene_draw(BHPlayerScene *);
#endif
