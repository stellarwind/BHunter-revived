#ifndef BH_HUD_STATUS_H
#define BH_HUD_STATUS_H
/* Shared observed bindings for the complete world/flat status HUD owners. */
typedef struct BHHudWeaponAmmo {
    unsigned char opaque[0x10c];
    long amount;
} BHHudWeaponAmmo;
/* Explicit bindings, not an original compact player layout. Original fields:
   gauge_a +24c, gauge_b +23c, selected bytes +2b6/+2b7, flags +2b8,
   signed counters +230/+234, pending +238, mutable countdowns +2b9/+2ba.
   weapons is the borrowed pointer sequence at player+258; selected indices
   are below its actual supplied count. Selected weapon objects supply signed
   amount at +10c. Table/objects/state survive synchronous calls; they must
   not overlap the draw queue. This view supplies no weapon callbacks or
   constructors and owns no weapon allocation/projectile lifetime. */
typedef struct BHHudStatusState {
    float gauge_a,gauge_b;
    const BHHudWeaponAmmo *const *weapons;
    unsigned long weapon_count;
    unsigned char primary,secondary,flags,countdown_a,countdown_b;
    long counter_a,counter_b,pending;
} BHHudStatusState;
#endif
