/* Native function check only: no level, renderer, game startup or input. */
#include "../src/bh_actor_vehicle_step.h"
#include "../src/bh_actor_attack.h"
#include "../src/bh_actor_bounds.h"
#include "../src/bh_target_steering.h"
#include "../src/bh_actor_portal_image.h"
#include "../src/bh_route_query.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "../build/ai-byte-match/original-tick.h"

static BHActorAttackContext attack;
static BHEntityPool *release_pool;
static BHEntity *release_actor;
static int released_during_query;

static void __cdecl release_during_motion(BHEntity *target, void *packet)
{
    float marker = .25f;
    (void)target;
    if (*(long *)packet != 16)
        return;
    memset((char *)packet + 4, 0, 36);
    released_during_query = bh_entity_release(release_pool, release_actor);
    memcpy((char *)release_actor + 0x180, &marker, 4);
}

static void word(BHEntity *e, unsigned long offset, unsigned long value)
{
    memcpy((char *)e + offset, &value, 4);
}

int main(void)
{
    BHMathEnvironment environment;
    BHEntityPool pool;
    BHEntity *source, *reference;
    BHActorPursuitRouteContext pursuit;
    BHActorRouteContext route;
    BHEffectProjectileContext effects;
    BHHomingProjectileContext homing;
    BHProjectileCreateContext projectiles;
    BHAreaProjectileContext area;
    BHNavigation navigation;
    BHWorldGeometry world;
    BHActorRouteState state;
    unsigned char *code;
    void (__cdecl *original)(BHEntity *);
    void (__cdecl *original_bounds)(const void *, void *);
    void (__cdecl *original_normal)(void *, int, float *);
    unsigned long i, j, differences = 0, bounds_differences = 0, alias_differences = 0, normal_differences = 0;
    unsigned long pursuit_transitions = 0, dependency_errors = 0;
    unsigned long guard_failures = 0;
    unsigned long phase_word;
    BHActorBoundsImage bounds;
    unsigned char alias_image[512];
    static const long alias_offsets[] = {-8,-4,0,4,8,12,16,20};
    unsigned char *world_bounds;
    BHVec3 raw_point, wrapped_point, reference_point;
    BHActorPortalImage portal_image;
    static const unsigned long edge_words[] = {0,0x80000000UL,0x3f800000UL,0xbf800000UL,
        0x7f800000UL,0xff800000UL,0x7fc00000UL,0xffc00000UL,1,0x80000001UL,
        0x7f7fffffUL,0xff7fffffUL};
    static const float phases[] = {0, .5f, .95f, .96666664f, .9666667f, 1, 1.25f, -1};
    if (!bh_math_environment_enter(&environment) || !bh_entities_init(&pool, 16, 0))
        return 2;
    memset(&attack, 0, sizeof(attack));
    memset(&pursuit, 0, sizeof(pursuit));
    memset(&route, 0, sizeof(route));
    memset(&effects, 0, sizeof(effects));
    memset(&homing, 0, sizeof(homing));
    memset(&projectiles, 0, sizeof(projectiles));
    memset(&area, 0, sizeof(area));
    memset(&navigation, 0, sizeof(navigation));
    memset(&world, 0, sizeof(world));
    attack.pursuit = &pursuit;
    attack.effects = &effects;
    attack.projectiles = &projectiles;
    attack.area = &area;
    effects.effects = &homing;
    homing.pool = &pool;
    pursuit.route = &route;
    pursuit.navigation = &navigation;
    route.tick = 0xbeef;
    world.minimum.x = world.minimum.y = world.minimum.z = -1000;
    world.maximum.x = world.maximum.y = world.maximum.z = 1000;
    source = bh_entity_acquire(&pool);
    reference = bh_entity_acquire(&pool);
    if (!source || !reference)
        return 2;
    if (!bh_actor_vehicle_tick_bind(&attack, source) || !bh_actor_vehicle_tick_bind(&attack, reference))
        return 2;
    code = VirtualAlloc(0, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!code)
        return 2;
    install_original_tick(code);
    install_original_bounds(code + 512);
    install_original_normal(code + 1024);
    FlushInstructionCache(GetCurrentProcess(), code, 4096);
    original = (void (__cdecl *)(BHEntity *))code;
    original_bounds = (void (__cdecl *)(const void *, void *))(code + 512);
    original_normal = (void (__cdecl *)(void *, int, float *))(code + 1024);
    for (i = 0; i < 512; ++i)
    {
        memset(&state, 0, sizeof(state));
        state.current = state.next = &world;
        state.duration = 1;
        state.duration_ticks = 30;
        state.phase = phases[i % 8];
        if (i % 8 == 3 || i % 8 == 4)
        {
            memcpy(&phase_word, &state.phase, 4);
            phase_word += (i / 8) % 7 - 3;
            memcpy(&state.phase, &phase_word, 4);
        }
        state.speed = (float)(i % 31 + 1);
        state.speed_scale = 1;
        state.sound = BH_SOUND_INVALID;
        for (j = 0; j < 12; ++j)
            ((float *)state.controls)[j] = (float)((long)((i * 13 + j * 29) % 1001) - 500);
        bh_actor_route_write(source, &state);
        word(source, 0xfc, 6);
        word(source, 0x1b8, 0);
        word(source, 0x1d4, 1);
        memcpy((char *)source + 0x1bc, state.controls, 24);
        memcpy(reference->payload, source->payload, sizeof(source->payload));
        source->step(source);
        original(reference);
        if (memcmp(source->payload, reference->payload, sizeof(source->payload)))
            ++differences;
        if (!*(unsigned long *)((char *)source + 0x1d4))
            ++pursuit_transitions;
        if (attack.error || route.error)
            ++dependency_errors;
    }
    for (i = 0; i < 8192; ++i)
    {
        for (j = 0; j < 3; ++j)
        {
            ((float *)&world.minimum)[j] = (float)((long)((i * 13 + j * 29) % 1001) - 1500);
            ((float *)&world.maximum)[j] = (float)((i * 17 + j * 37) % 1001 + 1);
            ((float *)&raw_point)[j] = (float)((long)((i * 199 + j * 229) % 10001) - 5000);
            if (i % 4 == 0)
                memcpy((float *)&raw_point + j, edge_words + ((i / 4 + j) % 12), 4);
            if (i % 32 == 3)
                memcpy((float *)&world.minimum + j, edge_words + ((i / 32 + j) % 12), 4);
            if (i % 32 == 7)
                memcpy((float *)&world.maximum + j, edge_words + ((i / 32 + j) % 12), 4);
        }
        bounds.minimum = world.minimum;
        bounds.maximum = world.maximum;
        wrapped_point = reference_point = raw_point;
        original_bounds(&bounds, &reference_point);
        bh_actor_bounds_clamp(&bounds, &raw_point);
        bh_world_clamp_point(&world, &wrapped_point);
        if (memcmp(&raw_point, &reference_point, 12) || memcmp(&wrapped_point, &reference_point, 12))
            ++bounds_differences;
    }
    for (i = 0; i < 256; ++i)
    {
        world_bounds = (unsigned char *)&world + offsetof(BHWorldGeometry, minimum);
        for (j = 0; j < 10; ++j)
        {
            phase_word = edge_words[(i / 8 + j) % 12];
            memcpy(world_bounds - 8 + j * 4, &phase_word, 4);
        }
        memcpy(alias_image + 0x1b0 - 8, world_bounds - 8, 40);
        original_bounds(alias_image, alias_image + 0x1b0 + alias_offsets[i % 8]);
        bh_world_clamp_point(&world, (BHVec3 *)(world_bounds + alias_offsets[i % 8]));
        if (memcmp(world_bounds - 8, alias_image + 0x1b0 - 8, 40))
            ++alias_differences;
    }
    for (i = 0; i < 1024; ++i)
    {
        unsigned long slot = i % 6;
        portal_image.portals[slot].direction = (i / 6) % 10 - 1;
        memcpy(&raw_point.x, edge_words + 6, 4);
        memcpy(&raw_point.y, edge_words + 1, 4);
        raw_point.z = .25f;
        wrapped_point = reference_point = raw_point;
        original_normal(&portal_image, slot, (float *)&reference_point);
        bh_actor_portal_normal_core(&portal_image, slot, (float *)&raw_point);
        bh_portal_normal(portal_image.portals + slot, &wrapped_point);
        if (memcmp(&raw_point, &reference_point, 12) || memcmp(&wrapped_point, &reference_point, 12))
            ++normal_differences;
    }
    printf("{\"owner\":\"004398c0\",\"cases\":512,\"payload_differences\":%lu,"
           "\"pursuit_transitions\":%lu,\"dependency_errors\":%lu,"
           "\"level_or_game_executed\":false}\n",
           differences, pursuit_transitions, dependency_errors);
    printf("{\"owner\":\"004340f0\",\"cases\":8192,\"raw_and_adapter_differences\":%lu,"
           "\"alias_cases\":256,\"alias_differences\":%lu,"
           "\"includes_ieee_edges\":true,\"level_or_game_executed\":false}\n", bounds_differences, alias_differences);
    printf("{\"owner\":\"00434040\",\"cases\":1024,\"raw_and_adapter_differences\":%lu,"
           "\"level_or_game_executed\":false}\n", normal_differences);
    /* The checked callback must leave the matched body immediately if its
       real attack dependency detects that a target callback released it. */
    release_pool = &pool;
    release_actor = source;
    word(source, 0x1b8, (unsigned long)reference);
    word(source, 0x1d4, 1);
    if (!bh_entity_set_query(&pool, reference, release_during_motion))
        return 2;
    source->step(source);
    memcpy(&phase_word, (char *)source + 0x180, 4);
    if (!released_during_query || bh_entity_is_active(&pool, source) || attack.error != 3 ||
        attack.step_abort || phase_word != 0x3e800000 || !bh_math_environment_supported() || !bh_entities_valid(&pool))
        ++guard_failures;
    printf("{\"source_adapter\":\"checked-vehicle-tick\",\"release_guard_failures\":%lu,"
           "\"level_or_game_executed\":false}\n", guard_failures);
    VirtualFree(code, 0, MEM_RELEASE);
    bh_entities_dispose(&pool);
    bh_math_environment_leave(&environment);
    return differences || bounds_differences || alias_differences || normal_differences || dependency_errors || guard_failures || pursuit_transitions == 0;
}
