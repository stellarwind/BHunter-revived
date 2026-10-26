#ifndef BH_BALLISTIC_PROJECTILE_H
#define BH_BALLISTIC_PROJECTILE_H
#include "bh_homing_projectile.h"
/* Shared borrowed effect/world bindings retain their own actual original
   identities. first_image is original4f0090; second_image is4f0030 here.
   hit_sound is4f5564, liquid_sound4f54fc; ricochet_sound is4f550c. */
typedef struct BHBallisticProjectileContext {
 BHHomingProjectileContext bindings;
 unsigned long ricochet_sound,bounces;
} BHBallisticProjectileContext;
/* Count-zero binding into actual acquired entity. Same lifetime contracts as
   homing bridge, original fields retained, no projectile constructor claim.
   Optional actual projectile query/lifetime context remains unchanged. */
int __cdecl bh_ballistic_projectile_bind(BHBallisticProjectileContext *,BHEntity *);
void __cdecl bh_ballistic_projectile_tick(BHEntity *); /*whole0043ea90*/
#endif
