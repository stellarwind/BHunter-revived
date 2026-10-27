#ifndef BH_PROJECTILE_LIFETIME_H
#define BH_PROJECTILE_LIFETIME_H
#include "bh_entities.h"
#include "bh_sound_queue.h"
typedef struct BHProjectileLifetime
{
    BHEntityPool *pool;
    BHSoundQueue *sound;
    int error;
} BHProjectileLifetime;
typedef struct BHProjectileQueryState
{
    float countdown;
    unsigned long sound, identity;
    BHEntity *target, *secondary;
} BHProjectileQueryState;
typedef struct BHProjectileSyncPacket
{
    long event;
    unsigned long identity, kind, reserved;
    BHVec3 position;
} BHProjectileSyncPacket;
/* Count-zero explicit source binding/field views. Context/pool/sound and
   referenced entities remain borrowed. Installs actual whole query only;
   existing step/refresh remain caller-owned until original event20 cleanup.
   Original projectile construction, movement and rendering are not claimed. */
int __cdecl bh_projectile_lifetime_bind(BHProjectileLifetime *, BHEntity *, const BHProjectileQueryState *);
int __cdecl bh_projectile_query_state(const BHEntity *, BHProjectileQueryState *);
void __cdecl bh_projectile_query(BHEntity *, void *); /*00440810*/
void __cdecl bh_projectile_cleanup(BHEntity *);       /*00440030*/
#endif
