#ifndef BH_WEAPON_FACTORY_H
#define BH_WEAPON_FACTORY_H
#include "bh_weapon_removal.h"

/* The six original table slots are retained as verified identities. An
   identity is data until its source behavior has a real callable owner. */
typedef struct BHWeaponObjectState
{
    BHWeaponCommonState common; /* original +e8..+117 */
    unsigned long word_118;     /* factory leaves this word alone */
    unsigned long method_11c, method_120, method_124;
    unsigned long method_128, method_12c, method_130;
} BHWeaponObjectState;
typedef struct BHWeaponRecord
{
    BHEntity *entity; /* borrowed from pool */
    unsigned long generation;
    unsigned long constructor_index;
} BHWeaponRecord;
typedef struct BHWeaponRegistry
{
    BHEntityPool *pool;
    BHWeaponRecord slots[10];
} BHWeaponRegistry;
typedef enum BHWeaponFactoryError
{
    BH_WEAPON_FACTORY_OK,
    BH_WEAPON_FACTORY_ARGUMENT,
    BH_WEAPON_FACTORY_POOL,
    BH_WEAPON_FACTORY_BINDING,
    BH_WEAPON_FACTORY_STALE,
    BH_WEAPON_FACTORY_METHOD
} BHWeaponFactoryError;
typedef struct BHWeaponFactory
{
    BHWeaponRegistry registry;
    BHSoundQueue *sound; /* real sound owner for query event17 */
    BHWeaponFactoryError error;
} BHWeaponFactory;

/* One live factory is bound to the pool callback ABI, whose original query
   signature contains no user pointer. Close only after released query owners.
   This explicit source binding is not a recovered global or player constructor. */
int __cdecl bh_weapon_factory_open(BHWeaponFactory *, BHEntityPool *, BHSoundQueue *);
int __cdecl bh_weapon_factory_close(BHWeaponFactory *);
/* Complete initialization effects of 00433ea0, plus a source registry entry.
   The caller owns the returned pool allocation. Index must be 0..9. */
BHEntity *__cdecl bh_weapon_factory_create(BHWeaponFactory *, unsigned int index);
/* Borrowed result valid while pool allocation/generation remains live. */
int __cdecl bh_weapon_registry_resolve(BHWeaponRegistry *, unsigned int selection, BHEntity **entity,
                                       BHWeaponObjectState **state);
int __cdecl bh_weapon_registry_hud(BHWeaponRegistry *, unsigned int selection, BHEntity **entity,
                                   BHWeaponObjectState **state, unsigned long *identity);
/* Explicit result for an unresolved gameplay method. No RET or missing method
   is represented by a successful callable no-op. */
int __cdecl bh_weapon_method_available(unsigned long identity);
/* Dispatch only complete source-owned context-free +12c methods. Other
   identities report METHOD; live storage and generation are checked first. */
int __cdecl bh_weapon_factory_tick(BHWeaponFactory *, unsigned int selection);
BHWeaponObjectState *__cdecl bh_weapon_object_state(BHEntity *);
#endif
