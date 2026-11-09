#include "bh_game_app.h"
#include "bh_platform.h"
#include "bh_game_bootstrap.h"
#include "bh_game_settings.h"
#include "bh_loading_progress.h"
#include "bh_input_devices.h"
#include "bh_input_messages.h"
#include "bh_frame_service.h"
#include "bh_frame_callback.h"
#include "bh_render_startup.h"
#include "bh_render_execute.h"
#include "bh_present_begin.h"
#include "bh_atlas_text.h"
#include "bh_render_interpolation.h"
#include "bh_primitive_dispatch.h"
#include "bh_sprite_quad.h"
#include "bh_horizon_color.h"
#include "bh_model_path.h"
#include "bh_texture_policy.h"
#include "bh_loading_image.h"
#include "bh_light_config.h"
#include "bh_gdi_text.h"
#include "bh_attachment.h"
#include "bh_input_text.h"
#include "bh_menu_profile.h"
#include "bh_target_construct.h"
#include "bh_actor_response.h"
#include "bh_effect_orbit_tick.h"
#include "bh_effect_accelerated.h"
#include "bh_effect_split_tick.h"
#include "bh_actor_construct.h"
#include "bh_actor_launch.h"
#include "bh_script_actor_construct.h"
#include "bh_vfile.h"
#include "bh_fp.h"
#include "bh_rotation.h"
#include "bh_sound_startup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#define GAME_WORLD_CAPACITY 6000
#define GAME_DYNAMIC_CAPACITY 256
#define GAME_STARTS 32
#define GAME_TRACE_TICKS 120
#define N(a) (sizeof(a) / sizeof((a)[0]))
#ifdef DEBUGGING
typedef struct BHGamePerfSample
{
    double time_ms, interval_ms, simulation_ms, render_ms, present_ms;
    unsigned long frame, produced, mode, paused, commands, entities, projectiles, contacts, rebuilds;
} BHGamePerfSample;
#define PERF_BLOCK_SAMPLES 4096
typedef struct BHGamePerfBlock
{
    struct BHGamePerfBlock *next;
    unsigned long count;
    BHGamePerfSample samples[PERF_BLOCK_SAMPLES];
} BHGamePerfBlock;
typedef struct BHGamePerfCapture
{
    BHGamePerfBlock *head, *tail;
    unsigned long count, dropped, previous[7];
} BHGamePerfCapture;
static BHGamePerfSample *perf_append(BHGamePerfCapture *capture)
{
    BHGamePerfBlock *block;
    if (!capture->tail || capture->tail->count == PERF_BLOCK_SAMPLES)
    {
        block = (BHGamePerfBlock *)calloc(1, sizeof(*block));
        if (!block)
        {
            ++capture->dropped;
            return NULL;
        }
        if (capture->tail)
            capture->tail->next = block;
        else
            capture->head = block;
        capture->tail = block;
    }
    ++capture->count;
    return capture->tail->samples + capture->tail->count++;
}
static unsigned long perf_contact_delta(BHGamePerfCapture *capture, const unsigned long *current)
{
    unsigned long i, total = 0;
    for (i = 0; i < 7; ++i)
    {
        total += current[i] >= capture->previous[i] ? current[i] - capture->previous[i] : current[i];
        capture->previous[i] = current[i];
    }
    return total;
}
static void perf_dispose(BHGamePerfCapture *capture)
{
    BHGamePerfBlock *block = capture->head, *next;
    while (block)
    {
        next = block->next;
        free(block);
        block = next;
    }
    memset(capture, 0, sizeof(*capture));
}

#endif
typedef struct BHGameTraceTick
{
    unsigned long tick, frame, entities, retirements, commands, camera_waypoint;
    long mode, paused, confirmation, waypoint;
    float route_phase;
    BHVec3 eye;
    int frame_error, player_error, menu_error, flyby_error;
    const char *menu_stage;
} BHGameTraceTick;
typedef struct BHGameApp
{
    int hardware, fullscreen;
    long quit;
    int initialized, error, focus_sample;
    const char *failure_function;
    int failure_line;
    BHGameTraceTick trace[GAME_TRACE_TICKS];
    unsigned int trace_next, trace_count;
    char archive[512], config_path[512];
    long width, height, selected_waypoint, mission, ship, slot, input_changed, stay, fine_warning, pending_profile;
    unsigned long frame, actor_count, availability[4][32], event_value, mission_identity, instability;
    float yaw;
    BHVec3 zero, eye, angles;
    BHEntity *player, *camera_entity, *damage_source, *actor_head, *actors[GAME_DYNAMIC_CAPACITY + 2], *remote[8];
    BHMathEnvironment math;
    BHConfig config;
    BHInputProfile *profile;
    BHPlayerInputOwner inputs;
    BHPlayerInputState decoded, global_state;
    BHInputSnapshot snapshot;
    BHInputCollection collection;
    BHInputDevices devices;
    BHLoadingImages loading_images;
    BHLoadingDisplay loading_display;
    BHLoadingProgress loading_progress;
    BHFrameBuffers frames;
    BHDrawQueue queue, slots[3];
    BHRenderInterpolation interpolation;
    BHTextQueue text;
    BHFrameService service;
    BHFrameCallback callback;
    LPDIRECTDRAW draw;
    LPDIRECTDRAWSURFACE primary, target;
    BHDeviceName device_names[8];
    char text_buffer[160];
    BHRenderMode render_mode;
    BHRenderMaterial material;
    BHViewConfig view, scene_view;
    BHMainCamera main_camera;
    BHClipContext clip;
    BHScreenClipContext screen_clip;
    BHAttributeClipContext attribute_clip;
    BHLightConfig light;
    BHLightContext lights;
    BHModelContext models;
    BHRenderResources resources;
    BHModelInstanceContext prototype;
    BHAtlasContext atlas;
    BHRenderState render_state;
    BHPrimitiveDispatch primitive;
    BHHorizonColor horizon_colors;
    BHSpriteBillboardContext billboard;
    BHSpriteOrientedContext oriented;
    BHSpriteOrientedVertex path[4096];
    BHSpriteQuadContext sprite_quad;
    BHScreenQuadContext screen_quad;
    BHPickupDrawContext pickup_draw;
    BHDrawResourceContext resource_draw;
    BHDrawSubmitContext model_submit;
    BHDrawBillboardSubmitContext billboard_submit;
    BHScreenQuadSubmit screen_submit;
    BHQuadQueueContext quad_submit;
    BHPathQueueContext path_submit;
    BHSegmentQueueContext segment_submit;
    BHDrawControlContext controls;
    BHPortalClipCache portal_cache;
    BHHorizonContext horizon;
    BHHorizonQueueContext horizon_submit;
    BHFlareSubmit flare_submit;
    BHFrameDisplay display;
    BHFadeContext fade;
    BHPresentContext present;
    BHPresentBegin begin;
    BHRenderExecute render;
    BHGdiText gdi;
    BHSoundQueue sound;
    BHSoundStartupStats sound_stats;
    BHSoundLoad sounds;
    BHCdClose cd;
    BHCdNotify cd_notify;
    BHCdAudio cd_audio;
    BHMediaControl media;
    BHGameBootstrap bootstrap;
    BHBootstrapBinding bindings[256];
    unsigned long binding_count;
    BHScriptVMContext vm;
    BHScriptParse parser;
    BHScriptBlocks blocks;
    BHScriptLifetimeContext lifetime;
    BHScriptMotionContext motion;
    BHScriptMotionBinding *motion_bindings;
    BHScriptSensorContext sensors;
    BHScriptEmitterContext emitters;
    BHScriptTimerActor timer_actors[GAME_WORLD_CAPACITY];
    BHScriptVMActor vm_actors[GAME_WORLD_CAPACITY];
    BHScriptVMWaypoint vm_starts[GAME_STARTS];
    BHPlayerCameraStart starts[GAME_STARTS];
    BHVec3 route_starts[GAME_STARTS];
    BHEntityPool pool;
    BHEditorWorld editor_world;
    BHEditorControl editor;
    BHEditorNode *nodes[GAME_WORLD_CAPACITY];
    BHWorldLoad world_load;
    BHEditorInput editor_input;
    BHPlayerInputState editor_state;
    BHScriptBrowser browser;
    BHEditorNumericContext numeric;
    BHRailPaths rails;
    BHRailWagonContext wagon;
    BHNavigation navigation;
    BHWorldSegmentContext segment;
    BHWorldSegmentRoom *segment_rooms;
    BHWorldSegmentRoom **segment_room_index;
    BHBindingIndex world_binding_index;
    BHSceneAllocator binding_allocator;
    BHSceneBindings scene_bindings;
    BHSceneBindingInput *scene_inputs;
    BHPickupRoom *rooms;
    BHEntityScheduleRoom *scheduled;
    BHEntityScheduleContext schedule;
    BHSceneOwner scene;
    BHWorldReset world_reset;
    BHWorldResetNode *reset_nodes;
    BHPhysicsState player_motion;
    BHMat34 player_inverse;
    BHPhysicsStepContext physics;
    BHPhysicsStepWorld *physics_worlds;
    BHGeometry *physics_models[GAME_DYNAMIC_CAPACITY + 1];
    BHPhysicsStepBody physics_bodies[GAME_DYNAMIC_CAPACITY + 1];
    BHPhysicsEffects physics_effects;
    BHActorBasisContext basis;
    BHActorBasisSample samples[15];
    BHActorBasisGroup basis_a[7], basis_b[7];
    BHActorBasisIndices indices_a[7], indices_b[7];
    BHActorBasisScale scales_a[7], scales_b[7];
    BHPlayerForceControl player_force;
    BHPlayerTickState player_state;
    BHPlayerTick player_tick;
    BHPlayerCreate player_create;
    BHPlayerQueryContext player_query;
    BHPlayerReset player_reset;
    BHPlayerCameraContext player_camera;
    BHCameraStateContext camera_state;
    BHCameraCorrectionOwner correction;
    BHCollisionModel player_collision;
    BHPlayerScene player_scene;
    BHSceneBackground background;
    BHHudContext hud;
    BHHudMarkers markers;
    BHRadarFrame radar_frame;
    BHRadarContext radar;
    BHAttachmentContext attachment;
    BHHudWorldContext world_hud;
    BHHudFlatContext flat_hud;
    BHNetworkGame network_game;
    BHNetworkTransport transport;
    BHRemotePackets remote_packets;
    BHRemoteMessages remote_messages;
    BHRemoteTick remote_tick;
    BHRemoteRender remote_render;
    BHRemoteQuery remote_query;
    BHRemoteFire remote_fire;
    BHNetworkScores network_scores;
    BHNetworkEffectReceive network_effects;
    BHEffectNetworkContext effect_network;
    BHEffectWorldIndex *effect_indices;
    BHPulseWorldIndex *pulse_indices;
    BHNavigation network_positions;
    BHWorldGeometry *network_world, *remote_spawn_world;
    unsigned short network_world_index;
    float network_latency[8];
    unsigned long sync_identity;
    FILE *network_log;
    BHMenuRuntime *network_entry;
    char network_address[16];
    long network_ship;
    int network_playing, network_cancel_pressed;
    BHHudScores scores;
    BHHudScoreDraw score_draw;
    BHHudScoreText score_text;
    BHFlybyScene flyby;
    BHFlybyConstruct flyby_construct;
    BHFlybyCameraContext flyby_camera;
    BHFlybyCameraState flyby_state;
    BHCameraRouteContext camera_route;
    BHFlybyCredits credits;
    BHMenuRuntimeContext menu;
    BHMenuActions menu_actions;
    BHMenuEdits menu_edits;
    BHMenuSettings menu_settings;
    BHMenuProfileContext menu_profiles;
    BHMenuPresent menu_present;
    BHMenuTransition transition;
    BHInputTextCapture text_capture;
    BHInputKeyCapture key_capture;
    BHMessageState messages;
    BHMessagePanel panel;
    BHMissionProgressContext mission_progress;
    BHMissionStory story;
    BHWeaponFactory weapons;
    BHWeaponDispatch weapon_dispatch;
    BHWeaponReadyContext weapon_ready;
    BHWeaponTargetContext weapon_target;
    BHWeaponArcLifecycleContext weapon_arc;
    BHWeaponEmptyContext weapon_empty;
    BHWeaponFireContext weapon_fire;
    BHWeaponPulseContext weapon_pulse;
    BHWeaponEffectFireContext weapon_effect;
    BHWeaponPulseLaunchContext weapon_launch;
    BHWeaponArcFireContext weapon_arc_fire;
    BHArcContext arc_hud;
    BHWeaponHudTargetContext target_hud;
    BHProjectileCreateContext projectiles;
    BHProjectileLifetime projectile_lifetime;
    BHProjectileDrawContext projectile_draw;
    BHBallisticProjectileContext ballistic;
    BHHomingProjectileContext homing, projectile_effects;
    BHAreaProjectileContext area;
    BHEffectProjectileContext effect;
    BHEffectOrbitTickContext orbit;
    BHEffectAcceleratedContext accelerated;
    BHEffectSplitContext split;
    BHPulseProjectileContext pulse;
    BHScriptParticleContext particles;
    BHContactEffects contacts;
    BHFlashFlareContext flash;
    BHScriptDebrisContext debris;
    BHScriptExplosionContext explosions;
    BHHitMarkerContext hit_markers;
    BHPickupContext pickups;
    BHPickupRenderBinding pickup_render;
    BHEntityResourceDrawContext special_draw;
    unsigned long pickup_resources[22];
    BHDebrisContext debris_response;
    BHTargetConstructContext targets;
    BHTargetConstructSlot target_slots[GAME_DYNAMIC_CAPACITY];
    BHTargetDrawContext target_draw;
    BHActorRouteContext actor_route;
    BHActorDamageContext actor_damage;
    BHActorAttackContext actor_attack;
    BHActorVehicleLifetimeContext vehicle_lifetime;
    BHActorResponseContext actor_response;
    BHActorConstructContext actor_construct;
    BHActorConstructRoom *actor_rooms;
    BHActorVehicleDrawContext vehicle_draw;
    BHActorLaunchContext actor_launch;
    unsigned long actor_models[25], actor_sounds[25], cycle_models[4];
    BHScriptActorConstructorContext script_actors;
    BHScriptActorDrawContext script_draw;
    BHRadarQueryContext radar_query;
    BHActorPursuitRouteContext pursuit;
    BHSegmentQuery block_queries[600];
    BHHomingWorldResponse *world_responses;
    long *city_ids;
    BHEffectDrawContext effect_draw;
    BHTrailContext trails;
    BHEffectPuffContext puffs;
    BHPulseVisualContext pulse_draw;
    BHGeometry *base_models[1];
    BHWorldSegmentActor segment_actors[GAME_DYNAMIC_CAPACITY + 1];
    BHSegmentQuery actor_queries[GAME_DYNAMIC_CAPACITY + 1];
    unsigned long actor_query_generations[GAME_DYNAMIC_CAPACITY + 1];
    BHEntity *actor_query_entities[GAME_DYNAMIC_CAPACITY + 1];
    unsigned char key_result[2];
    unsigned long empty_warning;
    long weapon_warning;
#ifdef DEBUGGING
    int perf_enabled;
    BHGamePerfCapture perf_capture;
    unsigned long perf_projectiles;
    uint64_t perf_origin, perf_previous;
    char perf_path[512];
#endif
} BHGameApp;
/* Original city-name pointer table at0048fb58, indexed by placement city ID. */
static const char *const city_names[16] = {"Downtown",
                                           "Old Bronx",
                                           "Central Stadium",
                                           "The Docks",
                                           "Industrial Complex",
                                           "Space Port",
                                           "Underground City",
                                           "Scraptown",
                                           "Grand Vegas",
                                           "Financial District",
                                           "Mall Town",
                                           "Chinatown",
                                           "the Aqua Tunnels\nnear The Docks",
                                           "Mining Tunnels\nat Industrial\nComplex",
                                           "City Ring",
                                           "Bronx/Downtown\nMetro Station"};
