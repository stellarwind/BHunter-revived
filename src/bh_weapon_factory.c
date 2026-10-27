#include "bh_weapon_factory.h"
#include "bh_weapon_timer.h"
#include "bh_weapon_latch.h"
#include <stddef.h>
#include <string.h>

typedef char weapon_object_size[sizeof(BHWeaponObjectState) == 0x4c ? 1 : -1];
typedef char weapon_object_method_offset[offsetof(BHWeaponObjectState, method_11c) == 0x34 ? 1 : -1];
typedef char weapon_object_hud_offset[offsetof(BHWeaponObjectState, method_130) == 0x48 ? 1 : -1];
typedef char weapon_entity_fit[sizeof(((BHEntity *)0)->payload) >= 0x50 ? 1 : -1];

/* The addresses identify the actual six entries of each PE table record at
   0048fa60. They are not executable pointers in this source-built target. */
static const unsigned long methods[10][6] = {
    {0x004309b0UL, 0x0047d680UL, 0x0042f730UL, 0x004305f0UL, 0x00431050UL, 0x0047d680UL},
    {0x004309b0UL, 0x0047d680UL, 0x0042f730UL, 0x0042f7a0UL, 0x00431050UL, 0x0047d680UL},
    {0x00430d80UL, 0x0047d680UL, 0x004309d0UL, 0x00430a30UL, 0x00431050UL, 0x00431130UL},
    {0x00433170UL, 0x0047d680UL, 0x004331c0UL, 0x0047d680UL, 0x00431050UL, 0x0047d680UL},
    {0x00432c60UL, 0x00432ff0UL, 0x00432560UL, 0x004327a0UL, 0x004324a0UL, 0x00432510UL},
    {0x00431180UL, 0x004314f0UL, 0x00431810UL, 0x0047d680UL, 0x004310c0UL, 0x00431130UL},
    {0x00431f00UL, 0x0047d680UL, 0x00431db0UL, 0x0047d680UL, 0x00431050UL, 0x0047d680UL},
    {0x00433b00UL, 0x0047d680UL, 0x00433b30UL, 0x0047d680UL, 0x004310c0UL, 0x0047d680UL},
    {0x00433820UL, 0x0047d680UL, 0x00433850UL, 0x0047d680UL, 0x004310c0UL, 0x0047d680UL},
    {0x0047d680UL, 0x0047d680UL, 0x00433dc0UL, 0x0047d680UL, 0x004310c0UL, 0x0047d680UL}};
static const long ammo_limits[10] = {750, 400, 150, 50, 300, 15, 10, 4, 1, 8};
static BHWeaponFactory *active_factory;

