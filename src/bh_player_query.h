#ifndef BH_PLAYER_QUERY_H
#define BH_PLAYER_QUERY_H
#include "bh_weapon_factory.h"
#include "bh_weapon_target_lifecycle.h"
#include "bh_weapon_arc_lifecycle.h"
#include "bh_script_lifetime.h"
#include "bh_messages.h"
#include "bh_network_transport.h"
#include "bh_physics_response.h"
typedef union BHPlayerQueryPacket {long words[10];float floats[10];} BHPlayerQueryPacket;
typedef struct BHPlayerQueryContext {
 BHEntity *player;BHWeaponRegistry *weapons;BHSoundQueue *sound;
 BHMessageState *messages;BHWeaponReadyContext *ready;
 BHWeaponTargetContext *target;BHWeaponArcLifecycleContext *arc;
 BHScriptLifetimeContext *lifetime;BHNetworkTransport *transport;
 BHEntity **actors;long *fine_warning;BHEntity **damage_source;
 const long *mode_count;const unsigned long *simulation_tick; /*original004dfac4, not a player/DPID identity*/
 BHWorldGeometry *const *world;const unsigned short *world_index;
 unsigned long contact_side,contact_axial,primary_cue,secondary_cue;
 BHVec3 *sound_velocity;
 /* Count-zero contact bridge. The original callback and physics shared entity
    fields; mirror event2 writes into the source physics component before the
    enclosing step finishes/publishes. Borrowed from the real player owner. */
 BHPhysicsState *contact_physics;
 unsigned long calls,random_calls,broadcasts,sound_requests,packets;
 int error;
} BHPlayerQueryContext;
/* Complete0041d7c0 over actual player offsets and ten real registry owners.
   Explicit original-global bindings stay live during synchronous callbacks.
   PC53 nearest/masked; moderate finite physical values and valid pickup
   indices0..9,20,21 required. A priced pickup requires nonnegative price,
   money and count; fine payment requires positive price (original zero-price
   branch divides by zero). Source rejects invalid packets before mutation.
   No player construction/tick claim. Binding/side-effect errors preserve prior
   effects, and verified RET methods are represented by no operation. */
int __cdecl bh_player_query_dispatch(BHPlayerQueryContext *,BHPlayerQueryPacket *);
/* Count-zero install/uninstall. One live player context per process; its real
   entity, pool and context must outlive installed callback. */
int __cdecl bh_player_query_bind(BHPlayerQueryContext *);
int __cdecl bh_player_query_unbind(BHPlayerQueryContext *);
void __cdecl bh_player_query(BHEntity *,void *);
#endif
