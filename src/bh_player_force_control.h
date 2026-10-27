#ifndef BH_PLAYER_FORCE_CONTROL_H
#define BH_PLAYER_FORCE_CONTROL_H
#include "bh_actor_basis_control.h"
#include "bh_physics_step.h"
typedef struct BHPlayerForceControl
{
    BHActorBasisContext *basis;
    BHPhysicsStepBody *body;
    unsigned long *instability_counter; /* actual shared original004dfa90 */
    int error;
} BHPlayerForceControl;
/* Whole00415530, original second argument unused. Borrow the authoritative
   physics body and actual player entity; pose/velocity/acceleration remain in
   that same component. Original control inputs+18c/+190/+194, inertia+78,
   instability+250 and15plumechannels+2bc remain entity-owned. Native input is
   never read. The caller serializes writes and publishes physics normally.
   skip_forces retains smoothing and all eight visual endpoint writes. PC53,
   finite moderate inputs, initialized actual basis and nonzero mass/inertia.
   Uses shared real CRT rand exactly on original instability cadence. */
int __cdecl bh_player_force_control_b(BHPlayerForceControl *, float unused, int skip_forces);
/* Other whole force-control variants00414880,00416120,00416eb0. Positive dt
   is required for the input-coupled variants C/D; each owns its complete
   stabilizer, force selection, smoothing and visual endpoint behavior. */
int __cdecl bh_player_force_control_a(BHPlayerForceControl *, float dt, int skip_forces);
int __cdecl bh_player_force_control_c(BHPlayerForceControl *, float dt, int skip_forces);
int __cdecl bh_player_force_control_d(BHPlayerForceControl *, float dt, int skip_forces);
#endif