BHWeaponObjectState *__cdecl bh_weapon_object_state(BHEntity *entity)
{
    return entity ? (BHWeaponObjectState *)(entity->payload + 4) : NULL;
}
static int live(const BHWeaponRegistry *registry, const BHWeaponRecord *record)
{
    BHEntity *cursor;
    unsigned long count;
    if (!registry || !registry->pool || !record->entity)
        return 0;
    cursor = registry->pool->active;
    count = 0;
    while (cursor && count++ < registry->pool->used)
    {
        if (cursor == record->entity)
            return cursor->generation == record->generation;
        cursor = cursor->next;
    }
    return 0;
}
static void __cdecl weapon_query(BHEntity *entity, void *packet)
{
    BHWeaponFactory *factory = active_factory;
    unsigned int i;
    BHWeaponRecord *record = NULL;
    if (!factory || !entity || !packet)
        return;
    for (i = 0; i < 10; i++)
        if (factory->registry.slots[i].entity == entity && factory->registry.slots[i].generation == entity->generation)
        {
            record = &factory->registry.slots[i];
            break;
        }
    if (!record || !live(&factory->registry, record))
    {
        factory->error = BH_WEAPON_FACTORY_STALE;
        return;
    }
    if (!bh_weapon_target_removed(factory->sound, entity, (const BHWeaponRemovedPacket *)packet))
        factory->error = BH_WEAPON_FACTORY_BINDING;
}
int __cdecl bh_weapon_factory_open(BHWeaponFactory *factory, BHEntityPool *pool, BHSoundQueue *sound)
{
    if (!factory || !pool || !sound || active_factory)
        return 0;
    memset(factory, 0, sizeof(*factory));
    factory->registry.pool = pool;
    factory->sound = sound;
    active_factory = factory;
    return 1;
}
int __cdecl bh_weapon_factory_close(BHWeaponFactory *factory)
{
    unsigned int i;
    if (!factory || active_factory != factory)
        return 0;
    for (i = 0; i < 10; i++)
        if (live(&factory->registry, &factory->registry.slots[i]))
        {
            factory->error = BH_WEAPON_FACTORY_BINDING;
            return 0;
        }
    active_factory = NULL;
    factory->registry.pool = NULL;
    factory->sound = NULL;
    factory->error = BH_WEAPON_FACTORY_OK;
    return 1;
}
BHEntity *__cdecl bh_weapon_factory_create(BHWeaponFactory *factory, unsigned int index)
{
    BHEntity *entity;
    BHWeaponObjectState *state;
    BHWeaponRecord *record;
    if (!factory || active_factory != factory || index >= 10)
    {
        if (factory)
            factory->error = BH_WEAPON_FACTORY_ARGUMENT;
        return NULL;
    }
    record = &factory->registry.slots[index];
    if (live(&factory->registry, record))
    {
        factory->error = BH_WEAPON_FACTORY_BINDING;
        return NULL;
    }
    entity = bh_entity_acquire(factory->registry.pool);
    if (!entity)
    {
        factory->error = BH_WEAPON_FACTORY_POOL;
        return NULL;
    }
    state = bh_weapon_object_state(entity);
    state->common.word_e8 = 0;
    state->common.available_ec = 0;
    state->common.word_f0 = 0;
    state->common.word_f4 = 0;
    state->common.hud.target = NULL;
    state->common.hud.charge = 0;
    state->common.word_100 = -1;
    state->common.word_104 = 0;
    state->common.word_108 = 0;
    state->common.ammo = 0;
    state->common.ammo_limit = ammo_limits[index];
    state->common.delay = 0;
    state->method_12c = methods[index][0];
    state->method_130 = methods[index][1];
    state->method_124 = methods[index][2];
    state->method_128 = methods[index][3];
    state->method_11c = methods[index][4];
    state->method_120 = methods[index][5];
    record->entity = entity;
    record->generation = entity->generation;
    record->constructor_index = index;
    if (!bh_entity_set_query(factory->registry.pool, entity, weapon_query))
    {
        record->entity = NULL;
        bh_entity_release(factory->registry.pool, entity);
        factory->error = BH_WEAPON_FACTORY_POOL;
        return NULL;
    }
    factory->error = BH_WEAPON_FACTORY_OK;
    return entity;
}
int __cdecl bh_weapon_registry_resolve(BHWeaponRegistry *registry, unsigned int selection, BHEntity **entity,
                                       BHWeaponObjectState **state)
{
    BHWeaponRecord *record;
    BHWeaponObjectState *current;
    if (entity)
        *entity = NULL;
    if (state)
        *state = NULL;
    if (!registry || !entity || !state || selection >= 10)
        return 0;
    record = &registry->slots[selection];
    if (record->constructor_index != selection || !live(registry, record))
        return 0;
    current = bh_weapon_object_state(record->entity);
    if (current->method_12c != methods[selection][0] || current->method_130 != methods[selection][1] ||
        current->method_124 != methods[selection][2] || current->method_128 != methods[selection][3] ||
        current->method_11c != methods[selection][4] || current->method_120 != methods[selection][5])
        return 0;
    *entity = record->entity;
    *state = current;
    return 1;
}
int __cdecl bh_weapon_registry_hud(BHWeaponRegistry *registry, unsigned int selection, BHEntity **entity,
                                   BHWeaponObjectState **state, unsigned long *identity)
{
    if (identity)
        *identity = 0;
    if (!identity || !bh_weapon_registry_resolve(registry, selection, entity, state))
        return 0;
    *identity = (*state)->method_130;
    return 1;
}
int __cdecl bh_weapon_method_available(unsigned long identity)
{
    switch (identity)
    {
    case 0x004309b0UL:
    case 0x00431050UL:
    case 0x00431130UL:
    case 0x00433170UL:
    case 0x00432c60UL:
    case 0x00432ff0UL:
    case 0x004324a0UL:
    case 0x00432510UL:
    case 0x00431180UL:
    case 0x004314f0UL:
    case 0x004310c0UL:
    case 0x00433820UL:
    case 0x00433b00UL:
    case 0x0042f730UL:
    case 0x004309d0UL:
    case 0x00432560UL:
    case 0x00433dc0UL:
        return 1;
    default:
        return 0;
    }
}
int __cdecl bh_weapon_factory_tick(BHWeaponFactory *factory, unsigned int selection)
{
    BHEntity *entity;
    BHWeaponObjectState *state;
    int result;
    if (!factory || active_factory != factory)
        return 0;
    if (!bh_weapon_registry_resolve(&factory->registry, selection, &entity, &state))
    {
        factory->error = BH_WEAPON_FACTORY_STALE;
        return 0;
    }
    switch (state->method_12c)
    {
    case 0x004309b0UL:
        result = bh_weapon_delay_tick(entity);
        break;
    case 0x00433170UL:
        result = bh_weapon_latch_tick(entity);
        break;
    case 0x00433820UL:
    case 0x00433b00UL:
        result = bh_weapon_single_timer_tick(entity);
        break;
    default:
        factory->error = BH_WEAPON_FACTORY_METHOD;
        return 0;
    }
    factory->error = result ? BH_WEAPON_FACTORY_OK : BH_WEAPON_FACTORY_BINDING;
    return result;
}