static int record_failure(BHGameApp *app, int code, const char *function, int line)
{
    app->error = code;
    if (!app->failure_function)
    {
        app->failure_function = function;
        app->failure_line = line;
    }
    return 0;
}
#define failure(app, code) record_failure((app), (code), __func__, __LINE__)
static void trace_tick(BHGameApp *app)
{
    BHGameTraceTick *tick = app->trace + app->trace_next;
    tick->tick = app->service.calls;
    tick->frame = app->frame;
    tick->entities = app->pool.used;
    tick->retirements = app->pool.retirements;
    tick->commands = app->queue.count;
    tick->mode = app->player_state.mode;
    tick->paused = app->player_state.paused;
    tick->confirmation = app->player_state.confirmation;
    tick->waypoint = app->selected_waypoint;
    tick->camera_waypoint = app->flyby_state.route.waypoint_index;
    tick->route_phase = app->flyby_state.route.phase;
    tick->eye = app->flyby_state.camera.position;
    tick->frame_error = app->service.error;
    tick->player_error = app->player_tick.error;
    tick->menu_error = app->menu.error;
    tick->flyby_error = app->flyby.error;
    tick->menu_stage = app->player_tick.menu_return_stage;
    app->trace_next = (app->trace_next + 1) % N(app->trace);
    if (app->trace_count < N(app->trace))
        ++app->trace_count;
}

/* Offsets relative to this symbol are stable under address randomization. */
static unsigned long callback_offset(uintptr_t callback)
{
    return callback ? (unsigned long)(callback - (uintptr_t)bh_game_app_run) : 0;
}

static void dump_entity(FILE *log, const BHEntityPool *pool, const char *label, const BHEntity *entity)
{
    if (!bh_entity_is_active(pool, entity))
    {
        fprintf(log, "%s %p inactive\n", label, (const void *)entity);
        return;
    }
    fprintf(log, "%s %p gen=%lu kind=%lu flags=%08lx pos=(%.9g %.9g %.9g) "
                 "step=%08lx refresh=%08lx query=%08lx next=%p\n",
            label, (const void *)entity, entity->generation, entity->kind, entity->flags,
            entity->position.x, entity->position.y, entity->position.z,
            callback_offset((uintptr_t)entity->step), callback_offset((uintptr_t)entity->refresh),
            callback_offset((uintptr_t)entity->query), (void *)entity->next);
}

static void dump_mutation(FILE *log, const BHEntityMutation *mutation, int checks_generation)
{
    if (mutation->dispatch)
    {
        fprintf(log, "%s mutation entity=%p gen=%lu callback=%08lx successor=%p",
                mutation->dispatch, (const void *)mutation->entity, mutation->generation,
                callback_offset(mutation->callback), (const void *)mutation->successor);
        if (checks_generation)
            fprintf(log, " expected_gen=%lu", mutation->successor_generation);
        fputc('\n', log);
    }
}

static void dump_game_state(FILE *log, const BHGameApp *app)
{
    const BHCameraRouteState *route = &app->flyby_state.route;
    const BHCameraState *camera = &app->flyby_state.camera;
    const BHEntity *entity;
    unsigned int i;
    fprintf(log, "Build %s %s, GPU %s, resolution %ldx%ld, initialized=%d\n",
            __DATE__, __TIME__, bh_sdl_gpu_driver(), app->width, app->height, app->initialized);
    fprintf(log, "State paused=%ld confirm=%ld ending=%ld death_frames=%ld ship=%ld player_tick=%lu focus=%d\n",
            app->player_state.paused, app->player_state.confirmation, app->player_state.ending,
            app->player_state.death_frames, app->ship, app->player_state.tick, app->focus_sample);
    fprintf(log, "Last menu return stage=%s, settle steps=%lu, reset timers=%lu immediate=%lu, transition error=%d\n",
            app->player_tick.menu_return_stage ? app->player_tick.menu_return_stage : "none",
            app->menu.camera_steps, app->world_reset.timers, app->world_reset.immediate, app->transition.error);
    fprintf(log, "Last flyby camera failure=%s\n", app->flyby_camera.last_failure ? app->flyby_camera.last_failure : "none");
    fprintf(log, "Flyby camera mode=%ld eye=(%.9g %.9g %.9g) angles=(%.9g %.9g %.9g) orbit=%.9g "
                 "route waypoint=%lu target=%lu phase=%.9g rate=%.9g room=%ld next=%ld\n",
            camera->current.mode, camera->position.x, camera->position.y, camera->position.z,
            camera->angles.x, camera->angles.y, camera->angles.z, app->flyby_state.orbit_time,
            route->waypoint_index, app->camera_route.selected_waypoint, route->phase, route->parameter_rate,
            bh_navigation_find(&app->navigation, route->current), bh_navigation_find(&app->navigation, route->next));
    for (i = 0; i < N(route->curve); ++i)
        fprintf(log, "  Route control %u (%.9g %.9g %.9g)\n",
                i, route->curve[i].x, route->curve[i].y, route->curve[i].z);
    for (i = 0; i < N(app->bootstrap.menus); ++i)
    {
        const BHBootstrapMenu *menu = app->bootstrap.menus + i;
        if (bh_entity_is_active(&app->pool, menu->runtime.entity) && menu->runtime.entity->step)
            fprintf(log, "Active menu %u id=%08lx selection=%ld distance=%.9g exiting=%ld\n",
                    i, menu->identity, menu->settings.selection, menu->runtime.distance, menu->runtime.exiting);
    }
    fprintf(log, "Entities used=%lu capacity=%lu retirements=%lu vehicles=%lu physics_bodies=%lu pool_error=%d\n",
            app->pool.used, app->pool.capacity, app->pool.retirements, app->vehicle_lifetime.count,
            app->physics.body_count, app->pool.error);
    fprintf(log, "Physics error=%d world_segment=%d, VM depth=%lu executed=%lu, player_reset=%d\n",
            app->physics.error, app->segment.error, app->vm.depth, app->vm.executed, app->player_reset.error);
    fprintf(log, "Frame service calls=%lu produced=%lu failures=%lu buffers active=%ld latest=%ld draw=%lu/%lu\n",
            app->service.calls, app->service.produced, app->service.failures, app->frames.active, app->frames.latest,
            app->queue.count, app->queue.capacity);
    fprintf(log, "Backend results render=%08lx begin=%08lx end=%08lx audio=%08lx voices=%lu listener=%ld\n",
            (unsigned long)app->render.last_hresult, (unsigned long)app->begin.last_hresult,
            (unsigned long)app->present.last_hresult, (unsigned long)app->service.sound_stats.last_error,
            app->sound.voice_count, app->sound.listener_active);
    fprintf(log, "\nEntity callbacks are hex offsets from bh_game_app_run (32-bit wrap).\n");
    dump_mutation(log, &app->pool.step_mutation, 0);
    dump_mutation(log, &app->pool.query_mutation, 1);
    dump_entity(log, &app->pool, "Player", app->player);
    dump_entity(log, &app->pool, "Flyby", app->camera_entity);
    for (i = 0, entity = app->pool.active; i < 64 && bh_entity_is_active(&app->pool, entity);
         ++i, entity = entity->next)
        dump_entity(log, &app->pool, "Active", entity);
    fprintf(log, "\nRecent simulation ticks (oldest first, errors=frame/player/menu/flyby)\n");
    for (i = 0; i < app->trace_count; ++i)
    {
        const BHGameTraceTick *tick =
            app->trace + (app->trace_next + N(app->trace) - app->trace_count + i) % N(app->trace);
        fprintf(log, "tick=%lu frame=%lu mode=%ld paused=%ld confirm=%ld wp=%lu->%ld phase=%.9g "
                     "eye=(%.9g %.9g %.9g) entities=%lu retired=%lu commands=%lu errors=%d/%d/%d/%d menu=%s\n",
                tick->tick, tick->frame, tick->mode, tick->paused, tick->confirmation, tick->camera_waypoint,
                tick->waypoint, tick->route_phase, tick->eye.x, tick->eye.y, tick->eye.z,
                tick->entities, tick->retirements, tick->commands, tick->frame_error, tick->player_error,
                tick->menu_error, tick->flyby_error, tick->menu_stage ? tick->menu_stage : "none");
    }
}

static void report_failure(const BHGameApp *app)
{
    char summary[256], details[1024], dialog[1024], timestamp[32] = "unknown";
    const char *path = bh_sdl_settings_file("bhunter-crash.log");
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    FILE *log;
    int write_error;
    snprintf(summary, sizeof(summary),
             "Game stopped with error %d in %s (line %d).\n"
             "Frame %lu, mode %ld, mission %ld, waypoint %ld.",
             app->error, app->failure_function ? app->failure_function : "startup", app->failure_line,
             app->frame, app->player_state.mode, app->mission, app->selected_waypoint);
    snprintf(details, sizeof(details),
             "Frame service %d (step %d, refresh %d), sound %d, render %d, draw queue %d.\n"
             "Player %d, player query %d, script %d, timer %d, world reset %d.\n"
             "Menu %d, flyby %d, camera route %d, scene %d, schedule %d.\n"
             "Actor construction %d, actor response %d, attack %d.\n%.300s",
             app->service.error, app->service.step_error, app->service.refresh_error,
             app->sound.error, app->render.error, app->queue.error,
             app->player_tick.error, app->player_query.error, app->vm.error, app->lifetime.timers.error,
             app->world_reset.error, app->menu.error, app->flyby.error, app->camera_route.error,
             app->scene.error, app->schedule.error, app->actor_construct.error, app->actor_response.error,
             app->actor_attack.error, bh_sdl_error());
    log = fopen(path, "a");
    if (!log)
    {
        snprintf(dialog, sizeof(dialog), "Could not open crash report %.600s\n%.100s\n\n%s",
                 path, strerror(errno), summary);
        bh_sdl_report_error(dialog);
        return;
    }
    if (utc)
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", utc);
    fprintf(log, "\n=== BHunter crash report %s ===\n%s\n%s\n", timestamp, summary, details);
    dump_game_state(log, app);
    write_error = ferror(log);
    if (fclose(log))
        write_error = 1;
    snprintf(dialog, sizeof(dialog), "%s\n%.650s\n\n%s",
             write_error ? "Could not finish writing crash report" : "Crash report saved to", path, summary);
    bh_sdl_report_error(dialog);
}
#ifdef DEBUGGING
static double perf_ms(BHGameApp *app, uint64_t end, uint64_t start)
{
    return (double)(end - start) / 1000000.0;
}
static int perf_start(BHGameApp *app)
{
    if (!app->perf_enabled)
        return 1;
    app->perf_origin = bh_sdl_nanoseconds();
    {
        char name[96];
        sprintf(name, "frame-times-%llu.csv", (unsigned long long)app->perf_origin);
        if (snprintf(app->perf_path, sizeof(app->perf_path), "%s", bh_sdl_settings_file(name)) >=
            sizeof(app->perf_path))
            return failure(app, 12);
    }
    return 1;
}
static void perf_record(BHGameApp *app, int produced, uint64_t simulation_start, uint64_t render_start,
                        uint64_t present_start, uint64_t end)
{
    BHGamePerfSample *sample;
    BHEntity *entity;
    unsigned long contacts[7], count = 0;
    sample = perf_append(&app->perf_capture);
    if (!sample)
        return;
    sample->time_ms = perf_ms(app, end, app->perf_origin);
    sample->interval_ms = app->perf_previous ? perf_ms(app, end, app->perf_previous) : 0;
    app->perf_previous = end;
    sample->simulation_ms = produced ? perf_ms(app, render_start, simulation_start) : 0;
    sample->render_ms = perf_ms(app, present_start, render_start);
    sample->present_ms = perf_ms(app, end, present_start);
    sample->frame = app->frame;
    sample->produced = produced;
    sample->mode = app->player_state.mode;
    sample->paused = app->player_state.paused;
    sample->commands = app->slots[app->frames.latest].count;
    sample->entities = app->pool.used;
    sample->rebuilds = app->targets.rebuilds;
    if (produced)
    {
        app->perf_projectiles = 0;
        for (entity = app->pool.active; entity && count++ < app->pool.capacity; entity = entity->next)
            if (entity->query == bh_projectile_query)
                ++app->perf_projectiles;
    }
    sample->projectiles = app->perf_projectiles;
    contacts[0] = app->physics.contacts;
    contacts[1] = app->ballistic.bindings.world_hits;
    contacts[2] = app->ballistic.bindings.proximity;
    contacts[3] = app->homing.world_hits;
    contacts[4] = app->homing.proximity;
    contacts[5] = app->projectile_effects.world_hits;
    contacts[6] = app->projectile_effects.proximity;
    sample->contacts = perf_contact_delta(&app->perf_capture, contacts);
}
static void perf_save(BHGameApp *app)
{
    FILE *f;
    unsigned long i;
    BHGamePerfSample *sample;
    BHGamePerfBlock *block;

    if (!app->perf_capture.head)
        return;
    f = fopen(app->perf_path, "w");
    if (f)
    {
        fprintf(
            f,
            "time_ms,interval_ms,simulation_ms,render_ms,present_ms,frame,produced,mode,paused,commands,entities,projectiles,contacts,rebuilds\n");
        for (block = app->perf_capture.head; block; block = block->next)
            for (i = 0; i < block->count; ++i)
            {
                sample = block->samples + i;
                fprintf(f, "%.6f,%.6f,%.6f,%.6f,%.6f,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", sample->time_ms,
                        sample->interval_ms, sample->simulation_ms, sample->render_ms, sample->present_ms,
                        sample->frame, sample->produced, sample->mode, sample->paused, sample->commands,
                        sample->entities, sample->projectiles, sample->contacts, sample->rebuilds);
            }
        fclose(f);
    }
    perf_dispose(&app->perf_capture);
}
#endif
static BHEntity **marker_room_head(BHEntity *owner, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    unsigned long i;
    long found = bh_binding_find(&app->world_load.actor_index, owner, NULL);
    if (found >= 0 && (unsigned long)found < app->homing.response_count &&
        app->world_responses[found].receiver == owner)
        return &app->rooms[found].pickups;
    for (i = 0; i < app->homing.response_count; ++i)
        if (app->world_responses[i].receiver == owner)
            return &app->rooms[i].pickups;
    return 0;
}
static int refresh_segment_actors(BHWorldSegmentContext *ctx, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    BHEntity *entity;
    BHGeometry *geometry;
    BHSegmentQuery *query;
    unsigned long i, j, index;
    /* Follow the same authoritative ordered registry used by player/debris
      constructors, resolving meshes through their actual owning contexts. */
    for (i = 0; i < N(app->actors) && app->actors[i]; ++i)
    {
        if (i >= N(app->segment_actors))
            return 0;
        entity = app->actors[i];
        index = 0;
        geometry = 0;
        if (entity == app->player)
            geometry = &app->player_collision.geometry;
        else if (app->segment_actors[i].entity == entity && app->segment_actors[i].query &&
                 (index = (unsigned long)(app->segment_actors[i].query - app->actor_queries)) > 0 &&
                 index <= N(app->target_slots) && app->actor_query_entities[index] == entity &&
                 app->actor_query_generations[index] == entity->generation &&
                 app->target_slots[index - 1].body.entity == entity &&
                 app->target_slots[index - 1].body.generation == entity->generation)
            geometry = &app->target_slots[index - 1].collision.geometry;
        else
            for (j = 0; j < N(app->target_slots); ++j)
                if (app->target_slots[j].body.entity == entity &&
                    app->target_slots[j].body.generation == entity->generation)
                {
                    index = j + 1;
                    geometry = &app->target_slots[j].collision.geometry;
                    break;
                }
        if (!geometry || !geometry->bsp)
            return 0;
        query = app->actor_queries + index;
        if (query->geometry != geometry || app->actor_query_entities[index] != entity ||
            app->actor_query_generations[index] != entity->generation)
        {
            if (query->frames && !bh_segment_query_dispose(query))
                return 0;
            if (!bh_segment_query_init(query, geometry))
                return 0;
            app->actor_query_generations[index] = entity->generation;
            app->actor_query_entities[index] = entity;
        }
        app->segment_actors[i].entity = entity;
        app->segment_actors[i].query = query;
    }
    if (i == N(app->actors))
        return 0;
    ctx->actors = app->segment_actors;
    ctx->actor_count = i;
    return 1;
}
static int restore_display(BHGameApp *app)
{
    LPDIRECTDRAWSURFACE surfaces[2];
    HRESULT hr;
    unsigned long i;

    surfaces[0] = app->primary;
    surfaces[1] = app->target;
    for (i = 0; i < 2; ++i)
        if (surfaces[i])
        {
            hr = IDirectDrawSurface_IsLost(surfaces[i]);
            if (hr == DDERR_SURFACELOST)
                hr = IDirectDrawSurface_Restore(surfaces[i]);
            if (FAILED(hr))
            {
                return failure(app, 13);
            }
        }
    return 1;
}

