#ifndef BH_WEAPON_PULSE_H
#define BH_WEAPON_PULSE_H
#include "bh_homing_projectile.h"
#include "bh_script_debris.h"
#include "bh_network_transport.h"
#include "bh_weapon_common.h"
typedef struct BHWeaponPulseContext
{
    BHHomingProjectileContext effects;
    BHScriptDebrisContext *debris;
    BHNetworkTransport *transport;
    unsigned long shot_sound, smoke_image, flare_image, trail_image;
    long peers, local_id;
    unsigned short sender;
    unsigned long shots, packets, packet_bytes;
    unsigned char packet[16];
    HRESULT last_send;
    int error;
} BHWeaponPulseContext;
/* Complete0042f7a0 with explicit source global owners. Actual player fields
   +e8,+280 direction,+2a4 recoil and query receiver remain shared entity state;
   this does not recover missing whole player query/constructor/tick behavior.
   All real borrowed callbacks/owners survive call, including source weapon
   cache+104, player query, effect world-response mapping and network transport.
   No gameplay loop/input or provider/session creation occurs. PC53/masked.
   Returns1 for a whole valid call (also odd phase/empty ammo),0 source guard.
   Partial side effects persist on source dependency error. */
int __cdecl bh_weapon_pulse_fire(BHWeaponPulseContext *, BHEntity *weapon, BHEntity *player);
#endif
