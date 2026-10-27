#ifndef BH_WEAPON_FIRE_H
#define BH_WEAPON_FIRE_H
#include "bh_weapon_common.h"
#include "bh_projectile_create.h"
#include "bh_area_projectile.h"
#include "bh_messages.h"
#include "bh_network_transport.h"
typedef struct BHWeaponFireContext
{
    BHProjectileCreateContext *projectiles;
    BHAreaProjectileContext *area;
    BHSoundQueue *sound;
    BHNetworkTransport *transport;
    BHMessageState *messages;
    unsigned long *empty_warning;
    BHEntity *local, *remote[8];
    long local_id;
    unsigned short sender;
    unsigned long peers, ballistic_sound, homing_sound, empty_sound, area_sound;
    unsigned long shots, bursts, packets, empty_cues;
    unsigned char packet[272];
    unsigned long packet_bytes;
    int error;
} BHWeaponFireContext;
/* Count-zero shared original firing packet and empty-cue source helpers. */
void __cdecl bh_weapon_fire_packet(BHWeaponFireContext *, unsigned char, unsigned char, unsigned char, BHEntity *);
int __cdecl bh_weapon_fire_empty(BHWeaponFireContext *, const BHEntity *);
/* Whole actual original table methods. Initialized borrowed contexts retain
   shared identity, live projectile factories, source voice and packet owners.
   No playback, game loop or actual network session required by these owners. */
int __cdecl bh_weapon_fire_ballistic(BHWeaponFireContext *, BHEntity *, BHEntity *); /*004305f0*/
int __cdecl bh_weapon_fire_homing(BHWeaponFireContext *, BHEntity *, BHEntity *);    /*00430a30*/
int __cdecl bh_weapon_area_start(BHWeaponFireContext *, BHEntity *, BHEntity *);     /*00431db0*/
int __cdecl bh_weapon_area_tick(BHWeaponFireContext *, BHEntity *, BHEntity *);      /*00431f00*/
#endif