static HRESULT loading_present(LPDIRECTDRAWSURFACE color, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    return IDirectDrawSurface_Blt(app->primary, 0, color, 0, DDBLT_WAIT, 0);
}
static int loading_begin(BHGameApp *app)
{
    if (!bh_loading_images_bind(&app->loading_images, app->width, 2, app->render_mode.engine.textures.display_right,
                                app->render_mode.engine.textures.display_left, 0) ||
        !bh_loading_images_init(&app->loading_images))
        return failure(app, 6);
    app->loading_display.images = &app->loading_images;
    app->loading_display.color = app->target;
    app->loading_display.primary = app->primary;
    app->loading_display.back = app->render_mode.render;
    app->loading_display.width = app->width;
    app->loading_display.height = app->height;
    app->loading_display.scale = 1;
    app->loading_display.previous[0] = app->loading_display.previous[1] = -1;
    /* Fullscreen retains0044aac0's native Flip and alternating histories. */
    if (!app->fullscreen)
    {
        app->loading_display.present = loading_present;
        app->loading_display.present_user = app;
    }
    app->loading_progress.display = &app->loading_display;
    if (!bh_loading_progress_install(&app->loading_progress))
        return failure(app, 6);
    return 1;
}
static void sdl_input(const BHPlatformEvent *event, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    if (event->type == BH_PLATFORM_FOCUS && !event->down)
        app->focus_sample = 0;
    bh_input_event(&app->snapshot, event);
}

static BHTextureImage *image(unsigned long handle, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    BHTextureImage *p;
    for (p = app->render_mode.engine.textures.images; p; p = p->next)
        if ((unsigned long)p == handle)
            return p;
    return 0;
}
static int polygon(BHClipVertex *ring, BHTextureImage *texture, unsigned long color, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    int result;
    app->primitive.raster.pixels = app->display.color_pixels;
    app->primitive.raster.pitch = app->display.color_pitch;
    app->primitive.raster.depth = (unsigned short *)app->display.auxiliary_pixels;
    app->primitive.raster.depth_pitch = app->display.auxiliary_pitch;
    result = bh_primitive_emit(&app->primitive, texture, ring, color);
    return result;
}
/* 00455f70 dispatches ribbon vertices to 0046e430, whose shade comes from
   each vertex's scalar. The plain polygon callback would discard it. */
