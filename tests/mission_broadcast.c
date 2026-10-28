/* Source-only mission broadcast/ownership check; no game loop or renderer. */
#include "../src/bh_script_vm.h"
#include "../src/bh_weapon_factory.h"
#include "../src/bh_script_flash.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHEntity *entities[8500];
static int list_contains(const BHEntityPool *pool, const BHEntity *entity)
{
    const BHEntity *at;
    for (at = pool->active; at; at = at->next)
        if (at == entity) return 1;
    return 0;
}
int main(void)
{
    BHEntityPool pool, other;
    BHScriptLifetimeContext lifetime;
    BHScriptVMContext vm;
    BHWeaponFactory weapons;
    BHSoundQueue sound;
    BHEntity *weapon, *resolved;
    BHWeaponObjectState *weapon_state;
    BHScriptVMState before, after;
    BHScriptTimerPacket packet;
    LARGE_INTEGER frequency, start, end;
    long args[5] = {0, 0, 0, 0, 0}, selectors[5] = {-1, -1, -1, -1, -1};
    unsigned long i;
    int actual_random, expected_random;
    memset(&lifetime, 0, sizeof(lifetime));
    memset(&vm, 0, sizeof(vm));
    memset(&packet, 0, sizeof(packet));
    if (!bh_entities_init(&pool, 8000, 0) || !bh_entities_init(&other, 1, 0)) return 1;
    lifetime.timers.pool = &pool;
    vm.lifetime = &lifetime;
    vm.script_loading = 1;
    for (i = 0; i < 8500; ++i)
        if (!(entities[i] = bh_script_vm_create(&vm, 0, 0, args, selectors))) return 2;
    if (!bh_entities_valid(&pool)) return 3;
    for (i = 0; i < 8500; ++i)
        if (bh_entity_is_active(&pool, entities[i]) != list_contains(&pool, entities[i])) return 21;
    srand(91);
    expected_random = rand();
    srand(91);
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    for (i = 0; i < 3; ++i)
    {
        packet.event = i == 0 ? 9 : i == 1 ? 10 : 17;
        if (!bh_entities_query(&pool, 0, 0, 0, 0, &packet)) return 4;
    }
    QueryPerformanceCounter(&end);
    actual_random = rand();
    if (actual_random != expected_random || vm.error || vm.executed || pool.used != 8500) return 5;
    if (!bh_script_vm_read(entities[0], &before)) return 6;
    for (i = 1; i < 8500; ++i)
        if (!bh_script_vm_read(entities[i], &after) || memcmp(&before, &after, sizeof(before))) return 7;
    printf("8500 real VM receivers, 3 broadcasts: %.3f ms; state/RNG unchanged\n",
           (double)(end.QuadPart-start.QuadPart)*1000.0/(double)frequency.QuadPart);
#ifdef BH_CHECK_FAST_MEMBERSHIP
    if (!bh_entity_is_active(&pool, entities[0]) || !bh_entity_is_active(&pool, entities[8499]) ||
        bh_entity_is_active(&other, entities[0]) || bh_entity_is_active(&pool, 0) ||
        bh_entity_is_active(&pool, (BHEntity *)((char *)entities[0] + 1))) return 8;
#endif
    packet.event = 4;
    if (!bh_entities_query(&pool, 0, 0, 0, 0, &packet) || pool.used || !bh_entities_valid(&pool)) return 9;
    for (i = 0; i < 8500; ++i)
        if (bh_entity_is_active(&pool, entities[i]) != list_contains(&pool, entities[i])) return 22;
#ifdef BH_CHECK_FAST_MEMBERSHIP
    if (bh_entity_is_active(&pool, entities[0]) || bh_entity_is_active(&pool, entities[8499])) return 10;
    if (!bh_entity_is_active(&pool, bh_entity_acquire(&pool))) return 11;
#endif
    bh_entities_dispose(&other);
    bh_entities_dispose(&pool);
    memset(&sound, 0, sizeof(sound));
    if (!bh_entities_init(&pool, 9000, 0) || !bh_weapon_factory_open(&weapons, &pool, &sound)) return 12;
    weapon = bh_weapon_factory_create(&weapons, 0);
    if (!weapon) return 13;
    for (i = 0; i < 8500; ++i)
        if (!(entities[i] = bh_entity_acquire(&pool))) return 14;
    QueryPerformanceCounter(&start);
    for (i = 0; i < 2000; ++i)
        if (!bh_weapon_registry_resolve(&weapons.registry, 0, &resolved, &weapon_state) || resolved != weapon) return 15;
    QueryPerformanceCounter(&end);
    printf("2000 actual weapon lookups behind 8500 entities: %.3f ms\n",
           (double)(end.QuadPart-start.QuadPart)*1000.0/(double)frequency.QuadPart);
    if (!bh_script_flash_bind(&pool, entities[0], 1.0f)) return 16;
    if (!bh_entity_release(&pool, entities[0]) || bh_script_flash_bind(&pool, entities[0], 1.0f)) return 17;
    if (!bh_entity_release(&pool, weapon) || bh_weapon_registry_resolve(&weapons.registry, 0, &resolved, &weapon_state)) return 18;
    if (bh_entity_acquire(&pool) != weapon || bh_weapon_registry_resolve(&weapons.registry, 0, &resolved, &weapon_state)) return 19;
    if (!bh_weapon_factory_close(&weapons) || !bh_entities_valid(&pool) || !bh_entities_dispose(&pool)) return 20;
    puts("Ownership, growth, broadcast deletion and recycling passed");
    return 0;
}