static int scalar_polygon(BHClipVertex *ring, BHTextureImage *texture, unsigned long color, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    int result;
    (void)color;
    app->primitive.raster.pixels = app->display.color_pixels;
    app->primitive.raster.pitch = app->display.color_pitch;
    app->primitive.raster.depth = (unsigned short *)app->display.auxiliary_pixels;
    app->primitive.raster.depth_pitch = app->display.auxiliary_pitch;
    result = bh_primitive_emit_scalar(&app->primitive, texture, ring);
    return result;
}
static int horizon_polygon(BHClipVertex *ring, BHTextureImage *texture, int gradient, const float rgb[3], void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    int result;
    unsigned long color = (unsigned long)rgb[0] * 0x10000UL + (unsigned long)rgb[1] * 0x100UL + (unsigned long)rgb[2];
    if (!gradient)
        return polygon(ring, texture, 0x80ffffffUL, user);
    app->primitive.raster.pixels = app->display.color_pixels;
    app->primitive.raster.pitch = app->display.color_pitch;
    app->primitive.raster.depth = (unsigned short *)app->display.auxiliary_pixels;
    app->primitive.raster.depth_pitch = app->display.auxiliary_pitch;
    result = bh_primitive_emit_gradient(&app->primitive, texture, ring, color);
    return result;
}
static int platform(BHGameApp *app)
{
    DDSURFACEDESC surface;
    BHSoftwareRaster raster;
    bh_game_settings_load(bh_sdl_settings_file(BH_GAME_SETTINGS_FILENAME));
    if (!bh_sdl_open(0, sdl_input, app))
        return failure(app, 3);
    strcpy(app->device_names[0].name, "SDL3 GPU");
    if (!bh_config_init(&app->config, 0) || !bh_config_bootstrap(&app->config, app->config_path, app->device_names, 1))
        return failure(app, 6);
    app->hardware = 1;
    app->fullscreen = 1; /* Internal flip path; SDL independently owns window mode. */
    if (DirectDrawCreate(0, &app->draw, 0) != DD_OK)
        return failure(app, 4);
    memset(&surface, 0, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface.dwWidth = app->width;
    surface.dwHeight = app->height;
    surface.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (IDirectDraw_CreateSurface(app->draw, &surface, &app->primary, 0) != DD_OK)
        return failure(app, 4);
    surface.ddsCaps.dwCaps = DDSCAPS_3DDEVICE;
    if (IDirectDraw_CreateSurface(app->draw, &surface, &app->target, 0) != DD_OK ||
        !bh_render_startup_hardware(&app->render_mode.engine, app->draw, app->target, app->primary, app->width,
                                    app->height, 16))
        return failure(app, 4);
    app->render_mode.screen = app->primary;
    app->render_mode.render = app->target;
    IDirectDrawSurface_AddRef(app->primary);
    IDirectDrawSurface_AddRef(app->target);
    if (app->hardware && !bh_texture_bind_display(&app->render_mode.engine.textures, &app->render_mode.engine.pixel))
        return failure(app, 4);
    if (!bh_view_config_init(&app->view, app->width, app->height) ||
        !bh_main_camera_settings(&app->view, bh_sdl_fov_degrees(), .05, 25000) ||
        !bh_main_camera_pose(&app->view, &app->main_camera, 0, 0, 0, 0, 0, 0) ||
        !bh_clip_init(&app->clip, app->view.near_z, app->view.far_z, 0, (float)app->width, 0, (float)app->height) ||
        !bh_screen_clip_init(&app->screen_clip, app->view.near_z, app->view.far_z, 0, (float)app->width, 0,
                             (float)app->height) ||
        !bh_attribute_clip_init(&app->attribute_clip, app->view.near_z, app->view.far_z, 0, (float)app->width, 0,
                                (float)app->height))
        return failure(app, 5);
    if (!bh_render_state_init(&app->render_state, &app->render_mode.engine.textures) ||
        !bh_horizon_color_init(&app->horizon_colors, &app->render_mode.engine.textures))
        return failure(app, 5);
    memset(&raster, 0, sizeof(raster));
    raster.width = app->width;
    raster.height = app->height;
    raster.near_z = app->view.near_z;
    raster.tables = &app->render_mode.engine.tables;
    raster.gradient = &app->horizon_colors;
    if (!bh_primitive_dispatch_init(&app->primitive, &app->render_state, &raster, 0, (float)app->width, 0,
                                    (float)app->height))
        return failure(app, 5);
    app->prototype.camera = &app->main_camera;
    app->prototype.config = &app->view;
    app->prototype.clip = &app->clip;
    app->prototype.emit = polygon;
    app->prototype.emit_user = app;
    app->prototype.light_config = &app->light;
    app->prototype.lights = &app->lights;
    app->prototype.light_reference = &app->zero;
    app->prototype.texture_mask = 0xffffffffUL;
    app->prototype.base_color = 0xff000000UL;
    if (!bh_model_context_init(&app->models, &app->render_mode.engine.textures, 0) ||
        !bh_model_path_set(&app->models, "bsp", 0) ||
        !bh_render_resources_init(&app->resources, &app->models, &app->prototype, 0) ||
        !bh_atlas_context_init(&app->atlas, &app->render_mode.engine.textures, 320))
        return failure(app, 5);
    app->billboard.camera = &app->main_camera;
    app->billboard.config = &app->view;
    app->billboard.clip = &app->screen_clip;
    app->billboard.emit = polygon;
    app->billboard.emit_user = app;
    app->oriented.path = app->path;
    app->oriented.capacity = N(app->path);
    app->oriented.clip = &app->attribute_clip;
    app->oriented.emit = scalar_polygon;
    app->oriented.emit_user = app;
    app->oriented.near_z = app->view.near_z;
    app->oriented.projection_scale = app->main_camera.focal;
    app->sprite_quad.camera = &app->main_camera;
    app->sprite_quad.clip = &app->clip;
    app->sprite_quad.directional = &app->light;
    app->sprite_quad.world_lights = &app->lights;
    app->sprite_quad.emit = polygon;
    app->sprite_quad.emit_user = app;
    app->screen_quad.view = &app->view;
    app->screen_quad.clip = &app->screen_clip;
    app->screen_quad.emit = polygon;
    app->screen_quad.emit_user = app;
    app->pickup_draw.queue = &app->queue;
    app->pickup_draw.camera = &app->main_camera;
    app->pickup_draw.view = &app->view;
    app->pickup_draw.clip = &app->screen_clip;
    app->pickup_draw.emit = polygon;
    app->pickup_draw.emit_user = app;
    app->pickup_draw.display_depth =
        app->width; /* Legacy field name: original004e2998 is viewport width, not bit depth. */
    app->resource_draw.camera = &app->main_camera;
    app->resource_draw.config = &app->view;
    app->resource_draw.billboard = &app->billboard;
    app->resource_draw.oriented = &app->oriented;
    app->model_submit.draw = &app->resource_draw;
    app->model_submit.lookup = bh_render_resources_lookup;
    app->model_submit.lookup_user = &app->resources;
    app->billboard_submit.draw = &app->billboard;
    app->billboard_submit.lookup = image;
    app->billboard_submit.lookup_user = app;
    app->screen_submit.draw = &app->screen_quad;
    app->screen_submit.lookup = image;
    app->screen_submit.lookup_user = app;
    app->quad_submit.draw = &app->sprite_quad;
    app->quad_submit.lookup = image;
    app->quad_submit.lookup_user = app;
    app->path_submit.view = &app->view;
    app->path_submit.camera = &app->main_camera;
    app->path_submit.draw = &app->oriented;
    app->path_submit.lookup = image;
    app->path_submit.lookup_user = app;
    app->segment_submit.camera = &app->main_camera;
    app->segment_submit.draw = &app->oriented;
    app->segment_submit.lookup = image;
    app->segment_submit.lookup_user = app;
    app->controls.view = &app->view;
    app->controls.camera = &app->main_camera;
    app->controls.portal_cache = &app->portal_cache;
    app->controls.resources = &app->resources;
    app->controls.model_clip = &app->clip;
    app->controls.screen_clip = &app->screen_clip;
    app->controls.attribute_clip = &app->attribute_clip;
    app->horizon.color_mode = app->render_mode.engine.bytes_per_pixel;
    app->horizon.camera = &app->main_camera;
    app->horizon.clip = &app->attribute_clip;
    app->horizon.state = &app->render_state;
    app->horizon.colors = &app->horizon_colors;
    app->horizon.emit = horizon_polygon;
    app->horizon.user = app;
    app->horizon_submit.draw = &app->horizon;
    app->horizon_submit.lookup = image;
    app->horizon_submit.lookup_user = app;
    app->flare_submit.camera = &app->main_camera;
    app->flare_submit.view = &app->view;
    app->flare_submit.width = app->width;
    app->flare_submit.height = app->height;
    app->flare_submit.screen = &app->screen_submit;
    app->display.textures = &app->render_mode.engine.textures;
    app->display.atlas = &app->atlas;
    app->display.color = app->target;
    app->display.auxiliary = app->render_mode.engine.depth;
    app->display.auxiliary_bytes = app->width * app->height * 2;
    app->fade.render = &app->render_state;
    app->fade.width = app->width;
    app->fade.height = app->height;
    app->fade.pixel_bytes = 2;
    app->fade.near_plane = app->view.near_z;
    app->fade.masks[0] = 0xf800;
    app->fade.masks[1] = 0x7e0;
    app->fade.masks[2] = 31;
    app->fade.extract[0] = 11;
    app->fade.extract[1] = 5;
    app->fade.expand[0] = app->fade.expand[2] = 3;
    app->fade.expand[1] = 2;
    app->fade.cached_amount = -1;
    app->present.fade = &app->fade;
    app->present.color = app->target;
    app->present.auxiliary = app->render_mode.engine.depth;
    app->begin.present = &app->present;
    app->gdi.display = &app->display;
    app->render.display = &app->display;
    app->render.text = &app->text;
    app->render.atlas = &app->atlas;
    app->render.draw = &app->queue;
    app->render.view = &app->view;
    app->render.camera = &app->main_camera;
    app->render.state = &app->render_state;
    app->render.horizon = &app->horizon_submit;
    app->render.image_lookup = image;
    app->render.image_user = app;
    app->render.model = &app->model_submit;
    app->render.billboard = &app->billboard_submit;
    app->render.controls = &app->controls;
    app->render.screen = &app->screen_submit;
    app->render.rect = &app->pickup_draw;
    app->render.plane = &app->quad_submit;
    app->render.path = &app->path_submit;
    app->render.segment = &app->segment_submit;
    app->render.fade = &app->fade;
    app->render.flare = &app->flare_submit;
    app->render.light = &app->light;
    app->render.gdi = &app->gdi;
    app->special_draw.queue = &app->queue;
    return 1;
}
static void binding(BHGameApp *app, unsigned long identity, unsigned long *destination)
{
    BHBootstrapBinding *b;
    if (app->binding_count >= N(app->bindings))
    {
        app->error = 1;
        return;
    }
    b = app->bindings + app->binding_count++;
    b->identity = identity;
    b->destination = destination;
}
#define B(id, field) binding(app, id, (unsigned long *)&app->field)
static void resource_bindings(BHGameApp *app)
{
    unsigned long i;
    B(0x4cdb88, text_capture.sound_resource);
    B(0x4cdb88, key_capture.sound_resource);
    B(0x4cdb88, menu_actions.denial_sound);
    /* Menus copy the action template before this sound loads. */
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
        B(0x4cdb88, bootstrap.menus[i].actions.denial_sound);
    B(0x4cdb88, player_tick.resources.menu_cue);
    B(0x496570, player_tick.resources.ui_cue);
    B(0x4cda14, player_tick.resources.no_weapon);
    B(0x4d1ab0, player_tick.resources.damage_warning);
    B(0x4d0474, player_tick.resources.hull_warning);
    B(0x4f554c, player_tick.resources.collision_side);
    B(0x4f550c, player_tick.resources.electro);
    B(0x4cda20, player_tick.resources.target_cue);
    B(0x4cda10, player_tick.resources.lock_cue);
    B(0x4f555c, player_tick.resources.ground_loop);
    B(0x4f5514, player_tick.resources.death_loop);
    B(0x4f5540, player_tick.resources.death_cue);
    B(0x4d195c, player_tick.resources.death_model);
    B(0x4cda18, player_tick.resources.cheat_model);
    B(0x4cdc98, player_tick.resources.boost_start);
    B(0x4d1904, player_tick.resources.boost_loop);
    B(0x4cdc74, player_tick.resources.boost_end);
    B(0x4f0028, player_tick.resources.smoke_image);
    B(0x4f0090, player_tick.resources.second_smoke);
    B(0x4f00a8, player_tick.resources.water_cloud);
    B(0x4f0058, player_tick.resources.water_ring);
    B(0x4f5544, player_create.engine_loop);
    B(0x4d1a6c, menu.move_sound);
    B(0x4d1a6c, menu.open_sound);
    B(0x4d1a70, menu.slide_sound);
    B(0x4d1a68, menu.press_sound);
    B(0x4d1a6c, menu.release_sound);
    B(0x4f00a0, flash.textures[2]);
    B(0x4f0098, flash.textures[0]);
    B(0x4f009c, flash.textures[1]);
    B(0x4f00a4, flash.textures[3]);
    B(0x4f0030, explosions.image);
    B(0x4f54f8, explosions.sound_resource);
    B(0x4df23c, debris.image);
    B(0x4f00bc, contacts.spark_texture);
    B(0x4f0088, contacts.quad_texture);
    B(0x4f00a8, contacts.line_texture);
    B(0x4f0058, contacts.rays_texture);
    B(0x4f0084, hit_markers.image);
    B(0x4d1aa0, player_scene.material_image);
    B(0x4f005c, player_scene.horizon_image);
    B(0x4f0040, player_scene.aim_image);
    B(0x4f00d0, player_scene.aim_overlay_image);
    B(0x4f54d8, player_scene.particle_image);
    B(0x4962f4, player_scene.confirmation_image);
    B(0x4f00cc, player_scene.trail_image);
    B(0x4f002c, player_scene.overlay_image);
    B(0x496338, player_scene.cockpit_resource);
    B(0x4f005c, flyby.horizon_image);
    B(0x4d4424, flyby.full_image);
    B(0x4f0068, flyby.corner_left_image);
    B(0x4f0080, flyby.corner_right_image);
    B(0x4d1960, flyby.scenario_resources[0]);
    B(0x4d1974, flyby.scenario_resources[1]);
    B(0x4d1978, flyby.scenario_resources[2]);
    B(0x4d1a98, background.first_model);
    B(0x4df238, background.second_model);
    B(0x4f0078, background.first_flare);
    B(0x4f007c, background.second_flare);
    B(0x4f0028, emitters.fountain_texture);
    B(0x4f00a8, emitters.steam_texture);
    B(0x4f00c0, hud.background_image);
    B(0x4f54e4, world_hud.atlas_image);
    B(0x4f0054, world_hud.weapon_icons_image);
    B(0x4f54e4, flat_hud.atlas_image);
    B(0x4f0054, flat_hud.weapon_icons_image);
    B(0x4f00b4, markers.brackets_image);
    B(0x4f54e4, pickup_draw.atlas_image);
    B(0x4f00b0, pickup_draw.glow_image);
    B(0x4f00b0, special_draw.glow_image);
    B(0x4cdb9c, markers.status_image);
    B(0x4f00b8, attachment.default_image);
    B(0x4f006c, attachment.flag200_image);
    B(0x4f0070, attachment.flag400_image);
    B(0x4f0074, attachment.flag800_image);
    B(0x4f54dc, radar.marker_image);
    B(0x4f00b4, radar.bar_image);
    B(0x4d1918, radar_query.retired_sound);
    B(0x4f00ac, projectile_draw.ballistic_flare);
    B(0x4f007c, projectile_draw.ballistic_trail);
    B(0x4f0094, projectile_draw.homing_flare);
    B(0x4f003c, projectile_draw.homing_trail);
    B(0x4f0034, projectile_draw.pulse_image);
    B(0x4f5548, physics_effects.impact_sound);
    B(0x4f54fc, physics_effects.quad_sound);
    B(0x4f54fc, physics_effects.rays_sound);
    B(0x4f5560, pickups.take_sound);
    B(0x4d0478, wagon.model);
    B(0x4d1a64, wagon.sound_resource);
    app->bootstrap.bindings = app->bindings;
    app->bootstrap.binding_count = app->binding_count;
}
#undef B
static int world_ready(BHGameApp *a);
static int player_ready(BHGameApp *a);
static int resources_ready(BHGameApp *a);
static int runtime_owners(BHGameApp *a);
static int compose(BHGameBootstrap *bootstrap, unsigned long phase, void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    (void)bootstrap;
    switch (phase)
    {
    case 1:
        return world_ready(app);
    case 2:
        return player_ready(app);
    case 3:
        return 1;
    case 5:
        app->menu_present.frame_resource = bh_bootstrap_value(&app->bootstrap, 0x496304);
        app->flyby_camera.credit_font = app->bootstrap.menu_font;
        return app->flyby_camera.credit_font != 0;
    case 4:
        return resources_ready(app);
    default:
        return failure(app, 1);
    }
}
static void network_menu_start(void *user, const char *address);
static int source_owners(BHGameApp *app)
{
    unsigned long i;
    app->actor_route.live_tick = &app->player_state.tick;
    app->vehicle_draw.clock = &app->actor_route;
    app->script_draw.clock = &app->actor_route;
    if (!bh_player_input_owner_init(&app->inputs, 0) ||
        !bh_input_collection_init(&app->collection, &app->snapshot, &app->inputs) ||
        !bh_input_devices_bind(&app->devices, &app->collection, 0))
        return failure(app, 6);
    app->profile = app->config.profiles;
    app->collection.disable_mouse_motion = 1;
    if (!bh_input_devices_start(&app->devices))
        return failure(app, 6);
    if (!bh_entities_init(&app->pool, 0x2580, 0) || !bh_sound_queue_init(&app->sound, 0, 0, 0) ||
        !bh_rail_paths_init(&app->rails, 0) || !bh_navigation_init(&app->navigation, 0))
        return failure(app, 6);
    bh_text_queue_init(&app->text);
    if (!bh_draw_queue_init(&app->queue, 3500, 0) || !bh_frame_buffers_bind(&app->frames, 0) ||
        !bh_script_particles_init(&app->particles, &app->pool, &app->queue) ||
        !bh_contact_effects_init(&app->contacts, &app->pool, &app->queue, 0, 0, 0, 0) ||
        !bh_collision_model_init(&app->player_collision, 0) ||
        !bh_weapon_factory_open(&app->weapons, &app->pool, &app->sound))
        return failure(app, 6);
    app->sounds.queue = &app->sound;
    app->cd.last_track = -1;
    app->cd_notify.cd = &app->cd;
    app->cd_audio.notify = &app->cd_notify;
    app->cd_audio.music_path = "MUSIC_DATA/track02.ogg";
    app->cd_audio.music_fallback_path = "MUSIC_DATA/track02.wav";
    app->media.audio = &app->cd_audio;
    bh_message_state_init(&app->messages);
    app->messages.panel_entity = 0;
    app->panel.pool = &app->pool;
    app->panel.messages = &app->messages;
    app->panel.text = &app->text;
    app->panel.sound = &app->sound;
    app->panel.draw = &app->queue;
    app->panel.screen_width = app->width;
    app->panel.near_depth = app->view.near_z;
    app->lifetime.timers.pool = &app->pool;
    app->lifetime.timers.actors = app->timer_actors;
    app->lifetime.sounds = &app->sound;
    app->motion.lifetime = &app->lifetime;
    app->sensors.lifetime = &app->lifetime;
    app->sensors.radar = app->actors;
    app->sensors.radar_count = N(app->actors);
    app->emitters.lifetime = &app->lifetime;
    app->emitters.particles = &app->particles;
    app->flash.pool = &app->pool;
    app->flash.queue = &app->queue;
    app->debris.pool = &app->pool;
    app->debris.queue = &app->queue;
    app->explosions.pool = &app->pool;
    app->explosions.queue = &app->queue;
    app->explosions.sound = &app->sound;
    app->explosions.debris = &app->debris;
    app->explosions.flash = &app->flash;
    app->hit_markers.pool = &app->pool;
    app->hit_markers.queue = &app->queue;
    app->pickups.pool = &app->pool;
    app->pickups.resources = app->pickup_resources;
    app->pickups.resource_count = N(app->pickup_resources);
    app->pickups.outside_actors = app->actors;
    app->pickups.sound = &app->sound;
    app->pickup_render.default_draw = &app->pickup_draw;
    app->pickup_render.special_draw = &app->special_draw;
    if (!bh_pickup_render_bind(&app->pickups, &app->pickup_render))
        return failure(app, 6);
    app->vm.lifetime = &app->lifetime;
    app->vm.motion = &app->motion;
    app->vm.sensors = &app->sensors;
    app->vm.emitters = &app->emitters;
    app->vm.explosions = &app->explosions;
    app->vm.contacts = &app->contacts;
    app->vm.flash = &app->flash;
    app->vm.script_actors = &app->script_actors;
    app->vm.targets = &app->targets;
    app->vm.pickups = &app->pickups;
    app->vm.launch = &app->actor_launch;
    app->vm.rails = &app->wagon;
    app->vm.radar = &app->radar_query;
    app->vm.actors = app->vm_actors;
    app->vm.waypoints = app->vm_starts;
    app->vm.waypoint_count = GAME_STARTS;
    app->parser.vm = &app->vm;
    app->parser.sounds = &app->sounds;
    app->parser.script_prefix = "bsp\\";
    app->parser.sound_prefix = "sound\\";
    if (!bh_script_blocks_init(&app->blocks, &app->parser, &app->resources, "bsp\\", BH_BSP_PREFIX_FIRST))
        return failure(app, 6);
    app->editor_world.pool = &app->pool;
    app->editor_world.library = &app->blocks;
    app->editor_world.queue = &app->queue;
    app->editor_world.sound = &app->sound;
    app->editor_world.nodes = app->nodes;
    app->editor_world.capacity = GAME_WORLD_CAPACITY;
    app->editor_world.input = &app->editor_state;
    app->editor_world.numeric = &app->numeric;
    app->editor_world.browser = &app->browser;
    app->editor_world.mode = 1;
    app->editor.world = &app->editor_world;
    app->editor.step_limit = 65535;
    app->numeric.selected = -1;
    app->browser.library = &app->blocks;
    app->browser.queue = &app->queue;
    app->browser.sound = &app->sound;
    app->browser.height = app->height;
    app->browser.selected = -1;
    app->world_load.editor = &app->editor;
    app->world_load.vm = &app->vm;
    app->world_load.rails = &app->rails;
    app->world_load.states = app->timer_actors;
    app->world_load.actor_capacity = GAME_WORLD_CAPACITY;
    app->editor_input.owner = &app->inputs;
    app->editor_input.pool = &app->pool;
    app->editor_input.state = &app->editor_state;
    app->editor_input.name = "";
    app->basis.samples = app->samples;
    app->basis.sample_capacity = N(app->samples);
    app->basis.groups_a = app->basis_a;
    app->basis.groups_b = app->basis_b;
    app->basis.indices_a = app->indices_a;
    app->basis.indices_b = app->indices_b;
    app->basis.scales_a = app->scales_a;
    app->basis.scales_b = app->scales_b;
    app->basis.a_capacity = app->basis.b_capacity = 7;
    app->player_force.basis = &app->basis;
    app->player_force.body = &app->physics_bodies[0];
    app->player_force.instability_counter = &app->instability;
    app->player_state.force = bh_player_force_control_b;
    app->physics_bodies[0].physics = &app->player_motion;
    app->physics_bodies[0].target_world = &app->player_motion.pose;
    app->physics_bodies[0].target_local = &app->player_inverse;
    app->player_inverse.m[0][0] = app->player_inverse.m[1][1] = app->player_inverse.m[2][2] = 1;
    app->weapon_dispatch.factory = &app->weapons;
    app->weapon_dispatch.ready = &app->weapon_ready;
    app->weapon_dispatch.target = &app->weapon_target;
    app->weapon_dispatch.arc = &app->weapon_arc;
    app->weapon_dispatch.empty = &app->weapon_empty;
    app->weapon_dispatch.fire = &app->weapon_fire;
    app->weapon_dispatch.pulse = &app->weapon_pulse;
    app->weapon_dispatch.effects = &app->weapon_effect;
    app->weapon_dispatch.pulse_launch = &app->weapon_launch;
    app->weapon_dispatch.arc_fire = &app->weapon_arc_fire;
    app->weapon_dispatch.arc_hud = &app->arc_hud;
    app->weapon_dispatch.target_hud = &app->target_hud;
    app->weapon_ready.sound = &app->sound;
    app->weapon_empty.sound = &app->sound;
    app->weapon_empty.messages = &app->messages;
    app->weapon_empty.warning_seen = &app->weapon_warning;
    app->player_camera.starts = app->starts;
    app->player_camera.start_count = GAME_STARTS;
    app->player_camera.camera = &app->camera_state;
    app->correction.query = &app->segment;
    app->player_reset.current_player = &app->player;
    app->player_reset.weapons = &app->weapons.registry;
    app->player_reset.sound = &app->sound;
    app->player_reset.frame_count = &app->player_state.death_frames;
    app->player_tick.pool = &app->pool;
    app->player_tick.state = &app->player_state;
    app->player_tick.physics = &app->physics;
    app->player_tick.body = &app->physics_bodies[0];
    app->player_tick.targets = &app->targets;
    app->player_tick.force = &app->player_force;
    app->player_tick.decoded = &app->decoded;
    app->player_tick.snapshot = &app->snapshot;
    app->player_tick.weapons = &app->weapon_dispatch;
    app->player_tick.camera = &app->player_camera;
    app->player_tick.reset = &app->player_reset;
    app->player_tick.scene = &app->player_scene;
    app->player_tick.panel = &app->panel;
    app->player_tick.world_reset = &app->world_reset;
    app->player_tick.menu = &app->menu;
    app->player_tick.media = &app->media;
    app->player_tick.sound = &app->sound;
    app->player_tick.world = &app->segment;
    app->player_tick.lifetime = &app->lifetime;
    app->player_tick.particles = &app->particles;
    app->player_tick.contacts = &app->contacts;
    app->player_tick.flash = &app->flash;
    app->player_tick.explosions = &app->explosions;
    app->player_tick.camera_yaw = &app->yaw;
    app->transport.game = &app->network_game;
    app->player_tick.game = &app->network_game;
    app->player_tick.network = &app->remote_messages;
    app->player_tick.scores = &app->network_scores;
    app->player_tick.navigation = &app->navigation;
    app->player_tick.latency = app->network_latency;
    app->player_tick.damage_source = &app->damage_source;
    app->player_create.tick = &app->player_tick;
    app->player_create.query = &app->player_query;
    app->player_create.current = &app->player;
    app->player_create.input_owner = &app->inputs;
    app->player_create.profile = app->profile;
    app->player_create.collision = &app->player_collision;
    app->player_create.correction = &app->correction;
    app->player_create.actors = app->actors;
    app->player_create.actor_capacity = N(app->actors);
    app->player_query.weapons = &app->weapons.registry;
    app->player_query.sound = &app->sound;
    app->player_query.messages = &app->messages;
    app->player_query.ready = &app->weapon_ready;
    app->player_query.target = &app->weapon_target;
    app->player_query.arc = &app->weapon_arc;
    app->player_query.lifetime = &app->lifetime;
    app->player_query.actors = &app->vehicle_lifetime.head;
    app->player_query.fine_warning = &app->fine_warning;
    app->player_query.damage_source = &app->damage_source;
    app->player_query.sound_velocity = &app->zero;
    app->player_query.transport = &app->transport;
    app->player_query.mode_count = &app->player_state.mode;
    app->player_query.simulation_tick = &app->player_state.tick;
    app->player_query.world = &app->network_world;
    app->player_query.world_index = &app->network_world_index;
    app->physics_effects.transport = &app->transport;
    app->world_reset.vm = &app->vm;
    app->world_reset.loot_rows = app->availability;
    app->physics_effects.effects = &app->contacts;
    app->physics_effects.sound = &app->sound;
    app->menu.pool = &app->pool;
    app->menu.snapshot = &app->snapshot;
    app->menu.sound = &app->sound;
    app->menu.transition = &app->transition;
    app->menu.flyby = &app->flyby;
    app->menu.mode = &app->player_state.mode;
    app->menu.paused = &app->player_state.paused;
    app->menu.waypoint = &app->selected_waypoint;
    app->menu.input_changed = &app->input_changed;
    app->menu.stay = &app->stay;
    app->menu_settings.input_profile = app->profile;
    app->menu_actions.devices = app->device_names;
    app->menu_actions.device_count = 1;
    app->menu_actions.config = &app->config;
    app->menu_actions.save_path = app->config_path;
    app->menu_actions.settings = &app->menu_settings;
    app->menu_actions.profiles = &app->menu_profiles;
    app->menu_actions.text = &app->text_capture;
    app->menu_actions.media = &app->media;
    app->menu_actions.messages = &app->messages;
    app->menu_actions.sound = &app->sound;
    app->menu_actions.player_name = app->bootstrap.player_name;
    app->menu_actions.network_address = app->network_address;
    app->menu_actions.network_ship = &app->network_ship;
    app->menu_actions.network_start = network_menu_start;
    app->menu_actions.network_user = app;
    app->menu_profiles.config = &app->config;
    app->menu_profiles.current = &app->profile;
    app->menu_profiles.pending_selection = &app->pending_profile;
    app->menu_edits.config = &app->config;
    app->menu_edits.settings = &app->menu_settings;
    app->menu_edits.audio = &app->cd_audio;
    app->menu_edits.capture = &app->key_capture;
    app->menu_edits.mode = &app->player_state.mode;
    app->menu_edits.mission = &app->mission;
    app->menu_edits.ship = &app->ship;
    app->menu_edits.unlock_all_levels = bh_game_settings_unlock_all_levels();
    app->transition.pool = &app->pool;
    app->transition.draw = &app->queue;
    app->transition.media = &app->media;
    app->transition.messages = &app->messages;
    app->transition.config = &app->config;
    app->transition.config_path = app->config_path;
    app->transition.hardware_flags = 1;
    app->transition.camera_index = &app->selected_waypoint;
    app->transition.quit = &app->quit;
    app->key_capture.pool = &app->pool;
    app->key_capture.snapshot = &app->snapshot;
    app->key_capture.state = &app->global_state;
    app->key_capture.settings = &app->menu_settings;
    app->key_capture.sound = &app->sound;
    app->key_capture.result = app->key_result;
    app->text_capture.buffer = app->text_buffer;
    app->text_capture.capacity = sizeof(app->text_buffer);
    app->text_capture.address_commit = network_menu_start;
    app->text_capture.address_user = app;
    app->text_capture.pool = &app->pool;
    app->text_capture.snapshot = &app->snapshot;
    app->text_capture.state = &app->global_state;
    app->text_capture.profile = &app->menu_profiles;
    app->text_capture.sound = &app->sound;
    app->flyby.entities = &app->pool;
    app->flyby.current_player = &app->player;
    app->flyby.queue = &app->queue;
    app->flyby.sound = &app->sound;
    app->flyby.background = &app->background;
    app->flyby.scene = &app->scene;
    app->flyby.input_owner = &app->inputs;
    app->flyby.input_profile = app->profile;
    app->flyby.player_camera = &app->player_camera;
    app->flyby.flyby_camera = &app->flyby_camera;
    app->flyby.flyby_state = &app->flyby_state;
    app->flyby.starts = app->starts;
    app->flyby.start_count = GAME_STARTS;
    app->flyby.mode_count = &app->player_state.mode;
    app->flyby.selected_waypoint = &app->selected_waypoint;
    app->flyby.scenario = &app->ship;
    app->flyby.player_slot = &app->mission;
    app->flyby.display_width = &app->width;
    app->flyby.display_height = &app->height;
    app->flyby.near_z = &app->view.near_z;
    app->flyby.event_value = &app->event_value;
    app->flyby.mission_identity = &app->mission_identity;
    app->flyby.force_control = &app->player_state.force;
    app->flyby_camera.route = &app->camera_route;
    app->flyby_camera.camera = &app->camera_state;
    app->flyby_camera.starts = app->starts;
    app->flyby_camera.start_count = GAME_STARTS;
    app->flyby_camera.text = &app->text;
    app->flyby_camera.credits = &app->credits;
    app->flyby_construct.scene = &app->flyby;
    app->flyby_construct.current_camera = &app->camera_entity;
    app->camera_route.navigation = &app->navigation;
    app->camera_route.waypoints = app->route_starts;
    app->camera_route.waypoint_count = GAME_STARTS;
    app->selected_waypoint = 10;
    app->mission_progress.config = &app->config;
    app->mission_progress.path = app->config_path;
    app->mission_progress.lifetime = &app->lifetime;
    app->story.progress = &app->mission_progress;
    app->story.messages = &app->messages;
    app->story.weapons = &app->weapons.registry;
    app->story.actors = &app->vehicle_lifetime.head;
    app->story.actor_limit = 9600;
    app->story.availability = app->availability;
    app->story.sound = &app->sound;
    app->story.force_control = &app->player_state.force;
    app->story.force_gate = &app->stay;
    app->story.ending = &app->player_state.ending;
    app->bootstrap.textures = &app->render_mode.engine.textures;
    app->bootstrap.resources = &app->resources;
    app->bootstrap.sounds = &app->sounds;
    app->bootstrap.atlas = &app->atlas;
    app->bootstrap.text = &app->text;
    app->bootstrap.blocks = &app->blocks;
    app->bootstrap.world = &app->world_load;
    app->bootstrap.editor = &app->editor_input;
    app->bootstrap.navigation = &app->navigation;
    app->bootstrap.player = &app->player_create;
    app->bootstrap.story = &app->story;
    app->bootstrap.availability = app->availability;
    app->bootstrap.inputs = &app->inputs;
    app->bootstrap.pool = &app->pool;
    app->bootstrap.snapshot = &app->snapshot;
    app->bootstrap.flyby = &app->flyby_construct;
    app->bootstrap.runtime = &app->menu;
    app->bootstrap.actions = &app->menu_actions;
    app->bootstrap.edits = &app->menu_edits;
    app->bootstrap.panel = &app->panel;
    app->bootstrap.transition = &app->transition;
    app->bootstrap.present = &app->menu_present;
    app->bootstrap.compose = compose;
    app->bootstrap.compose_user = app;
    app->menu_present.queue = &app->queue;
    app->menu_present.eye = &app->flyby_state.camera.position;
    app->menu_present.angles = &app->flyby_state.camera.angles;
    resource_bindings(app);
    for (i = 0; i < 8; ++i)
        app->background.slots[i].elapsed = -1;
    app->background.queue = &app->queue;
    return 1;
}
/* Placement metadata starts with BHWorldGeometry, but it is never a
   BHEntity. Resolve each relationship through the owning block/node table. */
static int world_ready(BHGameApp *app)
{
    unsigned long i, index_capacity, n = app->editor_world.count;
    BHEditorNode *node;
    BHScriptBlock *block;
    if (!n || n > GAME_WORLD_CAPACITY || app->blocks.count > N(app->block_queries))
        return failure(app, 7);
    app->motion_bindings = (BHScriptMotionBinding *)calloc(n, sizeof(*app->motion_bindings));
    app->motion.bindings = app->motion_bindings;
    app->motion.binding_count = n;
    app->rooms = (BHPickupRoom *)calloc(n, sizeof(*app->rooms));
    app->scheduled = (BHEntityScheduleRoom *)calloc(n, sizeof(*app->scheduled));
    app->scene_inputs = (BHSceneBindingInput *)calloc(n, sizeof(*app->scene_inputs));
    app->segment_rooms = (BHWorldSegmentRoom *)calloc(n, sizeof(*app->segment_rooms));
    app->physics_worlds = (BHPhysicsStepWorld *)calloc(n, sizeof(*app->physics_worlds));
    app->reset_nodes = (BHWorldResetNode *)calloc(n, sizeof(*app->reset_nodes));
    if (!app->motion_bindings || !app->rooms || !app->scheduled || !app->scene_inputs || !app->segment_rooms ||
        !app->physics_worlds || !app->reset_nodes)
        return failure(app, 7);
    for (i = 0; i < app->blocks.count; ++i)
        if (app->blocks.blocks[i].collision.geometry.bsp &&
            !bh_segment_query_init(app->block_queries + i, &app->blocks.blocks[i].collision.geometry))
            return failure(app, 7);
    for (i = 0; i < n; ++i)
    {
        node = app->nodes[i];
        if (!node || node->model_index >= app->blocks.count)
            return failure(app, 7);
        block = app->blocks.blocks + node->model_index;
        app->motion_bindings[i].entity = node->entity;
        app->motion_bindings[i].world = &node->placement.geometry;
        app->motion_bindings[i].draw_angles = &app->scheduled[i].angles;
        app->motion_bindings[i].attachments_owner = &app->rooms[i].pickups;
        app->scene_inputs[i].world = &node->placement.geometry;
        app->scene_inputs[i].resource = block->resource;
        app->scene_inputs[i].resource_flags = block->definition.flags;
        app->scene_inputs[i].angles = node->placement.angles;
        app->segment_rooms[i].world = &node->placement.geometry;
        app->segment_rooms[i].actors_owner = &app->rooms[i].actors;
        app->segment_rooms[i].query = block->collision.geometry.bsp ? app->block_queries + node->model_index : 0;
        app->segment_rooms[i].kind = node->entity->kind;
        app->physics_worlds[i].world = &node->placement.geometry;
        memcpy(&app->physics_worlds[i].mass, (char *)node->entity + 0xb4, 4);
        app->physics_worlds[i].response = node->entity;
        app->physics_worlds[i].index = (unsigned short)i;
        app->reset_nodes[i].entity = node->entity;
        app->reset_nodes[i].placement = &node->placement;
        app->reset_nodes[i].definition = &block->definition;
        app->reset_nodes[i].draw_angles = &app->scheduled[i].angles;
    }
    app->scene_view = app->view;
    app->schedule.queue = &app->queue;
    app->schedule.viewport = &app->scene_view;
    app->schedule.camera = &app->scene.camera;
    app->schedule.max_rooms = n;
    app->schedule.max_entities = app->pool.capacity;
    app->schedule.max_depth = n;
    if (!bh_scene_bindings_init(&app->scene_bindings, app->scene_inputs, n, app->rooms, app->scheduled, &app->resources,
                                &app->schedule) ||
        !bh_scene_owner_init(&app->scene, &app->scene_bindings, app->width, app->height) ||
        !bh_world_segment_init(&app->segment, &app->nodes[0]->placement.geometry, n, app->segment_rooms, n, 0, 0,
                               app->pool.capacity))
        return failure(app, 7);
    app->scene.view_fov_degrees = (float)bh_sdl_fov_degrees();
    for (index_capacity = 1; index_capacity < n * 2; index_capacity *= 2)
        ;
    app->segment_room_index = (BHWorldSegmentRoom **)calloc(index_capacity, sizeof(*app->segment_room_index));
    if (!app->segment_room_index ||
        !bh_world_segment_index_rooms(&app->segment, app->segment_room_index, index_capacity))
        return failure(app, 7);
    app->binding_allocator = app->pool.allocator;
    if (!bh_binding_init(&app->world_binding_index, n, &app->binding_allocator))
        return failure(app, 7);
    for (i = 0; i < n; ++i)
        if (!bh_binding_insert(&app->world_binding_index, app->segment_rooms[i].world, NULL, i))
            return failure(app, 7);
    app->actor_route.room_index = &app->world_binding_index;
    app->player_tick.room_index = &app->world_binding_index;
    app->projectile_effects.response_index = &app->world_binding_index;
    app->segment.refresh_actors = refresh_segment_actors;
    app->segment.refresh_user = app;
    for (i = 0; i < n; ++i)
    {
        app->timer_actors[i].active = 0;
        app->scheduled[i].activity = &app->timer_actors[i].active;
        app->scheduled[i].activity_entity = app->nodes[i]->entity;
    }
    app->world_reset.nodes = app->reset_nodes;
    app->world_reset.count = n;
    app->camera_route.first = app->segment.first;
    app->camera_route.scene_count = n;
    app->player_tick.rooms = app->rooms;
    app->player_tick.room_count = n;
    app->pickups.rooms = app->rooms;
    app->pickups.room_count = n;
    app->pickups.first_world = app->segment.first;
    app->pickups.world_count = n;
    for (i = 0; i < GAME_STARTS; ++i)
    {
        app->starts[i].position = app->vm_starts[i].position;
        app->starts[i].angles = app->vm_starts[i].angles;
        memcpy(app->starts[i].metadata, &app->vm_starts[i].speed, 8);
        app->route_starts[i] = app->vm_starts[i].position;
    }
    return 1;
}
static int player_ready(BHGameApp *app)
{
    if (app->actor_route.live_tick != &app->player_state.tick || app->vehicle_draw.clock != &app->actor_route ||
        app->script_draw.clock != &app->actor_route)
        return failure(app, 8);
    if (app->player_query.contact_physics != &app->player_motion)
        return failure(app, 8);
    if (!app->player || app->physics_bodies[0].entity != app->player ||
        app->physics_bodies[0].geometry != &app->player_collision.geometry ||
        bh_player_tick_context(app->player) != &app->player_tick)
        return failure(app, 8);
    app->base_models[0] = &app->player_collision.geometry;
    app->physics_bodies[0].model_index = 0;
    if (!bh_physics_step_init(&app->physics, &app->pool, &app->physics_bodies[0], 1, app->physics_worlds,
                              app->editor_world.count, app->base_models, 1, &app->physics_effects, 0, 0))
        return failure(app, 8);
    app->schedule.overlay_position = (const BHVec3 *)((char *)app->player + 0x160);
    app->player_scene.player = app->player;
    app->player_scene.queue = &app->queue;
    app->player_scene.sound = &app->sound;
    app->player_scene.hud = &app->hud;
    app->player_scene.markers = &app->markers;
    app->player_scene.radar = &app->radar;
    app->player_scene.world_status = &app->world_hud;
    app->player_scene.flat_status = &app->flat_hud;
    app->player_scene.score_draw = &app->score_draw;
    app->player_scene.remote_packets = &app->remote_packets;
    app->player_scene.scores = &app->scores;
    app->player_scene.background = &app->background;
    app->player_scene.scene = &app->scene;
    app->player_scene.weapons = &app->weapons.registry;
    app->player_scene.arc = &app->arc_hud;
    app->player_scene.target = &app->target_hud;
    app->player_scene.actor_head = &app->vehicle_lifetime.head;
    app->player_scene.pickup_head = &app->pickups.head;
    app->player_scene.mode_count = &app->player_state.mode;
    app->player_scene.frame_count = &app->player_state.death_frames;
    app->player_scene.display_width = &app->width;
    app->player_scene.display_height = &app->height;
    app->player_scene.confirmation_gate = &app->player_state.confirmation;
    app->player_scene.pickup_limit = app->pool.capacity;
    app->attachment.queue = &app->queue;
    app->attachment.frame = &app->radar_frame;
    app->attachment.viewport_width = app->width;
    app->hud.attachment = &app->attachment;
    app->hud.frame = &app->radar_frame;
    app->radar.queue = &app->queue;
    app->radar.frame = &app->radar_frame;
    app->radar_frame.near_z = app->view.near_z;
    app->markers.projection = &app->pickup_draw;
    app->markers.radar = &app->radar;
    app->markers.display_width = app->width;
    app->markers.limit = app->pool.capacity;
    app->world_hud.hud = &app->hud;
    return 1;
}
static int resources_ready(BHGameApp *app)
{
    unsigned long i;
    if (!runtime_owners(app))
        return 0;
    /* Independent original call-site keys and shared globals:00441d80,
      00430a30,00433b30;004dfb28/34/38. Check the actual application wiring. */
    if (app->area.hit_sound != bh_bootstrap_value(&app->bootstrap, 0x4f54f8) ||
        app->weapon_fire.homing_sound != bh_bootstrap_value(&app->bootstrap, 0x4d1894) ||
        app->weapon_fire.empty_sound != bh_bootstrap_value(&app->bootstrap, 0x4cda10))
        return failure(app, 9);
    if (app->projectiles.next_identity != &app->debris.origin_tag || app->area.identity != &app->debris.origin_tag ||
        app->pulse.identity != &app->debris.origin_tag || app->effect.debris != &app->debris)
        return failure(app, 9);
    if (app->weapon_target.actors_owner != &app->vehicle_lifetime.head ||
        app->weapon_arc.head_owner != &app->vehicle_lifetime.head ||
        app->pursuit.actor_head_owner != &app->vehicle_lifetime.head)
        return failure(app, 9);
    if (!image(app->player_scene.particle_image, app) ||
        app->player_scene.particle_image != bh_bootstrap_value(&app->bootstrap, 0x4f54d8) ||
        app->oriented.emit != scalar_polygon || app->oriented.projection_scale != app->main_camera.focal ||
        app->main_camera.focal <= 1)
        return failure(app, 9);
    /* Every destination must receive the value published by the bootstrap;
      identities stay keys and are never interpreted as source pointers. */
    for (i = 0; i < app->binding_count; ++i)
        if (*app->bindings[i].destination != bh_bootstrap_value(&app->bootstrap, app->bindings[i].identity))
            return failure(app, 9);
    if (app->bootstrap.menus_created != BH_BOOTSTRAP_MENUS || !app->player || !app->camera_entity ||
        app->physics.pool != &app->pool || !bh_entities_valid(&app->pool))
        return failure(app, 9);
    return 1;
}
#define V(id) bh_bootstrap_value(&app->bootstrap, id)
static int runtime_owners(BHGameApp *app)
{
    unsigned long i, j, n = app->editor_world.count;
    long city;
    BHHomingProjectileContext *ctx = &app->projectile_effects;
    app->world_responses = (BHHomingWorldResponse *)calloc(n, sizeof(*app->world_responses));
    app->actor_rooms = (BHActorConstructRoom *)calloc(n, sizeof(*app->actor_rooms));
    app->city_ids = (long *)calloc(n, sizeof(*app->city_ids));
    if (!app->world_responses || !app->actor_rooms || !app->city_ids)
        return failure(app, 9);
    for (i = 0; i < n; ++i)
    {
        app->world_responses[i].world = app->rooms[i].world;
        app->world_responses[i].receiver = app->nodes[i]->entity;
        app->actor_rooms[i].room = app->rooms + i;
        memcpy(&city, (char *)app->nodes[i]->entity + 0x11c, 4);
        app->actor_rooms[i].bound_resource = (unsigned long)city;
        app->vm_actors[i].damage.world = app->rooms[i].world;
        app->vm_actors[i].damage.player = app->player;
    }
    for (i = 0; i < app->navigation.node_count; ++i)
    {
        for (j = 0; j < n; ++j)
            if (app->rooms[j].world == app->navigation.nodes[i].placement)
                break;
        if (j == n)
            return failure(app, 9);
        memcpy(app->city_ids + i, (char *)app->nodes[j]->entity + 0x11c, 4);
    }
    for (i = 0; i < N(app->actor_models); ++i)
    {
        app->actor_models[i] = V(0x4d1920 + i * 4);
        app->actor_sounds[i] = V(0x4d1a00 + i * 4);
    }
    for (i = 0; i < N(app->pickup_resources); ++i)
        app->pickup_resources[i] = V(0x4d19a0 + i * 4);
    app->sensors.player = app->player;
    app->story.camera = bh_player_camera_state(app->player);
    app->story.pickup_sound = V(0x4f5560);
    app->flat_hud.hud = &app->hud;
    app->flat_hud.viewport_height = app->height;
    app->arc_hud.queue = &app->queue;
    app->arc_hud.image = V(0x49632c);
    app->target_hud.projection = &app->pickup_draw;
    app->target_hud.viewport_width = app->width;
    app->target_hud.atlas_image = V(0x4f00b4);
    app->score_draw.page_image = V(0x4cdb9c);
    app->score_text.queue = &app->text;
    app->score_text.page = app->bootstrap.menu_font;
    app->scores.remote = app->remote;
    app->scores.local_name = app->bootstrap.player_name;
    app->score_draw.queue = &app->queue;
    app->score_draw.display_width = app->width;
    app->score_draw.near_z = app->view.near_z;
    app->projectile_lifetime.pool = &app->pool;
    app->projectile_lifetime.sound = &app->sound;
    app->projectile_draw.queue = &app->queue;
    ctx->pool = &app->pool;
    ctx->segment = &app->segment;
    ctx->sound = &app->sound;
    ctx->particles = &app->particles;
    ctx->contacts = &app->contacts;
    ctx->flash = &app->flash;
    ctx->markers = &app->hit_markers;
    ctx->responses = app->world_responses;
    ctx->response_count = n;
    ctx->hit_sound = V(0x4f5564);
    ctx->liquid_sound = V(0x4f54fc);
    ctx->first_image = V(0x4f0090);
    ctx->second_image = V(0x4f0030);
    app->hit_markers.owner_head = marker_room_head;
    app->hit_markers.owner_head_user = app;
    app->ballistic.bindings = *ctx;
    app->ballistic.ricochet_sound = V(0x4f550c);
    app->projectiles.lifetime = &app->projectile_lifetime;
    app->projectiles.ballistic = &app->ballistic;
    app->projectiles.homing = &app->homing;
    app->projectiles.draw = &app->projectile_draw;
    app->projectiles.next_identity = &app->debris.origin_tag;
    app->projectiles.loop_sound = V(0x4f5504);
    app->projectiles.launch_sound = V(0x4f5510);
    app->area.effects = ctx;
    app->area.draw = &app->projectile_draw;
    app->area.identity = &app->debris.origin_tag;
    app->area.loop_sound = V(0x4962f0);
    app->area.hit_sound = V(0x4f54f8);
    app->weapon_fire.projectiles = &app->projectiles;
    app->weapon_fire.area = &app->area;
    app->weapon_fire.sound = &app->sound;
    app->weapon_fire.transport = &app->transport;
    app->weapon_fire.local = app->player;
    app->weapon_fire.local_id = -1;
    app->weapon_pulse.transport = &app->transport;
    app->weapon_pulse.local_id = -1;
    app->pulse.transport = &app->transport;
    /* The original keeps the local impact ID separate from replica attribution. */
    app->pulse.sync_identity = &app->sync_identity;
    app->effect.pending_identity = &app->sync_identity;
    app->effect_network.transport = &app->transport;
    app->effect.network = &app->effect_network;
    app->actor_launch.transport = &app->transport;
    app->actor_launch.navigation = &app->navigation;
    app->weapon_fire.messages = &app->messages;
    app->weapon_fire.empty_warning = &app->empty_warning;
    app->weapon_fire.ballistic_sound = V(0x4d0480);
    app->weapon_fire.homing_sound = V(0x4d1894);
    app->weapon_fire.empty_sound = V(0x4cda10);
    app->weapon_fire.area_sound = V(0x4ce448);
    app->weapon_pulse.effects = *ctx;
    app->weapon_pulse.debris = &app->debris;
    app->weapon_pulse.shot_sound = V(0x4d0480);
    app->weapon_pulse.smoke_image = V(0x4f0028);
    app->weapon_pulse.flare_image = V(0x4f0030);
    app->weapon_pulse.trail_image = V(0x4f0060);
    app->homing = *ctx;
    app->homing.hit_sound = V(0x4f550c);
    app->homing.first_image = V(0x4f003c);
    app->homing.second_image = V(0x4f0094);
    app->weapon_ready.sound_resource = V(0x4cda10);
    app->weapon_empty.shift_sound = V(0x4cda10);
    app->weapon_target.sound = &app->sound;
    app->weapon_target.world = &app->segment;
    app->weapon_target.max_actors = app->pool.capacity;
    app->weapon_target.ready_sound = V(0x4cda20);
    app->weapon_target.beep_sound = V(0x4cdb88);
    app->weapon_target.lock_sound = V(0x4cdb8c);
    app->weapon_arc.sound = &app->sound;
    app->weapon_arc.world = &app->segment;
    app->weapon_arc.limit = app->pool.capacity;
    app->weapon_effect.fire = &app->weapon_fire;
    app->weapon_effect.projectile = &app->effect;
    app->weapon_effect.split_model = V(0x4cdc70);
    app->weapon_effect.shot_sound = V(0x4d1894);
    app->weapon_launch.fire = &app->weapon_fire;
    app->weapon_launch.projectile = &app->pulse;
    app->weapon_launch.sound = V(0x4cda20);
    app->weapon_arc_fire.fire = &app->weapon_fire;
    app->weapon_arc_fire.particles = &app->particles;
    app->weapon_arc_fire.flash = &app->flash;
    app->weapon_arc_fire.first_image = V(0x496334);
    app->weapon_arc_fire.second_image = V(0x4f0094);
    app->weapon_arc_fire.sound = V(0x4ce448);
    app->effect.effects = ctx;
    app->effect.cleanup.pool = &app->pool;
    app->effect.cleanup.sound = &app->sound;
    app->effect.debris = &app->debris;
    app->effect.explosion = &app->explosions;
    app->effect.draw = &app->effect_draw;
    app->effect.trails = &app->trails;
    app->effect.orbit = &app->orbit;
    app->effect.accelerated = &app->accelerated;
    app->effect.split = &app->split;
    app->orbit.core = &app->effect;
    app->accelerated.puffs = &app->puffs;
    app->puffs.queue = &app->queue;
    app->effect_draw.queue = &app->queue;
    app->trails.pool = &app->pool;
    app->trails.queue = &app->queue;
    app->pulse.effects = ctx;
    app->pulse.lifetime = &app->projectile_lifetime;
    app->pulse.draw = &app->pulse_draw;
    app->pulse.debris = &app->debris;
    app->pulse.explosion = &app->explosions;
    app->pulse.identity = &app->debris.origin_tag;
    app->pulse_draw.queue = &app->queue;
    if (!bh_debris_context_init(&app->debris_response, &app->pool, &app->sound, V(0x496218), app->actors,
                                N(app->actors)))
        return failure(app, 9);
    app->target_draw.queue = &app->queue;
    app->target_draw.camera = &app->main_camera;
    app->target_draw.view = &app->view;
    app->target_draw.player = app->player;
    app->target_draw.screen_width = app->width;
    app->target_draw.atlas_image = V(0x4f54e4);
    app->target_draw.marker_image = V(0x4f00b4);
    app->targets.debris = &app->debris_response;
    app->targets.physics = &app->physics;
    app->targets.effects = &app->physics_effects;
    app->targets.explosions = &app->explosions;
    app->targets.particles = &app->particles;
    app->targets.pickups = &app->pickups;
    app->targets.draw = &app->target_draw;
    app->targets.mode = &app->editor_world.mode;
    app->targets.smoke_image = V(0x4f0028);
    app->targets.slots = app->target_slots;
    app->targets.capacity = GAME_DYNAMIC_CAPACITY;
    app->targets.roster = app->physics_bodies;
    app->targets.models = app->physics_models;
    app->targets.base_bodies = app->physics_bodies;
    app->targets.base_count = 1;
    app->targets.base_models = app->base_models;
    app->targets.base_model_count = 1;
    app->targets.worlds = app->physics_worlds;
    app->targets.world_count = n;
    app->targets.resources = app->pickup_resources;
    app->targets.resource_count = N(app->pickup_resources);
    app->targets.prefix = "bsp\\";
    app->targets.search = BH_BSP_PREFIX_FIRST;
    app->actor_route.rooms = app->rooms;
    app->actor_route.room_count = n;
    app->actor_route.actor_limit = app->pool.capacity;
    app->actor_route.sound = &app->sound;
    app->vehicle_lifetime.pool = &app->pool;
    app->vehicle_lifetime.route = &app->actor_route;
    app->actor_response.damage = &app->actor_damage;
    app->actor_response.attack = &app->actor_attack;
    app->actor_response.vehicles = &app->vehicle_lifetime;
    app->actor_response.mission = &app->lifetime;
    app->actor_response.player = app->player;
    /* Original wreck angles are unwritten stack data. Use an explicit stable
      zero orientation adapter for that death-only undefined input. */
    app->actor_damage.legacy_death_angles = &app->zero;
    app->actor_damage.pool = &app->pool;
    app->actor_damage.route = &app->actor_route;
    app->actor_damage.particles = &app->particles;
    app->actor_damage.fragments = &app->debris;
    app->actor_damage.targets = &app->targets;
    app->actor_damage.loot_rows = app->availability;
    app->actor_damage.loot_row_count = 4;
    app->actor_damage.smoke_resource = V(0x4f0028);
    app->actor_damage.death_sound = V(0x4f5540);
    app->actor_response.mission_resource = V(0x4d1918);
    app->actor_attack.pursuit = &app->pursuit;
    app->actor_attack.effects = &app->effect;
    app->actor_attack.projectiles = &app->projectiles;
    app->actor_attack.area = &app->area;
    app->pursuit.route = &app->actor_route;
    app->pursuit.navigation = &app->navigation;
    app->pursuit.player = app->player;
    app->pursuit.actor_limit = app->pool.capacity;
    app->pursuit.city_ids = app->city_ids;
    app->pursuit.messages = &app->messages;
    app->weapon_target.actors_owner = &app->vehicle_lifetime.head;
    app->weapon_arc.head_owner = &app->vehicle_lifetime.head;
    app->pursuit.actor_head_owner = &app->vehicle_lifetime.head;
    app->pursuit.city_names = city_names;
    app->pursuit.city_count = N(city_names);
    app->vehicle_draw.queue = &app->queue;
    app->vehicle_draw.sound = &app->sound;
    app->vehicle_draw.damage_resource = V(0x4f0028);
    app->actor_construct.response = &app->actor_response;
    app->actor_construct.draw = &app->vehicle_draw;
    app->actor_construct.first = app->segment.first;
    app->actor_construct.world_count = n;
    app->actor_construct.rooms = app->actor_rooms;
    app->actor_construct.room_count = n;
    app->actor_construct.models = app->actor_models;
    app->actor_construct.sounds = app->actor_sounds;
    app->actor_construct.resource_count = N(app->actor_models);
    app->actor_construct.cycle_models = app->cycle_models;
    app->actor_construct.cycle_count = N(app->cycle_models);
    app->actor_launch.actors = &app->actor_construct;
    app->script_actors.first_world = app->segment.first;
    app->script_actors.world_count = n;
    app->script_actors.callbacks.pool = &app->pool;
    app->script_actors.callbacks.route = &app->actor_route;
    app->script_actors.callbacks.draw = &app->script_draw;
    app->script_draw.queue = &app->queue;
    app->script_draw.particles = &app->particles;
    app->script_actors.model_resources[0] = V(0x4d1890);
    app->script_actors.model_resources[1] = V(0x4d19f8);
    app->script_actors.model_resources[2] = V(0x4d18e4);
    app->script_actors.model_resources[3] = V(0x4961ec);
    app->script_actors.sound_resource = V(0x4d1a64);
    app->radar_query.pool = &app->pool;
    app->radar_query.lifetime = &app->lifetime;
    app->script_actors.callbacks.pulse_resource = V(0x4d441c);
    app->wagon.rails = &app->rails;
    app->wagon.pool = &app->pool;
    app->wagon.first_world = app->segment.first;
    app->wagon.world_count = n;
    app->wagon.rooms = app->rooms;
    app->wagon.room_count = n;
    app->wagon.queue = &app->queue;
    app->wagon.sound = &app->sound;
    app->player_query.contact_side = V(0x4f554c);
    app->player_query.contact_axial = V(0x4f5558);
    app->player_query.primary_cue = V(0x4961e8);
    app->player_query.secondary_cue = V(0x4962fc);
    /* These keys come from the consuming owners' retained original call sites,
      not from the load-table order (sound handle zero is a valid resource). */
    app->effect.blast_sound = V(0x4f5548);
    app->effect.impact_sound = V(0x496218);
    app->effect.ricochet_sound = V(0x4f550c);
    app->effect.loop_sound = V(0x4f5504);
    app->effect.second_loop_sound = V(0x49633c);
    app->effect.smoke_image = V(0x4f0028);
    app->effect.trail_image = V(0x4f004c);
    app->effect_draw.directional_model = V(0x496340);
    app->effect_draw.fixed_model = V(0x496300);
    app->effect_draw.timed_texture = V(0x4f0094);
    app->effect_draw.random_texture = V(0x4f008c);
    app->effect_draw.fixed_texture = V(0x4f00b0);
    app->accelerated.ring_image = V(0x4f0050);
    app->accelerated.second_ring_image = V(0x4f0064);
    app->accelerated.random_image = V(0x4f0090);
    app->accelerated.interval_sound = V(0x4df234);
    app->accelerated.expiry_sound = V(0x4ce448);
    app->split.split_sound = V(0x496218);
    app->weapon_arc.activate_sound = V(0x4cda20);
    app->weapon_arc.release_sound = V(0x4cdc74);
    app->weapon_arc.charge16_sound = V(0x4cdc98);
    app->weapon_arc.charge51_sound = V(0x4d1904);
    app->weapon_effect.shot_sound = V(0x4cda20);
    app->projectiles.loop_sound = V(0x4f5510);
    app->projectiles.launch_sound = V(0x4f550c);
    app->projectile_draw.ballistic_flare = V(0x4f00ac);
    app->projectile_draw.ballistic_trail = V(0x4f007c);
    app->projectile_draw.homing_flare = V(0x4f0094);
    app->projectile_draw.homing_trail = V(0x4f003c);
    app->projectile_draw.pulse_image = V(0x4f0034);
    app->pulse.loop_sound = V(0x4df234);
    app->pulse.bounce_sound = V(0x4f5558);
    app->pulse_draw.model = V(0x4d0470);
    app->pulse_draw.image = V(0x4f0034);
    for (i = 0; i < N(app->cycle_models); ++i)
        app->cycle_models[i] = V(0x496578 + i * 4);
    app->actor_construct.special_model = V(0x4d047c);
    app->vehicle_draw.glowing_model = V(0x4d1940);
    app->vehicle_draw.glow_first = V(0x4f0078);
    app->vehicle_draw.glow_second = V(0x4f008c);
    app->script_draw.smoke_resource = V(0x4f0028);
    app->script_draw.glowing_model = V(0x4d18e4);
    app->script_draw.glow_first = V(0x4f008c);
    app->script_draw.glow_second = V(0x4f00b0);
    app->actor_attack.orbit_cue = V(0x496be0);
    app->actor_attack.area_sound = V(0x4ce448);
    app->actor_attack.ballistic_sound = V(0x4d0480);
    app->actor_attack.homing_sound = V(0x4d1894);
    app->actor_attack.impact_sound = V(0x4f5564);
    app->actor_attack.liquid_sound = V(0x4f54fc);
    app->actor_attack.split_model = V(0x4cdc70);
    app->actor_attack.smoke_image = V(0x4f0028);
    app->actor_attack.impact_image = V(0x4f0090);
    app->actor_attack.flash_image = V(0x4f0030);
    app->actor_attack.beam_image = V(0x4f0060);
    app->panel.flash_image = (unsigned long)bh_atlas_page_image(app->panel.page);
    app->panel.alert_image = V(0x4f00b4);
    /* 004225ce: ordinary completion; 004225a3: chapter progression. */
    app->mission_progress.first_sound = V(0x496bdc);
    app->mission_progress.unlocked_sound = V(0x4d18e0);
    return 1;
}
#undef V
static int network_bind(BHGameApp *app)
{
    unsigned long i, count = app->editor_world.count;
    if (!app->network_positions.nodes)
    {
        if (!count || count > 65535)
            return 0;
        app->network_positions.nodes = calloc(count, sizeof(*app->network_positions.nodes));
        app->effect_indices = calloc(count, sizeof(*app->effect_indices));
        app->pulse_indices = calloc(count, sizeof(*app->pulse_indices));
        if (!app->network_positions.nodes || !app->effect_indices || !app->pulse_indices)
        {
            free(app->network_positions.nodes);
            free(app->effect_indices);
            free(app->pulse_indices);
            app->network_positions.nodes = NULL;
            app->effect_indices = NULL;
            app->pulse_indices = NULL;
            return 0;
        }
        /* The route graph excludes some rooms where a player can spawn or fly.
           Wire position indices cover every room in level order. */
        app->network_positions.node_count = count;
        app->player_tick.navigation = &app->network_positions;
        app->actor_launch.navigation = &app->network_positions;
        for (i = 0; i < count; ++i)
        {
            app->network_positions.nodes[i].placement = app->rooms[i].world;
            app->effect_indices[i].world = app->pulse_indices[i].world = app->rooms[i].world;
            app->effect_indices[i].index = app->pulse_indices[i].index = (unsigned short)i;
        }
    }
    app->effect_network.world_indices = app->effect_indices;
    app->effect_network.world_count = count;
    app->pulse.world_indices = app->pulse_indices;
    app->pulse.world_count = count;
    app->network_scores = (BHNetworkScores){
        .scores = &app->scores, .text = &app->score_text, .transport = &app->transport, .local_player = app->player};
    app->remote_messages = (BHRemoteMessages){.packets = &app->remote_packets,
                                              .scores = &app->network_scores,
                                              .messages = &app->messages,
                                              .transport = &app->transport,
                                              .sound = &app->sound,
                                              .contacts = &app->contacts,
                                              .explosions = &app->explosions,
                                              .effects = &app->network_effects,
                                              .launch = &app->actor_launch,
                                              .latency = app->network_latency,
                                              .peers = &app->player_state.peers};
    app->remote_packets = (BHRemotePackets){.pool = &app->pool,
                                            .remote = app->remote,
                                            .head = &app->vehicle_lifetime.head,
                                            .count = &app->vehicle_lifetime.count,
                                            .network = &app->network_game,
                                            .query = &app->remote_query,
                                            .render = &app->remote_render,
                                            .car_models = app->flyby.scenario_resources,
                                            .tick = &app->remote_tick,
                                            .simulation_tick = &app->player_state.tick,
                                            .local_slot = &app->slot,
                                            .local = app->player,
                                            .spawn_world = &app->remote_spawn_world,
                                            .rooms = app->rooms,
                                            .room_count = app->editor_world.count,
                                            .navigation = &app->network_positions};
    app->remote_query = (BHRemoteQuery){.pool = &app->pool,
                                        .rooms = app->rooms,
                                        .room_count = app->editor_world.count,
                                        .head = &app->vehicle_lifetime.head,
                                        .count = &app->vehicle_lifetime.count,
                                        .remote = app->remote,
                                        .local = app->player,
                                        .worlds = app->effect_indices,
                                        .world_count = count,
                                        .transport = &app->transport,
                                        .sound = &app->sound};
    app->remote_render = (BHRemoteRender){.queue = &app->queue,
                                          .sound = &app->sound,
                                          .spark_texture = bh_bootstrap_value(&app->bootstrap, 0x4f0094),
                                          .wreck_model = app->player_tick.resources.death_model};
    app->remote_tick = (BHRemoteTick){.fire = &app->remote_fire,
                                      .world = &app->segment,
                                      .particles = &app->particles,
                                      .sound = &app->sound,
                                      .smoke_image = app->player_tick.resources.smoke_image};
    app->remote_fire = (BHRemoteFire){.pulse = &app->weapon_pulse,
                                      .projectiles = &app->projectiles,
                                      .effects = &app->effect,
                                      .area = &app->area,
                                      .pulse_projectile = &app->pulse,
                                      .arc = &app->arc_hud,
                                      .local = app->player,
                                      .tick = &app->player_state.tick,
                                      .sync_identity = &app->debris.origin_tag,
                                      .ballistic_sound = app->weapon_fire.ballistic_sound,
                                      .homing_sound = app->weapon_fire.homing_sound,
                                      .target_sound = app->weapon_launch.sound,
                                      .area_sound = app->weapon_fire.area_sound,
                                      .arc_sound = app->weapon_arc_fire.sound,
                                      .special_sound = app->weapon_effect.shot_sound,
                                      .arc_image = app->weapon_arc_fire.first_image,
                                      .arc_flare = app->weapon_arc_fire.second_image,
                                      .split_model = app->weapon_effect.split_model};
    app->network_effects.navigation = &app->network_positions;
    app->network_effects.pool = &app->pool;
    for (i = 0; i < 8; ++i)
        app->remote_packets.engine_sounds[i] = bh_bootstrap_value(&app->bootstrap, 0x4f5544);
    return 1;
}

static void network_menu_status(BHGameApp *app, unsigned long row, const char *status)
{
    unsigned char body[40];
    snprintf((char *)body, sizeof(body), "%s", status);
    bh_menu_row_refresh(app->menu.network_menu->action_context->rows, row, NULL, body);
    if (app->network_log)
    {
        fprintf(app->network_log, "menu_status row=%lu %s\n", row, status);
        fflush(app->network_log);
    }
}

static int network_start(BHGameApp *app, BHMenuRuntime *entry, const char *address, unsigned short port)
{
    if (app->network_game.host || app->player_state.mode)
        return 0;
    if (!bh_network_open(&app->network_game, address, port, app->bootstrap.player_name, app->network_ship))
        return 0;
    if (!network_bind(app))
    {
        bh_network_close(&app->network_game);
        return 0;
    }
    app->network_entry = entry;
    app->network_cancel_pressed = 0;
    app->network_world = NULL;
    app->scores.local_score = 0;
    app->player_state.peers = 0;
    app->player_state.send_delay = 0;
    memset(app->network_latency, 0, sizeof(app->network_latency));
    if (address)
    {
        entry->input_blocked = 1;
        bh_input_set_enabled(entry->input, &app->snapshot, 0);
    }
    if (app->network_log)
    {
        fprintf(app->network_log, "session_start source=%s role=%s address=%s port=%u car=%u\n",
                entry == app->menu.network_menu ? "menu" : "command-line", address ? "join" : "host",
                address ? address : "*", port, (unsigned int)app->network_game.local_car);
        fflush(app->network_log);
    }
    return 1;
}

static void network_menu_start(void *user, const char *address)
{
    BHGameApp *app = user;
    if (app->network_log)
    {
        fprintf(app->network_log, "menu_request address=%s\n", address ? address : "*");
        fflush(app->network_log);
    }
    if (!network_start(app, app->menu.network_menu, address, BH_NETWORK_PORT))
    {
        network_menu_status(app, address ? 1 : 0, address ? "Check host IPv4" : "Could not open UDP port");
        return;
    }
    if (address)
    {
        snprintf(app->network_address, sizeof(app->network_address), "%s", address);
        network_menu_status(app, 1, "Connecting...\nEsc to cancel");
    }
}

static void network_roster(unsigned int slot, void *user)
{
    BHGameApp *app = user;
    bh_remote_roster(&app->remote_messages, slot, app->network_game.hosting);
    if (app->network_log)
        fprintf(app->network_log, "roster slot=%u active=%d spawns=%lu error=%d\n", slot,
                app->network_game.players[slot].active, app->remote_packets.spawns, app->remote_messages.error);
}

static void network_message(void *bytes, unsigned long size, void *user)
{
    BHGameApp *app = user;
    if (app->network_game.hosting)
        bh_remote_host_message(&app->remote_messages, bytes, size, BH_NETWORK_PACKET_CAPACITY);
    else
        bh_remote_client_message(&app->remote_messages, bytes, size, BH_NETWORK_PACKET_CAPACITY);
}

static void network_trace(BHGameApp *app)
{
    unsigned int i;
    unsigned long model;
    FILE *log = app->network_log;
    if (!log)
        return;
    memcpy(&model, (char *)app->player + 0x21c, sizeof(model));
    fprintf(log,
            "frame=%lu mode=%ld slot=%ld pos=%.3f,%.3f,%.3f sent_pose=%lu pose=%lu sent_shot=%lu shot=%lu "
            "fire=%lu scores=%lu rejected=%lu remote_draw=%lu car=%u model=%lu\n",
            app->frame, app->player_state.mode, app->slot, (double)app->player->position.x,
            (double)app->player->position.y, (double)app->player->position.z, app->network_game.sent[4],
            app->network_game.received[4], app->network_game.sent[8], app->network_game.received[8],
            app->remote_fire.shots, app->network_game.received[12], app->network_game.rejected,
            app->remote_render.attempted, (unsigned int)app->network_game.local_car, model);
    for (i = 0; i < 8; ++i)
        if (app->remote[i])
        {
            BHVec3 position;
            if (bh_remote_predict_position(app->remote[i], (unsigned short)app->player_state.tick, &position))
                fprintf(log, "remote slot=%u pos=%.3f,%.3f,%.3f car=%u model=%lu\n", i, (double)position.x,
                        (double)position.y, (double)position.z, (unsigned int)app->network_game.players[i].car,
                        app->remote_render.models[i]);
        }
    fflush(log);
}

static int network_poll(BHGameApp *app)
{
    BHNetworkGame *game = &app->network_game;
    int cancelled = 0;
    if (!game->host)
        return 1;
    bh_network_receive(game, network_message, network_roster, app);
    if (!app->network_playing && !game->local_player)
    {
        int escape_down = app->snapshot.current[27] != 0;
        cancelled = app->network_cancel_pressed && !escape_down;
        app->network_cancel_pressed = escape_down;
        if (cancelled)
            game->disconnected = 1;
    }
    if (game->disconnected || (!app->player_state.mode && app->network_playing))
    {
        int was_playing = app->player_state.mode > 1;
        int join_failed = game->disconnected && !app->network_playing;
        const char *join_error = game->welcome_timed_out ? "Connection timed out" : "Could not reach host";
        BHMenuRuntime *entry = app->network_entry;
        network_trace(app);
        if (!bh_remote_clear(&app->remote_messages))
            return failure(app, 12);
        bh_network_close(game);
        app->slot = -1;
        app->network_playing = 0;
        app->network_entry = NULL;
        entry->input_blocked = 0;
        app->flyby.player_slot = &app->mission;
        app->flyby.scenario = &app->ship;
        if (was_playing && !bh_player_leave_network(&app->player_tick))
            return failure(app, 12);
        if (was_playing)
            bh_message_enqueue(&app->messages, "1Network session ended.");
        else if (join_failed)
        {
            bh_input_set_enabled(entry->input, &app->snapshot, 1);
            if (entry == app->menu.network_menu)
                network_menu_status(app, 1, cancelled ? "Connection cancelled" : join_error);
            else if (!cancelled)
                bh_message_enqueue(&app->messages, "1Could not join network session.");
        }
        if (app->network_log)
        {
            fprintf(app->network_log, "session_end mode=%ld\n", app->player_state.mode);
            fflush(app->network_log);
        }
        return 1;
    }
    if (game->local_player && app->player_state.mode == 0)
    {
        BHMenuRuntime *entry = app->network_entry;
        app->slot = (long)game->local_player - 1;
        app->flyby.player_slot = &app->slot;
        app->flyby.scenario = &app->network_ship;
        app->scores.local_index = app->slot;
        app->player_state.mode = game->hosting ? 2 : 3;
        app->network_scores.mode = app->player_state.mode;
        app->network_playing = 1;
        entry->input_blocked = 0;
        bh_input_set_enabled(entry->input, &app->snapshot, 0);
        entry->exiting = 1;
    }
    return 1;
}

static void network_tick(BHGameApp *app)
{
    int active = app->player_state.mode > 1 && app->slot >= 0;
    unsigned short tick = (unsigned short)app->player_state.tick;
    if (active)
    {
        memcpy(&app->network_world, (char *)app->player + 0xc4, 4);
        if (app->network_world)
            app->network_world_index = (unsigned short)bh_navigation_find(&app->network_positions, app->network_world);
    }
    app->actor_launch.mode = app->player_state.mode;
    app->physics_effects.peer_count = app->effect_network.peers = app->pulse.peers = active ? 2 : 0;
    app->weapon_fire.peers = app->weapon_pulse.peers = active ? 2 : 0;
    app->weapon_fire.sender = app->weapon_pulse.sender = app->physics_effects.sender = tick;
    app->weapon_fire.local_id = app->weapon_pulse.local_id = active ? app->slot : -1;
    app->remote_query.local_id = (unsigned long)app->slot;
    memcpy(app->weapon_fire.remote, app->remote, sizeof(app->remote));
    if (active && app->frame % 30 == 0)
        network_trace(app);
}

static void source_produce(void *user)
{
    BHGameApp *app = (BHGameApp *)user;
    unsigned long clock = app->frame + (app->player_state.paused == 0);

    if (!bh_entities_focus(&app->pool, app->player_state.paused ? app->player : 0))
    {
        failure(app, 12);
        return;
    }
    /* The service increments the original frame clock before entity callbacks. */
    app->pursuit.clock = app->vehicle_draw.animation_clock = (long)clock;
    app->script_draw.animation_time = (float)clock;
    app->service.packet = app->bootstrap.all_keys->state;
    app->service.packet_bytes = sizeof(app->bootstrap.all_keys->state);
    if (!bh_frame_service_tick(&app->service))
    {
        failure(app, 12);
    }
}
static int render_frame(BHGameApp *app, BHDrawQueue *queue)
{
    int ok;
    BHDrawQueue *saved;
    if (!restore_display(app))
        return 0;
    if (!bh_present_begin(&app->begin))
        return failure(app, 13);
    app->display.color_pixels = app->fade.pixels;
    app->display.color_pitch = app->fade.pitch;
    app->display.auxiliary_pixels = app->begin.auxiliary_pixels;
    app->display.auxiliary_pitch = app->begin.auxiliary_pitch;
    app->display.locked = app->present.locked;
    bh_sdl_canvas_margins(&app->view.margin_x, &app->view.margin_y);
    app->view.left = app->clip.left = app->screen_clip.left = app->attribute_clip.left = -app->view.margin_x;
    app->view.right = app->clip.right = app->screen_clip.right = app->attribute_clip.right =
        app->width + app->view.margin_x;
    app->view.top = app->clip.top = app->screen_clip.top = app->attribute_clip.top = -app->view.margin_y;
    app->view.bottom = app->clip.bottom = app->screen_clip.bottom = app->attribute_clip.bottom =
        app->height + app->view.margin_y;
    saved = app->render.draw;
    app->render.draw = queue;
    ok = bh_render_execute(&app->render);
    if (queue == &app->interpolation.queue)
        bh_interpolation_rendered(&app->interpolation);
    app->render.draw = saved;
    app->fade.pixels = app->display.color_pixels;
    app->fade.pitch = app->display.color_pitch;
    app->present.locked = app->display.locked;
    if (!bh_present_end(&app->present))
        ok = 0;
    app->display.locked = app->present.locked;
    if (!ok)
    {
        return failure(app, 13);
    }
    return 1;
}
/* Simulation and audio run at 30 Hz. Render copies interpolate between the
   last two completed ticks; all callback mutations stay in the render copy. */
static int manual_loop(BHGameApp *app)
{
    BHSimulationClock clock;
    unsigned long i, ticks, tick;
    int produce, focused;
#ifdef DEBUGGING
    uint64_t simulation_start, render_start, present_start, frame_end;
#endif
    memset(&clock, 0, sizeof(clock));
#ifdef DEBUGGING
    if (!perf_start(app))
        return 0;
#endif
    if (app->frames.produce != source_produce || app->frames.produce_user != app ||
        app->frames.bytes != 3500UL * sizeof(BHDrawCommand) || !bh_draw_queue_dispose(&app->queue))
        return failure(app, 12);

    app->service.input = &app->collection;
    app->service.entities = &app->pool;
    app->service.sound = &app->sound;
    app->service.frames = &app->frames;
    app->service.draw = &app->queue;
    bh_interpolation_bind(&app->queue);
    for (i = 0; i < 3; ++i)
        app->service.slot_draw[i] = app->slots + i;
    app->service.player_input = &app->global_state;
    app->service.paused = &app->player_state.paused;
    app->service.frame_counter = &app->frame;
    app->text_capture.input = app->bootstrap.all_keys;
    app->key_capture.input = app->bootstrap.all_keys;

    while (!app->quit && !app->error)
    {
        if (!bh_sdl_poll())
            app->quit = 1;
        if (app->quit)
            break;
        if (!network_poll(app))
            break;
        focused = bh_sdl_active();
        if (!focused)
            app->focus_sample = 0;
        if (!focused && !app->network_game.host)
        {
            clock.ready = 0;
            app->interpolation.ready = 0;
#ifdef DEBUGGING
            app->perf_previous = 0;
#endif
            bh_sdl_delay(20);
            continue;
        }
        if (focused && !app->focus_sample)
        {
            clock.ready = 0;
            app->interpolation.ready = 0;
#ifdef DEBUGGING
            app->perf_previous = 0;
#endif
            if (!app->collection.disable_mouse_motion && !bh_sdl_warp(160, 100))
                return failure(app, 12);
            memset(app->snapshot.mouse_axes, 0, sizeof(app->snapshot.mouse_axes));
            memset(app->snapshot.pair4, 0, sizeof(app->snapshot.pair4));
            app->collection.delta[0] = app->collection.delta[1] = 0;
            app->focus_sample = 1;
        }
        ticks = bh_simulation_advance(&clock, bh_sdl_seconds());
        produce = ticks != 0;
#ifdef DEBUGGING
        if (app->perf_enabled)
            simulation_start = bh_sdl_nanoseconds();
#endif
        for (tick = 0; tick < ticks; ++tick)
        {
            app->flyby.input_profile = app->profile;
            app->menu_settings.input_profile = app->profile;
            app->player_create.profile = app->profile;
            app->actor_damage.mode = app->player_state.mode;
            network_tick(app);
            app->eye = app->flyby_state.camera.position;
            app->angles = app->flyby_state.camera.angles;
            app->contacts.has_camera = 1;
            app->contacts.camera_yaw = app->yaw = app->angles.x;
            app->frames.produce(app->frames.produce_user);
            trace_tick(app);
            if (app->error)
                break;
            bh_interpolation_capture(&app->interpolation, app->slots + app->frames.latest,
                                     app->player_state.mode * 100 + app->mission);
        }
        if (app->player_tick.error || app->player_query.error || app->vm.error || app->lifetime.timers.error ||
            app->menu.error || app->flyby.error || app->actor_construct.error || app->actor_response.error)
        {
            failure(app, 12);
            break;
        }
#ifdef DEBUGGING
        if (app->perf_enabled)
            render_start = bh_sdl_nanoseconds();
#endif

        {
            int render_ok;

            render_ok =
                render_frame(app, bh_interpolation_sample(&app->interpolation, (float)(clock.remainder * 30.0)));

            if (!render_ok)
                break;
        }
#ifdef DEBUGGING
        if (app->perf_enabled)
            present_start = bh_sdl_nanoseconds();
#endif
        if (app->fullscreen)
            if (app->target != app->render_mode.render &&
                IDirectDrawSurface_BltFast(app->render_mode.render, 0, 0, app->target, 0, DDBLTFAST_WAIT) != DD_OK)
            {
                failure(app, 13);
                break;
            }
        if (IDirectDrawSurface_Flip(app->primary, 0, DDFLIP_WAIT) != DD_OK)
        {
            failure(app, 13);
            break;
        }
#ifdef DEBUGGING
        if (app->perf_enabled)
        {
            frame_end = bh_sdl_nanoseconds();
            perf_record(app, produce, simulation_start, render_start, present_start, frame_end);
        }
#endif

        bh_sdl_pace();
    }
    return !app->error;
}
static void cleanup(BHGameApp *app)
{
    unsigned long i;
#ifdef DEBUGGING
    perf_save(app);
#endif
    for (i = 0; i < N(app->actor_queries); ++i)
        if (app->actor_queries[i].frames)
            bh_segment_query_dispose(app->actor_queries + i);
    if (app->hit_markers.pool)
        bh_hit_marker_dispose(&app->hit_markers);
    bh_loading_progress_remove(&app->loading_progress);
    bh_loading_images_dispose(&app->loading_images);
    /* No callback may survive its context. Quiesce all callbacks before releasing
      metadata; shared models/textures are released after their borrowers. */
    if (app->panel.entity)
        bh_message_panel_close(&app->panel);
    bh_flyby_scene_unbind(&app->flyby);
    bh_player_tick_unbind(&app->player_tick);
    bh_player_query_unbind(&app->player_query);
    if (app->physics.pool)
        bh_physics_step_dispose(&app->physics);
    bh_scene_bindings_dispose(&app->scene_bindings);
    for (i = 0; i < N(app->block_queries); ++i)
        if (app->block_queries[i].geometry)
            bh_segment_query_dispose(app->block_queries + i);
    if (app->pool.chunks)
    {
        bh_editor_world_dispose(&app->editor_world);
        while (app->pool.active)
            if (!bh_entity_release(&app->pool, app->pool.active))
                break;
        bh_weapon_factory_close(&app->weapons);
        bh_entities_dispose(&app->pool);
    }
    if (!app->pool.used && app->targets.slots)
    {
        for (i = 0; i < app->targets.capacity; ++i)
            app->targets.slots[i].body.entity = 0;
        bh_target_construct_dispose(&app->targets);
    }
    if (app->player_collision.bsp.allocator.release)
        bh_collision_model_dispose(&app->player_collision);
    bh_world_load_dispose_index(&app->world_load);
    bh_script_blocks_dispose(&app->blocks);
    bh_navigation_dispose(&app->navigation);
    bh_rail_paths_dispose(&app->rails);
    bh_text_queue_init(&app->text);
    for (i = 0; i < BH_BOOTSTRAP_MENUS; ++i)
        bh_menu_rows_dispose(&app->bootstrap.menus[i].object.rows);
    if (app->bootstrap.confirmation)
        bh_atlas_page_dispose(&app->atlas, app->bootstrap.confirmation);
    if (app->bootstrap.menu_font)
        bh_atlas_page_dispose(&app->atlas, app->bootstrap.menu_font);
    if (app->bootstrap.graffiti)
        bh_atlas_page_dispose(&app->atlas, app->bootstrap.graffiti);
    if (app->panel.page)
        bh_atlas_page_dispose(&app->atlas, app->panel.page);
    bh_atlas_context_dispose(&app->atlas);
    bh_render_resources_dispose(&app->resources);
    bh_horizon_color_dispose(&app->horizon_colors);
    bh_gdi_text_dispose(&app->gdi);
    bh_input_devices_dispose(&app->devices);
    bh_player_input_owner_dispose(&app->inputs);
    bh_config_dispose(&app->config);
    bh_cd_stop_close(&app->cd);
    bh_sound_queue_dispose(&app->sound);
    if (!app->queue.allocator.release)
        memset(&app->queue, 0, sizeof(app->queue));
    bh_interpolation_bind(0);
    bh_frame_buffers_dispose(&app->frames);
    bh_draw_queue_dispose(&app->queue);
    bh_fade_dispose(&app->fade);
    bh_render_material_dispose(&app->material);
    bh_render_mode_dispose(&app->render_mode);
    if (app->target)
        IDirectDrawSurface_Release(app->target);
    if (app->primary)
        IDirectDrawSurface_Release(app->primary);
    if (app->draw)
        IDirectDraw_Release(app->draw);
    bh_sdl_close();
    bh_vfile_shutdown();
    free(app->motion_bindings);
    free(app->world_responses);
    free(app->actor_rooms);
    free(app->city_ids);
    free(app->rooms);
    free(app->scheduled);
    free(app->scene_inputs);
    bh_binding_dispose(&app->world_binding_index, &app->binding_allocator);
    free(app->segment_room_index);
    free(app->segment_rooms);
    free(app->physics_worlds);
    free(app->reset_nodes);
}
int bh_game_app_run(const char *directory, const char *settings_path, int profile,
                    const char *network_address, unsigned short network_port)
{
    BHGameApp *app;
    int result;
    BHTextureImage *loaded;
    app = (BHGameApp *)calloc(1, sizeof(*app));
    if (!app)
        return 2;
    app->fullscreen = 1;
#ifdef DEBUGGING
    app->perf_enabled = profile;
#endif
    app->width = 640;
    app->height = 480;
    if (!directory || strlen(directory) > 450 || !settings_path || strlen(settings_path) >= sizeof(app->config_path))
    {
        free(app);
        return 2;
    }
    sprintf(app->archive, "%s/Bhunter.bin", directory);
    strcpy(app->config_path, settings_path);
    if (!bh_math_environment_enter(&app->math))
    {
        free(app);
        return 2;
    }

    if (!platform(app) || !source_owners(app))
        goto done;
    if (!bh_vfile_mount(app->archive))
    {
        failure(app, 2);
        goto done;
    }
    if (!loading_begin(app))
        goto done;
    if (!bh_sound_startup(&app->sound, &app->sound_stats))
    {
        failure(app, 6);
        goto done;
    }
    if (!bh_texture_set_path(&app->render_mode.engine.textures, "textures", 0) ||
        !bh_texture_policy_set(&app->render_mode.engine.textures, 2, 0) ||
        !bh_bootstrap_resources(&app->bootstrap, 5) || !bh_bootstrap_resources(&app->bootstrap, 6))
    {
        failure(app, 6);
        goto done;
    }
    if (!bh_game_bootstrap(&app->bootstrap))
    {
        failure(app, app->error ? app->error : 10);
        goto done;
    }
    {
        unsigned long i;

        for (i = 0; i < N(app->actor_models); ++i)
            if (app->actor_models[i] && !bh_render_resources_lookup(app->actor_models[i], &app->resources))
            {
                failure(app, 9);
                goto done;
            }
    }
    /* Original00479896: final archive display before material allocation. */
    if (!bh_loading_display(&app->loading_display, 1))
    {
        failure(app, 6);
        goto done;
    }
    /* Original00479200 calls material/texture allocation after004474c0 has
      loaded all assets. Before that, every class reference count is zero. */
    bh_render_material_initialize(&app->material, &app->render_mode.engine.textures, app->render_mode.engine.d3d);
    if (app->material.failures || (app->hardware && !app->material.material))
    {
        failure(app, 4);
        goto done;
    }
    if (app->hardware)
        for (loaded = app->render_mode.engine.textures.images; loaded; loaded = loaded->next)
            if (!loaded->format || !loaded->format->slots || !loaded->format->slots->handle)
            {
                failure(app, 4);
                goto done;
            }
    /* Original004798d5..004798f9: detach archive progress, initialize the
       producer buffers, then display twice. There is no logo wait/input loop. */
    bh_loading_progress_remove(&app->loading_progress);
    if (!bh_frame_buffers_start(&app->frames, source_produce, app, 30, 3500 * sizeof(BHDrawCommand)))
    {
        failure(app, 12);
        goto done;
    }
    if (!bh_loading_display(&app->loading_display, 1) || !bh_loading_display(&app->loading_display, 1))
    {
        failure(app, 6);
        goto done;
    }

    app->initialized = 1;
    app->slot = -1;
    if (getenv("BH_NETWORK_TRACE"))
        app->network_log = fopen(bh_sdl_settings_file("network.log"), "w");
    if (network_port && !network_start(app, app->menu.root, network_address, network_port))
    {
        bh_sdl_report_error("Could not open the multiplayer session. Check the IPv4 address and UDP port.");
        app->error = 2;
        goto done;
    }
    manual_loop(app);

done:
    result = app->error;
    if (result)
        report_failure(app);

    if (app->network_game.host)
    {
        network_trace(app);
        bh_remote_clear(&app->remote_messages);
        bh_network_close(&app->network_game);
    }
    if (app->network_log)
        fclose(app->network_log);
    free(app->network_positions.nodes);
    free(app->effect_indices);
    free(app->pulse_indices);
    cleanup(app);

    bh_math_environment_leave(&app->math);
    free(app);
    return result;
}
